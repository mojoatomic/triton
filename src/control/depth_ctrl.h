/**
 * RC Submarine Controller
 * depth_ctrl.h - Depth controller
 *
 * Power of 10 compliant
 */

#ifndef DEPTH_CTRL_H
#define DEPTH_CTRL_H

#include "types.h"

#include "pid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PidController_t pid;
    int32_t target_depth_cm;
    bool enabled;
} DepthController_t;

void depth_ctrl_init(DepthController_t* ctrl);
void depth_ctrl_set_target(DepthController_t* ctrl, int32_t depth_cm);
void depth_ctrl_enable(DepthController_t* ctrl, bool enable);
int8_t depth_ctrl_update(DepthController_t* ctrl, int32_t current_depth_cm, float dt);

#ifdef __cplusplus
}
#endif

#endif // DEPTH_CTRL_H
