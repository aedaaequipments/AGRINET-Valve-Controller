<![CDATA[# AGRINET Smart Valve Controller

<p align="center">
  <img src="docs/images/agrinet-logo.png" alt="AGRINET Logo" width="200"/>
</p>

<p align="center">
  <a href="https://github.com/agrinet/valve-controller/releases"><img src="https://img.shields.io/badge/version-4.2.0-blue.svg" alt="Version"></a>
  <a href="https://github.com/agrinet/valve-controller/blob/main/LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-STM8S003F3P6-orange.svg" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/framework-sduino-purple.svg" alt="Framework"></a>
  <a href="#"><img src="https://img.shields.io/badge/build-passing-brightgreen.svg" alt="Build"></a>
</p>

<p align="center">
  <b>Production-grade, fail-proof motorized valve controller for smart irrigation systems</b>
</p>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Hardware Requirements](#-hardware-requirements)
- [Quick Start](#-quick-start)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Commands](#-commands)
- [API Reference](#-api-reference)
- [Architecture](#-architecture)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

The **AGRINET Smart Valve Controller** is an industrial-grade firmware for controlling motorized irrigation valves. It runs on the STM8S003F3P6 microcontroller and provides:

- **Proportional valve control** with 5 preset positions (0%, 25%, 50%, 75%, 100%)
- **Automatic calibration** for different valve types
- **Position memory** that survives power loss (EEPROM)
- **Hardware watchdog** for fail-safe operation
- **Ultra-low power modes** for battery-powered installations
- **Simple UART protocol** for integration with master controllers

### Use Cases

- 🌾 **Smart Agriculture** - Automated drip irrigation systems
- 🏡 **Home Automation** - Garden and lawn irrigation
- 🏭 **Industrial** - Process control valve automation
- 🌱 **Greenhouse** - Climate-controlled irrigation

---

## ✨ Features

### Core Features

| Feature | Description |
|---------|-------------|
| **5-Position Control** | Close (0%), Quarter (25%), Half (50%), Three-Quarter (75%), Open (100%) |
| **Auto-Calibration** | Learns valve travel time automatically |
| **Position Memory** | EEPROM storage survives power loss |
| **Manual Override** | Physical buttons for local control |
| **Status LED** | Visual indication of system state |

### Safety Features

| Feature | Description |
|---------|-------------|
| **Hardware Watchdog** | Auto-recovery from firmware hang (~1.7s timeout) |
| **Motor Protection** | Stall detection, timeout protection |
| **Limit Switch Safety** | NC switches for fail-safe operation |
| **Position Verification** | Always re-homes from known position |

### Power Management

| Mode | Current | Wake Source |
|------|---------|-------------|
| Active | ~15 mA | - |
| Idle | ~5 mA | - |
| Sleep | ~350 µA | UART, Buttons |
| Hibernate | <1 µA | Buttons |

---

## 🔧 Hardware Requirements

### Microcontroller

| Parameter | Value |
|-----------|-------|
| **MCU** | STM8S003F3P6 |
| **Package** | TSSOP20 |
| **Flash** | 8 KB |
| **RAM** | 1 KB |
| **EEPROM** | 128 bytes |
| **Clock** | 16 MHz (HSI) |

### Components

| Component | Specification | Quantity |
|-----------|---------------|----------|
| STM8S003F3P6 | TSSOP20 MCU | 1 |
| L293D | Motor Driver IC | 1 |
| DC Motor | 12V Gear Motor | 1 |
| Limit Switch | NC (Normally Closed) | 2 |
| Push Button | Momentary | 2 |
| LED | 3mm + 330Ω resistor | 1 |
| Capacitor | 100nF ceramic | 2 |
| Voltage Regulator | 3.3V LDO | 1 |

### Pin Mapping

```
┌─────────────────────────────────────────────────────────────┐
│                    STM8S003F3P6 TSSOP20                     │
├─────────────────────────────────────────────────────────────┤
│  Pin 19 (PD2) ────► Motor IN1 (L293D)                       │
│  Pin 20 (PD3) ────► Motor IN2 (L293D)                       │
│  Pin 17 (PC7) ◄──── Button UP (Active LOW)                  │
│  Pin 16 (PC6) ◄──── Button DOWN (Active LOW)                │
│  Pin 15 (PC5) ◄──── Limit Switch CLOSED (NC)                │
│  Pin 14 (PC4) ◄──── Limit Switch OPEN (NC)                  │
│  Pin 11 (PB5) ────► Status LED                              │
│  Pin 2  (PD5) ────► UART TX (115200 baud)                   │
│  Pin 3  (PD6) ◄──── UART RX                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/agrinet/valve-controller.git
cd valve-controller
```

### 2. Install Dependencies

```bash
# Install sduino board package (Arduino IDE)
# Add this URL to Board Manager:
# https://github.com/tenbaht/sduino/raw/master/package_sduino_stm8_index.json

# Install SDCC compiler
sudo apt install sdcc sdcc-libraries  # Ubuntu/Debian
brew install sdcc                      # macOS
```

### 3. Build and Upload

```bash
# Using Make
make compile
make upload

# Using PlatformIO
pio run -t upload

# Using Arduino CLI
arduino-cli compile --fqbn Sduino:stm8:stm8s003f3 src/
arduino-cli upload --fqbn Sduino:stm8:stm8s003f3 -p /dev/ttyUSB0 src/
```

### 4. First Run

```
# Connect via serial (115200 baud)
# Run calibration
> cal

# Test positions
> open
> close
> half
```

---

## 📦 Installation

### Option 1: Arduino IDE

1. **Install sduino board package:**
   - File → Preferences → Additional Board URLs
   - Add: `https://github.com/tenbaht/sduino/raw/master/package_sduino_stm8_index.json`
   - Tools → Board → Boards Manager → Search "sduino" → Install

2. **Open firmware:**
   - File → Open → `src/AGRINET_Valve_Controller.ino`

3. **Select board:**
   - Tools → Board → STM8S Boards → STM8S003F3P6

4. **Upload:**
   - Sketch → Upload

### Option 2: PlatformIO

```bash
# Install PlatformIO
pip install platformio

# Build
pio run

# Upload
pio run -t upload

# Monitor serial
pio device monitor
```

### Option 3: Command Line

```bash
# Setup (first time)
make setup

# Build
make compile

# Upload
make upload

# Serial monitor
make serial
```

---

## ⚙️ Configuration

### Compile-Time Options

Edit `src/config.h` or modify defines in main file:

```c
/* Feature Toggles (1=enabled, 0=disabled) */
#define FEATURE_WATCHDOG        1   // Hardware watchdog
#define FEATURE_EEPROM          1   // Persistent storage
#define FEATURE_SLEEP           1   // Power saving modes
#define FEATURE_LED             1   // Status LED
#define FEATURE_STALL_DETECT    1   // Motor stall detection
#define FEATURE_AUTO_RECOVER    1   // Auto fault recovery
#define FEATURE_VERBOSE         0   // Debug output

/* Timing Configuration */
#define MOTOR_MAX_TRAVEL_MS     30000   // Max motor run time
#define IDLE_TIMEOUT_MS         30000   // Sleep after 30s
#define HIBERNATE_TIMEOUT_MS    300000  // Hibernate after 5min
#define UART_BAUD               115200  // Serial baud rate
```

### Pin Configuration

```c
/* Change pin assignments if needed */
#define PIN_MOTOR_IN1       PD2
#define PIN_MOTOR_IN2       PD3
#define PIN_BTN_UP          PC7
#define PIN_BTN_DOWN        PC6
#define PIN_LIMIT_CLOSED    PC5
#define PIN_LIMIT_OPEN      PC4
#define PIN_LED             LED_BUILTIN
```

---

## 📟 Commands

### Position Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `open` | `100` | Move to fully open (100%) |
| `close` | `0` | Move to fully closed (0%) |
| `half` | `50` | Move to half open (50%) |
| `quarter` | `25` | Move to quarter open (25%) |
| `thirdquarter` | `75` | Move to three-quarter (75%) |

### System Commands

| Command | Aliases | Description |
|---------|---------|-------------|
| `cal` | `calb`, `calibrate` | Run calibration |
| `status` | `stat`, `?` | Show system status |
| `version` | `ver`, `v` | Show firmware version |
| `pos` | - | Show current position |
| `help` | `h` | Show command list |

### Storage Commands

| Command | Description |
|---------|-------------|
| `save` | Save config to EEPROM |
| `load` | Load config from EEPROM |
| `clear` | Erase EEPROM data |

### Power Commands

| Command | Description |
|---------|-------------|
| `sleep` | Enter light sleep |
| `wake` | Force wake |
| `reset` | System reset |

### Response Format

```
OK                          # Success
ERR:REASON                  # Error with reason
P:50 C:Y T:5230 S:2 E:0 N:42  # Status response
│   │   │     │   │   └─ Cycle count
│   │   │     │   └───── Last error
│   │   │     └───────── System state
│   │   └─────────────── Travel time (ms)
│   └─────────────────── Calibrated (Y/N)
└─────────────────────── Position (0-100 or ?)
```

---

## 📖 API Reference

See [docs/API.md](docs/API.md) for complete API documentation.

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     AGRINET ARCHITECTURE                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │    UART     │    │   BUTTONS   │    │   TIMERS    │     │
│  │  Interface  │    │   Handler   │    │   (millis)  │     │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘     │
│         │                  │                  │             │
│         └──────────────────┼──────────────────┘             │
│                            │                                │
│                   ┌────────▼────────┐                       │
│                   │  STATE MACHINE  │                       │
│                   │                 │                       │
│                   │  IDLE ──► MOVING│                       │
│                   │    ▲        │   │                       │
│                   │    └── HOLD ◄┘  │                       │
│                   └────────┬────────┘                       │
│                            │                                │
│         ┌──────────────────┼──────────────────┐             │
│         │                  │                  │             │
│  ┌──────▼──────┐    ┌──────▼──────┐    ┌──────▼──────┐     │
│  │   MOTOR     │    │   EEPROM    │    │  WATCHDOG   │     │
│  │  Control    │    │   Storage   │    │   (IWDG)    │     │
│  └──────┬──────┘    └─────────────┘    └─────────────┘     │
│         │                                                   │
│  ┌──────▼──────┐    ┌─────────────┐                        │
│  │   L293D     │    │   LIMIT     │                        │
│  │   Driver    │◄───│  SWITCHES   │                        │
│  └──────┬──────┘    └─────────────┘                        │
│         │                                                   │
│  ┌──────▼──────┐                                           │
│  │    VALVE    │                                           │
│  │    MOTOR    │                                           │
│  └─────────────┘                                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed architecture documentation.

---

## 📁 Project Structure

```
AGRINET-Valve-Controller/
├── src/
│   ├── AGRINET_Valve_Controller.ino    # Main firmware
│   └── config.h                         # Configuration
├── docs/
│   ├── ARCHITECTURE.md                  # System architecture
│   ├── API.md                          # Command API reference
│   ├── HARDWARE.md                     # Hardware documentation
│   ├── MODULES.md                      # Module descriptions
│   └── TROUBLESHOOTING.md              # Troubleshooting guide
├── examples/
│   ├── Hardware_Test/                  # Hardware verification
│   └── Basic_Control/                  # Basic usage example
├── hardware/
│   ├── SCHEMATIC.md                    # Circuit schematic
│   └── BOM.md                          # Bill of materials
├── tools/
│   ├── flash.sh                        # Flash utility
│   └── serial_test.py                  # Serial test script
├── .github/
│   └── workflows/
│       └── build.yml                   # CI/CD pipeline
├── platformio.ini                       # PlatformIO config
├── Makefile                            # Build automation
├── LICENSE                             # MIT License
├── CHANGELOG.md                        # Version history
├── CONTRIBUTING.md                     # Contribution guide
└── README.md                           # This file
```

---

## 🤝 Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup

```bash
# Clone repo
git clone https://github.com/agrinet/valve-controller.git
cd valve-controller

# Create feature branch
git checkout -b feature/your-feature

# Make changes and test
make compile
make upload

# Commit and push
git add .
git commit -m "Add: your feature description"
git push origin feature/your-feature

# Create pull request
```

---

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

---

## 🙏 Acknowledgments

- [sduino](https://github.com/tenbaht/sduino) - Arduino-like framework for STM8
- [SDCC](http://sdcc.sourceforge.net/) - Small Device C Compiler
- STMicroelectronics - STM8S Reference Manual

---

## 📞 Support

- 📧 Email: support@agrinet.com
- 🐛 Issues: [GitHub Issues](https://github.com/agrinet/valve-controller/issues)
- 📖 Wiki: [GitHub Wiki](https://github.com/agrinet/valve-controller/wiki)

---

<p align="center">
  Made with ❤️ for Smart Agriculture
</p>
]]>