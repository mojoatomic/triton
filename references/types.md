# Type Definitions Reference

All shared type definitions for the RC Submarine Controller.

## types.h

```c
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================
// ERROR CODES
// ============================================================

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_I2C,
    ERR_SPI,
    ERR_BUSY,
    ERR_NOT_READY,
    ERR_OVERFLOW,
    ERR_CHECKSUM,
    ERR_LIMIT_EXCEEDED,
    ERR_HARDWARE,
    ERR_STATE,
    ERR_COUNT
} error_t;

// ============================================================
// EVENT CODES (for logging)
// ============================================================

typedef enum {
    EVT_NONE = 0,
    EVT_BOOT,
    EVT_INIT_COMPLETE,
    EVT_STATE_CHANGE,
    EVT_MODE_CHANGE,
    EVT_SIGNAL_LOST,
    EVT_SIGNAL_RESTORED,
    EVT_LOW_BATTERY,
    EVT_LEAK_DETECTED,
    EVT_DEPTH_EXCEEDED,
    EVT_PITCH_EXCEEDED,
    EVT_EMERGENCY_BLOW,
    EVT_WATCHDOG_RESET,
    EVT_ASSERT_FAIL,
    EVT_I2C_ERROR,
    EVT_SENSOR_FAULT,
    EVT_COUNT
} EventCode_t;

// ============================================================
// RC INPUT
// ============================================================

#define RC_CHANNEL_COUNT 6

typedef struct {
    uint16_t channels[RC_CHANNEL_COUNT];  // Raw PWM values (1000-2000 µs)
    uint32_t timestamp_ms;
    bool valid;
} RcFrame_t;

// ============================================================
// SENSOR READINGS
// ============================================================

typedef struct {
    int32_t depth_cm;         // Depth in centimeters (0 = surface)
    int32_t temp_c_x10;       // Temperature × 10 (e.g., 250 = 25.0°C)
    uint32_t timestamp_ms;
    bool valid;
} DepthReading_t;

typedef struct {
    int16_t pitch_deg_x10;    // Pitch × 10 (positive = nose up)
    int16_t roll_deg_x10;     // Roll × 10 (positive = right side down)
    uint32_t timestamp_ms;
    bool valid;
} AttitudeReading_t;

typedef struct {
    uint16_t voltage_mv;      // Battery voltage in millivolts
    uint8_t percent;          // Estimated charge percentage
    bool low_warning;
    uint32_t timestamp_ms;
} BatteryStatus_t;

// ============================================================
// CONTROL INPUTS (processed from RC)
// ============================================================

typedef struct {
    int8_t throttle;          // -100 to +100
    int8_t rudder;            // -100 to +100
    int8_t elevator;          // -100 to +100
    int8_t ballast;           // -100 to +100
} ControlInputs_t;

// ============================================================
// ACTUATOR OUTPUTS
// ============================================================

typedef struct {
    int8_t pump_speed;        // -100 to +100 (negative = reverse)
    bool valve_open;
    int8_t servo_rudder;      // -100 to +100
    int8_t servo_bowplane;    // -100 to +100
    int8_t servo_sternplane;  // -100 to +100
} ActuatorOutputs_t;

// ============================================================
// SYSTEM STATE
// ============================================================

typedef struct {
    // Current readings
    DepthReading_t depth;
    AttitudeReading_t attitude;
    BatteryStatus_t battery;
    RcFrame_t rc_frame;
    
    // Control state
    ControlInputs_t inputs;
    ActuatorOutputs_t outputs;
    
    // Flags
    bool rc_valid;
    bool leak_detected;
    bool emergency_active;
    
    // Timing
    uint32_t uptime_ms;
    uint32_t last_rc_ms;
} SystemState_t;

// ============================================================
// FAULT FLAGS
// ============================================================

typedef union {
    struct {
        uint16_t signal_lost     : 1;
        uint16_t low_battery     : 1;
        uint16_t leak            : 1;
        uint16_t depth_exceeded  : 1;
        uint16_t pitch_exceeded  : 1;
        uint16_t i2c_error       : 1;
        uint16_t sensor_fault    : 1;
        uint16_t watchdog        : 1;
        uint16_t reserved        : 8;
    } bits;
    uint16_t all;
} FaultFlags_t;

// ============================================================
// EVENT LOG ENTRY
// ============================================================

typedef struct {
    uint32_t timestamp_ms;
    EventCode_t code;
    uint8_t param1;
    uint8_t param2;
} EventLogEntry_t;

#define EVENT_LOG_SIZE 32

typedef struct {
    EventLogEntry_t entries[EVENT_LOG_SIZE];
    uint8_t head;
    uint8_t count;
} EventLog_t;

// ============================================================
// CONFIGURATION (stored in flash)
// ============================================================

typedef struct {
    // Version
    uint16_t version;
    uint16_t checksum;
    
    // Safety limits
    uint16_t max_depth_cm;
    uint8_t max_pitch_deg;
    uint16_t min_battery_mv;
    uint16_t signal_timeout_ms;
    
    // PID gains (× 100 for integer storage)
    int16_t depth_kp;
    int16_t depth_ki;
    int16_t depth_kd;
    int16_t pitch_kp;
    int16_t pitch_ki;
    int16_t pitch_kd;
    
    // Calibration
    int16_t depth_offset_cm;
    int16_t pitch_offset_x10;
    int16_t roll_offset_x10;
    
    // Servo calibration
    uint16_t servo_min_us[3];
    uint16_t servo_max_us[3];
    uint16_t servo_center_us[3];
    
} Config_t;

#define CONFIG_VERSION 1

// ============================================================
// UTILITY MACROS
// ============================================================

#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)    MIN(MAX(x, lo), hi)
#define ABS(x)              ((x) < 0 ? -(x) : (x))
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))

// ============================================================
// ASSERT MACRO (Power of 10)
// ============================================================

#ifndef NDEBUG
#define P10_ASSERT(cond) do { \
    if (!(cond)) { \
        p10_assert_fail(__FILE__, __LINE__, #cond); \
    } \
} while(0)
#else
#define P10_ASSERT(cond) ((void)0)
#endif

// Implemented in safety/emergency.c
void p10_assert_fail(const char* file, int line, const char* cond);

#endif // TYPES_H
```

## Usage Guidelines

### Error Handling Pattern

```c
error_t do_something(void) {
    error_t err;
    
    err = step_one();
    if (err != ERR_NONE) return err;
    
    err = step_two();
    if (err != ERR_NONE) return err;
    
    return ERR_NONE;
}
```

### Sensor Reading Pattern

```c
void control_loop(void) {
    DepthReading_t depth;
    error_t err = pressure_sensor_read(&depth);
    
    if (err != ERR_NONE || !depth.valid) {
        // Handle sensor fault
        log_event(EVT_SENSOR_FAULT, err, 0);
        return;
    }
    
    // Use depth.depth_cm
}
```

### Fault Flag Usage

```c
FaultFlags_t faults = {0};

// Set individual faults
faults.bits.signal_lost = true;
faults.bits.low_battery = true;

// Check any fault
if (faults.all != 0) {
    trigger_emergency_blow(EVT_EMERGENCY_BLOW);
}

// Check specific fault
if (faults.bits.leak) {
    // Handle leak
}
```
