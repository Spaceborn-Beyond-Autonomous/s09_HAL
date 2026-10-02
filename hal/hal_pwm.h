/* ANSA HAL API v0.1 — hal_pwm.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_PWM_H
#define ANSA_HAL_PWM_H

#include "hal_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_pwm hal_pwm_t; /* opaque, backend-defined */

hal_status_t hal_pwm_init(hal_pwm_t *pwm, uint32_t frequency_hz);
hal_status_t hal_pwm_set_duty(hal_pwm_t *pwm, float duty_percent);
hal_status_t hal_pwm_start(hal_pwm_t *pwm);
hal_status_t hal_pwm_stop(hal_pwm_t *pwm);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_PWM_H */
