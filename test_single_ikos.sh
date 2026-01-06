#!/bin/bash
# Test single file IKOS analysis

FILE=$1
if [ -z "$FILE" ]; then
    echo "Usage: $0 <file.c>"
    exit 1
fi

COMPONENT=$(basename "$FILE" .c)
TEMP_FILE="analysis/${COMPONENT}_test.c"
REPORT_FILE="ikos_reports/${COMPONENT}_test.txt"

echo "Testing IKOS on: $FILE"

# Create test file
cat > "$TEMP_FILE" << EOF
#define IKOS_ANALYSIS 1
#include "/src/analysis/pico_stubs_fixed.h"

EOF

# Add source without hardware includes and exclude BOOT_STAGE_NAMES definitions that conflict
grep -v "#include.*hardware\|#include.*pico/" "$FILE" | sed '/^const char\* const BOOT_STAGE_NAMES\[\]/,/^};/d' >> "$TEMP_FILE"

# Add printf stub AFTER all includes
cat >> "$TEMP_FILE" << EOF

// Printf stub - defined after all includes to avoid conflicts
#undef printf
static inline int stub_printf(const char* fmt, ...) { (void)fmt; return 0; }
#define printf stub_printf

// Handshake function implementations for main.c (types defined in handshake.h)
HandshakeTiming_t handshake_get_timing(void) {
    HandshakeTiming_t t = {100, 200, 300};  // Valid initialized values
    return t;
}

const char* handshake_result_str(HandshakeResult_t result) {
    (void)result;
    return "OK";  // Always returns valid string
}

EOF

# Add main function only if source doesn't have one
if ! grep -q "int main(" "$FILE"; then
cat >> "$TEMP_FILE" << EOF
int main() {
    return 0;
}
EOF
fi

echo "Running IKOS..."

# Run IKOS
docker run --rm -v "${PWD}:/src" ikos \
    "ikos \
    -I/src/src -I/src/src/drivers -I/src/src/safety -I/src/src/control -I/src/src/util \
    --analyses=boa,dbz,nullity,prover,uva \
    --hardware-addresses=0x40000000-0x60000000 \
    /src/$TEMP_FILE" 2>&1 | tee "$REPORT_FILE"

echo ""
echo "Analysis complete. Report saved to: $REPORT_FILE"

# Show result
if grep -q "The program is SAFE" "$REPORT_FILE"; then
    echo "✅ Result: SAFE"
elif grep -q "The program is UNSAFE" "$REPORT_FILE"; then
    echo "❌ Result: UNSAFE"
    echo "Issues:"
    grep "error:" "$REPORT_FILE"
else
    echo "⚠️ Result: ERROR/UNKNOWN"
fi

rm -f "$TEMP_FILE"