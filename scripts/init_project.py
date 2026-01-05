#!/usr/bin/env python3
"""
Initialize RC Submarine Controller project directory structure.

Usage:
    python init_project.py <project-path>
    
Example:
    python init_project.py ~/projects/rc-sub-controller
"""

import os
import sys
import argparse

# Project directory structure
DIRECTORIES = [
    "src",
    "src/drivers",
    "src/control",
    "src/safety",
    "src/util",
    "include",
    "pio",
    "test",
    "docs",
    "build",
]

# Template files to create
FILES = {
    "CMakeLists.txt": '''cmake_minimum_required(VERSION 3.13)

# Initialize Pico SDK (must be before project())
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(rc_sub_controller C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Compiler flags (Power of 10 compliance)
add_compile_options(
    -Wall
    -Werror
    -Wextra
    -Wno-unused-parameter
    -ffunction-sections
    -fdata-sections
)

# Source files
set(SOURCES
    src/main.c
    src/drivers/rc_input.c
    src/drivers/pressure_sensor.c
    src/drivers/imu.c
    src/drivers/pump.c
    src/drivers/valve.c
    src/drivers/servo.c
    src/drivers/battery.c
    src/drivers/leak.c
    src/control/pid.c
    src/control/depth_ctrl.c
    src/control/pitch_ctrl.c
    src/control/ballast_ctrl.c
    src/control/state_machine.c
    src/safety/safety_monitor.c
    src/safety/emergency.c
    src/util/log.c
)

# Create executable
add_executable(rc_sub_controller ${SOURCES})

# Include directories
target_include_directories(rc_sub_controller PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(rc_sub_controller
    pico_stdlib
    pico_multicore
    hardware_i2c
    hardware_adc
    hardware_pwm
    hardware_pio
    hardware_watchdog
)

# Generate PIO header
pico_generate_pio_header(rc_sub_controller ${CMAKE_CURRENT_SOURCE_DIR}/pio/pwm_capture.pio)

# Enable USB output, disable UART (pins used for RC)
pico_enable_stdio_usb(rc_sub_controller 1)
pico_enable_stdio_uart(rc_sub_controller 0)

# Create UF2 file for flashing
pico_add_extra_outputs(rc_sub_controller)
''',

    "src/config.h": '''#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PIN ASSIGNMENTS
// ============================================================

// RC Input (PIO0) - 6 channels
#define PIN_RC_CH1          0
#define PIN_RC_CH2          1
#define PIN_RC_CH3          2
#define PIN_RC_CH4          3
#define PIN_RC_CH5          4
#define PIN_RC_CH6          5
#define RC_CHANNEL_COUNT    6

// I2C0 - Sensors
#define PIN_I2C_SDA         8
#define PIN_I2C_SCL         9
#define I2C_PORT            i2c0
#define I2C_BAUDRATE        400000

// I2C Addresses
#define MS5837_ADDR         0x76
#define MPU6050_ADDR        0x68

// PWM Outputs - Servos
#define PIN_SERVO_RUDDER    10
#define PIN_SERVO_BOWPLANE  11
#define PIN_SERVO_STERNPLANE 12
#define SERVO_PWM_FREQ      50

// PWM Output - Pump
#define PIN_PUMP_PWM        14
#define PIN_PUMP_DIR        15
#define PUMP_PWM_FREQ       1000

// Digital Outputs
#define PIN_VALVE           13
#define PIN_LED_STATUS      25

// Analog Inputs
#define PIN_BATTERY_ADC     26
#define PIN_LEAK_ADC        27

// Digital Inputs
#define PIN_LEAK_DETECT     16
#define PIN_DEPTH_LIMIT     17

// ============================================================
// TIMING CONSTANTS
// ============================================================

#define CONTROL_LOOP_HZ     50
#define SAFETY_LOOP_HZ      100
#define CONTROL_LOOP_US     (1000000 / CONTROL_LOOP_HZ)
#define SAFETY_LOOP_US      (1000000 / SAFETY_LOOP_HZ)

// ============================================================
// SAFETY LIMITS
// ============================================================

#define SIGNAL_TIMEOUT_MS   3000
#define MAX_DEPTH_CM        300
#define MAX_PITCH_DEG       45
#define MIN_BATTERY_MV      6400
#define WATCHDOG_TIMEOUT_MS 1000

// ============================================================
// RC CALIBRATION
// ============================================================

#define RC_PWM_MIN          1000
#define RC_PWM_MAX          2000
#define RC_PWM_CENTER       1500
#define RC_DEADBAND         50

// ============================================================
// SERVO CALIBRATION
// ============================================================

#define SERVO_PWM_MIN       1000
#define SERVO_PWM_MAX       2000
#define SERVO_PWM_CENTER    1500

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

#define BATTERY_DIVIDER_MULT    403
#define BATTERY_DIVIDER_DIV     100

#endif // CONFIG_H
''',

    "src/types.h": '''#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Error codes
typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_I2C,
    ERR_BUSY,
    ERR_NOT_READY,
    ERR_HARDWARE,
    ERR_COUNT
} error_t;

// Event codes
typedef enum {
    EVT_NONE = 0,
    EVT_BOOT,
    EVT_INIT_COMPLETE,
    EVT_SIGNAL_LOST,
    EVT_LOW_BATTERY,
    EVT_LEAK_DETECTED,
    EVT_DEPTH_EXCEEDED,
    EVT_PITCH_EXCEEDED,
    EVT_EMERGENCY_BLOW,
    EVT_ASSERT_FAIL,
    EVT_COUNT
} EventCode_t;

// RC Input
#define RC_CHANNEL_COUNT 6

typedef struct {
    uint16_t channels[RC_CHANNEL_COUNT];
    uint32_t timestamp_ms;
    bool valid;
} RcFrame_t;

// Sensor readings
typedef struct {
    int32_t depth_cm;
    int32_t temp_c_x10;
    uint32_t timestamp_ms;
    bool valid;
} DepthReading_t;

typedef struct {
    int16_t pitch_deg_x10;
    int16_t roll_deg_x10;
    uint32_t timestamp_ms;
    bool valid;
} AttitudeReading_t;

// Utility macros
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)    MIN(MAX(x, lo), hi)
#define ABS(x)              ((x) < 0 ? -(x) : (x))

// Assert macro
#ifndef NDEBUG
#define P10_ASSERT(cond) do { \\
    if (!(cond)) { \\
        p10_assert_fail(__FILE__, __LINE__, #cond); \\
    } \\
} while(0)
#else
#define P10_ASSERT(cond) ((void)0)
#endif

void p10_assert_fail(const char* file, int line, const char* cond);

#endif // TYPES_H
''',

    "src/main.c": '''/**
 * RC Submarine Controller
 * main.c - Entry point and dual-core initialization
 * 
 * Power of 10 compliant
 */

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "config.h"
#include "types.h"

// Forward declarations
void core0_main(void);
void core1_main(void);

int main(void) {
    // Initialize stdio for debug output
    stdio_init_all();
    
    // Brief delay for USB enumeration
    sleep_ms(1000);
    
    printf("RC Submarine Controller starting...\\n");
    
    // Launch Core 1 (control logic)
    multicore_launch_core1(core1_main);
    
    // Core 0 runs safety monitor
    core0_main();
    
    // Never reached
    return 0;
}

// Placeholder - implement in safety/safety_monitor.c
void core0_main(void) {
    printf("Core 0: Safety monitor starting\\n");
    
    while (1) {
        // TODO: Implement safety monitor
        gpio_put(PIN_LED_STATUS, 1);
        sleep_ms(500);
        gpio_put(PIN_LED_STATUS, 0);
        sleep_ms(500);
    }
}

// Placeholder - implement in control logic
void core1_main(void) {
    printf("Core 1: Control logic starting\\n");
    
    while (1) {
        // TODO: Implement control loop
        sleep_ms(20);  // 50 Hz
    }
}
''',

    "pio/pwm_capture.pio": '''; PWM capture PIO program
; Measures pulse width in microseconds

.program pwm_capture

.wrap_target
    wait 0 pin 0        ; Wait for low
    wait 1 pin 0        ; Wait for rising edge
    mov x, ~null        ; Load max count
count_high:
    jmp pin, continue   ; If still high, continue
    jmp done            ; If low, done
continue:
    jmp x--, count_high [1]
done:
    mov isr, ~x         ; Invert to get count
    push noblock
.wrap

% c-sdk {
static inline void pwm_capture_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_sm_config c = pwm_capture_program_get_default_config(offset);
    sm_config_set_jmp_pin(&c, pin);
    sm_config_set_in_pins(&c, pin);
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
    sm_config_set_clkdiv(&c, 62.5f);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
%}
''',

    ".gitignore": '''# Build output
build/
*.uf2
*.elf
*.bin
*.hex
*.map
*.dis

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# OS
.DS_Store
Thumbs.db
''',
}

# Stub files for each source file
STUB_FILES = {
    "src/drivers/rc_input.c": "rc_input",
    "src/drivers/pressure_sensor.c": "pressure_sensor", 
    "src/drivers/imu.c": "imu",
    "src/drivers/pump.c": "pump",
    "src/drivers/valve.c": "valve",
    "src/drivers/servo.c": "servo",
    "src/drivers/battery.c": "battery",
    "src/drivers/leak.c": "leak",
    "src/control/pid.c": "pid",
    "src/control/depth_ctrl.c": "depth_ctrl",
    "src/control/pitch_ctrl.c": "pitch_ctrl",
    "src/control/ballast_ctrl.c": "ballast_ctrl",
    "src/control/state_machine.c": "state_machine",
    "src/safety/safety_monitor.c": "safety_monitor",
    "src/safety/emergency.c": "emergency",
    "src/util/log.c": "log",
}

def create_stub(module_name):
    """Create a stub C file for a module."""
    return f'''/**
 * RC Submarine Controller
 * {module_name}.c - TODO: Add description
 * 
 * Power of 10 compliant
 */

#include "../config.h"
#include "../types.h"

// TODO: Implement {module_name}
'''


def main():
    parser = argparse.ArgumentParser(description="Initialize RC Submarine Controller project")
    parser.add_argument("path", help="Path to create project directory")
    parser.add_argument("--force", "-f", action="store_true", help="Overwrite existing files")
    args = parser.parse_args()
    
    project_path = os.path.abspath(args.path)
    
    # Check if directory exists
    if os.path.exists(project_path) and not args.force:
        print(f"Error: Directory already exists: {project_path}")
        print("Use --force to overwrite")
        sys.exit(1)
    
    print(f"Creating project at: {project_path}")
    
    # Create directories
    for dir_path in DIRECTORIES:
        full_path = os.path.join(project_path, dir_path)
        os.makedirs(full_path, exist_ok=True)
        print(f"  Created: {dir_path}/")
    
    # Create template files
    for file_path, content in FILES.items():
        full_path = os.path.join(project_path, file_path)
        with open(full_path, 'w') as f:
            f.write(content)
        print(f"  Created: {file_path}")
    
    # Create stub files
    for file_path, module_name in STUB_FILES.items():
        full_path = os.path.join(project_path, file_path)
        if not os.path.exists(full_path) or args.force:
            with open(full_path, 'w') as f:
                f.write(create_stub(module_name))
            print(f"  Created: {file_path} (stub)")
    
    print("\nProject initialized successfully!")
    print("\nNext steps:")
    print(f"  1. cd {project_path}")
    print("  2. export PICO_SDK_PATH=~/pico-sdk")
    print("  3. mkdir build && cd build")
    print("  4. cmake ..")
    print("  5. make")


if __name__ == "__main__":
    main()
