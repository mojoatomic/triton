/**
 * RC Submarine Controller
 * pressure_sensor.h - MS5837 pressure sensor driver
 *
 * Power of 10 compliant
 */

#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t pressure_sensor_init(void);
error_t pressure_sensor_read(DepthReading_t* reading);

#ifdef __cplusplus
}
#endif

#endif // PRESSURE_SENSOR_H
