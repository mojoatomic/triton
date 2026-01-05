/**
 * RC Submarine Controller
 * servo.c - Servo output driver
 *
 * Power of 10 compliant
 */

#include "servo.h"

#include "config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define SERVO_PWM_WRAP   19999U
#define SERVO_PWM_CLKDIV 125.0f

static const uint s_servo_pins[SERVO_COUNT] = {
    PIN_SERVO_RUDDER,
    PIN_SERVO_BOWPLANE,
    PIN_SERVO_STERNPLANE
};

static bool s_initialized = false;

static uint16_t position_to_pulse_us(int8_t position) {
    P10_ASSERT(SERVO_PWM_MAX > SERVO_PWM_MIN);

    int16_t p = (int16_t)position;
    if (p > 100) {
        p = 100;
    }
    if (p < -100) {
        p = -100;
    }

    int32_t pulse = (int32_t)SERVO_PWM_CENTER + ((int32_t)p * 5);
    if (pulse < (int32_t)SERVO_PWM_MIN) {
        pulse = (int32_t)SERVO_PWM_MIN;
    }
    if (pulse > (int32_t)SERVO_PWM_MAX) {
        pulse = (int32_t)SERVO_PWM_MAX;
    }

    return (uint16_t)pulse;
}

error_t servo_init(void) {
    P10_ASSERT(!s_initialized);

    if (s_initialized) {
        return ERR_NONE;
    }

    for (uint i = 0; i < (uint)SERVO_COUNT; i++) {
        gpio_set_function(s_servo_pins[i], GPIO_FUNC_PWM);

        const uint slice = pwm_gpio_to_slice_num(s_servo_pins[i]);
        pwm_set_wrap(slice, (uint16_t)SERVO_PWM_WRAP);
        pwm_set_clkdiv(slice, SERVO_PWM_CLKDIV);
        pwm_set_enabled(slice, true);

        pwm_set_gpio_level(s_servo_pins[i], (uint16_t)SERVO_PWM_CENTER);
    }

    s_initialized = true;
    return ERR_NONE;
}

void servo_set_position(ServoChannel_t channel, int8_t position) {
    P10_ASSERT(channel < SERVO_COUNT);
    P10_ASSERT(s_initialized);

    if ((channel >= SERVO_COUNT) || (!s_initialized)) {
        return;
    }

    pwm_set_gpio_level(s_servo_pins[channel], position_to_pulse_us(position));
}
