# Triton - RC Submarine Controller

## Kickoff Prompt

Copy and paste this into Claude CLI to start:

---

I need you to build the Triton RC submarine controller firmware. This is a Raspberry Pi Pico project.

**First, read these files to understand the project:**
1. Read `CLAUDE.md` for coding rules and project overview
2. Read `references/pinout.md` for GPIO assignments
3. Read `references/types.md` for data types

**Then execute these tasks IN ORDER. Complete each fully before moving to the next:**

### Phase 1: Foundation
- F1: Run `python3 scripts/init_project.py .` to create project structure
- F2: Create `src/main.c` with a simple LED blink to verify toolchain works
- F3: Modify main.c for dual-core - Core 0 blinks LED, Core 1 prints to serial
- F4: Create `src/config.h` with all pin definitions from `references/pinout.md`
- F5: Create `src/types.h` with all types from `references/types.md`

### Phase 2: Drivers (read `references/drivers.md` first)
- D1: `src/drivers/rc_input.c` - PIO-based RC PWM capture
- D2: `src/drivers/pressure_sensor.c` - MS5837 I2C driver
- D3: `src/drivers/imu.c` - MPU-6050 I2C driver  
- D4: `src/drivers/pump.c` - PWM motor control via L298N
- D5: `src/drivers/valve.c` - GPIO solenoid control
- D6: `src/drivers/servo.c` - PWM servo output
- D7: `src/drivers/battery.c` - ADC voltage monitor
- D8: `src/drivers/leak.c` - GPIO leak detection

### Phase 3: Control (read `references/control.md` first)
- C1: `src/control/pid.c` - Generic PID controller
- C2: `src/control/depth_ctrl.c` - Depth hold using PID
- C3: `src/control/pitch_ctrl.c` - Pitch stabilization using PID
- C4: `src/control/ballast_ctrl.c` - Ballast state machine
- C5: `src/control/state_machine.c` - Main operational states

### Phase 4: Safety (read `references/safety.md` first)
- S1: `src/safety/safety_monitor.c` - Core 0 fault detection at 100Hz
- S2: `src/safety/emergency.c` - Emergency blow sequence
- S3: Add watchdog to safety_monitor.c
- S4: `src/util/log.c` - Event logging

### Phase 5: Integration
- I1: Wire up all sensors in main.c, verify readings
- I2: Wire up control loops, test depth/pitch hold
- I3: Wire up safety system, test fault triggers
- I4: Full integration - everything running together

**After each task:**
1. Run `python3 ci/p10_check.py src/` to verify Power of 10 compliance
2. Fix any violations before proceeding
3. Tell me what you completed and what's next

**Rules you must follow:**
- No malloc/free - static allocation only
- All functions under 60 lines
- P10_ASSERT() for all preconditions
- Check all return values
- Follow patterns in `references/` exactly

Start with F1 now.

---

## What This Does

When you paste this into Claude CLI:
1. Claude reads all the reference files
2. Claude executes F1 (init_project.py)
3. Claude creates the blink test (F2)
4. Claude proceeds through each task in order
5. Claude validates P10 compliance after each step
6. Claude reports progress and moves to next task

## Alternative: One Phase at a Time

If you want more control, just paste the "Phase 1" section and tell Claude to start. When done, paste Phase 2, etc.
