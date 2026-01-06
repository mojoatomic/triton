#!/bin/bash
# Direct IKOS analysis runner - single file analysis with proper parameters

if [ -z "$1" ]; then
    echo "Usage: ./ikos_direct.sh <file.c>"
    exit 1
fi

FILE=$1
BASENAME=$(basename "$FILE" .c)
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_DB="ikos_reports/${BASENAME}_${TIMESTAMP}.db"
OUTPUT_TXT="ikos_reports/${BASENAME}_${TIMESTAMP}_analysis.txt"

echo "Running IKOS analysis on: $FILE"
echo "Output: $OUTPUT_TXT"
echo ""

# Create output directory
mkdir -p ikos_reports

# Run IKOS with proper parameters
docker run --rm -v "${PWD}:/src" -w /src ikos \
    sh -c "ikos \
    -I/src/analysis \
    -I/src \
    -DIKOS_ANALYSIS \
    -DPICO_NO_HARDWARE \
    --entry-points=safety_monitor_init \
    --analyses=boa,dbz,nullity,prover,uva \
    --display-checks=all \
    --display-times=short \
    --hardware-addresses=0x40000000-0x60000000 \
    --output-db=$OUTPUT_DB \
    $FILE" 2>&1 | tee "$OUTPUT_TXT"

echo ""
echo "Analysis complete. Results saved to: $OUTPUT_TXT"

# Quick summary
if grep -q "The program is SAFE" "$OUTPUT_TXT"; then
    echo "✓ Result: SAFE"
elif grep -q "The program is UNSAFE" "$OUTPUT_TXT"; then
    echo "✗ Result: UNSAFE - Review the report for details"
else
    echo "? Result: WARNINGS or ERRORS - Review the report"
fi