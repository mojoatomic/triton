/**
 * RC Submarine Controller
 * emergency.h - Emergency blow sequence
 *
 * Power of 10 compliant
 */

#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "config.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Emergency blow public API
void trigger_emergency_blow(EventCode_t reason);
void emergency_blow_run(void);
bool is_emergency_active(void);

// P10 assert failure handler
void p10_assert_fail(const char* file, int line, const char* cond);

#ifdef __cplusplus
}
#endif

#endif // EMERGENCY_H
