#!/bin/bash
# Test IKOS setup with a minimal example

# Create a simple test file
cat > test_safety.c << 'EOF'
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Stub out Pico includes
#ifdef IKOS_ANALYSIS
#include "analysis/pico_stubs.h"
#else
#include "pico/stdlib.h"
#endif

// Simple function to test
int check_depth(int depth) {
    if (depth < 0) {
        return -1;  // Error
    }
    
    if (depth > 1000) {
        return -2;  // Too deep
    }
    
    // Potential buffer overflow test
    int buffer[10];
    buffer[depth % 10] = depth;  // Safe due to modulo
    
    return buffer[0];
}

int main() {
    int result = check_depth(500);
    return result;
}
EOF

echo "Testing IKOS with simple safety check..."
echo ""

# Run IKOS
docker run --rm -v "${PWD}:/work" -w /work ikos \
    ikos \
    -I/work/analysis \
    -DIKOS_ANALYSIS \
    --analyses=boa,dbz,nullity \
    --display-checks=all \
    test_safety.c

# Cleanup
rm -f test_safety.c test_safety.db