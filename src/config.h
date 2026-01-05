#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PIN ASSIGNMENTS
// ============================================================

// RC Input (PIO0) - 6 channels
#define PIN_RC_CH1          0   // Throttle
#define PIN_RC_CH2          1   // Rudder
#define PIN_RC_CH3          2   // Elevator
#define PIN_RC_CH4          3   // Aux/Ballast
#define PIN_RC_CH5          4   // Mode switch
#define PIN_RC_CH6          5   // Emergency
#ifndef RC_CHANNEL_COUNT
#define RC_CHANNEL_COUNT    6
#endif

// I2C0 - Sensors
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9
#define I2C_PORT            i2c0
#define I2C_BAUDRATE        400000  // 400 kHz

// I2C Addresses
#define MS5837_ADDR         0x76
#define MPU6050_ADDR        0x68

// PWM Outputs - Servos
#define PIN_SERVO_RUDDER    10
#define PIN_SERVO_BOWPLANE  11
#define PIN_SERVO_STERNPLANE 12
#define SERVO_PWM_FREQ      50  // 50 Hz standard servo

// PWM Output - Pump
#define PIN_PUMP_PWM        14
#define PIN_PUMP_DIR        15
#define PUMP_PWM_FREQ       1000  // 1 kHz

// Digital Outputs
#define PIN_VALVE           13
#define PIN_LED_STATUS      25  // Onboard LED

// Analog Inputs
#define PIN_BATTERY_ADC     26  // ADC0
#define PIN_LEAK_ADC        27  // ADC1 (optional)

// Digital Inputs
#define PIN_LEAK_DETECT     16
#define PIN_DEPTH_LIMIT     17  // Optional hardware limit

// ============================================================
// TIMING CONSTANTS
// ============================================================

#define CONTROL_LOOP_HZ     50      // Core 1 control loop
#define SAFETY_LOOP_HZ      100     // Core 0 safety loop
#define CONTROL_LOOP_US     (1000000 / CONTROL_LOOP_HZ)
#define SAFETY_LOOP_US      (1000000 / SAFETY_LOOP_HZ)

// ============================================================
// SAFETY LIMITS
// ============================================================

#define SIGNAL_TIMEOUT_MS   3000    // RC signal loss timeout
#define MAX_DEPTH_CM        300     // 3 meters default
#define MAX_PITCH_DEG       45      // ±45 degrees
#define MIN_BATTERY_MV      6400    // 6.4V for 2S LiPo
#define WATCHDOG_TIMEOUT_MS 1000    // 1 second watchdog

// ============================================================
// RC CALIBRATION
// ============================================================

#define RC_PWM_MIN          1000    // Minimum valid PWM (µs)
#define RC_PWM_MAX          2000    // Maximum valid PWM (µs)
#define RC_PWM_CENTER       1500    // Center position (µs)
#define RC_DEADBAND         50      // Deadband around center (µs)

// ============================================================
// SERVO CALIBRATION
// ============================================================

#define SERVO_PWM_MIN       1000    // Full one direction (µs)
#define SERVO_PWM_MAX       2000    // Full other direction (µs)
#define SERVO_PWM_CENTER    1500    // Center position (µs)

// ============================================================
// PID DEFAULTS
// ============================================================

#define PID_DEPTH_KP        2.0f
#define PID_DEPTH_KI        0.1f
#define PID_DEPTH_KD        0.5f

#define PID_PITCH_KP        1.5f
#define PID_PITCH_KI        0.05f
#define PID_PITCH_KD        0.3f

// ============================================================
// BATTERY VOLTAGE DIVIDER
// ============================================================

// Voltage divider: R1 = 10k (high side), R2 = 3.3k (low side)
// Ratio = R2 / (R1 + R2) = 3.3 / 13.3 = 0.248
// ADC sees: Vbatt * 0.248
// To convert: Vbatt = ADC_mV / 0.248 = ADC_mV * 4.03
#define BATTERY_DIVIDER_MULT    403     // × 0.01
#define BATTERY_DIVIDER_DIV     100

#endif // CONFIG_H
