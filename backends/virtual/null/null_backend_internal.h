/*
 * Internal to the null backend. Exposes the concrete layout of each opaque
 * HAL handle so that test/board-config code linked against this specific
 * backend can allocate storage for them (e.g. as static instances).
 *
 * Application code and anything above the HAL must NEVER include this file
 * — that would violate ADR-001 (CPU/backend identity invisible above HAL).
 * It exists only for tests/unit/test_null_backend.c and, in a real backend,
 * for that backend's own board-config allocation code.
 */
#ifndef ANSA_NULL_BACKEND_INTERNAL_H
#define ANSA_NULL_BACKEND_INTERNAL_H

#include "hal_gpio.h"
#include "hal_uart.h"
#include "hal_spi.h"
#include "hal_i2c.h"
#include "hal_timer.h"
#include "hal_pwm.h"
#include "hal_can.h"
#include "hal_dma.h"
#include "hal_adc.h"
#include "hal_sensor.h"

struct hal_gpio_pin        { int unused; };
struct hal_uart             { int unused; };
struct hal_spi               { int unused; };
struct hal_i2c               { int unused; };
struct hal_timer             { int unused; };
struct hal_pwm                { int unused; };
struct hal_can                 { int unused; };
struct hal_dma_channel          { int unused; };
struct hal_adc_channel           { int unused; };
struct hal_sensor                 { int unused; };

#endif /* ANSA_NULL_BACKEND_INTERNAL_H */
