/**
 * RC Submarine Controller
 * handshake.h - Two-stage Core 1 startup handshake
 *
 * Power of 10 compliant
 *
 * Solves: Cold boot timeout failures (~5% failure rate)
 *
 * Stage 1: ALIVE - Core 1 code is executing (fast, <100ms)
 * Stage 2: READY - Core 1 is fully initialized (slow, <5000ms)
 *
 * This separates "is the core running?" from "are sensors ready?"
 * and prevents false emergency triggers during slow cold boots.
 */

#ifndef HANDSHAKE_H
#define HANDSHAKE_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

/*===========================================================================
 * Handshake Magic Numbers
 *===========================================================================*/

#define CORE1_ALIVE_MAGIC       0xC0DE0001  /* Core 1 code is executing */
#define CORE1_READY_MAGIC       0xC0DE1001  /* Core 1 fully initialized */
#define CORE1_INIT_FAILED       0xC0DEDEAD  /* Core 1 init failed */

/*===========================================================================
 * Timeout Configuration
 *===========================================================================*/

#define CORE1_ALIVE_TIMEOUT_MS  100     /* Should be nearly instant */
#define CORE1_READY_TIMEOUT_MS  5000    /* Sensors can be slow on cold boot */

/*===========================================================================
 * Handshake Result
 *===========================================================================*/

typedef enum {
    HANDSHAKE_OK,               /* Both stages succeeded */
    HANDSHAKE_ALIVE_TIMEOUT,    /* Core 1 code not executing */
    HANDSHAKE_ALIVE_BAD_MAGIC,  /* Core 1 sent wrong alive magic */
    HANDSHAKE_READY_TIMEOUT,    /* Core 1 stuck in initialization */
    HANDSHAKE_INIT_FAILED,      /* Core 1 reported init failure */
    HANDSHAKE_READY_BAD_MAGIC   /* Core 1 sent wrong ready magic */
} HandshakeResult_t;

/*===========================================================================
 * Handshake Timing (for diagnostics)
 *===========================================================================*/

typedef struct {
    uint32_t alive_ms;      /* Time to receive ALIVE magic */
    uint32_t ready_ms;      /* Time to receive READY magic */
    uint32_t total_ms;      /* Total handshake time */
} HandshakeTiming_t;

/*===========================================================================
 * Public Functions - Core 0 Side
 *===========================================================================*/

/**
 * Wait for Core 1 to complete handshake
 *
 * Call this from main() after multicore_launch_core1()
 * Feeds watchdog during wait to prevent timeout.
 * Updates display with boot progress.
 *
 * Returns: Handshake result
 */
HandshakeResult_t handshake_wait_for_core1(void);

/**
 * Get timing from last handshake
 * Useful for diagnosing cold boot issues
 */
HandshakeTiming_t handshake_get_timing(void);

/**
 * Convert handshake result to string
 */
const char* handshake_result_str(HandshakeResult_t result);

/*===========================================================================
 * Public Functions - Core 1 Side
 *===========================================================================*/

/**
 * Send ALIVE signal
 *
 * Call this as FIRST instruction in core1_main()
 * Must be called before any initialization.
 */
void handshake_send_alive(void);

/**
 * Send READY signal
 *
 * Call this after all initialization is complete.
 */
void handshake_send_ready(void);

/**
 * Send INIT_FAILED signal
 *
 * Call this if any initialization step fails.
 */
void handshake_send_failed(void);

#endif /* HANDSHAKE_H */
