# RC Submarine Controller

Raspberry Pi Pico-based embedded controller for hobby RC submarines with ballast, depth, and pitch control systems.

**Status:** Development  
**Validation:** 6 unit test suites, 35 files P10 compliant, cppcheck static analysis clean  
**Code:** 2,352 lines C11, 17 source files, 18 header files

## What This Does

Controls RC submarine operations through dual-core safety architecture:

- **Core 0 (100Hz)**: Safety monitor, fault detection, emergency blow system
- **Core 1 (50Hz)**: Sensor reading, PID control, RC input processing, actuator control
- **Hardware**: MS5837 pressure sensor, MPU-6050 IMU, 6-channel RC input, ballast pump, control valves, servo surfaces

Safety system triggers emergency surface on: RC signal loss (3s timeout), low battery (6.4V threshold), water leak detection, depth exceeded (300cm), pitch exceeded (45 degrees), Core 1 stall (100ms detection), hardware watchdog timeout.

Control systems: Depth hold PID controller, pitch stabilization PID, ballast state machine, manual/auto mode switching.

## Limitations

- Maximum depth: 300cm (configurable in config.h)
- RC signal range: Standard 1000-2000us PWM only
- Battery monitoring: 2S LiPo only with 10k/3.3k voltage divider
- No GPS or compass navigation
- No wireless telemetry or data logging to external devices
- Emergency blow cannot be cancelled once triggered (requires power cycle)
- No automatic leak pump-out (manual recovery required)

## Hardware Requirements

- Raspberry Pi Pico (RP2040)
- MS5837 pressure sensor (I2C address 0x76)
- MPU-6050 IMU (I2C address 0x68)
- 6-channel RC receiver with PWM outputs
- Ballast pump with direction control
- Vent valve (normally closed)
- 3 servo outputs (rudder, bow/stern planes)
- Leak detection sensor (GPIO interrupt)
- 2S LiPo battery with voltage divider monitoring

Pin assignments in `src/config.h`. Wiring diagrams in `docs/hardware/`.

## Installation

Requires Pico SDK, CMake, gcc-arm-none-eabi, and cppcheck.

```bash
export PICO_SDK_PATH=~/pico-sdk
git clone https://github.com/mojoatomic/triton.git
cd triton
mkdir build && cd build
cmake ..
make
```

Flash: Hold BOOTSEL, connect USB, copy `rc_sub_controller.uf2` to RPI-RP2 drive.

## Validation

**Power of 10 Compliance:**
```bash
python3 ci/p10_check.py src/ --strict
# 35 files checked, 0 errors, 0 warnings
```

**Static Analysis:**
```bash
cppcheck --enable=all --error-exitcode=1 src/
# No issues found
```

**Unit Tests:**
```bash
cd test && ./run_tests.sh
# 6 suites: PID, State Machine, Safety Monitor, Emergency, Ballast Control, Depth Control
# Tests: 6 | Passed: 6 | Failed: 0 | Skipped: 0
```

**Integration Tests:**
- Core 1 startup handshake validation
- Cross-core heartbeat monitoring
- Emergency blow trigger verification
- Hardware watchdog timeout simulation

## Safety Features

**Dual-Core Isolation:**
- Core 0: Cannot be blocked, feeds hardware watchdog, minimal code (200 lines)
- Core 1: Complex logic isolated from safety system
- Cross-core health monitoring with 100ms stall detection

**Emergency Procedures:**
- Automatic emergency surface on any critical fault
- Hardware watchdog resets system if Core 0 stalls
- Multiple independent fault detection methods
- Emergency state cannot be cancelled (fail-safe design)

**Fault Detection:**
- Signal loss: 3000ms timeout
- Battery: 6400mV threshold with hysteresis
- Leak: GPIO interrupt with debouncing
- Depth: Configurable maximum (300cm default)
- Pitch: +/-45 degree limit with rate monitoring
- Core health: Cross-core heartbeat validation

## Development

**Code Standards:**
- NASA/JPL Power of 10 rules enforced
- Maximum 60 lines per function
- Static allocation only (no malloc/free)
- All return values checked
- Minimum 2 assertions per function

**Testing:**
```bash
make ci          # Full validation
make test        # Unit tests only
make p10-check   # Power of 10 validation
```

**Git Hooks:**
Pre-commit validation runs formatting checks, P10 compliance, and static analysis automatically.

## Known Issues

- Startup handshake timeout occurs on approximately 5% of cold boots due to hardware timing variance
- Battery voltage readings have +/-50mV accuracy with current ADC calibration
- PID controllers require manual tuning for different submarine configurations
- No compensation for water density variations (freshwater vs saltwater)

## Architecture

**Control Flow:**
1. Hardware initialization and self-test
2. Core 1 launch with startup handshake
3. Core 0 safety monitor (100Hz fixed timing)
4. Core 1 control loop (50Hz with sensor fusion)
5. Emergency procedures override all normal operation

**State Machine:**
- SURFACE: RC control, ballast disabled, safety checks active
- DIVING: Transition to underwater operation
- SUBMERGED: Full autonomous control, depth/pitch hold available
- EMERGENCY: Emergency blow active, surface immediately

**Data Flow:**
```
RC Input --> State Machine --> Controllers --> Actuators
Sensors --> Safety Monitor --> Fault Detection --> Emergency System
```

**Memory Usage:**
- Static allocation: 8KB RAM
- Stack usage: less than 2KB per core
- Flash usage: 64KB program + 32KB constants
