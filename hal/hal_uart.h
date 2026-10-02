/* ANSA HAL API v0.1 — hal_uart.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_UART_H
#define ANSA_HAL_UART_H

#include "hal_status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_uart hal_uart_t; /* opaque, backend-defined */

typedef struct {
    uint32_t baud_rate;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    uint8_t  parity; /* 0=none, 1=even, 2=odd */
} hal_uart_config_t;

typedef void (*hal_uart_rx_cb_t)(hal_uart_t *uart, uint8_t byte, void *ctx);
typedef void (*hal_uart_tx_done_cb_t)(hal_uart_t *uart, hal_status_t result, void *ctx);

hal_status_t hal_uart_init(hal_uart_t *uart, const hal_uart_config_t *config);
hal_status_t hal_uart_write(hal_uart_t *uart, const uint8_t *data, size_t len);
hal_status_t hal_uart_read(hal_uart_t *uart, uint8_t *data_out, size_t len, size_t *bytes_read_out);
hal_status_t hal_uart_write_async(hal_uart_t *uart, const uint8_t *data, size_t len,
                                   hal_uart_tx_done_cb_t cb, void *ctx);
hal_status_t hal_uart_register_rx_callback(hal_uart_t *uart, hal_uart_rx_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_UART_H */
