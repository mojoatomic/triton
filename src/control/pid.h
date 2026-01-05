/**
 * RC Submarine Controller
 * pid.h - PID controller
 *
 * Power of 10 compliant
 */

#ifndef PID_H
#define PID_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Gains
    float kp;
    float ki;
    float kd;

    // State
    float integral;
    float prev_error;
    float prev_measurement;

    // Limits
    float integral_limit;
    float output_min;
    float output_max;

    // Configuration
    bool use_derivative_on_measurement;
} PidController_t;

void pid_init(PidController_t* pid, float kp, float ki, float kd);
void pid_set_limits(PidController_t* pid, float out_min, float out_max, float int_limit);
float pid_update(PidController_t* pid, float setpoint, float measurement, float dt);
void pid_reset(PidController_t* pid);

#ifdef __cplusplus
}
#endif

#endif // PID_H
