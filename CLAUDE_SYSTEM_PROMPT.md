# Claude CLI System Prompt - RC Submarine Controller

Copy this to your project's `.claude/instructions.md` or use as the system prompt.

---

You are developing an RC Submarine Controller - a Raspberry Pi Pico (RP2040) embedded system with dual-core safety architecture. This is a safety-critical project following NASA/JPL Power of 10 coding standards.

## Architecture
- **Core 0 (Safety)**: 100Hz watchdog, fault detection, emergency blow
- **Core 1 (Control)**: 50Hz sensors, PID loops, state machine

## Critical Rules (Power of 10)
1. **No malloc/free** - Static allocation only
2. **Fixed loop bounds** - No unbounded while/for loops
3. **No recursion** - Functions cannot call themselves
4. **Functions ≤60 lines** - Keep functions short
5. **P10_ASSERT()** - Validate all preconditions
6. **Check return values** - Never ignore errors
7. **-Wall -Werror** - Zero warnings allowed

## Essential Commands
```bash
./scripts/setup.sh              # Install dependencies (first time)
make ci-quick                   # Quick validation
make test                       # Run unit tests
make build                      # Build firmware
python3 ci/p10_check.py src/    # Check P10 compliance
```

## Project Structure
```
src/
├── config.h            # Pin assignments
├── types.h             # Shared types, P10_ASSERT
├── drivers/            # Hardware (rc_input, pressure_sensor, imu, pump, valve, servo, battery, leak)
├── control/            # Algorithms (pid, depth_ctrl, pitch_ctrl, ballast_ctrl, state_machine)
└── safety/             # Core 0 (safety_monitor, emergency)
```

## Before Writing Code
1. Read `skill/references/` for implementation patterns
2. Use `python3 scripts/generate_driver.py <name> --type <type>` for boilerplate
3. Every function needs: P10_ASSERT checks, error_t return, <60 lines

## Error Pattern
```c
error_t foo(int x) {
    P10_ASSERT(x >= 0);
    error_t err = bar();
    if (err != ERR_NONE) return err;
    return ERR_NONE;
}
```

## Key References
- `skill/references/pinout.md` - GPIO assignments
- `skill/references/drivers.md` - Driver implementations
- `skill/references/control.md` - PID, state machines
- `skill/references/safety.md` - Safety system

## CI Enforcement
Git hooks automatically check:
- Formatting (tabs, whitespace)
- P10 compliance
- Static analysis (cppcheck)

Never skip CI. If it fails, fix the issue.
