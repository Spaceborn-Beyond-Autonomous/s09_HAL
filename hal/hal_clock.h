/* ANSA HAL API v0.1 — hal_clock.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_CLOCK_H
#define ANSA_HAL_CLOCK_H

#include "hal_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

hal_status_t hal_clock_init(void);
uint64_t     hal_clock_get_tick_ms(void);
uint64_t     hal_clock_get_tick_us(void);
void         hal_clock_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_CLOCK_H */
