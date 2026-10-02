/* ANSA HAL API v0.1 — hal_dma.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_DMA_H
#define ANSA_HAL_DMA_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_dma_channel hal_dma_channel_t; /* opaque, backend-defined */

typedef enum {
    HAL_DMA_DIR_MEM_TO_MEM,
    HAL_DMA_DIR_MEM_TO_PERIPH,
    HAL_DMA_DIR_PERIPH_TO_MEM
} hal_dma_direction_t;

typedef struct {
    void                *src;
    void                *dst;
    size_t               length;
    hal_dma_direction_t  direction;
} hal_dma_config_t;

hal_status_t hal_dma_configure(hal_dma_channel_t *channel, const hal_dma_config_t *config);
hal_status_t hal_dma_start(hal_dma_channel_t *channel);
hal_status_t hal_dma_stop(hal_dma_channel_t *channel);
hal_status_t hal_dma_get_status(hal_dma_channel_t *channel, hal_status_t *status_out);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_DMA_H */
