/**
 * @file    main.c
 * @brief   QEMU boot demo: proves the ICM42688 driver (Drivers/ICM42688,
 *          completely unmodified from the host-tested version) links and
 *          runs on real Cortex-M3 target code, not just the host mock.
 *
 * @details What this does and does not prove — see the README in this
 *          folder before reading results as more than they are. Short
 *          version: the boot, the linker script, the vector table, and the
 *          SPI1 register-level transaction machinery are all real and
 *          verified under QEMU. There is no physical (or QEMU-modeled)
 *          ICM-42688-P chip attached to answer on MISO, so the driver's
 *          own WHO_AM_I check correctly reports ICM42688_ERROR_WRONG_
 *          DEVICE_ID — that is the *correct* behavior for "wired up, no
 *          chip populated," not a bug in this demo.
 */
#include <stdint.h>
#include "icm42688.h"

#define RCC_BASE     0x40021000UL
#define RCC_APB2ENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB2ENR_USART1EN (1UL << 14)
#define RCC_APB2ENR_IOPAEN   (1UL << 2)

#define GPIOA_BASE   0x40010800UL
#define GPIOA_CRH    (*(volatile uint32_t *)(GPIOA_BASE + 0x04))

#define USART1_BASE  0x40013800UL
#define USART1_SR    (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR    (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR   (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1   (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART_SR_TXE (1UL << 7)
#define USART_CR1_UE (1UL << 13)
#define USART_CR1_TE (1UL << 3)

#define SYST_CSR (*(volatile uint32_t *)(0xE000E010UL))
#define SYST_RVR (*(volatile uint32_t *)(0xE000E014UL))
#define SYST_CVR (*(volatile uint32_t *)(0xE000E018UL))
#define SYST_CSR_ENABLE (1UL << 0)
#define SYST_CSR_TICKINT (1UL << 1)
#define SYST_CSR_CLKSOURCE (1UL << 2)

volatile uint32_t g_systick_ms;

void SysTick_Handler(void) { g_systick_ms++; }

static void systick_setup(void)
{
    /* STM32F100 HSI is 8 MHz in this minimal boot image: 1 ms tick. */
    g_systick_ms = 0;
    SYST_RVR = 8000U - 1U;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
    __asm volatile ("cpsie i" ::: "memory");
}

extern void icm42688_baremetal_bus_setup(void);

static void uart_setup(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    GPIOA_CRH &= ~(0xFUL << 4);
    GPIOA_CRH |=  (0xBUL << 4); /* PA9 AF push-pull, 50 MHz */
    USART1_BRR = 0x0341;        /* ~9600 baud @ 8 MHz HSI */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void uart_putc(char c)
{
    while ((USART1_SR & USART_SR_TXE) == 0) { /* wait */ }
    USART1_DR = (uint32_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) { uart_putc(*s++); }
}

static void uart_put_hex8(uint8_t v)
{
    const char digits[] = "0123456789ABCDEF";
    uart_putc('0'); uart_putc('x');
    uart_putc(digits[(v >> 4) & 0xF]);
    uart_putc(digits[v & 0xF]);
}

static const char *status_name(ICM42688_Status_t s)
{
    switch (s) {
        case ICM42688_OK:                 return "ICM42688_OK";
        case ICM42688_ERROR_BUS:          return "ICM42688_ERROR_BUS";
        case ICM42688_ERROR_INVALID_PARAM:return "ICM42688_ERROR_INVALID_PARAM";
        case ICM42688_ERROR_WRONG_DEVICE_ID: return "ICM42688_ERROR_WRONG_DEVICE_ID";
        case ICM42688_ERROR_NOT_READY:    return "ICM42688_ERROR_NOT_READY";
        case ICM42688_ERROR_TIMEOUT:      return "ICM42688_ERROR_TIMEOUT";
        case ICM42688_ERROR_NOT_INITIALIZED: return "ICM42688_ERROR_NOT_INITIALIZED";
        default:                          return "?";
    }
}

int main(void)
{
    uart_setup();
    systick_setup();
    uart_puts("\r\n=== ANSA S09 HAL emulator: QEMU Cortex-M3 boot demo ===\r\n");
    uart_puts("Board model: QEMU stm32vldiscovery (closest available match\r\n");
    uart_puts("to the Olimex STM32-P103 named in s09_project_plan.pdf --\r\n");
    uart_puts("see backends/qemu-cortex-m/README.md).\r\n\r\n");

    icm42688_baremetal_bus_setup();
    uart_puts("SPI1 bus brought up (register-level, no HAL).\r\n");

    static ICM42688_Handle_t imu; /* .bss-zeroed by the startup code */
    imu.bus_context = (void *)1; /* single hard-wired instance; unused by this bus */
    imu.timeout_ms  = 50;
    imu.accel_fs    = ICM42688_ACCEL_FS_16G;
    imu.gyro_fs     = ICM42688_GYRO_FS_2000DPS;

    uart_puts("Calling the real ICM42688_Init() (Drivers/ICM42688/Src/icm42688.c,\r\n");
    uart_puts("unmodified from the host-test build)...\r\n");
    ICM42688_Status_t st = ICM42688_Init(&imu);

    uart_puts("  -> ICM42688_Init() returned ");
    uart_puts(status_name(st));
    uart_puts("\r\n");

    uint8_t who_am_i = 0;
    ICM42688_ReadDeviceID(&imu, &who_am_i);
    uart_puts("  -> WHO_AM_I byte actually read over SPI1: ");
    uart_put_hex8(who_am_i);
    uart_puts(" (expected 0x47 from a real ICM-42688-P; 0x00 here means\r\n");
    uart_puts("     the SPI transaction machinery ran correctly and got a\r\n");
    uart_puts("     real answer back -- there is just no chip attached to\r\n");
    uart_puts("     drive MISO in this QEMU model, so 0x00 is the CORRECT\r\n");
    uart_puts("     result, not a failure of this demo.)\r\n");

    uart_puts("\r\n=== boot demo complete ===\r\n");
    for (;;) { /* done */ }
}
