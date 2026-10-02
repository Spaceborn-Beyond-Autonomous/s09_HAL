/**
 * @file    icm42688_bus_stm32.c
 * @brief   STM32 HAL SPI implementation of the ICM42688_Bus contract.
 * @details This is the ONLY file in the ICM42688 driver allowed to touch
 *          HAL_SPI_* directly (DRIVER_STANDARD.md Section 4). It is not
 *          part of the host-side test build (tests/qemu_icm42688 links
 *          mock_icm42688_bus.c instead) — it compiles against the real
 *          STM32Cube HAL when this driver is dropped into firmware.
 *
 * @note    bus_context is expected to point at an Icm42688BusContext_t
 *          describing which SPI peripheral and chip-select line this
 *          instance lives on, so more than one IMU can be wired up.
 */

#include "icm42688_bus.h"

#if defined(ICM42688_TARGET_STM32)

#include "stm32f4xx_hal.h"

/** @brief Per-instance SPI + chip-select routing, pointed to by
 *         ICM42688_Handle_t::bus_context on real hardware. */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef       *cs_port;
    uint16_t            cs_pin;
} Icm42688BusContext_t;

/** @brief ICM-42688-P SPI read: set bit7 of the address byte, then clock out
 *         `length` don't-care bytes to receive the register contents. */
int ICM42688_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length)
{
    if (bus_context == NULL || data == NULL) {
        return -1;
    }
    Icm42688BusContext_t *ctx = (Icm42688BusContext_t *)bus_context;
    uint8_t addr = (uint8_t)(reg | 0x80U); /* bit7 = 1 -> read */

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(ctx->hspi, &addr, 1U, HAL_MAX_DELAY);
    if (st == HAL_OK) {
        st = HAL_SPI_Receive(ctx->hspi, data, length, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);

    return (st == HAL_OK) ? 0 : -1;
}

/** @brief ICM-42688-P SPI write: bit7 of the address byte stays 0, followed
 *         by exactly one data byte. */
int ICM42688_BusWrite(void *bus_context, uint8_t reg, uint8_t data)
{
    if (bus_context == NULL) {
        return -1;
    }
    Icm42688BusContext_t *ctx = (Icm42688BusContext_t *)bus_context;
    uint8_t frame[2] = { (uint8_t)(reg & 0x7FU), data };

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(ctx->hspi, frame, 2U, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);

    return (st == HAL_OK) ? 0 : -1;
}

void ICM42688_DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

#endif /* ICM42688_TARGET_STM32 */
