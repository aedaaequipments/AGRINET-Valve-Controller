# AGRINET Valve Controller - Troubleshooting Guide

Solutions for common problems and debugging tips.

---

## Quick Diagnostics

### LED Status Patterns

| Pattern | Meaning | Action |
|---------|---------|--------|
| Slow blink (1Hz) | Idle, ready | Normal operation |
| Fast blink (4Hz) | Motor running | Wait for completion |
| Solid ON | At position | Normal, temporary |
| Error blink | Fault state | Check error code |
| OFF | Sleep/Hibernate | Press button to wake |
| No LED at all | No power / not booting | Check power supply |

### Quick Status Check

```
> status
P:50 C:Y T:5230 S:2 E:0 N:42
```

| Field | Meaning | Normal Value |
|-------|---------|--------------|
| P | Position | 0-100 or ? |
| C | Calibrated | Y |
| T | Travel time | 500-30000 |
| S | State | 2 (IDLE) |
| E | Error | 0 |
| N | Cycles | Any number |

---

## Error Codes and Solutions

### ERR:STALL - Motor Stall

**Symptoms:**
- Motor doesn't move
- No sound from motor

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| No power to motor | Check 12V supply to L293D VCC2 |
| Motor wiring | Verify OUT1/OUT2 connections |
| L293D faulty | Test with direct 12V on motor |
| Mechanical jam | Check valve for obstructions |
| Motor burned out | Test motor with external power |

**Debug Steps:**
```bash
# 1. Enter hardware test mode (upload Hardware_Test.ino)
# 2. Press '1' to test motor close
# 3. Press '2' to test motor open
# 4. If motor doesn't run, check wiring
```

---

### ERR:TIMEOUT - Movement Timeout

**Symptoms:**
- Motor runs but never reaches limit
- Takes more than 30 seconds

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| Limit switch not wired | Check limit switch connections |
| Wrong switch type (NO instead of NC) | Use NC switches |
| Switch mounted wrong | Verify switch triggers at limit |
| Motor too slow | Use faster motor or increase timeout |
| Mechanical binding | Check valve mechanism |

**Debug Steps:**
```bash
# 1. Upload Hardware_Test.ino
# 2. Press 's' to see switch status
# 3. Manually trigger each limit switch
# 4. Verify PC5 goes HIGH when CLOSED triggered
# 5. Verify PC4 goes HIGH when OPEN triggered
```

---

### ERR:LIMIT - Limit Switch Failure

**Symptoms:**
- Calibration fails
- Position never confirmed

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| Switch wiring wrong | Verify NC contact to GND |
| No pull-up | Internal pull-up should be enabled |
| Switch stuck | Clean or replace switch |
| Wire broken | Check continuity |

**Expected Readings:**

| Position | PC5 (CLOSED) | PC4 (OPEN) |
|----------|--------------|------------|
| Fully closed | HIGH | LOW |
| Fully open | LOW | HIGH |
| In between | LOW | LOW |

---

### ERR:CAL_FAIL - Calibration Failed

**Symptoms:**
- Calibration doesn't complete
- Multiple retries fail

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| Limits not reached | Check motor and switches |
| Travel too short | Check if motor moves full range |
| Travel too long | Motor too slow or limits wrong |

**Calibration Requirements:**
- Must reach both limits
- Travel time: 500ms - 30,000ms
- Both switches must trigger correctly

---

### ERR:CAL_RANGE - Travel Time Invalid

**Symptoms:**
- Calibration completes but reports error
- Time too short or too long

**Solutions:**

| Problem | Solution |
|---------|----------|
| Time < 500ms | Motor too fast, add gear reduction |
| Time > 30,000ms | Motor too slow, check power |

---

### ERR:EEPROM - Storage Error

**Symptoms:**
- Settings don't persist
- CRC failure on load

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| EEPROM corrupted | Run `clear` command |
| Version mismatch | Clear and recalibrate |
| Hardware failure | Rare, may need MCU replacement |

**Recovery:**
```
> clear
EEPROM cleared
OK

> cal
(recalibrate)
```

---

### ERR:BAD_CMD - Invalid Command

**Symptoms:**
- Unknown command error

**Solutions:**
- Type `help` for command list
- Check spelling
- Commands are case-insensitive

---

## Common Problems

### Motor Runs Wrong Direction

**Fix:**
Swap motor wires at L293D OUT1/OUT2, OR swap PD2/PD3 in firmware:

```c
// In firmware, swap these:
#define PIN_MOTOR_IN1   PD3  // Was PD2
#define PIN_MOTOR_IN2   PD2  // Was PD3
```

---

### No Serial Communication

**Checklist:**
1. ✅ Baud rate = 115200
2. ✅ TX connects to RX (crossover)
3. ✅ RX connects to TX (crossover)
4. ✅ Common GND
5. ✅ Line ending CR or LF

**Test:**
```
# Send any character - LED should blink
# Send "help" + Enter
```

---

### Device Keeps Resetting

**Symptoms:**
- Startup message repeats
- See "RST:IWDG" message

**Causes & Solutions:**

| Cause | Solution |
|-------|----------|
| Watchdog timeout | Code stuck in loop without wdtFeed() |
| Power brownout | Improve power supply filtering |
| Motor noise | Add decoupling capacitors |

**Debug:**
1. Disable watchdog: `#define FEATURE_WATCHDOG 0`
2. Recompile and test
3. Find where code hangs

---

### Position Always Unknown (?)

**Causes:**
- Never calibrated
- Buttons used (loses known position)
- Power lost without save

**Solution:**
```
> cal
(calibrate)

> save
(save after positioning)
```

---

### Intermediate Positions Inaccurate

**Causes:**
- Poor calibration
- Motor speed varies with load
- Mechanical backlash

**Solutions:**
1. Re-run calibration: `cal`
2. Test with no load first
3. Accept ±5% tolerance for time-based positioning
4. Consider encoder feedback (hardware modification)

---

### Device Won't Wake from Sleep

**Check:**
1. Press button firmly (not just tap)
2. Send serial character
3. If truly stuck, power cycle

**Note:** IWDG will reset device if stuck >1.7 seconds

---

## Hardware Test Mode

Upload `examples/Hardware_Test/Hardware_Test.ino` for diagnostics:

### Test Commands

| Key | Action |
|-----|--------|
| `1` | Motor CLOSE (IN1=HIGH, IN2=LOW) |
| `2` | Motor OPEN (IN1=LOW, IN2=HIGH) |
| `0` | Motor STOP |
| `s` | Show all input status |
| `l` | Toggle LED |
| `t` | LED blink test |

### Expected Output

```
========================================
   AGRINET Hardware Test Mode v1.0
========================================

Commands:
  1 = Motor CLOSE (IN1=HIGH, IN2=LOW)
  2 = Motor OPEN  (IN1=LOW, IN2=HIGH)
  0 = Motor STOP  (both LOW)
  s = Read all inputs
  l = Toggle LED
  t = Test LED blink

> s
--- INPUT STATUS ---
BTN_UP (PC7):       released
BTN_DOWN (PC6):     released
LIMIT_CLOSED (PC5): not at limit
LIMIT_OPEN (PC4):   not at limit
--------------------
```

---

## Debug Logging

Enable verbose output:

```c
#define FEATURE_VERBOSE 1
```

This adds state change and EEPROM operation logging.

---

## Factory Reset

Complete reset to defaults:

```
> clear
EEPROM cleared
OK

> reset
Resetting...
(device reboots)

> cal
(recalibrate)
```

---

## Getting Help

1. **Check this guide** first
2. **Run hardware test** to isolate issue
3. **Check GitHub Issues** for known problems
4. **Create new issue** with:
   - Firmware version (`version` command)
   - Full `status` output
   - Error messages
   - Steps to reproduce
