#!/bin/bash
# Comprehensive IKOS Analysis for Triton Project

REPORT_DIR="ikos_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_FILE="$REPORT_DIR/IKOS_SUMMARY_${TIMESTAMP}.md"

# Create report directory
mkdir -p "$REPORT_DIR"

# Initialize summary
cat > "$SUMMARY_FILE" << EOF
# IKOS Static Analysis Report - Triton Project
Date: $(date)

## Summary

EOF

# Track statistics
TOTAL_FILES=0
SAFE_FILES=0
UNSAFE_FILES=0
ERROR_FILES=0

# Function to analyze a file
analyze_component() {
    local file=$1
    local component=$(basename "$file" .c)
    
    echo ""
    echo "Analyzing: $component"
    echo "========================================="
    
    TOTAL_FILES=$((TOTAL_FILES + 1))
    
    # Run the wrapper
    ./ikos_wrapper.sh "$file" > /dev/null 2>&1
    
    # Find the latest report for this component
    REPORT=$(ls -t "$REPORT_DIR/${component}_"*_analysis.txt 2>/dev/null | head -1)
    
    if [ -z "$REPORT" ]; then
        echo "❌ ERROR: Analysis failed"
        ERROR_FILES=$((ERROR_FILES + 1))
        echo "- **$component**: ❌ ERROR - Analysis failed" >> "$SUMMARY_FILE"
        return
    fi
    
    # Parse results
    if grep -q "The program is SAFE" "$REPORT"; then
        echo "✅ SAFE"
        SAFE_FILES=$((SAFE_FILES + 1))
        echo "- **$component**: ✅ SAFE" >> "$SUMMARY_FILE"
    elif grep -q "The program is UNSAFE" "$REPORT"; then
        echo "❌ UNSAFE"
        UNSAFE_FILES=$((UNSAFE_FILES + 1))
        echo "- **$component**: ❌ UNSAFE" >> "$SUMMARY_FILE"
        
        # Extract error details
        echo "  Issues found:" >> "$SUMMARY_FILE"
        grep -A1 "error:" "$REPORT" | sed 's/^/    /' >> "$SUMMARY_FILE"
        echo "" >> "$SUMMARY_FILE"
    else
        echo "⚠️  UNKNOWN/ERROR"
        ERROR_FILES=$((ERROR_FILES + 1))
        echo "- **$component**: ⚠️  ERROR" >> "$SUMMARY_FILE"
    fi
    
    # Extract check counts if available
    CHECKS=$(grep "Total number of checks" "$REPORT" | awk '{print $NF}')
    SAFE_CHECKS=$(grep "Total number of safe checks" "$REPORT" | awk '{print $NF}')
    
    if [ -n "$CHECKS" ] && [ -n "$SAFE_CHECKS" ]; then
        echo "  Checks: $SAFE_CHECKS/$CHECKS safe"
    fi
}

echo "Starting IKOS Analysis for Triton Project"
echo "=========================================="

# Analyze components in priority order
echo "" >> "$SUMMARY_FILE"
echo "### Safety-Critical Components" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

for file in src/drivers/battery.c \
            src/drivers/leak.c \
            src/drivers/pressure_sensor.c \
            src/safety/emergency.c \
            src/safety/safety_monitor.c; do
    if [ -f "$file" ]; then
        analyze_component "$file"
    fi
done

echo "" >> "$SUMMARY_FILE"
echo "### Control Components" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

for file in src/control/pid.c \
            src/control/depth_ctrl.c \
            src/control/ballast_ctrl.c \
            src/control/pitch_ctrl.c; do
    if [ -f "$file" ]; then
        analyze_component "$file"
    fi
done

echo "" >> "$SUMMARY_FILE"
echo "### Driver Components" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

for file in src/drivers/pump.c \
            src/drivers/valve.c \
            src/drivers/servo.c \
            src/drivers/rc_input.c \
            src/drivers/imu.c; do
    if [ -f "$file" ]; then
        analyze_component "$file"
    fi
done

# Add statistics to summary
cat >> "$SUMMARY_FILE" << EOF

## Statistics

| Metric | Count |
|--------|-------|
| Total Files Analyzed | $TOTAL_FILES |
| Safe Files | $SAFE_FILES |
| Unsafe Files | $UNSAFE_FILES |
| Failed Analysis | $ERROR_FILES |

## Key Findings

EOF

# Add key findings
if [ $UNSAFE_FILES -gt 0 ]; then
    echo "### Critical Issues Found:" >> "$SUMMARY_FILE"
    echo "" >> "$SUMMARY_FILE"
    
    # Extract all errors
    for report in "$REPORT_DIR"/*_analysis.txt; do
        if grep -q "The program is UNSAFE" "$report"; then
            component=$(basename "$report" | cut -d'_' -f1)
            echo "**$component:**" >> "$SUMMARY_FILE"
            grep -B1 -A1 "error:" "$report" | sed 's/^/  /' >> "$SUMMARY_FILE"
            echo "" >> "$SUMMARY_FILE"
        fi
    done
else
    echo "No critical issues found in analyzed components." >> "$SUMMARY_FILE"
fi

# Add recommendations
cat >> "$SUMMARY_FILE" << EOF

## Recommendations

1. **Fix Unsafe Components First**: Address all components marked as UNSAFE immediately
2. **Review Warning Components**: Components with errors need investigation
3. **Add Missing Assertions**: IKOS works better with more P10_ASSERT statements
4. **Initialize All Variables**: Ensure all variables are initialized before use
5. **Check Array Bounds**: All array accesses should be bounds-checked

## Next Steps

1. Fix identified issues in unsafe components
2. Re-run analysis after fixes
3. Add more assertions to improve analysis coverage
4. Consider adding hardware address ranges for embedded pointers
EOF

echo ""
echo "=========================================="
echo "Analysis Complete!"
echo "=========================================="
echo ""
echo "Summary Statistics:"
echo "  Total Files:  $TOTAL_FILES"
echo "  Safe:         $SAFE_FILES"
echo "  Unsafe:       $UNSAFE_FILES"
echo "  Errors:       $ERROR_FILES"
echo ""
echo "Full report: $SUMMARY_FILE"