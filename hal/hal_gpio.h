/* ANSA HAL API v0.1 — hal_gpio.h — pure interface, no vendor types. */
#ifndef ANSA_HAL_GPIO_H
#define ANSA_HAL_GPIO_H

#include "hal_status.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_gpio_pin hal_gpio_pin_t; /* opaque, backend-defined */

typedef enum {
    HAL_GPIO_MODE_INPUT,
    HAL_GPIO_MODE_OUTPUT,
    HAL_GPIO_MODE_ANALOG,
    HAL_GPIO_MODE_ALT_FUNCTION
} hal_gpio_mode_t;

typedef void (*hal_gpio_irq_cb_t)(hal_gpio_pin_t *pin, void *ctx);

hal_status_t hal_gpio_init(hal_gpio_pin_t *pin, hal_gpio_mode_t mode);
hal_status_t hal_gpio_write(hal_gpio_pin_t *pin, bool level);
hal_status_t hal_gpio_read(hal_gpio_pin_t *pin, bool *level_out);
hal_status_t hal_gpio_toggle(hal_gpio_pin_t *pin);
hal_status_t hal_gpio_set_mode(hal_gpio_pin_t *pin, hal_gpio_mode_t mode);
hal_status_t hal_gpio_set_interrupt(hal_gpio_pin_t *pin, hal_gpio_irq_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif
#endif /* ANSA_HAL_GPIO_H */
