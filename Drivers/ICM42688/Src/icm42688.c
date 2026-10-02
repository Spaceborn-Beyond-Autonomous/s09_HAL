/**
 * @file    icm42688.c
 * @brief   Driver implementation for the ICM-42688-P 6-axis IMU.
 * @details Every public entry point null-checks its handle, threads
 *          bus_context through unexamined, and checks the return of every
 *          single ICM42688_BusRead/BusWrite call (DRIVER_STANDARD.md
 *          Section 4 & 5) before touching the handle's cached state.
 */

#include "icm42688.h"
#include "icm42688_registers.h"
#include "icm42688_bus.h"
#include <string.h>

/*============================================================================*
 *                              INTERNAL HELPERS                              *
 *============================================================================*/

/** @brief NULL / not-yet-initialized guard shared by every call that needs
 *         a running device (everything except Init itself). */
static ICM42688_Status_t validate_running(const ICM42688_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }
    if (!handle->initialized) {
        return ICM42688_ERROR_NOT_INITIALIZED;
    }
    return ICM42688_OK;
}

/** @brief Records a status on the handle and, if it was a bus fault, bumps
 *         the rolling fault counter used for health reporting. */
static ICM42688_Status_t record_status(ICM42688_Handle_t *handle, ICM42688_Status_t status)
{
    handle->last_error = status;
    if (status == ICM42688_ERROR_BUS || status == ICM42688_ERROR_TIMEOUT) {
        handle->fault_count++;
    }
    return status;
}

static int16_t combine_be16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
}

/*============================================================================*
 *                             REQUIRED API SET                               *
 *============================================================================*/

ICM42688_Status_t ICM42688_Init(ICM42688_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }

    handle->initialized  = false;
    handle->fifo_enabled = false;
    handle->fault_count  = 0U;
    if (handle->timeout_ms == 0U) {
        handle->timeout_ms = 100U; /* sane default if the caller left it zero */
    }

    ICM42688_Status_t status = ICM42688_Reset(handle);
    if (status != ICM42688_OK) {
        return record_status(handle, status);
    }
    ICM42688_DelayMs(2U); /* device datasheet reset settle time */

    uint8_t who_am_i = 0U;
    status = ICM42688_ReadDeviceID(handle, &who_am_i);
    if (status != ICM42688_OK) {
        return record_status(handle, status);
    }
    if (who_am_i != ICM42688_WHO_AM_I_VALUE) {
        return record_status(handle, ICM42688_ERROR_WRONG_DEVICE_ID);
    }

    /* Apply the handle's requested full-scale ranges (default-constructed
     * handles read as 0 == ICM42688_ACCEL_FS_16G / ICM42688_GYRO_FS_2000DPS,
     * which is intentional: widest range first, callers narrow it later). */
    status = ICM42688_SetAccelFS(handle, handle->accel_fs);
    if (status != ICM42688_OK) {
        return record_status(handle, status);
    }
    status = ICM42688_SetGyroFS(handle, handle->gyro_fs);
    if (status != ICM42688_OK) {
        return record_status(handle, status);
    }

    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_PWR_MGMT0,
                           ICM42688_PWR_MGMT0_DEFAULT_RUN) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    ICM42688_DelayMs(1U); /* mode-change settle time */

    handle->initialized = true;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_DeInit(ICM42688_Handle_t *handle)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }

    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_PWR_MGMT0,
                           ICM42688_PWR_MGMT0_SLEEP) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }

    handle->initialized  = false;
    handle->fifo_enabled = false;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_ReadDeviceID(ICM42688_Handle_t *handle, uint8_t *id)
{
    if (handle == NULL || handle->bus_context == NULL || id == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }
    if (ICM42688_BusRead(handle->bus_context, ICM42688_REG_WHO_AM_I, id, 1U) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    return ICM42688_OK;
}

ICM42688_Status_t ICM42688_Reset(ICM42688_Handle_t *handle)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }
    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_DEVICE_CONFIG,
                           ICM42688_DEVICE_CONFIG_SOFT_RESET) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    return ICM42688_OK;
}

ICM42688_Status_t ICM42688_ReadData(ICM42688_Handle_t *handle, ICM42688_RawData_t *data)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }
    if (data == NULL) {
        return record_status(handle, ICM42688_ERROR_INVALID_PARAM);
    }

    /* TEMP_DATA1(0x1D)..GYRO_DATA_Z0(0x2A) are 14 contiguous bytes:
     * [temp_h, temp_l, ax_h, ax_l, ay_h, ay_l, az_h, az_l,
     *  gx_h, gx_l, gy_h, gy_l, gz_h, gz_l] */
    uint8_t raw[14];
    if (ICM42688_BusRead(handle->bus_context, ICM42688_REG_TEMP_DATA1, raw, sizeof(raw)) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }

    data->temp_raw     = combine_be16(raw[0],  raw[1]);
    data->accel_x_raw  = combine_be16(raw[2],  raw[3]);
    data->accel_y_raw  = combine_be16(raw[4],  raw[5]);
    data->accel_z_raw  = combine_be16(raw[6],  raw[7]);
    data->gyro_x_raw   = combine_be16(raw[8],  raw[9]);
    data->gyro_y_raw   = combine_be16(raw[10], raw[11]);
    data->gyro_z_raw   = combine_be16(raw[12], raw[13]);

    return record_status(handle, ICM42688_OK);
}

/*============================================================================*
 *                          SENSOR-SPECIFIC EXTRAS                            *
 *============================================================================*/

ICM42688_Status_t ICM42688_SetAccelFS(ICM42688_Handle_t *handle, ICM42688_AccelFs_t fs)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }
    uint8_t reg_val = (uint8_t)(((uint8_t)fs << ICM42688_FS_SEL_SHIFT) & ICM42688_FS_SEL_MASK);
    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_ACCEL_CONFIG0, reg_val) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    handle->accel_fs = fs;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_SetGyroFS(ICM42688_Handle_t *handle, ICM42688_GyroFs_t fs)
{
    if (handle == NULL || handle->bus_context == NULL) {
        return ICM42688_ERROR_INVALID_PARAM;
    }
    uint8_t reg_val = (uint8_t)(((uint8_t)fs << ICM42688_FS_SEL_SHIFT) & ICM42688_FS_SEL_MASK);
    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_GYRO_CONFIG0, reg_val) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    handle->gyro_fs = fs;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_FifoEnable(ICM42688_Handle_t *handle)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }
    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_FIFO_CONFIG,
                           ICM42688_FIFO_MODE_STREAM) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    handle->fifo_enabled = true;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_FifoDisable(ICM42688_Handle_t *handle)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }
    if (ICM42688_BusWrite(handle->bus_context, ICM42688_REG_FIFO_CONFIG,
                           ICM42688_FIFO_MODE_BYPASS) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    handle->fifo_enabled = false;
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_FifoGetCount(ICM42688_Handle_t *handle, uint16_t *count)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }
    if (count == NULL) {
        return record_status(handle, ICM42688_ERROR_INVALID_PARAM);
    }

    uint8_t raw[2];
    if (ICM42688_BusRead(handle->bus_context, ICM42688_REG_FIFO_COUNTH, raw, sizeof(raw)) != 0) {
        return record_status(handle, ICM42688_ERROR_BUS);
    }
    *count = (uint16_t)(((uint16_t)raw[0] << 8) | (uint16_t)raw[1]);
    return record_status(handle, ICM42688_OK);
}

ICM42688_Status_t ICM42688_FifoRead(ICM42688_Handle_t *handle, ICM42688_RawData_t *out,
                                     uint16_t max_packets, uint16_t *packets_read)
{
    ICM42688_Status_t guard = validate_running(handle);
    if (guard != ICM42688_OK) {
        return guard;
    }
    if (out == NULL || packets_read == NULL) {
        return record_status(handle, ICM42688_ERROR_INVALID_PARAM);
    }
    if (!handle->fifo_enabled) {
        return record_status(handle, ICM42688_ERROR_NOT_READY);
    }

    uint16_t available_bytes = 0U;
    ICM42688_Status_t status = ICM42688_FifoGetCount(handle, &available_bytes);
    if (status != ICM42688_OK) {
        return status;
    }

    uint16_t available_packets = (uint16_t)(available_bytes / ICM42688_FIFO_PACKET_BYTES);
    uint16_t to_read = (available_packets < max_packets) ? available_packets : max_packets;

    for (uint16_t i = 0U; i < to_read; i++) {
        uint8_t raw[12];
        if (ICM42688_BusRead(handle->bus_context, ICM42688_REG_FIFO_DATA, raw, sizeof(raw)) != 0) {
            *packets_read = i;
            return record_status(handle, ICM42688_ERROR_BUS);
        }
        out[i].accel_x_raw = combine_be16(raw[0], raw[1]);
        out[i].accel_y_raw = combine_be16(raw[2], raw[3]);
        out[i].accel_z_raw = combine_be16(raw[4], raw[5]);
        out[i].gyro_x_raw  = combine_be16(raw[6], raw[7]);
        out[i].gyro_y_raw  = combine_be16(raw[8], raw[9]);
        out[i].gyro_z_raw  = combine_be16(raw[10], raw[11]);
        out[i].temp_raw    = 0; /* not carried in the FIFO packet */
    }

    *packets_read = to_read;
    return record_status(handle, ICM42688_OK);
}
