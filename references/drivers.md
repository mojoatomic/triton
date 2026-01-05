# Driver Implementation Reference

Detailed implementation guide for all hardware drivers.

## RC Input Driver (PIO-based)

### Overview

Uses RP2040 PIO for accurate PWM measurement. PIO runs independently of CPU, ensuring precise timing even under CPU load.

### PIO Program (pwm_capture.pio)

```pio
; PWM capture - measures pulse width in microseconds
; Input: GPIO pin
; Output: pulse width count to FIFO

.program pwm_capture
.wrap_target
    wait 0 pin 0        ; Wait for low
    wait 1 pin 0        ; Wait for rising edge
    mov x, ~null        ; Load max count into X
count_high:
    jmp pin, continue   ; If still high, continue counting
    jmp done            ; If low, we're done
continue:
    jmp x--, count_high [1]  ; Decrement X, 2 cycles per iteration
done:
    mov isr, ~x         ; Invert X to get count
    push noblock        ; Push to FIFO
.wrap

% c-sdk {
static inline void pwm_capture_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_sm_config c = pwm_capture_program_get_default_config(offset);
    sm_config_set_jmp_pin(&c, pin);
    sm_config_set_in_pins(&c, pin);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    
    // Clock divider for 1 µs resolution
    // 125 MHz / 125 = 1 MHz = 1 µs per count
    // But we have 2 cycles per count, so divide by 62.5
    sm_config_set_clkdiv(&c, 62.5f);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
%}
```

### C Driver

```c
// rc_input.h
#ifndef RC_INPUT_H
#define RC_INPUT_H

#include "types.h"

error_t rc_input_init(void);
error_t rc_input_read(RcFrame_t* frame);
bool rc_input_is_valid(void);
uint32_t rc_input_get_last_valid_ms(void);

#endif

// rc_input.c
#include "rc_input.h"
#include "config.h"
#include "hardware/pio.h"
#include "pwm_capture.pio.h"

static PIO pio = pio0;
static uint sm_base;
static uint program_offset;
static RcFrame_t last_frame;
static uint32_t last_valid_ms;

error_t rc_input_init(void) {
    P10_ASSERT(pio != NULL);
    
    // Load PIO program
    program_offset = pio_add_program(pio, &pwm_capture_program);
    
    // Initialize state machines for each channel
    static const uint pins[RC_CHANNEL_COUNT] = {
        PIN_RC_CH1, PIN_RC_CH2, PIN_RC_CH3,
        PIN_RC_CH4, PIN_RC_CH5, PIN_RC_CH6
    };
    
    for (uint i = 0; i < RC_CHANNEL_COUNT; i++) {
        uint sm = pio_claim_unused_sm(pio, true);
        if (i == 0) sm_base = sm;
        pwm_capture_program_init(pio, sm, program_offset, pins[i]);
    }
    
    return ERR_NONE;
}

error_t rc_input_read(RcFrame_t* frame) {
    P10_ASSERT(frame != NULL);
    
    bool all_valid = true;
    
    for (uint i = 0; i < RC_CHANNEL_COUNT; i++) {
        if (!pio_sm_is_rx_fifo_empty(pio, sm_base + i)) {
            uint32_t count = pio_sm_get_blocking(pio, sm_base + i);
            
            // Validate pulse width
            if (count >= RC_PWM_MIN && count <= RC_PWM_MAX) {
                frame->channels[i] = (uint16_t)count;
            } else {
                all_valid = false;
            }
        }
    }
    
    frame->timestamp_ms = to_ms_since_boot(get_absolute_time());
    frame->valid = all_valid;
    
    if (all_valid) {
        last_frame = *frame;
        last_valid_ms = frame->timestamp_ms;
    }
    
    return ERR_NONE;
}

bool rc_input_is_valid(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    return (now - last_valid_ms) < SIGNAL_TIMEOUT_MS;
}

uint32_t rc_input_get_last_valid_ms(void) {
    return last_valid_ms;
}
```

---

## Pressure Sensor Driver (MS5837)

### Overview

The MS5837-30BA is a high-resolution pressure sensor with I2C interface. It requires calibration data read from PROM and temperature compensation.

### Register Map

| Command | Value | Description |
|---------|-------|-------------|
| RESET | 0x1E | Reset device |
| PROM_READ | 0xA0-0xAE | Read calibration data (7 words) |
| CONVERT_D1 | 0x40-0x48 | Start pressure conversion |
| CONVERT_D2 | 0x50-0x58 | Start temperature conversion |
| ADC_READ | 0x00 | Read ADC result |

OSR (oversampling) settings in command low nibble: 0=256, 2=512, 4=1024, 6=2048, 8=4096

### C Driver

```c
// pressure_sensor.h
#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "types.h"

error_t pressure_sensor_init(void);
error_t pressure_sensor_read(DepthReading_t* reading);

#endif

// pressure_sensor.c
#include "pressure_sensor.h"
#include "config.h"
#include "hardware/i2c.h"

#define CMD_RESET       0x1E
#define CMD_PROM_BASE   0xA0
#define CMD_CONV_D1     0x48    // OSR 4096
#define CMD_CONV_D2     0x58    // OSR 4096
#define CMD_ADC_READ    0x00
#define CONV_TIME_MS    20      // Conversion time for OSR 4096

static uint16_t cal[7];         // Calibration coefficients
static bool initialized = false;

// Fluid density for fresh water (kg/m³)
#define FLUID_DENSITY   997

static error_t read_prom(void) {
    uint8_t cmd, data[2];
    
    for (int i = 0; i < 7; i++) {
        cmd = CMD_PROM_BASE + (i * 2);
        int ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, true);
        if (ret < 0) return ERR_I2C;
        
        ret = i2c_read_blocking(I2C_INSTANCE, MS5837_ADDR, data, 2, false);
        if (ret < 0) return ERR_I2C;
        
        cal[i] = (data[0] << 8) | data[1];
    }
    
    return ERR_NONE;
}

error_t pressure_sensor_init(void) {
    uint8_t cmd = CMD_RESET;
    
    // Reset device
    int ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, false);
    if (ret < 0) return ERR_I2C;
    
    sleep_ms(10);  // Wait for reset
    
    // Read calibration data
    error_t err = read_prom();
    if (err != ERR_NONE) return err;
    
    initialized = true;
    return ERR_NONE;
}

error_t pressure_sensor_read(DepthReading_t* reading) {
    P10_ASSERT(reading != NULL);
    P10_ASSERT(initialized);
    
    uint8_t cmd, data[3];
    uint32_t D1, D2;
    
    // Start pressure conversion
    cmd = CMD_CONV_D1;
    int ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, false);
    if (ret < 0) return ERR_I2C;
    sleep_ms(CONV_TIME_MS);
    
    // Read pressure ADC
    cmd = CMD_ADC_READ;
    ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, true);
    if (ret < 0) return ERR_I2C;
    ret = i2c_read_blocking(I2C_INSTANCE, MS5837_ADDR, data, 3, false);
    if (ret < 0) return ERR_I2C;
    D1 = (data[0] << 16) | (data[1] << 8) | data[2];
    
    // Start temperature conversion
    cmd = CMD_CONV_D2;
    ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, false);
    if (ret < 0) return ERR_I2C;
    sleep_ms(CONV_TIME_MS);
    
    // Read temperature ADC
    cmd = CMD_ADC_READ;
    ret = i2c_write_blocking(I2C_INSTANCE, MS5837_ADDR, &cmd, 1, true);
    if (ret < 0) return ERR_I2C;
    ret = i2c_read_blocking(I2C_INSTANCE, MS5837_ADDR, data, 3, false);
    if (ret < 0) return ERR_I2C;
    D2 = (data[0] << 16) | (data[1] << 8) | data[2];
    
    // Calculate temperature compensated pressure
    // (MS5837 datasheet algorithm)
    int32_t dT = D2 - ((uint32_t)cal[5] << 8);
    int32_t TEMP = 2000 + ((int64_t)dT * cal[6] >> 23);
    
    int64_t OFF = ((int64_t)cal[2] << 16) + (((int64_t)cal[4] * dT) >> 7);
    int64_t SENS = ((int64_t)cal[1] << 15) + (((int64_t)cal[3] * dT) >> 8);
    int32_t P = (((D1 * SENS) >> 21) - OFF) >> 13;
    
    // P is in units of 10 Pa (0.1 mbar)
    // Convert to depth: depth = (P - P_atm) / (rho * g)
    // At surface, P ≈ 101325 Pa = 10132.5 units
    // rho * g ≈ 9780 Pa/m for fresh water
    // depth_cm = (P - 10133) * 100 / 978
    
    int32_t P_surface = 10133;  // ~1 atm in sensor units
    reading->depth_cm = ((P - P_surface) * 100) / 978;
    reading->temp_c_x10 = TEMP / 10;  // Convert to 0.1°C units
    reading->timestamp_ms = to_ms_since_boot(get_absolute_time());
    reading->valid = true;
    
    return ERR_NONE;
}
```

---

## IMU Driver (MPU-6050)

### Overview

The MPU-6050 provides 3-axis accelerometer and gyroscope. Use complementary filter for stable pitch/roll.

### Register Map (Key Registers)

| Register | Address | Description |
|----------|---------|-------------|
| PWR_MGMT_1 | 0x6B | Power management |
| SMPLRT_DIV | 0x19 | Sample rate divider |
| CONFIG | 0x1A | DLPF config |
| GYRO_CONFIG | 0x1B | Gyro range |
| ACCEL_CONFIG | 0x1C | Accel range |
| ACCEL_XOUT_H | 0x3B | Accel X high byte |
| GYRO_XOUT_H | 0x43 | Gyro X high byte |

### C Driver

```c
// imu.h
#ifndef IMU_H
#define IMU_H

#include "types.h"

error_t imu_init(void);
error_t imu_read(AttitudeReading_t* reading);

#endif

// imu.c
#include "imu.h"
#include "config.h"
#include "hardware/i2c.h"
#include <math.h>

#define REG_PWR_MGMT_1  0x6B
#define REG_SMPLRT_DIV  0x19
#define REG_CONFIG      0x1A
#define REG_GYRO_CFG    0x1B
#define REG_ACCEL_CFG   0x1C
#define REG_ACCEL_XOUT  0x3B
#define REG_GYRO_XOUT   0x43

// Complementary filter coefficient
#define ALPHA           0.98f

static float pitch_deg = 0.0f;
static float roll_deg = 0.0f;
static uint32_t last_update_us = 0;
static bool initialized = false;

static error_t write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    int ret = i2c_write_blocking(I2C_INSTANCE, MPU6050_ADDR, buf, 2, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

static error_t read_regs(uint8_t reg, uint8_t* data, uint8_t len) {
    int ret = i2c_write_blocking(I2C_INSTANCE, MPU6050_ADDR, &reg, 1, true);
    if (ret < 0) return ERR_I2C;
    ret = i2c_read_blocking(I2C_INSTANCE, MPU6050_ADDR, data, len, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

error_t imu_init(void) {
    error_t err;
    
    // Wake up device
    err = write_reg(REG_PWR_MGMT_1, 0x00);
    if (err != ERR_NONE) return err;
    sleep_ms(100);
    
    // Set sample rate to 100 Hz (8 kHz / (1 + 79))
    err = write_reg(REG_SMPLRT_DIV, 79);
    if (err != ERR_NONE) return err;
    
    // Set DLPF to ~44 Hz bandwidth
    err = write_reg(REG_CONFIG, 0x03);
    if (err != ERR_NONE) return err;
    
    // Gyro range: ±500 °/s
    err = write_reg(REG_GYRO_CFG, 0x08);
    if (err != ERR_NONE) return err;
    
    // Accel range: ±4g
    err = write_reg(REG_ACCEL_CFG, 0x08);
    if (err != ERR_NONE) return err;
    
    last_update_us = time_us_32();
    initialized = true;
    return ERR_NONE;
}

error_t imu_read(AttitudeReading_t* reading) {
    P10_ASSERT(reading != NULL);
    P10_ASSERT(initialized);
    
    uint8_t data[14];
    error_t err = read_regs(REG_ACCEL_XOUT, data, 14);
    if (err != ERR_NONE) return err;
    
    // Parse raw values (big-endian)
    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];
    // data[6-7] = temperature (ignored)
    int16_t gx = (data[8] << 8) | data[9];
    int16_t gy = (data[10] << 8) | data[11];
    // int16_t gz = (data[12] << 8) | data[13];  // Not used for pitch/roll
    
    // Convert to physical units
    // Accel: ±4g range = 8192 LSB/g
    float ax_g = ax / 8192.0f;
    float ay_g = ay / 8192.0f;
    float az_g = az / 8192.0f;
    
    // Gyro: ±500 °/s range = 65.5 LSB/(°/s)
    float gx_dps = gx / 65.5f;
    float gy_dps = gy / 65.5f;
    
    // Calculate dt
    uint32_t now_us = time_us_32();
    float dt = (now_us - last_update_us) / 1000000.0f;
    last_update_us = now_us;
    
    // Clamp dt to reasonable range
    if (dt <= 0.0f || dt > 0.5f) dt = 0.02f;
    
    // Accelerometer-based angles
    float accel_pitch = atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) * 180.0f / M_PI;
    float accel_roll = atan2f(ay_g, az_g) * 180.0f / M_PI;
    
    // Complementary filter
    pitch_deg = ALPHA * (pitch_deg + gy_dps * dt) + (1.0f - ALPHA) * accel_pitch;
    roll_deg = ALPHA * (roll_deg + gx_dps * dt) + (1.0f - ALPHA) * accel_roll;
    
    // Output in 0.1° units
    reading->pitch_deg_x10 = (int16_t)(pitch_deg * 10.0f);
    reading->roll_deg_x10 = (int16_t)(roll_deg * 10.0f);
    reading->timestamp_ms = to_ms_since_boot(get_absolute_time());
    reading->valid = true;
    
    return ERR_NONE;
}
```

---

## Pump Driver

```c
// pump.h
#ifndef PUMP_H
#define PUMP_H

#include "types.h"

error_t pump_init(void);
void pump_set_speed(int8_t speed);  // -100 to +100
void pump_stop(void);

#endif

// pump.c
#include "pump.h"
#include "config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

static uint slice_num;

error_t pump_init(void) {
    // Direction pin
    gpio_init(PIN_PUMP_DIR);
    gpio_set_dir(PIN_PUMP_DIR, GPIO_OUT);
    gpio_put(PIN_PUMP_DIR, false);
    
    // PWM pin
    gpio_set_function(PIN_PUMP_PWM, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(PIN_PUMP_PWM);
    
    // Configure PWM: 125 MHz / (125 * 1000) = 1 kHz
    pwm_set_wrap(slice_num, 999);
    pwm_set_clkdiv(slice_num, 125.0f);
    pwm_set_enabled(slice_num, true);
    
    pump_stop();
    return ERR_NONE;
}

void pump_set_speed(int8_t speed) {
    // Clamp to valid range
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;
    
    // Set direction
    gpio_put(PIN_PUMP_DIR, speed >= 0);
    
    // Set PWM duty cycle
    uint16_t duty = (uint16_t)(abs(speed) * 10);  // 0-1000
    pwm_set_gpio_level(PIN_PUMP_PWM, duty);
}

void pump_stop(void) {
    pwm_set_gpio_level(PIN_PUMP_PWM, 0);
}
```

---

## Valve Driver

```c
// valve.h
#ifndef VALVE_H
#define VALVE_H

#include "types.h"

error_t valve_init(void);
void valve_open(void);
void valve_close(void);
bool valve_is_open(void);

#endif

// valve.c
#include "valve.h"
#include "config.h"
#include "hardware/gpio.h"

static bool is_open = false;

error_t valve_init(void) {
    gpio_init(PIN_VALVE);
    gpio_set_dir(PIN_VALVE, GPIO_OUT);
    valve_close();
    return ERR_NONE;
}

void valve_open(void) {
    gpio_put(PIN_VALVE, true);
    is_open = true;
}

void valve_close(void) {
    gpio_put(PIN_VALVE, false);
    is_open = false;
}

bool valve_is_open(void) {
    return is_open;
}
```

---

## Servo Driver

```c
// servo.h
#ifndef SERVO_H
#define SERVO_H

#include "types.h"

typedef enum {
    SERVO_RUDDER = 0,
    SERVO_BOWPLANE,
    SERVO_STERNPLANE,
    SERVO_COUNT
} ServoChannel_t;

error_t servo_init(void);
void servo_set_position(ServoChannel_t channel, int8_t position);  // -100 to +100

#endif

// servo.c
#include "servo.h"
#include "config.h"
#include "hardware/pwm.h"

static const uint servo_pins[SERVO_COUNT] = {
    PIN_SERVO_RUDDER,
    PIN_SERVO_BOWPLANE,
    PIN_SERVO_STERNPLANE
};

error_t servo_init(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        gpio_set_function(servo_pins[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(servo_pins[i]);
        
        // 50 Hz servo PWM: 125 MHz / (125 * 20000) = 50 Hz
        pwm_set_wrap(slice, 19999);
        pwm_set_clkdiv(slice, 125.0f);
        pwm_set_enabled(slice, true);
        
        // Center position
        servo_set_position((ServoChannel_t)i, 0);
    }
    return ERR_NONE;
}

void servo_set_position(ServoChannel_t channel, int8_t position) {
    P10_ASSERT(channel < SERVO_COUNT);
    
    // Clamp position
    if (position > 100) position = 100;
    if (position < -100) position = -100;
    
    // Map -100..+100 to 1000..2000 µs
    // At 50 Hz with wrap=19999, 1 count = 1 µs
    uint16_t pulse_us = SERVO_PWM_CENTER + (position * 5);  // ±500 µs range
    
    pwm_set_gpio_level(servo_pins[channel], pulse_us);
}
```

---

## Battery Monitor

```c
// battery.h
#ifndef BATTERY_H
#define BATTERY_H

#include "types.h"

error_t battery_init(void);
uint16_t battery_read_mv(void);
bool battery_is_low(void);

#endif

// battery.c
#include "battery.h"
#include "config.h"
#include "hardware/adc.h"

error_t battery_init(void) {
    adc_init();
    adc_gpio_init(PIN_BATTERY_ADC);
    return ERR_NONE;
}

uint16_t battery_read_mv(void) {
    adc_select_input(PIN_BATTERY_ADC - 26);  // ADC0 = GPIO26
    uint16_t raw = adc_read();  // 12-bit: 0-4095
    
    // Convert to mV at ADC pin: 3300 mV / 4096 * raw
    uint32_t adc_mv = (raw * 3300) / 4096;
    
    // Apply voltage divider correction
    uint32_t batt_mv = (adc_mv * BATTERY_DIVIDER_MULT) / BATTERY_DIVIDER_DIV;
    
    return (uint16_t)batt_mv;
}

bool battery_is_low(void) {
    return battery_read_mv() < MIN_BATTERY_MV;
}
```

---

## Leak Detector

```c
// leak.h
#ifndef LEAK_H
#define LEAK_H

#include "types.h"

error_t leak_init(void);
bool leak_detected(void);

#endif

// leak.c
#include "leak.h"
#include "config.h"
#include "hardware/gpio.h"

static volatile bool leak_flag = false;

static void leak_isr(uint gpio, uint32_t events) {
    (void)gpio;
    (void)events;
    leak_flag = true;
}

error_t leak_init(void) {
    gpio_init(PIN_LEAK_DETECT);
    gpio_set_dir(PIN_LEAK_DETECT, GPIO_IN);
    gpio_pull_down(PIN_LEAK_DETECT);
    
    // Interrupt on rising edge (water detected)
    gpio_set_irq_enabled_with_callback(PIN_LEAK_DETECT, 
        GPIO_IRQ_EDGE_RISE, true, &leak_isr);
    
    return ERR_NONE;
}

bool leak_detected(void) {
    return leak_flag || gpio_get(PIN_LEAK_DETECT);
}
```
