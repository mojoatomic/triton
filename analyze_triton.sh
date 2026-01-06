#!/bin/bash
# analyze_triton.sh - Run IKOS static analysis on Triton codebase
#
# Usage: ./analyze_triton.sh [file.c]
#        ./analyze_triton.sh           # Analyze all C files
#        ./analyze_triton.sh src/main.c # Analyze specific file

set -e

DOCKER_IMAGE="ikos"
REPORT_DIR="ikos_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo " IKOS Static Analysis - Triton Project"
echo "=========================================="

# Check Docker is running
if ! docker info > /dev/null 2>&1; then
    echo -e "${RED}Error: Docker is not running${NC}"
    echo "Start Docker Desktop and try again"
    exit 1
fi

# Check IKOS image exists
if ! docker image inspect $DOCKER_IMAGE > /dev/null 2>&1; then
    echo -e "${RED}Error: IKOS Docker image not found${NC}"
    echo "Build it first with: docker build -t ikos -f Dockerfile.ikos ."
    exit 1
fi

# Create report directory
mkdir -p $REPORT_DIR

# Function to analyze a single file
analyze_file() {
    local file=$1
    local basename=$(basename "$file" .c)
    local report="$REPORT_DIR/${basename}_${TIMESTAMP}.txt"
    
    echo -e "${YELLOW}Analyzing: $file${NC}"
    
    # Run IKOS with common checks for embedded safety code
    docker run --rm -v "${PWD}:/src" $DOCKER_IMAGE \
        ikos \
            -I/src/analysis \
            -DIKOS_ANALYSIS \
            --entry-points=main \
            --analyses=boa,dbz,nullity,prover,uva \
            --display-checks=all \
            --display-times=short \
            --hardware-addresses=0x40000000-0x60000000 \
            --output-db=/src/$REPORT_DIR/${basename}.db \
            /src/$file 2>&1 | tee "$report"
    
    # Check result
    if grep -q "The program is definitely SAFE" "$report"; then
        echo -e "${GREEN}✓ $file: SAFE${NC}"
    elif grep -q "The program is definitely UNSAFE" "$report"; then
        echo -e "${RED}✗ $file: UNSAFE - Review $report${NC}"
    else
        echo -e "${YELLOW}? $file: WARNINGS - Review $report${NC}"
    fi
    echo ""
}

# Analyze specific file or all files
if [ -n "$1" ]; then
    # Single file analysis
    if [ -f "$1" ]; then
        analyze_file "$1"
    else
        echo -e "${RED}Error: File not found: $1${NC}"
        exit 1
    fi
else
    # Analyze all C files in key directories
    echo "Analyzing all source files..."
    echo ""
    
    # Safety-critical files first
    for file in src/safety/*.c; do
        [ -f "$file" ] && analyze_file "$file"
    done
    
    # Drivers
    for file in src/drivers/*.c; do
        [ -f "$file" ] && analyze_file "$file"
    done
    
    # Control
    for file in src/control/*.c; do
        [ -f "$file" ] && analyze_file "$file"
    done
    
    # Main
    [ -f "src/main.c" ] && analyze_file "src/main.c"
fi

echo "=========================================="
echo " Analysis Complete"
echo " Reports saved to: $REPORT_DIR/"
echo "=========================================="

# Summary
echo ""
echo "Summary:"
SAFE_COUNT=$(grep -l "definitely SAFE" $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l)
UNSAFE_COUNT=$(grep -l "definitely UNSAFE" $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l)
WARNING_COUNT=$(ls $REPORT_DIR/*_${TIMESTAMP}.txt 2>/dev/null | wc -l)
WARNING_COUNT=$((WARNING_COUNT - SAFE_COUNT - UNSAFE_COUNT))

echo -e "  ${GREEN}SAFE:     $SAFE_COUNT${NC}"
echo -e "  ${RED}UNSAFE:   $UNSAFE_COUNT${NC}"
echo -e "  ${YELLOW}WARNINGS: $WARNING_COUNT${NC}"

# View results interactively (if any db files exist)
if ls $REPORT_DIR/*.db 1> /dev/null 2>&1; then
    echo ""
    echo "To view detailed results interactively:"
    echo "  docker run --rm -v \${PWD}:/src -p 8080:8080 ikos \\"
    echo "    \"ikos-view /src/$REPORT_DIR/<filename>.db\""
fi
