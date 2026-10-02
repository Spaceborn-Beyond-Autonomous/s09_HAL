/* ANSA HAL API v0.1 — hal_i2c.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_I2C_H
#define ANSA_HAL_I2C_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_i2c hal_i2c_t; /* opaque, backend-defined */

typedef struct {
    uint32_t clock_hz;
} hal_i2c_config_t;

hal_status_t hal_i2c_init(hal_i2c_t *i2c, const hal_i2c_config_t *config);
hal_status_t hal_i2c_write(hal_i2c_t *i2c, uint8_t device_addr, const uint8_t *data, size_t len);
hal_status_t hal_i2c_read(hal_i2c_t *i2c, uint8_t device_addr, uint8_t *data_out, size_t len);
hal_status_t hal_i2c_mem_write(hal_i2c_t *i2c, uint8_t device_addr, uint16_t mem_addr,
                                const uint8_t *data, size_t len);
hal_status_t hal_i2c_mem_read(hal_i2c_t *i2c, uint8_t device_addr, uint16_t mem_addr,
                               uint8_t *data_out, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_I2C_H */
