/**
 * AGRINET Smart Valve Controller v4.4.0
 * Gate valve controller with I2C slave & multi-device support
 * Target: STM8S003F3P6 (8KB flash, 1KB RAM, 128B EEPROM)
 *
 * Pin Map v4.4:
 *   PD2=IN1 PD3=IN2 PC7=BtnUP PC6=BtnDN PC5=LimCL PC4=LimOP
 *   PA3=LED PB5=SCL PB4=SDA PD5=TX PD6=RX
 */

#include <Arduino.h>
#include "config.h"

#ifdef __SDCC__
/* STM8-only registers */
#ifndef FLASH_IAPSR
#define FLASH_IAPSR     (*(volatile uint8_t*)0x505F)
#endif
#ifndef FLASH_DUKR
#define FLASH_DUKR      (*(volatile uint8_t*)0x5064)
#endif
#define FL_DUL  0x08
#define FL_EOP  0x04

#ifndef IWDG_KR
#define IWDG_KR         (*(volatile uint8_t*)0x50E0)
#endif
#ifndef IWDG_PR
#define IWDG_PR         (*(volatile uint8_t*)0x50E1)
#endif
#ifndef IWDG_RLR
#define IWDG_RLR        (*(volatile uint8_t*)0x50E2)
#endif
#ifndef WWDG_CR
#define WWDG_CR         (*(volatile uint8_t*)0x50D1)
#endif
#ifndef RST_SR
#define RST_SR          (*(volatile uint8_t*)0x50B3)
#endif

#else
/* STM32 — use C wrappers from stm32_compat.h */
#include "stm32_compat.h"
/* Dummy FLASH/WDT macros so shared code compiles (replaced by functions below) */
#define FL_DUL  0x08
#define FL_EOP  0x04
#endif /* __SDCC__ */

/* Enums - compact */
enum SysState { ST_BOOT=0,ST_INIT,ST_IDLE,ST_CAL,ST_MOPEN,ST_MCLOSE,
                ST_POS,ST_HOLD,ST_SLEEP,ST_HIBER,ST_FAULT,ST_RECOVER };
enum ErrCode  { E_OK=0,E_STALL,E_TMO,E_LIM,E_STUCK,E_CAL,E_RANGE,
                E_POS,E_EE,E_CMD,E_WDT,E_INT };
enum MotDir   { M_STOP=0,M_OPEN,M_CLOSE };
enum MotSt    { MS_IDLE=0,MS_RUN,MS_STALL,MS_LIM };
enum Cmd      { C_NONE=0,C_OPEN,C_CLOSE,C_QTR,C_HALF,C_3QTR,C_CAL,
                C_STAT,C_VER,C_POS,C_HELP,C_SAVE,C_LOAD,C_CLR,
                C_SLP,C_WAKE,C_RST,C_STOP,C_DEVID,C_INV };
enum Led      { L_OFF=0,L_ON,L_SLOW,L_FAST,L_SOS };

/* Globals */
uint8_t  g_st     = ST_BOOT;
uint8_t  g_err    = E_OK;
uint8_t  g_errcnt = 0;
uint8_t  g_mdir   = M_STOP;
uint8_t  g_mst    = MS_IDLE;
bool     g_men    = true;
uint32_t g_mstart = 0;
int8_t   g_pos    = -1;
int8_t   g_tgt    = -1;
bool     g_cal    = false;
uint32_t g_trvl   = 0;
uint32_t g_actMs  = 0;
uint32_t g_ledMs  = 0;
uint32_t g_cycl   = 0;
uint32_t g_fltMs  = 0;
char     g_cmd[CMD_BUFFER_LEN + 1];
uint8_t  g_ci     = 0;
uint8_t  g_lm     = L_OFF;
bool     g_ls     = false;
uint8_t  g_sos    = 0;
bool     g_svn    = false;
bool     g_wdt    = false;
bool     g_estop  = false;
uint8_t  g_devId  = DEVICE_ID_DEFAULT;

#if FEATURE_I2C
volatile uint8_t g_i2r[I2C_REG_COUNT];
volatile uint8_t g_i2p  = 0;
volatile uint8_t g_i2c  = 0;  /* pending cmd */
volatile bool    g_i2f  = true; /* first byte flag */
#endif

/* Prototypes */
void stLoop(void);
void stSet(uint8_t s);
void gpioInit(void);
bool limCl(void);
bool limOp(void);
bool btnU(void);
bool btnD(void);
void motSet(uint8_t d);
void motOff(void);
bool motLim(uint8_t d, uint32_t tmo);
bool motTime(uint8_t d, uint32_t ms);
bool motPos(int8_t pct);
bool motAbort(void);
bool calRun(void);
void posUpd(int8_t p);
void eeInit(void);
bool eeUlk(void);
void eeLk(void);
bool eeWr(uint8_t a, uint8_t d);
uint8_t eeRd(uint8_t a);
bool eeW32(uint8_t a, uint32_t d);
uint32_t eeR32(uint8_t a);
bool eeSave(void);
bool eeLoad(void);
void eeClr(void);
uint8_t eeCRC(void);
void txS(const char* s);
void txN(int32_t n);
void txOK(void);
void txE(uint8_t e);
const char* eStr(uint8_t e);
#if FEATURE_UART_COMMANDS
void rxProc(void);
uint8_t rxParse(const char* s);
void rxExec(const char* s);
#endif
void cmdDo(uint8_t c);
void btnChk(void);
void btnDir(uint8_t d);
void pwrChk(void);
void pwrSlp(void);
void pwrHib(void);
void pwrWk(void);
void wdtI(void);
void wdtF(void);
void ledS(uint8_t m);
void ledU(void);
void errS(uint8_t e);
void errC(void);
void errR(void);
void idleRst(void);
void dlyW(uint32_t ms);
#if FEATURE_I2C
void i2cI(void);
void i2cU(void);
#endif

/* =============================================================================
 * SETUP & LOOP
 * ============================================================================= */

void setup() {
#ifdef __SDCC__
    uint8_t rr = RST_SR;
    RST_SR = 0xFF;
#else
    uint8_t rr = stm32_rst_reason();
#endif
    gpioInit();
    motOff();
    Serial_begin(UART_BAUD);

    #if FEATURE_EEPROM
    eeLoad();
    #endif
    #if FEATURE_WATCHDOG
    wdtI();
    #endif
    #if FEATURE_I2C
    i2cI();
    #endif

    if (rr & 0x20) g_err = E_WDT;
    txOK();
    g_actMs = millis();
    stSet(ST_IDLE);
}

void loop() {
    #if FEATURE_WATCHDOG
    wdtF();
    #endif
    #if FEATURE_LED
    ledU();
    #endif

    #if FEATURE_I2C
    i2cU();
    if (g_i2c && g_st == ST_IDLE) {
        uint8_t ic = g_i2c;
        g_i2c = 0;
        idleRst();
        switch (ic) {
            case I2C_CMD_STOP:  cmdDo(C_STOP); break;
            case I2C_CMD_OPEN:  cmdDo(C_OPEN); break;
            case I2C_CMD_CLOSE: cmdDo(C_CLOSE); break;
            case I2C_CMD_QUARTER: cmdDo(C_QTR); break;
            case I2C_CMD_HALF:  cmdDo(C_HALF); break;
            case I2C_CMD_THREEQUARTER: cmdDo(C_3QTR); break;
            case I2C_CMD_CALIBRATE: cmdDo(C_CAL); break;
            case I2C_CMD_SAVE:  cmdDo(C_SAVE); break;
            case I2C_CMD_SLEEP: cmdDo(C_SLP); break;
            case I2C_CMD_RESET: cmdDo(C_RST); break;
            case I2C_CMD_CLEAR: cmdDo(C_CLR); break;
        }
    }
    #endif

    stLoop();
}

/* State Machine */
void stLoop(void) {
    switch (g_st) {
        case ST_IDLE:
            ledS(L_SLOW);
            #if FEATURE_UART_COMMANDS
            rxProc();
            #endif
            btnChk();
            #if FEATURE_SLEEP
            pwrChk();
            #endif
            break;
        case ST_CAL: case ST_MOPEN: case ST_MCLOSE: case ST_POS:
            ledS(L_FAST);
            break;
        case ST_HOLD:
            ledS(L_ON); dlyW(300); stSet(ST_IDLE);
            break;
        case ST_SLEEP:
            #if FEATURE_SLEEP
            pwrSlp();
            #else
            stSet(ST_IDLE);
            #endif
            break;
        case ST_HIBER:
            #if FEATURE_SLEEP
            pwrHib();
            #else
            stSet(ST_IDLE);
            #endif
            break;
        case ST_FAULT:
            ledS(L_SOS);
            #if FEATURE_UART_COMMANDS
            rxProc();
            #endif
            #if FEATURE_AUTO_RECOVER
            if (!g_fltMs) g_fltMs = millis();
            if ((millis() - g_fltMs) > 10000UL) { g_fltMs = 0; errR(); }
            #endif
            break;
        case ST_RECOVER:
            errR();
            break;
        default:
            stSet(ST_IDLE);
            break;
    }
}

void stSet(uint8_t s) {
    if (s == ST_FAULT) g_fltMs = 0;
    if (s == ST_FAULT || s == ST_IDLE) g_sos = 0;
    g_st = s;
}

/* =============================================================================
 * GPIO
 * ============================================================================= */

void gpioInit(void) {
    pinMode(PIN_MOTOR_IN1, OUTPUT); pinMode(PIN_MOTOR_IN2, OUTPUT);
    digitalWrite(PIN_MOTOR_IN1, LOW); digitalWrite(PIN_MOTOR_IN2, LOW);
    pinMode(PIN_BTN_UP, INPUT_PULLUP); pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_LIMIT_CLOSED, INPUT_PULLUP); pinMode(PIN_LIMIT_OPEN, INPUT_PULLUP);
    #if FEATURE_LED
    pinMode(PIN_LED, OUTPUT); digitalWrite(PIN_LED, LOW);
    #endif
}

bool limCl(void) { return digitalRead(PIN_LIMIT_CLOSED) == HIGH; }
bool limOp(void) { return digitalRead(PIN_LIMIT_OPEN) == HIGH; }
bool btnU(void)  { return digitalRead(PIN_BTN_UP) == LOW; }
bool btnD(void)  { return digitalRead(PIN_BTN_DOWN) == LOW; }

/* =============================================================================
 * MOTOR
 * ============================================================================= */

void motSet(uint8_t d) {
    if (!g_men) { motOff(); return; }
    if (d == M_OPEN) {
        #if MOTOR_DIRECTION_INVERTED
        digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);
        #else
        digitalWrite(PIN_MOTOR_IN1, LOW);  digitalWrite(PIN_MOTOR_IN2, HIGH);
        #endif
    } else if (d == M_CLOSE) {
        #if MOTOR_DIRECTION_INVERTED
        digitalWrite(PIN_MOTOR_IN1, LOW);  digitalWrite(PIN_MOTOR_IN2, HIGH);
        #else
        digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);
        #endif
    } else { motOff(); return; }
    g_mdir = d; g_mst = MS_RUN; g_mstart = millis();
}

void motOff(void) {
    digitalWrite(PIN_MOTOR_IN1, LOW); digitalWrite(PIN_MOTOR_IN2, LOW);
    g_mdir = M_STOP; g_mst = MS_IDLE;
}

bool motAbort(void) {
    if (Serial_available() > 0) { Serial_read(); g_estop = true; motOff(); return true; }
    return false;
}

bool motLim(uint8_t d, uint32_t tmo) {
    uint8_t r; uint32_t st, el; bool at, vf;
    if (d == M_STOP) return false;
    g_estop = false;

    for (r = 0; r < MAX_RETRIES_LIMIT; r++) {
        if (d == M_OPEN && limOp())  { g_pos = 100; g_mst = MS_LIM; return true; }
        if (d == M_CLOSE && limCl()) { g_pos = 0;   g_mst = MS_LIM; return true; }

        st = millis(); at = false;
        motSet(d);
        while (!at) {
            #if FEATURE_WATCHDOG
            wdtF();
            #endif
            if (motAbort()) return false;
            at = (d == M_OPEN) ? limOp() : limCl();
            el = millis() - st;
            if (el > tmo) { motOff(); errS(E_TMO); return false; }
            #if FEATURE_STALL_DETECT
            if (el > MOTOR_STALL_DETECT_MS && el > (tmo/2) && !at) {
                motOff(); errS(E_STALL); return false;
            }
            #endif
        }
        motOff(); dlyW(MOTOR_DEBOUNCE_MS);
        vf = (d == M_OPEN) ? limOp() : limCl();
        if (vf) { g_pos = (d == M_OPEN) ? 100 : 0; g_mst = MS_LIM; return true; }
        dlyW(100);
    }
    errS(E_LIM); return false;
}

bool motTime(uint8_t d, uint32_t ms) {
    uint32_t st = millis(), tgt = ms + MOTOR_POSITION_MARGIN_MS;
    bool hit = false;
    if (d == M_STOP || ms == 0) return false;
    g_estop = false;
    motSet(d);
    while ((millis() - st) < tgt) {
        #if FEATURE_WATCHDOG
        wdtF();
        #endif
        if (motAbort()) return false;
        if (d == M_OPEN && limOp())  { hit = true; break; }
        if (d == M_CLOSE && limCl()) { hit = true; break; }
    }
    motOff();
    if (hit) g_pos = (d == M_OPEN) ? 100 : 0;
    return true;
}

bool motPos(int8_t pct) {
    uint32_t ms; int16_t dl;
    if (pct < 0 || pct > 100) { errS(E_CMD); return false; }
    if (g_pos == pct) { txOK(); return true; }

    if (pct == 0) {
        stSet(ST_MCLOSE);
        if (motLim(M_CLOSE, MOTOR_MAX_TRAVEL_MS)) { posUpd(0); txOK(); stSet(ST_HOLD); return true; }
        if (!g_estop) stSet(ST_FAULT); return false;
    }
    if (pct == 100) {
        stSet(ST_MOPEN);
        if (motLim(M_OPEN, MOTOR_MAX_TRAVEL_MS)) { posUpd(100); txOK(); stSet(ST_HOLD); return true; }
        if (!g_estop) stSet(ST_FAULT); return false;
    }

    if (!g_cal || g_trvl == 0) { if (!calRun()) return false; }

    stSet(ST_POS);
    if (g_pos != 100) {
        if (!motLim(M_OPEN, MOTOR_MAX_TRAVEL_MS)) {
            if (!g_estop) stSet(ST_FAULT); return false;
        }
        g_pos = 100; dlyW(MOTOR_SETTLE_MS);
    }

    dl = 100 - pct;
    ms = ((uint32_t)dl * g_trvl) / 100UL;
    if (!motTime(M_CLOSE, ms)) { if (!g_estop) stSet(ST_FAULT); return false; }
    posUpd(pct); txOK(); stSet(ST_HOLD); return true;
}

/* =============================================================================
 * CALIBRATION
 * ============================================================================= */

bool calRun(void) {
    uint32_t st, dur; uint8_t a;
    stSet(ST_CAL);

    for (a = 1; a <= MAX_RETRIES_CALIBRATION; a++) {
        if (!motLim(M_CLOSE, MOTOR_MAX_TRAVEL_MS)) continue;
        dlyW(MOTOR_SETTLE_MS);
        if (!motLim(M_OPEN, MOTOR_MAX_TRAVEL_MS)) continue;
        dlyW(MOTOR_SETTLE_MS);
        st = millis();
        motSet(M_CLOSE);
        while (!limCl()) {
            #if FEATURE_WATCHDOG
            wdtF();
            #endif
            if ((millis()-st) > MOTOR_MAX_TRAVEL_MS) { motOff(); break; }
        }
        motOff();
        if (!limCl()) continue;
        dur = millis() - st;
        if (dur >= MOTOR_MIN_TRAVEL_MS && dur <= MOTOR_MAX_TRAVEL_MS) {
            g_trvl = dur; g_cal = true; g_pos = 0;
            #if FEATURE_EEPROM
            eeSave();
            #endif
            txN(dur);
            stSet(ST_IDLE); return true;
        }
    }
    errS(E_CAL); stSet(ST_FAULT); return false;
}

void posUpd(int8_t p) {
    g_pos = p; g_tgt = p; g_cycl++; g_svn = true;
    if ((g_cycl % 10) == 0) {
        #if FEATURE_EEPROM
        eeSave();
        #endif
        g_svn = false;
    }
}

/* =============================================================================
 * EEPROM
 * ============================================================================= */

#if FEATURE_EEPROM

void eeInit(void) {}

#ifdef __SDCC__
bool eeUlk(void) {
    if (FLASH_IAPSR & FL_DUL) return true;
    FLASH_DUKR = 0xAE; FLASH_DUKR = 0x56;
    uint16_t t = 1000;
    while (!(FLASH_IAPSR & FL_DUL) && t > 0) t--;
    return (FLASH_IAPSR & FL_DUL) != 0;
}

void eeLk(void) { FLASH_IAPSR &= ~FL_DUL; }

bool eeWr(uint8_t a, uint8_t d) {
    if (a >= EEPROM_SIZE) return false;
    volatile uint8_t* p = (volatile uint8_t*)(EEPROM_BASE_ADDR + a);
    if (*p == d) return true;
    *p = d;
    uint16_t t = 10000;
    while (!(FLASH_IAPSR & FL_EOP) && t > 0) t--;
    return t > 0;
}

uint8_t eeRd(uint8_t a) {
    if (a >= EEPROM_SIZE) return 0xFF;
    return *(volatile uint8_t*)(EEPROM_BASE_ADDR + a);
}

#else  /* STM32 — use EEPROM emulation via stm32_compat */
bool eeUlk(void) { return true; }
void eeLk(void)  {}

bool eeWr(uint8_t a, uint8_t d) {
    if (a >= EEPROM_SIZE) return false;
    return stm32_ee_write(a, d);
}

uint8_t eeRd(uint8_t a) {
    if (a >= EEPROM_SIZE) return 0xFF;
    return stm32_ee_read(a);
}
#endif /* __SDCC__ */

bool eeW32(uint8_t a, uint32_t d) {
    bool ok = true;
    ok &= eeWr(a, (uint8_t)(d>>24)); ok &= eeWr(a+1, (uint8_t)(d>>16));
    ok &= eeWr(a+2, (uint8_t)(d>>8)); ok &= eeWr(a+3, (uint8_t)d);
    #if FEATURE_WATCHDOG
    wdtF();
    #endif
    return ok;
}

uint32_t eeR32(uint8_t a) {
    return ((uint32_t)eeRd(a)<<24)|((uint32_t)eeRd(a+1)<<16)|
           ((uint32_t)eeRd(a+2)<<8)|(uint32_t)eeRd(a+3);
}

uint8_t eeCRC(void) {
    uint8_t x = 0;
    for (uint8_t i = EE_ADDR_MAGIC_H; i < EE_ADDR_CHECKSUM; i++) x ^= eeRd(i);
    return x;
}

bool eeSave(void) {
    uint8_t fl = 0; bool ok = true;
    if (!eeUlk()) return false;
    ok &= eeWr(EE_ADDR_MAGIC_H, (uint8_t)(EE_MAGIC_VALUE>>8));
    ok &= eeWr(EE_ADDR_MAGIC_L, (uint8_t)EE_MAGIC_VALUE);
    ok &= eeWr(EE_ADDR_VERSION, EE_CONFIG_VERSION);
    if (g_cal) fl |= EE_FLAG_CALIBRATED;
    if (g_pos >= 0) fl |= EE_FLAG_VALID_POS;
    ok &= eeWr(EE_ADDR_FLAGS, fl);
    ok &= eeW32(EE_ADDR_TRAVEL_B3, g_trvl);
    ok &= eeWr(EE_ADDR_POSITION, (g_pos >= 0) ? g_pos : 0xFF);
    ok &= eeW32(EE_ADDR_CYCLES_B3, g_cycl);
    ok &= eeWr(EE_ADDR_ERRORS, g_errcnt);
    ok &= eeWr(EE_ADDR_CHECKSUM, eeCRC());
    ok &= eeWr(EE_ADDR_DEVICE_ID, g_devId);
    eeLk();
    #if FEATURE_WATCHDOG
    wdtF();
    #endif
    return ok;
}

bool eeLoad(void) {
    uint16_t m = ((uint16_t)eeRd(EE_ADDR_MAGIC_H)<<8)|eeRd(EE_ADDR_MAGIC_L);
    if (m != EE_MAGIC_VALUE) return false;
    uint8_t v = eeRd(EE_ADDR_VERSION);
    if (v != EE_CONFIG_VERSION && v != 0x42 && v != 0x43) { eeClr(); return false; }
    if (eeRd(EE_ADDR_CHECKSUM) != eeCRC()) { txE(E_EE); return false; }
    uint8_t fl = eeRd(EE_ADDR_FLAGS);
    g_trvl = eeR32(EE_ADDR_TRAVEL_B3);
    g_cal = (fl & EE_FLAG_CALIBRATED) && g_trvl >= MOTOR_MIN_TRAVEL_MS && g_trvl <= MOTOR_MAX_TRAVEL_MS;
    uint8_t p = eeRd(EE_ADDR_POSITION);
    g_pos = (p <= 100 && (fl & EE_FLAG_VALID_POS)) ? (int8_t)p : -1;
    g_cycl = eeR32(EE_ADDR_CYCLES_B3);
    g_errcnt = eeRd(EE_ADDR_ERRORS);
    uint8_t did = eeRd(EE_ADDR_DEVICE_ID);
    if (did > 0 && did < DEVICE_ID_BROADCAST) g_devId = did;
    return true;
}

void eeClr(void) {
    if (!eeUlk()) return;
    for (uint8_t i = 0; i < 20; i++) { eeWr(i, 0xFF); wdtF(); }
    eeLk();
    g_cal = false; g_trvl = 0; g_pos = -1; g_cycl = 0; g_errcnt = 0;
    txOK();
}

#endif

/* =============================================================================
 * UART
 * ============================================================================= */

void txS(const char* s) { Serial_print_s(s); Serial_print_s("\r\n"); }

void txN(int32_t n) {
    char b[7]; char* p = b+6; bool neg = (n<0);
    *p = '\0';
    if (neg) n = -n;
    if (n == 0) *--p = '0';
    else while (n > 0 && p > b) { *--p = '0'+(n%10); n /= 10; }
    if (neg && p > b) *--p = '-';
    Serial_print_s(p);
}

void txOK(void) { Serial_print_s("OK\r\n"); }

void txE(uint8_t e) {
    Serial_print_s("E"); Serial_print_s(eStr(e)); Serial_print_s("\r\n");
}

const char* eStr(uint8_t e) {
    switch (e) {
        case E_STALL: return "STL";
        case E_TMO:   return "TMO";
        case E_LIM:   return "LIM";
        case E_CAL:   return "CAL";
        case E_EE:    return "EE";
        case E_CMD:   return "CMD";
        default:      return "?";
    }
}

/* =============================================================================
 * COMMAND PARSING & DISPATCH
 * ============================================================================= */

#if FEATURE_UART_COMMANDS

void rxProc(void) {
    while (Serial_available() > 0) {
        char c = Serial_read();
        idleRst();
        if (c == '\n' || c == '\r') {
            if (g_ci > 0) { g_cmd[g_ci] = '\0'; rxExec(g_cmd); g_ci = 0; }
        }
        else if (c < 0x20 || c > 0x7E) { /* noise filter */ }
        else if (g_ci < CMD_BUFFER_LEN) {
            if (c >= 'A' && c <= 'Z') c += 32;
            g_cmd[g_ci++] = c;
        }
        else { g_ci = 0; txE(E_CMD); }
    }
}

/* Compact parser: length + first two chars */
uint8_t rxParse(const char* s) {
    uint8_t len = 0;
    while (s[len]) len++;
    if (len < 4) return C_INV;

    uint16_t h = ((uint16_t)s[0]<<8)|s[1];
    switch (len) {
        case 4:
            if (h==0x6F70) return C_OPEN;  /* open */
            if (h==0x6861) return C_HALF;  /* half */
            if (h==0x7361) return C_SAVE;  /* save */
            if (h==0x6C6F) return C_LOAD;  /* load */
            if (h==0x7374) return s[2]=='a' ? C_STAT : (s[2]=='o' ? C_STOP : C_INV);
            if (h==0x6361) return C_CAL;   /* calb */
            if (h==0x6865) return C_HELP;  /* help */
            if (h==0x696E) return C_STAT;  /* info */
            break;
        case 5:
            if (h==0x636C) return s[2]=='o' ? C_CLOSE : (s[2]=='e' ? C_CLR : C_INV);
            if (h==0x736C) return C_SLP;   /* sleep */
            if (h==0x7265) return C_RST;   /* reset */
            if (h==0x6465) return C_DEVID; /* devid */
            break;
        case 6:
            if (h==0x7374) return C_STAT;  /* status */
            break;
        case 7:
            if (h==0x7175) return C_QTR;   /* quarter */
            break;
        case 9:
            if (h==0x6361) return C_CAL;   /* calibrate */
            break;
        case 12:
            if (h==0x7468) return C_3QTR;  /* thirdquarter */
            break;
    }
    return C_INV;
}

void rxExec(const char* s) {
    const char* cs = s;

    #if FEATURE_UART_ADDRESSING
    if (s[0] == '@') {
        uint8_t tid = 0, i = 1;
        while (s[i] >= '0' && s[i] <= '9') { tid = tid*10+(s[i]-'0'); i++; }
        if (s[i] != ':' || (tid != g_devId && tid != DEVICE_ID_BROADCAST)) return;
        cs = &s[i+1];
    }
    #endif

    if (cs[0] == 's') { uint8_t c = rxParse(cs+1); if (c != C_INV) { cmdDo(c); return; } }

    uint8_t c = rxParse(cs);

    /* devid: show or set device ID */
    if (c == C_DEVID) {
        uint8_t j = 5, nid = 0;
        while (cs[j] == ' ') j++;
        if (cs[j] >= '0' && cs[j] <= '9') {
            while (cs[j] >= '0' && cs[j] <= '9') { nid = nid*10+(cs[j]-'0'); j++; }
            if (nid > 0 && nid < 255) { g_devId = nid;
                #if FEATURE_EEPROM
                eeSave();
                #endif
            }
        }
        txN(g_devId); Serial_print_s("\r\n"); return;
    }
    cmdDo(c);
}

#endif /* FEATURE_UART_COMMANDS */

/* Shared command dispatch */
void cmdDo(uint8_t c) {
    switch (c) {
        case C_OPEN:  motPos(100); break;
        case C_CLOSE: motPos(0);   break;
        case C_QTR:   motPos(25);  break;
        case C_HALF:  motPos(50);  break;
        case C_3QTR:  motPos(75);  break;
        case C_CAL:   calRun();    break;
        case C_STAT:
            txN(g_devId); Serial_print_s(" ");
            if (g_pos >= 0) txN(g_pos); else Serial_print_s("?");
            Serial_print_s(g_cal ? " C " : " U ");
            txN(g_trvl); Serial_print_s(" ");
            txN((int32_t)g_err); Serial_print_s(" ");
            txN(g_cycl); Serial_print_s("\r\n");
            break;
        case C_VER: txS("v" FW_VERSION_STRING); break;
        case C_POS:
            if (g_pos >= 0) txN(g_pos); else Serial_print_s("?");
            Serial_print_s("\r\n"); break;
        case C_HELP: txS("? see docs"); break;
        case C_SAVE:
            #if FEATURE_EEPROM
            if (eeSave()) txOK(); else txE(E_EE);
            #else
            txE(E_EE);
            #endif
            break;
        case C_LOAD:
            #if FEATURE_EEPROM
            if (eeLoad()) txOK(); else txE(E_EE);
            #endif
            break;
        case C_CLR:
            #if FEATURE_EEPROM
            eeClr();
            #else
            g_cal = false; g_trvl = 0; g_pos = -1; g_cycl = 0; g_errcnt = 0; txOK();
            #endif
            break;
        case C_SLP:  stSet(ST_SLEEP); break;
        case C_WAKE: pwrWk(); break;
        case C_RST:
#ifdef __SDCC__
            WWDG_CR = 0x80; while(1){}
#else
            stm32_sw_reset();
#endif
            break;
        case C_STOP:
            motOff(); g_estop = true; txOK(); stSet(ST_IDLE); break;
        case C_INV: default:
            txE(E_CMD); break;
    }
}

/* =============================================================================
 * BUTTONS (merged)
 * ============================================================================= */

void btnChk(void) {
    static uint32_t lc = 0;
    static bool lu = false, ld = false;
    if ((millis()-lc) < MOTOR_DEBOUNCE_MS) return;
    lc = millis();
    bool u = btnU(), d = btnD();
    if (u && !lu && !limCl()) btnDir(M_CLOSE);
    if (d && !ld && !limOp()) btnDir(M_OPEN);
    lu = u; ld = d;
}

void btnDir(uint8_t d) {
    uint32_t bs = millis();
    idleRst(); ledS(L_FAST); motSet(d);
    while (1) {
        if (d == M_CLOSE && !btnU()) break;
        if (d == M_OPEN && !btnD()) break;
        if (d == M_CLOSE && limCl()) break;
        if (d == M_OPEN && limOp()) break;
        #if FEATURE_WATCHDOG
        wdtF();
        #endif
        #if FEATURE_LED
        ledU();
        #endif
        if ((millis()-bs) > MOTOR_MAX_TRAVEL_MS) break;
    }
    motOff();
    if (d == M_CLOSE && limCl()) posUpd(0);
    else if (d == M_OPEN && limOp()) posUpd(100);
    else g_pos = -1;
}

/* =============================================================================
 * POWER MANAGEMENT
 * ============================================================================= */

#if FEATURE_SLEEP

void pwrChk(void) {
    uint32_t id = millis() - g_actMs;
    if (id > HIBERNATE_TIMEOUT_MS) stSet(ST_HIBER);
    else if (id > IDLE_TIMEOUT_MS) stSet(ST_SLEEP);
}

void pwrSlp(void) {
    dlyW(50);
    #if FEATURE_LED
    digitalWrite(PIN_LED, LOW);
    #endif
    while (!Serial_available() && !btnU() && !btnD()) {
        #if FEATURE_WATCHDOG
        wdtF();
        #endif
        delay(50);
        if ((millis()-g_actMs) > HIBERNATE_TIMEOUT_MS) { stSet(ST_HIBER); return; }
    }
    pwrWk();
}

void pwrHib(void) {
    #if FEATURE_EEPROM
    if (g_svn) { eeSave(); g_svn = false; }
    #endif
    dlyW(50);
    #if FEATURE_LED
    digitalWrite(PIN_LED, LOW);
    #endif
    while (!Serial_available() && !btnU() && !btnD()) {
        #if FEATURE_WATCHDOG
        wdtF();
        #endif
        delay(200);
    }
    pwrWk();
}

void pwrWk(void) {
    idleRst(); Serial_begin(UART_BAUD); ledS(L_SLOW); stSet(ST_IDLE);
}

#else
void pwrWk(void) { idleRst(); stSet(ST_IDLE); }
#endif

/* =============================================================================
 * WATCHDOG
 * ============================================================================= */

#if FEATURE_WATCHDOG
void wdtI(void) {
#ifdef __SDCC__
    IWDG_KR = 0xCC; IWDG_KR = 0x55;
    IWDG_PR = 0x06; IWDG_RLR = 0xFF;
    IWDG_KR = 0xAA;
#else
    stm32_wdt_init();
#endif
    g_wdt = true;
}
void wdtF(void) {
    if (!g_wdt) return;
#ifdef __SDCC__
    IWDG_KR = 0xAA;
#else
    stm32_wdt_kick();
#endif
}
#else
void wdtI(void) {}
void wdtF(void) {}
#endif

/* =============================================================================
 * LED
 * ============================================================================= */

#if FEATURE_LED

void ledS(uint8_t m) { if (m != g_lm) g_sos = 0; g_lm = m; }

void ledU(void) {
    uint32_t now = millis(), iv;
    switch (g_lm) {
        case L_OFF: digitalWrite(PIN_LED, LOW); break;
        case L_ON:  digitalWrite(PIN_LED, HIGH); break;
        case L_SLOW: case L_FAST:
            iv = (g_lm == L_SLOW) ? 500 : 125;
            if ((now-g_ledMs) >= iv) { g_ls = !g_ls; digitalWrite(PIN_LED, g_ls); g_ledMs = now; }
            break;
        case L_SOS:
            { bool isL = (g_sos >= 6 && g_sos < 12 && !(g_sos & 1));
              iv = (g_sos >= 18) ? 800 : (isL ? 400 : 150); }
            if ((now-g_ledMs) >= iv) {
                g_ledMs = now;
                if (g_sos >= 19) { g_sos = 0; digitalWrite(PIN_LED, LOW); }
                else { digitalWrite(PIN_LED, !(g_sos & 1)); g_sos++; }
            }
            break;
    }
}

#endif

/* =============================================================================
 * ERROR HANDLING
 * ============================================================================= */

void errS(uint8_t e) {
    g_err = e; if (g_errcnt < 255) g_errcnt++;
    motOff(); txE(e);
    if (e != E_OK && e != E_CMD) stSet(ST_FAULT);
}

void errC(void) { g_err = E_OK; if (g_st == ST_FAULT) stSet(ST_IDLE); }

void errR(void) { g_men = true; errC(); g_pos = -1; stSet(ST_IDLE); }

/* Utility */
void idleRst(void) { g_actMs = millis(); }

void dlyW(uint32_t ms) {
    uint32_t s = millis();
    while ((millis()-s) < ms) { wdtF(); }
}

/* =============================================================================
 * I2C SLAVE (v4.4) - Hardware I2C on PB4/PB5
 * ============================================================================= */

#if FEATURE_I2C

void i2cI(void) {
    uint8_t addr = I2C_BASE_ADDR + g_devId;
    I2C_CR1_REG = 0;
    I2C_FREQR_REG = 16;
    I2C_OARL_REG = (addr << 1);
    I2C_OARH_REG = 0x40;
    I2C_CCRL_REG = 80;
    I2C_CCRH_REG = 0;
    I2C_TRISER_REG = 17;
    I2C_CR2_REG = I2C_CR2_ACK;
    I2C_ITR_REG = I2C_ITR_ITEVTEN | I2C_ITR_ITBUFEN | I2C_ITR_ITERREN;
    I2C_CR1_REG = I2C_CR1_PE;
    g_i2p = 0; g_i2f = true; g_i2c = 0;
    i2cU();
}

void i2cU(void) {
    uint8_t fl = 0;
    if (g_cal) fl |= I2C_FLAG_CALIBRATED;
    if (g_mst == MS_RUN) fl |= I2C_FLAG_MOVING;
    if (g_st == ST_FAULT) fl |= I2C_FLAG_FAULT;
    if (g_st == ST_SLEEP || g_st == ST_HIBER) fl |= I2C_FLAG_SLEEPING;

    g_i2r[0] = g_devId;
    g_i2r[1] = 0;
    g_i2r[2] = (g_pos >= 0) ? (uint8_t)g_pos : 0xFF;
    g_i2r[3] = fl;
    g_i2r[4] = g_err;
    g_i2r[5] = (uint8_t)(g_trvl >> 8);
    g_i2r[6] = (uint8_t)(g_trvl & 0xFF);
    g_i2r[7] = (FW_VERSION_MAJOR << 4) | FW_VERSION_MINOR;
}

INTERRUPT_HANDLER(I2C_IRQHandler, 19) {
    uint8_t sr1 = I2C_SR1_REG;
    uint8_t sr2 = I2C_SR2_REG;

    if (sr2 & (I2C_SR2_BERR|I2C_SR2_ARLO|I2C_SR2_AF|I2C_SR2_OVR)) {
        I2C_SR2_REG = 0; g_i2f = true; return;
    }

    if (sr1 & I2C_SR1_ADDR) {
        uint8_t sr3 = I2C_SR3_REG;
        g_i2f = true;
        if (sr3 & I2C_SR3_TRA) {
            I2C_DR_REG = g_i2r[g_i2p & 0x07];
            g_i2p = (g_i2p + 1) & 0x07;
        }
        return;
    }

    if (sr1 & I2C_SR1_TXE) {
        I2C_DR_REG = g_i2r[g_i2p & 0x07];
        g_i2p = (g_i2p + 1) & 0x07;
        return;
    }

    if (sr1 & I2C_SR1_RXNE) {
        uint8_t d = I2C_DR_REG;
        if (g_i2f) { g_i2p = d & 0x07; g_i2f = false; }
        else {
            uint8_t r = g_i2p & 0x07;
            if (r == I2C_REG_DEV_ID && d > 0 && d < DEVICE_ID_BROADCAST) {
                g_devId = d; g_i2r[0] = d;
            }
            else if (r == I2C_REG_CMD) { g_i2c = d; }
            g_i2p = (g_i2p + 1) & 0x07;
        }
        return;
    }

    if (sr1 & I2C_SR1_STOPF) {
        I2C_CR2_REG |= I2C_CR2_ACK;
        g_i2f = true;
        return;
    }
}

#endif /* FEATURE_I2C */
