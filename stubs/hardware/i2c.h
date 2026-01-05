#ifndef STUB_HARDWARE_I2C_H
#define STUB_HARDWARE_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef unsigned int uint;

typedef struct i2c_inst i2c_inst_t;

extern i2c_inst_t i2c0_inst;
extern i2c_inst_t i2c1_inst;

#define i2c0 (&i2c0_inst)
#define i2c1 (&i2c1_inst)

int i2c_write_blocking(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop);
int i2c_read_blocking(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop);

#endif // STUB_HARDWARE_I2C_H
