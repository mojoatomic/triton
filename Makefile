# RC Submarine Controller Makefile
.PHONY: all build clean flash test ci ci-quick monitor todos loc help install-hooks

# Default target
all: build

# Build firmware
build:
	@echo "Building RC Submarine Controller..."
	@mkdir -p build
	@cd build && cmake .. && make -j4

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build
	@rm -rf test/build
	@rm -rf ikos_out

# Flash to Pico (requires BOOTSEL mode)
flash: build
	@echo "Flashing to Pico (make sure device is in BOOTSEL mode)..."
	@if [ -d "/media/$(USER)/RPI-RP2" ]; then \
		cp build/rc_sub_controller.uf2 /media/$(USER)/RPI-RP2/; \
		echo "Flash complete!"; \
	else \
		echo "ERROR: Pico not found in BOOTSEL mode at /media/$(USER)/RPI-RP2"; \
		exit 1; \
	fi

# Run all unit tests
test:
	@echo "Running unit tests..."
	@cd test && ./run_tests.sh

# Run specific test
test-%:
	@echo "Running test_$*..."
	@cd test && ./run_tests.sh test_$*

# Run full CI suite
ci:
	@echo "Running full CI suite..."
	@bash ci/run_ci.sh

# Quick CI checks
ci-quick:
	@echo "Running quick CI checks..."
	@bash ci/run_ci.sh --quick

# Monitor serial output
monitor:
	@echo "Opening serial monitor (Ctrl+A, Ctrl+X to exit)..."
	@minicom -D /dev/ttyACM0 -b 115200

# List TODOs and FIXMEs
todos:
	@echo "TODOs and FIXMEs in source:"
	@grep -rn --color=always "TODO\|FIXME" src/ || echo "No TODOs or FIXMEs found!"

# Count lines of code
loc:
	@echo "Lines of code:"
	@find src -name "*.c" -o -name "*.h" | xargs wc -l | sort -n

# Install git hooks
install-hooks:
	@echo "Installing git hooks..."
	@bash ci/install_hooks.sh

# Help
help:
	@echo "RC Submarine Controller - Make targets:"
	@echo ""
	@echo "  make build        - Build firmware"
	@echo "  make clean        - Clean build artifacts"
	@echo "  make flash        - Flash to Pico (BOOTSEL mode required)"
	@echo "  make test         - Run all unit tests"
	@echo "  make test-<name>  - Run specific test (e.g., make test-pid)"
	@echo "  make ci           - Run full CI suite"
	@echo "  make ci-quick     - Run quick CI checks"
	@echo "  make monitor      - Open serial monitor"
	@echo "  make todos        - List TODOs and FIXMEs"
	@echo "  make loc          - Count lines of code"
	@echo "  make install-hooks - Install git hooks"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Common workflow:"
	@echo "  1. make install-hooks  (first time only)"
	@echo "  2. make build"
	@echo "  3. make test"
	@echo "  4. make ci-quick"
	@echo "  5. make flash"
	@echo "  6. make monitor"