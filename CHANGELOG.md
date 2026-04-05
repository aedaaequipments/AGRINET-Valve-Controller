# Changelog

All notable changes to the AGRINET Smart Valve Controller project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [4.4.0] - 2026-03-21

### Added
- **I2C Slave Mode** on PB4(SDA)/PB5(SCL) for multi-valve bus deployments
- **Device ID** (1-254) stored in EEPROM for unique bus addressing
- **UART Addressing** with `@<id>:command` format for multi-device UART bus
- **4 Build Profiles**: I2C, UART-addressed, legacy single-valve, debug
- **Shared command dispatch** (`cmdDo`) used by both UART and I2C paths
- **I2C Register Map**: 8 registers (device ID, command, position, flags, error, travel, FW version)
- **devid command** to read/set device ID via UART
- Professional documentation: I2C protocol, multi-device guide, v4.5 roadmap

### Changed
- **LED pin moved** from PB5 to PA3 (frees PB5 for I2C SCL)
- **Aggressive flash optimization** - freed ~600 bytes through:
  - Merged `btnHandleUp`/`btnHandleDown` into single `btnHandleDir`
  - Hash-based command parser (replaces strcmp chain)
  - Shortened error/status strings
  - XOR checksum instead of CRC-8
  - Compact variable naming
- Command buffer reduced from 40 to 24 bytes
- Error output format: `ECMD` instead of `ERR:CMD`

### Fixed
- Non-printable UART characters now filtered (noise immunity)
- SDCC C89 variable scope issues resolved

---

## [4.2.0] - 2026-03-20

### 🚨 Critical Fixes
- **EEPROM**: Fixed silent write failures - proper DUKR unlock sequence (0xAE, 0x56) now implemented
- **EEPROM**: Added EOP (End Of Programming) bit polling instead of fixed delay
- **Watchdog**: Implemented real STM8 IWDG with register-level access (was empty stubs)

### 🔧 High Priority Fixes
- **Reset**: Changed from AVR null-pointer trick to proper STM8 WWDG reset
- **Hibernate**: Added watchdog feeding during hibernate (every 500ms within 1.7s window)

### ✨ Improvements
- **EEPROM**: Added version validation in `eeLoad()` with migration support
- **UI**: Position `-1` now displays as `?` instead of raw number
- **Motor**: `motBrake()` no longer permanently disables motor
- **Startup**: Reports reset reason (IWDG, WWDG, or software reset)

### 📝 Documentation
- Added complete module documentation
- Added architecture diagrams
- Added troubleshooting guide

---

## [4.1.0] - 2026-03-20

### 🔧 Fixes
- Removed AVR-specific `noInterrupts()` / `interrupts()` calls
- Added STM8-compatible interrupt handling notes

### ✨ Features
- Complete rewrite with state machine architecture
- Added 17 UART commands with aliases
- Added LED status patterns
- Added auto-recovery from faults

### ⚠️ Known Issues
- EEPROM writes silently fail (fixed in 4.2.0)
- Watchdog not functional (fixed in 4.2.0)

---

## [4.0.0] - 2026-03-20

### ✨ Major Rewrite
- Complete firmware rewrite for production use
- Added fail-safe motor control
- Added position memory via EEPROM
- Added hardware watchdog support
- Added power management modes

### 🆕 New Features
- Automatic calibration sequence
- Multiple command aliases
- Cycle count tracking
- Error code reporting

---

## [3.0.0] - 2026-03-19

### ✨ Features
- Initial sduino port from original Arduino code
- 5-position valve control
- Basic UART command interface
- Manual button control

### 📝 Notes
- Based on original `stm8-gv-clib-opens-first_latestv3.ino`
- Adapted for STM8S003F3P6

---

## [2.0.0] - 2026-03-01

### Legacy Version
- Original Arduino-based firmware
- ATmega328P target

---

## Version History Summary

| Version | Date | Status | Description |
|---------|------|--------|-------------|
| 4.2.0 | 2026-03-20 | ✅ Current | Production-ready with all fixes |
| 4.1.0 | 2026-03-20 | ⚠️ Deprecated | EEPROM/WDT broken |
| 4.0.0 | 2026-03-20 | ⚠️ Deprecated | Initial rewrite |
| 3.0.0 | 2026-03-19 | ❌ Legacy | First STM8 port |
| 2.0.0 | 2026-03-01 | ❌ Legacy | Arduino original |

---

## Upgrade Notes

### Upgrading to 4.2.0

1. **EEPROM Format Changed**: Run `clear` command after upgrade to reset EEPROM
2. **Calibration Required**: Run `cal` command to re-calibrate after upgrade
3. **Watchdog Active**: System now has real hardware watchdog protection

```bash
# After flashing v4.2.0:
> clear
> cal
> status
```
