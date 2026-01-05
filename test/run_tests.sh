#!/bin/bash
#
# Unit Test Runner for RC Submarine Controller
#
# Runs host-based unit tests (no hardware required)
# Tests are compiled for the host machine, not the Pico
#
# Usage:
#   ./test/run_tests.sh           # Run all tests
#   ./test/run_tests.sh test_pid  # Run specific test
#   ./test/run_tests.sh --verbose # Verbose output

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DIR="$SCRIPT_DIR"
BUILD_DIR="$TEST_DIR/build"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

VERBOSE=false
SPECIFIC_TEST=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        *)
            SPECIFIC_TEST="$1"
            shift
            ;;
    esac
done

echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  RC Submarine Controller - Unit Tests${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

# Create build directory
mkdir -p "$BUILD_DIR"

# Find all test files
if [[ -n "$SPECIFIC_TEST" ]]; then
    TEST_FILES=("$TEST_DIR/${SPECIFIC_TEST}.c")
    if [[ ! -f "${TEST_FILES[0]}" ]]; then
        echo -e "${RED}Test not found: $SPECIFIC_TEST${NC}"
        exit 1
    fi
else
    TEST_FILES=("$TEST_DIR"/test_*.c)
fi

if [[ ${#TEST_FILES[@]} -eq 0 ]] || [[ ! -f "${TEST_FILES[0]}" ]]; then
    echo -e "${YELLOW}No test files found${NC}"
    echo "Create test files as test/test_*.c"
    exit 0
fi

PASSED=0
FAILED=0
SKIPPED=0

# Compile and run each test
for test_file in "${TEST_FILES[@]}"; do
    test_name=$(basename "$test_file" .c)
    test_bin="$BUILD_DIR/$test_name"
    
    echo ""
    echo -e "${YELLOW}▶ $test_name${NC}"
    
    # Compile test
    # Include mocks and the source file being tested
    INCLUDES="-I$PROJECT_ROOT/src -I$TEST_DIR/mocks -I$TEST_DIR"
    CFLAGS="-Wall -Wextra -g -DUNIT_TEST"
    
    # Find corresponding source files to link
    # Convention: test_pid.c tests src/control/pid.c
    source_name="${test_name#test_}"
    source_files=""
    source_dir=""
    extra_sources=""

    # Search for source file in various directories
    for dir in control drivers safety util; do
        if [[ -f "$PROJECT_ROOT/src/$dir/${source_name}.c" ]]; then
            source_files="$PROJECT_ROOT/src/$dir/${source_name}.c"
            source_dir="$dir"
            break
        fi
    done

    # Link common dependencies (keep explicit to avoid dragging Pico-only drivers into host builds)
    if [[ "$source_dir" == "control" && "$source_name" != "pid" ]]; then
        # Controllers often depend on PID
        if [[ -f "$PROJECT_ROOT/src/control/pid.c" ]]; then
            extra_sources="$PROJECT_ROOT/src/control/pid.c"
        fi
    fi

    # Compile
    compile_cmd="gcc $CFLAGS $INCLUDES -o $test_bin $test_file $source_files $extra_sources $TEST_DIR/mocks/*.c 2>&1"
    
    if [[ "$VERBOSE" == true ]]; then
        echo "  Compiling: $compile_cmd"
    fi
    
    if compile_output=$(eval $compile_cmd 2>&1); then
        if [[ "$VERBOSE" == true && -n "$compile_output" ]]; then
            echo "$compile_output"
        fi
    else
        echo -e "${RED}  Compilation failed:${NC}"
        echo "$compile_output"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Run test
    echo "  Running..."
    
    set +e
    test_output=$("$test_bin" 2>&1)
    test_exit_code=$?
    set -e
    
    if [[ $test_exit_code -eq 0 ]]; then
        echo "$test_output" | sed 's/^/    /'
        echo -e "${GREEN}  ✓ PASSED${NC}"
        PASSED=$((PASSED + 1))
    else
        echo "$test_output" | sed 's/^/    /'
        echo -e "${RED}  ✗ FAILED (exit code: $test_exit_code)${NC}"
        FAILED=$((FAILED + 1))
    fi
done

# Summary
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "  Tests: $((PASSED + FAILED + SKIPPED)) | ${GREEN}Passed: $PASSED${NC} | ${RED}Failed: $FAILED${NC} | ${YELLOW}Skipped: $SKIPPED${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

if [[ $FAILED -gt 0 ]]; then
    exit 1
fi

exit 0
