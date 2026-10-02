/* ANSA HAL API v0.1 — hal_flash.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_FLASH_H
#define ANSA_HAL_FLASH_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

hal_status_t hal_flash_read(uint32_t address, uint8_t *data_out, size_t len);
hal_status_t hal_flash_write(uint32_t address, const uint8_t *data, size_t len);
hal_status_t hal_flash_erase_sector(uint32_t sector_index);
hal_status_t hal_flash_get_status(hal_status_t *status_out);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_FLASH_H */
