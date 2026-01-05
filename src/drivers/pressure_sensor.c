/**
 * RC Submarine Controller
 * pressure_sensor.c - MS5837 pressure sensor driver
 *
 * Power of 10 compliant
 */

#include "pressure_sensor.h"

#include "config.h"

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define CMD_RESET       0x1EU
#define CMD_PROM_BASE   0xA0U
#define CMD_CONV_D1     0x48U    // OSR 4096
#define CMD_CONV_D2     0x58U    // OSR 4096
#define CMD_ADC_READ    0x00U
#define CONV_TIME_MS    20U

static uint16_t s_cal[7];
static bool s_initialized = false;

static error_t write_cmd(uint8_t cmd, bool nostop) {
    P10_ASSERT(I2C_PORT != NULL);

    const int ret = i2c_write_blocking(I2C_PORT, MS5837_ADDR, &cmd, 1, nostop);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

static error_t read_bytes(uint8_t* dst, uint32_t len) {
    P10_ASSERT(dst != NULL);
    P10_ASSERT(len > 0U);

    const int ret = i2c_read_blocking(I2C_PORT, MS5837_ADDR, dst, len, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}

static error_t read_prom_word(uint32_t idx, uint16_t* out) {
    P10_ASSERT(out != NULL);
    P10_ASSERT(idx < 7U);

    uint8_t data[2];
    const uint8_t cmd = (uint8_t)(CMD_PROM_BASE + (idx * 2U));

    error_t err = write_cmd(cmd, true);
    if (err != ERR_NONE) {
        return err;
    }

    err = read_bytes(data, 2U);
    if (err != ERR_NONE) {
        return err;
    }

    *out = (uint16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    return ERR_NONE;
}

static error_t read_prom(void) {
    P10_ASSERT(I2C_PORT != NULL);

    for (uint32_t i = 0; i < 7U; i++) {
        const error_t err = read_prom_word(i, &s_cal[i]);
        if (err != ERR_NONE) {
            return err;
        }
    }

    return ERR_NONE;
}

static error_t read_adc(uint32_t* out) {
    P10_ASSERT(out != NULL);

    uint8_t data[3];
    error_t err = write_cmd(CMD_ADC_READ, true);
    if (err != ERR_NONE) {
        return err;
    }

    err = read_bytes(data, 3U);
    if (err != ERR_NONE) {
        return err;
    }

    *out = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[2];
    return ERR_NONE;
}

static error_t convert_and_read(uint8_t conv_cmd, uint32_t* out) {
    P10_ASSERT(out != NULL);

    error_t err = write_cmd(conv_cmd, false);
    if (err != ERR_NONE) {
        return err;
    }

    sleep_ms(CONV_TIME_MS);

    err = read_adc(out);
    if (err != ERR_NONE) {
        return err;
    }

    return ERR_NONE;
}

static void compute_reading(uint32_t d1, uint32_t d2, DepthReading_t* reading) {
    P10_ASSERT(reading != NULL);
    if (reading == NULL) {
        return;
    }

    const int32_t dT = (int32_t)d2 - ((int32_t)((uint32_t)s_cal[5] << 8));
    const int32_t temp_c_x100 = 2000 + (int32_t)(((int64_t)dT * (int64_t)s_cal[6]) >> 23);

    const int64_t off = ((int64_t)s_cal[2] << 16) + (((int64_t)s_cal[4] * (int64_t)dT) >> 7);
    const int64_t sens = ((int64_t)s_cal[1] << 15) + (((int64_t)s_cal[3] * (int64_t)dT) >> 8);
    const int32_t p = (int32_t)(((((int64_t)d1 * sens) >> 21) - off) >> 13);

    // p is in 0.1 mbar (10 Pa). At surface: ~101325 Pa => 10132.5 units.
    const int32_t p_surface = 10133;
    reading->depth_cm = ((p - p_surface) * 100) / 978;

    // Convert 0.01C to 0.1C units.
    reading->temp_c_x10 = temp_c_x100 / 10;
}

error_t pressure_sensor_init(void) {
    P10_ASSERT(I2C_PORT != NULL);

    error_t err = write_cmd(CMD_RESET, false);
    if (err != ERR_NONE) {
        return err;
    }

    sleep_ms(10);

    err = read_prom();
    if (err != ERR_NONE) {
        return err;
    }

    s_initialized = true;
    return ERR_NONE;
}

error_t pressure_sensor_read(DepthReading_t* reading) {
    P10_ASSERT(reading != NULL);
    P10_ASSERT(s_initialized);

    reading->valid = false;

    uint32_t d1;
    uint32_t d2;

    error_t err = convert_and_read(CMD_CONV_D1, &d1);
    if (err != ERR_NONE) {
        return err;
    }

    err = convert_and_read(CMD_CONV_D2, &d2);
    if (err != ERR_NONE) {
        return err;
    }

    compute_reading(d1, d2, reading);

    reading->timestamp_ms = to_ms_since_boot(get_absolute_time());
    reading->valid = true;

    return ERR_NONE;
}
