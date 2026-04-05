/**
 * =============================================================================
 * AGRINET Hardware Test Mode
 * =============================================================================
 * 
 * @file    Hardware_Test.ino
 * @brief   Hardware verification sketch for AGRINET Valve Controller
 * @version 1.0.0
 * 
 * PURPOSE:
 * Upload this sketch BEFORE the main firmware to verify all hardware
 * connections are correct. Tests motor, limit switches, buttons, and LED.
 * 
 * USAGE:
 * 1. Upload this sketch
 * 2. Open Serial Monitor (115200 baud)
 * 3. Use keyboard commands to test each component
 * 4. Verify all readings match expected behavior
 * 5. Upload main firmware when hardware is verified
 * 
 * =============================================================================
 */

#include <Arduino.h>

/* Pin definitions - same as main firmware */
#define PIN_MOTOR_IN1       PD2     /* Motor driver IN1 */
#define PIN_MOTOR_IN2       PD3     /* Motor driver IN2 */
#define PIN_BTN_UP          PC7     /* Button UP */
#define PIN_BTN_DOWN        PC6     /* Button DOWN */
#define PIN_LIMIT_CLOSED    PC5     /* Limit switch CLOSED */
#define PIN_LIMIT_OPEN      PC4     /* Limit switch OPEN */
#define PIN_LED             LED_BUILTIN  /* Status LED (PB5) */

/* Test mode state */
bool motorRunning = false;
bool ledState = false;

void setup() {
    /* Initialize pins */
    pinMode(PIN_MOTOR_IN1, OUTPUT);
    pinMode(PIN_MOTOR_IN2, OUTPUT);
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_LIMIT_CLOSED, INPUT_PULLUP);
    pinMode(PIN_LIMIT_OPEN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    
    /* Motor off */
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    
    /* LED off */
    digitalWrite(PIN_LED, LOW);
    
    /* Initialize UART */
    Serial_begin(115200);
    
    /* Print welcome message */
    Serial_print_s("\r\n");
    Serial_print_s("╔════════════════════════════════════════════════════╗\r\n");
    Serial_print_s("║     AGRINET Hardware Test Mode v1.0                ║\r\n");
    Serial_print_s("║     Valve Controller Hardware Verification         ║\r\n");
    Serial_print_s("╚════════════════════════════════════════════════════╝\r\n");
    Serial_print_s("\r\n");
    Serial_print_s("Pin Configuration:\r\n");
    Serial_print_s("  PD2 (Pin 19) = Motor IN1\r\n");
    Serial_print_s("  PD3 (Pin 20) = Motor IN2\r\n");
    Serial_print_s("  PC7 (Pin 17) = Button UP\r\n");
    Serial_print_s("  PC6 (Pin 16) = Button DOWN\r\n");
    Serial_print_s("  PC5 (Pin 15) = Limit CLOSED\r\n");
    Serial_print_s("  PC4 (Pin 14) = Limit OPEN\r\n");
    Serial_print_s("  PB5 (Pin 11) = LED\r\n");
    Serial_print_s("\r\n");
    Serial_print_s("Commands:\r\n");
    Serial_print_s("  1 = Motor CLOSE (IN1=HIGH, IN2=LOW)\r\n");
    Serial_print_s("  2 = Motor OPEN  (IN1=LOW, IN2=HIGH)\r\n");
    Serial_print_s("  0 = Motor STOP  (both LOW)\r\n");
    Serial_print_s("  b = Motor BRAKE (both HIGH, brief)\r\n");
    Serial_print_s("  s = Show all input status\r\n");
    Serial_print_s("  l = Toggle LED\r\n");
    Serial_print_s("  t = LED blink test (5 times)\r\n");
    Serial_print_s("  c = Continuous status monitoring\r\n");
    Serial_print_s("  h = Show this help\r\n");
    Serial_print_s("\r\n");
    Serial_print_s("Button/switch changes are automatically reported.\r\n");
    Serial_print_s("════════════════════════════════════════════════════\r\n");
    Serial_print_s("\r\n");
    
    /* Blink LED 3 times to indicate ready */
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED, HIGH);
        delay(100);
        digitalWrite(PIN_LED, LOW);
        delay(100);
    }
    
    Serial_print_s("Ready! Type a command...\r\n\r\n");
}

void loop() {
    static bool lastBtnUp = false;
    static bool lastBtnDown = false;
    static bool lastLimitClosed = false;
    static bool lastLimitOpen = false;
    static bool continuousMode = false;
    static uint32_t lastStatusTime = 0;
    
    /* Read current state */
    bool btnUp = (digitalRead(PIN_BTN_UP) == LOW);
    bool btnDown = (digitalRead(PIN_BTN_DOWN) == LOW);
    bool limitClosed = (digitalRead(PIN_LIMIT_CLOSED) == HIGH);
    bool limitOpen = (digitalRead(PIN_LIMIT_OPEN) == HIGH);
    
    /* Report button changes */
    if (btnUp != lastBtnUp) {
        Serial_print_s("BTN_UP (PC7): ");
        Serial_print_s(btnUp ? ">>> PRESSED <<<" : "released");
        Serial_print_s("\r\n");
        lastBtnUp = btnUp;
    }
    
    if (btnDown != lastBtnDown) {
        Serial_print_s("BTN_DOWN (PC6): ");
        Serial_print_s(btnDown ? ">>> PRESSED <<<" : "released");
        Serial_print_s("\r\n");
        lastBtnDown = btnDown;
    }
    
    /* Report limit switch changes */
    if (limitClosed != lastLimitClosed) {
        Serial_print_s("LIMIT_CLOSED (PC5): ");
        Serial_print_s(limitClosed ? ">>> AT LIMIT <<<" : "not at limit");
        Serial_print_s("\r\n");
        lastLimitClosed = limitClosed;
    }
    
    if (limitOpen != lastLimitOpen) {
        Serial_print_s("LIMIT_OPEN (PC4): ");
        Serial_print_s(limitOpen ? ">>> AT LIMIT <<<" : "not at limit");
        Serial_print_s("\r\n");
        lastLimitOpen = limitOpen;
    }
    
    /* Continuous mode status update */
    if (continuousMode && (millis() - lastStatusTime > 500)) {
        printStatus(btnUp, btnDown, limitClosed, limitOpen);
        lastStatusTime = millis();
    }
    
    /* Process serial commands */
    if (Serial_available() > 0) {
        char c = Serial_read();
        
        switch (c) {
            case '1':
                /* Motor CLOSE */
                digitalWrite(PIN_MOTOR_IN1, HIGH);
                digitalWrite(PIN_MOTOR_IN2, LOW);
                motorRunning = true;
                Serial_print_s("MOTOR: CLOSE direction\r\n");
                Serial_print_s("  IN1 (PD2) = HIGH\r\n");
                Serial_print_s("  IN2 (PD3) = LOW\r\n");
                break;
                
            case '2':
                /* Motor OPEN */
                digitalWrite(PIN_MOTOR_IN1, LOW);
                digitalWrite(PIN_MOTOR_IN2, HIGH);
                motorRunning = true;
                Serial_print_s("MOTOR: OPEN direction\r\n");
                Serial_print_s("  IN1 (PD2) = LOW\r\n");
                Serial_print_s("  IN2 (PD3) = HIGH\r\n");
                break;
                
            case '0':
                /* Motor STOP (coast) */
                digitalWrite(PIN_MOTOR_IN1, LOW);
                digitalWrite(PIN_MOTOR_IN2, LOW);
                motorRunning = false;
                Serial_print_s("MOTOR: STOP (coast)\r\n");
                Serial_print_s("  IN1 (PD2) = LOW\r\n");
                Serial_print_s("  IN2 (PD3) = LOW\r\n");
                break;
                
            case 'b':
            case 'B':
                /* Motor BRAKE */
                digitalWrite(PIN_MOTOR_IN1, HIGH);
                digitalWrite(PIN_MOTOR_IN2, HIGH);
                Serial_print_s("MOTOR: BRAKE (brief)\r\n");
                delay(100);
                digitalWrite(PIN_MOTOR_IN1, LOW);
                digitalWrite(PIN_MOTOR_IN2, LOW);
                motorRunning = false;
                Serial_print_s("MOTOR: STOPPED\r\n");
                break;
                
            case 's':
            case 'S':
                /* Show status */
                printStatus(btnUp, btnDown, limitClosed, limitOpen);
                break;
                
            case 'l':
            case 'L':
                /* Toggle LED */
                ledState = !ledState;
                digitalWrite(PIN_LED, ledState);
                Serial_print_s("LED: ");
                Serial_print_s(ledState ? "ON" : "OFF");
                Serial_print_s("\r\n");
                break;
                
            case 't':
            case 'T':
                /* LED blink test */
                Serial_print_s("LED TEST: Blinking 5 times...\r\n");
                for (int i = 0; i < 5; i++) {
                    digitalWrite(PIN_LED, HIGH);
                    Serial_print_s("  Blink ");
                    Serial_print_s("ON\r\n");
                    delay(200);
                    digitalWrite(PIN_LED, LOW);
                    delay(200);
                }
                Serial_print_s("LED TEST: Complete\r\n");
                ledState = false;
                break;
                
            case 'c':
            case 'C':
                /* Toggle continuous mode */
                continuousMode = !continuousMode;
                Serial_print_s("Continuous mode: ");
                Serial_print_s(continuousMode ? "ON (press 'c' to stop)" : "OFF");
                Serial_print_s("\r\n");
                break;
                
            case 'h':
            case 'H':
            case '?':
                /* Help */
                Serial_print_s("\r\nCommands:\r\n");
                Serial_print_s("  1 = Motor CLOSE\r\n");
                Serial_print_s("  2 = Motor OPEN\r\n");
                Serial_print_s("  0 = Motor STOP\r\n");
                Serial_print_s("  b = Motor BRAKE\r\n");
                Serial_print_s("  s = Show status\r\n");
                Serial_print_s("  l = Toggle LED\r\n");
                Serial_print_s("  t = LED test\r\n");
                Serial_print_s("  c = Continuous monitoring\r\n");
                Serial_print_s("  h = Help\r\n");
                Serial_print_s("\r\n");
                break;
                
            case '\r':
            case '\n':
                /* Ignore line endings */
                break;
                
            default:
                Serial_print_s("Unknown command: ");
                Serial_print_s(&c);
                Serial_print_s(" (type 'h' for help)\r\n");
                break;
        }
    }
    
    /* Small delay */
    delay(10);
}

/**
 * @brief Print current status of all inputs
 */
void printStatus(bool btnUp, bool btnDown, bool limitClosed, bool limitOpen) {
    Serial_print_s("\r\n");
    Serial_print_s("┌────────────────────────────────────┐\r\n");
    Serial_print_s("│         CURRENT STATUS             │\r\n");
    Serial_print_s("├────────────────────────────────────┤\r\n");
    
    Serial_print_s("│ BTN_UP (PC7):       ");
    if (btnUp) {
        Serial_print_s("PRESSED     │\r\n");
    } else {
        Serial_print_s("released    │\r\n");
    }
    
    Serial_print_s("│ BTN_DOWN (PC6):     ");
    if (btnDown) {
        Serial_print_s("PRESSED     │\r\n");
    } else {
        Serial_print_s("released    │\r\n");
    }
    
    Serial_print_s("│ LIMIT_CLOSED (PC5): ");
    if (limitClosed) {
        Serial_print_s("AT LIMIT    │\r\n");
    } else {
        Serial_print_s("not at limit│\r\n");
    }
    
    Serial_print_s("│ LIMIT_OPEN (PC4):   ");
    if (limitOpen) {
        Serial_print_s("AT LIMIT    │\r\n");
    } else {
        Serial_print_s("not at limit│\r\n");
    }
    
    Serial_print_s("│ LED (PB5):          ");
    if (ledState) {
        Serial_print_s("ON          │\r\n");
    } else {
        Serial_print_s("OFF         │\r\n");
    }
    
    Serial_print_s("│ MOTOR:              ");
    if (motorRunning) {
        Serial_print_s("RUNNING     │\r\n");
    } else {
        Serial_print_s("STOPPED     │\r\n");
    }
    
    Serial_print_s("└────────────────────────────────────┘\r\n");
    Serial_print_s("\r\n");
}
