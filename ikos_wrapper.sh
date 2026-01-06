#!/bin/bash
# IKOS Analysis with Test Wrapper

FILE=$1
if [ -z "$FILE" ]; then
    echo "Usage: ./ikos_wrapper.sh <file.c>"
    exit 1
fi

BASENAME=$(basename "$FILE" .c)
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT="ikos_reports/${BASENAME}_${TIMESTAMP}_analysis.txt"
TEMP_FILE="analysis/${BASENAME}_test.c"

echo "Creating test wrapper for: $FILE"

# Extract function names from the source file
INIT_FUNC="${BASENAME}_init"
UPDATE_FUNC="${BASENAME}_update"

# Create test wrapper
cat > "$TEMP_FILE" << EOF
// IKOS Test Wrapper for $BASENAME
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

// Include original source (without hardware includes)
EOF

# Add filtered source content
grep -v "#include.*hardware" "$FILE" | grep -v "#include.*pico/" >> "$TEMP_FILE"

# Add test main function
cat >> "$TEMP_FILE" << EOF

// Test harness
int main() {
    error_t err;
    
    // Test initialization
    err = ${INIT_FUNC}();
    if (err != ERR_NONE) {
        return 1;
    }
    
    // Test update/read functions
    #ifdef ${BASENAME}_update
    ${BASENAME}_update();
    #endif
    
    #ifdef ${BASENAME}_read
    int value = ${BASENAME}_read();
    if (value < 0) {
        return 2;
    }
    #endif
    
    return 0;
}
EOF

echo "Running IKOS analysis..."

# Run IKOS
docker run --rm -v ${PWD}:/src ikos \
    "ikos \
    -I/src/src \
    -I/src/src/drivers \
    -I/src/src/safety \
    -I/src/src/control \
    --entry-points=main \
    --analyses=boa,dbz,nullity,prover,uva \
    --display-checks=all \
    --display-times=short \
    --hardware-addresses=0x40000000-0x60000000 \
    /src/$TEMP_FILE" 2>&1 | tee "$REPORT"

echo ""
echo "Analysis complete. Report: $REPORT"

# Parse results
echo ""
if grep -q "The program is SAFE" "$REPORT"; then
    echo "✅ RESULT: SAFE - No issues found"
elif grep -q "The program is UNSAFE" "$REPORT"; then
    echo "❌ RESULT: UNSAFE - Issues detected:"
    echo ""
    grep -B2 -A2 "error:" "$REPORT" | head -20
else
    CHECKS=$(grep "Total number of checks" "$REPORT" | awk '{print $NF}')
    SAFE=$(grep "Total number of safe checks" "$REPORT" | awk '{print $NF}')
    UNSAFE=$(grep "Total number of definite unsafe" "$REPORT" | awk '{print $NF}')
    WARNINGS=$(grep "Total number of warnings" "$REPORT" | awk '{print $NF}')
    
    if [ -n "$CHECKS" ]; then
        echo "📊 ANALYSIS SUMMARY:"
        echo "   Total checks: $CHECKS"
        echo "   Safe: $SAFE"
        echo "   Unsafe: $UNSAFE"
        echo "   Warnings: $WARNINGS"
    else
        echo "⚠️  Analysis may have encountered errors"
    fi
fi

# Cleanup
rm -f "$TEMP_FILE"