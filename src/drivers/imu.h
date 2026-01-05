/**
 * RC Submarine Controller
 * imu.h - MPU-6050 IMU driver
 *
 * Power of 10 compliant
 */

#ifndef IMU_H
#define IMU_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t imu_init(void);
error_t imu_read(AttitudeReading_t* reading);

#ifdef __cplusplus
}
#endif

#endif // IMU_H
