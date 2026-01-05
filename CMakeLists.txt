cmake_minimum_required(VERSION 3.13)

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
