/**
 * stm32_compat.cpp - STM32 C++ implementations of STM8-style C API
 * Only compiled when NOT using SDCC (i.e. on STM32 with GCC ARM).
 */
#ifndef __SDCC__

#include <Arduino.h>
#include <EEPROM.h>
#include <IWatchdog.h>

extern "C" {

/* ---------- Serial ---------- */
void Serial_begin(unsigned long baud) {
    Serial.begin(baud);
}

int Serial_available(void) {
    return Serial.available();
}

char Serial_read(void) {
    return (char)Serial.read();
}

void Serial_print_s(const char *s) {
    Serial.print(s);
}

/* ---------- EEPROM (emulated) ---------- */
uint8_t stm32_ee_read(uint8_t addr) {
    return EEPROM.read(addr);
}

bool stm32_ee_write(uint8_t addr, uint8_t data) {
    EEPROM.update(addr, data);  /* only writes if value changed */
    return true;
}

/* ---------- Watchdog ---------- */
static bool s_wdt_active = false;

void stm32_wdt_init(void) {
    IWatchdog.begin(2000000);   /* 2 second timeout in microseconds */
    s_wdt_active = true;
}

void stm32_wdt_kick(void) {
    if (s_wdt_active) IWatchdog.reload();
}

/* ---------- Reset ---------- */
void stm32_sw_reset(void) {
    NVIC_SystemReset();
}

uint8_t stm32_rst_reason(void) {
    uint8_t reason = 0;
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) ||
        __HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
        reason |= 0x20;     /* match STM8 RST_SR watchdog bit */
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return reason;
}

} /* extern "C" */

#endif /* !__SDCC__ */
