/**
 * RC Submarine Controller
 * pid.c - PID controller
 *
 * Power of 10 compliant
 */

#include "pid.h"

#include <stddef.h>

void pid_init(PidController_t* pid, float kp, float ki, float kd) {
    P10_ASSERT(pid != NULL);

    if (pid == NULL) {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;

    pid->integral_limit = 1000.0f;
    pid->output_min = -100.0f;
    pid->output_max = 100.0f;
    pid->use_derivative_on_measurement = true;
}

void pid_set_limits(PidController_t* pid, float out_min, float out_max, float int_limit) {
    P10_ASSERT(pid != NULL);
    P10_ASSERT(out_min < out_max);

    if (pid == NULL) {
        return;
    }

    if (out_min >= out_max) {
        return;
    }

    pid->output_min = out_min;
    pid->output_max = out_max;
    pid->integral_limit = int_limit;
}

float pid_update(PidController_t* pid, float setpoint, float measurement, float dt) {
    P10_ASSERT(pid != NULL);
    P10_ASSERT(dt > 0.0f);

    if ((pid == NULL) || (dt <= 0.0f)) {
        return 0.0f;
    }

    const float error = setpoint - measurement;

    const float p_term = pid->kp * error;

    pid->integral += error * dt;
    if (pid->integral > pid->integral_limit) {
        pid->integral = pid->integral_limit;
    } else if (pid->integral < -pid->integral_limit) {
        pid->integral = -pid->integral_limit;
    }
    const float i_term = pid->ki * pid->integral;

    float d_term;
    if (pid->use_derivative_on_measurement) {
        const float d_meas = (measurement - pid->prev_measurement) / dt;
        d_term = -pid->kd * d_meas;
    } else {
        const float d_err = (error - pid->prev_error) / dt;
        d_term = pid->kd * d_err;
    }

    pid->prev_error = error;
    pid->prev_measurement = measurement;

    float output = p_term + i_term + d_term;
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }

    return output;
}

void pid_reset(PidController_t* pid) {
    P10_ASSERT(pid != NULL);

    if (pid == NULL) {
        return;
    }

    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
}
