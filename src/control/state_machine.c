/**
 * RC Submarine Controller
 * state_machine.c - Main state machine
 *
 * Power of 10 compliant
 */

#include "state_machine.h"

#include "config.h"

#include <stddef.h>

#define SURFACE_DEPTH_CM    10
#define DIVE_COMPLETE_CM    50

static void state_machine_set_outputs(StateMachine_t* sm, int8_t ballast_level, bool depth_hold) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    sm->ballast_target_level = ballast_level;
    sm->depth_hold_enabled = depth_hold;
}

static void state_machine_handle_init(StateMachine_t* sm, uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    sm->state = STATE_SURFACE;
    sm->state_start_ms = now_ms;
    state_machine_set_outputs(sm, -100, false);
}

static void state_machine_handle_surface(StateMachine_t* sm, Command_t cmd, uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    state_machine_set_outputs(sm, -100, false);
    if ((cmd == CMD_DIVE) && (sm->target_depth_cm > 0)) {
        sm->state = STATE_DIVING;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, 50, false);
    }
}

static void state_machine_handle_diving(StateMachine_t* sm, Command_t cmd, int32_t depth_cm, uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    state_machine_set_outputs(sm, 50, false);
    if (cmd == CMD_SURFACE) {
        sm->state = STATE_SURFACING;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, -100, false);
    } else if (depth_cm >= DIVE_COMPLETE_CM) {
        sm->state = STATE_SUBMERGED_MANUAL;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, 0, false);
    }
}

static void state_machine_handle_submerged_manual(StateMachine_t* sm,
                                                  Command_t cmd,
                                                  int32_t depth_cm,
                                                  uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    state_machine_set_outputs(sm, 0, false);
    if (cmd == CMD_SURFACE) {
        sm->state = STATE_SURFACING;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, -100, false);
    } else if (cmd == CMD_DEPTH_HOLD) {
        sm->state = STATE_SUBMERGED_DEPTH_HOLD;
        state_machine_set_target_depth(sm, depth_cm);
        state_machine_set_outputs(sm, 0, true);
    }
}

static void state_machine_handle_submerged_depth_hold(StateMachine_t* sm,
                                                      Command_t cmd,
                                                      int32_t depth_cm,
                                                      uint32_t now_ms) {
    (void)depth_cm;

    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    state_machine_set_outputs(sm, 0, true);
    if (cmd == CMD_SURFACE) {
        sm->state = STATE_SURFACING;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, -100, false);
    } else if (cmd == CMD_MANUAL) {
        sm->state = STATE_SUBMERGED_MANUAL;
        sm->state_start_ms = now_ms;
        state_machine_set_outputs(sm, 0, false);
    }
}

static void state_machine_handle_surfacing(StateMachine_t* sm, int32_t depth_cm, uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    state_machine_set_outputs(sm, -100, false);
    if (depth_cm <= SURFACE_DEPTH_CM) {
        sm->state = STATE_SURFACE;
        sm->state_start_ms = now_ms;
    }
}

void state_machine_init(StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    sm->state = STATE_INIT;
    sm->target_depth_cm = 0;
    sm->state_start_ms = 0U;

    state_machine_set_outputs(sm, -100, false);
}

void state_machine_set_target_depth(StateMachine_t* sm, int32_t depth_cm) {
    P10_ASSERT(sm != NULL);
    P10_ASSERT(depth_cm >= 0);
    P10_ASSERT(depth_cm <= MAX_DEPTH_CM);

    if (sm == NULL) {
        return;
    }

    if ((depth_cm < 0) || (depth_cm > MAX_DEPTH_CM)) {
        return;
    }

    sm->target_depth_cm = depth_cm;
}

void state_machine_trigger_emergency(StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    sm->state = STATE_EMERGENCY;
    state_machine_set_outputs(sm, -100, false);
}

void state_machine_process(StateMachine_t* sm,
                           Command_t cmd,
                           int32_t depth_cm,
                           uint32_t now_ms) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return;
    }

    if (cmd == CMD_EMERGENCY) {
        state_machine_trigger_emergency(sm);
        return;
    }

    if (sm->state == STATE_EMERGENCY) {
        return;
    }

    switch (sm->state) {
        case STATE_INIT:
            state_machine_handle_init(sm, now_ms);
            break;

        case STATE_SURFACE:
            state_machine_handle_surface(sm, cmd, now_ms);
            break;

        case STATE_DIVING:
            state_machine_handle_diving(sm, cmd, depth_cm, now_ms);
            break;

        case STATE_SUBMERGED_MANUAL:
            state_machine_handle_submerged_manual(sm, cmd, depth_cm, now_ms);
            break;

        case STATE_SUBMERGED_DEPTH_HOLD:
            state_machine_handle_submerged_depth_hold(sm, cmd, depth_cm, now_ms);
            break;

        case STATE_SURFACING:
            state_machine_handle_surfacing(sm, depth_cm, now_ms);
            break;

        case STATE_EMERGENCY:
            break;

        default:
            sm->state = STATE_SURFACE;
            sm->state_start_ms = now_ms;
            state_machine_set_outputs(sm, -100, false);
            break;
    }
}

MainState_t state_machine_get_state(const StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return STATE_INIT;
    }

    return sm->state;
}

int8_t state_machine_get_ballast_target(const StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return 0;
    }

    return sm->ballast_target_level;
}

bool state_machine_get_depth_hold_enabled(const StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);

    if (sm == NULL) {
        return false;
    }

    return sm->depth_hold_enabled;
}
