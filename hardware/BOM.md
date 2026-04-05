# AGRINET Valve Controller - Bill of Materials

Complete parts list for building the controller.

---

## Essential Components

| # | Part | Description | Package | Qty | Reference | Est. Cost |
|---|------|-------------|---------|-----|-----------|-----------|
| 1 | STM8S003F3P6 | 8-bit MCU, 8KB Flash, 1KB RAM | TSSOP20 | 1 | U1 | $0.50 |
| 2 | L293D | Dual H-Bridge Motor Driver | DIP16 | 1 | U2 | $1.50 |
| 3 | AMS1117-3.3 | 3.3V 1A LDO Regulator | SOT223 | 1 | U3 | $0.30 |
| 4 | 100nF | Ceramic Capacitor | 0805 | 3 | C1,C2,C3 | $0.10 |
| 5 | 10µF | Electrolytic Capacitor | 5x7mm | 2 | C4,C5 | $0.20 |
| 6 | 330Ω | Resistor | 0805 | 1 | R1 | $0.02 |
| 7 | 10kΩ | Resistor (optional pull-up) | 0805 | 2 | R2,R3 | $0.04 |
| 8 | LED Green | 3mm LED | THT | 1 | D1 | $0.10 |
| 9 | Tactile Switch | 6x6mm Momentary | THT | 2 | SW1,SW2 | $0.20 |
| 10 | Microswitch | NC Type Limit Switch | - | 2 | LS1,LS2 | $1.00 |
| 11 | Terminal Block | 2-pin 5mm Pitch | THT | 2 | J1,J2 | $0.50 |
| 12 | Header | 2.54mm 1x3 | THT | 1 | J3 | $0.10 |
| 13 | Header | 2.54mm 1x4 | THT | 1 | J4 | $0.15 |

**Estimated Total: ~$5.00**

---

## Component Details

### U1: STM8S003F3P6 Microcontroller

| Parameter | Value |
|-----------|-------|
| Manufacturer | STMicroelectronics |
| Core | STM8 |
| Flash | 8 KB |
| RAM | 1 KB |
| EEPROM | 128 bytes |
| Clock | 16 MHz (internal) |
| I/O Pins | 16 |
| Operating Voltage | 2.95V - 5.5V |
| Package | TSSOP20 |

**Alternatives:**
- STM8S003K3T6 (LQFP32) - more pins
- STM8S103F3P6 - identical, different marking

---

### U2: L293D Motor Driver

| Parameter | Value |
|-----------|-------|
| Channels | 2 (Dual H-Bridge) |
| Output Current | 600mA per channel |
| Peak Current | 1.2A |
| Logic Voltage | 4.5V - 7V |
| Motor Voltage | 4.5V - 36V |
| Package | DIP16 |

**Alternatives:**
- TB6612FNG - smaller, more efficient
- DRV8833 - lower voltage, higher current

---

### U3: AMS1117-3.3 Voltage Regulator

| Parameter | Value |
|-----------|-------|
| Output Voltage | 3.3V |
| Output Current | 1A (max) |
| Dropout | ~1.1V |
| Input Voltage | 4.5V - 12V |
| Package | SOT223 |

**Alternatives:**
- LM1117-3.3
- XC6206P332MR (lower quiescent current)

---

### LS1, LS2: Limit Switches

| Parameter | Value |
|-----------|-------|
| Type | SPDT Microswitch |
| Contact | NC (Normally Closed) |
| Current Rating | 100mA minimum |
| Actuator | Roller or Lever |
| Life | >10,000 cycles |

**Recommended:**
- Omron SS-5GL
- Omron V-156-1C25
- Generic KW11-3Z

**IMPORTANT:** Use NC (Normally Closed) switches for fail-safe operation!

---

### Motor Requirements

| Parameter | Recommended Value |
|-----------|-------------------|
| Voltage | 12V DC |
| Current | <600mA |
| Type | Gear Motor |
| Output | Ball Valve Compatible |
| Travel Time | 5-30 seconds |

**Typical Motors:**
- 12V DC Gear Motor with 1:100 ratio
- Motorized Ball Valve (DN15-DN25)
- Linear Actuator with limit switches

---

## Connector Pinouts

### J1: Power Input (2-pin Terminal)

| Pin | Signal |
|-----|--------|
| 1 | +12V |
| 2 | GND |

### J2: Motor Output (2-pin Terminal)

| Pin | Signal |
|-----|--------|
| 1 | Motor + |
| 2 | Motor - |

### J3: UART (1x3 Header)

| Pin | Signal |
|-----|--------|
| 1 | TX (PD5) |
| 2 | RX (PD6) |
| 3 | GND |

### J4: SWIM Debug (1x4 Header)

| Pin | Signal |
|-----|--------|
| 1 | VCC (3.3V) |
| 2 | SWIM (PD1) |
| 3 | GND |
| 4 | NRST |

---

## Suggested Vendors

| Vendor | URL | Notes |
|--------|-----|-------|
| LCSC | lcsc.com | Best prices, China |
| Mouser | mouser.com | Wide selection, fast shipping |
| DigiKey | digikey.com | Good availability |
| AliExpress | aliexpress.com | Cheapest, slow shipping |
| Amazon | amazon.com | Fast shipping, higher prices |

---

## Assembly Notes

1. **Start with SMD components** (U1, U3, capacitors, resistors)
2. **Add through-hole components** (U2, LED, switches, headers)
3. **Install connectors last** (terminal blocks, headers)
4. **Test power section first** before connecting MCU
5. **Program MCU** via SWIM header before connecting motor

---

## Test Points

| Signal | Location | Expected Value |
|--------|----------|----------------|
| VIN | J1-1 | 12V |
| 3V3 | U3 output | 3.3V ±5% |
| VCAP | U1 pin 8 | ~1.8V |
| TX | J3-1 | 3.3V (idle) |
| LED | D1 anode | Blinking |
