/**
 * @file    bmp388_bus_stm32.c
 * @brief   STM32 HAL SPI implementation of the BMP388_Bus contract.
 * @details This is the ONLY file in the BMP388 driver allowed to touch
 *          HAL_SPI_* directly. Not part of the host-side test build
 *          (tests/qemu_bmp388 links mock_bmp388_bus.c instead).
 *
 * @note    BMP388 supports both I2C and SPI; this project follows the same
 *          SPI wiring already used for the IMU (see icm42688_bus_stm32.c),
 *          so one SPI bus serves both flight sensors.
 */

#include "bmp388_bus.h"

#if defined(BMP388_TARGET_STM32)

#include "stm32f4xx_hal.h"

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef       *cs_port;
    uint16_t            cs_pin;
} Bmp388BusContext_t;

int BMP388_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length)
{
    if (bus_context == NULL || data == NULL) {
        return -1;
    }
    Bmp388BusContext_t *ctx = (Bmp388BusContext_t *)bus_context;
    uint8_t addr = (uint8_t)(reg | 0x80U); /* bit7 = 1 -> read */

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(ctx->hspi, &addr, 1U, HAL_MAX_DELAY);
    if (st == HAL_OK) {
        st = HAL_SPI_Receive(ctx->hspi, data, length, HAL_MAX_DELAY);
    }
    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);

    return (st == HAL_OK) ? 0 : -1;
}

int BMP388_BusWrite(void *bus_context, uint8_t reg, uint8_t data)
{
    if (bus_context == NULL) {
        return -1;
    }
    Bmp388BusContext_t *ctx = (Bmp388BusContext_t *)bus_context;
    uint8_t frame[2] = { (uint8_t)(reg & 0x7FU), data };

    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(ctx->hspi, frame, 2U, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(ctx->cs_port, ctx->cs_pin, GPIO_PIN_SET);

    return (st == HAL_OK) ? 0 : -1;
}

void BMP388_DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

#endif /* BMP388_TARGET_STM32 */
