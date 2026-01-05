#ifndef STUB_PWM_CAPTURE_PIO_H
#define STUB_PWM_CAPTURE_PIO_H

#include "hardware/pio.h"

typedef unsigned int uint;

extern const pio_program_t pwm_capture_program;

void pwm_capture_program_init(PIO pio, uint sm, uint offset, uint pin);

#endif // STUB_PWM_CAPTURE_PIO_H
