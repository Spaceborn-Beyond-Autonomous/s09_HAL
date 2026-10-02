/**
 * @file    icm42688.h
 * @brief   Public API for the TDK InvenSense ICM-42688-P 6-axis IMU driver.
 * @details Handle-based, hardware-agnostic driver for the S09 ANSA HAL
 *          Emulator. Zero direct calls to any platform SPI/I2C API live in
 *          this header or in icm42688.c — all bus traffic goes through the
 *          ICM42688_BusRead / ICM42688_BusWrite / ICM42688_DelayMs contract
 *          declared in icm42688_bus.h, which has one implementation per
 *          backend (real STM32 SPI, or the QEMU/host mock bus).
 *
 * @note    Part of the ANSA Hardware Abstraction Layer Driver Framework
 *          (see DRIVER_STANDARD.md). Depends only on the standard library.
 */

#ifndef ICM42688_H
#define ICM42688_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              STATUS CODES                                  *
 *============================================================================*/

/**
 * @brief ICM42688 driver status/return codes.
 * @details Deliberately more granular than ok/error so callers can tell a
 *          transport failure from a bad handle from a genuine identity
 *          mismatch (DRIVER_STANDARD.md Section 5).
 */
typedef enum {
    ICM42688_OK = 0,                    /**< Operation succeeded. */
    ICM42688_ERROR_BUS,                 /**< SPI/I2C transaction failed. */
    ICM42688_ERROR_INVALID_PARAM,       /**< Null pointer or bad handle field. */
    ICM42688_ERROR_WRONG_DEVICE_ID,     /**< WHO_AM_I did not read back 0x47. */
    ICM42688_ERROR_NOT_READY,           /**< FIFO empty / data not yet available. */
    ICM42688_ERROR_TIMEOUT,             /**< Bus transaction exceeded its budget. */
    ICM42688_ERROR_NOT_INITIALIZED      /**< Called before ICM42688_Init() succeeded. */
} ICM42688_Status_t;

/*============================================================================*
 *                          CONFIGURATION ENUMS                               *
 *============================================================================*/

/** @brief Accelerometer full-scale range (ACCEL_CONFIG0 bits [7:5]). */
typedef enum {
    ICM42688_ACCEL_FS_16G = 0x00, /**< Default applied by Init(): +-16 g */
    ICM42688_ACCEL_FS_8G  = 0x01, /**< +-8 g */
    ICM42688_ACCEL_FS_4G  = 0x02, /**< +-4 g */
    ICM42688_ACCEL_FS_2G  = 0x03  /**< +-2 g, best LSB/g resolution */
} ICM42688_AccelFs_t;

/** @brief Gyroscope full-scale range (GYRO_CONFIG0 bits [7:5]). */
typedef enum {
    ICM42688_GYRO_FS_2000DPS = 0x00, /**< Default applied by Init(): +-2000 dps */
    ICM42688_GYRO_FS_1000DPS = 0x01,
    ICM42688_GYRO_FS_500DPS  = 0x02,
    ICM42688_GYRO_FS_250DPS  = 0x03,
    ICM42688_GYRO_FS_125DPS  = 0x04
} ICM42688_GyroFs_t;

/*============================================================================*
 *                         DATA STRUCTURES & HANDLE                           *
 *============================================================================*/

/**
 * @brief One raw sample: uncalibrated 2's-complement register counts.
 * @details No scale, bias, or temperature compensation is applied — exactly
 *          what came off the bus, matching the project's "raw pipeline"
 *          convention (see hmc5883l.h for the equivalent contract on the
 *          magnetometer driver).
 */
typedef struct {
    int16_t accel_x_raw;    /**< Raw X accel count. */
    int16_t accel_y_raw;    /**< Raw Y accel count. */
    int16_t accel_z_raw;    /**< Raw Z accel count. */
    int16_t gyro_x_raw;     /**< Raw X gyro count. */
    int16_t gyro_y_raw;     /**< Raw Y gyro count. */
    int16_t gyro_z_raw;     /**< Raw Z gyro count. */
    int16_t temp_raw;       /**< Raw die temperature count. */
} ICM42688_RawData_t;

/**
 * @brief Master handle for one ICM-42688-P instance.
 * @details No static or global mutable state lives in icm42688.c — every
 *          field the driver needs across calls lives here, so more than one
 *          physical (or virtual, on QEMU) device can be driven concurrently.
 */
typedef struct {
    void    *bus_context;      /**< Opaque platform bus context, passed through
                                     unexamined to ICM42688_BusRead/Write. */
    uint32_t timeout_ms;       /**< Per-transaction bus timeout budget. */

    ICM42688_AccelFs_t accel_fs; /**< Active accelerometer full-scale range. */
    ICM42688_GyroFs_t  gyro_fs;  /**< Active gyroscope full-scale range. */

    bool     initialized;      /**< True once Init() has completed successfully. */
    bool     fifo_enabled;     /**< True after FifoEnable(), false after FifoDisable(). */
    uint32_t fault_count;      /**< Rolling bus-error counter for health reporting. */
    ICM42688_Status_t last_error; /**< Most recent status returned by any call. */
} ICM42688_Handle_t;

/*============================================================================*
 *                          PUBLIC API — REQUIRED SET                         *
 *============================================================================*/

/**
 * @brief Soft-resets the device, verifies WHO_AM_I, and enables accel+gyro
 *        in low-noise mode at the handle's configured full-scale ranges.
 * @param[in,out] handle Device instance. accel_fs/gyro_fs/bus_context/
 *                       timeout_ms must already be set by the caller.
 * @retval ICM42688_OK on success.
 * @retval ICM42688_ERROR_INVALID_PARAM if handle or its bus hooks are missing.
 * @retval ICM42688_ERROR_WRONG_DEVICE_ID if WHO_AM_I does not read 0x47.
 * @retval ICM42688_ERROR_BUS if any underlying transaction fails.
 */
ICM42688_Status_t ICM42688_Init(ICM42688_Handle_t *handle);

/**
 * @brief Parks accel and gyro in their lowest-power (off) state.
 * @param[in,out] handle Initialized device instance.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_DeInit(ICM42688_Handle_t *handle);

/**
 * @brief Reads the WHO_AM_I register.
 * @param[in,out] handle Device instance.
 * @param[out]    id     Destination for the raw WHO_AM_I byte (expect 0x47).
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_ReadDeviceID(ICM42688_Handle_t *handle, uint8_t *id);

/**
 * @brief Issues a software reset (DEVICE_CONFIG bit0) without touching the
 *        rest of the handle's cached configuration.
 * @param[in,out] handle Device instance.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_Reset(ICM42688_Handle_t *handle);

/**
 * @brief Burst-reads temperature + accelerometer + gyroscope in one 14-byte
 *        transaction (registers 0x1D..0x2A are contiguous on this chip).
 * @param[in,out] handle Initialized device instance.
 * @param[out]    data   Destination for the raw sample.
 * @retval ICM42688_OK on success.
 * @retval ICM42688_ERROR_NOT_INITIALIZED if Init() has not succeeded yet.
 * @retval ICM42688_ERROR_BUS on transport failure.
 */
ICM42688_Status_t ICM42688_ReadData(ICM42688_Handle_t *handle, ICM42688_RawData_t *data);

/*============================================================================*
 *                       PUBLIC API — SENSOR-SPECIFIC EXTRAS                  *
 *============================================================================*/

/**
 * @brief Changes the accelerometer full-scale range at runtime.
 * @param[in,out] handle Initialized device instance.
 * @param[in]     fs     New full-scale selection.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_SetAccelFS(ICM42688_Handle_t *handle, ICM42688_AccelFs_t fs);

/**
 * @brief Changes the gyroscope full-scale range at runtime.
 * @param[in,out] handle Initialized device instance.
 * @param[in]     fs     New full-scale selection.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_SetGyroFS(ICM42688_Handle_t *handle, ICM42688_GyroFs_t fs);

/**
 * @brief Switches the FIFO into stream mode so samples accumulate on-chip.
 * @param[in,out] handle Initialized device instance.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_FifoEnable(ICM42688_Handle_t *handle);

/**
 * @brief Returns the FIFO to bypass mode.
 * @param[in,out] handle Initialized device instance.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_FifoDisable(ICM42688_Handle_t *handle);

/**
 * @brief Reads the current FIFO_COUNT (bytes available to drain).
 * @param[in,out] handle Initialized device instance.
 * @param[out]    count  Destination for the byte count.
 * @retval ICM42688_OK on success; error code otherwise.
 */
ICM42688_Status_t ICM42688_FifoGetCount(ICM42688_Handle_t *handle, uint16_t *count);

/**
 * @brief Drains up to max_packets 12-byte accel+gyro packets from the FIFO.
 * @param[in,out] handle       Initialized device instance.
 * @param[out]    out          Array of at least max_packets entries.
 * @param[in]     max_packets  Capacity of out.
 * @param[out]    packets_read Number of whole packets actually returned.
 * @retval ICM42688_OK on success (including reading zero packets).
 * @retval ICM42688_ERROR_NOT_READY if the FIFO is currently disabled.
 */
ICM42688_Status_t ICM42688_FifoRead(ICM42688_Handle_t *handle, ICM42688_RawData_t *out,
                                     uint16_t max_packets, uint16_t *packets_read);

#ifdef __cplusplus
}
#endif

#endif /* ICM42688_H */
