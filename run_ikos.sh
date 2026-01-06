#!/bin/bash
# Simple IKOS runner for Triton project

FILE=$1
if [ -z "$FILE" ]; then
    echo "Usage: ./run_ikos.sh <file.c>"
    exit 1
fi

BASENAME=$(basename "$FILE" .c)
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT="ikos_reports/${BASENAME}_${TIMESTAMP}_report.txt"

echo "Analyzing: $FILE"

# Create a preprocessed version
TEMP_FILE="/tmp/${BASENAME}_ikos.c"

# Create preprocessed file with stubs
cat > "$TEMP_FILE" << 'EOF'
// IKOS Analysis Version
#define IKOS_ANALYSIS 1
#include "/src/analysis/pico_stubs.h"

// Stub hardware includes
#define HARDWARE_GPIO_H
#define HARDWARE_I2C_H
#define HARDWARE_ADC_H
#define HARDWARE_PWM_H
#define HARDWARE_WATCHDOG_H
#define PICO_STDLIB_H
#define PICO_MULTICORE_H

EOF

# Add original content without hardware includes
grep -v "#include.*hardware" "$FILE" | grep -v "#include.*pico/" >> "$TEMP_FILE"

# Copy to Docker-accessible location
cp "$TEMP_FILE" "analysis/temp_ikos.c"

# Run IKOS
docker run --rm -v ${PWD}:/src ikos \
    "ikos \
    -I/src/src \
    -I/src/src/drivers \
    -I/src/src/safety \
    -I/src/src/control \
    --analyses=boa,dbz,nullity \
    --display-checks=all \
    --hardware-addresses=0x40000000-0x60000000 \
    /src/analysis/temp_ikos.c" 2>&1 | tee "$REPORT"

echo ""
echo "Report saved to: $REPORT"

# Cleanup
rm -f "$TEMP_FILE" "analysis/temp_ikos.c"

# Show summary
if grep -q "The program is SAFE" "$REPORT"; then
    echo "✓ Result: SAFE"
elif grep -q "The program is UNSAFE" "$REPORT"; then
    echo "✗ Result: UNSAFE"
    echo ""
    echo "Issues found:"
    grep -A1 -E "error:|warning:" "$REPORT" | head -10
fi