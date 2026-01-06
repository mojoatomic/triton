/**
 * pico_stubs.h - Stub definitions for IKOS static analysis
 *
 * These stubs allow IKOS to analyze Triton code without the actual
 * Pico SDK. Include this header when running static analysis.
 *
 * Usage: ikos -I/src/analysis -DIKOS_ANALYSIS /src/src/main.c
 */

#ifndef PICO_STUBS_H
#define PICO_STUBS_H

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
// typedef struct { int dummy; } absolute_time_t;  // Defined later

#define i2c0 ((i2c_inst_t*)0x40044000)
#define i2c1 ((i2c_inst_t*)0x40048000)
#define spi0 ((spi_inst_t*)0x4003c000)
#define spi1 ((spi_inst_t*)0x40040000)

/*===========================================================================
 * GPIO Functions
 *===========================================================================*/

#define GPIO_FUNC_I2C 3
#define GPIO_FUNC_SPI 1
#define GPIO_FUNC_PWM 4
#define GPIO_FUNC_PIO0 6
#define GPIO_FUNC_PIO1 7

#define GPIO_OUT 1
#define GPIO_IN 0

#define GPIO_IRQ_EDGE_RISE  0x08
#define GPIO_IRQ_EDGE_FALL  0x04
#define GPIO_IRQ_LEVEL_HIGH 0x02
#define GPIO_IRQ_LEVEL_LOW  0x01

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t events);

static inline void gpio_init(uint gpio) { (void)gpio; }
static inline void gpio_set_dir(uint gpio, bool out) { (void)gpio; (void)out; }
static inline void gpio_put(uint gpio, bool value) { (void)gpio; (void)value; }
static inline bool gpio_get(uint gpio) { (void)gpio; return false; }
static inline void gpio_set_function(uint gpio, uint fn) { (void)gpio; (void)fn; }
static inline void gpio_pull_up(uint gpio) { (void)gpio; }
static inline void gpio_pull_down(uint gpio) { (void)gpio; }
static inline void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled) {
    (void)gpio; (void)events; (void)enabled;
}
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
    return (int)len;  /* Success: return bytes written */
}

static inline int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr,
                                     uint8_t *dst, size_t len,
                                     bool nostop) {
    (void)i2c; (void)addr; (void)dst; (void)nostop;
    return (int)len;  /* Success: return bytes read */
}

/*===========================================================================
 * Time Functions
 *===========================================================================*/

static inline void sleep_ms(uint32_t ms) { (void)ms; }
static inline void sleep_us(uint64_t us) { (void)us; }
static inline uint32_t time_us_32(void) { return 0; }
static inline uint64_t time_us_64(void) { return 0; }

static inline absolute_time_t get_absolute_time(void) {
    absolute_time_t t = {0};
    return t;
}

static inline uint32_t to_ms_since_boot(absolute_time_t t) {
    (void)t;
    return 0;
}

static inline void busy_wait_us_32(uint32_t delay_us) { (void)delay_us; }
static inline void tight_loop_contents(void) { }

/*===========================================================================
 * Watchdog Functions
 *===========================================================================*/

static inline void watchdog_enable(uint32_t delay_ms, bool pause_on_debug) {
    (void)delay_ms; (void)pause_on_debug;
}

static inline void watchdog_update(void) { }

static inline bool watchdog_caused_reboot(void) { return false; }

static inline bool watchdog_enable_caused_reboot(void) { return false; }

/*===========================================================================
 * Multicore Functions
 *===========================================================================*/

typedef void (*core1_entry_t)(void);

static inline void multicore_launch_core1(core1_entry_t entry) {
    (void)entry;
}

static inline void multicore_reset_core1(void) { }

static inline bool multicore_fifo_rvalid(void) { return true; }

static inline bool multicore_fifo_wready(void) { return true; }

static inline void multicore_fifo_push_blocking(uint32_t data) {
    (void)data;
}

static inline uint32_t multicore_fifo_pop_blocking(void) {
    return 0;
}

/*===========================================================================
 * ADC Functions
 *===========================================================================*/

static inline void adc_init(void) { }
static inline void adc_gpio_init(uint gpio) { (void)gpio; }
static inline void adc_select_input(uint input) { (void)input; }
static inline uint16_t adc_read(void) { return 2048; } /* Mid-scale */

/*===========================================================================
 * PWM Functions
 *===========================================================================*/

typedef struct { int dummy; } pwm_config;

static inline uint pwm_gpio_to_slice_num(uint gpio) { (void)gpio; return 0; }
static inline uint pwm_gpio_to_channel(uint gpio) { (void)gpio; return 0; }
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
static inline void pwm_set_enabled(uint slice, bool enabled) {
    (void)slice; (void)enabled;
}

/*===========================================================================
 * Standard Library
 *===========================================================================*/

static inline void stdio_init_all(void) { }

/*===========================================================================
 * Critical Sections
 *===========================================================================*/

typedef struct {
    uint32_t lock;
} spin_lock_t;

typedef struct {
    spin_lock_t spin_lock;
} critical_section_t;

static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t status) { (void)status; }
static inline void critical_section_init(critical_section_t *cs) { (void)cs; }
static inline void critical_section_enter_blocking(critical_section_t *cs) { (void)cs; }
static inline void critical_section_exit(critical_section_t *cs) { (void)cs; }

/*===========================================================================
 * Hardware Registers (Memory-mapped I/O simulation)
 *===========================================================================*/

/* For IKOS, we simulate hardware registers as returning valid values */
#define hw_set_bits(addr, mask) ((void)0)
#define hw_clear_bits(addr, mask) ((void)0)
#define hw_xor_bits(addr, mask) ((void)0)

/*===========================================================================
 * PIO (Programmable I/O) Functions
 *===========================================================================*/

typedef struct {
    uint32_t dummy;
} pio_sm_config;

typedef enum {
    pio0 = 0,
    pio1 = 1
} PIO;

static inline pio_sm_config pio_get_default_sm_config(void) {
    pio_sm_config c = {0};
    return c;
}

static inline uint pio_add_program(PIO pio, const void *program) {
    (void)pio; (void)program;
    return 0;
}

static inline void pio_sm_init(PIO pio, uint sm, uint offset, const pio_sm_config *config) {
    (void)pio; (void)sm; (void)offset; (void)config;
}

static inline void pio_sm_set_enabled(PIO pio, uint sm, bool enabled) {
    (void)pio; (void)sm; (void)enabled;
}

static inline bool pio_sm_is_rx_fifo_empty(PIO pio, uint sm) {
    (void)pio; (void)sm;
    return true;
}

static inline uint32_t pio_sm_get_blocking(PIO pio, uint sm) {
    (void)pio; (void)sm;
    return 0;
}

/*===========================================================================
 * Timer Functions
 *===========================================================================*/

typedef int64_t absolute_time_t;

static inline absolute_time_t make_timeout_time_ms(uint32_t ms) {
    (void)ms;
    return 0;
}

static inline bool time_reached(absolute_time_t t) {
    (void)t;
    return false;
}

/*===========================================================================
 * UART Functions
 *===========================================================================*/

static inline void uart_init(void *uart, uint baudrate) {
    (void)uart; (void)baudrate;
}

static inline bool uart_is_readable(void *uart) {
    (void)uart;
    return false;
}

static inline char uart_getc(void *uart) {
    (void)uart;
    return 0;
}

static inline void uart_putc(void *uart, char c) {
    (void)uart; (void)c;
}

#endif /* IKOS_ANALYSIS */

#endif /* PICO_STUBS_H */
