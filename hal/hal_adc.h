/* ANSA HAL API v0.1 — hal_adc.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_ADC_H
#define ANSA_HAL_ADC_H

#include "hal_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_adc_channel hal_adc_channel_t; /* opaque, backend-defined */

typedef void (*hal_adc_cb_t)(hal_adc_channel_t *channel, uint16_t raw_value, void *ctx);

hal_status_t hal_adc_init(hal_adc_channel_t *channel, uint8_t resolution_bits);
hal_status_t hal_adc_read(hal_adc_channel_t *channel, uint16_t *raw_value_out);
hal_status_t hal_adc_start_continuous(hal_adc_channel_t *channel, uint32_t sample_rate_hz);
hal_status_t hal_adc_register_callback(hal_adc_channel_t *channel, hal_adc_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_ADC_H */
