/**
 * @file    bmp388_bus.h
 * @brief   Platform bus contract for the BMP388 driver
 *          (DRIVER_STANDARD.md Section 4).
 * @details bmp388.c calls only these three functions to touch hardware.
 *          Exactly one implementation is linked into any given binary:
 *            - Real hardware:  bmp388_bus_stm32.c  (STM32 HAL, SPI or I2C)
 *            - QEMU/host test: mock_bmp388_bus.c   (in-memory register file)
 */

#ifndef BMP388_BUS_H
#define BMP388_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads length bytes starting at register reg.
 * @param[in]  bus_context Opaque context from BMP388_Handle_t::bus_context.
 * @param[in]  reg         Starting register address.
 * @param[out] data        Destination buffer, at least length bytes.
 * @param[in]  length      Number of bytes to read.
 * @return 0 on success, nonzero on any bus failure.
 */
int BMP388_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length);

/**
 * @brief Writes a single byte to register reg.
 * @param[in] bus_context Opaque context, see BMP388_BusRead().
 * @param[in] reg         Register address to write.
 * @param[in] data        Byte to write.
 * @return 0 on success, nonzero on any bus failure.
 */
int BMP388_BusWrite(void *bus_context, uint8_t reg, uint8_t data);

/**
 * @brief Blocking millisecond delay.
 * @param[in] ms Duration to block for.
 */
void BMP388_DelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* BMP388_BUS_H */
