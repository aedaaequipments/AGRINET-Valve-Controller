/**
 * =============================================================================
 * AGRINET Smart Valve Controller - Configuration Header v4.4.0
 * =============================================================================
 * @file    config.h
 * @brief   User-configurable settings for the valve controller
 * @version 4.4.0
 *
 * Build flags from platformio.ini override these defaults via #ifndef guards.
 * =============================================================================
 */

#ifndef AGRINET_CONFIG_H
#define AGRINET_CONFIG_H

/* =============================================================================
 * HARDWARE PIN CONFIGURATION
 * STM8 uses bare port names (PD2).  STM32 Arduino needs underscore format (PD_2).
 * ============================================================================= */

#ifdef __SDCC__
/* ---------- STM8 pin names ---------- */
#ifndef PIN_MOTOR_IN1
#define PIN_MOTOR_IN1           PD2     /* L293D IN1 - Close direction */
#endif
#ifndef PIN_MOTOR_IN2
#define PIN_MOTOR_IN2           PD3     /* L293D IN2 - Open direction */
#endif
#ifndef PIN_BTN_UP
#define PIN_BTN_UP              PC7     /* UP button - moves toward CLOSE */
#endif
#ifndef PIN_BTN_DOWN
#define PIN_BTN_DOWN            PC6     /* DOWN button - moves toward OPEN */
#endif
#ifndef PIN_LIMIT_CLOSED
#define PIN_LIMIT_CLOSED        PC5     /* Limit switch at CLOSED position */
#endif
#ifndef PIN_LIMIT_OPEN
#define PIN_LIMIT_OPEN          PC4     /* Limit switch at OPEN position */
#endif
#ifndef PIN_LED
#define PIN_LED                 PA3     /* Status LED */
#endif

#else
/* ---------- STM32 pin names (PX_N format) ---------- */
#ifndef PIN_MOTOR_IN1
#define PIN_MOTOR_IN1           PD_2    /* L293D IN1 - Close direction */
#endif
#ifndef PIN_MOTOR_IN2
#define PIN_MOTOR_IN2           PD_3    /* L293D IN2 - Open direction */
#endif
#ifndef PIN_BTN_UP
#define PIN_BTN_UP              PC_7    /* UP button - moves toward CLOSE */
#endif
#ifndef PIN_BTN_DOWN
#define PIN_BTN_DOWN            PC_6    /* DOWN button - moves toward OPEN */
#endif
#ifndef PIN_LIMIT_CLOSED
#define PIN_LIMIT_CLOSED        PC_5    /* Limit switch at CLOSED position */
#endif
#ifndef PIN_LIMIT_OPEN
#define PIN_LIMIT_OPEN          PC_4    /* Limit switch at OPEN position */
#endif
#ifndef PIN_LED
#define PIN_LED                 PA_3    /* Status LED */
#endif

#endif /* __SDCC__ */

#ifndef MOTOR_DIRECTION_INVERTED
#define MOTOR_DIRECTION_INVERTED    0
#endif

/* =============================================================================
 * TIMING CONFIGURATION (milliseconds)
 * ============================================================================= */

#ifndef MOTOR_DEBOUNCE_MS
#define MOTOR_DEBOUNCE_MS           50UL
#endif
#ifndef MOTOR_SETTLE_MS
#define MOTOR_SETTLE_MS             200UL
#endif
#ifndef MOTOR_MIN_TRAVEL_MS
#define MOTOR_MIN_TRAVEL_MS         500UL
#endif
#ifndef MOTOR_MAX_TRAVEL_MS
#define MOTOR_MAX_TRAVEL_MS         30000UL
#endif
#ifndef MOTOR_STALL_DETECT_MS
#define MOTOR_STALL_DETECT_MS       5000UL
#endif
#ifndef MOTOR_POSITION_MARGIN_MS
#define MOTOR_POSITION_MARGIN_MS    200UL
#endif

/* Power Management */
#ifndef IDLE_TIMEOUT_MS
#define IDLE_TIMEOUT_MS             30000UL
#endif
#ifndef HIBERNATE_TIMEOUT_MS
#define HIBERNATE_TIMEOUT_MS        300000UL
#endif

/* Communication */
#ifndef UART_BAUD
#define UART_BAUD                   115200UL
#endif
#ifndef CMD_BUFFER_LEN
#define CMD_BUFFER_LEN              24      /* Reduced from 40 to save RAM */
#endif

/* Retry Limits */
#ifndef MAX_RETRIES_CALIBRATION
#define MAX_RETRIES_CALIBRATION     3
#endif
#ifndef MAX_RETRIES_LIMIT
#define MAX_RETRIES_LIMIT           3
#endif

/* =============================================================================
 * FEATURE TOGGLES
 * ============================================================================= */

#ifndef FEATURE_WATCHDOG
#define FEATURE_WATCHDOG            1
#endif
#ifndef FEATURE_EEPROM
#define FEATURE_EEPROM              1
#endif
#ifndef FEATURE_SLEEP
#define FEATURE_SLEEP               1
#endif
#ifndef FEATURE_LED
#define FEATURE_LED                 1
#endif
#ifndef FEATURE_STALL_DETECT
#define FEATURE_STALL_DETECT        1
#endif
#ifndef FEATURE_AUTO_RECOVER
#define FEATURE_AUTO_RECOVER        1
#endif
#ifndef FEATURE_VERBOSE
#define FEATURE_VERBOSE             0
#endif

/* v4.4 new features */
#ifndef FEATURE_I2C
#define FEATURE_I2C                 0       /* I2C slave mode */
#endif
#ifndef FEATURE_UART_ADDRESSING
#define FEATURE_UART_ADDRESSING     0       /* @<id>:command prefix */
#endif
#ifndef FEATURE_UART_COMMANDS
#define FEATURE_UART_COMMANDS       1       /* Full UART command parsing */
#endif

/* =============================================================================
 * DEVICE IDENTIFICATION
 * ============================================================================= */

#ifndef FW_VERSION_MAJOR
#define FW_VERSION_MAJOR        4
#endif
#ifndef FW_VERSION_MINOR
#define FW_VERSION_MINOR        4
#endif
#ifndef FW_VERSION_PATCH
#define FW_VERSION_PATCH        0
#endif
#ifndef FW_VERSION_STRING
#define FW_VERSION_STRING       "4.4.0"
#endif
#ifndef FW_BUILD_DATE
#define FW_BUILD_DATE           __DATE__
#endif
#ifndef DEVICE_MODEL
#define DEVICE_MODEL            "AGN-GV"    /* Shortened to save flash */
#endif

/* Device ID for multi-valve bus */
#ifndef DEVICE_ID_DEFAULT
#define DEVICE_ID_DEFAULT       1           /* Default device ID */
#endif
#define DEVICE_ID_BROADCAST     0xFF        /* Broadcast address */
#define DEVICE_LABEL_LEN        8           /* Max label length */
#define DEVICE_LABEL_DEFAULT    "VALVE01"   /* Default label */

/* =============================================================================
 * EEPROM MEMORY MAP (128 bytes @ 0x4000)
 * ============================================================================= */

#define EEPROM_BASE_ADDR            0x4000
#define EEPROM_SIZE                 128

#define EE_ADDR_MAGIC_H             0x00
#define EE_ADDR_MAGIC_L             0x01
#define EE_ADDR_VERSION             0x02
#define EE_ADDR_FLAGS               0x03
#define EE_ADDR_TRAVEL_B3           0x04
#define EE_ADDR_TRAVEL_B2           0x05
#define EE_ADDR_TRAVEL_B1           0x06
#define EE_ADDR_TRAVEL_B0           0x07
#define EE_ADDR_POSITION            0x08
#define EE_ADDR_CYCLES_B3           0x09
#define EE_ADDR_CYCLES_B2           0x0A
#define EE_ADDR_CYCLES_B1           0x0B
#define EE_ADDR_CYCLES_B0           0x0C
#define EE_ADDR_ERRORS              0x0D
#define EE_ADDR_CHECKSUM            0x0E
#define EE_ADDR_DEVICE_ID           0x0F    /* v4.4: Device ID */
#define EE_ADDR_LABEL               0x10    /* v4.4: Device label (8 bytes) */

#define EE_MAGIC_VALUE              0x4147  /* "AG" */
#define EE_CONFIG_VERSION           0x44    /* v4.4 format */
#define EE_FLAG_CALIBRATED          0x01
#define EE_FLAG_VALID_POS           0x02

/* =============================================================================
 * I2C SLAVE CONFIGURATION (v4.4)
 * ============================================================================= */

#if FEATURE_I2C

#define I2C_BASE_ADDR               0x20    /* I2C addr = 0x20 + device_id */

/* I2C Register Map */
#define I2C_REG_COUNT               8
#define I2C_REG_DEV_ID              0x00    /* R/W: Device ID */
#define I2C_REG_CMD                 0x01    /* W:   Command register */
#define I2C_REG_POSITION            0x02    /* R:   Position 0-100 */
#define I2C_REG_FLAGS               0x03    /* R:   Status flags */
#define I2C_REG_ERROR               0x04    /* R:   Last error code */
#define I2C_REG_TRAVEL_H            0x05    /* R:   Travel time MSB */
#define I2C_REG_TRAVEL_L            0x06    /* R:   Travel time LSB */
#define I2C_REG_FW_VER              0x07    /* R:   FW version */

/* I2C Command Bytes (written to register 0x01) */
#define I2C_CMD_STOP                0x00
#define I2C_CMD_OPEN                0x01
#define I2C_CMD_CLOSE               0x02
#define I2C_CMD_QUARTER             0x03
#define I2C_CMD_HALF                0x04
#define I2C_CMD_THREEQUARTER        0x05
#define I2C_CMD_CALIBRATE           0x06
#define I2C_CMD_SAVE                0x07
#define I2C_CMD_SLEEP               0x08
#define I2C_CMD_RESET               0x09
#define I2C_CMD_CLEAR               0x0A

/* I2C Flags byte (register 0x03) */
#define I2C_FLAG_CALIBRATED         0x01
#define I2C_FLAG_MOVING             0x02
#define I2C_FLAG_FAULT              0x04
#define I2C_FLAG_SLEEPING           0x08

/* STM8-only I2C Peripheral Registers (not needed on STM32 — Wire library used instead) */
#ifdef __SDCC__
#define I2C_CR1_REG     (*(volatile uint8_t*)0x5210)
#define I2C_CR2_REG     (*(volatile uint8_t*)0x5211)
#define I2C_FREQR_REG   (*(volatile uint8_t*)0x5212)
#define I2C_OARL_REG    (*(volatile uint8_t*)0x5213)
#define I2C_OARH_REG    (*(volatile uint8_t*)0x5214)
#define I2C_DR_REG      (*(volatile uint8_t*)0x5216)
#define I2C_SR1_REG     (*(volatile uint8_t*)0x5217)
#define I2C_SR2_REG     (*(volatile uint8_t*)0x5218)
#define I2C_SR3_REG     (*(volatile uint8_t*)0x5219)
#define I2C_ITR_REG     (*(volatile uint8_t*)0x521A)
#define I2C_CCRL_REG    (*(volatile uint8_t*)0x521B)
#define I2C_CCRH_REG    (*(volatile uint8_t*)0x521C)
#define I2C_TRISER_REG  (*(volatile uint8_t*)0x521D)

/* I2C CR1 bits */
#define I2C_CR1_PE      0x01
#define I2C_CR1_NOSTRETCH 0x80

/* I2C CR2 bits */
#define I2C_CR2_ACK     0x04
#define I2C_CR2_STOP    0x02
#define I2C_CR2_START   0x01

/* I2C SR1 bits */
#define I2C_SR1_SB      0x01
#define I2C_SR1_ADDR    0x02
#define I2C_SR1_BTF     0x04
#define I2C_SR1_STOPF   0x10
#define I2C_SR1_RXNE    0x40
#define I2C_SR1_TXE     0x80

/* I2C SR2 bits */
#define I2C_SR2_BERR    0x01
#define I2C_SR2_ARLO    0x02
#define I2C_SR2_AF      0x04
#define I2C_SR2_OVR     0x08

/* I2C SR3 bits */
#define I2C_SR3_MSL     0x01
#define I2C_SR3_BUSY    0x02
#define I2C_SR3_TRA     0x04

/* I2C ITR bits */
#define I2C_ITR_ITERREN 0x01
#define I2C_ITR_ITEVTEN 0x02
#define I2C_ITR_ITBUFEN 0x04

#endif /* __SDCC__ */
#endif /* FEATURE_I2C */

#endif /* AGRINET_CONFIG_H */
