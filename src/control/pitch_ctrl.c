/**
 * RC Submarine Controller
 * pitch_ctrl.c - Pitch controller
 *
 * Power of 10 compliant
 */

#include "pitch_ctrl.h"

#include "config.h"

#include <stddef.h>

static int16_t pitch_ctrl_limit_x10(void) {
    return (int16_t)(MAX_PITCH_DEG * 10);
}

void pitch_ctrl_init(PitchController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    pid_init(&ctrl->pid, PID_PITCH_KP, PID_PITCH_KI, PID_PITCH_KD);
    pid_set_limits(&ctrl->pid, -100.0f, 100.0f, 200.0f);

    ctrl->target_pitch_x10 = 0;
    ctrl->enabled = true;
}

void pitch_ctrl_set_target(PitchController_t* ctrl, int16_t pitch_x10) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    const int16_t limit_x10 = pitch_ctrl_limit_x10();
    if ((pitch_x10 < -limit_x10) || (pitch_x10 > limit_x10)) {
        return;
    }

    ctrl->target_pitch_x10 = pitch_x10;
}

void pitch_ctrl_enable(PitchController_t* ctrl, bool enable) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    if (enable && !ctrl->enabled) {
        pid_reset(&ctrl->pid);
    }

    ctrl->enabled = enable;
}

int8_t pitch_ctrl_update(PitchController_t* ctrl, int16_t current_pitch_x10, float dt) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return 0;
    }

    if (!ctrl->enabled) {
        return 0;
    }

    const float output = pid_update(&ctrl->pid,
                                    (float)ctrl->target_pitch_x10,
                                    (float)current_pitch_x10,
                                    dt);

    // Positive output = nose up command (e.g., bowplane up)
    return (int8_t)output;
}
