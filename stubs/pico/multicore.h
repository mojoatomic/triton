#ifndef STUB_PICO_MULTICORE_H
#define STUB_PICO_MULTICORE_H

#ifdef __cplusplus
extern "C" {
#endif

void multicore_launch_core1(void (*entry)(void));

#ifdef __cplusplus
}
#endif

#endif // STUB_PICO_MULTICORE_H
