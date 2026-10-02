/* ANSA HAL API v0.1 — hal_can.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_CAN_H
#define ANSA_HAL_CAN_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_can hal_can_t; /* opaque, backend-defined */

typedef struct {
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
    bool     is_extended_id;
} hal_can_frame_t;

typedef struct {
    uint32_t id_filter;
    uint32_t id_mask;
} hal_can_filter_t;

hal_status_t hal_can_init(hal_can_t *can, uint32_t bitrate);
hal_status_t hal_can_send(hal_can_t *can, const hal_can_frame_t *frame);
hal_status_t hal_can_receive(hal_can_t *can, hal_can_frame_t *frame_out);
hal_status_t hal_can_set_filter(hal_can_t *can, const hal_can_filter_t *filter);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_CAN_H */
