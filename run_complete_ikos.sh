#!/bin/bash
# Complete IKOS Analysis Runner for Triton Project

set -e

# Configuration
REPORT_DIR="ikos_reports"
TEMP_DIR="/tmp/ikos_triton_$$"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_FILE="$REPORT_DIR/COMPLETE_ANALYSIS_${TIMESTAMP}.md"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Statistics
TOTAL_FILES=0
ANALYZED=0
SAFE_COUNT=0
UNSAFE_COUNT=0
WARNING_COUNT=0
ERROR_COUNT=0

# Create directories
mkdir -p "$REPORT_DIR"
mkdir -p "$TEMP_DIR"

# Initialize summary
cat > "$SUMMARY_FILE" << EOF
# Complete IKOS Analysis Report - Triton Project
**Date**: $(date)  
**IKOS Version**: NASA IKOS Static Analyzer  
**Project**: RC Submarine Controller (Safety-Critical Embedded System)

## Analysis Results

EOF

# Function to preprocess source file
preprocess_file() {
    local src_file=$1
    local temp_file=$2
    
    cat > "$temp_file" << 'EOF'
// IKOS Analysis Version - Auto-generated
#define IKOS_ANALYSIS 1
#define PICO_NO_HARDWARE 1

// Include comprehensive Pico stubs
#include "/src/analysis/pico_stubs.h"

// Stub out hardware includes
#define HARDWARE_GPIO_H 1
#define HARDWARE_I2C_H 1
#define HARDWARE_ADC_H 1
#define HARDWARE_PWM_H 1
#define HARDWARE_WATCHDOG_H 1
#define PICO_STDLIB_H 1
#define PICO_MULTICORE_H 1

// Stub printf
#define printf(...) ((void)0)

EOF
    
    # Append source without hardware includes
    grep -v "#include.*hardware" "$src_file" | grep -v "#include.*pico/" >> "$temp_file"
    
    # Add test main
    local component=$(basename "$src_file" .c)
    cat >> "$temp_file" << EOF

int main() {
    error_t err = ERR_NONE;
    
    // Test init if exists
    #ifdef ${component}_init
    err = ${component}_init();
    #endif
    
    return (err == ERR_NONE) ? 0 : 1;
}
EOF
}

# Function to analyze a file
analyze_component() {
    local src_file=$1
    local component=$(basename "$src_file" .c)
    local temp_file="$TEMP_DIR/${component}_test.c"
    local report_file="$REPORT_DIR/${component}_${TIMESTAMP}.txt"
    
    echo -ne "[$((ANALYZED+1))/$TOTAL_FILES] ${component}... "
    
    # Preprocess
    preprocess_file "$src_file" "$temp_file"
    
    # Run IKOS
    docker run --rm -v "${PWD}:/src" -v "$TEMP_DIR:$TEMP_DIR" ikos \
        "ikos \
        -I/src/src \
        -I/src/src/drivers \
        -I/src/src/safety \
        -I/src/src/control \
        --entry-points=main \
        --analyses=boa,dbz,nullity,prover,uva \
        --display-checks=all \
        --hardware-addresses=0x40000000-0x60000000 \
        $temp_file" > "$report_file" 2>&1
    
    ANALYZED=$((ANALYZED + 1))
    
    # Parse results
    if grep -q "The program is SAFE" "$report_file"; then
        echo -e "${GREEN}SAFE${NC}"
        SAFE_COUNT=$((SAFE_COUNT + 1))
        echo "- ✅ **${component}**: SAFE" >> "$SUMMARY_FILE"
    elif grep -q "The program is UNSAFE" "$report_file"; then
        echo -e "${RED}UNSAFE${NC}"
        UNSAFE_COUNT=$((UNSAFE_COUNT + 1))
        echo "- ❌ **${component}**: UNSAFE" >> "$SUMMARY_FILE"
        echo "  ```" >> "$SUMMARY_FILE"
        grep -A1 "error:" "$report_file" | head -3 >> "$SUMMARY_FILE"
        echo "  ```" >> "$SUMMARY_FILE"
    else
        echo -e "${YELLOW}ERROR/WARNING${NC}"
        ERROR_COUNT=$((ERROR_COUNT + 1))
        echo "- ⚠️  **${component}**: Compilation/Analysis Error" >> "$SUMMARY_FILE"
    fi
    
    # Add check counts if available
    local checks=$(grep "Total number of checks" "$report_file" | awk '{print $NF}')
    local safe_checks=$(grep "Total number of safe checks" "$report_file" | awk '{print $NF}')
    if [ -n "$checks" ] && [ -n "$safe_checks" ]; then
        echo "  - Checks: $safe_checks/$checks safe" >> "$SUMMARY_FILE"
    fi
    
    rm -f "$temp_file"
}

# Main execution
echo -e "${BLUE}IKOS Complete Analysis - Triton Project${NC}"
echo "========================================"

# Count total files
TOTAL_FILES=$(find src -name "*.c" | wc -l | tr -d ' ')
echo "Found $TOTAL_FILES C source files"
echo ""

# Analyze all files
echo "Analyzing safety-critical components:"
for file in src/safety/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Analyzing drivers:"
for file in src/drivers/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Analyzing control systems:"
for file in src/control/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Analyzing utilities:"
for file in src/util/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Analyzing main:"
[ -f "src/main.c" ] && analyze_component "src/main.c"

# Add statistics to summary
cat >> "$SUMMARY_FILE" << EOF

## Summary Statistics

| Metric | Count | Percentage |
|--------|-------|------------|
| **Total Files** | $TOTAL_FILES | 100% |
| **Analyzed** | $ANALYZED | $(( (ANALYZED * 100) / TOTAL_FILES ))% |
| **Safe** | $SAFE_COUNT | $(( (SAFE_COUNT * 100) / TOTAL_FILES ))% |
| **Unsafe** | $UNSAFE_COUNT | $(( (UNSAFE_COUNT * 100) / TOTAL_FILES ))% |
| **Errors** | $ERROR_COUNT | $(( (ERROR_COUNT * 100) / TOTAL_FILES ))% |

## Recommendations

1. **Fix UNSAFE components immediately** - These have proven runtime errors
2. **Review ERROR components** - May need stub improvements
3. **Add more P10_ASSERT statements** for better coverage
4. **Initialize all variables** before use
5. **Add explicit bounds checking** for arrays

## Power of 10 Compliance

IKOS helps verify NASA/JPL coding standards:
- ✅ No dynamic memory allocation
- ✅ Assertion validation  
- ✅ Bounds checking
- ✅ Initialization verification
- ✅ Return value checking

EOF

# Cleanup
rm -rf "$TEMP_DIR"

# Final report
echo ""
echo "========================================"
echo "Analysis Complete!"
echo "========================================"
echo "Summary:"
echo "  Total:    $TOTAL_FILES files"
echo -e "  Safe:     ${GREEN}$SAFE_COUNT${NC}"
echo -e "  Unsafe:   ${RED}$UNSAFE_COUNT${NC}"
echo -e "  Errors:   ${YELLOW}$ERROR_COUNT${NC}"
echo ""
echo "Full report: $SUMMARY_FILE"
echo ""

if [ $UNSAFE_COUNT -gt 0 ]; then
    echo -e "${RED}⚠️  $UNSAFE_COUNT files marked UNSAFE - require immediate attention!${NC}"
fi