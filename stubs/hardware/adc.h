#ifndef STUB_HARDWARE_ADC_H
#define STUB_HARDWARE_ADC_H

#include <stdint.h>

typedef unsigned int uint;

typedef uint16_t adc_hw_t;

void adc_init(void);
void adc_gpio_init(uint gpio);
void adc_select_input(uint input);
uint16_t adc_read(void);

#endif // STUB_HARDWARE_ADC_H
