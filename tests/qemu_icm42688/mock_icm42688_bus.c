/**
 * @file    mock_icm42688_bus.c
 * @brief   Software model of an ICM-42688-P used to test icm42688.c without
 *          any real hardware or QEMU peripheral model — a fake register
 *          file plus enough behavior (WHO_AM_I, power modes, changing
 *          accel/gyro/temp data, a draining FIFO) to exercise every driver
 *          code path (DRIVER_STANDARD.md Section 7).
 *
 * @details Implements the same ICM42688_BusRead / ICM42688_BusWrite /
 *          ICM42688_DelayMs contract that icm42688_bus_stm32.c implements
 *          for real hardware — icm42688.c cannot tell them apart.
 *
 *          Sample data is regenerated on every read (mirrors the project's
 *          established "no static return values" pattern for mock buses)
 *          using a small deterministic PRNG so runs are reproducible: a
 *          gently jittered +1 g on the accel Z axis, near-zero gyro rates,
 *          and a fixed die temperature, each with pseudo-Gaussian noise.
 */

#include "icm42688_bus.h"
#include "icm42688_registers.h"
#include <string.h>
#include <math.h>

/*============================================================================*
 *                         DETERMINISTIC PRNG + GAUSSIAN                      *
 *============================================================================*/

static uint32_t s_lcg_state = 0x1234ABCDU;

static uint32_t next_u32(void)
{
    /* Numerical Recipes LCG constants: fast, deterministic, good enough for
     * a test-data generator (not for anything security-sensitive). */
    s_lcg_state = (1664525U * s_lcg_state) + 1013904223U;
    return s_lcg_state;
}

static double next_uniform01(void)
{
    return (double)(next_u32() & 0x00FFFFFFU) / (double)0x01000000U; /* [0,1) */
}

/** @brief Box-Muller transform: one N(0,1) sample per call. */
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
 *                            MOCK DEVICE STATE                               *
 *============================================================================*/

typedef struct {
    uint8_t  who_am_i;
    uint8_t  device_config;
    uint8_t  pwr_mgmt0;
    uint8_t  accel_config0;
    uint8_t  gyro_config0;
    uint8_t  fifo_config;

    uint16_t fifo_count_bytes;   /**< Simulated bytes currently queued. */
    uint32_t reads_since_reset;  /**< Drives the FIFO "time passing" model. */

    int      fault_injected;     /**< Nonzero => every BusRead/BusWrite fails. */
} MockIcm42688_t;

static MockIcm42688_t s_dev;

/*============================================================================*
 *                          TEST-CONTROL ENTRY POINTS                         *
 *============================================================================*/

/** @brief Resets the mock to power-on defaults. Call at the top of every
 *         test case so cases can't leak state into one another. */
void mock_icm42688_bus_reset(void)
{
    memset(&s_dev, 0, sizeof(s_dev));
    s_dev.who_am_i = ICM42688_WHO_AM_I_VALUE;
    s_lcg_state = 0x1234ABCDU;
}

/** @brief Forces every subsequent bus call to fail with a nonzero return,
 *         to exercise the driver's ICM42688_ERROR_BUS propagation path. */
void mock_icm42688_bus_set_fault_injection(int enabled)
{
    s_dev.fault_injected = enabled;
}

/** @brief Overrides WHO_AM_I, to exercise ICM42688_ERROR_WRONG_DEVICE_ID. */
void mock_icm42688_bus_force_who_am_i(uint8_t value)
{
    s_dev.who_am_i = value;
}

/** @brief Simulates time passing: enqueues one more 12-byte packet's worth
 *         of FIFO data (only accumulates while the FIFO is in stream mode). */
void mock_icm42688_bus_tick(void)
{
    if ((s_dev.fifo_config & ICM42688_FIFO_MODE_MASK) == ICM42688_FIFO_MODE_STREAM) {
        s_dev.fifo_count_bytes = (uint16_t)(s_dev.fifo_count_bytes + ICM42688_FIFO_PACKET_BYTES);
    }
}

/*============================================================================*
 *                          SIMULATED DATA REGISTERS                          *
 *============================================================================*/

static int16_t simulate_accel_axis(double baseline_g)
{
    const double lsb_per_g = 2048.0; /* +-16 g range, Init()'s default FS */
    double noise_g = next_gaussian() * 0.01; /* ~10 mg jitter */
    double counts = (baseline_g + noise_g) * lsb_per_g;
    return (int16_t)counts;
}

static int16_t simulate_gyro_axis(void)
{
    const double lsb_per_dps = 16.384; /* +-2000 dps range, Init()'s default FS */
    double noise_dps = next_gaussian() * 0.5; /* ~0.5 dps jitter, stationary sensor */
    return (int16_t)(noise_dps * lsb_per_dps);
}

static int16_t simulate_temp(void)
{
    /* ICM-42688-P: Temp_degC = (TEMP_DATA / 132.48) + 25. Center on 25 C
     * with a small jitter so repeated reads visibly differ. */
    double noise_c = next_gaussian() * 0.05;
    return (int16_t)(noise_c * 132.48);
}

/** @brief Fills the 14 contiguous temp+accel+gyro data bytes for a burst read
 *         starting at ICM42688_REG_TEMP_DATA1. */
static void fill_primary_data_block(uint8_t *out14)
{
    int16_t t  = simulate_temp();
    int16_t ax = simulate_accel_axis(0.0);
    int16_t ay = simulate_accel_axis(0.0);
    int16_t az = simulate_accel_axis(1.0); /* device sitting flat: +1 g on Z */
    int16_t gx = simulate_gyro_axis();
    int16_t gy = simulate_gyro_axis();
    int16_t gz = simulate_gyro_axis();

    const int16_t vals[7] = { t, ax, ay, az, gx, gy, gz };
    for (int i = 0; i < 7; i++) {
        out14[2 * i]     = (uint8_t)(((uint16_t)vals[i] >> 8) & 0xFFU);
        out14[2 * i + 1] = (uint8_t)((uint16_t)vals[i] & 0xFFU);
    }
}

static void fill_fifo_packet(uint8_t *out12)
{
    int16_t ax = simulate_accel_axis(0.0);
    int16_t ay = simulate_accel_axis(0.0);
    int16_t az = simulate_accel_axis(1.0);
    int16_t gx = simulate_gyro_axis();
    int16_t gy = simulate_gyro_axis();
    int16_t gz = simulate_gyro_axis();

    const int16_t vals[6] = { ax, ay, az, gx, gy, gz };
    for (int i = 0; i < 6; i++) {
        out12[2 * i]     = (uint8_t)(((uint16_t)vals[i] >> 8) & 0xFFU);
        out12[2 * i + 1] = (uint8_t)((uint16_t)vals[i] & 0xFFU);
    }
}

/*============================================================================*
 *                    ICM42688_Bus CONTRACT IMPLEMENTATION                    *
 *============================================================================*/

int ICM42688_BusRead(void *bus_context, uint8_t reg, uint8_t *data, uint16_t length)
{
    (void)bus_context; /* the mock is a process-wide singleton */
    if (data == NULL || length == 0U) {
        return -1;
    }
    if (s_dev.fault_injected) {
        return -1;
    }

    /* Note: reading a register never itself advances simulated time — only
     * an explicit mock_icm42688_bus_tick() does. Ticking on every read would
     * make FIFO_COUNT and FIFO_DATA reads refill the very FIFO they're
     * trying to inspect/drain, which is not how a real ODR-driven FIFO
     * behaves. */

    switch (reg) {
        case ICM42688_REG_WHO_AM_I:
            if (length != 1U) return -1;
            data[0] = s_dev.who_am_i;
            return 0;

        case ICM42688_REG_TEMP_DATA1:
            if (length != 14U) return -1;
            fill_primary_data_block(data);
            return 0;

        case ICM42688_REG_FIFO_COUNTH:
            if (length != 2U) return -1;
            data[0] = (uint8_t)((s_dev.fifo_count_bytes >> 8) & 0xFFU);
            data[1] = (uint8_t)(s_dev.fifo_count_bytes & 0xFFU);
            return 0;

        case ICM42688_REG_FIFO_DATA:
            if (length != 12U) return -1;
            if (s_dev.fifo_count_bytes < ICM42688_FIFO_PACKET_BYTES) {
                return -1; /* underflow: caller should have checked count first */
            }
            fill_fifo_packet(data);
            s_dev.fifo_count_bytes = (uint16_t)(s_dev.fifo_count_bytes - ICM42688_FIFO_PACKET_BYTES);
            return 0;

        default:
            return -1; /* unmodeled register */
    }
}

int ICM42688_BusWrite(void *bus_context, uint8_t reg, uint8_t data)
{
    (void)bus_context;
    if (s_dev.fault_injected) {
        return -1;
    }

    switch (reg) {
        case ICM42688_REG_DEVICE_CONFIG:
            s_dev.device_config = data;
            if (data & ICM42688_DEVICE_CONFIG_SOFT_RESET) {
                uint8_t who_am_i_backup = s_dev.who_am_i; /* survives reset */
                int fault_backup = s_dev.fault_injected;
                memset(&s_dev, 0, sizeof(s_dev));
                s_dev.who_am_i = who_am_i_backup;
                s_dev.fault_injected = fault_backup;
            }
            return 0;

        case ICM42688_REG_PWR_MGMT0:
            s_dev.pwr_mgmt0 = data;
            return 0;

        case ICM42688_REG_ACCEL_CONFIG0:
            s_dev.accel_config0 = data;
            return 0;

        case ICM42688_REG_GYRO_CONFIG0:
            s_dev.gyro_config0 = data;
            return 0;

        case ICM42688_REG_FIFO_CONFIG:
            s_dev.fifo_config = data;
            if ((data & ICM42688_FIFO_MODE_MASK) == ICM42688_FIFO_MODE_BYPASS) {
                s_dev.fifo_count_bytes = 0U; /* disabling the FIFO flushes it */
            }
            return 0;

        default:
            return -1; /* unmodeled register */
    }
}

void ICM42688_DelayMs(uint32_t ms)
{
    (void)ms; /* host test: no need to actually block */
}
