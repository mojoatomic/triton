/**
 * RC Submarine Controller
 * leak.h - Leak detector driver
 *
 * Power of 10 compliant
 */

#ifndef LEAK_H
#define LEAK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t leak_init(void);
bool leak_detected(void);

#ifdef __cplusplus
}
#endif

#endif // LEAK_H
