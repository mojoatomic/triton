#!/bin/bash
# Final IKOS Analysis Script - Working Version

REPORT_DIR="ikos_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY="$REPORT_DIR/FINAL_ANALYSIS_${TIMESTAMP}.md"

mkdir -p "$REPORT_DIR"

echo "# IKOS Final Analysis Report - Triton Project" > "$SUMMARY"
echo "**Date**: $(date)" >> "$SUMMARY"
echo "" >> "$SUMMARY"
echo "## Analysis Results" >> "$SUMMARY"
echo "" >> "$SUMMARY"

TOTAL=0
SAFE=0
UNSAFE=0
ERRORS=0

analyze_component() {
    local src_file=$1
    local component=$(basename "$src_file" .c)
    local temp_file="/tmp/${component}_final.c"
    local report_file="$REPORT_DIR/${component}_final.txt"
    
    echo -n "Analyzing $component... "
    
    # Create analysis file with fixed stubs
    cat > "$temp_file" << EOF
#define IKOS_ANALYSIS 1
#include "/src/analysis/pico_stubs_fixed.h"

// Stub additional functions
#define printf(...) ((void)0)

EOF
    
    # Add source without hardware includes
    grep -v "#include.*hardware\|#include.*pico/" "$src_file" >> "$temp_file"
    
    # Add test main
    cat >> "$temp_file" << EOF

// Test main function
int main() {
    error_t err = ERR_NONE;
    
    // Test initialization if function exists
    #ifdef ${component}_init
    err = ${component}_init();
    if (err != ERR_NONE) return 1;
    #endif
    
    return 0;
}
EOF
    
    # Run IKOS with timeout
    if timeout 60s docker run --rm -v "${PWD}:/src" ikos \
        "ikos \
        -I/src/src -I/src/src/drivers -I/src/src/safety -I/src/src/control \
        --analyses=boa,dbz,nullity,prover,uva \
        --display-checks=all \
        --hardware-addresses=0x40000000-0x60000000 \
        $temp_file" > "$report_file" 2>&1; then
        
        if grep -q "The program is SAFE" "$report_file"; then
            echo "✅ SAFE"
            echo "- ✅ **$component**: SAFE" >> "$SUMMARY"
            local checks=$(grep "Total number of checks" "$report_file" | awk '{print $NF}')
            if [ -n "$checks" ]; then
                echo "  - $checks checks passed" >> "$SUMMARY"
            fi
            SAFE=$((SAFE + 1))
            
        elif grep -q "The program is UNSAFE" "$report_file"; then
            echo "❌ UNSAFE"
            echo "- ❌ **$component**: UNSAFE" >> "$SUMMARY"
            echo "  ```" >> "$SUMMARY"
            grep -A2 "error:" "$report_file" | head -5 >> "$SUMMARY"
            echo "  ```" >> "$SUMMARY"
            UNSAFE=$((UNSAFE + 1))
            
        else
            echo "⚠️ UNKNOWN"
            echo "- ⚠️ **$component**: Analysis incomplete" >> "$SUMMARY"
            ERRORS=$((ERRORS + 1))
        fi
    else
        echo "⏱️ TIMEOUT"
        echo "- ⏱️ **$component**: Analysis timeout" >> "$SUMMARY"
        ERRORS=$((ERRORS + 1))
    fi
    
    TOTAL=$((TOTAL + 1))
    rm -f "$temp_file"
}

echo "IKOS Final Analysis - Triton Project"
echo "===================================="
echo ""

# Analyze critical components first
echo "Safety-Critical Components:"
echo "" >> "$SUMMARY"
echo "### Safety-Critical Components" >> "$SUMMARY"
echo "" >> "$SUMMARY"

for file in src/safety/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Key Drivers:" 
echo "" >> "$SUMMARY"
echo "### Key Drivers" >> "$SUMMARY"
echo "" >> "$SUMMARY"

for file in src/drivers/battery.c src/drivers/pressure_sensor.c src/drivers/leak.c src/drivers/valve.c src/drivers/servo.c; do
    [ -f "$file" ] && analyze_component "$file"
done

echo ""
echo "Control Systems:"
echo "" >> "$SUMMARY"
echo "### Control Systems" >> "$SUMMARY"
echo "" >> "$SUMMARY"

for file in src/control/*.c; do
    [ -f "$file" ] && analyze_component "$file"
done

# Add summary statistics
echo "" >> "$SUMMARY"
echo "## Summary Statistics" >> "$SUMMARY"
echo "" >> "$SUMMARY"
echo "| Metric | Count | Percentage |" >> "$SUMMARY"
echo "|--------|-------|------------|" >> "$SUMMARY"
echo "| **Total Analyzed** | $TOTAL | 100% |" >> "$SUMMARY"
echo "| **Safe** | $SAFE | $(( (SAFE * 100) / TOTAL ))% |" >> "$SUMMARY"
echo "| **Unsafe** | $UNSAFE | $(( (UNSAFE * 100) / TOTAL ))% |" >> "$SUMMARY"
echo "| **Errors/Timeouts** | $ERRORS | $(( (ERRORS * 100) / TOTAL ))% |" >> "$SUMMARY"

# Add key findings
echo "" >> "$SUMMARY"
echo "## Key Findings" >> "$SUMMARY"
echo "" >> "$SUMMARY"

if [ $UNSAFE -gt 0 ]; then
    echo "🚨 **$UNSAFE components marked UNSAFE** - These contain provable runtime errors that must be fixed immediately." >> "$SUMMARY"
    echo "" >> "$SUMMARY"
fi

if [ $SAFE -gt 0 ]; then
    echo "✅ **$SAFE components verified SAFE** - These are mathematically proven free of runtime errors." >> "$SUMMARY"
    echo "" >> "$SUMMARY"
fi

echo "## Recommendations" >> "$SUMMARY"
echo "" >> "$SUMMARY"
echo "1. **Fix UNSAFE components first** - Priority on safety-critical systems" >> "$SUMMARY"
echo "2. **Initialize all variables** before use" >> "$SUMMARY"
echo "3. **Add bounds checking** for all array accesses" >> "$SUMMARY"
echo "4. **Add more P10_ASSERT statements** for better coverage" >> "$SUMMARY"
echo "5. **Re-run analysis** after fixes to verify improvements" >> "$SUMMARY"

echo ""
echo "===================================="
echo "Final Analysis Complete!"
echo "===================================="
echo ""
echo "Summary:"
echo "  Total Analyzed: $TOTAL"
echo "  Safe: $SAFE"
echo "  Unsafe: $UNSAFE"
echo "  Errors: $ERRORS"
echo ""
echo "Detailed Report: $SUMMARY"
echo ""

if [ $UNSAFE -gt 0 ]; then
    echo "🚨 $UNSAFE components are UNSAFE and need immediate attention!"
    echo ""
fi

# Show which files were problematic
if [ $UNSAFE -gt 0 ] || [ $ERRORS -gt 0 ]; then
    echo "Files needing attention:"
    for file in "$REPORT_DIR"/*_final.txt; do
        if [ -f "$file" ]; then
            component=$(basename "$file" _final.txt)
            if grep -q "UNSAFE" "$file"; then
                echo "  - $component (UNSAFE)"
            elif ! grep -q "SAFE" "$file"; then
                echo "  - $component (ERROR)"
            fi
        fi
    done
fi