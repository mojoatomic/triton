/**
 * RC Submarine Controller
 * valve.c - Solenoid valve driver
 *
 * Power of 10 compliant
 */

#include "valve.h"

#include "config.h"

#include "hardware/gpio.h"

static bool s_initialized = false;
static bool s_is_open = false;

error_t valve_init(void) {
    P10_ASSERT(!s_initialized);

    if (s_initialized) {
        return ERR_NONE;
    }

    gpio_init(PIN_VALVE);
    gpio_set_dir(PIN_VALVE, GPIO_OUT);

    valve_close();

    s_initialized = true;
    return ERR_NONE;
}

void valve_open(void) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return;
    }

    gpio_put(PIN_VALVE, true);
    s_is_open = true;
}

void valve_close(void) {
    P10_ASSERT(PIN_VALVE != PIN_LED_STATUS);

    gpio_put(PIN_VALVE, false);
    s_is_open = false;
}

bool valve_is_open(void) {
    P10_ASSERT(PIN_VALVE != PIN_LED_STATUS);

    if (!s_initialized) {
        return false;
    }

    return s_is_open;
}
