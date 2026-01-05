# Hardware Reference

Bill of materials, wiring diagrams, and assembly notes.

## Bill of Materials

### Core Electronics

| Item | Part Number | Qty | Unit | Total | Source |
|------|-------------|-----|------|-------|--------|
| Raspberry Pi Pico | SC0915 | 2 | $4 | $8 | Adafruit, DigiKey |
| MS5837-30BA Pressure Sensor | MS5837-30BA | 1 | $15 | $15 | Blue Robotics |
| MPU-6050 Breakout | GY-521 | 1 | $5 | $5 | Amazon, AliExpress |
| L298N Motor Driver | — | 1 | $3 | $3 | Amazon, AliExpress |
| Water Sensor Module | — | 1 | $2 | $2 | Amazon |

### Actuators

| Item | Description | Qty | Unit | Total | Source |
|------|-------------|-----|------|-------|--------|
| Micro Submersible Pump | 6V DC, 1-2 L/min | 1 | $5 | $5 | Amazon |
| Solenoid Valve | 6V NC, 1/4" barb | 1 | $8 | $8 | Amazon |
| 9g Micro Servo | SG90 or MG90S | 2 | $3 | $6 | Amazon |

### Power

| Item | Description | Qty | Unit | Total | Source |
|------|-------------|-----|------|-------|--------|
| 2S LiPo Battery | 7.4V, 1000-2000mAh | 1 | $15 | $15 | HobbyKing |
| 5V BEC | 3A, for servos | 1 | $5 | $5 | Amazon |
| JST Connectors | Various | 1 | $5 | $5 | Amazon |

### Passive Components

| Item | Value | Qty | Notes |
|------|-------|-----|-------|
| Resistor | 10kΩ | 3 | Battery divider, pull-ups |
| Resistor | 3.3kΩ | 1 | Battery divider |
| Resistor | 4.7kΩ | 2 | I2C pull-ups (if needed) |
| Capacitor | 100nF | 5 | Decoupling |
| Capacitor | 10µF | 2 | Power filtering |

### Miscellaneous

| Item | Description | Qty | Unit | Total |
|------|-------------|-----|------|-------|
| Perfboard | 5×7 cm | 2 | $2 | $4 |
| Header Pins | Male/female | 1 | $3 | $3 |
| Silicone Wire | 22-26 AWG, various colors | 1 | $8 | $8 |
| Heat Shrink | Assorted | 1 | $5 | $5 |
| Waterproof Enclosure | For controller | 1 | $10 | $10 |

### Total Estimated Cost

| Category | Cost |
|----------|------|
| Core Electronics | $33 |
| Actuators | $19 |
| Power | $25 |
| Passives | ~$5 |
| Misc | $30 |
| **Total** | **~$112** |

*Note: Assumes existing RC transmitter and receiver.*

---

## Wiring Diagram

### Main Interconnect

```
                                    ┌─────────────────────────────────┐
                                    │         2S LiPo 7.4V            │
                                    │           Battery               │
                                    └───────────────┬─────────────────┘
                                                    │
                        ┌───────────────────────────┼───────────────────────────┐
                        │                           │                           │
                        ▼                           ▼                           ▼
                 ┌──────────────┐           ┌──────────────┐           ┌──────────────┐
                 │   5V BEC     │           │   L298N      │           │ Voltage      │
                 │   (Servos)   │           │   Motor      │           │ Divider      │
                 └──────┬───────┘           │   Driver     │           └──────┬───────┘
                        │                   └──────┬───────┘                  │
                        │                          │                          │
        ┌───────────────┼──────────────────────────┼──────────────────────────┼───────┐
        │               │                          │                          │       │
        │    ┌──────────┴──────────┐    ┌─────────┴─────────┐    ┌───────────┴────┐  │
        │    │      SERVOS         │    │   PUMP + VALVE    │    │   Pico ADC     │  │
        │    │  Rudder (GPIO10)    │    │   PWM (GPIO14)    │    │   (GPIO26)     │  │
        │    │  Bowplane (GPIO11)  │    │   DIR (GPIO15)    │    └────────────────┘  │
        │    │  Sternplane (GPIO12)│    │   Valve (GPIO13)  │                        │
        │    └─────────────────────┘    └───────────────────┘                        │
        │                                                                            │
        │    ┌─────────────────────────────────────────────────────────────────┐    │
        │    │                      RASPBERRY PI PICO                          │    │
        │    │                                                                 │    │
        │    │   RC Input (PIO)           I2C Sensors            Safety       │    │
        │    │   GPIO 0-5                 GPIO 8,9 (SDA,SCL)     GPIO 16,25   │    │
        │    │        ▲                        │                     │        │    │
        │    └────────┼────────────────────────┼─────────────────────┼────────┘    │
        │             │                        │                     │             │
        │    ┌────────┴────────┐    ┌─────────┴─────────┐    ┌──────┴──────┐      │
        │    │   RC Receiver   │    │   MS5837   MPU6050│    │ Leak Sensor │      │
        │    │   (6 channel)   │    │   Pressure   IMU  │    │ Status LED  │      │
        │    └─────────────────┘    └───────────────────┘    └─────────────┘      │
        │                                                                          │
        └──────────────────────────────────────────────────────────────────────────┘
```

### Detailed Pin Connections

#### RC Receiver to Pico

```
RC Receiver                 Pico
───────────                 ────
CH1 Signal ────────────────► GPIO 0
CH2 Signal ────────────────► GPIO 1
CH3 Signal ────────────────► GPIO 2
CH4 Signal ────────────────► GPIO 3
CH5 Signal ────────────────► GPIO 4
CH6 Signal ────────────────► GPIO 5
GND ───────────────────────► GND
(+5V from BEC, not Pico)
```

#### I2C Bus

```
                    3.3V
                     │
                    ┌┴┐
               4.7k │ │    (Pull-ups - check if modules have them)
                    └┬┘
                     │
MS5837              │         MPU-6050              Pico
──────              │         ────────              ────
SDA ────────────────┼─────────── SDA ─────────────► GPIO 8
SCL ────────────────┼─────────── SCL ─────────────► GPIO 9
VCC ────────────────┼─────────── VCC ─────────────► 3.3V
GND ────────────────┼─────────── GND ─────────────► GND
```

#### Motor Driver (L298N)

```
L298N                           Pico / Power
─────                           ────────────
ENA ──────────────────────────► GPIO 14 (PWM)
IN1 ──────────────────────────► GPIO 15 (Direction)
IN2 ──────────────────────────► Tied to opposite of IN1*
OUT1 ─────────────────────────► Pump Motor +
OUT2 ─────────────────────────► Pump Motor -
+12V ─────────────────────────► Battery + (7.4V OK)
GND ──────────────────────────► Battery -, Pico GND

* Use inverter or second GPIO for IN2
```

#### Solenoid Valve

```
                     7.4V
                      │
                      │
            ┌─────────┴─────────┐
            │    Solenoid       │
            │      Valve        │
            └─────────┬─────────┘
                      │
                      │ D1 (flyback diode)
                    ──┴──
                    ▲   │
                    └───┤
                        │
                      │/
  GPIO 13 ──[1k]────│   NPN (2N2222 or similar)
                      │\
                        │
                       GND
```

#### Battery Monitor

```
Battery +
    │
   ─┴─
  │   │ 10kΩ
   ─┬─
    │
    ├──────────────────► GPIO 26 (ADC0)
    │
   ─┴─
  │   │ 3.3kΩ
   ─┬─
    │
   GND
   
Divider ratio: 3.3 / (10 + 3.3) = 0.248
Max input: 3.3V / 0.248 = 13.3V (safe for 2S-3S)
```

#### Leak Sensor

```
3.3V
 │
─┴─
│ │ 10kΩ (pull-down)
─┬─
 │
 ├──────────────────► GPIO 16
 │
[Sensor Probes]
 │
GND

When dry: GPIO 16 = LOW
When wet: GPIO 16 = HIGH (water conducts)
```

---

## Assembly Notes

### Pressure Sensor (MS5837)

The MS5837-30BA is designed for underwater use but requires proper installation:

1. **Waterproofing**: The sensor face must be exposed to water. Use marine epoxy to seal the cable entry point.

2. **Mounting**: Mount with sensor facing downward to prevent air bubbles.

3. **Cable**: Keep I2C cable short (<30 cm) or use shielded cable.

4. **Depth Rating**: 30 bar = 300 meters. Far exceeds hobby submarine needs.

### Pump Installation

1. **Intake**: Must be submerged (inside ballast tank).

2. **Output**: Route to outside hull for filling, or to vent for draining.

3. **Priming**: Ensure pump can self-prime or pre-fill lines.

4. **Direction**: Test which polarity fills vs drains.

### Solenoid Valve

1. **Position**: At top of ballast tank for venting air.

2. **Normally Closed**: Use NC valve so it holds water when unpowered.

3. **Flyback Diode**: REQUIRED - solenoid is inductive load.

4. **Flow Rate**: Match to pump output.

### Servo Installation

1. **Waterproofing**: Either use waterproof servos or apply silicone grease to standard servos.

2. **Linkage**: Keep short and direct to minimize slop.

3. **Travel Limits**: Mechanical stops should match servo travel.

---

## Waterproofing Strategy

### Electronics Enclosure

**Option 1: Dry Hull**
- All electronics in watertight compartment
- Only sensor cables penetrate hull
- Use cable glands or potted penetrators

**Option 2: Flooded Compartment**
- Conformal coat PCBs
- Pot critical connections in marine epoxy
- MS5837 and leak sensor directly exposed

### Recommended Approach

For hobby submarine, use **dry hull with penetrators**:

1. Main electronics in waterproof box (IP67 or better)
2. MS5837 mounted externally with cable gland
3. Pump and valve in separate wet compartment
4. Servos external with silicone grease

### Cable Penetrators

```
       Outside (Wet)                    Inside (Dry)
            │                                │
            │     ┌──────────────────┐      │
  Cable ────┼─────│   Cable Gland   │──────┼──── Cable
            │     │   (PG7 or M12)  │      │
            │     └──────────────────┘      │
            │                                │
         ───┴────────────────────────────────┴───
                     Hull Wall
```

Use PG7 cable glands for small cables (sensor wires).
Use PG9 or larger for motor cables.
Apply silicone sealant for extra protection.

---

## Testing Checklist

### Before Sealing Hull

- [ ] All I2C devices respond (scan with `i2cdetect`)
- [ ] RC receiver outputs valid PWM on all channels
- [ ] Pump runs in both directions
- [ ] Valve opens and closes
- [ ] All servos move full range
- [ ] Battery voltage reads correctly
- [ ] Leak sensor triggers when wet
- [ ] Pressure sensor reads atmospheric (~1013 mbar)
- [ ] IMU reads reasonable pitch/roll

### Bench Test (Dry)

- [ ] Emergency blow triggers on button press
- [ ] Signal loss failsafe works (disconnect RC)
- [ ] Low battery failsafe works (use bench supply)
- [ ] State machine transitions correctly

### Tub Test

- [ ] Leak sensor doesn't false-trigger
- [ ] Pressure sensor tracks water depth
- [ ] Ballast fills and drains correctly
- [ ] Buoyancy is roughly neutral when ballast half-full
- [ ] Emergency blow brings model to surface
