#!/bin/bash

echo "=== Full Project IKOS Analysis ==="
echo "Analyzing all source files in the Triton project..."

# Source files to analyze
SOURCES=(
    "src/drivers/battery.c"
    "src/drivers/pressure_sensor.c" 
    "src/drivers/servo.c"
    "src/drivers/valve.c"
    "src/drivers/pump.c"
    "src/drivers/leak.c"
    "src/drivers/rc_input.c"
    "src/drivers/imu.c"
    "src/drivers/display.c"
    "src/control/pid.c"
    "src/control/depth_ctrl.c"
    "src/control/pitch_ctrl.c"
    "src/control/ballast_ctrl.c"
    "src/control/state_machine.c"
    "src/safety/safety_monitor.c"
    "src/safety/emergency.c"
    "src/safety/handshake.c"
    "src/util/log.c"
    "src/main.c"
)

# Create output directory
mkdir -p ikos_reports

# Create combined test file
cat > analysis/full_project_test.c << 'EOF'
// Combined analysis test for all source files

// Include stub definitions
#include "pico_stubs.h"

// Include all headers
#include "config.h"
#include "types.h"

// Declare external functions we'll call for testing
extern error_t battery_init(void);
extern error_t pressure_sensor_init(void);
extern error_t servo_init(void);
extern error_t valve_init(void);
extern void pump_init(void);
extern error_t leak_detector_init(void);
extern error_t rc_input_init(void);
extern error_t imu_init(void);
extern error_t display_init(void);
extern void safety_monitor_init(void);
extern void emergency_blow_init(void);
extern void handshake_init(void);
extern void log_init(EventLog_t* log);
extern EventLog_t* get_event_log(void);

// Test main function that calls initialization functions
int main() {
    // Initialize event logging
    EventLog_t* event_log = get_event_log();
    if (event_log != NULL) {
        log_init(event_log);
    }
    
    // Initialize drivers (check return codes)
    error_t err;
    
    err = battery_init();
    if (err != ERR_NONE) return 1;
    
    err = pressure_sensor_init();
    if (err != ERR_NONE) return 2;
    
    err = servo_init();
    if (err != ERR_NONE) return 3;
    
    err = valve_init();
    if (err != ERR_NONE) return 4;
    
    pump_init();
    
    err = leak_detector_init();
    if (err != ERR_NONE) return 5;
    
    err = rc_input_init();
    if (err != ERR_NONE) return 6;
    
    err = imu_init();
    if (err != ERR_NONE) return 7;
    
    err = display_init();
    if (err != ERR_NONE) return 8;
    
    // Initialize safety systems
    safety_monitor_init();
    emergency_blow_init();
    handshake_init();
    
    return 0;
}
EOF

echo "Created full project test file..."

# Build the source list for IKOS
SOURCE_LIST=""
for src in "${SOURCES[@]}"; do
    if [ -f "$src" ]; then
        SOURCE_LIST="$SOURCE_LIST $src"
    else
        echo "Warning: $src not found, skipping..."
    fi
done

# Include all required source files for analysis
ALL_SOURCES="analysis/full_project_test.c $SOURCE_LIST"

echo "Running IKOS on full project..."
echo "Source files: $ALL_SOURCES"

# Run IKOS analysis
docker run --rm -v ${PWD}:/src ikos \
    "ikos \
    --display-times=short \
    --display-summary \
    --display-checks \
    --hardware-addresses=0x40000000-0x60000000 \
    -I /src/src \
    -I /src/analysis \
    -D PICO_PLATFORM_RP2040=1 \
    -D __ARM_ARCH_6M__=1 \
    $ALL_SOURCES" > ikos_reports/full_project_analysis.txt 2>&1

# Check results
if [ $? -eq 0 ]; then
    echo "✅ IKOS analysis completed successfully!"
    echo "📊 Results summary:"
    tail -n 20 ikos_reports/full_project_analysis.txt
else
    echo "❌ IKOS analysis failed"
    echo "📋 Error details:"
    tail -n 50 ikos_reports/full_project_analysis.txt
fi

echo ""
echo "Full report saved to: ikos_reports/full_project_analysis.txt"