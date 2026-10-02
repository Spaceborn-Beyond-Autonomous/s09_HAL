/**
 * @file    icm42688_bus.h
 * @brief   Platform bus contract for the ICM-42688-P driver
 *          (DRIVER_STANDARD.md Section 4).
 * @details icm42688.c calls only these three functions to touch hardware.
 *          Exactly one implementation of this contract is linked into any
 *          given binary:
 *            - Real hardware:  icm42688_bus_stm32.c   (STM32 HAL SPI)
 *            - QEMU/host test: mock_icm42688_bus.c    (in-memory register file)
 */

#ifndef ICM42688_BUS_H
#define ICM42688_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads length bytes starting at register reg.
 * @param[in]  bus_context Opaque context from ICM42688_Handle_t::bus_context,
 *                         passed straight through (e.g. a platform SPI+CS
 *                         descriptor on real hardware, unused on the mock).
 * @param[in]  reg         Starting register address.
 * @param[out] data        Destination buffer, at least length bytes.
 * @param[in]  length      Number of bytes to read.
 * @return 0 on success, nonzero on any bus failure.
 */
int ICM42688_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length);

/**
 * @brief Writes a single byte to register reg.
 * @param[in] bus_context Opaque context, see ICM42688_BusRead().
 * @param[in] reg         Register address to write.
 * @param[in] data        Byte to write.
 * @return 0 on success, nonzero on any bus failure.
 */
int ICM42688_BusWrite(void *bus_context, uint8_t reg, uint8_t data);

/**
 * @brief Blocking millisecond delay.
 * @param[in] ms Duration to block for.
 */
void ICM42688_DelayMs(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* ICM42688_BUS_H */
