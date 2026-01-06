/**
 * pwm_capture.pio.h - Stub for PIO generated header
 *
 * This file provides stubs for the PIO program that would normally be
 * generated from pwm_capture.pio during the build process.
 * Used for IKOS static analysis when the actual PIO compilation isn't available.
 */

#ifndef _PWM_CAPTURE_PIO_H
#define _PWM_CAPTURE_PIO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PIO program structure stub
struct pio_program {
    const uint16_t* instructions;
    uint8_t length;
    int8_t origin;
};
typedef struct pio_program pio_program_t;

// PIO configuration structure stub
typedef struct {
    uint32_t clkdiv;
    uint32_t pins_out_base;
    uint32_t pins_out_count;
    uint32_t pins_in_base;
    uint32_t pins_set_base;
    uint32_t pins_set_count;
    uint32_t pins_sideset_base;
    uint32_t pins_sideset_count;
    bool sideset_optional;
    bool sideset_pindirs;
    uint32_t exec_mov_status;
    uint32_t exec_mov_status_sel;
    uint32_t exec_mov_status_n;
    uint32_t exec_out_status;
    uint32_t exec_out_status_sel;
    uint32_t exec_out_status_n;
    uint32_t exec_jmp_pin;
    uint32_t exec_side_set;
    uint32_t exec_side_en;
    uint32_t exec_wrap_target;
    uint32_t exec_wrap;
} pio_sm_config;

// PWM capture program constants
#define pwm_capture_wrap_target 0
#define pwm_capture_wrap 7
#define pwm_capture_offset_start 0u

// PWM capture program stub
static const uint16_t pwm_capture_program_instructions[] = {
    0x0000, // Dummy instruction
};

static const struct pio_program pwm_capture_program = {
    .instructions = pwm_capture_program_instructions,
    .length = 1,
    .origin = -1,
};

// Function stubs
static inline pio_sm_config pwm_capture_program_get_default_config(uint offset) {
    (void)offset;
    pio_sm_config config = {0};
    config.clkdiv = 1.0f;
    return config;
}

static inline void pwm_capture_program_init(void* pio, uint sm, uint offset, uint pin) {
    (void)pio;
    (void)sm;
    (void)offset;
    (void)pin;
    // Stub implementation - do nothing
}

// Additional PIO-related stubs that might be needed
static inline void pio_sm_config_set_in_pins(pio_sm_config* config, uint pin_base) {
    (void)config;
    (void)pin_base;
}

static inline void pio_sm_config_set_jmp_pin(pio_sm_config* config, uint pin) {
    (void)config;
    (void)pin;
}

static inline void pio_sm_config_set_in_shift(pio_sm_config* config, bool shift_right, bool autopush, uint threshold) {
    (void)config;
    (void)shift_right;
    (void)autopush;
    (void)threshold;
}

static inline void pio_sm_config_set_clkdiv(pio_sm_config* config, float div) {
    (void)config;
    (void)div;
}

#ifdef __cplusplus
}
#endif

#endif // _PWM_CAPTURE_PIO_H