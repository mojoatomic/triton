/**
 * RC Submarine Controller
 * ballast_ctrl.h - Ballast state machine
 *
 * Power of 10 compliant
 */

#ifndef BALLAST_CTRL_H
#define BALLAST_CTRL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BALLAST_STATE_IDLE = 0,
    BALLAST_STATE_FILLING,
    BALLAST_STATE_DRAINING,
    BALLAST_STATE_HOLDING
} BallastState_t;

typedef struct {
    BallastState_t state;
    int8_t target_level;         // -100 (empty) to +100 (full)
    int8_t current_level;        // Estimated current level
    int32_t current_level_x1000; // Fixed-point internal estimate
    uint32_t last_update_ms;
    uint32_t fill_time_ms;
} BallastController_t;

void ballast_ctrl_init(BallastController_t* ctrl);
void ballast_ctrl_set_target(BallastController_t* ctrl, int8_t level);
void ballast_ctrl_update(BallastController_t* ctrl,
                         uint32_t now_ms,
                         int8_t* pump_speed_out,
                         bool* valve_open_out);
BallastState_t ballast_ctrl_get_state(const BallastController_t* ctrl);
int8_t ballast_ctrl_get_target(const BallastController_t* ctrl);
int8_t ballast_ctrl_get_current(const BallastController_t* ctrl);

#ifdef __cplusplus
}
#endif

#endif // BALLAST_CTRL_H
