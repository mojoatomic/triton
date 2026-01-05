# Safety System Reference

Core 0 safety monitor, emergency blow, watchdog, and fault handling.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         CORE 0                                  │
│                    SAFETY MONITOR                               │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   100 Hz Main Loop                       │   │
│  │                                                          │   │
│  │  1. Feed watchdog                                        │   │
│  │  2. Check RC signal timeout                              │   │
│  │  3. Check battery voltage                                │   │
│  │  4. Check leak sensor                                    │   │
│  │  5. Check depth limit                                    │   │
│  │  6. Check pitch limit                                    │   │
│  │  7. Update fault flags                                   │   │
│  │  8. Trigger emergency if any critical fault              │   │
│  │  9. Heartbeat LED                                        │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              EMERGENCY BLOW (if triggered)               │   │
│  │                                                          │   │
│  │  • Set atomic emergency flag                             │   │
│  │  • Open vent valve                                       │   │
│  │  • Reverse pump at full speed                            │   │
│  │  • Full up on control surfaces                           │   │
│  │  • Log event                                             │   │
│  │  • Continue running until power cycle                    │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Fault Conditions

| Fault | Threshold | Response | Recovery |
|-------|-----------|----------|----------|
| Signal Lost | No valid RC frame for 3s | Emergency blow | Power cycle |
| Low Battery | < 6.4V (2S LiPo) | Emergency blow | Power cycle |
| Leak Detected | GPIO high | Emergency blow | Power cycle |
| Depth Exceeded | > MAX_DEPTH_CM | Emergency blow | Power cycle |
| Pitch Exceeded | > ±45° | Emergency blow | Power cycle |
| Watchdog | Core 0 fails to feed | Hardware reset | Auto-reboot |
| Assert Failure | P10_ASSERT fails | Emergency blow | Power cycle |
| I2C Error | Sensor comm fails | Log + continue | Auto-recover |

## Implementation

### Safety Monitor (Core 0)

```c
// safety_monitor.h
#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "types.h"

void safety_monitor_init(void);
void safety_monitor_run(void);  // Called from Core 0 main loop
FaultFlags_t safety_monitor_get_faults(void);
bool safety_monitor_is_emergency(void);

#endif

// safety_monitor.c
#include "safety_monitor.h"
#include "emergency.h"
#include "config.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"

// Shared state with Core 1 (volatile for cross-core visibility)
static volatile FaultFlags_t g_faults = {0};
static volatile bool g_emergency = false;
static volatile uint32_t g_last_rc_valid_ms = 0;
static volatile int32_t g_current_depth_cm = 0;
static volatile int16_t g_current_pitch_x10 = 0;

// Called from Core 1 to update shared state
void safety_update_rc_time(uint32_t ms) {
    g_last_rc_valid_ms = ms;
}

void safety_update_depth(int32_t depth_cm) {
    g_current_depth_cm = depth_cm;
}

void safety_update_pitch(int16_t pitch_x10) {
    g_current_pitch_x10 = pitch_x10;
}

void safety_monitor_init(void) {
    // Enable hardware watchdog (1 second timeout)
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
    
    // Initialize LED for heartbeat
    gpio_init(PIN_LED_STATUS);
    gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);
    
    g_faults.all = 0;
    g_emergency = false;
}

void safety_monitor_run(void) {
    static uint32_t last_led_toggle_ms = 0;
    static bool led_state = false;
    
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    
    // 1. Feed watchdog (must happen every loop)
    watchdog_update();
    
    // 2. Check RC signal timeout
    if ((now_ms - g_last_rc_valid_ms) > SIGNAL_TIMEOUT_MS) {
        if (!g_faults.bits.signal_lost) {
            g_faults.bits.signal_lost = 1;
            log_event(EVT_SIGNAL_LOST, 0, 0);
        }
    } else {
        if (g_faults.bits.signal_lost) {
            g_faults.bits.signal_lost = 0;
            log_event(EVT_SIGNAL_RESTORED, 0, 0);
        }
    }
    
    // 3. Check battery voltage
    uint16_t batt_mv = battery_read_mv();
    if (batt_mv < MIN_BATTERY_MV) {
        if (!g_faults.bits.low_battery) {
            g_faults.bits.low_battery = 1;
            log_event(EVT_LOW_BATTERY, batt_mv >> 8, batt_mv & 0xFF);
        }
    }
    
    // 4. Check leak sensor
    if (leak_detected()) {
        if (!g_faults.bits.leak) {
            g_faults.bits.leak = 1;
            log_event(EVT_LEAK_DETECTED, 0, 0);
        }
    }
    
    // 5. Check depth limit
    if (g_current_depth_cm > MAX_DEPTH_CM) {
        if (!g_faults.bits.depth_exceeded) {
            g_faults.bits.depth_exceeded = 1;
            log_event(EVT_DEPTH_EXCEEDED, g_current_depth_cm >> 8, 
                      g_current_depth_cm & 0xFF);
        }
    }
    
    // 6. Check pitch limit
    int16_t pitch_deg = g_current_pitch_x10 / 10;
    if (ABS(pitch_deg) > MAX_PITCH_DEG) {
        if (!g_faults.bits.pitch_exceeded) {
            g_faults.bits.pitch_exceeded = 1;
            log_event(EVT_PITCH_EXCEEDED, pitch_deg, 0);
        }
    }
    
    // 7. Trigger emergency if any critical fault
    // Critical faults: signal_lost, low_battery, leak, depth_exceeded, pitch_exceeded
    uint16_t critical_mask = 0x001F;  // First 5 bits
    if ((g_faults.all & critical_mask) && !g_emergency) {
        trigger_emergency_blow(EVT_EMERGENCY_BLOW);
        g_emergency = true;
    }
    
    // 8. Heartbeat LED
    uint32_t blink_rate = g_emergency ? 100 : 500;  // Fast blink if emergency
    if ((now_ms - last_led_toggle_ms) >= blink_rate) {
        led_state = !led_state;
        gpio_put(PIN_LED_STATUS, led_state);
        last_led_toggle_ms = now_ms;
    }
}

FaultFlags_t safety_monitor_get_faults(void) {
    return g_faults;
}

bool safety_monitor_is_emergency(void) {
    return g_emergency;
}
```

### Emergency Blow

```c
// emergency.h
#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "types.h"

void trigger_emergency_blow(EventCode_t reason);
void emergency_blow_run(void);  // Called continuously in emergency state
bool is_emergency_active(void);

#endif

// emergency.c
#include "emergency.h"
#include "pump.h"
#include "valve.h"
#include "servo.h"
#include "log.h"

static volatile bool emergency_active = false;
static EventCode_t emergency_reason = EVT_NONE;

void trigger_emergency_blow(EventCode_t reason) {
    // Atomic set - cannot be undone
    emergency_active = true;
    emergency_reason = reason;
    
    // Immediate actions
    // 1. Open vent valve (allow air to escape from ballast)
    valve_open();
    
    // 2. Run pump in reverse at full speed (expel water)
    pump_set_speed(-100);
    
    // 3. Full up on all control surfaces
    servo_set_position(SERVO_RUDDER, 0);       // Neutral rudder
    servo_set_position(SERVO_BOWPLANE, 100);   // Full up
    servo_set_position(SERVO_STERNPLANE, 100); // Full up
    
    // 4. Log the event
    log_event(reason, 0, 0);
}

void emergency_blow_run(void) {
    if (!emergency_active) return;
    
    // Continuously ensure emergency state
    // (in case something tries to override)
    valve_open();
    pump_set_speed(-100);
    servo_set_position(SERVO_BOWPLANE, 100);
    servo_set_position(SERVO_STERNPLANE, 100);
}

bool is_emergency_active(void) {
    return emergency_active;
}

// Assert failure handler
void p10_assert_fail(const char* file, int line, const char* cond) {
    // Log as much as we can
    log_fatal("ASSERT FAIL: %s:%d: %s", file, line, cond);
    
    // Trigger emergency
    trigger_emergency_blow(EVT_ASSERT_FAIL);
    
    // Spin forever (watchdog will reset if safety monitor stops)
    while (1) {
        emergency_blow_run();
        sleep_ms(10);
    }
}
```

### Fault Logging (Survives Soft Reset)

```c
// log.h
#ifndef LOG_H
#define LOG_H

#include "types.h"

void log_init(void);
void log_event(EventCode_t code, uint8_t param1, uint8_t param2);
void log_fatal(const char* fmt, ...);
EventLogEntry_t* log_get_entries(uint8_t* count);
void log_dump_to_serial(void);

#endif

// log.c
#include "log.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdarg.h>

// Place in .uninitialized_data section to survive soft reset
static EventLog_t __attribute__((section(".uninitialized_data"))) g_log;
static uint32_t __attribute__((section(".uninitialized_data"))) g_log_magic;

#define LOG_MAGIC 0xDEADBEEF

void log_init(void) {
    // Check if log survived reset
    if (g_log_magic != LOG_MAGIC) {
        // First boot or hard reset - initialize
        g_log.head = 0;
        g_log.count = 0;
        g_log_magic = LOG_MAGIC;
    }
    
    // Log boot event
    log_event(EVT_BOOT, 0, 0);
}

void log_event(EventCode_t code, uint8_t param1, uint8_t param2) {
    EventLogEntry_t* entry = &g_log.entries[g_log.head];
    
    entry->timestamp_ms = to_ms_since_boot(get_absolute_time());
    entry->code = code;
    entry->param1 = param1;
    entry->param2 = param2;
    
    g_log.head = (g_log.head + 1) % EVENT_LOG_SIZE;
    if (g_log.count < EVENT_LOG_SIZE) {
        g_log.count++;
    }
}

void log_fatal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    printf("[FATAL] ");
    vprintf(fmt, args);
    printf("\n");
    
    va_end(args);
}

EventLogEntry_t* log_get_entries(uint8_t* count) {
    *count = g_log.count;
    return g_log.entries;
}

void log_dump_to_serial(void) {
    printf("=== Event Log (%d entries) ===\n", g_log.count);
    
    static const char* event_names[] = {
        "NONE", "BOOT", "INIT_COMPLETE", "STATE_CHANGE", "MODE_CHANGE",
        "SIGNAL_LOST", "SIGNAL_RESTORED", "LOW_BATTERY", "LEAK_DETECTED",
        "DEPTH_EXCEEDED", "PITCH_EXCEEDED", "EMERGENCY_BLOW", 
        "WATCHDOG_RESET", "ASSERT_FAIL", "I2C_ERROR", "SENSOR_FAULT"
    };
    
    uint8_t idx = (g_log.head + EVENT_LOG_SIZE - g_log.count) % EVENT_LOG_SIZE;
    for (uint8_t i = 0; i < g_log.count; i++) {
        EventLogEntry_t* e = &g_log.entries[idx];
        const char* name = (e->code < EVT_COUNT) ? event_names[e->code] : "UNKNOWN";
        printf("%8lu ms: %s (0x%02X, 0x%02X)\n", 
               e->timestamp_ms, name, e->param1, e->param2);
        idx = (idx + 1) % EVENT_LOG_SIZE;
    }
    printf("=============================\n");
}
```

---

## Dual-Core Coordination

### Core 0 Main (Safety)

```c
// core0_main.c
#include "safety_monitor.h"
#include "emergency.h"
#include "log.h"

void core0_main(void) {
    // Initialize safety systems
    safety_monitor_init();
    log_init();
    
    // 100 Hz safety loop
    const uint32_t loop_period_us = 10000;  // 10 ms
    uint32_t next_loop_us = time_us_32();
    
    while (1) {
        // Run safety checks
        safety_monitor_run();
        
        // If in emergency, run blow sequence
        if (is_emergency_active()) {
            emergency_blow_run();
        }
        
        // Maintain loop timing
        next_loop_us += loop_period_us;
        int32_t sleep_us = next_loop_us - time_us_32();
        if (sleep_us > 0) {
            sleep_us(sleep_us);
        }
    }
}
```

### Core 1 Main (Control)

```c
// core1_main.c
#include "rc_input.h"
#include "pressure_sensor.h"
#include "imu.h"
#include "depth_ctrl.h"
#include "pitch_ctrl.h"
#include "ballast_ctrl.h"
#include "state_machine.h"
#include "safety_monitor.h"

void core1_main(void) {
    // Initialize drivers
    rc_input_init();
    pressure_sensor_init();
    imu_init();
    pump_init();
    valve_init();
    servo_init();
    battery_init();
    leak_init();
    
    // Initialize controllers
    DepthController_t depth_ctrl;
    PitchController_t pitch_ctrl;
    BallastController_t ballast_ctrl;
    StateMachine_t state_machine;
    
    depth_ctrl_init(&depth_ctrl);
    pitch_ctrl_init(&pitch_ctrl);
    ballast_ctrl_init(&ballast_ctrl);
    state_machine_init(&state_machine);
    
    log_event(EVT_INIT_COMPLETE, 0, 0);
    
    // 50 Hz control loop
    const uint32_t loop_period_us = 20000;  // 20 ms
    uint32_t next_loop_us = time_us_32();
    uint32_t last_loop_us = next_loop_us;
    
    while (1) {
        uint32_t now_us = time_us_32();
        float dt = (now_us - last_loop_us) / 1000000.0f;
        last_loop_us = now_us;
        
        // Skip if in emergency (Core 0 handles it)
        if (is_emergency_active()) {
            sleep_ms(100);
            continue;
        }
        
        // Read sensors
        RcFrame_t rc;
        DepthReading_t depth;
        AttitudeReading_t attitude;
        
        rc_input_read(&rc);
        pressure_sensor_read(&depth);
        imu_read(&attitude);
        
        // Update safety monitor with current values
        if (rc.valid) {
            safety_update_rc_time(rc.timestamp_ms);
        }
        if (depth.valid) {
            safety_update_depth(depth.depth_cm);
        }
        if (attitude.valid) {
            safety_update_pitch(attitude.pitch_deg_x10);
        }
        
        // Process state machine
        Command_t cmd = rc_to_command(&rc);
        state_machine_process(&state_machine, cmd, depth.depth_cm, 
                              to_ms_since_boot(get_absolute_time()));
        
        // Run controllers based on state
        MainState_t state = state_machine_get_state(&state_machine);
        
        if (state == STATE_SUBMERGED_DEPTH_HOLD) {
            int8_t ballast_cmd = depth_ctrl_update(&depth_ctrl, 
                                                    depth.depth_cm, dt);
            ballast_ctrl_set_target(&ballast_ctrl, ballast_cmd);
        }
        
        // Always run pitch stabilization when submerged
        if (state == STATE_SUBMERGED_MANUAL || 
            state == STATE_SUBMERGED_DEPTH_HOLD) {
            int8_t pitch_cmd = pitch_ctrl_update(&pitch_ctrl,
                                                  attitude.pitch_deg_x10, dt);
            servo_set_position(SERVO_BOWPLANE, pitch_cmd);
        }
        
        // Update ballast
        ballast_ctrl_update(&ballast_ctrl, 
                           to_ms_since_boot(get_absolute_time()));
        
        // Manual control inputs
        ControlInputs_t inputs;
        rc_to_control(&rc, &inputs);
        servo_set_position(SERVO_RUDDER, inputs.rudder);
        
        // Maintain loop timing
        next_loop_us += loop_period_us;
        int32_t sleep_us = next_loop_us - time_us_32();
        if (sleep_us > 0) {
            sleep_us(sleep_us);
        }
    }
}
```

### Main Entry Point

```c
// main.c
#include "pico/stdlib.h"
#include "pico/multicore.h"

extern void core0_main(void);
extern void core1_main(void);

int main(void) {
    // Initialize stdio for debug output
    stdio_init_all();
    
    // Launch Core 1
    multicore_launch_core1(core1_main);
    
    // Core 0 runs safety monitor
    core0_main();
    
    // Never reached
    return 0;
}
```
