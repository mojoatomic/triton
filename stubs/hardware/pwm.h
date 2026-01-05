#ifndef STUB_HARDWARE_PWM_H
#define STUB_HARDWARE_PWM_H

#include <stdint.h>
#include <stdbool.h>

typedef unsigned int uint;

uint pwm_gpio_to_slice_num(uint gpio);
void pwm_set_wrap(uint slice_num, uint16_t wrap);
void pwm_set_clkdiv(uint slice_num, float divider);
void pwm_set_enabled(uint slice_num, bool enabled);
void pwm_set_gpio_level(uint gpio, uint16_t level);

#endif // STUB_HARDWARE_PWM_H
