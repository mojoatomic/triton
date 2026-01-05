#!/bin/bash
#
# RC Submarine Controller - Local CI Runner
#
# Runs all checks that would run in CI, locally.
# Use before committing to catch issues early.
#
# Usage:
#   ./ci/run_ci.sh              # Run all checks
#   ./ci/run_ci.sh --quick      # Skip slow checks (build, unit tests)
#   ./ci/run_ci.sh --fix        # Auto-fix what can be fixed
#   ./ci/run_ci.sh build        # Run only build check
#   ./ci/run_ci.sh lint         # Run only linting
#   ./ci/run_ci.sh test         # Run only unit tests

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Counters
PASS=0
FAIL=0
SKIP=0

# Options
QUICK=false
FIX=false
SPECIFIC=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --quick|-q)
            QUICK=true
            shift
            ;;
        --fix|-f)
            FIX=true
            shift
            ;;
        build|lint|test|format|p10|all)
            SPECIFIC="$1"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options] [check]"
            echo ""
            echo "Options:"
            echo "  --quick, -q    Skip slow checks (build, unit tests)"
            echo "  --fix, -f      Auto-fix what can be fixed"
            echo "  --help, -h     Show this help"
            echo ""
            echo "Checks:"
            echo "  all            Run all checks (default)"
            echo "  build          Compile the project"
            echo "  lint           Run cppcheck static analysis"
            echo "  test           Run unit tests"
            echo "  format         Check code formatting"
            echo "  p10            Power of 10 compliance"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Default to all checks
if [[ -z "$SPECIFIC" ]]; then
    SPECIFIC="all"
fi

#------------------------------------------------------------------------------
# Helper functions
#------------------------------------------------------------------------------

print_header() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
}

print_step() {
    echo -e "${YELLOW}▶ $1${NC}"
}

print_pass() {
    echo -e "${GREEN}✓ $1${NC}"
    PASS=$((PASS + 1))
}

print_fail() {
    echo -e "${RED}✗ $1${NC}"
    FAIL=$((FAIL + 1))
}

print_skip() {
    echo -e "${YELLOW}○ $1 (skipped)${NC}"
    SKIP=$((SKIP + 1))
}

should_run() {
    [[ "$SPECIFIC" == "all" || "$SPECIFIC" == "$1" ]]
}

#------------------------------------------------------------------------------
# Check: Source file formatting
#------------------------------------------------------------------------------

check_format() {
    print_header "Code Formatting"
    
    local errors=0
    
    # Check for tabs (should use spaces)
    print_step "Checking for tabs..."
    if grep -rn $'\t' --include="*.c" --include="*.h" "$PROJECT_ROOT/src" 2>/dev/null; then
        echo -e "${RED}  Found tabs - use 4 spaces instead${NC}"
        errors=$((errors + 1))
    else
        echo "  No tabs found"
    fi
    
    # Check for trailing whitespace
    print_step "Checking for trailing whitespace..."
    if grep -rn ' $' --include="*.c" --include="*.h" "$PROJECT_ROOT/src" 2>/dev/null; then
        echo -e "${RED}  Found trailing whitespace${NC}"
        if [[ "$FIX" == true ]]; then
            echo "  Fixing..."
            if sed --version >/dev/null 2>&1; then
                find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" | xargs sed -i 's/[[:space:]]*$//'
            else
                find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" -print0 | \
                    xargs -0 sed -i '' -e 's/[[:space:]]*$//'
            fi
        else
            errors=$((errors + 1))
        fi
    else
        echo "  No trailing whitespace"
    fi
    
    # Check line length (max 100 chars recommended, 120 hard limit)
    print_step "Checking line lengths..."
    local long_lines=$(find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" | \
        xargs awk 'length > 120 {print FILENAME ":" NR ": " length " chars"}' 2>/dev/null | head -20)
    if [[ -n "$long_lines" ]]; then
        echo -e "${RED}  Lines over 120 characters:${NC}"
        echo "$long_lines"
        errors=$((errors + 1))
    else
        echo "  All lines within limits"
    fi
    
    # Check for CRLF line endings
    print_step "Checking line endings..."
    if find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" | xargs file | grep -q CRLF 2>/dev/null; then
        echo -e "${RED}  Found CRLF line endings - use LF only${NC}"
        errors=$((errors + 1))
    else
        echo "  Line endings OK (LF)"
    fi
    
    if [[ $errors -eq 0 ]]; then
        print_pass "Format check passed"
        return 0
    else
        print_fail "Format check failed ($errors issues)"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: Power of 10 compliance
#------------------------------------------------------------------------------

check_p10() {
    print_header "Power of 10 Compliance"
    
    local validator="$SCRIPT_DIR/p10_check.py"
    
    if [[ ! -f "$validator" ]]; then
        print_skip "P10 validator not found"
        return 0
    fi
    
    print_step "Running Power of 10 validator..."
    
    if python3 "$validator" "$PROJECT_ROOT/src" --strict; then
        print_pass "Power of 10 check passed"
        return 0
    else
        print_fail "Power of 10 check failed"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: Static analysis with cppcheck
#------------------------------------------------------------------------------

check_lint() {
    print_header "Static Analysis (cppcheck)"
    
    if ! command -v cppcheck &> /dev/null; then
        print_fail "cppcheck not installed"
        if [[ "$(uname)" == "Darwin" ]]; then
            echo "  Install with: brew install cppcheck"
        else
            echo "  Install with: sudo apt install cppcheck"
        fi
        return 1
    fi
    
    print_step "Running cppcheck..."
    
    local cppcheck_opts=(
        --enable=warning,style,performance,portability
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --suppress=nullPointerRedundantCheck
        --suppress=compareValueOutOfTypeRangeError
        --inline-suppr
        -I "$PROJECT_ROOT/src"
        -I "$PROJECT_ROOT/include"
        --std=c11
    )
    
    # Add Pico SDK include path if available
    if [[ -n "$PICO_SDK_PATH" ]]; then
        cppcheck_opts+=(-I "$PICO_SDK_PATH/src/common/pico_base/include")
        cppcheck_opts+=(-I "$PICO_SDK_PATH/src/rp2_common/hardware_gpio/include")
        # Suppress issues in third-party Pico SDK headers (e.g., inline asm syntax)
        cppcheck_opts+=(--suppress="*:${PICO_SDK_PATH}/*")
    fi
    
    if cppcheck "${cppcheck_opts[@]}" "$PROJECT_ROOT/src" 2>&1; then
        print_pass "Static analysis passed"
        return 0
    else
        print_fail "Static analysis failed"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: Compiler warnings
#------------------------------------------------------------------------------

check_build() {
    print_header "Build Check"
    
    if [[ "$QUICK" == true ]]; then
        print_skip "Build (--quick mode)"
        return 0
    fi
    
    local build_dir="$PROJECT_ROOT/build"
    
    # Check for Pico SDK
    if [[ -z "$PICO_SDK_PATH" ]]; then
        print_skip "Build (PICO_SDK_PATH not set)"
        echo "  Set with: export PICO_SDK_PATH=~/pico-sdk"
        return 0
    fi
    
    print_step "Configuring with CMake..."
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    if ! cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. > cmake_output.txt 2>&1; then
        cat cmake_output.txt
        print_fail "CMake configuration failed"
        return 1
    fi
    
    print_step "Building project..."
    
    local jobs=4
    if command -v nproc >/dev/null 2>&1; then
        jobs=$(nproc)
    elif command -v sysctl >/dev/null 2>&1; then
        jobs=$(sysctl -n hw.ncpu)
    fi

    # Capture build output to check for warnings
    if make -j"$jobs" 2>&1 | tee build_output.txt; then
        # Check if there were any warnings (our CMakeLists.txt uses -Werror, so warnings = errors)
        if grep -q "warning:" build_output.txt; then
            print_fail "Build has warnings (treated as errors)"
            return 1
        fi
        print_pass "Build succeeded"
        return 0
    else
        print_fail "Build failed"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: Unit tests
#------------------------------------------------------------------------------

check_test() {
    print_header "Unit Tests"
    
    if [[ "$QUICK" == true ]]; then
        print_skip "Unit tests (--quick mode)"
        return 0
    fi
    
    local test_dir="$PROJECT_ROOT/test"
    local test_runner="$test_dir/run_tests.sh"
    
    if [[ ! -d "$test_dir" ]] || [[ ! -f "$test_runner" ]]; then
        print_skip "Unit tests (no test directory)"
        return 0
    fi
    
    print_step "Running unit tests..."
    
    cd "$test_dir"
    if bash run_tests.sh; then
        print_pass "Unit tests passed"
        return 0
    else
        print_fail "Unit tests failed"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: Header guards
#------------------------------------------------------------------------------

check_headers() {
    print_header "Header Guard Check"
    
    local errors=0
    
    print_step "Checking header guards..."
    
    while IFS= read -r -d '' header; do
        local filename=$(basename "$header" .h)
        local guard="${filename^^}_H"
        
        if ! grep -q "#ifndef $guard" "$header"; then
            echo -e "${RED}  Missing/incorrect guard in: $header${NC}"
            echo "  Expected: #ifndef $guard"
            errors=$((errors + 1))
        fi
    done < <(find "$PROJECT_ROOT/src" -name "*.h" -print0 2>/dev/null)
    
    if [[ $errors -eq 0 ]]; then
        print_pass "Header guards OK"
        return 0
    else
        print_fail "Header guard issues ($errors files)"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check: TODO/FIXME audit
#------------------------------------------------------------------------------

check_todos() {
    print_header "TODO/FIXME Audit"
    
    print_step "Scanning for TODO and FIXME comments..."
    
    local count=$(grep -rn "TODO\|FIXME\|XXX\|HACK" --include="*.c" --include="*.h" "$PROJECT_ROOT/src" 2>/dev/null | wc -l)
    
    if [[ $count -gt 0 ]]; then
        echo -e "${YELLOW}  Found $count TODO/FIXME comments:${NC}"
        grep -rn "TODO\|FIXME\|XXX\|HACK" --include="*.c" --include="*.h" "$PROJECT_ROOT/src" 2>/dev/null | head -20
        if [[ $count -gt 20 ]]; then
            echo "  ... and $((count - 20)) more"
        fi
    else
        echo "  No TODO/FIXME comments found"
    fi
    
    # This is informational, not a failure
    print_pass "TODO audit complete ($count items)"
    return 0
}

#------------------------------------------------------------------------------
# Check: Include what you use
#------------------------------------------------------------------------------

check_includes() {
    print_header "Include Analysis"
    
    print_step "Checking for common include issues..."
    
    local errors=0
    
    # Check that .c files include their own .h
    while IFS= read -r -d '' source; do
        local filename=$(basename "$source" .c)
        local header="${filename}.h"
        
        # Skip main.c
        [[ "$filename" == "main" ]] && continue
        
        if ! grep -q "#include.*$header" "$source"; then
            echo -e "${YELLOW}  $source doesn't include $header${NC}"
        fi
    done < <(find "$PROJECT_ROOT/src" -name "*.c" -print0 2>/dev/null)
    
    # Check for duplicate includes
    while IFS= read -r -d '' file; do
        local dups=$(grep "^#include" "$file" | sort | uniq -d)
        if [[ -n "$dups" ]]; then
            echo -e "${RED}  Duplicate includes in $file:${NC}"
            echo "$dups"
            errors=$((errors + 1))
        fi
    done < <(find "$PROJECT_ROOT/src" -name "*.c" -o -name "*.h" -print0 2>/dev/null)
    
    if [[ $errors -eq 0 ]]; then
        print_pass "Include analysis passed"
        return 0
    else
        print_fail "Include issues found"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------

print_header "RC Submarine Controller - Local CI"
echo "Project: $PROJECT_ROOT"
echo "Mode: $SPECIFIC $([ "$QUICK" == true ] && echo "(quick)") $([ "$FIX" == true ] && echo "(fix)")"

cd "$PROJECT_ROOT"

# Track overall status
OVERALL=0

# Run checks based on selection
if should_run "format"; then
    check_format || OVERALL=1
fi

if should_run "p10"; then
    check_p10 || OVERALL=1
fi

if should_run "lint"; then
    check_lint || OVERALL=1
fi

if should_run "all"; then
    check_headers || OVERALL=1
    check_includes || OVERALL=1
    check_todos || true  # Informational only
fi

if should_run "build"; then
    check_build || OVERALL=1
fi

if should_run "test"; then
    check_test || OVERALL=1
fi

#------------------------------------------------------------------------------
# Summary
#------------------------------------------------------------------------------

print_header "Summary"

echo -e "  ${GREEN}Passed: $PASS${NC}"
echo -e "  ${RED}Failed: $FAIL${NC}"
echo -e "  ${YELLOW}Skipped: $SKIP${NC}"
echo ""

if [[ $OVERALL -eq 0 ]]; then
    echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ✓ All checks passed - ready to commit!${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
    exit 0
else
    echo -e "${RED}════════════════════════════════════════════════════════════${NC}"
    echo -e "${RED}  ✗ Some checks failed - please fix before committing${NC}"
    echo -e "${RED}════════════════════════════════════════════════════════════${NC}"
    exit 1
fi
