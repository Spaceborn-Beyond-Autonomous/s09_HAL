/* ANSA HAL API v0.1 — hal_sensor.h — generic sensor interface used by the
 * Virtual Sensor API (Stage 5). Kept minimal and hardware-agnostic here;
 * concrete sensor types (GPS, IMU, ...) live in sensors/. */
#ifndef ANSA_HAL_SENSOR_H
#define ANSA_HAL_SENSOR_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_sensor hal_sensor_t; /* opaque, backend-defined */

typedef enum {
    HAL_SENSOR_HEALTH_OK,
    HAL_SENSOR_HEALTH_DEGRADED,
    HAL_SENSOR_HEALTH_STALE,
    HAL_SENSOR_HEALTH_FAILED
} hal_sensor_health_t;

typedef void (*hal_sensor_cb_t)(hal_sensor_t *sensor, const void *data, size_t len, void *ctx);

hal_status_t hal_sensor_init(hal_sensor_t *sensor);
hal_status_t hal_sensor_read(hal_sensor_t *sensor, void *data_out, size_t len);
hal_status_t hal_sensor_subscribe(hal_sensor_t *sensor, hal_sensor_cb_t cb, void *ctx);
hal_status_t hal_sensor_get_health_status(hal_sensor_t *sensor, hal_sensor_health_t *health_out);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_SENSOR_H */
