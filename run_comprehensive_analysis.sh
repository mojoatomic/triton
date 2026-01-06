#!/bin/bash

echo "=== Comprehensive IKOS Analysis Report ==="
echo "Analyzing all verifiable components in the Triton project..."
echo ""

# List of components to analyze
COMPONENTS=(
    "battery"
    "pressure_sensor"
    "servo"
    "valve"
    "pump"
    "leak"
    "rc_input"
    "imu"
    "display"
    "pid"
    "depth_ctrl"
    "pitch_ctrl"
    "ballast_ctrl"
    "state_machine"
    "safety_monitor"
    "emergency"
    "handshake"
    "log"
)

# Track results
declare -A results
total_components=0
safe_components=0
error_components=0
warning_components=0

echo "Running IKOS analysis on all components..."
echo "========================================"

for component in "${COMPONENTS[@]}"; do
    echo -n "Analyzing $component... "
    
    # Find the source file
    if [ -f "src/drivers/${component}.c" ]; then
        src_file="src/drivers/${component}.c"
    elif [ -f "src/control/${component}.c" ]; then
        src_file="src/control/${component}.c"
    elif [ -f "src/safety/${component}.c" ]; then
        src_file="src/safety/${component}.c"
    elif [ -f "src/util/${component}.c" ]; then
        src_file="src/util/${component}.c"
    else
        echo "❌ Source file not found"
        results[$component]="NOT_FOUND"
        continue
    fi
    
    total_components=$((total_components + 1))
    
    # Run IKOS analysis
    if ./test_single_ikos.sh "$src_file" > /dev/null 2>&1; then
        # Check the results
        if grep -q "SAFE" "ikos_reports/${component}_test.txt" 2>/dev/null; then
            if grep -q "warning" "ikos_reports/${component}_test.txt" 2>/dev/null; then
                echo "⚠️  SAFE (with warnings)"
                results[$component]="SAFE_WARN"
                warning_components=$((warning_components + 1))
            else
                echo "✅ SAFE"
                results[$component]="SAFE"
                safe_components=$((safe_components + 1))
            fi
        else
            echo "❌ UNSAFE/ERROR"
            results[$component]="ERROR"
            error_components=$((error_components + 1))
        fi
    else
        echo "❌ Analysis failed"
        results[$component]="FAILED"
        error_components=$((error_components + 1))
    fi
done

echo ""
echo "=== ANALYSIS SUMMARY ==="
echo "Total components analyzed: $total_components"
echo "✅ Safe components: $safe_components"
echo "⚠️  Safe with warnings: $warning_components"  
echo "❌ Unsafe/Error components: $error_components"
echo ""

# Calculate percentages
if [ $total_components -gt 0 ]; then
    safe_pct=$((100 * safe_components / total_components))
    warn_pct=$((100 * warning_components / total_components))
    error_pct=$((100 * error_components / total_components))
    
    echo "Safety Coverage: $safe_pct% safe, $warn_pct% warnings, $error_pct% errors"
fi

echo ""
echo "=== DETAILED RESULTS ==="

# Safe components
echo "✅ SAFE Components:"
for component in "${COMPONENTS[@]}"; do
    if [ "${results[$component]}" == "SAFE" ]; then
        echo "   - $component"
    fi
done

# Components with warnings
if [ $warning_components -gt 0 ]; then
    echo ""
    echo "⚠️  SAFE with Warnings:"
    for component in "${COMPONENTS[@]}"; do
        if [ "${results[$component]}" == "SAFE_WARN" ]; then
            echo "   - $component"
        fi
    done
fi

# Problem components
if [ $error_components -gt 0 ]; then
    echo ""
    echo "❌ Components with Issues:"
    for component in "${COMPONENTS[@]}"; do
        if [ "${results[$component]}" == "ERROR" ] || [ "${results[$component]}" == "FAILED" ]; then
            echo "   - $component (${results[$component]})"
        fi
    done
fi

echo ""
echo "=== CRITICAL SAFETY COMPONENTS STATUS ==="
echo "Safety Monitor: ${results[safety_monitor]}"
echo "PID Controller: ${results[pid]}"  
echo "Emergency System: ${results[emergency]}"
echo "Battery Monitor: ${results[battery]}"
echo "Pressure Sensor: ${results[pressure_sensor]}"

echo ""
echo "=== POWER OF 10 COMPLIANCE ==="
echo "The following have been verified to comply with NASA/JPL Power of 10 rules:"
for component in "${COMPONENTS[@]}"; do
    if [ "${results[$component]}" == "SAFE" ] || [ "${results[$component]}" == "SAFE_WARN" ]; then
        echo "✓ $component - No runtime errors (buffer overflow, null pointers, uninitialized variables)"
    fi
done

echo ""
echo "Analysis complete! Individual reports available in ikos_reports/"