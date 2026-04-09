/**
 * stm32_compat.h - STM32 compatibility shim for STM8 Arduino code
 * Provides C-callable wrappers for Serial, EEPROM, Watchdog on STM32.
 * Ignored on STM8 (__SDCC__ defined).
 */
#ifndef STM32_COMPAT_H
#define STM32_COMPAT_H

#ifndef __SDCC__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Serial */
void     Serial_begin(unsigned long baud);
int      Serial_available(void);
char     Serial_read(void);
void     Serial_print_s(const char *s);

/* EEPROM (emulated via flash) */
uint8_t  stm32_ee_read(uint8_t addr);
bool     stm32_ee_write(uint8_t addr, uint8_t data);

/* Watchdog */
void     stm32_wdt_init(void);
void     stm32_wdt_kick(void);

/* Reset */
void     stm32_sw_reset(void);
uint8_t  stm32_rst_reason(void);   /* bit 0x20 = watchdog reset (same as STM8 RST_SR bit) */

#ifdef __cplusplus
}
#endif

#endif /* !__SDCC__ */
#endif /* STM32_COMPAT_H */
