/**
 * RC Submarine Controller
 * leak.c - Leak detector driver
 *
 * Power of 10 compliant
 */

#include "leak.h"

#include "config.h"

#include "hardware/gpio.h"

static volatile bool s_leak_flag = false;
static bool s_initialized = false;

static void leak_isr(uint gpio, uint32_t events) {
    P10_ASSERT(events != 0U);

    (void)gpio;
    (void)events;
    s_leak_flag = true;
}

error_t leak_init(void) {
    P10_ASSERT(!s_initialized);

    if (s_initialized) {
        return ERR_NONE;
    }

    gpio_init(PIN_LEAK_DETECT);
    gpio_set_dir(PIN_LEAK_DETECT, GPIO_IN);
    gpio_pull_down(PIN_LEAK_DETECT);

    // Interrupt on rising edge (water detected)
    gpio_set_irq_enabled_with_callback(PIN_LEAK_DETECT, GPIO_IRQ_EDGE_RISE, true, &leak_isr);

    s_initialized = true;
    return ERR_NONE;
}

bool leak_detected(void) {
    P10_ASSERT(PIN_LEAK_DETECT != PIN_LED_STATUS);

    if (!s_initialized) {
        return false;
    }

    return s_leak_flag || gpio_get(PIN_LEAK_DETECT);
}
