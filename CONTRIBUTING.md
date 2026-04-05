# Contributing to AGRINET Valve Controller

Thank you for your interest in contributing! This guide will help you get started.

---

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [How to Contribute](#how-to-contribute)
3. [Development Setup](#development-setup)
4. [Coding Standards](#coding-standards)
5. [Testing](#testing)
6. [Pull Request Process](#pull-request-process)

---

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Keep discussions technical and professional

---

## How to Contribute

### Reporting Bugs

1. **Search existing issues** to avoid duplicates
2. **Create a new issue** with:
   - Clear title describing the bug
   - Firmware version (`version` command output)
   - Hardware setup description
   - Steps to reproduce
   - Expected vs actual behavior
   - Error messages or logs

### Suggesting Features

1. **Check existing issues** for similar requests
2. **Create a feature request** with:
   - Clear description of the feature
   - Use case / problem it solves
   - Proposed implementation (if any)

### Code Contributions

1. **Fork the repository**
2. **Create a feature branch**
3. **Make your changes**
4. **Test thoroughly**
5. **Submit a pull request**

---

## Development Setup

### Prerequisites

```bash
# Install SDCC compiler
sudo apt install sdcc sdcc-libraries   # Ubuntu/Debian
brew install sdcc                       # macOS

# Install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Install sduino board package
arduino-cli core install Sduino:stm8 \
  --additional-urls https://github.com/tenbaht/sduino/raw/master/package_sduino_stm8_index.json
```

### Clone and Build

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/valve-controller.git
cd valve-controller

# Create feature branch
git checkout -b feature/your-feature

# Build
make compile

# Upload (if testing on hardware)
make upload
```

---

## Coding Standards

### General Guidelines

1. **Use descriptive names** for functions and variables
2. **Comment complex logic** - explain "why", not just "what"
3. **Keep functions small** - single responsibility
4. **Avoid magic numbers** - use #define constants

### Naming Conventions

```c
/* Constants - UPPER_SNAKE_CASE */
#define MOTOR_MAX_TRAVEL_MS     30000UL

/* Types - PascalCase with _t suffix */
typedef enum {
    SYS_STATE_IDLE,
    SYS_STATE_MOVING
} SysState_t;

/* Global variables - g_ prefix, camelCase */
volatile uint32_t g_travelMs = 0;

/* Functions - camelCase, verb prefix */
void motStop(void);
bool calRun(void);
uint8_t eeReadByte(uint8_t addr);

/* Local variables - camelCase */
uint32_t startTime = millis();
```

### Code Structure

```c
/**
 * @brief Brief description of function
 * @param paramName Description of parameter
 * @return Description of return value
 * @note Any special notes
 */
bool functionName(uint8_t paramName) {
    /* Local variable declarations first */
    uint32_t localVar = 0;
    
    /* Input validation */
    if (paramName > MAX_VALUE) {
        return false;
    }
    
    /* Main logic with comments */
    /* ... */
    
    return true;
}
```

### Memory Efficiency

```c
/* Use smallest appropriate type */
uint8_t smallValue;      /* 0-255 */
uint16_t mediumValue;    /* 0-65535 */
uint32_t largeValue;     /* 0-4 billion */

/* Use const for read-only data */
const char* const errorStrings[] = { "OK", "STALL", "TIMEOUT" };

/* Avoid dynamic allocation - STM8 has only 1KB RAM */
```

### Feature Flags

```c
/* Wrap optional features in #if blocks */
#if FEATURE_EEPROM
void eeSave(void) {
    /* EEPROM code */
}
#endif
```

---

## Testing

### Hardware Testing

1. **Upload Hardware_Test.ino** first
2. **Test each component:**
   - Motor direction
   - Both limit switches
   - Both buttons
   - LED
   - UART communication

### Firmware Testing

1. **Test all commands** from API
2. **Test error conditions:**
   - Disconnect motor → ERR:STALL
   - Block limit switch → ERR:TIMEOUT
   - Invalid command → ERR:BAD_CMD
3. **Test power cycling:**
   - Save → power off → power on → verify load
4. **Test calibration:**
   - Full calibration sequence
   - Verify travel time reasonable

### Code Testing

```bash
# Compile with warnings
make compile CFLAGS="-Wall -Wextra"

# Check code size
make size
```

---

## Pull Request Process

### Before Submitting

- [ ] Code compiles without errors
- [ ] Code compiles without warnings
- [ ] All existing functionality still works
- [ ] New features are tested
- [ ] Documentation updated if needed
- [ ] CHANGELOG.md updated

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing Done
- [ ] Hardware tested on STM8S003F3P6
- [ ] All commands verified
- [ ] Error handling tested

## Checklist
- [ ] Code follows project style
- [ ] Comments added for complex logic
- [ ] No compiler warnings
- [ ] CHANGELOG.md updated
```

### Review Process

1. **Automated checks** must pass
2. **Code review** by maintainer
3. **Testing verification** if applicable
4. **Merge** when approved

---

## Project Structure

```
AGRINET-Valve-Controller/
├── src/                    # Main firmware source
│   └── AGRINET_Valve_Controller.ino
├── docs/                   # Documentation
│   ├── API.md
│   ├── HARDWARE.md
│   ├── MODULES.md
│   └── TROUBLESHOOTING.md
├── examples/               # Example sketches
│   ├── Hardware_Test/
│   └── Basic_Control/
├── hardware/               # Hardware documentation
├── tools/                  # Utility scripts
├── .github/workflows/      # CI/CD
├── platformio.ini          # PlatformIO config
├── Makefile               # Build automation
├── CHANGELOG.md           # Version history
├── CONTRIBUTING.md        # This file
├── LICENSE                # MIT License
└── README.md              # Project overview
```

---

## Questions?

- Open a GitHub issue for technical questions
- Tag issue with "question" label

Thank you for contributing! 🌱
