/**
 * @file    mock_bmp388_bus.c
 * @brief   Software model of a BMP388 used to test bmp388.c without any
 *          real hardware — a fake register file with CHIP_ID, a working
 *          STATUS/DRDY handshake, a fixed-but-realistic 21-byte NVM
 *          calibration block, and changing pressure/temperature samples
 *          (DRIVER_STANDARD.md Section 7).
 *
 * @details Implements the same BMP388_BusRead / BMP388_BusWrite /
 *          BMP388_DelayMs contract that bmp388_bus_stm32.c implements for
 *          real hardware — bmp388.c cannot tell them apart.
 *
 *          The 21 calibration bytes below are a synthetic-but-internally-
 *          consistent trim set (generated once, offline, by inverting
 *          bmp388.c's own parse_calibration() scale factors against target
 *          floating-point coefficients) — decoding them through the real
 *          driver reproduces a plausible ~25 C / ~101.3 kPa sea-level
 *          reading, not a randomly made-up one.
 */

#include "bmp388_bus.h"
#include "bmp388_registers.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>

/*============================================================================*
 *                         DETERMINISTIC PRNG + GAUSSIAN                      *
 *============================================================================*/

static uint32_t s_lcg_state = 0x9E3779B9U;

static uint32_t next_u32(void)
{
    s_lcg_state = (1664525U * s_lcg_state) + 1013904223U;
    return s_lcg_state;
}

static double next_uniform01(void)
{
    return (double)(next_u32() & 0x00FFFFFFU) / (double)0x01000000U;
}

static double next_gaussian(void)
{
    double u1 = next_uniform01();
    double u2 = next_uniform01();
    if (u1 < 1e-12) {
        u1 = 1e-12;
    }
    return sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979323846 * u2);
}

/*============================================================================*
 *                       SYNTHETIC NVM CALIBRATION BLOCK                      *
 *============================================================================*/

/**
 * @brief Self-consistent synthetic trim block. See file header: generated
 *        by inverting this driver's own parse_calibration() formula, not
 *        copied from a specific physical unit.
 */
static const uint8_t kCalibrationBytes[BMP388_CALIB_DATA_LEN] = {
    0x71, 0x72, 0xAA, 0x64, 0x00, 0x6D, 0x27, 0x16, 0x01, 0x00,
    0x00, 0x0A, 0x32, 0x00, 0x02, 0xFF, 0x00, 0x7E, 0x10, 0x08, 0xFE
};

/** @brief Baseline raw ADC counts that, run through the real compensation
 *         formula against kCalibrationBytes, land near 25 C / 101.3 kPa. */
#define BASELINE_TEMP_RAW  ((uint32_t)8540000U)
#define BASELINE_PRESS_RAW ((uint32_t)198000U)

/*============================================================================*
 *                            MOCK DEVICE STATE                               *
 *============================================================================*/

typedef struct {
    uint8_t  chip_id;
    uint8_t  pwr_ctrl;
    uint8_t  osr;
    uint8_t  odr;
    bool     press_ready;
    bool     temp_ready;
    int      fault_injected;
} MockBmp388_t;

static MockBmp388_t s_dev;

/*============================================================================*
 *                          TEST-CONTROL ENTRY POINTS                         *
 *============================================================================*/

void mock_bmp388_bus_reset(void)
{
    memset(&s_dev, 0, sizeof(s_dev));
    s_dev.chip_id = BMP388_CHIP_ID_VALUE;
    s_lcg_state = 0x9E3779B9U;
}

void mock_bmp388_bus_set_fault_injection(int enabled)
{
    s_dev.fault_injected = enabled;
}

void mock_bmp388_bus_force_chip_id(uint8_t value)
{
    s_dev.chip_id = value;
}

/** @brief Simulates one ODR cycle completing: a fresh pressure+temperature
 *         pair becomes available until the next DATA_0..DATA_5 read. */
void mock_bmp388_bus_tick(void)
{
    s_dev.press_ready = true;
    s_dev.temp_ready  = true;
}

/** @brief Forces DRDY back to "not ready", to exercise
 *         BMP388_ERROR_NOT_READY without waiting on real timing. */
void mock_bmp388_bus_clear_data_ready(void)
{
    s_dev.press_ready = false;
    s_dev.temp_ready  = false;
}

/*============================================================================*
 *                          SIMULATED DATA REGISTERS                          *
 *============================================================================*/

static uint32_t simulate_pressure_raw(void)
{
    double jitter = next_gaussian() * 40.0; /* a few Pa of noise once compensated */
    double v = (double)BASELINE_PRESS_RAW + jitter;
    if (v < 0.0) v = 0.0;
    return (uint32_t)v & 0x00FFFFFFU;
}

static uint32_t simulate_temperature_raw(void)
{
    double jitter = next_gaussian() * 400.0; /* small fractional-degree noise */
    double v = (double)BASELINE_TEMP_RAW + jitter;
    if (v < 0.0) v = 0.0;
    return (uint32_t)v & 0x00FFFFFFU;
}

/*============================================================================*
 *                    BMP388_Bus CONTRACT IMPLEMENTATION                      *
 *============================================================================*/

int BMP388_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length)
{
    (void)bus_context;
    if (data == NULL || length == 0U) {
        return -1;
    }
    if (s_dev.fault_injected) {
        return -1;
    }

    switch (reg) {
        case BMP388_REG_CHIP_ID:
            if (length != 1U) return -1;
            data[0] = s_dev.chip_id;
            return 0;

        case BMP388_REG_STATUS:
            if (length != 1U) return -1;
            data[0] = BMP388_STATUS_CMD_RDY;
            if (s_dev.press_ready) data[0] |= BMP388_STATUS_DRDY_PRESS;
            if (s_dev.temp_ready)  data[0] |= BMP388_STATUS_DRDY_TEMP;
            return 0;

        case BMP388_REG_DATA_0: {
            if (length != 6U) return -1;
            uint32_t p = simulate_pressure_raw();
            uint32_t t = simulate_temperature_raw();
            data[0] = (uint8_t)(p & 0xFFU);
            data[1] = (uint8_t)((p >> 8) & 0xFFU);
            data[2] = (uint8_t)((p >> 16) & 0xFFU);
            data[3] = (uint8_t)(t & 0xFFU);
            data[4] = (uint8_t)((t >> 8) & 0xFFU);
            data[5] = (uint8_t)((t >> 16) & 0xFFU);
            /* Reading data consumes the "fresh sample" — matches the real
             * BMP388's DRDY-clears-on-read behavior. */
            s_dev.press_ready = false;
            s_dev.temp_ready  = false;
            return 0;
        }

        case BMP388_REG_CALIB_DATA:
            if (length != BMP388_CALIB_DATA_LEN) return -1;
            memcpy(data, kCalibrationBytes, BMP388_CALIB_DATA_LEN);
            return 0;

        default:
            return -1; /* unmodeled register */
    }
}

int BMP388_BusWrite(void *bus_context, uint8_t reg, uint8_t data)
{
    (void)bus_context;
    if (s_dev.fault_injected) {
        return -1;
    }

    switch (reg) {
        case BMP388_REG_CMD:
            if (data == BMP388_CMD_SOFT_RESET) {
                uint8_t chip_id_backup = s_dev.chip_id;
                int fault_backup = s_dev.fault_injected;
                memset(&s_dev, 0, sizeof(s_dev));
                s_dev.chip_id = chip_id_backup;
                s_dev.fault_injected = fault_backup;
            }
            return 0;

        case BMP388_REG_OSR:
            s_dev.osr = data;
            return 0;

        case BMP388_REG_ODR:
            s_dev.odr = data;
            return 0;

        case BMP388_REG_PWR_CTRL:
            s_dev.pwr_ctrl = data;
            if ((data & (BMP388_PWR_PRESS_EN | BMP388_PWR_TEMP_EN)) ==
                (BMP388_PWR_PRESS_EN | BMP388_PWR_TEMP_EN)) {
                /* Mock simplification: treat the first conversion as
                 * completing immediately so Init() -> ReadData() works
                 * without the caller needing to poll. Use
                 * mock_bmp388_bus_clear_data_ready() to exercise the
                 * not-ready path explicitly instead. */
                s_dev.press_ready = true;
                s_dev.temp_ready  = true;
            }
            return 0;

        default:
            return -1; /* unmodeled register */
    }
}

void BMP388_DelayMs(uint32_t ms)
{
    (void)ms;
}
