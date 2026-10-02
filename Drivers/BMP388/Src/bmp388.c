/**
 * @file    bmp388.c
 * @brief   Driver implementation for the Bosch BMP388 barometric sensor.
 * @details Every public entry point null-checks its handle, threads
 *          bus_context through unexamined, and checks the return of every
 *          single BMP388_BusRead/BusWrite call before touching cached
 *          state (DRIVER_STANDARD.md Section 4 & 5).
 *
 *          The temperature/pressure compensation math below reproduces
 *          Bosch's published floating-point compensation formula (the same
 *          one used by Bosch's own BMP3-Sensor-API reference driver) —
 *          this project does not re-derive that formula from scratch, it
 *          reimplements the documented one against a handle-scoped
 *          BMP388_Calib_t instead of a static/global calibration block.
 */

#include "bmp388.h"
#include "bmp388_registers.h"
#include "bmp388_bus.h"
#include <string.h>

/*============================================================================*
 *                              INTERNAL HELPERS                              *
 *============================================================================*/

static BMP388_Status_t validate_running(const BMP388_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized) {
        return BMP388_ERROR_NOT_INITIALIZED;
    }
    return BMP388_OK;
}

static BMP388_Status_t record_status(BMP388_Handle_t *handle, BMP388_Status_t status)
{
    handle->last_error = status;
    if (status == BMP388_ERROR_BUS || status == BMP388_ERROR_TIMEOUT) {
        handle->fault_count++;
    }
    return status;
}

static uint16_t parse_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t parse_s16_le(const uint8_t *p)
{
    return (int16_t)parse_u16_le(p);
}

/**
 * @brief Converts the 21-byte raw NVM block into the floating-point trim
 *        coefficients used by the compensation formulas below, following
 *        the scale factors from Bosch's published BMP388 driver.
 */
static void parse_calibration(const uint8_t *raw21, BMP388_Calib_t *out)
{
    uint16_t nvm_par_t1 = parse_u16_le(&raw21[0]);
    uint16_t nvm_par_t2 = parse_u16_le(&raw21[2]);
    int8_t   nvm_par_t3 = (int8_t)raw21[4];

    int16_t  nvm_par_p1 = parse_s16_le(&raw21[5]);
    int16_t  nvm_par_p2 = parse_s16_le(&raw21[7]);
    int8_t   nvm_par_p3 = (int8_t)raw21[9];
    int8_t   nvm_par_p4 = (int8_t)raw21[10];
    uint16_t nvm_par_p5 = parse_u16_le(&raw21[11]);
    uint16_t nvm_par_p6 = parse_u16_le(&raw21[13]);
    int8_t   nvm_par_p7 = (int8_t)raw21[15];
    int8_t   nvm_par_p8 = (int8_t)raw21[16];
    int16_t  nvm_par_p9 = parse_s16_le(&raw21[17]);
    int8_t   nvm_par_p10 = (int8_t)raw21[19];
    int8_t   nvm_par_p11 = (int8_t)raw21[20];

    out->par_t1 = (double)nvm_par_t1 * 256.0;
    out->par_t2 = (double)nvm_par_t2 / 1073741824.0;
    out->par_t3 = (double)nvm_par_t3 / 281474976710656.0;

    out->par_p1  = ((double)nvm_par_p1 - 16384.0) / 1048576.0;
    out->par_p2  = ((double)nvm_par_p2 - 16384.0) / 536870912.0;
    out->par_p3  = (double)nvm_par_p3 / 4294967296.0;
    out->par_p4  = (double)nvm_par_p4 / 137438953472.0;
    out->par_p5  = (double)nvm_par_p5 * 8.0;
    out->par_p6  = (double)nvm_par_p6 / 64.0;
    out->par_p7  = (double)nvm_par_p7 / 256.0;
    out->par_p8  = (double)nvm_par_p8 / 32768.0;
    out->par_p9  = (double)nvm_par_p9 / 281474976710656.0;
    out->par_p10 = (double)nvm_par_p10 / 281474976710656.0;
    out->par_p11 = (double)nvm_par_p11 / 36893488147419103232.0;
}

/** @brief Bosch compensation, temperature stage. Returns t_lin (degrees C),
 *         which the pressure stage also needs as an input. */
static double compensate_temperature(uint32_t uncomp_temp, const BMP388_Calib_t *c)
{
    double partial_data1 = (double)uncomp_temp - c->par_t1;
    double partial_data2 = partial_data1 * c->par_t2;
    return partial_data2 + (partial_data1 * partial_data1) * c->par_t3;
}

/** @brief Bosch compensation, pressure stage. t_lin must come from
 *         compensate_temperature() run on the same sample. */
static double compensate_pressure(uint32_t uncomp_press, double t_lin, const BMP388_Calib_t *c)
{
    double t2 = t_lin * t_lin;
    double t3 = t2 * t_lin;

    double partial_out1 = c->par_p5 + (c->par_p6 * t_lin) + (c->par_p7 * t2) + (c->par_p8 * t3);

    double partial_out2 = (double)uncomp_press *
        (c->par_p1 + (c->par_p2 * t_lin) + (c->par_p3 * t2) + (c->par_p4 * t3));

    double p2 = (double)uncomp_press * (double)uncomp_press;
    double p3 = p2 * (double)uncomp_press;
    double partial_data = (p2 * (c->par_p9 + (c->par_p10 * t_lin))) + (p3 * c->par_p11);

    return partial_out1 + partial_out2 + partial_data;
}

/**
 * @brief Sanity-checks a freshly parsed calibration block. A brand-new,
 *        never-trimmed or bus-glitched part tends to read back as all
 *        0x00 or all 0xFF, which parses into par_t1 == 0 — a real BMP388
 *        never ships with that value, so treat it as a loud, well-defined
 *        error instead of silently returning nonsense pressures later.
 */
static bool calibration_looks_sane(const BMP388_Calib_t *c)
{
    return c->par_t1 > 1.0;
}

/*============================================================================*
 *                             REQUIRED API SET                               *
 *============================================================================*/

BMP388_Status_t BMP388_Init(BMP388_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }

    handle->initialized  = false;
    handle->calib_loaded = false;
    handle->fault_count  = 0U;
    if (handle->timeout_ms == 0U) {
        handle->timeout_ms = 100U;
    }

    BMP388_Status_t status = BMP388_Reset(handle);
    if (status != BMP388_OK) {
        return record_status(handle, status);
    }
    BMP388_DelayMs(10U); /* datasheet-conservative post-reset settle time */

    uint8_t chip_id = 0U;
    status = BMP388_ReadDeviceID(handle, &chip_id);
    if (status != BMP388_OK) {
        return record_status(handle, status);
    }
    if (chip_id != BMP388_CHIP_ID_VALUE) {
        return record_status(handle, BMP388_ERROR_WRONG_DEVICE_ID);
    }

    uint8_t calib_raw[BMP388_CALIB_DATA_LEN];
    if (BMP388_BusRead(handle->bus_context, BMP388_REG_CALIB_DATA, calib_raw,
                        BMP388_CALIB_DATA_LEN) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    parse_calibration(calib_raw, &handle->calib);
    if (!calibration_looks_sane(&handle->calib)) {
        return record_status(handle, BMP388_ERROR_CALIBRATION);
    }
    handle->calib_loaded = true;

    status = BMP388_SetOversampling(handle, handle->press_osr, handle->temp_osr);
    if (status != BMP388_OK) {
        return record_status(handle, status);
    }

    if (BMP388_BusWrite(handle->bus_context, BMP388_REG_ODR, BMP388_ODR_25HZ) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    if (BMP388_BusWrite(handle->bus_context, BMP388_REG_PWR_CTRL,
                         BMP388_PWR_CTRL_DEFAULT_RUN) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }

    handle->initialized = true;
    return record_status(handle, BMP388_OK);
}

BMP388_Status_t BMP388_DeInit(BMP388_Handle_t *handle)
{
    BMP388_Status_t guard = validate_running(handle);
    if (guard != BMP388_OK) {
        return guard;
    }
    if (BMP388_BusWrite(handle->bus_context, BMP388_REG_PWR_CTRL, BMP388_PWR_MODE_SLEEP) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    handle->initialized = false;
    return record_status(handle, BMP388_OK);
}

BMP388_Status_t BMP388_ReadDeviceID(BMP388_Handle_t *handle, uint8_t *id)
{
    if (handle == NULL || handle->bus_context == NULL || id == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }
    if (BMP388_BusRead(handle->bus_context, BMP388_REG_CHIP_ID, id, 1U) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    return BMP388_OK;
}

BMP388_Status_t BMP388_Reset(BMP388_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }
    if (BMP388_BusWrite(handle->bus_context, BMP388_REG_CMD, BMP388_CMD_SOFT_RESET) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    return BMP388_OK;
}

BMP388_Status_t BMP388_ReadData(BMP388_Handle_t *handle, BMP388_Data_t *data)
{
    BMP388_Status_t guard = validate_running(handle);
    if (guard != BMP388_OK) {
        return guard;
    }
    if (data == NULL) {
        return record_status(handle, BMP388_ERROR_INVALID_PARAM);
    }
    if (!handle->calib_loaded) {
        return record_status(handle, BMP388_ERROR_CALIBRATION);
    }

    bool ready = false;
    BMP388_Status_t status = BMP388_IsDataReady(handle, &ready);
    if (status != BMP388_OK) {
        return status;
    }
    if (!ready) {
        return record_status(handle, BMP388_ERROR_NOT_READY);
    }

    uint8_t raw[6];
    if (BMP388_BusRead(handle->bus_context, BMP388_REG_DATA_0, raw, sizeof(raw)) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }

    data->pressure_raw    = ((uint32_t)raw[2] << 16) | ((uint32_t)raw[1] << 8) | (uint32_t)raw[0];
    data->temperature_raw = ((uint32_t)raw[5] << 16) | ((uint32_t)raw[4] << 8) | (uint32_t)raw[3];

    double t_lin = compensate_temperature(data->temperature_raw, &handle->calib);
    data->temperature_c = t_lin;
    data->pressure_pa   = compensate_pressure(data->pressure_raw, t_lin, &handle->calib);

    return record_status(handle, BMP388_OK);
}

/*============================================================================*
 *                          SENSOR-SPECIFIC EXTRAS                            *
 *============================================================================*/

BMP388_Status_t BMP388_SetOversampling(BMP388_Handle_t *handle,
                                        BMP388_Oversampling_t press_osr,
                                        BMP388_Oversampling_t temp_osr)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }
    uint8_t reg_val = (uint8_t)(((uint8_t)press_osr << BMP388_OSR_P_SHIFT) |
                                 ((uint8_t)temp_osr << BMP388_OSR_T_SHIFT));
    if (BMP388_BusWrite(handle->bus_context, BMP388_REG_OSR, reg_val) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    handle->press_osr = press_osr;
    handle->temp_osr  = temp_osr;
    return record_status(handle, BMP388_OK);
}

BMP388_Status_t BMP388_IsDataReady(BMP388_Handle_t *handle, bool *ready)
{
    if (handle == NULL || handle->bus_context == NULL || ready == NULL) {
        return BMP388_ERROR_INVALID_PARAM;
    }
    uint8_t status_reg = 0U;
    if (BMP388_BusRead(handle->bus_context, BMP388_REG_STATUS, &status_reg, 1U) != 0) {
        return record_status(handle, BMP388_ERROR_BUS);
    }
    *ready = ((status_reg & BMP388_STATUS_DRDY_PRESS) != 0U) &&
             ((status_reg & BMP388_STATUS_DRDY_TEMP) != 0U);
    return record_status(handle, BMP388_OK);
}
