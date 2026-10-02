/* ANSA HAL API v0.1 — hal_spi.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_SPI_H
#define ANSA_HAL_SPI_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_spi hal_spi_t; /* opaque, backend-defined */

typedef struct {
    uint32_t clock_hz;
    uint8_t  mode;      /* CPOL/CPHA combination 0-3 */
    bool     msb_first;
} hal_spi_config_t;

typedef void (*hal_spi_transfer_done_cb_t)(hal_spi_t *spi, hal_status_t result, void *ctx);

hal_status_t hal_spi_init(hal_spi_t *spi, const hal_spi_config_t *config);
hal_status_t hal_spi_transfer(hal_spi_t *spi, const uint8_t *tx, uint8_t *rx, size_t len);
hal_status_t hal_spi_transfer_async(hal_spi_t *spi, const uint8_t *tx, uint8_t *rx, size_t len,
                                     hal_spi_transfer_done_cb_t cb, void *ctx);
hal_status_t hal_spi_set_cs(hal_spi_t *spi, bool asserted);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_SPI_H */
