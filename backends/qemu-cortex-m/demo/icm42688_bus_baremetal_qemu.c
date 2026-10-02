/**
 * @file    icm42688_bus_baremetal_qemu.c
 * @brief   Third implementation of the ICM42688_Bus contract (see
 *          Drivers/ICM42688/Inc/icm42688_bus.h): direct STM32F100 SPI1
 *          register pokes, no STM32Cube HAL, no RTOS — for this QEMU
 *          boot demo specifically.
 *
 * @details The driver (icm42688.c) is completely unaware this exists: it
 *          only ever calls ICM42688_BusRead/BusWrite/DelayMs. This file is
 *          proof that the bus-abstraction boundary in DRIVER_STANDARD.md
 *          Section 4 does what it's for — the exact same icm42688.c that
 *          the host-side mock test links against (tests/qemu_icm42688)
 *          also links, unmodified, into a real Cortex-M3 boot image here.
 *
 *          A real STM32Cube-based firmware would use
 *          Drivers/ICM42688/Src/icm42688_bus_stm32.c instead (HAL_SPI_*
 *          calls) — this register-level version exists only because
 *          pulling in the full STM32Cube HAL/CMSIS device pack was out of
 *          scope for a from-scratch QEMU boot demo.
 */

#include "icm42688_bus.h"
#include <stddef.h>

#define RCC_BASE     0x40021000UL
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB2ENR_IOPAEN (1UL << 2)
#define RCC_APB2ENR_SPI1EN (1UL << 12)

#define GPIOA_BASE   0x40010800UL
#define GPIOA_CRL    (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH    (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_ODR    (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))

#define SPI1_BASE    0x40013000UL
#define SPI1_CR1     (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_SR      (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR      (*(volatile uint32_t *)(SPI1_BASE + 0x0C))
#define SPI_SR_RXNE  (1UL << 0)
#define SPI_SR_TXE   (1UL << 1)
#define SPI_CR1_SPE  (1UL << 6)

/** @brief PA4 as a plain GPIO output, bit-banged as SPI1's chip-select —
 *         this demo wires exactly one IMU instance, so bus_context is
 *         unused (kept only so the function signature matches the
 *         contract every other bus backend implements). */
static void cs_low(void)  { GPIOA_ODR &= ~(1UL << 4); }
static void cs_high(void) { GPIOA_ODR |=  (1UL << 4); }

/** @brief One-time SPI1 + GPIO bring-up. Call before the driver's Init(). */
void icm42688_baremetal_bus_setup(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    /* PA4 = CS (general push-pull output), PA5 = SCK, PA7 = MOSI: AF
     * push-pull; PA6 = MISO: input floating. CRL covers pins 0-7. */
    GPIOA_CRL &= ~((0xFUL << 16) | (0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    GPIOA_CRL |=  ((0x3UL << 16) /* PA4 general PP out, 50 MHz */
                 | (0xBUL << 20) /* PA5 AF PP out, 50 MHz (SCK)  */
                 | (0x4UL << 24) /* PA6 input floating (MISO)    */
                 | (0xBUL << 28)); /* PA7 AF PP out, 50 MHz (MOSI) */
    cs_high();

    /* Master mode, baud/8, CPOL=0/CPHA=0, software NSS management. */
    SPI1_CR1 = (1UL << 2) /* MSTR   */
             | (3UL << 3) /* BR[2:0]: fPCLK/16 */
             | (1UL << 8) /* SSI    */
             | (1UL << 9) /* SSM    */
             | SPI_CR1_SPE;
}

static uint8_t spi_transfer(uint8_t out)
{
    while ((SPI1_SR & SPI_SR_TXE) == 0) { /* wait for TX buffer empty */ }
    SPI1_DR = out;
    while ((SPI1_SR & SPI_SR_RXNE) == 0) { /* wait for RX buffer full */ }
    return (uint8_t)SPI1_DR;
}

int ICM42688_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length)
{
    (void)bus_context;
    if (data == NULL) {
        return -1;
    }
    cs_low();
    spi_transfer((uint8_t)(reg | 0x80U)); /* bit7 = 1 -> read */
    for (uint16_t i = 0; i < length; i++) {
        data[i] = spi_transfer(0x00U); /* clock out dummy bytes to receive */
    }
    cs_high();
    return 0;
}

int ICM42688_BusWrite(void *bus_context, uint8_t reg, uint8_t data)
{
    (void)bus_context;
    cs_low();
    spi_transfer((uint8_t)(reg & 0x7FU));
    spi_transfer(data);
    cs_high();
    return 0;
}

extern volatile uint32_t g_systick_ms;

void ICM42688_DelayMs(uint32_t ms)
{
    uint32_t start = g_systick_ms;
    while ((uint32_t)(g_systick_ms - start) < ms) { /* interrupt-driven delay */ }
}
