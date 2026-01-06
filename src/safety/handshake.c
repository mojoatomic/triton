/**
 * RC Submarine Controller
 * handshake.c - Two-stage Core 1 startup handshake
 *
 * Power of 10 compliant
 */

#include "handshake.h"
#include "display.h"
#include "config.h"
#include "util/log.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/watchdog.h"
#include <stdio.h>

/*===========================================================================
 * Static Data
 *===========================================================================*/

// Boot stage names for display
static const char* BOOT_STAGE_NAMES[] = {
    "Core 1",
    "Pressure sensor",
    "IMU sensor",
    "RC input",
    "Battery monitor",
    "Leak detector",
    "Complete"
};

static HandshakeTiming_t s_timing = {0};

/*===========================================================================
 * Private Helper Functions
 *===========================================================================*/

/**
 * Wait for FIFO to have data with bounded timeout
 * Returns true if data available, false on timeout
 */
static bool wait_for_fifo(uint32_t timeout_ms, uint32_t* elapsed_ms) {
    P10_ASSERT(elapsed_ms != NULL);
    P10_ASSERT(timeout_ms <= 10000);  // Max 10 second timeout

    *elapsed_ms = 0;

    for (uint32_t i = 0; i < timeout_ms; i++) {
        if (multicore_fifo_rvalid()) {
            *elapsed_ms = i;
            return true;
        }
        sleep_ms(1);
        watchdog_update();
    }

    *elapsed_ms = timeout_ms;
    return false;
}

/**
 * Stage 1: Wait for ALIVE signal from Core 1
 * Proves Core 1 code is executing.
 */
static HandshakeResult_t wait_for_alive(uint32_t start_time) {
    P10_ASSERT(start_time > 0);

    uint32_t stage_start = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = 0;

    display_boot_progress(BOOT_STAGE_CORE1, false);
    display_refresh();

    printf("HANDSHAKE: Waiting for Core 1 ALIVE...\n");

    if (!wait_for_fifo(CORE1_ALIVE_TIMEOUT_MS, &elapsed)) {
        s_timing.alive_ms = elapsed;
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
        printf("HANDSHAKE: ALIVE timeout after %lu ms\n", (unsigned long)elapsed);
        display_fault(FAULT_CORE1_FAILED);
        display_refresh();
        return HANDSHAKE_ALIVE_TIMEOUT;
    }

    uint32_t alive_magic = multicore_fifo_pop_blocking();
    s_timing.alive_ms = to_ms_since_boot(get_absolute_time()) - stage_start;

    if (alive_magic != CORE1_ALIVE_MAGIC) {
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
        printf("HANDSHAKE: Bad ALIVE magic: 0x%08lX\n", (unsigned long)alive_magic);
        display_fault(FAULT_CORE1_FAILED);
        display_refresh();
        return HANDSHAKE_ALIVE_BAD_MAGIC;
    }

    printf("HANDSHAKE: Core 1 ALIVE (%lu ms)\n", (unsigned long)s_timing.alive_ms);
    display_boot_progress(BOOT_STAGE_CORE1, true);
    display_refresh();

    return HANDSHAKE_OK;
}

/**
 * Process a single message from Core 1 during READY wait
 * Returns result code, or HANDSHAKE_OK to continue waiting
 */
static HandshakeResult_t process_ready_message(uint32_t msg, uint32_t start_time,
                                                uint32_t stage_start) {
    P10_ASSERT(start_time > 0);
    P10_ASSERT(stage_start >= start_time);

    if (msg == CORE1_READY_MAGIC) {
        s_timing.ready_ms = to_ms_since_boot(get_absolute_time()) - stage_start;
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;

        printf("HANDSHAKE: Core 1 READY (%lu ms)\n", (unsigned long)s_timing.ready_ms);
        printf("HANDSHAKE: Total boot time: %lu ms\n", (unsigned long)s_timing.total_ms);

        display_boot_progress(BOOT_STAGE_COMPLETE, true);
        display_refresh();
        return HANDSHAKE_OK;
    }

    if (msg == CORE1_INIT_FAILED) {
        s_timing.ready_ms = to_ms_since_boot(get_absolute_time()) - stage_start;
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
        printf("HANDSHAKE: Core 1 reported init failure\n");
        return HANDSHAKE_INIT_FAILED;
    }

    /* Progress updates from Core 1 (stage numbers) */
    if (msg >= BOOT_STAGE_PRESSURE && msg < BOOT_STAGE_COMPLETE) {
        BootStage_t current_stage = (BootStage_t)msg;
        display_boot_progress(current_stage, false);
        display_refresh();
        printf("HANDSHAKE: Initializing %s...\n", BOOT_STAGE_NAMES[current_stage]);
    }

    /* Return a sentinel value to indicate "keep waiting" */
    return (HandshakeResult_t)0xFF;
}

/**
 * Stage 2: Wait for READY signal from Core 1
 * Core 1 initializes sensors and sends progress updates.
 */
static HandshakeResult_t wait_for_ready(uint32_t start_time) {
    P10_ASSERT(start_time > 0);

    uint32_t stage_start = to_ms_since_boot(get_absolute_time());

    printf("HANDSHAKE: Waiting for Core 1 READY...\n");

    for (uint32_t timeout = 0; timeout < CORE1_READY_TIMEOUT_MS; timeout++) {
        if (multicore_fifo_rvalid()) {
            uint32_t msg = multicore_fifo_pop_blocking();
            HandshakeResult_t result = process_ready_message(msg, start_time, stage_start);

            /* 0xFF means keep waiting, anything else is final result */
            if (result != (HandshakeResult_t)0xFF) {
                return result;
            }
        }

        sleep_ms(1);
        watchdog_update();
    }

    /* Timeout waiting for READY */
    s_timing.ready_ms = CORE1_READY_TIMEOUT_MS;
    s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
    printf("HANDSHAKE: READY timeout after %lu ms\n", (unsigned long)CORE1_READY_TIMEOUT_MS);
    display_fault(FAULT_INIT_TIMEOUT);
    display_refresh();
    return HANDSHAKE_READY_TIMEOUT;
}

/*===========================================================================
 * Core 0 Public Functions
 *===========================================================================*/

HandshakeResult_t handshake_wait_for_core1(void) {
    P10_ASSERT(CORE1_ALIVE_TIMEOUT_MS > 0);
    P10_ASSERT(CORE1_READY_TIMEOUT_MS > 0);

    uint32_t start_time = to_ms_since_boot(get_absolute_time());

    /* Stage 1: Wait for ALIVE */
    HandshakeResult_t result = wait_for_alive(start_time);
    if (result != HANDSHAKE_OK) {
        return result;
    }

    /* Stage 2: Wait for READY */
    return wait_for_ready(start_time);
}

HandshakeTiming_t handshake_get_timing(void) {
    return s_timing;
}

const char* handshake_result_str(HandshakeResult_t result) {
    P10_ASSERT(result <= HANDSHAKE_READY_BAD_MAGIC || result == (HandshakeResult_t)0xFF);

    switch (result) {
        case HANDSHAKE_OK:              return "OK";
        case HANDSHAKE_ALIVE_TIMEOUT:   return "ALIVE timeout";
        case HANDSHAKE_ALIVE_BAD_MAGIC: return "ALIVE bad magic";
        case HANDSHAKE_READY_TIMEOUT:   return "READY timeout";
        case HANDSHAKE_INIT_FAILED:     return "Init failed";
        case HANDSHAKE_READY_BAD_MAGIC: return "READY bad magic";
        default:                        return "Unknown";
    }
}

/*===========================================================================
 * Core 1 Functions
 *===========================================================================*/

void handshake_send_alive(void) {
    multicore_fifo_push_blocking(CORE1_ALIVE_MAGIC);
}

void handshake_send_ready(void) {
    multicore_fifo_push_blocking(CORE1_READY_MAGIC);
}

void handshake_send_failed(void) {
    multicore_fifo_push_blocking(CORE1_INIT_FAILED);
}
