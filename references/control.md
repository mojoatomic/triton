# Control System Reference

PID controllers, state machines, and control logic specifications.

## PID Controller

### Generic PID Implementation

```c
// pid.h
#ifndef PID_H
#define PID_H

#include "types.h"

typedef struct {
    // Gains
    float kp;
    float ki;
    float kd;
    
    // State
    float integral;
    float prev_error;
    float prev_measurement;  // For derivative on measurement
    
    // Limits
    float integral_limit;    // Anti-windup
    float output_min;
    float output_max;
    
    // Configuration
    bool use_derivative_on_measurement;  // Avoids derivative kick
} PidController_t;

void pid_init(PidController_t* pid, float kp, float ki, float kd);
void pid_set_limits(PidController_t* pid, float out_min, float out_max, float int_limit);
float pid_update(PidController_t* pid, float setpoint, float measurement, float dt);
void pid_reset(PidController_t* pid);

#endif

// pid.c
#include "pid.h"
#include <math.h>

void pid_init(PidController_t* pid, float kp, float ki, float kd) {
    P10_ASSERT(pid != NULL);
    
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
    pid->integral_limit = 1000.0f;  // Default
    pid->output_min = -100.0f;
    pid->output_max = 100.0f;
    pid->use_derivative_on_measurement = true;
}

void pid_set_limits(PidController_t* pid, float out_min, float out_max, float int_limit) {
    P10_ASSERT(pid != NULL);
    P10_ASSERT(out_min < out_max);
    
    pid->output_min = out_min;
    pid->output_max = out_max;
    pid->integral_limit = int_limit;
}

float pid_update(PidController_t* pid, float setpoint, float measurement, float dt) {
    P10_ASSERT(pid != NULL);
    P10_ASSERT(dt > 0.0f);
    
    float error = setpoint - measurement;
    
    // Proportional term
    float p_term = pid->kp * error;
    
    // Integral term with anti-windup
    pid->integral += error * dt;
    if (pid->integral > pid->integral_limit) {
        pid->integral = pid->integral_limit;
    } else if (pid->integral < -pid->integral_limit) {
        pid->integral = -pid->integral_limit;
    }
    float i_term = pid->ki * pid->integral;
    
    // Derivative term (on measurement to avoid derivative kick)
    float d_term;
    if (pid->use_derivative_on_measurement) {
        float d_measurement = (measurement - pid->prev_measurement) / dt;
        d_term = -pid->kd * d_measurement;
    } else {
        float d_error = (error - pid->prev_error) / dt;
        d_term = pid->kd * d_error;
    }
    
    // Store for next iteration
    pid->prev_error = error;
    pid->prev_measurement = measurement;
    
    // Calculate output
    float output = p_term + i_term + d_term;
    
    // Clamp output
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }
    
    return output;
}

void pid_reset(PidController_t* pid) {
    P10_ASSERT(pid != NULL);
    
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_measurement = 0.0f;
}
```

### PID Tuning Guidelines

| Controller | Kp | Ki | Kd | Notes |
|------------|----|----|----|----|
| Depth | 2.0 | 0.1 | 0.5 | Slow response OK, avoid overshoot |
| Pitch | 1.5 | 0.05 | 0.3 | Faster response, moderate damping |

**Tuning procedure:**
1. Set Ki=0, Kd=0
2. Increase Kp until oscillation begins, then reduce by 50%
3. Add Ki slowly until steady-state error eliminated
4. Add Kd to reduce overshoot

---

## Depth Controller

```c
// depth_ctrl.h
#ifndef DEPTH_CTRL_H
#define DEPTH_CTRL_H

#include "types.h"
#include "pid.h"

typedef struct {
    PidController_t pid;
    int32_t target_depth_cm;
    bool enabled;
} DepthController_t;

void depth_ctrl_init(DepthController_t* ctrl);
void depth_ctrl_set_target(DepthController_t* ctrl, int32_t depth_cm);
void depth_ctrl_enable(DepthController_t* ctrl, bool enable);
int8_t depth_ctrl_update(DepthController_t* ctrl, int32_t current_depth_cm, float dt);

#endif

// depth_ctrl.c
#include "depth_ctrl.h"
#include "config.h"

void depth_ctrl_init(DepthController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);
    
    pid_init(&ctrl->pid, PID_DEPTH_KP, PID_DEPTH_KI, PID_DEPTH_KD);
    pid_set_limits(&ctrl->pid, -100.0f, 100.0f, 500.0f);
    ctrl->target_depth_cm = 0;
    ctrl->enabled = false;
}

void depth_ctrl_set_target(DepthController_t* ctrl, int32_t depth_cm) {
    P10_ASSERT(ctrl != NULL);
    P10_ASSERT(depth_cm >= 0);
    P10_ASSERT(depth_cm <= MAX_DEPTH_CM);
    
    ctrl->target_depth_cm = depth_cm;
}

void depth_ctrl_enable(DepthController_t* ctrl, bool enable) {
    P10_ASSERT(ctrl != NULL);
    
    if (enable && !ctrl->enabled) {
        pid_reset(&ctrl->pid);
    }
    ctrl->enabled = enable;
}

int8_t depth_ctrl_update(DepthController_t* ctrl, int32_t current_depth_cm, float dt) {
    P10_ASSERT(ctrl != NULL);
    
    if (!ctrl->enabled) {
        return 0;
    }
    
    float output = pid_update(&ctrl->pid, 
                              (float)ctrl->target_depth_cm,
                              (float)current_depth_cm,
                              dt);
    
    // Positive output = need to go deeper = fill ballast
    // Negative output = need to go shallower = empty ballast
    return (int8_t)output;
}
```

---

## Pitch Controller

```c
// pitch_ctrl.h
#ifndef PITCH_CTRL_H
#define PITCH_CTRL_H

#include "types.h"
#include "pid.h"

typedef struct {
    PidController_t pid;
    int16_t target_pitch_x10;  // 0.1° units
    bool enabled;
} PitchController_t;

void pitch_ctrl_init(PitchController_t* ctrl);
void pitch_ctrl_set_target(PitchController_t* ctrl, int16_t pitch_x10);
void pitch_ctrl_enable(PitchController_t* ctrl, bool enable);
int8_t pitch_ctrl_update(PitchController_t* ctrl, int16_t current_pitch_x10, float dt);

#endif

// pitch_ctrl.c
#include "pitch_ctrl.h"
#include "config.h"

void pitch_ctrl_init(PitchController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);
    
    pid_init(&ctrl->pid, PID_PITCH_KP, PID_PITCH_KI, PID_PITCH_KD);
    pid_set_limits(&ctrl->pid, -100.0f, 100.0f, 200.0f);
    ctrl->target_pitch_x10 = 0;  // Level
    ctrl->enabled = true;        // Always try to stay level
}

void pitch_ctrl_set_target(PitchController_t* ctrl, int16_t pitch_x10) {
    P10_ASSERT(ctrl != NULL);
    
    ctrl->target_pitch_x10 = pitch_x10;
}

void pitch_ctrl_enable(PitchController_t* ctrl, bool enable) {
    P10_ASSERT(ctrl != NULL);
    
    if (enable && !ctrl->enabled) {
        pid_reset(&ctrl->pid);
    }
    ctrl->enabled = enable;
}

int8_t pitch_ctrl_update(PitchController_t* ctrl, int16_t current_pitch_x10, float dt) {
    P10_ASSERT(ctrl != NULL);
    
    if (!ctrl->enabled) {
        return 0;
    }
    
    float output = pid_update(&ctrl->pid,
                              (float)ctrl->target_pitch_x10,
                              (float)current_pitch_x10,
                              dt);
    
    // Positive output = nose up = bowplane up
    return (int8_t)output;
}
```

---

## Ballast State Machine

```c
// ballast_ctrl.h
#ifndef BALLAST_CTRL_H
#define BALLAST_CTRL_H

#include "types.h"

typedef enum {
    BALLAST_IDLE,
    BALLAST_FILLING,
    BALLAST_DRAINING,
    BALLAST_HOLDING
} BallastState_t;

typedef struct {
    BallastState_t state;
    int8_t target_level;     // -100 (empty) to +100 (full)
    int8_t current_level;    // Estimated current level
    uint32_t state_start_ms;
    uint32_t fill_time_ms;   // Time for full fill cycle
} BallastController_t;

void ballast_ctrl_init(BallastController_t* ctrl);
void ballast_ctrl_set_target(BallastController_t* ctrl, int8_t level);
void ballast_ctrl_update(BallastController_t* ctrl, uint32_t now_ms);
BallastState_t ballast_ctrl_get_state(BallastController_t* ctrl);

#endif

// ballast_ctrl.c
#include "ballast_ctrl.h"
#include "pump.h"
#include "valve.h"

#define FILL_TIME_MS        10000   // 10 seconds for full cycle
#define LEVEL_TOLERANCE     5       // ±5% tolerance

void ballast_ctrl_init(BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);
    
    ctrl->state = BALLAST_IDLE;
    ctrl->target_level = 0;
    ctrl->current_level = 0;
    ctrl->state_start_ms = 0;
    ctrl->fill_time_ms = FILL_TIME_MS;
}

void ballast_ctrl_set_target(BallastController_t* ctrl, int8_t level) {
    P10_ASSERT(ctrl != NULL);
    
    // Clamp to valid range
    if (level > 100) level = 100;
    if (level < -100) level = -100;
    
    ctrl->target_level = level;
}

void ballast_ctrl_update(BallastController_t* ctrl, uint32_t now_ms) {
    P10_ASSERT(ctrl != NULL);
    
    int8_t error = ctrl->target_level - ctrl->current_level;
    
    switch (ctrl->state) {
        case BALLAST_IDLE:
            pump_stop();
            valve_close();
            
            if (error > LEVEL_TOLERANCE) {
                ctrl->state = BALLAST_FILLING;
                ctrl->state_start_ms = now_ms;
            } else if (error < -LEVEL_TOLERANCE) {
                ctrl->state = BALLAST_DRAINING;
                ctrl->state_start_ms = now_ms;
            }
            break;
            
        case BALLAST_FILLING:
            valve_close();
            pump_set_speed(100);  // Full speed fill
            
            // Estimate level based on time
            {
                uint32_t elapsed = now_ms - ctrl->state_start_ms;
                int32_t level_change = (elapsed * 200) / ctrl->fill_time_ms;
                ctrl->current_level += (int8_t)(level_change > 100 ? 100 : level_change);
                if (ctrl->current_level > 100) ctrl->current_level = 100;
            }
            
            if (ctrl->current_level >= ctrl->target_level) {
                ctrl->state = BALLAST_HOLDING;
                pump_stop();
            }
            break;
            
        case BALLAST_DRAINING:
            valve_open();
            pump_set_speed(-100);  // Reverse pump
            
            // Estimate level based on time
            {
                uint32_t elapsed = now_ms - ctrl->state_start_ms;
                int32_t level_change = (elapsed * 200) / ctrl->fill_time_ms;
                ctrl->current_level -= (int8_t)(level_change > 100 ? 100 : level_change);
                if (ctrl->current_level < -100) ctrl->current_level = -100;
            }
            
            if (ctrl->current_level <= ctrl->target_level) {
                ctrl->state = BALLAST_HOLDING;
                pump_stop();
                valve_close();
            }
            break;
            
        case BALLAST_HOLDING:
            pump_stop();
            valve_close();
            
            // Check if target has changed significantly
            if (abs(error) > LEVEL_TOLERANCE * 2) {
                ctrl->state = BALLAST_IDLE;
            }
            break;
    }
}

BallastState_t ballast_ctrl_get_state(BallastController_t* ctrl) {
    P10_ASSERT(ctrl != NULL);
    return ctrl->state;
}
```

---

## Main State Machine

### State Diagram

```
                              ┌──────────────────────────────────────┐
                              │            EMERGENCY                 │
                              │  • Valve open, pump reverse          │
                              │  • Surfaces full up                  │
                              │  • Ignores all commands              │
                              │  • Only exit: power cycle            │
                              └──────────────────────────────────────┘
                                              ▲
                                              │ Any fault trigger
                              ┌───────────────┴───────────────┐
                              │                               │
┌─────────┐    init    ┌──────┴──────┐    dive cmd    ┌───────┴───────┐
│  INIT   │ ─────────► │   SURFACE   │ ─────────────► │    DIVING     │
│         │            │             │                │               │
└─────────┘            └─────────────┘                └───────┬───────┘
                              ▲                               │
                              │                               │ depth reached
                              │ surface cmd                   ▼
                       ┌──────┴──────┐                ┌───────────────┐
                       │  SURFACING  │ ◄───────────── │  SUBMERGED    │
                       │             │   surface cmd  │   MANUAL      │
                       └─────────────┘                └───────┬───────┘
                              ▲                               │
                              │ surface cmd                   │ depth hold
                              │                               ▼
                              │                       ┌───────────────┐
                              └────────────────────── │  SUBMERGED    │
                                                      │  DEPTH_HOLD   │
                                                      └───────────────┘
```

### Implementation

```c
// state_machine.h
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "types.h"

typedef enum {
    STATE_INIT,
    STATE_SURFACE,
    STATE_DIVING,
    STATE_SUBMERGED_MANUAL,
    STATE_SUBMERGED_DEPTH_HOLD,
    STATE_SURFACING,
    STATE_EMERGENCY
} MainState_t;

typedef enum {
    CMD_NONE,
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
} StateMachine_t;

void state_machine_init(StateMachine_t* sm);
void state_machine_process(StateMachine_t* sm, Command_t cmd, 
                           int32_t depth_cm, uint32_t now_ms);
MainState_t state_machine_get_state(StateMachine_t* sm);
void state_machine_trigger_emergency(StateMachine_t* sm);

#endif

// state_machine.c
#include "state_machine.h"
#include "depth_ctrl.h"
#include "ballast_ctrl.h"
#include "safety.h"

#define SURFACE_DEPTH_CM    10      // Consider "surfaced" at <10cm
#define DIVE_COMPLETE_CM    50      // Consider "submerged" at >50cm

extern DepthController_t g_depth_ctrl;
extern BallastController_t g_ballast_ctrl;

void state_machine_init(StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);
    
    sm->state = STATE_INIT;
    sm->target_depth_cm = 0;
    sm->state_start_ms = 0;
}

void state_machine_process(StateMachine_t* sm, Command_t cmd,
                           int32_t depth_cm, uint32_t now_ms) {
    P10_ASSERT(sm != NULL);
    
    // Emergency command always takes precedence
    if (cmd == CMD_EMERGENCY) {
        state_machine_trigger_emergency(sm);
        return;
    }
    
    // Emergency state is terminal
    if (sm->state == STATE_EMERGENCY) {
        return;
    }
    
    switch (sm->state) {
        case STATE_INIT:
            // Transition to surface after initialization
            sm->state = STATE_SURFACE;
            sm->state_start_ms = now_ms;
            depth_ctrl_enable(&g_depth_ctrl, false);
            break;
            
        case STATE_SURFACE:
            ballast_ctrl_set_target(&g_ballast_ctrl, -100);  // Empty
            
            if (cmd == CMD_DIVE && sm->target_depth_cm > 0) {
                sm->state = STATE_DIVING;
                sm->state_start_ms = now_ms;
            }
            break;
            
        case STATE_DIVING:
            ballast_ctrl_set_target(&g_ballast_ctrl, 50);  // Partial fill
            
            if (cmd == CMD_SURFACE) {
                sm->state = STATE_SURFACING;
                sm->state_start_ms = now_ms;
            } else if (depth_cm >= DIVE_COMPLETE_CM) {
                sm->state = STATE_SUBMERGED_MANUAL;
                sm->state_start_ms = now_ms;
            }
            break;
            
        case STATE_SUBMERGED_MANUAL:
            if (cmd == CMD_SURFACE) {
                sm->state = STATE_SURFACING;
                sm->state_start_ms = now_ms;
            } else if (cmd == CMD_DEPTH_HOLD) {
                sm->state = STATE_SUBMERGED_DEPTH_HOLD;
                depth_ctrl_set_target(&g_depth_ctrl, depth_cm);
                depth_ctrl_enable(&g_depth_ctrl, true);
            }
            break;
            
        case STATE_SUBMERGED_DEPTH_HOLD:
            if (cmd == CMD_SURFACE) {
                sm->state = STATE_SURFACING;
                sm->state_start_ms = now_ms;
                depth_ctrl_enable(&g_depth_ctrl, false);
            } else if (cmd == CMD_MANUAL) {
                sm->state = STATE_SUBMERGED_MANUAL;
                depth_ctrl_enable(&g_depth_ctrl, false);
            }
            break;
            
        case STATE_SURFACING:
            ballast_ctrl_set_target(&g_ballast_ctrl, -100);  // Empty
            depth_ctrl_enable(&g_depth_ctrl, false);
            
            if (depth_cm <= SURFACE_DEPTH_CM) {
                sm->state = STATE_SURFACE;
                sm->state_start_ms = now_ms;
            }
            break;
            
        case STATE_EMERGENCY:
            // Terminal state - handled by safety system
            break;
    }
}

MainState_t state_machine_get_state(StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);
    return sm->state;
}

void state_machine_trigger_emergency(StateMachine_t* sm) {
    P10_ASSERT(sm != NULL);
    sm->state = STATE_EMERGENCY;
}
```

---

## RC Input to Command Mapping

```c
// Map RC switch positions to commands
Command_t rc_to_command(RcFrame_t* frame) {
    P10_ASSERT(frame != NULL);
    
    // CH6 = Emergency switch (overrides all)
    if (frame->channels[5] > 1800) {
        return CMD_EMERGENCY;
    }
    
    // CH5 = Mode switch (3-position)
    uint16_t mode_pwm = frame->channels[4];
    
    if (mode_pwm < 1300) {
        return CMD_SURFACE;
    } else if (mode_pwm < 1700) {
        return CMD_MANUAL;
    } else {
        return CMD_DEPTH_HOLD;
    }
}

// Map RC stick positions to control inputs
void rc_to_control(RcFrame_t* frame, ControlInputs_t* inputs) {
    P10_ASSERT(frame != NULL);
    P10_ASSERT(inputs != NULL);
    
    // CH1 = Throttle (forward/reverse)
    inputs->throttle = pwm_to_percent(frame->channels[0]);
    
    // CH2 = Rudder
    inputs->rudder = pwm_to_percent(frame->channels[1]);
    
    // CH3 = Elevator/Bowplane
    inputs->elevator = pwm_to_percent(frame->channels[2]);
    
    // CH4 = Ballast (manual override)
    inputs->ballast = pwm_to_percent(frame->channels[3]);
}
```
