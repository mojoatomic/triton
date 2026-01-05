/**
 * RC Submarine Controller
 * rc_input.c - RC input (PIO PWM capture)
 *
 * Power of 10 compliant
 */

#include "rc_input.h"

#include "config.h"

#include "hardware/pio.h"
#include "pico/time.h"

#include "pwm_capture.pio.h"

#define RC_FRAME_MAX_AGE_MS 100U
#define RC_PIO_COUNT 2U
#define PIO_INDEX(pio) (((pio) == pio1) ? 1U : 0U)
#define RC_NOW_MS() (to_ms_since_boot(get_absolute_time()))

static PIO g_channel_pio[RC_CHANNEL_COUNT];
static uint8_t g_channel_sm[RC_CHANNEL_COUNT];
static uint g_program_offset[RC_PIO_COUNT];
static bool g_program_loaded[RC_PIO_COUNT];
static bool g_initialized = false;

static uint16_t g_last_pulse_us[RC_CHANNEL_COUNT];
static uint32_t g_last_update_ms[RC_CHANNEL_COUNT];
static uint32_t g_last_valid_ms = 0U;

#define RC_PULSE_VALID(pulse_us) (((pulse_us) >= RC_PWM_MIN) && ((pulse_us) <= RC_PWM_MAX))

static error_t init_channel(uint ch, uint pin, PIO pio) {
    P10_ASSERT(ch < RC_CHANNEL_COUNT);
    P10_ASSERT(pio != NULL);

    const uint8_t pio_idx = PIO_INDEX(pio);

    if (!g_program_loaded[pio_idx]) {
        g_program_offset[pio_idx] = pio_add_program(pio, &pwm_capture_program);
        g_program_loaded[pio_idx] = true;
    }

    const int sm = pio_claim_unused_sm(pio, false);
    if (sm < 0) {
        return ERR_HARDWARE;
    }

    g_channel_pio[ch] = pio;
    g_channel_sm[ch] = (uint8_t)sm;

    pwm_capture_program_init(pio, (uint)sm, g_program_offset[pio_idx], pin);

    g_last_pulse_us[ch] = (uint16_t)RC_PWM_CENTER;
    g_last_update_ms[ch] = 0U;

    return ERR_NONE;
}

error_t rc_input_init(void) {
    P10_ASSERT(RC_CHANNEL_COUNT == 6);

    if (g_initialized) {
        return ERR_NONE;
    }

    static const uint pins[RC_CHANNEL_COUNT] = {
        PIN_RC_CH1, PIN_RC_CH2, PIN_RC_CH3,
        PIN_RC_CH4, PIN_RC_CH5, PIN_RC_CH6
    };

    for (uint ch = 0; ch < RC_CHANNEL_COUNT; ch++) {
        const PIO pio = (ch < 4U) ? pio0 : pio1;
        const error_t err = init_channel(ch, pins[ch], pio);
        if (err != ERR_NONE) {
            for (uint i = 0; i < ch; i++) {
                pio_sm_unclaim(g_channel_pio[i], (uint)g_channel_sm[i]);
            }
            return err;
        }
    }

    g_last_valid_ms = 0U;
    g_initialized = true;

    return ERR_NONE;
}

error_t rc_input_read(RcFrame_t* frame) {
    P10_ASSERT(frame != NULL);
    P10_ASSERT(g_initialized);

    const uint32_t now = RC_NOW_MS();
    bool all_valid = true;

    for (uint ch = 0; ch < RC_CHANNEL_COUNT; ch++) {
        const PIO pio = g_channel_pio[ch];
        const uint sm = (uint)g_channel_sm[ch];
        uint32_t pulse_us = 0U;
        bool have_new = false;

        for (uint flush = 0; flush < 4U; flush++) {
            if (pio_sm_is_rx_fifo_empty(pio, sm)) {
                break;
            }
            pulse_us = pio_sm_get_blocking(pio, sm);
            have_new = true;
        }

        if (have_new) {
            if (RC_PULSE_VALID(pulse_us)) {
                g_last_pulse_us[ch] = (uint16_t)pulse_us;
                g_last_update_ms[ch] = now;
            } else {
                all_valid = false;
            }
        }

        if ((g_last_update_ms[ch] != 0U) && ((now - g_last_update_ms[ch]) <= RC_FRAME_MAX_AGE_MS)) {
            frame->channels[ch] = g_last_pulse_us[ch];
        } else {
            all_valid = false;
            frame->channels[ch] = (uint16_t)RC_PWM_CENTER;
        }
    }

    frame->timestamp_ms = now;
    frame->valid = all_valid;

    if (all_valid) {
        g_last_valid_ms = now;
    }

    return ERR_NONE;
}

bool rc_input_is_valid(void) {
    P10_ASSERT(RC_CHANNEL_COUNT == 6);

    const uint32_t now = RC_NOW_MS();
    return (g_last_valid_ms != 0U) && ((now - g_last_valid_ms) < SIGNAL_TIMEOUT_MS);
}

uint32_t rc_input_get_last_valid_ms(void) {
    P10_ASSERT(RC_CHANNEL_COUNT == 6);

    return g_last_valid_ms;
}

RcInputStatus_t rc_input_get_status(void) {
    P10_ASSERT(RC_CHANNEL_COUNT == 6);

    RcInputStatus_t s;
    s.last_valid_ms = g_last_valid_ms;
    return s;
}

RcInputDebug_t rc_input_get_debug(void) {
    P10_ASSERT(RC_CHANNEL_COUNT == 6);

    RcInputDebug_t d;

    d.initialized = g_initialized;

    for (uint ch = 0; ch < RC_CHANNEL_COUNT; ch++) {
        d.channel_pio[ch] = PIO_INDEX(g_channel_pio[ch]);
        d.channel_sm[ch] = g_channel_sm[ch];
    }

    for (uint i = 0; i < RC_PIO_COUNT; i++) {
        d.program_offset[i] = (uint16_t)g_program_offset[i];
    }

    return d;
}
