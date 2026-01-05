/**
 * Mock Pico SDK for Unit Testing
 *
 * Provides stub implementations of Pico SDK functions
 * so code can be compiled and tested on the host machine.
 */

#ifndef MOCK_PICO_H
#define MOCK_PICO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Type definitions that Pico SDK provides
typedef unsigned int uint;
typedef struct i2c_inst { int dummy; } i2c_inst_t;

// ============================================================
// pico/stdlib.h mocks
// ============================================================

static inline void stdio_init_all(void) {}
static inline void sleep_ms(uint32_t ms) { (void)ms; }
static inline void sleep_us(uint64_t us) { (void)us; }

// ============================================================
// pico/time.h mocks
// ============================================================

typedef uint64_t absolute_time_t;

static uint32_t _mock_time_ms = 0;
static uint32_t _mock_time_us = 0;

static inline absolute_time_t get_absolute_time(void) {
    return (absolute_time_t)_mock_time_us;
}

static inline uint32_t to_ms_since_boot(absolute_time_t t) {
    return (uint32_t)(t / 1000);
}

static inline uint32_t time_us_32(void) {
    return _mock_time_us;
}

// Test helpers
static inline void mock_set_time_ms(uint32_t ms) {
    _mock_time_ms = ms;
    _mock_time_us = ms * 1000;
}

static inline void mock_advance_time_ms(uint32_t ms) {
    _mock_time_ms += ms;
    _mock_time_us += ms * 1000;
}

// ============================================================
// hardware/gpio.h mocks
// ============================================================

#define GPIO_OUT 1
#define GPIO_IN 0
#define GPIO_FUNC_PWM 4
#define GPIO_IRQ_EDGE_RISE 0x08

static uint32_t _mock_gpio_state = 0;
static uint32_t _mock_gpio_dir = 0;

static inline void gpio_init(uint gpio) { (void)gpio; }
static inline void gpio_set_dir(uint gpio, bool out) {
    if (out) _mock_gpio_dir |= (1 << gpio);
    else _mock_gpio_dir &= ~(1 << gpio);
}
static inline void gpio_put(uint gpio, bool value) {
    if (value) _mock_gpio_state |= (1 << gpio);
    else _mock_gpio_state &= ~(1 << gpio);
}
static inline bool gpio_get(uint gpio) {
    return (_mock_gpio_state >> gpio) & 1;
}
static inline void gpio_set_function(uint gpio, uint fn) { (void)gpio; (void)fn; }
static inline void gpio_pull_up(uint gpio) { (void)gpio; }
static inline void gpio_pull_down(uint gpio) { (void)gpio; }

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t events);
static inline void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events,
                                                       bool enabled, gpio_irq_callback_t cb) {
    (void)gpio; (void)events; (void)enabled; (void)cb;
}

// Test helpers
static inline void mock_gpio_set(uint gpio, bool value) {
    if (value) _mock_gpio_state |= (1 << gpio);
    else _mock_gpio_state &= ~(1 << gpio);
}

static inline void mock_gpio_reset(void) {
    _mock_gpio_state = 0;
    _mock_gpio_dir = 0;
}

// ============================================================
// hardware/i2c.h mocks
// ============================================================

typedef struct i2c_inst i2c_inst_t;
extern i2c_inst_t* i2c0;
extern i2c_inst_t* i2c1;

static uint8_t _mock_i2c_read_data[256];
static size_t _mock_i2c_read_len = 0;
static size_t _mock_i2c_read_idx = 0;
static int _mock_i2c_error = 0;

static inline int i2c_write_blocking(i2c_inst_t* i2c, uint8_t addr,
                                      const uint8_t* src, size_t len, bool nostop) {
    (void)i2c; (void)addr; (void)src; (void)nostop;
    if (_mock_i2c_error) return -1;
    return (int)len;
}

static inline int i2c_read_blocking(i2c_inst_t* i2c, uint8_t addr,
                                     uint8_t* dst, size_t len, bool nostop) {
    (void)i2c; (void)addr; (void)nostop;
    if (_mock_i2c_error) return -1;
    for (size_t i = 0; i < len && _mock_i2c_read_idx < _mock_i2c_read_len; i++) {
        dst[i] = _mock_i2c_read_data[_mock_i2c_read_idx++];
    }
    return (int)len;
}

// Test helpers
static inline void mock_i2c_set_read_data(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len && i < sizeof(_mock_i2c_read_data); i++) {
        _mock_i2c_read_data[i] = data[i];
    }
    _mock_i2c_read_len = len;
    _mock_i2c_read_idx = 0;
}

static inline void mock_i2c_set_error(int error) {
    _mock_i2c_error = error;
}

static inline void mock_i2c_reset(void) {
    _mock_i2c_read_len = 0;
    _mock_i2c_read_idx = 0;
    _mock_i2c_error = 0;
}

// ============================================================
// hardware/adc.h mocks
// ============================================================

static uint16_t _mock_adc_values[4] = {0};
static uint _mock_adc_input = 0;

static inline void adc_init(void) {}
static inline void adc_gpio_init(uint gpio) { (void)gpio; }
static inline void adc_select_input(uint input) { _mock_adc_input = input; }
static inline uint16_t adc_read(void) {
    return _mock_adc_values[_mock_adc_input % 4];
}

// Test helpers
static inline void mock_adc_set_value(uint input, uint16_t value) {
    _mock_adc_values[input % 4] = value;
}

// ============================================================
// hardware/pwm.h mocks
// ============================================================

static uint16_t _mock_pwm_levels[16] = {0};  // 8 slices × 2 channels
static uint16_t _mock_pwm_wrap[8] = {0};
static bool _mock_pwm_enabled[8] = {false};

static inline uint pwm_gpio_to_slice_num(uint gpio) { return gpio / 2; }
static inline void pwm_set_wrap(uint slice_num, uint16_t wrap) {
    _mock_pwm_wrap[slice_num] = wrap;
}
static inline void pwm_set_clkdiv(uint slice_num, float divider) {
    (void)slice_num; (void)divider;
}
static inline void pwm_set_enabled(uint slice_num, bool enabled) {
    _mock_pwm_enabled[slice_num] = enabled;
}
static inline void pwm_set_gpio_level(uint gpio, uint16_t level) {
    _mock_pwm_levels[gpio % 16] = level;
}

// Test helpers
static inline uint16_t mock_pwm_get_level(uint gpio) {
    return _mock_pwm_levels[gpio % 16];
}

static inline void mock_pwm_reset(void) {
    for (int i = 0; i < 16; i++) _mock_pwm_levels[i] = 0;
    for (int i = 0; i < 8; i++) {
        _mock_pwm_wrap[i] = 0;
        _mock_pwm_enabled[i] = false;
    }
}

// ============================================================
// hardware/watchdog.h mocks
// ============================================================

static bool _mock_watchdog_enabled = false;
static uint32_t _mock_watchdog_timeout = 0;
static uint32_t _mock_watchdog_updates = 0;

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug) {
    (void)pause_on_debug;
    _mock_watchdog_enabled = true;
    _mock_watchdog_timeout = delay_ms;
}

static inline void watchdog_update(void) {
    _mock_watchdog_updates++;
}

// Test helpers
static inline uint32_t mock_watchdog_get_updates(void) {
    return _mock_watchdog_updates;
}

static inline void mock_watchdog_reset(void) {
    _mock_watchdog_enabled = false;
    _mock_watchdog_timeout = 0;
    _mock_watchdog_updates = 0;
}

// ============================================================
// pico/multicore.h mocks
// ============================================================

typedef void (*multicore_func_t)(void);
static multicore_func_t _mock_core1_func = NULL;

static inline void multicore_launch_core1(void (*entry)(void)) {
    _mock_core1_func = entry;
    // Note: We don't actually call it in unit tests
}

// Hardware instances - use pointers to avoid multiple definition errors
static i2c_inst_t _i2c0_inst;
static i2c_inst_t _i2c1_inst;
#define i2c0 (&_i2c0_inst)
#define i2c1 (&_i2c1_inst)

// ============================================================
// Global mock reset
// ============================================================

static inline void mock_reset_all(void) {
    mock_set_time_ms(0);
    mock_gpio_reset();
    mock_i2c_reset();
    mock_pwm_reset();
    mock_watchdog_reset();
}

#endif // MOCK_PICO_H
