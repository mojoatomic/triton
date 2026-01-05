/**
 * RC Submarine Controller
 * battery.c - Battery monitor driver
 *
 * Power of 10 compliant
 */

#include "battery.h"

#include "config.h"

#include "hardware/adc.h"

static bool s_initialized = false;

error_t battery_init(void) {
    P10_ASSERT(!s_initialized);

    if (s_initialized) {
        return ERR_NONE;
    }

    adc_init();
    adc_gpio_init(PIN_BATTERY_ADC);

    s_initialized = true;
    return ERR_NONE;
}

uint16_t battery_read_mv(void) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return 0U;
    }

    // ADC0 = GPIO26
    const uint adc_input = (uint)(PIN_BATTERY_ADC - 26);
    adc_select_input(adc_input);

    // 12-bit: 0-4095
    const uint16_t raw = adc_read();

    // Convert to mV at ADC pin: 3300 mV / 4096 * raw
    const uint32_t adc_mv = ((uint32_t)raw * 3300U) / 4096U;

    // Apply voltage divider correction
    const uint32_t batt_mv = (adc_mv * (uint32_t)BATTERY_DIVIDER_MULT) / (uint32_t)BATTERY_DIVIDER_DIV;

    return (uint16_t)batt_mv;
}

bool battery_is_low(void) {
    P10_ASSERT(MIN_BATTERY_MV > 0);

    if (!s_initialized) {
        return true;
    }

    return battery_read_mv() < (uint16_t)MIN_BATTERY_MV;
}
