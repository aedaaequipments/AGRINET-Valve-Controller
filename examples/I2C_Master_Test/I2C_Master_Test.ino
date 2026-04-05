/**
 * AGRINET Valve Controller - I2C Master Test
 *
 * Upload this to an Arduino Uno/Nano to test the valve controller
 * via I2C bus. Connect SDA(A4), SCL(A5), GND.
 *
 * Commands via Serial Monitor (9600 baud):
 *   o = Open valve
 *   c = Close valve
 *   h = Half (50%)
 *   q = Quarter (25%)
 *   t = Three-quarter (75%)
 *   k = Calibrate
 *   s = Read status
 *   i = Change device ID
 */

#include <Wire.h>

#define VALVE_BASE_ADDR 0x20
uint8_t valveId = 1;
uint8_t valveAddr;

/* I2C Register Map */
#define REG_DEV_ID    0x00
#define REG_CMD       0x01
#define REG_POSITION  0x02
#define REG_FLAGS     0x03
#define REG_ERROR     0x04
#define REG_TRAVEL_H  0x05
#define REG_TRAVEL_L  0x06
#define REG_FW_VER    0x07

/* Commands */
#define CMD_STOP      0x00
#define CMD_OPEN      0x01
#define CMD_CLOSE     0x02
#define CMD_QUARTER   0x03
#define CMD_HALF      0x04
#define CMD_3QTR      0x05
#define CMD_CAL       0x06
#define CMD_SAVE      0x07

void setup() {
    Wire.begin();
    Serial.begin(9600);
    valveAddr = VALVE_BASE_ADDR + valveId;

    Serial.println(F("AGRINET I2C Master Test"));
    Serial.print(F("Valve ID: ")); Serial.println(valveId);
    Serial.print(F("I2C Addr: 0x")); Serial.println(valveAddr, HEX);
    Serial.println(F("Commands: o=open c=close h=half q=qtr t=3qtr k=cal s=status"));
    Serial.println();

    readAllRegs();
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'o': sendCmd(CMD_OPEN);    Serial.println(F("-> OPEN"));  break;
            case 'c': sendCmd(CMD_CLOSE);   Serial.println(F("-> CLOSE")); break;
            case 'h': sendCmd(CMD_HALF);    Serial.println(F("-> HALF"));  break;
            case 'q': sendCmd(CMD_QUARTER); Serial.println(F("-> QTR"));   break;
            case 't': sendCmd(CMD_3QTR);    Serial.println(F("-> 3QTR"));  break;
            case 'k': sendCmd(CMD_CAL);     Serial.println(F("-> CAL"));   break;
            case 's': readAllRegs();        break;
            case 'i':
                Serial.println(F("Enter new ID (1-254):"));
                while (!Serial.available()) delay(10);
                delay(50);
                {
                    int newId = Serial.parseInt();
                    if (newId > 0 && newId < 255) {
                        setDeviceId(newId);
                        valveId = newId;
                        valveAddr = VALVE_BASE_ADDR + valveId;
                        Serial.print(F("New addr: 0x")); Serial.println(valveAddr, HEX);
                    }
                }
                break;
        }
    }
}

void sendCmd(uint8_t cmd) {
    Wire.beginTransmission(valveAddr);
    Wire.write(REG_CMD);
    Wire.write(cmd);
    Wire.endTransmission();
}

void setDeviceId(uint8_t newId) {
    Wire.beginTransmission(valveAddr);
    Wire.write(REG_DEV_ID);
    Wire.write(newId);
    Wire.endTransmission();
}

uint8_t readReg(uint8_t reg) {
    Wire.beginTransmission(valveAddr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(valveAddr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void readAllRegs() {
    Wire.beginTransmission(valveAddr);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(valveAddr, (uint8_t)8);

    if (Wire.available() < 8) {
        Serial.println(F("ERROR: No I2C response!"));
        Serial.print(F("Check wiring to addr 0x")); Serial.println(valveAddr, HEX);
        return;
    }

    uint8_t id    = Wire.read();
    Wire.read(); /* CMD reg (skip) */
    uint8_t pos   = Wire.read();
    uint8_t flags = Wire.read();
    uint8_t err   = Wire.read();
    uint16_t trvl = (Wire.read() << 8) | Wire.read();
    uint8_t fw    = Wire.read();

    Serial.println(F("=== Valve Status ==="));
    Serial.print(F("Device ID:  ")); Serial.println(id);
    Serial.print(F("Position:   "));
    if (pos == 0xFF) Serial.println(F("UNKNOWN"));
    else { Serial.print(pos); Serial.println(F("%")); }
    Serial.print(F("Calibrated: ")); Serial.println(flags & 0x01 ? F("YES") : F("NO"));
    Serial.print(F("Moving:     ")); Serial.println(flags & 0x02 ? F("YES") : F("NO"));
    Serial.print(F("Fault:      ")); Serial.println(flags & 0x04 ? F("YES") : F("NO"));
    Serial.print(F("Sleeping:   ")); Serial.println(flags & 0x08 ? F("YES") : F("NO"));
    Serial.print(F("Error:      ")); Serial.println(err);
    Serial.print(F("Travel(ms): ")); Serial.println(trvl);
    Serial.print(F("FW Version: ")); Serial.print(fw >> 4); Serial.print('.'); Serial.println(fw & 0x0F);
    Serial.println();
}
