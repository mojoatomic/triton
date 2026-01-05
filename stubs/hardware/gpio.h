#ifndef STUB_HARDWARE_GPIO_H
#define STUB_HARDWARE_GPIO_H

#include <stdint.h>
#include <stdbool.h>

typedef unsigned int uint;

enum {
    GPIO_OUT = 1,
    GPIO_IN = 0,
    GPIO_FUNC_PWM = 4,
};

#define GPIO_IRQ_EDGE_RISE 0x08u

typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t events);

void gpio_init(uint gpio);
void gpio_set_dir(uint gpio, bool out);
void gpio_put(uint gpio, bool value);
bool gpio_get(uint gpio);
void gpio_set_function(uint gpio, uint fn);
void gpio_pull_up(uint gpio);
void gpio_pull_down(uint gpio);
void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events, bool enabled, gpio_irq_callback_t cb);

#endif // STUB_HARDWARE_GPIO_H
