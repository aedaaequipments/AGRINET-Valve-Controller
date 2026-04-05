# AGRINET Valve Controller - Hardware Documentation

Complete hardware reference and wiring guide.

---

## Table of Contents

1. [Block Diagram](#block-diagram)
2. [Component List](#component-list)
3. [Pin Mapping](#pin-mapping)
4. [Wiring Diagram](#wiring-diagram)
5. [Power Supply](#power-supply)
6. [Motor Driver](#motor-driver)
7. [Limit Switches](#limit-switches)
8. [PCB Design Notes](#pcb-design-notes)

---

## Block Diagram

```
                            +12V DC
                               │
                    ┌──────────┴──────────┐
                    │                     │
              ┌─────┴─────┐         ┌─────┴─────┐
              │   3.3V    │         │   L293D   │
              │   LDO     │         │  DRIVER   │
              └─────┬─────┘         └─────┬─────┘
                    │                     │
              ┌─────┴─────────────────────┴─────┐
              │                                  │
              │         STM8S003F3P6             │
              │                                  │
              │  ┌────────────────────────────┐  │
              │  │      MOTOR CONTROL         │  │
              │  │  PD2 ──────► L293D IN1     │  │
              │  │  PD3 ──────► L293D IN2     │  │
              │  └────────────────────────────┘  │
              │                                  │
              │  ┌────────────────────────────┐  │
              │  │      LIMIT SWITCHES        │  │
              │  │  PC5 ◄────── CLOSED SW     │  │
              │  │  PC4 ◄────── OPEN SW       │  │
              │  └────────────────────────────┘  │
              │                                  │
              │  ┌────────────────────────────┐  │
              │  │      BUTTONS               │  │
              │  │  PC7 ◄────── UP BTN        │  │
              │  │  PC6 ◄────── DOWN BTN      │  │
              │  └────────────────────────────┘  │
              │                                  │
              │  ┌────────────────────────────┐  │
              │  │      UART (Master)         │  │
              │  │  PD5 ──────► TX            │  │
              │  │  PD6 ◄────── RX            │  │
              │  └────────────────────────────┘  │
              │                                  │
              │  ┌────────────────────────────┐  │
              │  │      STATUS LED            │  │
              │  │  PB5 ──────► LED           │  │
              │  └────────────────────────────┘  │
              │                                  │
              └──────────────────────────────────┘
                               │
                              GND
```

---

## Component List (BOM)

### Essential Components

| # | Component | Value/Part | Package | Qty | Notes |
|---|-----------|------------|---------|-----|-------|
| 1 | MCU | STM8S003F3P6 | TSSOP20 | 1 | Main controller |
| 2 | Motor Driver | L293D | DIP16 | 1 | Dual H-Bridge |
| 3 | Voltage Regulator | AMS1117-3.3 | SOT223 | 1 | 3.3V LDO |
| 4 | Capacitor | 100nF ceramic | 0805 | 3 | Decoupling |
| 5 | Capacitor | 10µF electrolytic | 5x7mm | 2 | Power filter |
| 6 | Resistor | 330Ω | 0805 | 1 | LED current limit |
| 7 | LED | 3mm Green | THT | 1 | Status indicator |
| 8 | Button | Tactile switch | 6x6mm | 2 | UP/DOWN |
| 9 | Limit Switch | Microswitch NC | - | 2 | Closed/Open |
| 10 | Connector | 2.54mm header | - | 1 | UART |
| 11 | Connector | Screw terminal | 5mm | 1 | Motor/Power |

### Optional Components

| Component | Purpose |
|-----------|---------|
| Crystal 16MHz | External clock (if HSI not used) |
| TVS Diode | ESD protection |
| Fuse 1A | Overcurrent protection |
| Reverse polarity diode | Power protection |

---

## Pin Mapping

### STM8S003F3P6 TSSOP20 Pinout

```
                    ┌────────────────────┐
           PD4   1  │○                   │ 20  PD3 ──► Motor IN2
  UART TX  PD5   2  │                    │ 19  PD2 ──► Motor IN1
  UART RX  PD6   3  │                    │ 18  PD1 (SWIM)
          NRST   4  │                    │ 17  PC7 ◄── Button UP
           PA1   5  │    STM8S003F3P6    │ 16  PC6 ◄── Button DOWN
           PA2   6  │                    │ 15  PC5 ◄── Limit CLOSED
           VSS   7  │                    │ 14  PC4 ◄── Limit OPEN
          VCAP   8  │                    │ 13  PC3
           VDD   9  │                    │ 12  PB4
           PA3  10  │                    │ 11  PB5 ──► LED
                    └────────────────────┘
```

### Pin Assignment Table

| Pin # | Port | Function | Direction | Configuration |
|-------|------|----------|-----------|---------------|
| 2 | PD5 | UART TX | Output | Alternate (UART) |
| 3 | PD6 | UART RX | Input | Alternate (UART) |
| 4 | NRST | Reset | Input | Internal pull-up |
| 7 | VSS | Ground | - | Power |
| 8 | VCAP | Capacitor | - | 100nF to GND |
| 9 | VDD | Power | - | 3.3V |
| 11 | PB5 | LED | Output | Push-pull |
| 14 | PC4 | Limit Open | Input | Pull-up enabled |
| 15 | PC5 | Limit Closed | Input | Pull-up enabled |
| 16 | PC6 | Button DOWN | Input | Pull-up enabled |
| 17 | PC7 | Button UP | Input | Pull-up enabled |
| 18 | PD1 | SWIM Debug | I/O | Programming |
| 19 | PD2 | Motor IN1 | Output | Push-pull |
| 20 | PD3 | Motor IN2 | Output | Push-pull |

---

## Wiring Diagram

### Complete Circuit

```
                                    +12V INPUT
                                        │
                    ┌───────────────────┴───────────────────┐
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │         POWER SECTION          │  │
                    │    │                                │  │
               +12V─┼────┤  +12V ──┬── L293D VCC2 (8)    │  │
                    │    │         │                      │  │
                    │    │         └── C1 10µF ── GND     │  │
                    │    │                                │  │
                    │    │  +12V ──► AMS1117 ──► +3.3V   │  │
                    │    │              │                 │  │
                    │    │              └── C2 10µF ─GND  │  │
                    │    │                                │  │
                    │    │  +3.3V ─┬── L293D VCC1 (16)   │  │
                    │    │         │                      │  │
                    │    │         ├── STM8 VDD (9)      │  │
                    │    │         │                      │  │
                    │    │         └── C3 100nF ── GND   │  │
                    │    │                                │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │         MOTOR SECTION          │  │
                    │    │                                │  │
                    │    │         ┌─────────────┐        │  │
                    │    │         │    L293D    │        │  │
                    │    │         │             │        │  │
      PD2 (Pin 19)──┼────┼─────────┤ IN1 (2)    │        │  │
                    │    │         │             │        │  │
      PD3 (Pin 20)──┼────┼─────────┤ IN2 (7)    │        │  │
                    │    │         │             │        │  │
              +3.3V─┼────┼─────────┤ EN1 (1)    │        │  │
                    │    │         │             │        │  │
                    │    │    M+◄──┤ OUT1 (3)   │        │  │
                    │    │         │             │        │  │
                    │    │    M-◄──┤ OUT2 (6)   │        │  │
                    │    │         │             │        │  │
                    │    │   GND───┤ GND (4,5)  │        │  │
                    │    │         │             │        │  │
                    │    │         └─────────────┘        │  │
                    │    │                                │  │
                    │    │              ┌─────┐           │  │
                    │    │    MOTOR ────┤  M  │           │  │
                    │    │              └─────┘           │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │       LIMIT SWITCHES           │  │
                    │    │                                │  │
                    │    │   PC5 (Pin 15)                 │  │
                    │    │       │                        │  │
                    │    │       ├───┤ NC SW ├─── GND    │  │
                    │    │       │    CLOSED              │  │
                    │    │      10K                       │  │
                    │    │       │                        │  │
                    │    │     +3.3V                      │  │
                    │    │                                │  │
                    │    │   PC4 (Pin 14)                 │  │
                    │    │       │                        │  │
                    │    │       ├───┤ NC SW ├─── GND    │  │
                    │    │       │     OPEN               │  │
                    │    │      10K                       │  │
                    │    │       │                        │  │
                    │    │     +3.3V                      │  │
                    │    │                                │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │         BUTTONS                │  │
                    │    │                                │  │
                    │    │   PC7 (Pin 17) ──┤ BTN ├── GND│  │
                    │    │                   UP           │  │
                    │    │                                │  │
                    │    │   PC6 (Pin 16) ──┤ BTN ├── GND│  │
                    │    │                  DOWN          │  │
                    │    │                                │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │         STATUS LED             │  │
                    │    │                                │  │
                    │    │   PB5 (Pin 11)                 │  │
                    │    │       │                        │  │
                    │    │      330Ω                      │  │
                    │    │       │                        │  │
                    │    │      LED                       │  │
                    │    │       │                        │  │
                    │    │      GND                       │  │
                    │    │                                │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    │    ┌───────────────────────────────┐  │
                    │    │         UART HEADER            │  │
                    │    │                                │  │
                    │    │   PD5 (Pin 2) ────► TX        │  │
                    │    │   PD6 (Pin 3) ◄──── RX        │  │
                    │    │   GND ─────────────── GND      │  │
                    │    │                                │  │
                    │    └───────────────────────────────┘  │
                    │                                       │
                    └───────────────────────────────────────┘
```

---

## Power Supply

### Specifications

| Parameter | Value |
|-----------|-------|
| Input Voltage | 12V DC (10-14V acceptable) |
| MCU Supply | 3.3V (AMS1117-3.3 LDO) |
| Motor Supply | 12V (direct from input) |
| MCU Current | ~15mA (active), <1µA (hibernate) |
| Motor Current | Up to 600mA per channel |

### Power Filtering

```
+12V ──┬── C1 (10µF) ── GND    (Input filter)
       │
       └── AMS1117 ──┬── C2 (10µF) ── GND    (Output filter)
                     │
                     └── +3.3V ──┬── C3 (100nF) ── GND    (MCU decoupling)
                                 │
                                 └── STM8 VDD
```

---

## Motor Driver

### L293D Configuration

| Pin | Name | Connection |
|-----|------|------------|
| 1 | EN1,2 | +3.3V (always enabled) |
| 2 | IN1 | PD2 (STM8) |
| 3 | OUT1 | Motor + |
| 4,5 | GND | Ground |
| 6 | OUT2 | Motor - |
| 7 | IN2 | PD3 (STM8) |
| 8 | VCC2 | +12V (motor supply) |
| 16 | VCC1 | +3.3V (logic supply) |

### Direction Control Truth Table

| IN1 (PD2) | IN2 (PD3) | Motor Action |
|-----------|-----------|--------------|
| LOW | LOW | Coast (stop) |
| HIGH | LOW | Forward (close) |
| LOW | HIGH | Reverse (open) |
| HIGH | HIGH | Brake (brief only!) |

---

## Limit Switches

### Switch Type
Use **NC (Normally Closed)** microswitches for fail-safe operation.

### Why NC Switches?

| Condition | NO Switch | NC Switch |
|-----------|-----------|-----------|
| Normal operation | Open → LOW | Closed → LOW |
| At limit | Closed → HIGH | Open → HIGH |
| **Wire broken** | Open → LOW (WRONG!) | Open → HIGH (SAFE!) |

With NC switches, a broken wire is treated the same as reaching the limit - the motor stops. This is fail-safe behavior.

### Recommended Switches

- Omron SS-5GL
- Any microswitch rated for >10,000 cycles
- Current rating: 100mA minimum

---

## PCB Design Notes

### Layout Guidelines

1. **Decoupling capacitors** close to MCU VDD/VSS pins
2. **Separate motor and logic grounds** - join at single point near power input
3. **VCAP capacitor** as close as possible to pin 8
4. **Motor traces** wider (at least 0.5mm for 600mA)
5. **Keep analog signals away** from motor driver

### Recommended Stack-up

- 2-layer PCB sufficient
- Top: Components, signal traces
- Bottom: Ground plane, power traces

### Connector Recommendations

| Connection | Connector Type |
|------------|----------------|
| Power Input | Screw terminal, 2-pin |
| Motor | Screw terminal, 2-pin |
| UART | 2.54mm header, 3-pin (TX, RX, GND) |
| SWIM | 2.54mm header, 4-pin (VCC, SWIM, GND, RST) |
| Limit Switches | Screw terminal or JST |
| Buttons | Tactile on-board or JST |
