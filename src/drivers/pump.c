/**
 * RC Submarine Controller
 * pump.c - Ballast pump driver
 *
 * Power of 10 compliant
 */

#include "pump.h"

#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#define PUMP_PWM_WRAP       999U
#define PUMP_PWM_CLKDIV     125.0f

static uint s_slice_num = 0U;
static bool s_initialized = false;

static uint16_t speed_to_level(int8_t speed) {
    P10_ASSERT(true);

    int16_t s = (int16_t)speed;
    if (s > 100) {
        s = 100;
    }
    if (s < -100) {
        s = -100;
    }

    const uint16_t mag = (s < 0) ? (uint16_t)(-s) : (uint16_t)s;
    return (uint16_t)((mag * PUMP_PWM_WRAP) / 100U);
}

error_t pump_init(void) {
    P10_ASSERT(!s_initialized);

    if (s_initialized) {
        return ERR_NONE;
    }

    // Direction pin
    gpio_init(PIN_PUMP_DIR);
    gpio_set_dir(PIN_PUMP_DIR, GPIO_OUT);
    gpio_put(PIN_PUMP_DIR, false);

    // PWM pin
    gpio_set_function(PIN_PUMP_PWM, GPIO_FUNC_PWM);
    s_slice_num = pwm_gpio_to_slice_num(PIN_PUMP_PWM);

    // Configure PWM: 125 MHz / (125 * 1000) = 1 kHz
    (void)PUMP_PWM_FREQ;
    pwm_set_wrap(s_slice_num, (uint16_t)PUMP_PWM_WRAP);
    pwm_set_clkdiv(s_slice_num, PUMP_PWM_CLKDIV);
    pwm_set_enabled(s_slice_num, true);

    pump_stop();

    s_initialized = true;
    return ERR_NONE;
}

void pump_set_speed(int8_t speed) {
    P10_ASSERT(s_initialized);

    if (!s_initialized) {
        return;
    }

    // Direction: high = fill, low = drain
    gpio_put(PIN_PUMP_DIR, (speed >= 0));

    // Duty cycle
    pwm_set_gpio_level(PIN_PUMP_PWM, speed_to_level(speed));
}

void pump_stop(void) {
    P10_ASSERT(PIN_PUMP_PWM != PIN_PUMP_DIR);

    pwm_set_gpio_level(PIN_PUMP_PWM, 0U);
}
