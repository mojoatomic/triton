/**
 * RC Submarine Controller
 * state_machine.h - Main state machine
 *
 * Power of 10 compliant
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATE_INIT = 0,
    STATE_SURFACE,
    STATE_DIVING,
    STATE_SUBMERGED_MANUAL,
    STATE_SUBMERGED_DEPTH_HOLD,
    STATE_SURFACING,
    STATE_EMERGENCY
} MainState_t;

typedef enum {
    CMD_NONE = 0,
    CMD_DIVE,
    CMD_SURFACE,
    CMD_DEPTH_HOLD,
    CMD_MANUAL,
    CMD_EMERGENCY
} Command_t;

typedef struct {
    MainState_t state;
    int32_t target_depth_cm;
    uint32_t state_start_ms;

    // Outputs (decisions)
    int8_t ballast_target_level;  // -100 (empty) to +100 (full)
    bool depth_hold_enabled;
} StateMachine_t;

void state_machine_init(StateMachine_t* sm);
void state_machine_set_target_depth(StateMachine_t* sm, int32_t depth_cm);
void state_machine_process(StateMachine_t* sm,
                           Command_t cmd,
                           int32_t depth_cm,
                           uint32_t now_ms);
MainState_t state_machine_get_state(const StateMachine_t* sm);
int8_t state_machine_get_ballast_target(const StateMachine_t* sm);
bool state_machine_get_depth_hold_enabled(const StateMachine_t* sm);
void state_machine_trigger_emergency(StateMachine_t* sm);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H
