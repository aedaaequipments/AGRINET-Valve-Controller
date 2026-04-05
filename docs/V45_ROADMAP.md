# AGRINET Valve Controller - v4.5 Roadmap

## DO NOT FORGET - Next Iteration Features

These items were identified during v4.4 development and MUST be addressed
in v4.5 or later versions.

### Priority 1: Critical for Production

- [ ] **I2C Wake from Sleep** - Currently I2C address match does NOT wake the
  STM8S003 from HALT mode. Need GPIO interrupt on SDA (PB4) as external
  wake-up source. Pin change interrupt → wake → re-init I2C.

- [ ] **Device Label String** - Removed from v4.4 due to flash constraint.
  Add 8-char label (e.g. "VALVE01") stored in EEPROM 0x10-0x17 for
  app display. Need ~200 bytes flash space. Consider removing help command
  or other features to make room.

- [ ] **UART→I2C Bridge Mode** - Master sends UART command, slave forwards
  to I2C bus. Enables cheap Arduino Nano as bus master with serial interface
  to main controller/phone app.

### Priority 2: Enhancements

- [ ] **Current Sensing for True Stall Detection** - Current implementation
  is time-based heuristic. Add ACS712 current sensor on motor supply.
  ADC on PA3 or PC3. Detect actual motor stall by current spike.

- [ ] **10-bit Position Resolution** - Current 0-100% (8-bit). Add fine
  positioning with 0.1% steps. Requires I2C register expansion (2 bytes
  for position). Useful for precision drip irrigation.

- [ ] **Multi-Byte I2C Registers** - Expand register map to include:
  - Cycle count (4 bytes)
  - Device label (8 bytes)
  - Custom position presets (4x 1-byte)
  - Temperature reading (if sensor added)

- [ ] **OTA Configuration via I2C** - Write timing parameters (debounce,
  settle time, stall threshold) via I2C registers without recompiling.
  Store in EEPROM extended region (0x18-0x7F).

### Priority 3: Nice to Have

- [ ] **Bus Error Recovery** - Add timeout-based I2C bus recovery.
  If SDA stuck low, toggle SCL 9 times to free bus. Auto-detect and
  recover from bus lockup.

- [ ] **Heartbeat Register** - Auto-incrementing counter in I2C register.
  Master polls to verify controller is alive (not just powered but
  actually running firmware).

- [ ] **Position Feedback Confirmation** - After time-based positioning,
  add brief reverse-and-check to verify actual position matches target.
  Compensate for motor slippage.

- [ ] **PWM Motor Speed Control** - Use Timer2 PWM on motor pins for
  variable speed. Slow approach near limit switches for smoother operation.
  Requires Timer2 configuration (~100 bytes code).

- [ ] **Multiple Valve Types** - Current firmware is gate-valve specific.
  Add build profiles for solenoid valves (on/off only, no positioning)
  and ball valves (different limit switch behavior).

### Flash Budget Planning for v4.5

Current v4.4 flash usage:
- I2C build: 8054/8192 (138 bytes free)
- Legacy:    8162/8192 (30 bytes free)

To add features, must consider:
- **Moving to STM8S003K3** (32-pin, 8KB flash, more pins) - same flash
- **Moving to STM8S005C6** (48-pin, 32KB flash) - plenty of room
- **Splitting into bootloader + app** - requires STM8S with larger flash
- **Assembly optimization** of critical loops - could free 200-400 bytes
- **Removing features** from specific build profiles

### Hardware Changes for v4.5

- [ ] Consider adding EEPROM IC (24C02, 256 bytes) on I2C bus for
  expanded storage. Device label, custom presets, event log.
- [ ] Add test points for I2C SDA/SCL on PCB
- [ ] Add bus connector (4-pin JST-PH) for daisy-chain wiring
- [ ] Consider adding status RGB LED (WS2812B) instead of single LED
  (would require ~300 bytes for protocol driver)
