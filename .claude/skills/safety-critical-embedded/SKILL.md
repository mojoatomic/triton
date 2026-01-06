---
name: safety-critical-embedded
description: Safety-critical embedded system design patterns. Use when building dual-core safety monitors, failsafe systems, watchdog implementations, or any system where failure could cause harm. Covers isolation, fault detection, and emergency procedures.
---

# Safety-Critical Embedded Systems

Design patterns for embedded systems where failure modes must be managed and safety cannot be compromised.

## Core Principle: Defense in Depth

No single point of failure. Every safety mechanism has a backup.

```
┌─────────────────────────────────────────────────────────────┐
│                    SAFETY LAYERS                            │
├─────────────────────────────────────────────────────────────┤
│ Layer 1: Software assertions (P10_ASSERT)                   │
│ Layer 2: Runtime fault detection (safety monitor)           │
│ Layer 3: Hardware watchdog (independent timer)              │
│ Layer 4: Physical failsafe (mechanical, electrical)         │
└─────────────────────────────────────────────────────────────┘
```

## Dual-Core Architecture

Isolate safety-critical code from complex logic.

```
┌─────────────────────────────────────────────────────────────┐
│  CORE 0 (Safety)              │  CORE 1 (Application)       │
│  ─────────────────            │  ────────────────────       │
│  • Cannot be blocked          │  • Can be complex           │
│  • Minimal code               │  • Runs control algorithms  │
│  • Feeds watchdog             │  • Reads sensors            │
│  • Monitors faults            │  • Updates actuators        │
│  • Triggers emergency         │  • State machines           │
│  • 100 Hz fixed loop          │  • 50 Hz or variable        │
│                               │                             │
│  If Core 1 dies, Core 0       │  Core 1 checks              │
│  still triggers failsafe      │  is_emergency_active()      │
└─────────────────────────────────────────────────────────────┘
                    ↕ Shared State (volatile, atomic) ↕
```

### Core 0 Requirements

- Maximum 200 lines total
- No blocking calls
- No dynamic memory
- No complex logic
- Fixed execution time
- Always feeds watchdog

```c
// Core 0 main loop - MINIMAL
void core0_main(void) {
    safety_init();
    
    const uint32_t period_us = 10000;  // 100 Hz
    uint32_t next_us = time_us_32();
    
    while (1) {
        watchdog_update();
        safety_monitor_run();
        
        if (g_emergency_active) {
            emergency_maintain();
        }
        
        // Fixed timing
        next_us += period_us;
        while (time_us_32() < next_us) {
            tight_loop_contents();
        }
    }
}
```

### Cross-Core Communication

Use volatile variables with atomic semantics:

```c
// Shared state - declared volatile
static volatile uint32_t g_core1_heartbeat = 0;
static volatile bool g_emergency_active = false;
static volatile FaultFlags_t g_faults = {0};

// Core 1 increments heartbeat each loop
void core1_loop(void) {
    g_core1_heartbeat++;  // Atomic on ARM Cortex-M
    // ... rest of loop
}

// Core 0 detects stall
void check_core1_health(void) {
    static uint32_t last_seen = 0;
    static uint32_t stall_count = 0;
    
    if (g_core1_heartbeat == last_seen) {
        stall_count++;
        if (stall_count > STALL_THRESHOLD) {
            trigger_emergency(FAULT_CORE1_STALL);
        }
    } else {
        stall_count = 0;
    }
    last_seen = g_core1_heartbeat;
}
```

## Fault Detection

### Fault Categories

```c
typedef union {
    struct {
        uint16_t signal_lost     : 1;  // No RC signal
        uint16_t low_battery     : 1;  // Below threshold
        uint16_t leak_detected   : 1;  // Water ingress
        uint16_t over_temp       : 1;  // Thermal limit
        uint16_t depth_exceeded  : 1;  // Beyond max depth
        uint16_t pitch_exceeded  : 1;  // Beyond safe angle
        uint16_t core1_stall     : 1;  // Application hung
        uint16_t watchdog_warn   : 1;  // Near timeout
        uint16_t sensor_fault    : 1;  // Invalid readings
        uint16_t reserved        : 7;
    } bits;
    uint16_t all;
} FaultFlags_t;
```

### Detection Patterns

```c
// Signal loss - timeout based
void check_signal(uint32_t now_ms) {
    if ((now_ms - g_last_valid_signal_ms) > SIGNAL_TIMEOUT_MS) {
        set_fault(FAULT_SIGNAL_LOST);
    }
}

// Threshold - hysteresis to prevent flapping
void check_battery(void) {
    uint16_t mv = adc_read_battery();
    
    if (mv < BATT_CRITICAL_MV) {
        set_fault(FAULT_LOW_BATTERY);
    } else if (mv > BATT_RECOVERY_MV) {
        clear_fault(FAULT_LOW_BATTERY);  // Only if recoverable
    }
}

// Rate of change - detect sudden anomalies
void check_depth_rate(int32_t depth_cm, float dt) {
    static int32_t prev_depth = 0;
    int32_t rate = (depth_cm - prev_depth) / dt;
    
    if (abs(rate) > MAX_DESCENT_RATE_CM_S) {
        set_fault(FAULT_DESCENT_RATE);
    }
    prev_depth = depth_cm;
}
```

## Emergency Procedures

### Atomic Emergency State

Once triggered, emergency cannot be cancelled except by power cycle:

```c
static volatile bool g_emergency_active = false;
static volatile uint8_t g_emergency_reason = 0;

void trigger_emergency(uint8_t reason) {
    // Atomic set - cannot be undone
    g_emergency_active = true;
    g_emergency_reason = reason;
    
    // Immediate safe state
    actuators_safe_state();
    
    // Log for post-mortem
    log_event(EVT_EMERGENCY, reason);
}

bool is_emergency_active(void) {
    return g_emergency_active;
}
```

### Safe State Definition

Define what "safe" means for each actuator:

```c
void actuators_safe_state(void) {
    // Motors: stop
    motor_set_speed(MOTOR_PUMP, 0);
    motor_set_speed(MOTOR_PROP, 0);
    
    // Valves: open (vent pressure)
    valve_open(VALVE_VENT);
    
    // Servos: neutral or safe position
    servo_set(SERVO_RUDDER, SERVO_NEUTRAL);
    servo_set(SERVO_ELEVATOR, SERVO_FULL_UP);
    
    // Outputs: disable high-power
    gpio_put(PIN_ENABLE_POWER, 0);
}
```

### Emergency Maintenance Loop

Continuously enforce safe state in case something tries to override:

```c
void emergency_maintain(void) {
    // Re-assert safe state every cycle
    actuators_safe_state();
    
    // Blink LED fast to indicate emergency
    static uint32_t blink_timer = 0;
    if (++blink_timer % 10 == 0) {
        gpio_xor_mask(1 << PIN_LED);
    }
    
    // Sound alarm if equipped
    buzzer_pattern(PATTERN_SOS);
}
```

## Watchdog Implementation

### Hardware Watchdog

Use the MCU's hardware watchdog, not a software timer:

```c
void safety_init(void) {
    // Enable hardware watchdog - 1 second timeout
    watchdog_enable(1000, true);
}

void safety_monitor_run(void) {
    // Only feed if no faults - deliberate reset on fault
    if (g_faults.all == 0) {
        watchdog_update();
    }
    
    // Check all conditions...
}
```

### Watchdog Starvation

If Core 0 detects an unrecoverable fault, stop feeding the watchdog:

```c
void check_critical_faults(void) {
    uint16_t critical_mask = FAULT_LEAK | FAULT_CORE1_STALL | FAULT_SENSOR;
    
    if (g_faults.all & critical_mask) {
        // Stop feeding watchdog - force reset
        // Log will survive if in persistent memory
        log_event(EVT_WATCHDOG_STARVATION, g_faults.all);
        
        while (1) {
            // Wait for hardware reset
            tight_loop_contents();
        }
    }
}
```

## Startup Sequence

### Power-On Self-Test

```c
bool power_on_self_test(void) {
    bool pass = true;
    
    // Check each subsystem
    pass &= test_memory_integrity();
    pass &= test_watchdog_functional();
    pass &= test_sensors_responding();
    pass &= test_actuators_range();
    pass &= test_communication_link();
    
    if (!pass) {
        // Do not proceed - stay in safe state
        log_event(EVT_POST_FAILED, get_post_failures());
        actuators_safe_state();
    }
    
    return pass;
}
```

### Core Launch Handshake

Verify Core 1 actually started:

```c
#define CORE1_READY_MAGIC 0xC0DE1001

// In main()
int main(void) {
    core0_init();
    
    multicore_launch_core1(core1_main);
    
    // Wait for handshake with timeout
    uint32_t timeout = 0;
    while (!multicore_fifo_rvalid() && timeout < 1000) {
        sleep_ms(1);
        timeout++;
    }
    
    if (timeout >= 1000) {
        trigger_emergency(FAULT_CORE1_LAUNCH);
    }
    
    uint32_t magic = multicore_fifo_pop_blocking();
    if (magic != CORE1_READY_MAGIC) {
        trigger_emergency(FAULT_CORE1_LAUNCH);
    }
    
    // Core 1 is running, start safety monitor
    core0_main();
}

// In core1_main()
void core1_main(void) {
    core1_init();
    
    // Signal ready
    multicore_fifo_push_blocking(CORE1_READY_MAGIC);
    
    // Run application
    application_loop();
}
```

## Testing Safety Systems

### Fault Injection

```c
#ifdef TEST_MODE
void inject_fault(FaultType_t fault) {
    switch (fault) {
        case FAULT_SIGNAL_LOST:
            g_last_valid_signal_ms = 0;  // Force timeout
            break;
        case FAULT_LOW_BATTERY:
            g_test_battery_mv = 5000;  // Force low reading
            break;
        case FAULT_CORE1_STALL:
            // Core 1 test: stop incrementing heartbeat
            break;
    }
}
#endif
```

### Safety Monitor Tests

Every fault condition must have a test proving it triggers emergency:

```c
void test_signal_loss_triggers_emergency(void) {
    // Setup
    g_emergency_active = false;
    g_last_valid_signal_ms = time_ms() - SIGNAL_TIMEOUT_MS - 100;
    
    // Act
    safety_monitor_run();
    
    // Assert
    TEST_ASSERT(g_faults.bits.signal_lost == 1);
    TEST_ASSERT(g_emergency_active == true);
}
```

## Summary

Safety-critical systems require:

1. **Isolation** - Safety code runs independently of application
2. **Simplicity** - Safety code is minimal and verifiable
3. **Redundancy** - Multiple layers detect the same faults
4. **Fail-safe** - Default state is always safe
5. **Testability** - Every fault path is tested
6. **Auditability** - All events are logged for post-mortem

Never compromise safety for features. If in doubt, trigger emergency and surface.
