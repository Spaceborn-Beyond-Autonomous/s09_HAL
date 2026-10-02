#ifndef ANSA_VIRTUAL_BACKEND_H
#define ANSA_VIRTUAL_BACKEND_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
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

typedef struct { bool level; hal_gpio_mode_t mode; hal_gpio_irq_cb_t cb; void *ctx; } avhp_virtual_gpio_t;
typedef struct { hal_uart_config_t cfg; uint8_t rx[512]; size_t head,tail; hal_uart_rx_cb_t rx_cb; void *rx_ctx; } avhp_virtual_uart_t;
typedef struct { hal_spi_config_t cfg; bool cs; uint8_t loopback_xor; } avhp_virtual_spi_t;
typedef struct { hal_i2c_config_t cfg; uint8_t memory[128][256]; } avhp_virtual_i2c_t;
typedef struct { uint32_t period_us; bool running; hal_timer_cb_t cb; void *ctx; } avhp_virtual_timer_t;
typedef struct { uint32_t frequency_hz; float duty_percent; bool running; } avhp_virtual_pwm_t;
typedef struct { uint32_t bitrate; hal_can_filter_t filter; bool filter_set; hal_can_frame_t queue[16]; size_t head,tail; } avhp_virtual_can_t;
typedef struct { hal_dma_config_t cfg; bool configured,running; hal_status_t status; } avhp_virtual_dma_t;
typedef struct { uint8_t resolution_bits; uint16_t value; uint32_t sample_rate_hz; hal_adc_cb_t cb; void *ctx; } avhp_virtual_adc_t;
typedef struct { uint8_t data[64]; size_t len; hal_sensor_health_t health; hal_sensor_cb_t cb; void *ctx; } avhp_virtual_sensor_t;

void avhp_virtual_gpio_trigger(avhp_virtual_gpio_t *gpio,bool level);
void avhp_virtual_uart_feed(avhp_virtual_uart_t *uart,const uint8_t *data,size_t len);
void avhp_virtual_timer_fire(avhp_virtual_timer_t *timer);
void avhp_virtual_adc_set(avhp_virtual_adc_t *adc,uint16_t value);
#endif
