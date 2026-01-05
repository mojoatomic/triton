/**
 * RC Submarine Controller
 * depth_ctrl.c - Depth controller
 *
 * Power of 10 compliant
 */

#include "depth_ctrl.h"

#include "config.h"

#include <stddef.h>

void depth_ctrl_init(DepthController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    pid_init(&ctrl->pid, PID_DEPTH_KP, PID_DEPTH_KI, PID_DEPTH_KD);
    pid_set_limits(&ctrl->pid, -100.0f, 100.0f, 500.0f);

    ctrl->target_depth_cm = 0;
    ctrl->enabled = false;
}

void depth_ctrl_set_target(DepthController_t* ctrl, int32_t depth_cm) {
    P10_ASSERT(ctrl != NULL);
    P10_ASSERT(depth_cm >= 0);
    P10_ASSERT(depth_cm <= MAX_DEPTH_CM);

    if (ctrl == NULL) {
        return;
    }

    if ((depth_cm < 0) || (depth_cm > MAX_DEPTH_CM)) {
        return;
    }

    ctrl->target_depth_cm = depth_cm;
}

void depth_ctrl_enable(DepthController_t* ctrl, bool enable) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    if (enable && !ctrl->enabled) {
        pid_reset(&ctrl->pid);
    }

    ctrl->enabled = enable;
}

int8_t depth_ctrl_update(DepthController_t* ctrl, int32_t current_depth_cm, float dt) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return 0;
    }

    if (!ctrl->enabled) {
        return 0;
    }

    const float output = pid_update(&ctrl->pid,
                                    (float)ctrl->target_depth_cm,
                                    (float)current_depth_cm,
                                    dt);

    // Positive output = need to go deeper = fill ballast
    // Negative output = need to go shallower = empty ballast
    return (int8_t)output;
}
