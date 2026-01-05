/**
 * RC Submarine Controller
 * rc_input.h - RC input (PIO PWM capture)
 *
 * Power of 10 compliant
 */

#ifndef RC_INPUT_H
#define RC_INPUT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t last_valid_ms;
} RcInputStatus_t;

typedef struct {
    uint8_t channel_pio[RC_CHANNEL_COUNT];
    uint8_t channel_sm[RC_CHANNEL_COUNT];
    uint16_t program_offset[2];
    bool initialized;
} RcInputDebug_t;

error_t rc_input_init(void);
error_t rc_input_read(RcFrame_t* frame);
bool rc_input_is_valid(void);
uint32_t rc_input_get_last_valid_ms(void);

// Debug helpers (safe to call; returns zeros if uninitialized)
RcInputStatus_t rc_input_get_status(void);
RcInputDebug_t rc_input_get_debug(void);

#ifdef __cplusplus
}
#endif

#endif // RC_INPUT_H
