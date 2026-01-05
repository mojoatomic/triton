#ifndef STUB_PICO_STDLIB_H
#define STUB_PICO_STDLIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void stdio_init_all(void);
void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);

#ifdef __cplusplus
}
#endif

#endif // STUB_PICO_STDLIB_H
