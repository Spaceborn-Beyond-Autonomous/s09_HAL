/*
 * "Null backend" — Stage 1 deliverable.
 *
 * Purpose: prove every HAL header compiles and links standalone, before any
 * real implementation (Virtual/Stage 3, QEMU/Stage 4, F7/Stage 10, ...)
 * exists. Every function simply returns HAL_NOT_IMPLEMENTED. This is NOT a
 * usable backend — it exists only to keep the HAL Compliance test harness
 * (tests/hal_compliance/) buildable from day one of Stage 1 onward.
 */
#include "hal_flash.h"
#include "hal_clock.h"
#include "hal_power.h"
#include "null_backend_internal.h" /* opaque struct layouts + remaining hal_*.h includes */

/* ---- hal_gpio ---- */
hal_status_t hal_gpio_init(hal_gpio_pin_t *pin, hal_gpio_mode_t mode) { (void)pin; (void)mode; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_gpio_write(hal_gpio_pin_t *pin, bool level) { (void)pin; (void)level; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_gpio_read(hal_gpio_pin_t *pin, bool *level_out) { (void)pin; (void)level_out; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_gpio_toggle(hal_gpio_pin_t *pin) { (void)pin; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_gpio_set_mode(hal_gpio_pin_t *pin, hal_gpio_mode_t mode) { (void)pin; (void)mode; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_gpio_set_interrupt(hal_gpio_pin_t *pin, hal_gpio_irq_cb_t cb, void *ctx) { (void)pin; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_uart ---- */
hal_status_t hal_uart_init(hal_uart_t *u, const hal_uart_config_t *c) { (void)u; (void)c; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_uart_write(hal_uart_t *u, const uint8_t *d, size_t l) { (void)u; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_uart_read(hal_uart_t *u, uint8_t *d, size_t l, size_t *br) { (void)u; (void)d; (void)l; (void)br; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_uart_write_async(hal_uart_t *u, const uint8_t *d, size_t l, hal_uart_tx_done_cb_t cb, void *ctx) { (void)u; (void)d; (void)l; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_uart_register_rx_callback(hal_uart_t *u, hal_uart_rx_cb_t cb, void *ctx) { (void)u; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_spi ---- */
hal_status_t hal_spi_init(hal_spi_t *s, const hal_spi_config_t *c) { (void)s; (void)c; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_spi_transfer(hal_spi_t *s, const uint8_t *tx, uint8_t *rx, size_t l) { (void)s; (void)tx; (void)rx; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_spi_transfer_async(hal_spi_t *s, const uint8_t *tx, uint8_t *rx, size_t l, hal_spi_transfer_done_cb_t cb, void *ctx) { (void)s; (void)tx; (void)rx; (void)l; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_spi_set_cs(hal_spi_t *s, bool asserted) { (void)s; (void)asserted; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_i2c ---- */
hal_status_t hal_i2c_init(hal_i2c_t *i, const hal_i2c_config_t *c) { (void)i; (void)c; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_i2c_write(hal_i2c_t *i, uint8_t addr, const uint8_t *d, size_t l) { (void)i; (void)addr; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_i2c_read(hal_i2c_t *i, uint8_t addr, uint8_t *d, size_t l) { (void)i; (void)addr; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_i2c_mem_write(hal_i2c_t *i, uint8_t addr, uint16_t mem, const uint8_t *d, size_t l) { (void)i; (void)addr; (void)mem; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_i2c_mem_read(hal_i2c_t *i, uint8_t addr, uint16_t mem, uint8_t *d, size_t l) { (void)i; (void)addr; (void)mem; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_timer ---- */
hal_status_t hal_timer_init(hal_timer_t *t, uint32_t period_us) { (void)t; (void)period_us; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_timer_start(hal_timer_t *t) { (void)t; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_timer_stop(hal_timer_t *t) { (void)t; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_timer_set_period(hal_timer_t *t, uint32_t period_us) { (void)t; (void)period_us; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_timer_register_callback(hal_timer_t *t, hal_timer_cb_t cb, void *ctx) { (void)t; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_pwm ---- */
hal_status_t hal_pwm_init(hal_pwm_t *p, uint32_t freq_hz) { (void)p; (void)freq_hz; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_pwm_set_duty(hal_pwm_t *p, float duty_percent) { (void)p; (void)duty_percent; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_pwm_start(hal_pwm_t *p) { (void)p; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_pwm_stop(hal_pwm_t *p) { (void)p; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_can ---- */
hal_status_t hal_can_init(hal_can_t *c, uint32_t bitrate) { (void)c; (void)bitrate; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_can_send(hal_can_t *c, const hal_can_frame_t *f) { (void)c; (void)f; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_can_receive(hal_can_t *c, hal_can_frame_t *f) { (void)c; (void)f; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_can_set_filter(hal_can_t *c, const hal_can_filter_t *f) { (void)c; (void)f; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_dma ---- */
hal_status_t hal_dma_configure(hal_dma_channel_t *ch, const hal_dma_config_t *c) { (void)ch; (void)c; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_dma_start(hal_dma_channel_t *ch) { (void)ch; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_dma_stop(hal_dma_channel_t *ch) { (void)ch; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_dma_get_status(hal_dma_channel_t *ch, hal_status_t *status_out) { (void)ch; (void)status_out; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_adc ---- */
hal_status_t hal_adc_init(hal_adc_channel_t *ch, uint8_t res_bits) { (void)ch; (void)res_bits; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_adc_read(hal_adc_channel_t *ch, uint16_t *raw_out) { (void)ch; (void)raw_out; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_adc_start_continuous(hal_adc_channel_t *ch, uint32_t rate_hz) { (void)ch; (void)rate_hz; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_adc_register_callback(hal_adc_channel_t *ch, hal_adc_cb_t cb, void *ctx) { (void)ch; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_flash ---- */
hal_status_t hal_flash_read(uint32_t addr, uint8_t *d, size_t l) { (void)addr; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_flash_write(uint32_t addr, const uint8_t *d, size_t l) { (void)addr; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_flash_erase_sector(uint32_t sector) { (void)sector; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_flash_get_status(hal_status_t *status_out) { (void)status_out; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_clock ---- */
hal_status_t hal_clock_init(void) { return HAL_NOT_IMPLEMENTED; }
uint64_t hal_clock_get_tick_ms(void) { return 0; }
uint64_t hal_clock_get_tick_us(void) { return 0; }
void hal_clock_delay_ms(uint32_t ms) { (void)ms; }

/* ---- hal_power ---- */
hal_status_t hal_power_get_battery_voltage(float *v) { (void)v; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_power_get_current(float *a) { (void)a; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_power_set_low_power_mode(hal_power_mode_t mode) { (void)mode; return HAL_NOT_IMPLEMENTED; }

/* ---- hal_sensor ---- */
hal_status_t hal_sensor_init(hal_sensor_t *s) { (void)s; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_sensor_read(hal_sensor_t *s, void *d, size_t l) { (void)s; (void)d; (void)l; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_sensor_subscribe(hal_sensor_t *s, hal_sensor_cb_t cb, void *ctx) { (void)s; (void)cb; (void)ctx; return HAL_NOT_IMPLEMENTED; }
hal_status_t hal_sensor_get_health_status(hal_sensor_t *s, hal_sensor_health_t *h) { (void)s; (void)h; return HAL_NOT_IMPLEMENTED; }
