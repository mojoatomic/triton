/**
 * RC Submarine Controller
 * pump.h - Ballast pump driver
 *
 * Power of 10 compliant
 */

#ifndef PUMP_H
#define PUMP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t pump_init(void);
void pump_set_speed(int8_t speed);  // -100 to +100
void pump_stop(void);

#ifdef __cplusplus
}
#endif

#endif // PUMP_H
