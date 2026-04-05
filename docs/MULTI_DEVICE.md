# AGRINET Multi-Valve Deployment Guide

## Overview

The AGRINET valve controller supports multi-device deployments on a shared bus.
Each controller is assigned a unique Device ID (1-254) that determines its
I2C address or UART addressing prefix.

## Device ID Scheme

```
Device ID:  Sequential number 1-254
            - I2C address: 0x20 + Device_ID
            - UART address: @<ID>:command
            - 0xFF (255) = broadcast to ALL devices
            - Stored in EEPROM address 0x0F
            - Default on new boards: ID = 1

Max devices per bus: 254
Recommended per bus: 5-10 for reliable operation
```

## Typical Deployment (5-10 Valves)

```
Valve 1:  ID=1,  I2C=0x21,  UART=@1:open
Valve 2:  ID=2,  I2C=0x22,  UART=@2:open
Valve 3:  ID=3,  I2C=0x23,  UART=@3:open
  ...
Valve 10: ID=10, I2C=0x2A,  UART=@10:open
```

## Setting Device ID

### Via UART (before deploying on bus)

Connect each valve one at a time and set its ID:

```
devid 1     -> Sets device ID to 1 (saves to EEPROM)
devid 5     -> Sets device ID to 5
devid       -> Shows current device ID
```

### Via I2C (from master)

Write new ID to register 0x00 (DEV_ID):

```cpp
Wire.beginTransmission(0x21);  // Current address
Wire.write(0x00);              // Register: DEV_ID
Wire.write(5);                 // New ID = 5
Wire.endTransmission();
// Device now responds at 0x25 (0x20 + 5)
```

## I2C Bus Wiring

### Bus Topology

```
                   3.3V
                    |
                  4.7k  4.7k
                    |    |
Master ──SDA───────┤────┤────┤──── ... ──┤
         SCL───────┤────┤────┤──── ... ──┤
         GND───────┤────┤────┤──── ... ──┤
                   |    |    |           |
                 Valve1 Valve2 Valve3  ValveN
                 ID:1   ID:2   ID:3    ID:N
                 0x21   0x22   0x23    0x20+N
```

### Requirements

- **Pull-ups:** Single pair of 4.7k resistors to 3.3V on SDA and SCL
- **Bus length:** Max 1 meter for reliable 100kHz operation
- **Wire gauge:** 22-24 AWG for short runs, 20 AWG for longer
- **Decoupling:** 100nF capacitor near each valve controller VDD pin
- **Ground:** Common ground between all devices is CRITICAL

### Connector Standard (Recommended)

Use 4-pin JST-PH connectors for daisy-chain wiring:

```
Pin 1: VDD (3.3V or 12V depending on topology)
Pin 2: SDA
Pin 3: SCL
Pin 4: GND
```

## UART Bus Wiring (Alternative)

For UART-addressed mode, all devices share a single UART bus:

```
Master TX ──────────────────────────────────►
                |        |        |
              RX:V1    RX:V2    RX:V3

Master RX ◄──────────────────────────────────
                |        |        |
              TX:V1    TX:V2    TX:V3
              (via bus buffer / OR gate)
```

### UART Addressing Commands

```
@1:open          -> Open valve ID 1
@2:close         -> Close valve ID 2
@3:stat          -> Get status of valve ID 3
@255:stop        -> Emergency stop ALL valves (broadcast)
open             -> No prefix: executes on all (backward compat)
```

### UART Bus Limitations

- TX collision: Only one device can respond at a time
- Master must poll each device sequentially
- No hardware flow control
- For >4 devices, I2C bus is strongly recommended

## Master Controller Examples

### Arduino Master (I2C, scanning all valves)

```cpp
#include <Wire.h>

#define BASE_ADDR 0x20
#define NUM_VALVES 5
#define REG_POS 0x02
#define REG_CMD 0x01

void readAllValves() {
    for (uint8_t id = 1; id <= NUM_VALVES; id++) {
        Wire.beginTransmission(BASE_ADDR + id);
        Wire.write(REG_POS);
        Wire.endTransmission(false);
        Wire.requestFrom(BASE_ADDR + id, (uint8_t)1);
        if (Wire.available()) {
            uint8_t pos = Wire.read();
            Serial.print("Valve ");
            Serial.print(id);
            Serial.print(": ");
            Serial.print(pos);
            Serial.println("%");
        }
    }
}

void openAll() {
    for (uint8_t id = 1; id <= NUM_VALVES; id++) {
        Wire.beginTransmission(BASE_ADDR + id);
        Wire.write(REG_CMD);
        Wire.write(0x01);  // OPEN
        Wire.endTransmission();
        delay(100);  // Brief delay between commands
    }
}
```

## Commissioning Procedure

1. **Program each board** with the I2C firmware: `pio run -t upload -e stm8s003f3_i2c`
2. **Connect one valve at a time** to set unique Device ID:
   - Connect via UART
   - Send: `devid <N>` (where N = 1, 2, 3...)
   - Verify: `devid` (should echo back the ID)
3. **Wire all valves** on the I2C bus with pull-ups
4. **Scan bus** from master to verify all addresses respond
5. **Calibrate each valve:** Send calibrate command to each ID
6. **Test positions:** Send open/close/half to each valve individually
7. **Label physically:** Mark each valve with its ID number

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| No I2C response | Wrong address | Check device ID, verify with UART |
| Multiple valves respond | Duplicate IDs | Re-program one valve with unique ID |
| Bus hangs | Missing pull-ups | Add 4.7k pull-ups to SDA/SCL |
| Intermittent errors | Long bus wires | Shorten bus, add bus buffer IC |
| Wrong valve moves | ID mismatch | Verify IDs match physical labels |
