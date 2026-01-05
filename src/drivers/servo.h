/**
 * RC Submarine Controller
 * servo.h - Servo output driver
 *
 * Power of 10 compliant
 */

#ifndef SERVO_H
#define SERVO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SERVO_RUDDER = 0,
    SERVO_BOWPLANE,
    SERVO_STERNPLANE,
    SERVO_COUNT
} ServoChannel_t;

error_t servo_init(void);
void servo_set_position(ServoChannel_t channel, int8_t position);  // -100 to +100

#ifdef __cplusplus
}
#endif

#endif // SERVO_H
