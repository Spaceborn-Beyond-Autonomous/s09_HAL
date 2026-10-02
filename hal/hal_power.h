/* ANSA HAL API v0.1 — hal_power.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_POWER_H
#define ANSA_HAL_POWER_H

#include "hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_POWER_MODE_ACTIVE,
    HAL_POWER_MODE_LOW_POWER,
    HAL_POWER_MODE_STANDBY
} hal_power_mode_t;

hal_status_t hal_power_get_battery_voltage(float *volts_out);
hal_status_t hal_power_get_current(float *amps_out);
hal_status_t hal_power_set_low_power_mode(hal_power_mode_t mode);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_POWER_H */
