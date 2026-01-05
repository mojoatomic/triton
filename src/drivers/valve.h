/**
 * RC Submarine Controller
 * valve.h - Solenoid valve driver
 *
 * Power of 10 compliant
 */

#ifndef VALVE_H
#define VALVE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t valve_init(void);
void valve_open(void);
void valve_close(void);
bool valve_is_open(void);

#ifdef __cplusplus
}
#endif

#endif // VALVE_H
