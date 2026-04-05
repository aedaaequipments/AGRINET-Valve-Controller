# =============================================================================
# AGRINET Smart Valve Controller - Makefile
# =============================================================================
#
# Usage:
#   make setup     - Install dependencies and board packages
#   make compile   - Compile firmware
#   make upload    - Upload to device
#   make serial    - Open serial monitor
#   make clean     - Clean build files
#   make all       - Compile and upload
#   make help      - Show this help
#
# =============================================================================

# Project Configuration
PROJECT_NAME := AGRINET_Valve_Controller
VERSION := 4.2.0
SKETCH := src/AGRINET_Valve_Controller.ino

# Board Configuration
FQBN := Sduino:stm8:stm8s003f3
PORT ?= /dev/ttyUSB0
BAUD := 115200

# Tools
ARDUINO_CLI := arduino-cli
STMFLASH := stm8flash
PIO := pio

# Board Manager URL
BOARD_URL := https://github.com/tenbaht/sduino/raw/master/package_sduino_stm8_index.json

# Build Directory
BUILD_DIR := build

# =============================================================================
# Main Targets
# =============================================================================

.PHONY: all compile upload clean serial setup help test docs

# Default: compile and upload
all: compile upload

# Compile firmware
compile:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  Compiling $(PROJECT_NAME) v$(VERSION)                         ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@mkdir -p $(BUILD_DIR)
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--output-dir $(BUILD_DIR) \
		--warnings all \
		$(SKETCH)
	@echo ""
	@echo "✅ Compilation successful!"
	@echo "   Output: $(BUILD_DIR)/"
	@ls -lh $(BUILD_DIR)/*.hex 2>/dev/null || true

# Upload to device
upload:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  Uploading to STM8S003F3P6 via $(PORT)                         ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	$(ARDUINO_CLI) upload \
		--fqbn $(FQBN) \
		--port $(PORT) \
		$(SKETCH)
	@echo ""
	@echo "✅ Upload complete!"

# Serial monitor
serial:
	@echo "Opening serial monitor at $(BAUD) baud..."
	@echo "Press Ctrl+C to exit"
	$(ARDUINO_CLI) monitor --port $(PORT) --config baudrate=$(BAUD)

# Clean build files
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
	rm -rf .pio
	@echo "✅ Clean complete!"

# =============================================================================
# Setup and Dependencies
# =============================================================================

# First-time setup
setup:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  Setting up development environment                            ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "📦 Installing sduino board package..."
	$(ARDUINO_CLI) config init --overwrite 2>/dev/null || true
	$(ARDUINO_CLI) config add board_manager.additional_urls $(BOARD_URL)
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install Sduino:stm8
	@echo ""
	@echo "✅ Setup complete!"
	@echo ""
	@echo "📋 Also install these system packages:"
	@echo "   Ubuntu/Debian: sudo apt install sdcc sdcc-libraries stlink-tools"
	@echo "   macOS:         brew install sdcc stlink"
	@echo "   Windows:       Download SDCC from http://sdcc.sourceforge.net/"

# Check installation
check:
	@echo "Checking installation..."
	@echo ""
	@echo -n "Arduino CLI: " && which $(ARDUINO_CLI) || echo "NOT FOUND"
	@echo -n "SDCC:        " && which sdcc || echo "NOT FOUND"
	@echo -n "stm8flash:   " && which $(STMFLASH) || echo "NOT FOUND (optional)"
	@echo -n "PlatformIO:  " && which $(PIO) || echo "NOT FOUND (optional)"
	@echo ""
	@$(ARDUINO_CLI) core list 2>/dev/null | grep -i stm8 || echo "sduino: NOT INSTALLED (run 'make setup')"

# =============================================================================
# Alternative Upload Methods
# =============================================================================

# Upload using stm8flash (ST-LINK V2)
flash-stlink: compile
	@echo "Flashing with stm8flash (ST-LINK V2)..."
	$(STMFLASH) -c stlinkv2 -p stm8s003f3 -w $(BUILD_DIR)/*.hex

# Upload using stm8flash (ST-LINK V2.1)
flash-stlink21: compile
	@echo "Flashing with stm8flash (ST-LINK V2.1)..."
	$(STMFLASH) -c stlinkv21 -p stm8s003f3 -w $(BUILD_DIR)/*.hex

# Read flash backup
backup:
	@echo "Reading flash to backup.hex..."
	$(STMFLASH) -c stlinkv2 -p stm8s003f3 -r backup_$(shell date +%Y%m%d_%H%M%S).hex

# Erase flash
erase:
	@echo "Erasing flash..."
	$(STMFLASH) -c stlinkv2 -p stm8s003f3 -u

# =============================================================================
# PlatformIO Targets
# =============================================================================

pio-build:
	$(PIO) run

pio-upload:
	$(PIO) run -t upload

pio-monitor:
	$(PIO) device monitor

pio-clean:
	$(PIO) run -t clean

# =============================================================================
# Development Helpers
# =============================================================================

# Show code size
size: compile
	@echo ""
	@echo "Code Size Analysis:"
	@cat $(BUILD_DIR)/*.size 2>/dev/null || du -h $(BUILD_DIR)/*.hex

# List connected devices
ports:
	$(ARDUINO_CLI) board list

# Verify compilation only
verify: compile
	@echo "✅ Verification passed!"

# Run hardware test
test:
	@echo "Compiling hardware test..."
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--output-dir $(BUILD_DIR) \
		examples/Hardware_Test/Hardware_Test.ino
	@echo "Use 'make upload-test' to flash test firmware"

upload-test:
	$(ARDUINO_CLI) upload \
		--fqbn $(FQBN) \
		--port $(PORT) \
		examples/Hardware_Test/Hardware_Test.ino

# =============================================================================
# Documentation
# =============================================================================

docs:
	@echo "Documentation available in docs/ directory:"
	@ls -la docs/

# =============================================================================
# Release
# =============================================================================

release: clean compile
	@echo "Creating release package..."
	@mkdir -p release
	@cp $(BUILD_DIR)/*.hex release/$(PROJECT_NAME)_v$(VERSION).hex
	@zip -r release/$(PROJECT_NAME)_v$(VERSION)_source.zip \
		src/ docs/ examples/ hardware/ \
		README.md LICENSE CHANGELOG.md \
		platformio.ini Makefile
	@echo "✅ Release package created in release/"
	@ls -la release/

# =============================================================================
# Help
# =============================================================================

help:
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║  AGRINET Valve Controller - Build System                       ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Usage: make <target>"
	@echo ""
	@echo "Main Targets:"
	@echo "  compile     - Compile firmware"
	@echo "  upload      - Upload to device"
	@echo "  serial      - Open serial monitor"
	@echo "  clean       - Clean build files"
	@echo "  all         - Compile and upload"
	@echo ""
	@echo "Setup:"
	@echo "  setup       - Install board packages"
	@echo "  check       - Verify installation"
	@echo ""
	@echo "Alternative Upload:"
	@echo "  flash-stlink   - Flash via stm8flash (ST-LINK V2)"
	@echo "  flash-stlink21 - Flash via stm8flash (ST-LINK V2.1)"
	@echo "  backup         - Read flash backup"
	@echo "  erase          - Erase flash"
	@echo ""
	@echo "PlatformIO:"
	@echo "  pio-build   - Build with PlatformIO"
	@echo "  pio-upload  - Upload with PlatformIO"
	@echo "  pio-monitor - Monitor with PlatformIO"
	@echo ""
	@echo "Variables:"
	@echo "  PORT=$(PORT)"
	@echo "  BAUD=$(BAUD)"
	@echo ""
	@echo "Example:"
	@echo "  make upload PORT=/dev/ttyACM0"
