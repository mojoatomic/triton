#!/bin/bash
# IKOS Analysis Runner with Preprocessing
# Handles Pico SDK dependencies properly

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
REPORT_DIR="ikos_reports"
TEMP_DIR="/tmp/ikos_triton_$$"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create directories
mkdir -p "$REPORT_DIR"
mkdir -p "$TEMP_DIR"

# Function to preprocess a file for IKOS
preprocess_file() {
    local src_file=$1
    local temp_file="$TEMP_DIR/$(basename $src_file)"
    
    echo -e "${BLUE}Preprocessing: $src_file${NC}"
    
    # Create preprocessed version with IKOS stubs
    cat > "$temp_file" << 'EOF'
// IKOS Analysis Version - Auto-generated
#define IKOS_ANALYSIS 1
#define PICO_NO_HARDWARE 1

// Include Pico stubs instead of real SDK
#include "/src/analysis/pico_stubs.h"

// Stub out hardware includes
#define HARDWARE_GPIO_H
#define HARDWARE_I2C_H
#define HARDWARE_SPI_H
#define HARDWARE_PWM_H
#define HARDWARE_ADC_H
#define HARDWARE_WATCHDOG_H
#define HARDWARE_TIMER_H
#define PICO_STDLIB_H
#define PICO_MULTICORE_H

// Original file content follows
#line 1 "$src_file"
EOF
    
    # Append original content, filtering hardware includes
    sed -e '/#include.*hardware\//d' \
        -e '/#include.*pico\//d' \
        "$src_file" >> "$temp_file"
    
    echo "$temp_file"
}

# Function to analyze a file
analyze_file() {
    local src_file=$1
    local basename=$(basename "$src_file" .c)
    local report_file="$REPORT_DIR/${basename}_${TIMESTAMP}.txt"
    
    echo -e "\n${YELLOW}=== Analyzing: $src_file ===${NC}"
    
    # Preprocess the file
    temp_file=$(preprocess_file "$src_file")
    
    # Run IKOS analysis
    echo -e "${BLUE}Running IKOS analysis...${NC}"
    docker run --rm -v "${PWD}:/src" -v "$TEMP_DIR:$TEMP_DIR" ikos \
        "ikos \
        -I/src/src \
        -I/src/src/drivers \
        -I/src/src/safety \
        -I/src/src/control \
        -I/src/src/util \
        --analyses=boa,dbz,nullity,prover,uva \
        --display-checks=all \
        --display-times=short \
        --hardware-addresses=0x40000000-0x60000000 \
        $temp_file" 2>&1 | tee "$report_file"
    
    # Parse results
    if grep -q "The program is SAFE" "$report_file"; then
        echo -e "${GREEN}✓ Result: SAFE${NC}"
    elif grep -q "The program is UNSAFE" "$report_file"; then
        echo -e "${RED}✗ Result: UNSAFE${NC}"
        echo "Errors found:"
        grep -E "error:|warning:" "$report_file" | head -5
    else
        echo -e "${YELLOW}? Result: UNKNOWN/ERROR${NC}"
    fi
}

# Main analysis
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   IKOS Static Analysis for Triton${NC}"
echo -e "${BLUE}========================================${NC}"

if [ -n "$1" ]; then
    # Single file analysis
    if [ -f "$1" ]; then
        analyze_file "$1"
    else
        echo -e "${RED}Error: File not found: $1${NC}"
        exit 1
    fi
else
    # Analyze critical safety components
    echo -e "\n${YELLOW}Analyzing safety-critical components...${NC}"
    
    # Priority order: safety first
    FILES=(
        "src/safety/safety_monitor.c"
        "src/safety/emergency.c"
        "src/drivers/leak.c"
        "src/drivers/battery.c"
        "src/drivers/pressure_sensor.c"
        "src/control/depth_ctrl.c"
        "src/control/ballast_ctrl.c"
    )
    
    for file in "${FILES[@]}"; do
        if [ -f "$file" ]; then
            analyze_file "$file"
        fi
    done
fi

# Cleanup
rm -rf "$TEMP_DIR"

# Summary
echo -e "\n${BLUE}========================================${NC}"
echo -e "${BLUE}         Analysis Complete${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "Reports saved to: ${REPORT_DIR}/"
echo ""

# Count results
SAFE_COUNT=$(grep -l "The program is SAFE" $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l | tr -d ' ')
UNSAFE_COUNT=$(grep -l "The program is UNSAFE" $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l | tr -d ' ')
TOTAL_COUNT=$(ls $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l | tr -d ' ')
UNKNOWN_COUNT=$((TOTAL_COUNT - SAFE_COUNT - UNSAFE_COUNT))

echo -e "Summary:"
echo -e "  ${GREEN}SAFE:     $SAFE_COUNT${NC}"
echo -e "  ${RED}UNSAFE:   $UNSAFE_COUNT${NC}"
echo -e "  ${YELLOW}UNKNOWN:  $UNKNOWN_COUNT${NC}"
echo -e "  Total:    $TOTAL_COUNT"