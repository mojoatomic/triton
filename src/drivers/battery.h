/**
 * RC Submarine Controller
 * battery.h - Battery monitor driver
 *
 * Power of 10 compliant
 */

#ifndef BATTERY_H
#define BATTERY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t battery_init(void);
uint16_t battery_read_mv(void);
bool battery_is_low(void);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_H
