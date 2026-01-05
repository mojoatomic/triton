#!/usr/bin/env python3
"""
Generate driver boilerplate code for RC Submarine Controller.

Creates header and source files with proper structure, includes,
and Power of 10 compliant patterns.

Usage:
    python generate_driver.py <driver-name> [--output-dir <path>]
    
Example:
    python generate_driver.py temperature_sensor --output-dir src/drivers/
"""

import os
import sys
import argparse
from datetime import datetime


HEADER_TEMPLATE = '''#ifndef {guard}_H
#define {guard}_H

/**
 * RC Submarine Controller
 * {filename}.h - {description}
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "../types.h"

// ============================================================
// PUBLIC TYPES
// ============================================================

// TODO: Add driver-specific types here

// ============================================================
// PUBLIC API
// ============================================================

/**
 * Initialize the {name} driver.
 * 
 * @return ERR_NONE on success, error code otherwise
 */
error_t {prefix}_init(void);

// TODO: Add additional public functions

#endif // {guard}_H
'''

SOURCE_TEMPLATE = '''/**
 * RC Submarine Controller
 * {filename}.c - {description}
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "{filename}.h"
#include "../config.h"

// ============================================================
// PRIVATE CONSTANTS
// ============================================================

// TODO: Add private constants

// ============================================================
// PRIVATE DATA
// ============================================================

static bool g_initialized = false;

// ============================================================
// PRIVATE FUNCTIONS
// ============================================================

// TODO: Add private helper functions

// ============================================================
// PUBLIC FUNCTIONS
// ============================================================

error_t {prefix}_init(void) {{
    P10_ASSERT(!g_initialized);  // Don't initialize twice
    
    // TODO: Implement initialization
    
    g_initialized = true;
    return ERR_NONE;
}}

// TODO: Implement additional public functions
'''

# Specialized templates for common driver types
TEMPLATES = {
    'i2c_sensor': {
        'header': '''#ifndef {guard}_H
#define {guard}_H

/**
 * RC Submarine Controller
 * {filename}.h - {description} (I2C sensor)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "../types.h"

// ============================================================
// PUBLIC TYPES
// ============================================================

typedef struct {{
    // TODO: Add sensor-specific fields
    int32_t value;
    uint32_t timestamp_ms;
    bool valid;
}} {type_name}_t;

// ============================================================
// PUBLIC API
// ============================================================

/**
 * Initialize the {name} driver.
 * @return ERR_NONE on success, error code otherwise
 */
error_t {prefix}_init(void);

/**
 * Read sensor data.
 * @param reading Pointer to store reading
 * @return ERR_NONE on success, error code otherwise
 */
error_t {prefix}_read({type_name}_t* reading);

#endif // {guard}_H
''',
        'source': '''/**
 * RC Submarine Controller
 * {filename}.c - {description} (I2C sensor)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "{filename}.h"
#include "../config.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// ============================================================
// PRIVATE CONSTANTS
// ============================================================

#define SENSOR_ADDR     0x00    // TODO: Set I2C address
#define REG_DATA        0x00    // TODO: Set data register

// ============================================================
// PRIVATE DATA
// ============================================================

static bool g_initialized = false;

// ============================================================
// PRIVATE FUNCTIONS
// ============================================================

static error_t write_reg(uint8_t reg, uint8_t val) {{
    uint8_t buf[2] = {{reg, val}};
    int ret = i2c_write_blocking(I2C_PORT, SENSOR_ADDR, buf, 2, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}}

static error_t read_reg(uint8_t reg, uint8_t* data, uint8_t len) {{
    int ret = i2c_write_blocking(I2C_PORT, SENSOR_ADDR, &reg, 1, true);
    if (ret < 0) return ERR_I2C;
    ret = i2c_read_blocking(I2C_PORT, SENSOR_ADDR, data, len, false);
    return (ret < 0) ? ERR_I2C : ERR_NONE;
}}

// ============================================================
// PUBLIC FUNCTIONS
// ============================================================

error_t {prefix}_init(void) {{
    P10_ASSERT(!g_initialized);
    
    // TODO: Initialize sensor (reset, configure, etc.)
    
    g_initialized = true;
    return ERR_NONE;
}}

error_t {prefix}_read({type_name}_t* reading) {{
    P10_ASSERT(reading != NULL);
    P10_ASSERT(g_initialized);
    
    uint8_t data[2];
    error_t err = read_reg(REG_DATA, data, sizeof(data));
    if (err != ERR_NONE) {{
        reading->valid = false;
        return err;
    }}
    
    // TODO: Parse raw data
    reading->value = (data[0] << 8) | data[1];
    reading->timestamp_ms = to_ms_since_boot(get_absolute_time());
    reading->valid = true;
    
    return ERR_NONE;
}}
'''
    },
    
    'pwm_output': {
        'header': '''#ifndef {guard}_H
#define {guard}_H

/**
 * RC Submarine Controller
 * {filename}.h - {description} (PWM output)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "../types.h"

// ============================================================
// PUBLIC API
// ============================================================

/**
 * Initialize the {name} driver.
 * @return ERR_NONE on success
 */
error_t {prefix}_init(void);

/**
 * Set output level.
 * @param level -100 to +100 percent
 */
void {prefix}_set(int8_t level);

/**
 * Stop output (set to zero).
 */
void {prefix}_stop(void);

#endif // {guard}_H
''',
        'source': '''/**
 * RC Submarine Controller
 * {filename}.c - {description} (PWM output)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "{filename}.h"
#include "../config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

// ============================================================
// PRIVATE CONSTANTS
// ============================================================

#define PIN_PWM         0       // TODO: Set PWM pin
#define PWM_FREQ_HZ     1000    // TODO: Set PWM frequency

// ============================================================
// PRIVATE DATA
// ============================================================

static uint g_slice_num;

// ============================================================
// PUBLIC FUNCTIONS
// ============================================================

error_t {prefix}_init(void) {{
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    g_slice_num = pwm_gpio_to_slice_num(PIN_PWM);
    
    // Configure PWM frequency
    // 125 MHz / (wrap + 1) / divider = target freq
    pwm_set_wrap(g_slice_num, 999);
    pwm_set_clkdiv(g_slice_num, 125.0f);  // 1 kHz
    pwm_set_enabled(g_slice_num, true);
    
    {prefix}_stop();
    return ERR_NONE;
}}

void {prefix}_set(int8_t level) {{
    // Clamp to valid range
    if (level > 100) level = 100;
    if (level < -100) level = -100;
    
    // Map to PWM duty cycle (0-1000)
    uint16_t duty = (uint16_t)(ABS(level) * 10);
    pwm_set_gpio_level(PIN_PWM, duty);
}}

void {prefix}_stop(void) {{
    pwm_set_gpio_level(PIN_PWM, 0);
}}
'''
    },
    
    'gpio_input': {
        'header': '''#ifndef {guard}_H
#define {guard}_H

/**
 * RC Submarine Controller
 * {filename}.h - {description} (GPIO input)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "../types.h"

// ============================================================
// PUBLIC API
// ============================================================

/**
 * Initialize the {name} driver.
 * @return ERR_NONE on success
 */
error_t {prefix}_init(void);

/**
 * Read current state.
 * @return true if active, false otherwise
 */
bool {prefix}_read(void);

#endif // {guard}_H
''',
        'source': '''/**
 * RC Submarine Controller
 * {filename}.c - {description} (GPIO input)
 * 
 * Generated: {date}
 * Power of 10 compliant
 */

#include "{filename}.h"
#include "../config.h"
#include "hardware/gpio.h"

// ============================================================
// PRIVATE CONSTANTS
// ============================================================

#define PIN_INPUT       0       // TODO: Set input pin
#define ACTIVE_HIGH     true    // TODO: Set polarity

// ============================================================
// PUBLIC FUNCTIONS
// ============================================================

error_t {prefix}_init(void) {{
    gpio_init(PIN_INPUT);
    gpio_set_dir(PIN_INPUT, GPIO_IN);
    
    if (ACTIVE_HIGH) {{
        gpio_pull_down(PIN_INPUT);
    }} else {{
        gpio_pull_up(PIN_INPUT);
    }}
    
    return ERR_NONE;
}}

bool {prefix}_read(void) {{
    bool raw = gpio_get(PIN_INPUT);
    return ACTIVE_HIGH ? raw : !raw;
}}
'''
    },
}


def to_snake_case(name):
    """Convert name to snake_case."""
    return name.lower().replace('-', '_').replace(' ', '_')


def to_upper_case(name):
    """Convert name to UPPER_CASE."""
    return to_snake_case(name).upper()


def to_camel_case(name):
    """Convert name to CamelCase."""
    parts = to_snake_case(name).split('_')
    return ''.join(p.capitalize() for p in parts)


def generate_driver(name: str, template_type: str, output_dir: str, description: str):
    """Generate driver files."""
    
    snake_name = to_snake_case(name)
    prefix = snake_name
    guard = to_upper_case(name)
    type_name = to_camel_case(name) + "Reading"
    date = datetime.now().strftime("%Y-%m-%d")
    
    # Select template
    if template_type in TEMPLATES:
        header_tmpl = TEMPLATES[template_type]['header']
        source_tmpl = TEMPLATES[template_type]['source']
    else:
        header_tmpl = HEADER_TEMPLATE
        source_tmpl = SOURCE_TEMPLATE
    
    # Format templates
    format_args = {
        'filename': snake_name,
        'name': name.replace('_', ' '),
        'prefix': prefix,
        'guard': guard,
        'type_name': type_name,
        'description': description,
        'date': date,
    }
    
    header_content = header_tmpl.format(**format_args)
    source_content = source_tmpl.format(**format_args)
    
    # Write files
    header_path = os.path.join(output_dir, f"{snake_name}.h")
    source_path = os.path.join(output_dir, f"{snake_name}.c")
    
    os.makedirs(output_dir, exist_ok=True)
    
    with open(header_path, 'w') as f:
        f.write(header_content)
    print(f"Created: {header_path}")
    
    with open(source_path, 'w') as f:
        f.write(source_content)
    print(f"Created: {source_path}")


def main():
    parser = argparse.ArgumentParser(description="Generate driver boilerplate")
    parser.add_argument("name", help="Driver name (e.g., temperature_sensor)")
    parser.add_argument("--type", "-t", choices=['generic', 'i2c_sensor', 'pwm_output', 'gpio_input'],
                        default='generic', help="Driver template type")
    parser.add_argument("--output-dir", "-o", default=".", help="Output directory")
    parser.add_argument("--description", "-d", default="TODO: Add description",
                        help="Driver description")
    args = parser.parse_args()
    
    generate_driver(args.name, args.type, args.output_dir, args.description)
    print("\nDriver generated successfully!")
    print("Remember to:")
    print("  1. Update pin assignments in config.h")
    print("  2. Add source file to CMakeLists.txt")
    print("  3. Implement TODO sections")


if __name__ == "__main__":
    main()
