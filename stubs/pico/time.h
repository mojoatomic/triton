#ifndef STUB_PICO_TIME_H
#define STUB_PICO_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t absolute_time_t;

absolute_time_t get_absolute_time(void);
uint32_t to_ms_since_boot(absolute_time_t t);

#ifdef __cplusplus
}
#endif

#endif // STUB_PICO_TIME_H
