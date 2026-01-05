/**
 * RC Submarine Controller
 * ballast_ctrl.c - Ballast state machine
 *
 * Power of 10 compliant
 */

#include "ballast_ctrl.h"

#include <stddef.h>

#define BALLAST_FILL_TIME_MS        10000U
#define BALLAST_LEVEL_TOLERANCE     5
#define BALLAST_FULL_RANGE_UNITS    200
#define BALLAST_SCALE_X1000         1000

static int8_t clamp_i8(int16_t v, int8_t lo, int8_t hi) {
    P10_ASSERT(lo < hi);

    if (v < (int16_t)lo) {
        return lo;
    }
    if (v > (int16_t)hi) {
        return hi;
    }

    return (int8_t)v;
}

static void ballast_ctrl_default_outputs(int8_t* pump_speed_out, bool* valve_open_out) {
    P10_ASSERT(pump_speed_out != NULL);
    P10_ASSERT(valve_open_out != NULL);

    if ((pump_speed_out == NULL) || (valve_open_out == NULL)) {
        return;
    }

    *pump_speed_out = 0;
    *valve_open_out = false;
}

static uint32_t clamp_dt_ms(uint32_t dt_ms, uint32_t max_dt_ms) {
    P10_ASSERT(max_dt_ms > 0U);

    if (dt_ms > max_dt_ms) {
        return max_dt_ms;
    }

    return dt_ms;
}

static int32_t ballast_delta_x1000(uint32_t dt_ms, uint32_t fill_time_ms) {
    P10_ASSERT(fill_time_ms > 0U);

    if (fill_time_ms == 0U) {
        return 0;
    }

    const uint32_t dt_clamped = clamp_dt_ms(dt_ms, fill_time_ms);

    const uint64_t num = (uint64_t)dt_clamped * (uint64_t)(BALLAST_FULL_RANGE_UNITS * BALLAST_SCALE_X1000);
    const uint64_t delta_u = num / (uint64_t)fill_time_ms;

    if (delta_u > (uint64_t)INT32_MAX) {
        return INT32_MAX;
    }

    return (int32_t)delta_u;
}

static void ballast_update_level(BallastController_t* ctrl, int8_t direction, uint32_t now_ms) {
    P10_ASSERT(ctrl != NULL);
    P10_ASSERT((direction == -1) || (direction == 1));

    if (ctrl == NULL) {
        return;
    }

    if (ctrl->last_update_ms == 0U) {
        ctrl->last_update_ms = now_ms;
        return;
    }

    const uint32_t dt_ms = now_ms - ctrl->last_update_ms;
    ctrl->last_update_ms = now_ms;

    const int32_t delta = ballast_delta_x1000(dt_ms, ctrl->fill_time_ms);
    ctrl->current_level_x1000 += (int32_t)direction * delta;

    const int32_t min_x1000 = -100 * BALLAST_SCALE_X1000;
    const int32_t max_x1000 = 100 * BALLAST_SCALE_X1000;
    if (ctrl->current_level_x1000 < min_x1000) {
        ctrl->current_level_x1000 = min_x1000;
    } else if (ctrl->current_level_x1000 > max_x1000) {
        ctrl->current_level_x1000 = max_x1000;
    }

    ctrl->current_level = clamp_i8((int16_t)(ctrl->current_level_x1000 / BALLAST_SCALE_X1000), -100, 100);
}

void ballast_ctrl_init(BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    ctrl->state = BALLAST_STATE_IDLE;
    ctrl->target_level = 0;
    ctrl->current_level = 0;
    ctrl->current_level_x1000 = 0;
    ctrl->last_update_ms = 0U;
    ctrl->fill_time_ms = BALLAST_FILL_TIME_MS;
}

void ballast_ctrl_set_target(BallastController_t* ctrl, int8_t level) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return;
    }

    const int8_t clamped = clamp_i8((int16_t)level, -100, 100);
    ctrl->target_level = clamped;
}

void ballast_ctrl_update(BallastController_t* ctrl,
                         uint32_t now_ms,
                         int8_t* pump_speed_out,
                         bool* valve_open_out) {
    P10_ASSERT(ctrl != NULL);
    P10_ASSERT(pump_speed_out != NULL);
    P10_ASSERT(valve_open_out != NULL);

    ballast_ctrl_default_outputs(pump_speed_out, valve_open_out);

    if ((ctrl == NULL) || (pump_speed_out == NULL) || (valve_open_out == NULL)) {
        return;
    }

    const int16_t error = (int16_t)ctrl->target_level - (int16_t)ctrl->current_level;
    const int16_t abs_error = (int16_t)ABS(error);

    switch (ctrl->state) {
        case BALLAST_STATE_IDLE:
            if (abs_error > BALLAST_LEVEL_TOLERANCE) {
                if (error > 0) {
                    ctrl->state = BALLAST_STATE_FILLING;
                    ctrl->last_update_ms = 0U;
                    *pump_speed_out = 100;
                    *valve_open_out = false;
                } else {
                    ctrl->state = BALLAST_STATE_DRAINING;
                    ctrl->last_update_ms = 0U;
                    *pump_speed_out = -100;
                    *valve_open_out = true;
                }
            }
            break;

        case BALLAST_STATE_FILLING:
            *pump_speed_out = 100;
            *valve_open_out = false;

            ballast_update_level(ctrl, 1, now_ms);
            if (ctrl->current_level >= ctrl->target_level) {
                ctrl->current_level = ctrl->target_level;
                ctrl->current_level_x1000 = (int32_t)ctrl->target_level * BALLAST_SCALE_X1000;
                ctrl->state = BALLAST_STATE_HOLDING;
            }
            break;

        case BALLAST_STATE_DRAINING:
            *pump_speed_out = -100;
            *valve_open_out = true;

            ballast_update_level(ctrl, -1, now_ms);
            if (ctrl->current_level <= ctrl->target_level) {
                ctrl->current_level = ctrl->target_level;
                ctrl->current_level_x1000 = (int32_t)ctrl->target_level * BALLAST_SCALE_X1000;
                ctrl->state = BALLAST_STATE_HOLDING;
            }
            break;

        case BALLAST_STATE_HOLDING:
            if (abs_error > (BALLAST_LEVEL_TOLERANCE * 2)) {
                ctrl->state = BALLAST_STATE_IDLE;
            }
            break;

        default:
            ctrl->state = BALLAST_STATE_IDLE;
            break;
    }
}

BallastState_t ballast_ctrl_get_state(const BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return BALLAST_STATE_IDLE;
    }

    return ctrl->state;
}

int8_t ballast_ctrl_get_target(const BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return 0;
    }

    return ctrl->target_level;
}

int8_t ballast_ctrl_get_current(const BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);

    if (ctrl == NULL) {
        return 0;
    }

    return ctrl->current_level;
}
