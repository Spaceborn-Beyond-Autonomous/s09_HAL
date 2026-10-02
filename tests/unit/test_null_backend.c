/*
 * Stage 1 exit-criteria test: prove every HAL header links against the
 * null backend and every call returns HAL_NOT_IMPLEMENTED (or a sane
 * placeholder for void-returning clock functions). This is NOT behavioral
 * testing — behavioral compliance testing happens in tests/hal_compliance/
 * starting Stage 3, once a real backend (Virtual) exists.
 */
#include "hal_flash.h"
#include "hal_clock.h"
#include "hal_power.h"
#include "null_backend_internal.h" /* opaque struct layouts, test-only — see file header note */

#include <stdio.h>
#include <stdlib.h>

static int g_failures = 0;

#define CHECK_NOT_IMPLEMENTED(expr) do { \
    hal_status_t _r = (expr); \
    if (_r != HAL_NOT_IMPLEMENTED) { \
        fprintf(stderr, "FAIL %s:%d: %s returned %d, expected HAL_NOT_IMPLEMENTED\n", \
                __FILE__, __LINE__, #expr, (int)_r); \
        g_failures++; \
    } \
} while (0)

int main(void)
{
    hal_gpio_pin_t gpio; bool level = false;
    CHECK_NOT_IMPLEMENTED(hal_gpio_init(&gpio, HAL_GPIO_MODE_OUTPUT));
    CHECK_NOT_IMPLEMENTED(hal_gpio_write(&gpio, true));
    CHECK_NOT_IMPLEMENTED(hal_gpio_read(&gpio, &level));
    CHECK_NOT_IMPLEMENTED(hal_gpio_toggle(&gpio));
    CHECK_NOT_IMPLEMENTED(hal_gpio_set_mode(&gpio, HAL_GPIO_MODE_INPUT));
    CHECK_NOT_IMPLEMENTED(hal_gpio_set_interrupt(&gpio, NULL, NULL));

    hal_uart_t uart; hal_uart_config_t ucfg = {115200, 8, 1, 0};
    uint8_t byte = 0; size_t br = 0;
    CHECK_NOT_IMPLEMENTED(hal_uart_init(&uart, &ucfg));
    CHECK_NOT_IMPLEMENTED(hal_uart_write(&uart, &byte, 1));
    CHECK_NOT_IMPLEMENTED(hal_uart_read(&uart, &byte, 1, &br));
    CHECK_NOT_IMPLEMENTED(hal_uart_write_async(&uart, &byte, 1, NULL, NULL));
    CHECK_NOT_IMPLEMENTED(hal_uart_register_rx_callback(&uart, NULL, NULL));

    hal_spi_t spi; hal_spi_config_t scfg = {1000000, 0, true};
    CHECK_NOT_IMPLEMENTED(hal_spi_init(&spi, &scfg));
    CHECK_NOT_IMPLEMENTED(hal_spi_transfer(&spi, &byte, &byte, 1));
    CHECK_NOT_IMPLEMENTED(hal_spi_transfer_async(&spi, &byte, &byte, 1, NULL, NULL));
    CHECK_NOT_IMPLEMENTED(hal_spi_set_cs(&spi, true));

    hal_i2c_t i2c; hal_i2c_config_t icfg = {400000};
    CHECK_NOT_IMPLEMENTED(hal_i2c_init(&i2c, &icfg));
    CHECK_NOT_IMPLEMENTED(hal_i2c_write(&i2c, 0x42, &byte, 1));
    CHECK_NOT_IMPLEMENTED(hal_i2c_read(&i2c, 0x42, &byte, 1));
    CHECK_NOT_IMPLEMENTED(hal_i2c_mem_write(&i2c, 0x42, 0x00, &byte, 1));
    CHECK_NOT_IMPLEMENTED(hal_i2c_mem_read(&i2c, 0x42, 0x00, &byte, 1));

    hal_timer_t timer;
    CHECK_NOT_IMPLEMENTED(hal_timer_init(&timer, 1000));
    CHECK_NOT_IMPLEMENTED(hal_timer_start(&timer));
    CHECK_NOT_IMPLEMENTED(hal_timer_stop(&timer));
    CHECK_NOT_IMPLEMENTED(hal_timer_set_period(&timer, 2000));
    CHECK_NOT_IMPLEMENTED(hal_timer_register_callback(&timer, NULL, NULL));

    hal_pwm_t pwm;
    CHECK_NOT_IMPLEMENTED(hal_pwm_init(&pwm, 50000));
    CHECK_NOT_IMPLEMENTED(hal_pwm_set_duty(&pwm, 50.0f));
    CHECK_NOT_IMPLEMENTED(hal_pwm_start(&pwm));
    CHECK_NOT_IMPLEMENTED(hal_pwm_stop(&pwm));

    hal_can_t can; hal_can_frame_t frame = {0}; hal_can_filter_t filt = {0, 0};
    CHECK_NOT_IMPLEMENTED(hal_can_init(&can, 500000));
    CHECK_NOT_IMPLEMENTED(hal_can_send(&can, &frame));
    CHECK_NOT_IMPLEMENTED(hal_can_receive(&can, &frame));
    CHECK_NOT_IMPLEMENTED(hal_can_set_filter(&can, &filt));

    hal_dma_channel_t dma; hal_dma_config_t dcfg = {NULL, NULL, 0, HAL_DMA_DIR_MEM_TO_MEM};
    hal_status_t dma_status;
    CHECK_NOT_IMPLEMENTED(hal_dma_configure(&dma, &dcfg));
    CHECK_NOT_IMPLEMENTED(hal_dma_start(&dma));
    CHECK_NOT_IMPLEMENTED(hal_dma_stop(&dma));
    CHECK_NOT_IMPLEMENTED(hal_dma_get_status(&dma, &dma_status));

    hal_adc_channel_t adc; uint16_t raw = 0;
    CHECK_NOT_IMPLEMENTED(hal_adc_init(&adc, 12));
    CHECK_NOT_IMPLEMENTED(hal_adc_read(&adc, &raw));
    CHECK_NOT_IMPLEMENTED(hal_adc_start_continuous(&adc, 1000));
    CHECK_NOT_IMPLEMENTED(hal_adc_register_callback(&adc, NULL, NULL));

    uint8_t flash_buf[4] = {0};
    hal_status_t flash_status;
    CHECK_NOT_IMPLEMENTED(hal_flash_read(0, flash_buf, 4));
    CHECK_NOT_IMPLEMENTED(hal_flash_write(0, flash_buf, 4));
    CHECK_NOT_IMPLEMENTED(hal_flash_erase_sector(0));
    CHECK_NOT_IMPLEMENTED(hal_flash_get_status(&flash_status));

    CHECK_NOT_IMPLEMENTED(hal_clock_init());
    (void)hal_clock_get_tick_ms();
    (void)hal_clock_get_tick_us();
    hal_clock_delay_ms(0);

    float volts = 0.0f, amps = 0.0f;
    CHECK_NOT_IMPLEMENTED(hal_power_get_battery_voltage(&volts));
    CHECK_NOT_IMPLEMENTED(hal_power_get_current(&amps));
    CHECK_NOT_IMPLEMENTED(hal_power_set_low_power_mode(HAL_POWER_MODE_ACTIVE));

    hal_sensor_t sensor; hal_sensor_health_t health;
    CHECK_NOT_IMPLEMENTED(hal_sensor_init(&sensor));
    CHECK_NOT_IMPLEMENTED(hal_sensor_read(&sensor, flash_buf, 4));
    CHECK_NOT_IMPLEMENTED(hal_sensor_subscribe(&sensor, NULL, NULL));
    CHECK_NOT_IMPLEMENTED(hal_sensor_get_health_status(&sensor, &health));

    if (g_failures == 0) {
        printf("PASS: all HAL headers link and null-backend stubs behave as expected (13/13 headers)\n");
        return 0;
    }
    fprintf(stderr, "FAIL: %d assertion(s) failed\n", g_failures);
    return 1;
}
