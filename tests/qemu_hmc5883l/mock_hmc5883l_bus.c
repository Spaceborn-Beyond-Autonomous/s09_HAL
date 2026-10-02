/**
 * @file mock_hmc5883l_bus.c
 * @brief Host-side in-memory simulated bus implementation for the Honeywell HMC5883L.
 * @details Implements fully-featured register maps, auto-increment address pointers,
 *          revolving sensor-field simulators, self-test bias adjustments, and
 *          simplified register read-locking.
 *
 * @note Sourced against the official Honeywell HMC5883L datasheet.
 * 
 */

#include "hmc5883l.h"
#include "hmc5883l_registers.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*============================================================================*
 *                      FORWARD PROTOTYPES (HEADERLESS INTERFACE)             *
 *============================================================================*/

void mock_hmc5883l_reset_bus(void);
void mock_hmc5883l_set_reg(uint8_t reg_addr, uint8_t value);
uint8_t mock_hmc5883l_get_reg(uint8_t reg_addr);
void mock_hmc5883l_inject_bus_error(hal_status_t status);
void mock_hmc5883l_set_static_field(double x_gauss, double y_gauss, double z_gauss);
void mock_hmc5883l_enable_dynamic_sim(bool enable);
hal_status_t mock_hmc5883l_bus_read(void *bus, uint8_t dev_addr, uint8_t reg_addr, uint8_t *dest, size_t len, uint32_t timeout);
hal_status_t mock_hmc5883l_bus_write(void *bus, uint8_t dev_addr, uint8_t reg_addr, const uint8_t *src, size_t len, uint32_t timeout);

/*============================================================================*
 *                         SIMULATION STATIC STORAGE                          *
 *============================================================================*/

static uint8_t mock_regs[13];
static double  sim_angle_rad;
static bool    sim_is_dynamic;
static double  sim_static_x;
static double  sim_static_y;
static double  sim_static_z;
static bool    sim_self_test_override_active; /**< See mock_hmc5883l_set_static_field(). */

static hal_status_t bus_fault_injection;
static size_t       bytes_read_since_last_lock;
static bool         register_lock_active;

/* Honeywell HMC5883L Table 9 Gain settings mapping (page 114) */
static const uint16_t gain_scaling_lut[8] = {
    1370, /* GN = 0: ±0.88 Ga -> 1370 LSB/Gauss */
    1090, /* GN = 1: ±1.30 Ga -> 1090 LSB/Gauss (default) */
    820,  /* GN = 2: ±1.90 Ga -> 820 LSB/Gauss  */
    660,  /* GN = 3: ±2.50 Ga -> 660 LSB/Gauss  */
    440,  /* GN = 4: ±4.00 Ga -> 440 LSB/Gauss  */
    390,  /* GN = 5: ±4.70 Ga -> 390 LSB/Gauss  */
    330,  /* GN = 6: ±5.60 Ga -> 330 LSB/Gauss  */
    230   /* GN = 7: ±8.10 Ga -> 230 LSB/Gauss  */
};

/*============================================================================*
 *                        INTERNAL UTILITY CONSTRUCTORS                       *
 *============================================================================*/

static void update_measurement_registers(void) {
    /* Step 1: Decode hardware bias mode from CRA bits [1:0] */
    uint8_t cra_val = mock_regs[HMC5883L_REG_CRA];
    uint8_t bias_mode = (cra_val & HMC5883L_CRA_MS_MASK) >> HMC5883L_CRA_MS_SHIFT;

    /* Step 2: Decode amplification gain setting from CRB bits [7:5] */
    uint8_t crb_val = mock_regs[HMC5883L_REG_CRB];
    uint8_t gain_idx = (crb_val & HMC5883L_CRB_GN_MASK) >> HMC5883L_CRB_GN_SHIFT;
    if (gain_idx > 7) {
        gain_idx = 1; /* Fallback to default ±1.3 Ga */
    }
    double scale = (double)gain_scaling_lut[gain_idx];

    double x_gauss = 0.0;
    double y_gauss = 0.0;
    double z_gauss = 0.0;

    /* Step 3: Compute simulated magnetic axis loads based on operating states */
    if (bias_mode == 0x01) {
        /* Positive Bias Self-Test configuration (datasheet pages 92 & 132),
         * unless a test has explicitly forced a static field override via
         * mock_hmc5883l_set_static_field() — used to exercise
         * HMC5883L_RunSelfTest()'s out-of-range / failure path, which a
         * hardwired "always report the ideal excitation" self-test could
         * never reach. */
        if (sim_self_test_override_active) {
            x_gauss = sim_static_x;
            y_gauss = sim_static_y;
            z_gauss = sim_static_z;
        } else {
            x_gauss = 1.16;
            y_gauss = 1.16;
            z_gauss = 1.08;
        }
    } else if (bias_mode == 0x02) {
        /* Negative Bias Self-Test configuration */
        if (sim_self_test_override_active) {
            x_gauss = sim_static_x;
            y_gauss = sim_static_y;
            z_gauss = sim_static_z;
        } else {
            x_gauss = -1.16;
            y_gauss = -1.16;
            z_gauss = -1.08;
        }
    } else {
        /* Normal measurement flow */
        if (sim_is_dynamic) {
            /* Simulate rotating field vector representing machine rotation */
            x_gauss = cos(sim_angle_rad) * 0.48;
            y_gauss = sin(sim_angle_rad) * 0.48;
            z_gauss = -0.32; /* Sane physical Z-bias in micro-Tesla equivalents */

            /* Step rotation angle roughly 5 degrees per simulation clock */
            sim_angle_rad += 0.087;
            if (sim_angle_rad >= 2.0 * M_PI) {
                sim_angle_rad -= 2.0 * M_PI;
            }
        } else {
            x_gauss = sim_static_x;
            y_gauss = sim_static_y;
            z_gauss = sim_static_z;
        }
    }

    /* Step 4: Scale physical magnetic units to 12-bit integer Counts */
    int16_t x_raw = (int16_t)(x_gauss * scale);
    int16_t y_raw = (int16_t)(y_gauss * scale);
    int16_t z_raw = (int16_t)(z_gauss * scale);

    /* Enforce 12-bit saturation limits (-2048 to 2047) */
    if (x_raw > 2047) x_raw = 2047;
    if (x_raw < -2048) x_raw = -2048;
    if (y_raw > 2047) y_raw = 2047;
    if (y_raw < -2048) y_raw = -2048;
    if (z_raw > 2047) z_raw = 2047;
    if (z_raw < -2048) z_raw = -2048;

    /* Step 5: Write results into Honeywell ASIC-ordered registers (X -> Z -> Y) */
    mock_regs[HMC5883L_REG_DATA_X_MSB] = (uint8_t)((x_raw >> 8) & 0xFF);
    mock_regs[HMC5883L_REG_DATA_X_LSB] = (uint8_t)(x_raw & 0xFF);
    mock_regs[HMC5883L_REG_DATA_Z_MSB] = (uint8_t)((z_raw >> 8) & 0xFF);
    mock_regs[HMC5883L_REG_DATA_Z_LSB] = (uint8_t)(z_raw & 0xFF);
    mock_regs[HMC5883L_REG_DATA_Y_MSB] = (uint8_t)((y_raw >> 8) & 0xFF);
    mock_regs[HMC5883L_REG_DATA_Y_LSB] = (uint8_t)(y_raw & 0xFF);

    /* Step 6: Mark Ready status set in Status Register (0x09) */
    mock_regs[HMC5883L_REG_STATUS] |= HMC5883L_SR_RDY_MASK;
}

/*============================================================================*
 *                         PUBLIC UTILITY INTERFACES                          *
 *============================================================================*/

void mock_hmc5883l_reset_bus(void) {
    memset(mock_regs, 0x00, sizeof(mock_regs));

    /* Apply physical chip default values at power-on */
    mock_regs[HMC5883L_REG_CRA]    = 0x10; /* Average=1, ODR=15Hz, Bias=Normal */
    mock_regs[HMC5883L_REG_CRB]    = 0x20; /* Gain = 1090 LSB/Gauss (±1.3 Ga) */
    mock_regs[HMC5883L_REG_MODE]   = 0x01; /* Single-Measurement Mode default */
    mock_regs[HMC5883L_REG_STATUS] = 0x01; /* RDY bit high */

    /* Hardwired chip identifier register signatures */
    mock_regs[HMC5883L_REG_IDA] = HMC5883L_IDA_EXPECTED_VAL; /* 'H' (0x48) */
    mock_regs[HMC5883L_REG_IDB] = HMC5883L_IDB_EXPECTED_VAL; /* '4' (0x34) */
    mock_regs[HMC5883L_REG_IDC] = HMC5883L_IDC_EXPECTED_VAL; /* '3' (0x33) */

    /* Reset simulation coordinates */
    sim_angle_rad  = 0.0;
    sim_is_dynamic = true;
    sim_static_x   = 0.25;
    sim_static_y   = 0.15;
    sim_static_z   = -0.40;
    sim_self_test_override_active = false;

    bus_fault_injection = HAL_OK;
    bytes_read_since_last_lock = 0;
    register_lock_active = false;
}

void mock_hmc5883l_set_reg(uint8_t reg_addr, uint8_t value) {
    if (reg_addr < 13) {
        mock_regs[reg_addr] = value;
    }
}

uint8_t mock_hmc5883l_get_reg(uint8_t reg_addr) {
    if (reg_addr < 13) {
        return mock_regs[reg_addr];
    }
    return 0x00;
}

void mock_hmc5883l_inject_bus_error(hal_status_t status) {
    bus_fault_injection = status;
}

void mock_hmc5883l_set_static_field(double x_gauss, double y_gauss, double z_gauss) {
    sim_static_x   = x_gauss;
    sim_static_y   = y_gauss;
    sim_static_z   = z_gauss;
    sim_is_dynamic = false; /* Force static override */
    sim_self_test_override_active = true; /* also override the self-test excitation, see update_measurement_registers() */
}

void mock_hmc5883l_enable_dynamic_sim(bool enable) {
    sim_is_dynamic = enable;
}

/*============================================================================*
 *                      VIRTUAL BUS BINDINGS FOR DRIVER                       *
 *============================================================================*/

hal_status_t mock_hmc5883l_bus_read(void *bus, uint8_t dev_addr, uint8_t reg_addr, uint8_t *dest, size_t len, uint32_t timeout) {
    (void)bus;
    (void)timeout;

    /* Enforce slave address bounds verification */
    if (dev_addr != HMC5883L_I2C_ADDRESS) {
        return HAL_ERR_NACK;
    }

    /* Intercept and propagate any active fault injection codes */
    if (bus_fault_injection != HAL_OK) {
        return bus_fault_injection;
    }

    if (dest == NULL || len == 0) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* A real sensor in continuous mode keeps producing fresh samples on its
     * own between reads; this mock has no wall clock, so it treats "a read
     * starting at the data block, with dynamic sim enabled" as the signal
     * that simulated time has advanced and a new sample is due. Without
     * this, two back-to-back HMC5883L_ReadData() calls (which never write
     * to MODE in between) would return byte-for-byte identical data. */
    if (sim_is_dynamic && reg_addr >= HMC5883L_REG_DATA_X_MSB && reg_addr <= HMC5883L_REG_DATA_Y_LSB) {
        update_measurement_registers();
    }

    /* Perform block read with auto-increment pointer emulation */
    uint8_t curr_ptr = reg_addr;
    for (size_t i = 0; i < len; i++) {
        if (curr_ptr < 13) {
            dest[i] = mock_regs[curr_ptr];

            /* Simulate register read locks (datasheet pages 120-121) */
            if (curr_ptr >= HMC5883L_REG_DATA_X_MSB && curr_ptr <= HMC5883L_REG_DATA_Y_LSB) {
                if (!register_lock_active) {
                    register_lock_active = true;
                    mock_regs[HMC5883L_REG_STATUS] |= HMC5883L_SR_LOCK_MASK; /* Set LOCK */
                    bytes_read_since_last_lock = 0;
                }
                bytes_read_since_last_lock++;

                /* LOCK automatically clears once all 6 measurement registers are completely read */
                if (bytes_read_since_last_lock >= 6) {
                    register_lock_active = false;
                    mock_regs[HMC5883L_REG_STATUS] &= ~HMC5883L_SR_LOCK_MASK; /* Clear LOCK */
                    mock_regs[HMC5883L_REG_STATUS] &= ~HMC5883L_SR_RDY_MASK;  /* Clear RDY */
                    bytes_read_since_last_lock = 0;
                }
            }

            /* HMC5883L auto-increments the register pointer */
            curr_ptr++;
            /* Auto-wrap pointer if it overflows beyond Identification Register C */
            if (curr_ptr > HMC5883L_REG_IDC) {
                curr_ptr = HMC5883L_REG_DATA_X_MSB; /* Repoint back to Data X MSB */
            }
        } else {
            dest[i] = 0x00; /* Invalid register accesses return 0 (page 108) */
        }
    }

    return HAL_OK;
}

hal_status_t mock_hmc5883l_bus_write(void *bus, uint8_t dev_addr, uint8_t reg_addr, const uint8_t *src, size_t len, uint32_t timeout) {
    (void)bus;
    (void)timeout;

    if (dev_addr != HMC5883L_I2C_ADDRESS) {
        return HAL_ERR_NACK;
    }

    if (bus_fault_injection != HAL_OK) {
        return bus_fault_injection;
    }

    if (src == NULL || len == 0) {
        return HAL_ERR_INVALID_PARAM;
    }

    uint8_t curr_ptr = reg_addr;
    for (size_t i = 0; i < len; i++) {
        if (curr_ptr < 13) {
            /* Data Output, Status, and Identification registers are Read-Only; writes are ignored */
            if (curr_ptr <= HMC5883L_REG_MODE) {
                mock_regs[curr_ptr] = src[i];

                /* Trigger simulated measurements on operating mode updates */
                if (curr_ptr == HMC5883L_REG_MODE) {
                    uint8_t mode_val = src[i] & HMC5883L_MR_MD_MASK;

                    if (mode_val == 0x00) {
                        /* Continuous Measurement Mode trigger */
                        update_measurement_registers();
                    } else if (mode_val == 0x01) {
                        /* 
                         * Single Measurement Mode trigger.
                         * Device takes a single measurement, updates output registers,
                         * sets RDY high, and immediately transitions back to Idle (0x02).
                         */
                        update_measurement_registers();
                        mock_regs[HMC5883L_REG_MODE] = 0x02; /* Automatically returns to Idle Mode */
                    }
                }

                /* Changing CRA configuration automatically clears data register locks */
                if (curr_ptr == HMC5883L_REG_CRA) {
                    register_lock_active = false;
                    mock_regs[HMC5883L_REG_STATUS] &= ~HMC5883L_SR_LOCK_MASK;
                    bytes_read_since_last_lock = 0;
                }
            }
            curr_ptr++;
        }
    }

    return HAL_OK;
}
