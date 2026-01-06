/**
 * pico_stubs_fixed.h - Clean Pico SDK stubs for IKOS analysis
 */

#ifndef PICO_STUBS_FIXED_H
#define PICO_STUBS_FIXED_H

#ifdef IKOS_ANALYSIS

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*===========================================================================
 * Basic Types
 *===========================================================================*/

typedef unsigned int uint;
typedef struct { int dummy; } i2c_inst_t;
typedef struct { int dummy; } spi_inst_t;
typedef struct { int dummy; } pio_hw_t;
typedef int64_t absolute_time_t;
typedef struct { int dummy; } pwm_config;
typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t events);

#define i2c0 ((i2c_inst_t*)0x40044000)
#define i2c1 ((i2c_inst_t*)0x40048000)

/*===========================================================================
 * GPIO Definitions
 *===========================================================================*/

#define GPIO_OUT 1
#define GPIO_IN 0
#define GPIO_FUNC_I2C 3
#define GPIO_FUNC_PWM 4
#define GPIO_IRQ_EDGE_RISE  0x08

/*===========================================================================
 * GPIO Functions
 *===========================================================================*/

static inline void gpio_init(uint gpio) { (void)gpio; }
static inline void gpio_set_dir(uint gpio, bool out) { (void)gpio; (void)out; }
static inline void gpio_put(uint gpio, bool value) { (void)gpio; (void)value; }
static inline bool gpio_get(uint gpio) { (void)gpio; return false; }
static inline void gpio_set_function(uint gpio, uint fn) { (void)gpio; (void)fn; }
static inline void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events,
                                                     bool enabled, gpio_irq_callback_t callback) {
    (void)gpio; (void)events; (void)enabled; (void)callback;
}

/*===========================================================================
 * I2C Functions
 *===========================================================================*/

static inline uint i2c_init(i2c_inst_t *i2c, uint baudrate) {
    (void)i2c; (void)baudrate;
    return baudrate;
}

static inline int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr,
                                      const uint8_t *src, size_t len,
                                      bool nostop) {
    (void)i2c; (void)addr; (void)src; (void)nostop;
    return (int)len;
}

static inline int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr,
                                     uint8_t *dst, size_t len,
                                     bool nostop) {
    (void)i2c; (void)addr; (void)dst; (void)nostop;
    return (int)len;
}

/*===========================================================================
 * Time Functions
 *===========================================================================*/

static inline void sleep_ms(uint32_t ms) { (void)ms; }
static inline uint32_t time_us_32(void) { return 0; }

static inline absolute_time_t get_absolute_time(void) {
    return 0;
}

static inline uint32_t to_ms_since_boot(absolute_time_t t) {
    (void)t;
    return 0;
}

/*===========================================================================
 * ADC Functions
 *===========================================================================*/

static inline void adc_init(void) { }
static inline void adc_gpio_init(uint gpio) { (void)gpio; }
static inline void adc_select_input(uint input) { (void)input; }
static inline uint16_t adc_read(void) { return 2048; }

/*===========================================================================
 * PWM Functions
 *===========================================================================*/

static inline uint pwm_gpio_to_slice_num(uint gpio) { (void)gpio; return 0; }
static inline pwm_config pwm_get_default_config(void) {
    pwm_config c = {0};
    return c;
}
static inline void pwm_config_set_clkdiv(pwm_config *c, float div) {
    (void)c; (void)div;
}
static inline void pwm_config_set_wrap(pwm_config *c, uint16_t wrap) {
    (void)c; (void)wrap;
}
static inline void pwm_init(uint slice, pwm_config *c, bool start) {
    (void)slice; (void)c; (void)start;
}
static inline void pwm_set_gpio_level(uint gpio, uint16_t level) {
    (void)gpio; (void)level;
}

/*===========================================================================
 * Watchdog Functions
 *===========================================================================*/

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug) {
    (void)delay_ms; (void)pause_on_debug;
}
static inline void watchdog_update(void) { }

/*===========================================================================
 * Multicore Functions
 *===========================================================================*/

typedef void (*core1_entry_t)(void);

static inline void multicore_launch_core1(core1_entry_t entry) {
    (void)entry;
}
static inline void multicore_fifo_push_blocking(uint32_t data) {
    (void)data;
}
static inline uint32_t multicore_fifo_pop_blocking(void) {
    return 0;
}

/*===========================================================================
 * Standard Functions
 *===========================================================================*/

static inline void stdio_init_all(void) { }

/*===========================================================================
 * Critical Sections
 *===========================================================================*/

static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t status) { (void)status; }

/*===========================================================================
 * Missing Functions for Problematic Components
 *===========================================================================*/

// For handshake component
static inline void log_info(const char* msg, ...) { (void)msg; }
static inline void log_error(const char* msg, ...) { (void)msg; }

// Multicore functions for handshake
static inline bool multicore_fifo_rvalid(void) { return true; }

// Boot stage names for display and handshake - only define if not already defined
// This will be skipped if the source file defines its own BOOT_STAGE_NAMES
#ifndef BOOT_STAGE_NAMES_DEFINED
#define BOOT_STAGE_NAMES_DEFINED
static const char* const BOOT_STAGE_NAMES[] = {
    "INIT", "CORE1", "PRESSURE", "IMU", "RC",
    "BATTERY", "LEAK", "ACTUATORS", "COMPLETE", "ERROR"
};
#endif

// For display component
#define DISPLAY_I2C i2c0
#define DISPLAY_I2C_ADDR 0x3C

// For emergency component
static inline void panic(const char* msg) { (void)msg; }
static inline void watchdog_force_reset(void) { }

// For rc_input component - PIO and hardware stubs
typedef pio_hw_t* PIO;

#define pio0 ((PIO)0x50200000)
#define pio1 ((PIO)0x50300000)

static inline uint pio_add_program(PIO pio, const void* program) {
    (void)pio; (void)program; return 0;
}

static inline int pio_claim_unused_sm(PIO pio, bool required) {
    (void)pio; (void)required; return 0;
}

static inline void pio_sm_unclaim(PIO pio, uint sm) {
    (void)pio; (void)sm;
}

static inline bool pio_sm_is_rx_fifo_empty(PIO pio, uint sm) {
    (void)pio; (void)sm; return false;
}

static inline uint32_t pio_sm_get_blocking(PIO pio, uint sm) {
    (void)pio; (void)sm; return 1500; // Neutral RC pulse width
}

static inline void pio_sm_init(PIO pio, uint sm, uint initial_pc, const void* config) {
    (void)pio; (void)sm; (void)initial_pc; (void)config;
}

static inline void pio_sm_set_enabled(PIO pio, uint sm, bool enabled) {
    (void)pio; (void)sm; (void)enabled;
}

// For main component - State machine types and functions
typedef struct {
    int state;
    int mode;
} StateInfo_t;

#define MODE_DEPTH_HOLD 1
#define STATE_SUBMERGED 2

// Missing function declarations for main.c
static inline void state_machine_update(void* sm, void* inputs, void* depth, uint32_t now) {
    (void)sm; (void)inputs; (void)depth; (void)now;
}

static inline void ballast_ctrl_set_command(void* bc, int8_t cmd) {
    (void)bc; (void)cmd;
}

// Additional main.c functions that might be missing
// Note: state_machine_get_state has different signature in header, so don't define it here

static inline void sleep_us(uint64_t us) { (void)us; }
static inline void tight_loop_contents(void) { }

// ballast_ctrl_update removed - main.c includes real header which conflicts

// Handshake function implementations moved to main.c source section via test script

#endif /* IKOS_ANALYSIS */

#endif /* PICO_STUBS_FIXED_H */