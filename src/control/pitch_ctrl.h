/**
 * RC Submarine Controller
 * pitch_ctrl.h - Pitch controller
 *
 * Power of 10 compliant
 */

#ifndef PITCH_CTRL_H
#define PITCH_CTRL_H

#include "types.h"

#include "pid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PidController_t pid;
    int16_t target_pitch_x10;  // 0.1° units (positive = nose up)
    bool enabled;
} PitchController_t;

void pitch_ctrl_init(PitchController_t* ctrl);
void pitch_ctrl_set_target(PitchController_t* ctrl, int16_t pitch_x10);
void pitch_ctrl_enable(PitchController_t* ctrl, bool enable);
int8_t pitch_ctrl_update(PitchController_t* ctrl, int16_t current_pitch_x10, float dt);

#ifdef __cplusplus
}
#endif

#endif // PITCH_CTRL_H
