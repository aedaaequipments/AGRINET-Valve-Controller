# AGRINET Valve Controller - API Reference

Complete command reference for the UART interface.

---

## Communication Settings

| Parameter | Value |
|-----------|-------|
| **Baud Rate** | 115200 |
| **Data Bits** | 8 |
| **Parity** | None |
| **Stop Bits** | 1 |
| **Flow Control** | None |
| **Line Ending** | CR (`\r`) or LF (`\n`) or CRLF (`\r\n`) |

---

## Response Format

### Success Response
```
OK
```

### Error Response
```
ERR:<error_code>
```

### Data Response
```
<data>
```

---

## Position Commands

### OPEN - Move to Fully Open

```
Command:  open
Aliases:  100
Response: ->OPEN
          OPENED
          OK
```

### CLOSE - Move to Fully Closed

```
Command:  close
Aliases:  0
Response: ->CLOSE
          CLOSED
          OK
```

### HALF - Move to 50%

```
Command:  half
Aliases:  50
Response: ->OPEN first    (if not already at 100%)
          ->50% (2615ms)  (calculated move time)
          Done
          OK
```

### QUARTER - Move to 25%

```
Command:  quarter
Aliases:  25
Response: ->OPEN first
          ->25% (3922ms)
          Done
          OK
```

### THIRDQUARTER - Move to 75%

```
Command:  thirdquarter
Aliases:  75
Response: ->OPEN first
          ->75% (1307ms)
          Done
          OK
```

---

## System Commands

### CAL - Calibration

Runs automatic calibration sequence.

```
Command:  cal
Aliases:  calb, calibrate
Response: Calibrating...
          Cal:->CLOSE
          Cal:->OPEN
          Cal:Timing
          Cal:OK T=5230ms
          OK
```

**Error Responses:**
- `ERR:CAL_FAIL` - Calibration failed (limits not reached)
- `ERR:CAL_RANGE` - Travel time out of valid range

---

### STATUS - System Status

Returns complete system status.

```
Command:  status
Aliases:  stat, ?
Response: P:50 C:Y T:5230 S:2 E:0 N:42
```

**Field Descriptions:**

| Field | Example | Description |
|-------|---------|-------------|
| P | `P:50` | Position (0-100 or `?` for unknown) |
| C | `C:Y` | Calibrated (`Y`=yes, `N`=no) |
| T | `T:5230` | Travel time in milliseconds |
| S | `S:2` | System state (see state codes) |
| E | `E:0` | Last error code |
| N | `N:42` | Cycle count |

**System State Codes:**

| Code | State |
|------|-------|
| 0 | BOOT |
| 1 | INIT |
| 2 | IDLE |
| 3 | CALIBRATING |
| 4 | MOVING_OPEN |
| 5 | MOVING_CLOSE |
| 6 | POSITIONING |
| 7 | HOLDING |
| 8 | SLEEP |
| 9 | HIBERNATE |
| 10 | FAULT |
| 11 | RECOVERING |

---

### VERSION - Firmware Version

```
Command:  version
Aliases:  ver, v
Response: FW:v4.2.0
          AGN-V-STM8
```

---

### POS - Position Only

```
Command:  pos
Response: 50      (position 0-100)
          or
          ?       (position unknown)
```

---

### HELP - Command List

```
Command:  help
Aliases:  h
Response: === COMMANDS ===
          open/close/half
          quarter/thirdquarter
          0/25/50/75/100
          cal status version
          save load clear
          sleep wake reset
```

---

## Storage Commands

### SAVE - Save to EEPROM

Saves current configuration to non-volatile memory.

```
Command:  save
Response: OK
```

**Saved Data:**
- Calibration status
- Travel duration
- Current position
- Cycle count
- Error count

---

### LOAD - Load from EEPROM

Loads configuration from non-volatile memory.

```
Command:  load
Response: OK
          or
          ERR:EEPROM  (if data invalid)
```

---

### CLEAR - Clear EEPROM

Erases all stored data.

```
Command:  clear
Response: EEPROM cleared
          OK
```

**Warning:** This erases calibration data. Recalibration required after clearing.

---

## Power Commands

### SLEEP - Enter Light Sleep

```
Command:  sleep
Response: Sleep...
```

**Wake Sources:**
- Any UART character
- Button press

---

### WAKE - Force Wake

```
Command:  wake
Response: Awake
          OK
```

---

### RESET - System Reset

Performs hardware reset.

```
Command:  reset
Aliases:  rst
Response: Resetting...
          (device reboots)
```

---

## Error Codes

| Code | Name | Description | Recovery |
|------|------|-------------|----------|
| `OK` | No error | Operation successful | - |
| `STALL` | Motor stall | Motor not moving | Check wiring, motor, power |
| `TIMEOUT` | Timeout | Operation took too long | Check mechanical binding |
| `LIMIT` | Limit failure | Limit switch not detected | Check switch wiring |
| `STUCK` | Limit stuck | Limit always triggered | Check for short circuit |
| `CAL_FAIL` | Calibration failed | Could not complete calibration | Check both limit switches |
| `CAL_RANGE` | Range error | Travel time invalid (500ms-30s) | Check motor speed |
| `POS_UNK` | Position unknown | Position lost | Run calibration |
| `EEPROM` | EEPROM error | Storage read/write failed | Clear and reconfigure |
| `BAD_CMD` | Invalid command | Unknown command | Type `help` |
| `WDT` | Watchdog reset | System was reset by watchdog | Check for firmware hang |

---

## Example Sessions

### First-Time Setup

```
# Device boots
================
AGRINET v4.2.0
AGN-V-STM8
================
Cal:NO (run 'cal')
Pos:UNKNOWN
Ready. Type 'help'
OK

# Run calibration
> cal
Calibrating...
Cal:->CLOSE
Cal:->OPEN
Cal:Timing
Cal:OK T=5230ms
OK

# Test positions
> open
->OPEN
OPENED
OK

> close
->CLOSE
CLOSED
OK

> half
->OPEN first
->50% (2615ms)
Done
OK

# Check status
> status
P:50 C:Y T:5230 S:2 E:0 N:3
```

### Error Recovery

```
# Motor stalled
> open
->OPEN
ERR:STALL

# Check status
> status
P:? C:Y T:5230 S:10 E:1 N:4

# System auto-recovers after 10 seconds
Recover...
OK

# Or manually clear
> wake
Awake
OK

# Re-home position
> close
->CLOSE
CLOSED
OK
```

### Integration Example (Python)

```python
import serial
import time

class AgrinetValve:
    def __init__(self, port='/dev/ttyUSB0', baud=115200):
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)  # Wait for boot
        self.flush()
    
    def flush(self):
        self.ser.flushInput()
    
    def command(self, cmd, timeout=30):
        self.flush()
        self.ser.write(f"{cmd}\n".encode())
        
        lines = []
        start = time.time()
        while time.time() - start < timeout:
            line = self.ser.readline().decode().strip()
            if line:
                lines.append(line)
                if line in ['OK', 'ERR:'] or line.startswith('ERR:'):
                    break
        
        return lines
    
    def open(self):
        return self.command('open')
    
    def close(self):
        return self.command('close')
    
    def position(self, percent):
        return self.command(str(percent))
    
    def calibrate(self):
        return self.command('cal', timeout=60)
    
    def status(self):
        lines = self.command('status')
        if lines:
            # Parse: P:50 C:Y T:5230 S:2 E:0 N:42
            parts = lines[0].split()
            return {
                'position': parts[0].split(':')[1],
                'calibrated': parts[1].split(':')[1] == 'Y',
                'travel_ms': int(parts[2].split(':')[1]),
                'state': int(parts[3].split(':')[1]),
                'error': int(parts[4].split(':')[1]),
                'cycles': int(parts[5].split(':')[1])
            }
        return None

# Usage
valve = AgrinetValve('/dev/ttyUSB0')
valve.calibrate()
valve.open()
valve.position(50)
print(valve.status())
```
