# AGRINET Valve Controller - I2C Slave Protocol v4.4

## Overview

The AGRINET valve controller operates as an I2C slave device on PB4 (SDA) / PB5 (SCL).
Each valve has a unique Device ID (1-254) which determines its I2C address.

**I2C Address:** `0x20 + Device_ID` (7-bit addressing, standard mode 100kHz)

| Device ID | I2C Address | Binary |
|-----------|-------------|--------|
| 1 (default) | 0x21 | 0100001 |
| 2 | 0x22 | 0100010 |
| 5 | 0x25 | 0100101 |
| 10 | 0x2A | 0101010 |

## Register Map (8 Bytes)

| Reg | Name | R/W | Default | Description |
|-----|------|-----|---------|-------------|
| 0x00 | DEV_ID | R/W | 0x01 | Device ID (1-254). Writing changes bus address. |
| 0x01 | CMD | W | - | Command register. Write to trigger action. |
| 0x02 | POSITION | R | 0xFF | Current valve position (0-100, 0xFF=unknown) |
| 0x03 | FLAGS | R | 0x00 | Status flags (see below) |
| 0x04 | ERROR | R | 0x00 | Last error code |
| 0x05 | TRAVEL_H | R | 0x00 | Calibrated travel time MSB (milliseconds) |
| 0x06 | TRAVEL_L | R | 0x00 | Calibrated travel time LSB (milliseconds) |
| 0x07 | FW_VER | R | 0x44 | Firmware version (high nibble=major, low=minor) |

### FLAGS Register (0x03) Bit Map

| Bit | Name | Description |
|-----|------|-------------|
| 0 | CALIBRATED | 1 = Valve travel time has been calibrated |
| 1 | MOVING | 1 = Motor is currently running |
| 2 | FAULT | 1 = System is in fault state |
| 3 | SLEEPING | 1 = Controller is in sleep/hibernate mode |
| 4-7 | Reserved | Always 0 |

### Command Bytes (Register 0x01)

| Byte | Command | Description |
|------|---------|-------------|
| 0x00 | STOP | Emergency stop motor |
| 0x01 | OPEN | Move to 100% (fully open) |
| 0x02 | CLOSE | Move to 0% (fully closed) |
| 0x03 | QUARTER | Move to 25% open |
| 0x04 | HALF | Move to 50% open |
| 0x05 | THREE_QTR | Move to 75% open |
| 0x06 | CALIBRATE | Run auto-calibration |
| 0x07 | SAVE | Save state to EEPROM |
| 0x08 | SLEEP | Enter sleep mode |
| 0x09 | RESET | System reset |
| 0x0A | CLEAR | Clear EEPROM data |

### Error Codes (Register 0x04)

| Code | Name | Description |
|------|------|-------------|
| 0 | OK | No error |
| 1 | STALL | Motor stall detected |
| 2 | TMO | Movement timeout |
| 3 | LIM | Limit switch failure |
| 4 | STUCK | Limit switch stuck |
| 5 | CAL | Calibration failed |
| 6 | RANGE | Travel time out of range |
| 7 | POS | Position unknown |
| 8 | EE | EEPROM error |
| 9 | CMD | Invalid command |
| 10 | WDT | Watchdog reset occurred |

## I2C Transaction Examples

### Read All Registers
```
Master: [START] [0x42 W] [0x00] [RESTART] [0x43 R] [ACK] ... [NACK] [STOP]
        (addr=0x21, write reg ptr=0x00, then read 8 bytes)
```

### Write Command (Open Valve)
```
Master: [START] [0x42 W] [0x01] [0x01] [STOP]
        (addr=0x21, write reg ptr=0x01, data=0x01=OPEN)
```

### Read Position Only
```
Master: [START] [0x42 W] [0x02] [RESTART] [0x43 R] [NACK] [STOP]
        (addr=0x21, set reg ptr=0x02, read 1 byte)
```

### Change Device ID
```
Master: [START] [0x42 W] [0x00] [0x05] [STOP]
        (addr=0x21, write reg 0x00=DEV_ID, new ID=5)
        Note: After this, device responds at 0x25 (0x20+5)
```

## Arduino Master Example

```cpp
#include <Wire.h>

#define VALVE1_ADDR  0x21   // Device ID 1
#define REG_CMD      0x01
#define REG_POS      0x02
#define REG_FLAGS    0x03
#define CMD_OPEN     0x01
#define CMD_CLOSE    0x02
#define CMD_CAL      0x06

void setup() {
    Wire.begin();
    Serial.begin(9600);

    // Calibrate valve
    writeCmd(VALVE1_ADDR, CMD_CAL);
    delay(60000);  // Wait for calibration

    // Open valve
    writeCmd(VALVE1_ADDR, CMD_OPEN);
    delay(35000);

    // Read position
    uint8_t pos = readReg(VALVE1_ADDR, REG_POS);
    Serial.print("Position: ");
    Serial.println(pos);
}

void loop() {
    // Read all registers
    Wire.beginTransmission(VALVE1_ADDR);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(VALVE1_ADDR, 8);

    if (Wire.available() >= 8) {
        uint8_t id    = Wire.read();
        Wire.read(); // skip CMD
        uint8_t pos   = Wire.read();
        uint8_t flags = Wire.read();
        uint8_t err   = Wire.read();
        uint16_t trvl = (Wire.read() << 8) | Wire.read();
        uint8_t fw    = Wire.read();

        Serial.print("ID:"); Serial.print(id);
        Serial.print(" Pos:"); Serial.print(pos);
        Serial.print(" Cal:"); Serial.print(flags & 0x01);
        Serial.print(" Err:"); Serial.println(err);
    }
    delay(5000);
}

void writeCmd(uint8_t addr, uint8_t cmd) {
    Wire.beginTransmission(addr);
    Wire.write(REG_CMD);
    Wire.write(cmd);
    Wire.endTransmission();
}

uint8_t readReg(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}
```

## Wiring

```
Master (Arduino)          Valve Controller (STM8)
─────────────────         ─────────────────────
SDA (A4) ────────────┬──── PB4 (SDA) Pin 12
SCL (A5) ────────────┼──── PB5 (SCL) Pin 11
GND      ────────────┼──── VSS       Pin 4
                     │
              4.7k pull-up to 3.3V on SDA and SCL
```

## Important Notes

1. **Pull-up resistors:** 4.7k to 3.3V on both SDA and SCL lines
2. **Bus speed:** Standard mode (100kHz) only
3. **Clock stretching:** Enabled (controller may hold SCL low during WDT feed)
4. **Max devices:** 254 (IDs 1-254, address range 0x21-0x3E)
5. **Command execution:** Commands are queued and executed in the main loop, not in the ISR
6. **I2C does NOT wake from sleep:** Use UART byte or button press to wake first
