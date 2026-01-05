/**
 * RC Submarine Controller
 * main.c - Entry point and dual-core initialization
 *
 * Power of 10 compliant
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "config.h"
#include "types.h"

// Safety includes
#include "safety/safety_monitor.h"
#include "safety/emergency.h"
#include "util/log.h"

// Driver includes
#include "drivers/rc_input.h"
#include "drivers/pressure_sensor.h"
#include "drivers/imu.h"
#include "drivers/pump.h"
#include "drivers/valve.h"
#include "drivers/servo.h"
#include "drivers/battery.h"
#include "drivers/leak.h"

// Control includes
#include "control/state_machine.h"
#include "control/depth_ctrl.h"
#include "control/pitch_ctrl.h"
#include "control/ballast_ctrl.h"

// Forward declarations
void core0_main(void);
void core1_main(void);

// Helper functions for Core 1
static void init_hardware(void);
static void init_controllers(StateMachine_t* sm, DepthController_t* dc,
                           PitchController_t* pc, BallastController_t* bc);
static void read_sensors(RcFrame_t* rc, DepthReading_t* depth, AttitudeReading_t* att);
static void process_rc_inputs(const RcFrame_t* rc, ControlInputs_t* inputs);
static void run_control_loop(StateMachine_t* sm, DepthController_t* dc,
                            PitchController_t* pc, BallastController_t* bc,
                            const ControlInputs_t* inputs, const DepthReading_t* depth,
                            const AttitudeReading_t* att, float dt);
static void update_debug_output(uint32_t* loops, const StateMachine_t* sm,
                               const DepthReading_t* depth);

int main(void) {
    // Initialize stdio for debug output
    stdio_init_all();

    // Brief delay for USB enumeration
    sleep_ms(1000);

    printf("RC Submarine Controller starting...\n");

    // Initialize LED GPIO
    gpio_init(PIN_LED_STATUS);
    gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);

    // Launch Core 1 (control logic)
    multicore_launch_core1(core1_main);

    // Core 0 runs safety monitor
    core0_main();

    // Never reached
    return 0;
}

// Core 0: Safety monitor (100 Hz)
void core0_main(void) {
    printf("Core 0: Safety monitor starting\n");

    // Initialize safety systems
    safety_monitor_init();
    log_init();

    // 100 Hz safety loop
    const uint32_t loop_period_us = 10000;  // 10 ms
    uint32_t next_loop_us = time_us_32();
    uint32_t loops = 0;

    while (1) {
        // Run safety checks
        safety_monitor_run();

        // If in emergency, run blow sequence
        if (is_emergency_active()) {
            emergency_blow_run();
        }

        // Debug heartbeat every second
        loops++;
        if (loops >= 100) {
            printf("Core 0: alive (faults=0x%04X)\n",
                   safety_monitor_get_faults().all);
            loops = 0;
        }

        // Maintain loop timing
        next_loop_us += loop_period_us;
        int32_t sleep_us = next_loop_us - time_us_32();
        if (sleep_us > 0) {
            sleep_us(sleep_us);
        }
    }
}

// Helper function implementations
static void init_hardware(void) {
    P10_ASSERT(PIN_LED_STATUS == 25);  // Verify correct board config

    rc_input_init();
    pressure_sensor_init();
    imu_init();
    pump_init();
    valve_init();
    servo_init();
    battery_init();
    leak_init();
}

static void init_controllers(StateMachine_t* sm, DepthController_t* dc,
                           PitchController_t* pc, BallastController_t* bc) {
    P10_ASSERT(sm != NULL);
    P10_ASSERT(dc != NULL);
    P10_ASSERT(pc != NULL);
    P10_ASSERT(bc != NULL);

    state_machine_init(sm);
    depth_ctrl_init(dc);
    pitch_ctrl_init(pc);
    ballast_ctrl_init(bc);
}

static void read_sensors(RcFrame_t* rc, DepthReading_t* depth, AttitudeReading_t* att) {
    P10_ASSERT(rc != NULL);
    P10_ASSERT(depth != NULL);
    P10_ASSERT(att != NULL);

    rc_input_read(rc);
    pressure_sensor_read(depth);
    imu_read(att);

    // Update safety monitor
    if (rc->valid) {
        safety_update_rc_time(rc->timestamp_ms);
    }
    if (depth->valid) {
        safety_update_depth(depth->depth_cm);
    }
    if (att->valid) {
        safety_update_pitch(att->pitch_deg_x10);
    }
}

static void process_rc_inputs(const RcFrame_t* rc, ControlInputs_t* inputs) {
    P10_ASSERT(rc != NULL);
    P10_ASSERT(inputs != NULL);

    inputs->throttle = 0;
    inputs->rudder = 0;
    inputs->elevator = 0;
    inputs->ballast = 0;

    if (rc->valid) {
        // Convert RC PWM to -100 to +100 range
        inputs->throttle = (int8_t)((rc->channels[0] - RC_PWM_CENTER) / 5);
        inputs->rudder = (int8_t)((rc->channels[1] - RC_PWM_CENTER) / 5);
        inputs->elevator = (int8_t)((rc->channels[2] - RC_PWM_CENTER) / 5);
        inputs->ballast = (int8_t)((rc->channels[3] - RC_PWM_CENTER) / 5);

        // Clamp values
        inputs->throttle = CLAMP(inputs->throttle, -100, 100);
        inputs->rudder = CLAMP(inputs->rudder, -100, 100);
        inputs->elevator = CLAMP(inputs->elevator, -100, 100);
        inputs->ballast = CLAMP(inputs->ballast, -100, 100);
    }
}

static void run_control_loop(StateMachine_t* sm, DepthController_t* dc,
                            PitchController_t* pc, BallastController_t* bc,
                            const ControlInputs_t* inputs, const DepthReading_t* depth,
                            const AttitudeReading_t* att, float dt) {
    P10_ASSERT(sm != NULL);
    P10_ASSERT(dc != NULL);
    P10_ASSERT(pc != NULL);
    P10_ASSERT(bc != NULL);
    P10_ASSERT(inputs != NULL);
    P10_ASSERT(depth != NULL);
    P10_ASSERT(att != NULL);

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // Update state machine
    state_machine_update(sm, inputs, depth, now_ms);

    // Get current state
    StateInfo_t state_info;
    state_machine_get_state(sm, &state_info);

    // Run controllers based on state
    if (state_info.mode == MODE_DEPTH_HOLD && state_info.state == STATE_SUBMERGED) {
        int8_t ballast_cmd = depth_ctrl_update(dc, depth->depth_cm, dt);
        ballast_ctrl_set_command(bc, ballast_cmd);
    } else {
        ballast_ctrl_set_command(bc, inputs->ballast);
    }

    // Pitch stabilization when submerged
    if (state_info.state == STATE_SUBMERGED) {
        int8_t pitch_cmd = pitch_ctrl_update(pc, att->pitch_deg_x10 / 10, dt);
        servo_set_position(SERVO_BOWPLANE, pitch_cmd);
        servo_set_position(SERVO_STERNPLANE, pitch_cmd);
    } else {
        servo_set_position(SERVO_BOWPLANE, 0);
        servo_set_position(SERVO_STERNPLANE, 0);
    }

    // Update ballast and rudder
    ballast_ctrl_update(bc, now_ms);
    servo_set_position(SERVO_RUDDER, inputs->rudder);
}

static void update_debug_output(uint32_t* loops, const StateMachine_t* sm,
                               const DepthReading_t* depth) {
    P10_ASSERT(loops != NULL);
    P10_ASSERT(sm != NULL);
    P10_ASSERT(depth != NULL);

    (*loops)++;
    if (*loops >= 50) {
        StateInfo_t info;
        state_machine_get_state((StateMachine_t*)sm, &info);
        printf("Core 1: state=%d, mode=%d, depth=%d cm\n",
               info.state, info.mode, depth->depth_cm);
        *loops = 0;
    }
}

// Core 1: Control logic (50 Hz)
void core1_main(void) {
    printf("Core 1: Control logic starting\n");

    // Initialize hardware
    init_hardware();

    // Initialize controllers
    StateMachine_t state_machine;
    DepthController_t depth_ctrl;
    PitchController_t pitch_ctrl;
    BallastController_t ballast_ctrl;
    init_controllers(&state_machine, &depth_ctrl, &pitch_ctrl, &ballast_ctrl);

    log_event(EVT_INIT_COMPLETE, 0, 0);

    // 50 Hz control loop
    const uint32_t loop_period_us = 20000;  // 20 ms
    uint32_t next_loop_us = time_us_32();
    uint32_t last_loop_us = next_loop_us;
    uint32_t loops = 0;

    while (1) {
        uint32_t now_us = time_us_32();
        float dt = (now_us - last_loop_us) / 1000000.0f;
        last_loop_us = now_us;

        // Skip if in emergency
        if (is_emergency_active()) {
            sleep_ms(100);
            continue;
        }

        // Read sensors
        RcFrame_t rc;
        DepthReading_t depth;
        AttitudeReading_t attitude;
        read_sensors(&rc, &depth, &attitude);

        // Process inputs
        ControlInputs_t inputs;
        process_rc_inputs(&rc, &inputs);

        // Run control algorithms
        run_control_loop(&state_machine, &depth_ctrl, &pitch_ctrl, &ballast_ctrl,
                        &inputs, &depth, &attitude, dt);

        // Update debug output
        update_debug_output(&loops, &state_machine, &depth);

        // Maintain timing
        next_loop_us += loop_period_us;
        int32_t sleep_us_val = next_loop_us - time_us_32();
        if (sleep_us_val > 0) {
            sleep_us(sleep_us_val);
        }
    }
}
