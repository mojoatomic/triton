#!/bin/bash
# Quick IKOS scan of all files with enhanced error handling

REPORT_DIR="ikos_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY="$REPORT_DIR/quick_scan_${TIMESTAMP}.txt"

mkdir -p "$REPORT_DIR"

echo "IKOS Quick Scan - $(date)" > "$SUMMARY"
echo "========================================" >> "$SUMMARY"
echo ""

TOTAL=0
SAFE=0
UNSAFE=0
ERRORS=0

analyze_file() {
    local file=$1
    local component=$(basename "$file" .c)
    local temp_file="/tmp/${component}_ikos.c"
    
    echo -n "Analyzing $component... "
    
    # Create simple test file
    cat > "$temp_file" << EOF
#define IKOS_ANALYSIS 1
#include "/src/analysis/pico_stubs.h"
#define printf(...) ((void)0)
EOF
    
    # Add original without hardware includes
    grep -v "#include.*hardware\|#include.*pico/" "$file" >> "$temp_file"
    
    # Add simple main
    echo "int main() { return 0; }" >> "$temp_file"
    
    # Run IKOS with timeout
    local result_file="$REPORT_DIR/${component}_quick.txt"
    
    if timeout 30s docker run --rm -v "${PWD}:/src" ikos \
        "ikos -I/src/src --analyses=boa,dbz,nullity $temp_file" > "$result_file" 2>&1; then
        
        if grep -q "The program is SAFE" "$result_file"; then
            echo "SAFE"
            echo "$component: SAFE" >> "$SUMMARY"
            SAFE=$((SAFE + 1))
        elif grep -q "The program is UNSAFE" "$result_file"; then
            echo "UNSAFE"
            echo "$component: UNSAFE" >> "$SUMMARY"
            grep "error:" "$result_file" | head -2 >> "$SUMMARY"
            UNSAFE=$((UNSAFE + 1))
        else
            echo "ERROR"
            echo "$component: ERROR" >> "$SUMMARY"
            ERRORS=$((ERRORS + 1))
        fi
    else
        echo "TIMEOUT/ERROR"
        echo "$component: TIMEOUT" >> "$SUMMARY"
        ERRORS=$((ERRORS + 1))
    fi
    
    TOTAL=$((TOTAL + 1))
    rm -f "$temp_file"
}

echo "Starting quick IKOS scan..."

# Analyze key files
echo "Safety components:"
for file in src/safety/*.c; do
    [ -f "$file" ] && analyze_file "$file"
done

echo ""
echo "Key drivers:"
for file in src/drivers/battery.c src/drivers/pressure_sensor.c src/drivers/leak.c; do
    [ -f "$file" ] && analyze_file "$file"
done

echo ""
echo "Control systems:"
for file in src/control/*.c; do
    [ -f "$file" ] && analyze_file "$file"
done

# Summary
echo ""
echo "========================================" >> "$SUMMARY"
echo "Total: $TOTAL, Safe: $SAFE, Unsafe: $UNSAFE, Errors: $ERRORS" >> "$SUMMARY"

echo "Quick scan complete!"
echo "Total: $TOTAL files"
echo "Safe: $SAFE"
echo "Unsafe: $UNSAFE" 
echo "Errors: $ERRORS"
echo ""
echo "Report: $SUMMARY"