# GPIO Pin Assignments

Complete pin mapping for the RC Submarine Controller.

## Raspberry Pi Pico Pinout

### RC Input (PIO0)

| GPIO | Function | Description |
|------|----------|-------------|
| 0 | RC_CH1 | Throttle |
| 1 | RC_CH2 | Rudder |
| 2 | RC_CH3 | Elevator/Bowplane |
| 3 | RC_CH4 | Aux (Ballast) |
| 4 | RC_CH5 | Mode switch |
| 5 | RC_CH6 | Emergency switch |

### I2C Bus (I2C0)

| GPIO | Function | Description |
|------|----------|-------------|
| 8 | I2C0_SDA | Shared data line |
| 9 | I2C0_SCL | Shared clock line |

Devices on I2C0:
- MS5837 pressure sensor (address 0x76)
- MPU-6050 IMU (address 0x68)

### PWM Outputs

| GPIO | Function | PWM Slice | Description |
|------|----------|-----------|-------------|
| 10 | SERVO_RUDDER | 5A | Rudder servo |
| 11 | SERVO_BOWPLANE | 5B | Bow plane servo |
| 12 | SERVO_STERNPLANE | 6A | Stern plane servo |
| 14 | PUMP_PWM | 7A | Ballast pump speed |

### Digital Outputs

| GPIO | Function | Description |
|------|----------|-------------|
| 13 | VALVE_CTRL | Solenoid valve (active high) |
| 15 | PUMP_DIR | Pump direction (H=fill, L=drain) |
| 25 | LED_STATUS | Onboard LED (heartbeat) |

### Analog Inputs

| GPIO | ADC | Function | Description |
|------|-----|----------|-------------|
| 26 | ADC0 | BATTERY_V | Battery voltage (via 3:1 divider) |
| 27 | ADC1 | LEAK_ANALOG | Leak sensor analog (optional) |

### Digital Inputs

| GPIO | Function | Description |
|------|----------|-------------|
| 16 | LEAK_DETECT | Leak sensor digital (active high) |
| 17 | DEPTH_LIMIT | Hardware depth limit switch (optional) |

### Debug/Spare

| GPIO | Function | Description |
|------|----------|-------------|
| 18-22 | SPARE | Available for expansion |
| UART0 TX | GPIO0 | Conflicts with RC—use USB for debug |
| UART0 RX | GPIO1 | Conflicts with RC—use USB for debug |

## config.h Definition

```c
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
#define RC_CHANNEL_COUNT    6

// I2C0 - Sensors
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9
#define I2C_INSTANCE        i2c0
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
```

## Wiring Notes

### I2C Pull-ups
Both SDA and SCL require 4.7kΩ pull-up resistors to 3.3V. Many breakout modules include these.

### Pump Driver
Use L298N or TB6612 module:
- PIN_PUMP_PWM → ENA (enable/speed)
- PIN_PUMP_DIR → IN1 (direction)
- IN2 tied opposite to IN1 via inverter, or use separate GPIO

### Servo Power
Servos should be powered from a separate 5V BEC, not the Pico's 3.3V. Only signal wires connect to Pico GPIOs.

### Battery Monitoring
```
Vbatt ───[10kΩ]───┬───[3.3kΩ]─── GND
                  │
                  └─── GPIO26 (ADC0)
```

### Leak Sensor
Simple water detection circuit:
```
3.3V ───[10kΩ]───┬─── GPIO16
                 │
              [probes in bilge]
                 │
                GND
```
When water bridges probes, GPIO16 goes HIGH.
