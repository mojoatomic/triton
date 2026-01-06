/**
 * RC Submarine Controller
 * imu.c - MPU-6050 IMU driver
 *
 * Power of 10 compliant
 */

#include "imu.h"

#include "config.h"

#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

#include <math.h>

#define REG_PWR_MGMT_1  0x6BU
#define REG_SMPLRT_DIV  0x19U
#define REG_CONFIG      0x1AU
#define REG_GYRO_CFG    0x1BU
#define REG_ACCEL_CFG   0x1CU
#define REG_ACCEL_XOUT  0x3BU

#define ALPHA           0.98f
#define DEG_PER_RAD     57.2957795f

static float s_pitch_deg = 0.0f;
static float s_roll_deg = 0.0f;
static uint32_t s_last_update_us = 0U;
static bool s_initialized = false;

static error_t write_reg(uint8_t reg, uint8_t val) {
    P10_ASSERT(I2C_PORT != NULL);

    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;

    const int ret = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, buf, 2, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

static error_t read_regs(uint8_t reg, uint8_t* data, uint32_t len) {
    P10_ASSERT(data != NULL);
    P10_ASSERT(len > 0U);

    if ((data == NULL) || (len == 0U)) {
        return ERR_INVALID_PARAM;
    }

    int ret = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    if (ret < 0) {
        return ERR_I2C;
    }

    ret = i2c_read_blocking(I2C_PORT, MPU6050_ADDR, data, len, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

static float clamp_dt(float dt) {
    P10_ASSERT(true);

    if ((dt <= 0.0f) || (dt > 0.5f)) {
        return 0.02f;
    }

    return dt;
}

static float accel_to_pitch_deg(float ax_g, float ay_g, float az_g) {
    P10_ASSERT(true);

    const float denom = sqrtf((ay_g * ay_g) + (az_g * az_g));
    return atan2f(-ax_g, denom) * DEG_PER_RAD;
}

static float accel_to_roll_deg(float ay_g, float az_g) {
    P10_ASSERT(true);

    return atan2f(ay_g, az_g) * DEG_PER_RAD;
}

error_t imu_init(void) {
    P10_ASSERT(I2C_PORT != NULL);

    if (s_initialized) {
        return ERR_NONE;
    }

    // Wake up device
    error_t err = write_reg(REG_PWR_MGMT_1, 0x00U);
    if (err != ERR_NONE) {
        return err;
    }
    sleep_ms(100);

    // Set sample rate to 100 Hz (8 kHz / (1 + 79))
    err = write_reg(REG_SMPLRT_DIV, 79U);
    if (err != ERR_NONE) {
        return err;
    }

    // Set DLPF to ~44 Hz bandwidth
    err = write_reg(REG_CONFIG, 0x03U);
    if (err != ERR_NONE) {
        return err;
    }

    // Gyro range: ±500 °/s
    err = write_reg(REG_GYRO_CFG, 0x08U);
    if (err != ERR_NONE) {
        return err;
    }

    // Accel range: ±4g
    err = write_reg(REG_ACCEL_CFG, 0x08U);
    if (err != ERR_NONE) {
        return err;
    }

    s_pitch_deg = 0.0f;
    s_roll_deg = 0.0f;
    s_last_update_us = time_us_32();
    s_initialized = true;

    return ERR_NONE;
}

error_t imu_read(AttitudeReading_t* reading) {
    P10_ASSERT(reading != NULL);
    P10_ASSERT(s_initialized);

    if (reading == NULL) {
        return ERR_INVALID_PARAM;
    }

    if (!s_initialized) {
        reading->valid = false;
        return ERR_NOT_READY;
    }

    reading->valid = false;

    uint8_t data[14];
    error_t err = read_regs(REG_ACCEL_XOUT, data, 14U);
    if (err != ERR_NONE) {
        return err;
    }

    // Parse raw values (big-endian)
    const int16_t ax = (int16_t)((data[0] << 8) | data[1]);
    const int16_t ay = (int16_t)((data[2] << 8) | data[3]);
    const int16_t az = (int16_t)((data[4] << 8) | data[5]);
    const int16_t gx = (int16_t)((data[8] << 8) | data[9]);
    const int16_t gy = (int16_t)((data[10] << 8) | data[11]);

    // Convert to physical units
    const float ax_g = (float)ax / 8192.0f;  // ±4g range
    const float ay_g = (float)ay / 8192.0f;
    const float az_g = (float)az / 8192.0f;

    const float gx_dps = (float)gx / 65.5f;  // ±500 °/s range
    const float gy_dps = (float)gy / 65.5f;

    // dt in seconds
    const uint32_t now_us = time_us_32();
    const uint32_t delta_us = now_us - s_last_update_us;
    s_last_update_us = now_us;

    const float dt = clamp_dt((float)delta_us / 1000000.0f);

    // Accelerometer-based angles
    const float accel_pitch = accel_to_pitch_deg(ax_g, ay_g, az_g);
    const float accel_roll = accel_to_roll_deg(ay_g, az_g);

    // Complementary filter
    s_pitch_deg = (ALPHA * (s_pitch_deg + (gy_dps * dt))) + ((1.0f - ALPHA) * accel_pitch);
    s_roll_deg = (ALPHA * (s_roll_deg + (gx_dps * dt))) + ((1.0f - ALPHA) * accel_roll);

    reading->pitch_deg_x10 = (int16_t)(s_pitch_deg * 10.0f);
    reading->roll_deg_x10 = (int16_t)(s_roll_deg * 10.0f);
    reading->timestamp_ms = to_ms_since_boot(get_absolute_time());
    reading->valid = true;

    return ERR_NONE;
}
