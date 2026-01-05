#!/bin/bash
#
# RC Submarine Controller - Development Environment Setup
#
# This script installs all required dependencies and configures
# the development environment for the RC Submarine Controller project.
#
# Usage:
#   ./setup.sh              # Full setup
#   ./setup.sh --no-sdk     # Skip Pico SDK installation
#   ./setup.sh --check      # Check what's installed
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_SDK=true
CHECK_ONLY=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-sdk)
            INSTALL_SDK=false
            shift
            ;;
        --check)
            CHECK_ONLY=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --no-sdk    Skip Pico SDK installation"
            echo "  --check     Check installed dependencies only"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_header() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
}

check_command() {
    local cmd=$1
    local pkg=$2
    if command -v "$cmd" &> /dev/null; then
        echo -e "  ${GREEN}✓${NC} $cmd"
        return 0
    else
        echo -e "  ${RED}✗${NC} $cmd (install: $pkg)"
        return 1
    fi
}

#------------------------------------------------------------------------------
# Check mode
#------------------------------------------------------------------------------

if [[ "$CHECK_ONLY" == true ]]; then
    print_header "Checking Dependencies"
    
    echo ""
    echo "Build tools:"
    check_command gcc "build-essential" || true
    check_command cmake "cmake" || true
    check_command make "build-essential" || true
    
    echo ""
    echo "ARM toolchain:"
    check_command arm-none-eabi-gcc "gcc-arm-none-eabi" || true
    
    echo ""
    echo "Analysis tools:"
    check_command cppcheck "cppcheck" || true
    check_command python3 "python3" || true
    
    echo ""
    echo "Utilities:"
    check_command git "git" || true
    check_command minicom "minicom" || true
    
    echo ""
    echo "Pico SDK:"
    if [[ -n "$PICO_SDK_PATH" && -d "$PICO_SDK_PATH" ]]; then
        echo -e "  ${GREEN}✓${NC} PICO_SDK_PATH=$PICO_SDK_PATH"
    else
        echo -e "  ${RED}✗${NC} PICO_SDK_PATH not set or directory doesn't exist"
    fi
    
    exit 0
fi

#------------------------------------------------------------------------------
# Full installation
#------------------------------------------------------------------------------

print_header "RC Submarine Controller - Setup"

echo ""
echo "This script will install:"
echo "  • Build tools (gcc, cmake, make)"
echo "  • ARM cross-compiler (gcc-arm-none-eabi)"
echo "  • Static analysis (cppcheck)"
echo "  • Python 3"
echo "  • Serial tools (minicom)"
if [[ "$INSTALL_SDK" == true ]]; then
    echo "  • Raspberry Pi Pico SDK"
fi
echo ""
read -p "Continue? [Y/n] " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]?$ ]]; then
    echo "Aborted."
    exit 1
fi

#------------------------------------------------------------------------------
# Detect OS
#------------------------------------------------------------------------------

print_header "Detecting System"

if [[ "$(uname)" == "Darwin" ]]; then
    OS="macos"
    echo "Detected: macOS $(sw_vers -productVersion)"
elif [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS=$ID
    echo "Detected: $PRETTY_NAME"
else
    echo -e "${RED}Error: Cannot detect OS${NC}"
    exit 1
fi

#------------------------------------------------------------------------------
# Install packages
#------------------------------------------------------------------------------

print_header "Installing Packages"

case $OS in
    macos)
        echo "Using Homebrew package manager..."
        if ! command -v brew &> /dev/null; then
            echo -e "${RED}Error: Homebrew not installed${NC}"
            echo "Install from: https://brew.sh"
            exit 1
        fi
        brew update
        brew install \
            cmake \
            arm-none-eabi-gcc \
            cppcheck \
            python3 \
            git \
            minicom
        ;;
    ubuntu|debian|pop)
        echo "Using apt package manager..."
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            gcc-arm-none-eabi \
            libnewlib-arm-none-eabi \
            libstdc++-arm-none-eabi-newlib \
            cppcheck \
            python3 \
            python3-pip \
            git \
            minicom
        ;;
    fedora)
        echo "Using dnf package manager..."
        sudo dnf install -y \
            gcc gcc-c++ make \
            cmake \
            arm-none-eabi-gcc-cs \
            arm-none-eabi-newlib \
            cppcheck \
            python3 \
            python3-pip \
            git \
            minicom
        ;;
    arch|manjaro)
        echo "Using pacman package manager..."
        sudo pacman -Syu --noconfirm \
            base-devel \
            cmake \
            arm-none-eabi-gcc \
            arm-none-eabi-newlib \
            cppcheck \
            python \
            python-pip \
            git \
            minicom
        ;;
    *)
        echo -e "${YELLOW}Warning: Unsupported OS '$OS'${NC}"
        echo "Please install manually:"
        echo "  - gcc, g++, make, cmake"
        echo "  - arm-none-eabi-gcc, arm-none-eabi-newlib"
        echo "  - cppcheck, python3, git, minicom"
        ;;
esac

echo -e "${GREEN}✓ Packages installed${NC}"

#------------------------------------------------------------------------------
# Install Pico SDK
#------------------------------------------------------------------------------

if [[ "$INSTALL_SDK" == true ]]; then
    print_header "Installing Pico SDK"
    
    SDK_PATH="${PICO_SDK_PATH:-$HOME/pico-sdk}"
    
    if [[ -d "$SDK_PATH" ]]; then
        echo "Pico SDK already exists at $SDK_PATH"
        echo "Updating..."
        cd "$SDK_PATH"
        git pull
        git submodule update --init
    else
        echo "Cloning Pico SDK to $SDK_PATH..."
        git clone https://github.com/raspberrypi/pico-sdk.git "$SDK_PATH"
        cd "$SDK_PATH"
        git submodule update --init
    fi
    
    # Add to shell profile
    SHELL_RC=""
    if [[ "$OS" == "macos" ]]; then
        # macOS defaults to zsh
        if [[ -f "$HOME/.zshrc" ]]; then
            SHELL_RC="$HOME/.zshrc"
        elif [[ -f "$HOME/.bash_profile" ]]; then
            SHELL_RC="$HOME/.bash_profile"
        fi
    elif [[ -f "$HOME/.bashrc" ]]; then
        SHELL_RC="$HOME/.bashrc"
    elif [[ -f "$HOME/.zshrc" ]]; then
        SHELL_RC="$HOME/.zshrc"
    fi
    
    if [[ -n "$SHELL_RC" ]]; then
        if ! grep -q "PICO_SDK_PATH" "$SHELL_RC"; then
            echo "" >> "$SHELL_RC"
            echo "# Pico SDK" >> "$SHELL_RC"
            echo "export PICO_SDK_PATH=$SDK_PATH" >> "$SHELL_RC"
            echo -e "${GREEN}✓ Added PICO_SDK_PATH to $SHELL_RC${NC}"
        else
            echo "PICO_SDK_PATH already in $SHELL_RC"
        fi
    fi
    
    export PICO_SDK_PATH="$SDK_PATH"
    echo -e "${GREEN}✓ Pico SDK installed at $SDK_PATH${NC}"
fi

#------------------------------------------------------------------------------
# Setup project
#------------------------------------------------------------------------------

print_header "Setting Up Project"

cd "$SCRIPT_DIR"

# Make scripts executable
chmod +x ci/*.sh ci/*.py test/*.sh scripts/*.py 2>/dev/null || true
echo -e "${GREEN}✓ Scripts made executable${NC}"

# Install git hooks
if [[ -d .git ]]; then
    ./ci/install_hooks.sh
else
    echo "Not a git repository, skipping hook installation"
fi

# Initialize project structure if needed
if [[ ! -d src ]]; then
    echo "Initializing project structure..."
    python3 scripts/init_project.py . --force
fi

#------------------------------------------------------------------------------
# Verify installation
#------------------------------------------------------------------------------

print_header "Verifying Installation"

ERRORS=0

check_or_fail() {
    if ! check_command "$1" "$2"; then
        ((ERRORS++))
    fi
}

check_or_fail gcc "build-essential"
check_or_fail cmake "cmake"
check_or_fail arm-none-eabi-gcc "gcc-arm-none-eabi"
check_or_fail cppcheck "cppcheck"
check_or_fail python3 "python3"
check_or_fail git "git"

if [[ -n "$PICO_SDK_PATH" && -d "$PICO_SDK_PATH" ]]; then
    echo -e "  ${GREEN}✓${NC} PICO_SDK_PATH=$PICO_SDK_PATH"
else
    echo -e "  ${RED}✗${NC} PICO_SDK_PATH not set"
    ((ERRORS++))
fi

#------------------------------------------------------------------------------
# Summary
#------------------------------------------------------------------------------

echo ""
print_header "Setup Complete"

if [[ $ERRORS -eq 0 ]]; then
    echo -e "${GREEN}All dependencies installed successfully!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Source your shell profile: source ~/.bashrc"
    echo "  2. Run quick CI check: make ci-quick"
    echo "  3. Build the project: make build"
    echo "  4. Run tests: make test"
    echo ""
    echo "For help: make help"
else
    echo -e "${YELLOW}Setup completed with $ERRORS warnings.${NC}"
    echo "Some features may not work until missing dependencies are installed."
fi

# Remind to source profile
if [[ "$INSTALL_SDK" == true ]]; then
    echo ""
    if [[ "$OS" == "macos" ]]; then
        echo -e "${YELLOW}NOTE: Run 'source ~/.zshrc' to use PICO_SDK_PATH in this session${NC}"
    else
        echo -e "${YELLOW}NOTE: Run 'source ~/.bashrc' to use PICO_SDK_PATH in this session${NC}"
    fi
fi
