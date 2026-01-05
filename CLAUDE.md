# RC Submarine Controller - Claude CLI Project Guide

## Project Overview

You are working on an **RC Submarine Controller** - a Raspberry Pi Pico-based embedded system that controls ballast, depth, pitch, and safety systems for a hobby RC submarine. This is a safety-critical embedded project following **NASA/JPL Power of 10** coding standards.

## Quick Start

```bash
# 1. Install dependencies
./setup.sh

# 2. Initialize project structure
python3 scripts/init_project.py .

# 3. Install git hooks
make install-hooks

# 4. Verify setup
make ci-quick
```

## Required Packages

Install these before starting development:

```bash
# Ubuntu/Debian
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

# Pico SDK (required for building)
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

## Project Structure

```
project/
├── src/
│   ├── main.c              # Entry point, dual-core init
│   ├── config.h            # Pin assignments, constants
│   ├── types.h             # Shared types, error codes
│   ├── drivers/            # Hardware abstraction
│   │   ├── rc_input.c      # PIO-based RC PWM capture
│   │   ├── pressure_sensor.c # MS5837 I2C driver
│   │   ├── imu.c           # MPU-6050 I2C driver
│   │   ├── pump.c          # PWM motor control
│   │   ├── valve.c         # GPIO solenoid control
│   │   ├── servo.c         # PWM servo output
│   │   ├── battery.c       # ADC voltage monitor
│   │   └── leak.c          # GPIO leak detection
│   ├── control/            # Control algorithms
│   │   ├── pid.c           # Generic PID controller
│   │   ├── depth_ctrl.c    # Depth hold controller
│   │   ├── pitch_ctrl.c    # Pitch stabilization
│   │   ├── ballast_ctrl.c  # Ballast state machine
│   │   └── state_machine.c # Main operational states
│   ├── safety/             # Safety systems (Core 0)
│   │   ├── safety_monitor.c # Fault detection @ 100Hz
│   │   └── emergency.c     # Emergency blow sequence
│   └── util/
│       └── log.c           # Event logging
├── pio/
│   └── pwm_capture.pio     # PIO program for RC input
├── test/                   # Unit tests (host-based)
├── ci/                     # Local CI system
├── docs/                   # Documentation
├── CMakeLists.txt
└── Makefile
```

## Architecture

**Dual-Core Design:**
- **Core 0 (Safety)**: Runs at 100Hz, monitors faults, triggers emergency blow
- **Core 1 (Control)**: Runs at 50Hz, reads sensors, runs controllers

**Safety-First**: Core 0 can override Core 1 at any time. Emergency blow is atomic and cannot be cancelled.

## Coding Standards (Power of 10)

**CRITICAL**: All code must follow these rules:

1. **No dynamic memory**: No `malloc`, `free`, `calloc`, `realloc`. Use static allocation only.
2. **Fixed loop bounds**: All loops must have deterministic upper limits. No `while(1)` except in main loops.
3. **No recursion**: Functions cannot call themselves directly or indirectly.
4. **Short functions**: Maximum 60 lines per function.
5. **Assertions**: Use `P10_ASSERT()` to validate preconditions. Minimum 1 per function.
6. **Check return values**: All function returns must be checked.
7. **Limited pointers**: Maximum 2 levels of dereferencing (`**ptr` OK, `***ptr` not OK).
8. **No goto**: Never use goto statements.
9. **Compile clean**: Code must compile with `-Wall -Werror` with zero warnings.

**Assert Macro** (triggers emergency surface on failure):
```c
#define P10_ASSERT(cond) do { \
    if (!(cond)) { \
        p10_assert_fail(__FILE__, __LINE__, #cond); \
    } \
} while(0)
```

## Development Workflow

### Before Writing Code

1. Read the relevant skill reference file:
   ```bash
   cat skill/references/drivers.md    # For driver work
   cat skill/references/control.md    # For control algorithms
   cat skill/references/safety.md     # For safety systems
   cat skill/references/pinout.md     # For pin assignments
   ```

2. Check existing task status:
   ```bash
   python3 scripts/task_status.py .
   ```

### Writing Code

1. Create files using the generator (for drivers):
   ```bash
   python3 scripts/generate_driver.py flow_sensor --type i2c_sensor -o src/drivers/
   ```

2. Follow the patterns in `skill/references/drivers.md` exactly.

3. Every function must:
   - Have `P10_ASSERT()` for parameter validation
   - Return `error_t` for fallible operations
   - Be under 60 lines
   - Have no dynamic allocation

### Testing Code

```bash
# Run unit tests
make test

# Run specific test
make test-pid

# Run P10 compliance check
python3 ci/p10_check.py src/

# Run full CI
make ci
```

### Committing Code

Git hooks will automatically run:
- Format checks (tabs, whitespace, line endings)
- P10 compliance on changed files
- cppcheck static analysis

```bash
git add .
git commit -m "feat: implement pressure sensor driver"
```

To bypass (emergency only):
```bash
git commit --no-verify -m "WIP: debugging"
```

## Task List

### Foundation (F1-F5)
- [ ] F1: Project scaffold (CMakeLists.txt, directories)
- [ ] F2: Blink test (verify toolchain)
- [ ] F3: Dual-core hello (both cores running)
- [ ] F4: Config header (pin assignments)
- [ ] F5: Types header (shared types)

### Drivers (D1-D8)
- [ ] D1: RC input (PIO PWM capture)
- [ ] D2: Pressure sensor (MS5837 I2C)
- [ ] D3: IMU (MPU-6050 I2C)
- [ ] D4: Pump (PWM + direction)
- [ ] D5: Valve (GPIO)
- [ ] D6: Servo (PWM)
- [ ] D7: Battery monitor (ADC)
- [ ] D8: Leak detector (GPIO interrupt)

### Control (C1-C5)
- [ ] C1: PID module
- [ ] C2: Depth controller
- [ ] C3: Pitch controller
- [ ] C4: Ballast state machine
- [ ] C5: Main state machine

### Safety (S1-S4)
- [ ] S1: Safety monitor (Core 0)
- [ ] S2: Emergency blow
- [ ] S3: Watchdog setup
- [ ] S4: Fault logging

### Integration (I1-I5)
- [ ] I1: Sensor integration test
- [ ] I2: Control loop integration
- [ ] I3: Safety system integration
- [ ] I4: Full system bench test
- [ ] I5: Tub test

## Key Files Reference

| File | Purpose |
|------|---------|
| `src/config.h` | All pin assignments, timing constants, limits |
| `src/types.h` | Shared types, error codes, P10_ASSERT macro |
| `skill/references/pinout.md` | GPIO assignments with wiring notes |
| `skill/references/drivers.md` | Complete driver implementations |
| `skill/references/control.md` | PID, state machines, control logic |
| `skill/references/safety.md` | Safety monitor, emergency blow |
| `skill/references/hardware.md` | BOM, wiring diagrams |

## Common Commands

```bash
# Build
make build              # Compile firmware
make clean              # Clean build
make flash              # Flash to Pico (BOOTSEL mode)

# Test
make test               # All unit tests
make test-pid           # Specific test
make ci                 # Full CI suite
make ci-quick           # Quick checks only

# Development
make monitor            # Serial console (minicom)
make todos              # List TODO/FIXME
make loc                # Lines of code

# Analysis
python3 ci/p10_check.py src/ --verbose    # P10 compliance
cppcheck --enable=all src/                 # Static analysis
```

## Error Handling Pattern

```c
error_t do_something(int param) {
    P10_ASSERT(param >= 0);
    P10_ASSERT(param < MAX_VALUE);
    
    error_t err;
    
    err = step_one();
    if (err != ERR_NONE) {
        return err;
    }
    
    err = step_two();
    if (err != ERR_NONE) {
        return err;
    }
    
    return ERR_NONE;
}
```

## Safety System Behavior

**Fault Triggers** (any causes emergency blow):
- RC signal lost for 3 seconds
- Battery voltage < 6.4V
- Leak detected
- Depth > MAX_DEPTH_CM
- Pitch > ±45°
- Watchdog timeout
- P10_ASSERT failure

**Emergency Blow Sequence**:
1. Open vent valve
2. Reverse pump at 100%
3. Full up on control surfaces
4. Log event
5. Continue until power cycle (cannot be cancelled)

## Important Notes

1. **Never skip CI**: If CI fails, fix the issue. Don't bypass without documenting why.

2. **Safety code is special**: Changes to `src/safety/` require extra review. Core 0 code must be minimal and bulletproof.

3. **Test on host first**: Unit tests run on your development machine with mocked hardware. This catches most bugs before flashing.

4. **Incremental development**: Complete one task fully (including tests) before starting the next.

5. **Document deviations**: If you must deviate from P10 rules, add a comment explaining why and suppress the warning explicitly.

## Getting Help

- **Pin assignments**: `cat skill/references/pinout.md`
- **Driver examples**: `cat skill/references/drivers.md`
- **Control algorithms**: `cat skill/references/control.md`
- **Safety system**: `cat skill/references/safety.md`
- **Hardware/wiring**: `cat skill/references/hardware.md`
- **CI issues**: `cat ci/README.md` or `make help`
