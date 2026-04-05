# AGRINET Valve Controller - Module Documentation

This document provides detailed descriptions of each firmware module.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [GPIO Module](#gpio-module)
3. [Motor Control Module](#motor-control-module)
4. [Calibration Module](#calibration-module)
5. [EEPROM Module](#eeprom-module)
6. [UART Communication Module](#uart-communication-module)
7. [Button Handler Module](#button-handler-module)
8. [Power Management Module](#power-management-module)
9. [Watchdog Module](#watchdog-module)
10. [LED Status Module](#led-status-module)
11. [Error Handling Module](#error-handling-module)

---

## System Overview

### State Machine

The firmware operates as a finite state machine with the following states:

```
┌─────────────────────────────────────────────────────────────────┐
│                        STATE DIAGRAM                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                          ┌──────┐                               │
│                          │ BOOT │                               │
│                          └───┬──┘                               │
│                              │                                  │
│                          ┌───▼──┐                               │
│                          │ INIT │                               │
│                          └───┬──┘                               │
│                              │                                  │
│         ┌────────────────────▼────────────────────┐             │
│         │                  IDLE                    │◄────┐      │
│         └─────┬────────┬────────┬────────┬────────┘     │      │
│               │        │        │        │              │      │
│           ┌───▼───┐┌───▼───┐┌───▼───┐┌───▼───┐         │      │
│           │MOVING ││MOVING ││POSITN ││ CALIB │         │      │
│           │ OPEN  ││ CLOSE ││  ING  ││RATING │         │      │
│           └───┬───┘└───┬───┘└───┬───┘└───┬───┘         │      │
│               │        │        │        │              │      │
│               └────────┴────────┴────────┘              │      │
│                              │                          │      │
│                          ┌───▼───┐                      │      │
│                          │HOLDING│──────────────────────┘      │
│                          └───────┘                             │
│                                                                 │
│         ┌──────────────────────────────────────────┐           │
│         │              SLEEP / HIBERNATE            │           │
│         └─────────────────────┬────────────────────┘           │
│                               │                                 │
│                          ┌────▼────┐                            │
│                          │  FAULT  │◄──── Any error             │
│                          └────┬────┘                            │
│                               │ Auto-recover                    │
│                          ┌────▼─────┐                           │
│                          │RECOVERING│                           │
│                          └──────────┘                           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### State Definitions

| State | Value | Description |
|-------|-------|-------------|
| `SYS_STATE_BOOT` | 0 | Initial boot sequence |
| `SYS_STATE_INIT` | 1 | Initializing subsystems |
| `SYS_STATE_IDLE` | 2 | Ready for commands |
| `SYS_STATE_CALIBRATING` | 3 | Running calibration |
| `SYS_STATE_MOVING_OPEN` | 4 | Moving toward open |
| `SYS_STATE_MOVING_CLOSE` | 5 | Moving toward close |
| `SYS_STATE_POSITIONING` | 6 | Moving to intermediate |
| `SYS_STATE_HOLDING` | 7 | At target position |
| `SYS_STATE_SLEEP` | 8 | Light sleep mode |
| `SYS_STATE_HIBERNATE` | 9 | Deep hibernation |
| `SYS_STATE_FAULT` | 10 | Error condition |
| `SYS_STATE_RECOVERING` | 11 | Recovery in progress |

---

## GPIO Module

### Purpose
Handles all General Purpose Input/Output pin configurations and readings.

### Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `gpioInit()` | Initialize all GPIO pins | void |
| `gpioReadLimitClosed()` | Read closed limit switch | bool (true = at limit) |
| `gpioReadLimitOpen()` | Read open limit switch | bool (true = at limit) |
| `gpioReadBtnUp()` | Read UP button | bool (true = pressed) |
| `gpioReadBtnDown()` | Read DOWN button | bool (true = pressed) |

### Pin Configuration

```c
/* Motor outputs - Push-Pull, initially LOW */
pinMode(PIN_MOTOR_IN1, OUTPUT);  // PD2
pinMode(PIN_MOTOR_IN2, OUTPUT);  // PD3

/* Button inputs - Input with internal pull-up */
pinMode(PIN_BTN_UP, INPUT_PULLUP);    // PC7
pinMode(PIN_BTN_DOWN, INPUT_PULLUP);  // PC6

/* Limit switch inputs - Input with internal pull-up */
pinMode(PIN_LIMIT_CLOSED, INPUT_PULLUP);  // PC5
pinMode(PIN_LIMIT_OPEN, INPUT_PULLUP);    // PC4

/* LED output */
pinMode(PIN_LED, OUTPUT);  // PB5
```

### Limit Switch Logic

The system uses **Normally Closed (NC)** limit switches for fail-safe operation:

| Condition | Switch State | Pin Reading | Interpretation |
|-----------|--------------|-------------|----------------|
| Not at limit | Closed (NC) | LOW | Motor can move |
| At limit | Open | HIGH | Motor at limit |
| Wire broken | Open | HIGH | Treated as limit (safe) |

---

## Motor Control Module

### Purpose
Controls the DC motor via L293D H-bridge driver.

### Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `motInit()` | Initialize motor subsystem | void |
| `motSetDir(dir)` | Set motor direction | void |
| `motStop()` | Stop motor (coast) | void |
| `motBrake()` | Stop motor (hard brake) | void |
| `motEnable()` | Re-enable motor after fault | void |
| `motMoveToLimit(dir, timeout)` | Move until limit reached | bool |
| `motMoveForTime(dir, duration)` | Move for specified time | bool |
| `motMoveToPos(percent)` | Move to percentage position | bool |

### Direction Control

| Direction | IN1 (PD2) | IN2 (PD3) | Motor Action |
|-----------|-----------|-----------|--------------|
| `MOT_STOP` | LOW | LOW | Coast stop |
| `MOT_OPEN` | LOW | HIGH | Open valve |
| `MOT_CLOSE` | HIGH | LOW | Close valve |
| Brake | HIGH | HIGH | Hard stop (brief) |

### Safety Features

1. **Timeout Protection**: Motor stops after `MOTOR_MAX_TRAVEL_MS` (30 seconds)
2. **Stall Detection**: If no progress detected after 5 seconds
3. **Limit Verification**: Debounce and retry on limit detection
4. **Emergency Stop**: Both outputs LOW on any error

### Position Algorithm

For intermediate positions (25%, 50%, 75%), the system:

1. Moves to OPEN (100%) first for reference
2. Calculates close time: `moveTime = (100 - targetPercent) * travelTime / 100`
3. Closes for calculated duration
4. Position accuracy depends on calibration

---

## Calibration Module

### Purpose
Measures the full valve travel time for accurate positioning.

### Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `calRun()` | Run full calibration sequence | bool |
| `calValidate(duration)` | Validate measured duration | bool |

### Calibration Sequence

```
1. Move to CLOSED limit
   └─► Verify limit switch triggered
   
2. Move to OPEN limit  
   └─► Verify limit switch triggered
   
3. Start timer
   └─► Move from OPEN to CLOSED
   └─► Stop timer when CLOSED limit triggered
   
4. Validate timing
   └─► Must be between 500ms and 30000ms
   
5. Save to EEPROM
   └─► Store travel duration
   └─► Set calibration flag
```

### Validation Criteria

| Check | Range | Error Code |
|-------|-------|------------|
| Minimum travel | ≥ 500 ms | `ERR_CAL_RANGE` |
| Maximum travel | ≤ 30000 ms | `ERR_CAL_RANGE` |
| Limit detected | Must trigger | `ERR_CAL_FAIL` |

---

## EEPROM Module

### Purpose
Provides persistent storage for configuration and position data.

### Memory Map (STM8S003F3 - 128 bytes @ 0x4000)

| Address | Size | Name | Description |
|---------|------|------|-------------|
| 0x00 | 1 | MAGIC_H | Magic number high byte |
| 0x01 | 1 | MAGIC_L | Magic number low byte |
| 0x02 | 1 | VERSION | Config format version |
| 0x03 | 1 | FLAGS | Status flags |
| 0x04-0x07 | 4 | TRAVEL | Travel duration (ms) |
| 0x08 | 1 | POSITION | Last position (0-100) |
| 0x09-0x0C | 4 | CYCLES | Cycle count |
| 0x0D | 1 | ERRORS | Error count |
| 0x0E | 1 | CHECKSUM | Data checksum |
| 0x0F-0x7F | 113 | Reserved | Future use |

### Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `eeInit()` | Initialize EEPROM | void |
| `eeUnlock()` | Unlock EEPROM for writing | bool |
| `eeLock()` | Lock EEPROM after writing | void |
| `eeWriteByte(addr, data)` | Write single byte | void |
| `eeReadByte(addr)` | Read single byte | uint8_t |
| `eeWriteU32(addr, data)` | Write 32-bit value | void |
| `eeReadU32(addr)` | Read 32-bit value | uint32_t |
| `eeSave()` | Save all configuration | bool |
| `eeLoad()` | Load all configuration | bool |
| `eeClear()` | Erase all data | void |
| `eeCalcCRC()` | Calculate checksum | uint8_t |

### STM8 EEPROM Unlock Sequence

```c
/* CRITICAL: Must use exact unlock keys */
FLASH_DUKR = 0xAE;  /* First key */
FLASH_DUKR = 0x56;  /* Second key */

/* Wait for unlock bit */
while (!(FLASH_IAPSR & FLASH_IAPSR_DUL)) { }

/* Write data */
*ptr = data;

/* Wait for completion */
while (!(FLASH_IAPSR & FLASH_IAPSR_EOP)) { }

/* Re-lock */
FLASH_IAPSR &= ~FLASH_IAPSR_DUL;
```

---

## UART Communication Module

### Purpose
Handles serial communication with master controller.

### Configuration

| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Line Ending | CR or LF |

### Functions

| Function | Description |
|----------|-------------|
| `uartInit()` | Initialize UART |
| `uartProcess()` | Process incoming data |
| `uartSend(str)` | Send string with CRLF |
| `uartSendNum(num)` | Send number |
| `uartSendOK()` | Send "OK" response |
| `uartSendErr(err)` | Send error response |
| `uartParseCmd(str)` | Parse command string |
| `uartExecCmd(str)` | Execute parsed command |

### Command Protocol

```
Request:  <command>\n
Response: OK\n
          or
          ERR:<reason>\n
          or
          <data>\n
```

### Command List

| Command | Aliases | Action |
|---------|---------|--------|
| `open` | `100` | Move to 100% |
| `close` | `0` | Move to 0% |
| `half` | `50` | Move to 50% |
| `quarter` | `25` | Move to 25% |
| `thirdquarter` | `75` | Move to 75% |
| `cal` | `calb`, `calibrate` | Run calibration |
| `status` | `stat`, `?` | Show status |
| `version` | `ver`, `v` | Show version |
| `pos` | - | Show position |
| `help` | `h` | Show commands |
| `save` | - | Save to EEPROM |
| `load` | - | Load from EEPROM |
| `clear` | - | Clear EEPROM |
| `sleep` | - | Enter sleep |
| `wake` | - | Force wake |
| `reset` | `rst` | System reset |

---

## Button Handler Module

### Purpose
Handles physical button presses for manual control.

### Functions

| Function | Description |
|----------|-------------|
| `btnCheck()` | Poll and debounce buttons |
| `btnHandleUp()` | Handle UP button press |
| `btnHandleDown()` | Handle DOWN button press |

### Button Behavior

| Button | Direction | Action |
|--------|-----------|--------|
| UP | Close | Move toward CLOSED while held |
| DOWN | Open | Move toward OPEN while held |

### Debounce

- Check interval: 50 ms
- Edge detection: Falling edge (button press)
- Limit check: Prevents movement beyond limits

---

## Power Management Module

### Purpose
Manages power consumption through sleep modes.

### Sleep Modes

| Mode | Current | Wake Sources | Timeout |
|------|---------|--------------|---------|
| Active | ~15 mA | - | - |
| Idle | ~5 mA | - | - |
| Sleep | ~350 µA | UART, Buttons | 30 sec |
| Hibernate | <1 µA | Buttons | 5 min |

### Functions

| Function | Description |
|----------|-------------|
| `pwrCheckIdle()` | Check for idle timeout |
| `pwrSleep()` | Enter light sleep |
| `pwrHibernate()` | Enter deep hibernate |
| `pwrWake()` | Wake from sleep |

### Power State Flow

```
ACTIVE ──30s idle──► SLEEP ──5min──► HIBERNATE
   ▲                    │                 │
   │                    │                 │
   └────────────────────┴─────────────────┘
         Any activity (UART, buttons)
```

---

## Watchdog Module

### Purpose
Hardware watchdog timer for fail-safe operation.

### Configuration

| Parameter | Value |
|-----------|-------|
| Type | IWDG (Independent Watchdog) |
| Prescaler | /256 |
| Reload | 255 |
| Timeout | ~1.7 seconds |

### Functions

| Function | Description |
|----------|-------------|
| `wdtInit()` | Initialize and start IWDG |
| `wdtFeed()` | Refresh watchdog counter |
| `wdtDisable()` | Disable watchdog (NOT POSSIBLE) |

### STM8 IWDG Implementation

```c
/* Enable IWDG */
IWDG_KR = 0xCC;

/* Unlock registers */
IWDG_KR = 0x55;

/* Configure */
IWDG_PR = 0x06;   /* Prescaler /256 */
IWDG_RLR = 0xFF;  /* Reload value */

/* Start */
IWDG_KR = 0xAA;
```

### Important Notes

1. **Cannot be disabled** once started (by design for safety)
2. **Must feed within timeout** or system resets
3. **System reset for intentional reboot** uses WWDG instead

---

## LED Status Module

### Purpose
Provides visual status indication.

### LED Patterns

| Pattern | Frequency | Meaning |
|---------|-----------|---------|
| `LED_OFF` | - | Sleep/Hibernate |
| `LED_ON` | Solid | At target position |
| `LED_SLOW` | 1 Hz | Idle, ready |
| `LED_FAST` | 4 Hz | Motor running |
| `LED_SOS` | Variable | Fault condition |

### Functions

| Function | Description |
|----------|-------------|
| `ledInit()` | Initialize LED GPIO |
| `ledSet(mode)` | Set LED pattern |
| `ledUpdate()` | Update LED state (call in loop) |

---

## Error Handling Module

### Purpose
Manages error conditions and recovery.

### Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | `ERR_OK` | No error |
| 1 | `ERR_MOTOR_STALL` | Motor not moving |
| 2 | `ERR_MOTOR_TIMEOUT` | Movement timeout |
| 3 | `ERR_LIMIT_FAIL` | Limit switch failure |
| 4 | `ERR_LIMIT_STUCK` | Limit always active |
| 5 | `ERR_CAL_FAIL` | Calibration failed |
| 6 | `ERR_CAL_RANGE` | Travel time invalid |
| 7 | `ERR_POS_UNKNOWN` | Position lost |
| 8 | `ERR_EEPROM` | EEPROM error |
| 9 | `ERR_BAD_CMD` | Invalid command |
| 10 | `ERR_WDT_RESET` | Watchdog reset |
| 11 | `ERR_INTERNAL` | Internal error |

### Functions

| Function | Description |
|----------|-------------|
| `errSet(err)` | Set error state |
| `errClear()` | Clear error state |
| `errRecover()` | Attempt recovery |

### Auto-Recovery

When `FEATURE_AUTO_RECOVER=1`:
- System attempts recovery after 10 seconds in FAULT state
- Motor is re-enabled
- Position marked as unknown
- Returns to IDLE state

---

## Memory Usage

### Typical Build (all features enabled)

| Section | Size | Notes |
|---------|------|-------|
| Flash | ~7.2 KB | Of 8 KB available |
| RAM | ~400 bytes | Of 1 KB available |
| EEPROM | ~16 bytes | Of 128 bytes available |

### Minimal Build (features disabled)

| Section | Size | Notes |
|---------|------|-------|
| Flash | ~4.5 KB | Sleep, LED, stall detect off |
| RAM | ~300 bytes | Reduced buffers |
