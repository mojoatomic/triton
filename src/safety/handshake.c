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
 * Core 0 Functions
 *===========================================================================*/

HandshakeResult_t handshake_wait_for_core1(void) {
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    uint32_t stage_start = start_time;

    /*-----------------------------------------------------------------------
     * Stage 1: Wait for ALIVE signal
     * This proves Core 1 code is executing.
     * Should be nearly instant - just the first instruction of core1_main()
     *-----------------------------------------------------------------------*/

    display_boot_progress(BOOT_STAGE_CORE1, false);
    display_refresh();

    printf("HANDSHAKE: Waiting for Core 1 ALIVE...\n");

    uint32_t timeout = 0;
    while (!multicore_fifo_rvalid() && timeout < CORE1_ALIVE_TIMEOUT_MS) {
        sleep_ms(1);
        timeout++;
        watchdog_update();
    }

    if (timeout >= CORE1_ALIVE_TIMEOUT_MS) {
        s_timing.alive_ms = timeout;
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
        printf("HANDSHAKE: ALIVE timeout after %lu ms\n", timeout);
        display_fault(FAULT_CORE1_FAILED);
        display_refresh();
        return HANDSHAKE_ALIVE_TIMEOUT;
    }

    uint32_t alive_magic = multicore_fifo_pop_blocking();
    s_timing.alive_ms = to_ms_since_boot(get_absolute_time()) - stage_start;

    if (alive_magic != CORE1_ALIVE_MAGIC) {
        s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
        printf("HANDSHAKE: Bad ALIVE magic: 0x%08lX\n", alive_magic);
        display_fault(FAULT_CORE1_FAILED);
        display_refresh();
        return HANDSHAKE_ALIVE_BAD_MAGIC;
    }

    printf("HANDSHAKE: Core 1 ALIVE (%lu ms)\n", s_timing.alive_ms);
    display_boot_progress(BOOT_STAGE_CORE1, true);
    display_refresh();

    /*-----------------------------------------------------------------------
     * Stage 2: Wait for READY signal
     * Core 1 is now initializing sensors. This can take a while.
     * Update display with progress as Core 1 sends status.
     *-----------------------------------------------------------------------*/

    stage_start = to_ms_since_boot(get_absolute_time());
    printf("HANDSHAKE: Waiting for Core 1 READY...\n");

    timeout = 0;
    BootStage_t current_stage = BOOT_STAGE_PRESSURE;

    while (timeout < CORE1_READY_TIMEOUT_MS) {
        /* Check for message from Core 1 */
        if (multicore_fifo_rvalid()) {
            uint32_t msg = multicore_fifo_pop_blocking();

            /* Check for final status */
            if (msg == CORE1_READY_MAGIC) {
                s_timing.ready_ms = to_ms_since_boot(get_absolute_time()) - stage_start;
                s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;

                printf("HANDSHAKE: Core 1 READY (%lu ms)\n", s_timing.ready_ms);
                printf("HANDSHAKE: Total boot time: %lu ms\n", s_timing.total_ms);

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
                current_stage = (BootStage_t)msg;
                display_boot_progress(current_stage, false);
                display_refresh();
                printf("HANDSHAKE: Initializing %s...\n",
                       BOOT_STAGE_NAMES[current_stage]);
            }
        }

        sleep_ms(1);
        timeout++;
        watchdog_update();
    }

    /* Timeout waiting for READY */
    s_timing.ready_ms = timeout;
    s_timing.total_ms = to_ms_since_boot(get_absolute_time()) - start_time;
    printf("HANDSHAKE: READY timeout after %lu ms\n", timeout);
    display_fault(FAULT_INIT_TIMEOUT);
    display_refresh();
    return HANDSHAKE_READY_TIMEOUT;
}

HandshakeTiming_t handshake_get_timing(void) {
    return s_timing;
}

const char* handshake_result_str(HandshakeResult_t result) {
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

