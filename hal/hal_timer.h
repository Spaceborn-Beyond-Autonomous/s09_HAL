/* ANSA HAL API v0.1 — hal_timer.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_TIMER_H
#define ANSA_HAL_TIMER_H

#include "hal_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_timer hal_timer_t; /* opaque, backend-defined */

typedef void (*hal_timer_cb_t)(hal_timer_t *timer, void *ctx);

hal_status_t hal_timer_init(hal_timer_t *timer, uint32_t period_us);
hal_status_t hal_timer_start(hal_timer_t *timer);
hal_status_t hal_timer_stop(hal_timer_t *timer);
hal_status_t hal_timer_set_period(hal_timer_t *timer, uint32_t period_us);
hal_status_t hal_timer_register_callback(hal_timer_t *timer, hal_timer_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_TIMER_H */
