/**
 * @file hmc5883l.c
 * @brief Core Driver Implementation for the Honeywell HMC5883L 3-Axis Digital Compass.
 * @details Implements hardware-agnostic control loops, register configuration,
 *          and raw measurement retrieval sequences. All hardware transactions are
 *          abstracted through the virtual bus interface in the stateless handle structure.
 *
 * @note This file is part of the ANSA Hardware Abstraction Layer (HAL) Driver Framework.
 *       It strictly enforces the no-global-state and raw-only sensor read pipelines.
 *
 * .
 */

#include "hmc5883l.h"
#include "hmc5883l_registers.h"

/*============================================================================*
 *                        EXTERNAL & WEAK DECLARATIONS                        *
 *============================================================================*/

#ifndef ANSA_TIMER_H
/**
 * @brief External microsecond-precision hardware clock interface.
 * @details Sourced from Section 2.5 of the ANSA HAL Specification.
 */
extern uint32_t hal_timer_now_us(void *ctx);
#endif

#ifndef ANSA_DELAY_H
/**
 * @brief External millisecond-precision blocking delay abstraction.
 */
extern void hal_delay_ms(uint32_t ms);
#endif

/*============================================================================*
 *                 UNIFIED DRIVER REGISTER FRAMEWORK INTERFACE                 *
 *============================================================================*/

#ifndef ANSA_DRIVER_IF_H
#define ANSA_DRIVER_IF_H

typedef struct driver_instance driver_instance_t;

/**
 * @brief Driver operations vtable.
 * @details Sourced from Section 3.2 of the ANSA HAL Specification.
 */
typedef struct {
    hal_status_t (*init)(driver_instance_t *self, const void *cfg);
    hal_status_t (*deinit)(driver_instance_t *self);
    hal_status_t (*reset)(driver_instance_t *self);
    hal_status_t (*read)(driver_instance_t *self, void *out, size_t out_len);
    hal_status_t (*write)(driver_instance_t *self, const void *in, size_t in_len);
    hal_status_t (*configure)(driver_instance_t *self, uint32_t param_id, uint32_t value);
    hal_status_t (*self_test)(driver_instance_t *self);
    hal_status_t (*get_health)(driver_instance_t *self, uint8_t *health_0_100);
    void         (*irq_handler)(driver_instance_t *self, uint32_t event_flags);
    hal_status_t (*shutdown)(driver_instance_t *self);
} driver_ops_t;

/**
 * @brief Unified Device Manager registry node structure.
 */
struct driver_instance {
    const char         *name;         /**< Instance logical name, e.g. "hmc5883l_mag0" */
    const driver_ops_t *ops;          /**< Pointer to operations vtable */
    driver_state_t      state;        /**< Core FSM tracking state */
    void               *priv;         /**< Chip-specific private handle context */
    uint32_t            fault_count;  /**< Rolling bus/comms error counter */
    uint32_t            last_error;   /**< Last captured transaction status */
};

/**
 * @brief Constructor-style static registration mechanism.
 * @details Sourced from Section 3.3 of the ANSA HAL Specification.
 */
#define DRIVER_REGISTER(drv_name, ops_ptr) \
    static driver_instance_t _drv_##drv_name = { \
        .name = #drv_name, .ops = (ops_ptr), .state = DRV_STATE_UNINIT, .priv = NULL, .fault_count = 0, .last_error = 0 }; \
    static void _register_##drv_name(void) __attribute__((constructor)); \
    static void _register_##drv_name(void) { device_manager_register_driver(&_drv_##drv_name); }

#endif /* ANSA_DRIVER_IF_H */

/**
 * @brief Weak registration fallback stub.
 * @details Allows driver-level host compilation without compiling or linking the Device Manager core.
 */
extern void device_manager_register_driver(driver_instance_t *drv);
__attribute__((weak)) void device_manager_register_driver(driver_instance_t *drv) {
    (void)drv; /* Unused fallback */
}

/*============================================================================*
 *                         CORE DRIVER IMPLEMENTATION                         *
 *============================================================================*/

hal_status_t HMC5883L_Init(HMC5883L_Handle_t *handle) {
    /* Step 1: Enforce parameter validation constraints */
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (handle->bus_read == NULL || handle->bus_write == NULL) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = HAL_ERR_INVALID_PARAM;
        return HAL_ERR_INVALID_PARAM;
    }

    handle->state = DRV_STATE_INITIALIZING;
    handle->fault_count = 0;
    handle->last_error  = HAL_OK;

    /* Capture the caller's requested run configuration BEFORE calling
     * HMC5883L_Reset() below: Reset() intentionally snaps handle->averaging/
     * odr/bias/gain/mode back to power-on-default values to mirror what the
     * *device* looks like right after a reset. Reading handle-> fields for
     * Step 4/5/6 *after* that call would silently apply those hardware
     * defaults instead of what the caller actually asked for. */
    HMC5883L_Odr_t      requested_odr       = handle->odr;
    HMC5883L_Averages_t requested_averaging = handle->averaging;
    HMC5883L_Gain_t      requested_gain     = handle->gain;
    HMC5883L_Mode_t      requested_mode     = handle->mode;

    /* Step 2: Query device identity via chip-ID checks before applying configuration */
    uint8_t id_a = 0, id_b = 0, id_c = 0;
    hal_status_t status = HMC5883L_ReadDeviceID(handle, &id_a, &id_b, &id_c);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    if (id_a != HMC5883L_IDA_EXPECTED_VAL ||
        id_b != HMC5883L_IDB_EXPECTED_VAL ||
        id_c != HMC5883L_IDC_EXPECTED_VAL) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = HAL_ERR_HW_FAULT;
        return HAL_ERR_HW_FAULT;
    }

    /* Step 3: Reset register profiles to defaults */
    status = HMC5883L_Reset(handle);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    /* Step 4: Apply user-configured run configuration parameters (the
     * requested_* locals captured above, NOT handle->odr/averaging/gain/
     * mode, which Reset() just overwrote with power-on defaults). */
    status = HMC5883L_SetOdrAndAveraging(handle, requested_odr, requested_averaging);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    status = HMC5883L_SetGain(handle, requested_gain);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    status = HMC5883L_SetMode(handle, requested_mode);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    /* Step 5: Complete lifecycle transition to READY or ACTIVE depending on mode */
    if (handle->mode == HMC5883L_MODE_CONTINUOUS || handle->mode == HMC5883L_MODE_SINGLE) {
        handle->state = DRV_STATE_ACTIVE;
    } else {
        handle->state = DRV_STATE_READY;
    }

    handle->last_error = HAL_OK;
    return HAL_OK;
}

hal_status_t HMC5883L_DeInit(HMC5883L_Handle_t *handle) {
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Force chip into idle mode to conserve power */
    hal_status_t status = HMC5883L_SetMode(handle, HMC5883L_MODE_IDLE);
    if (status != HAL_OK) {
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    handle->state = DRV_STATE_SHUTDOWN;
    handle->last_error = HAL_OK;
    return HAL_OK;
}

hal_status_t HMC5883L_ReadDeviceID(HMC5883L_Handle_t *handle, uint8_t *id_a, uint8_t *id_b, uint8_t *id_c) {
    if (handle == NULL || id_a == NULL || id_b == NULL || id_c == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    uint8_t buffer[3] = {0};
    /* Perform a 3-byte burst read starting from Identification Register A */
    hal_status_t status = handle->bus_read(handle->bus_handle,
                                           handle->dev_address,
                                           HMC5883L_REG_IDA,
                                           buffer,
                                           3,
                                           handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    *id_a = buffer[0];
    *id_b = buffer[1];
    *id_c = buffer[2];

    return HAL_OK;
}

hal_status_t HMC5883L_Reset(HMC5883L_Handle_t *handle) {
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Reset Configuration Register A power-up default: 0x10 */
    uint8_t cra_default = 0x10;
    hal_status_t status = handle->bus_write(handle->bus_handle,
                                            handle->dev_address,
                                            HMC5883L_REG_CRA,
                                            &cra_default,
                                            1,
                                            handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    /* Reset Configuration Register B power-up default: 0x20 */
    uint8_t crb_default = 0x20;
    status = handle->bus_write(handle->bus_handle,
                               handle->dev_address,
                               HMC5883L_REG_CRB,
                               &crb_default,
                               1,
                               handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    /* Reset Mode Register power-up default: 0x01 (Single-Measurement Mode) */
    uint8_t mode_default = 0x01;
    status = handle->bus_write(handle->bus_handle,
                               handle->dev_address,
                               HMC5883L_REG_MODE,
                               &mode_default,
                               1,
                               handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    /* Update driver state fields to default values */
    handle->averaging = HMC5883L_AVG_1;
    handle->odr       = HMC5883L_ODR_15_HZ;
    handle->bias      = HMC5883L_BIAS_NORMAL;
    handle->gain      = HMC5883L_GAIN_1_3_GA;
    handle->mode      = HMC5883L_MODE_SINGLE;
    handle->state     = DRV_STATE_READY;
    handle->last_error = HAL_OK;

    return HAL_OK;
}

hal_status_t HMC5883L_ReadData(HMC5883L_Handle_t *handle, HMC5883L_RawData_t *data) {
    if (handle == NULL || data == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    if (handle->state == DRV_STATE_UNINIT) {
        return HAL_ERR_NOT_INITIALIZED;
    }

    uint8_t buffer[6] = {0};
    /* Perform a 6-byte burst read starting from the first data register (X MSB) */
    hal_status_t status = handle->bus_read(handle->bus_handle,
                                           handle->dev_address,
                                           HMC5883L_REG_DATA_X_MSB,
                                           buffer,
                                           6,
                                           handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->state = DRV_STATE_FAULT;
        handle->last_error = status;
        return status;
    }

    /* 
     * Reassemble values from big-endian 2's complement integers.
     * Note: HMC5883L registers sequence is X, Z, Y (not X, Y, Z).
     */
    int16_t x_raw = (int16_t)((buffer[0] << 8) | buffer[1]);
    int16_t z_raw = (int16_t)((buffer[2] << 8) | buffer[3]);
    int16_t y_raw = (int16_t)((buffer[4] << 8) | buffer[5]);

    /* Assign decoded results strictly in physical axis alignment */
    data->x_raw = x_raw;
    data->y_raw = y_raw;
    data->z_raw = z_raw;

    /* Capture hardware timestamp at the bottom-most boundary */
    data->timestamp_us = (uint64_t)hal_timer_now_us(NULL);

    handle->state = DRV_STATE_ACTIVE;
    handle->last_error = HAL_OK;

    return HAL_OK;
}

hal_status_t HMC5883L_SetGain(HMC5883L_Handle_t *handle, HMC5883L_Gain_t gain) {
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    if ((uint8_t)gain > 0x07) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Shift gain settings into Bits 7-5 of CRB; clear Bits 4-0 */
    uint8_t crb_val = (uint8_t)(((gain << HMC5883L_CRB_GN_SHIFT) & HMC5883L_CRB_GN_MASK));
    hal_status_t status = handle->bus_write(handle->bus_handle,
                                            handle->dev_address,
                                            HMC5883L_REG_CRB,
                                            &crb_val,
                                            1,
                                            handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    handle->gain = gain;
    handle->last_error = HAL_OK;
    return HAL_OK;
}

hal_status_t HMC5883L_SetMode(HMC5883L_Handle_t *handle, HMC5883L_Mode_t mode) {
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    if ((uint8_t)mode > 0x02) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Mode selector mapped directly to bits [1:0] of Mode Register */
    uint8_t mr_val = (uint8_t)(mode & HMC5883L_MR_MD_MASK);
    hal_status_t status = handle->bus_write(handle->bus_handle,
                                            handle->dev_address,
                                            HMC5883L_REG_MODE,
                                            &mr_val,
                                            1,
                                            handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    handle->mode = mode;
    handle->last_error = HAL_OK;
    return HAL_OK;
}

hal_status_t HMC5883L_SetOdrAndAveraging(HMC5883L_Handle_t *handle, HMC5883L_Odr_t odr, HMC5883L_Averages_t averaging) {
    if (handle == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    if ((uint8_t)odr > 0x06 || (uint8_t)averaging > 0x03) {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Assemble configuration bits: Average Count [6:5], Data Rate [4:2], Bias Polarity [1:0] */
    uint8_t cra_val = (uint8_t)(
        ((averaging << HMC5883L_CRA_MA_SHIFT) & HMC5883L_CRA_MA_MASK) |
        ((odr << HMC5883L_CRA_DO_SHIFT) & HMC5883L_CRA_DO_MASK) |
        ((handle->bias << HMC5883L_CRA_MS_SHIFT) & HMC5883L_CRA_MS_MASK)
    );

    hal_status_t status = handle->bus_write(handle->bus_handle,
                                            handle->dev_address,
                                            HMC5883L_REG_CRA,
                                            &cra_val,
                                            1,
                                            handle->timeout_ms);
    if (status != HAL_OK) {
        handle->fault_count++;
        handle->last_error = status;
        return status;
    }

    handle->averaging = averaging;
    handle->odr       = odr;
    handle->last_error = HAL_OK;
    return HAL_OK;
}

hal_status_t HMC5883L_RunSelfTest(HMC5883L_Handle_t *handle, bool *pass_out) {
    if (handle == NULL || pass_out == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    *pass_out = false;

    /* Step 1: Cache active hardware configuration profiles */
    HMC5883L_Averages_t orig_avg  = handle->averaging;
    HMC5883L_Odr_t      orig_odr  = handle->odr;
    HMC5883L_Bias_t     orig_bias = handle->bias;
    HMC5883L_Gain_t     orig_gain = handle->gain;
    HMC5883L_Mode_t     orig_mode = handle->mode;

    /* Step 2: Configure Positive Bias self-test mode with 8-averaging & 15Hz ODR (CRA = 0x71) */
    handle->bias = HMC5883L_BIAS_POSITIVE;
    hal_status_t status = HMC5883L_SetOdrAndAveraging(handle, HMC5883L_ODR_15_HZ, HMC5883L_AVG_8);
    if (status != HAL_OK) {
        goto restore_configs;
    }

    /* Step 3: Configure recommended amplification Gain = 5 (CRB = 0xA0) */
    status = HMC5883L_SetGain(handle, HMC5883L_GAIN_4_7_GA);
    if (status != HAL_OK) {
        goto restore_configs;
    }

    /* 
     * Step 4: Perform double acquisition cycle to bypass gain change latency.
     * The very first measurement after gain settings update reflects the previous gain.
     */
    
    /* Cycle 1: Trigger measurement and wait */
    status = HMC5883L_SetMode(handle, HMC5883L_MODE_SINGLE);
    if (status != HAL_OK) {
        goto restore_configs;
    }
    hal_delay_ms(10);

    HMC5883L_RawData_t dummy_data;
    status = HMC5883L_ReadData(handle, &dummy_data);
    if (status != HAL_OK) {
        goto restore_configs;
    }

    /* Cycle 2: Trigger the actual self-test measurement */
    status = HMC5883L_SetMode(handle, HMC5883L_MODE_SINGLE);
    if (status != HAL_OK) {
        goto restore_configs;
    }
    hal_delay_ms(10);

    HMC5883L_RawData_t test_data;
    status = HMC5883L_ReadData(handle, &test_data);
    if (status != HAL_OK) {
        goto restore_configs;
    }

    /* Step 5: Check positive bias limits (X/Y/Z must be between 243 and 575 LSBs for Gain=5) */
    bool test_passed = true;
    if (test_data.x_raw < 243 || test_data.x_raw > 575) {
        test_passed = false;
    }
    if (test_data.y_raw < 243 || test_data.y_raw > 575) {
        test_passed = false;
    }
    if (test_data.z_raw < 243 || test_data.z_raw > 575) {
        test_passed = false;
    }

    *pass_out = test_passed;

restore_configs:
    /* Step 6: Restore operational parameters to cached presets */
    handle->bias = orig_bias;
    (void)HMC5883L_SetOdrAndAveraging(handle, orig_odr, orig_avg);
    (void)HMC5883L_SetGain(handle, orig_gain);
    (void)HMC5883L_SetMode(handle, orig_mode);

    handle->last_error = status;
    return status;
}

/*============================================================================*
 *                DEVICE MANAGER INTEGRATION: DRIVER OPS TABLE                *
 *============================================================================*/

static hal_status_t hmc5883l_drv_init(driver_instance_t *self, const void *cfg) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    
    /* Auto-bind target configuration from Device Manager context if supplied */
    if (cfg != NULL) {
        const HMC5883L_Handle_t *cfg_handle = (const HMC5883L_Handle_t *)cfg;
        handle->bus_handle  = cfg_handle->bus_handle;
        handle->dev_address = cfg_handle->dev_address;
        handle->timeout_ms  = cfg_handle->timeout_ms;
        handle->averaging   = cfg_handle->averaging;
        handle->odr         = cfg_handle->odr;
        handle->bias        = cfg_handle->bias;
        handle->gain        = cfg_handle->gain;
        handle->mode        = cfg_handle->mode;
        handle->bus_read    = cfg_handle->bus_read;
        handle->bus_write   = cfg_handle->bus_write;
    }

    hal_status_t status = HMC5883L_Init(handle);
    self->state = handle->state;
    self->last_error = (uint32_t)status;
    return status;
}

static hal_status_t hmc5883l_drv_deinit(driver_instance_t *self) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    hal_status_t status = HMC5883L_DeInit(handle);
    self->state = handle->state;
    self->last_error = (uint32_t)status;
    return status;
}

static hal_status_t hmc5883l_drv_reset(driver_instance_t *self) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    hal_status_t status = HMC5883L_Reset(handle);
    self->state = handle->state;
    self->last_error = (uint32_t)status;
    return status;
}

static hal_status_t hmc5883l_drv_read(driver_instance_t *self, void *out, size_t out_len) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (out_len < sizeof(HMC5883L_RawData_t)) {
        return HAL_ERR_INVALID_PARAM;
    }

    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    hal_status_t status = HMC5883L_ReadData(handle, (HMC5883L_RawData_t *)out);
    self->state = handle->state;
    self->fault_count = handle->fault_count;
    self->last_error = (uint32_t)status;
    return status;
}

static hal_status_t hmc5883l_drv_configure(driver_instance_t *self, uint32_t param_id, uint32_t value) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    hal_status_t status = HAL_OK;

    switch (param_id) {
        case 1: /* Configuration Param 1: Set Gain */
            status = HMC5883L_SetGain(handle, (HMC5883L_Gain_t)value);
            break;
        case 2: /* Configuration Param 2: Set Mode */
            status = HMC5883L_SetMode(handle, (HMC5883L_Mode_t)value);
            break;
        case 3: /* Configuration Param 3: Set Rate & Average */
            status = HMC5883L_SetOdrAndAveraging(handle, (HMC5883L_Odr_t)(value & 0xFFFF), (HMC5883L_Averages_t)((value >> 16) & 0xFFFF));
            break;
        default:
            status = HAL_ERR_UNSUPPORTED;
            break;
    }

    self->state = handle->state;
    self->last_error = (uint32_t)status;
    return status;
}

static hal_status_t hmc5883l_drv_self_test(driver_instance_t *self) {
    if (self == NULL || self->priv == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }
    HMC5883L_Handle_t *handle = (HMC5883L_Handle_t *)self->priv;
    bool pass = false;
    hal_status_t status = HMC5883L_RunSelfTest(handle, &pass);
    
    self->state = handle->state;
    self->last_error = (uint32_t)status;

    if (status != HAL_OK || !pass) {
        return HAL_ERR_HW_FAULT;
    }
    return HAL_OK;
}

static hal_status_t hmc5883l_drv_get_health(driver_instance_t *self, uint8_t *health_0_100) {
    if (self == NULL || health_0_100 == NULL) {
        return HAL_ERR_INVALID_PARAM;
    }

    if (self->state == DRV_STATE_FAULT) {
        *health_0_100 = 0;
    } else if (self->fault_count > 10) {
        *health_0_100 = 20; /* Decidedly degraded and trending failure */
    } else if (self->fault_count > 0) {
        *health_0_100 = 70; /* Slightly degraded but active */
    } else if (self->state == DRV_STATE_ACTIVE) {
        *health_0_100 = 100;
    } else if (self->state == DRV_STATE_READY) {
        *health_0_100 = 90;
    } else {
        *health_0_100 = 50;
    }

    return HAL_OK;
}

/* Explicit binding of Device Manager driver callback operations */
static const driver_ops_t hmc5883l_ops = {
    .init        = hmc5883l_drv_init,
    .deinit      = hmc5883l_drv_deinit,
    .reset       = hmc5883l_drv_reset,
    .read        = hmc5883l_drv_read,
    .write       = NULL, /* Sensor is Read-Only for logical measurements */
    .configure   = hmc5883l_drv_configure,
    .self_test   = hmc5883l_drv_self_test,
    .get_health  = hmc5883l_drv_get_health,
    .irq_handler = NULL, /* Read polling driven */
    .shutdown    = hmc5883l_drv_deinit
};

/* Auto-register driver instance hmc5883l_mag0 with the Device Manager */
DRIVER_REGISTER(hmc5883l_mag0, &hmc5883l_ops)
/* NOTE: no trailing ';' — the macro's last line is a function definition,
 * so a trailing ';' here would be a stray empty declaration at file scope
 * (-Wpedantic correctly flags it). */
