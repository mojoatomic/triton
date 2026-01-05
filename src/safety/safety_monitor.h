/**
 * RC Submarine Controller
 * safety_monitor.h - Safety monitor (Core 0)
 *
 * Power of 10 compliant
 */

#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Safety monitor public API
void safety_monitor_init(void);
void safety_monitor_run(void);
FaultFlags_t safety_monitor_get_faults(void);
bool safety_monitor_is_emergency(void);

// Called from Core 1 to update shared state
void safety_update_rc_time(uint32_t ms);
void safety_update_depth(int32_t depth_cm);
void safety_update_pitch(int16_t pitch_x10);

#ifdef __cplusplus
}
#endif

#endif // SAFETY_MONITOR_H
