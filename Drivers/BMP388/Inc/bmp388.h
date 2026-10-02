/**
 * @file    bmp388.h
 * @brief   Public API for the Bosch BMP388 barometric pressure sensor driver.
 * @details Handle-based, hardware-agnostic driver for the S09 ANSA HAL
 *          Emulator. All bus traffic goes through the BMP388_BusRead /
 *          BMP388_BusWrite / BMP388_DelayMs contract in bmp388_bus.h.
 *
 * @note    Part of the ANSA Hardware Abstraction Layer Driver Framework
 *          (see DRIVER_STANDARD.md). Depends only on the standard library.
 */

#ifndef BMP388_H
#define BMP388_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*
 *                              STATUS CODES                                  *
 *============================================================================*/

/**
 * @brief BMP388 driver status/return codes (DRIVER_STANDARD.md Section 5).
 */
typedef enum {
    BMP388_OK = 0,                  /**< Operation succeeded. */
    BMP388_ERROR_BUS,                /**< SPI/I2C transaction failed. */
    BMP388_ERROR_INVALID_PARAM,      /**< Null pointer or bad handle field. */
    BMP388_ERROR_WRONG_DEVICE_ID,    /**< CHIP_ID did not read back 0x50. */
    BMP388_ERROR_NOT_READY,          /**< No fresh sample yet (DRDY bits clear). */
    BMP388_ERROR_TIMEOUT,            /**< Bus transaction exceeded its budget. */
    BMP388_ERROR_NOT_INITIALIZED,    /**< Called before BMP388_Init() succeeded. */
    BMP388_ERROR_CALIBRATION         /**< NVM trim coefficients failed a sanity check. */
} BMP388_Status_t;

/*============================================================================*
 *                          CONFIGURATION ENUMS                               *
 *============================================================================*/

/** @brief Oversampling setting, applies to either channel independently. */
typedef enum {
    BMP388_OVERSAMPLING_X1  = 0,
    BMP388_OVERSAMPLING_X2  = 1,
    BMP388_OVERSAMPLING_X4  = 2,
    BMP388_OVERSAMPLING_X8  = 3,  /**< Default pressure oversampling applied by Init(). */
    BMP388_OVERSAMPLING_X16 = 4,
    BMP388_OVERSAMPLING_X32 = 5
} BMP388_Oversampling_t;

/*============================================================================*
 *                         DATA STRUCTURES & HANDLE                           *
 *============================================================================*/

/**
 * @brief One sample: raw ADC counts plus their Bosch-compensated engineering
 *        values, computed from the handle's cached NVM trim coefficients.
 * @details Both are provided because the project plan calls for emulating
 *          the raw DATA_0..DATA_5 registers specifically, while pressure_pa
 *          and temperature_c are what any altitude/flight-control consumer
 *          actually needs — computing them here means every caller doesn't
 *          have to re-derive the (nontrivial) Bosch compensation formula.
 */
typedef struct {
    uint32_t pressure_raw;      /**< 24-bit raw ADC pressure count. */
    uint32_t temperature_raw;   /**< 24-bit raw ADC temperature count. */
    double   pressure_pa;       /**< Compensated pressure, Pascals. */
    double   temperature_c;     /**< Compensated temperature, degrees Celsius. */
} BMP388_Data_t;

/** @brief Bosch NVM trimming coefficients, converted to floating point once
 *         at Init() time (Bosch's own floating-point compensation formula). */
typedef struct {
    double par_t1, par_t2, par_t3;
    double par_p1, par_p2, par_p3, par_p4, par_p5, par_p6, par_p7, par_p8,
           par_p9, par_p10, par_p11;
} BMP388_Calib_t;

/**
 * @brief Master handle for one BMP388 instance. No static or global mutable
 *        state lives in bmp388.c — everything the driver needs across calls,
 *        including the parsed calibration block, lives here.
 */
typedef struct {
    void    *bus_context;      /**< Opaque platform bus context. */
    uint32_t timeout_ms;       /**< Per-transaction bus timeout budget. */

    BMP388_Oversampling_t press_osr; /**< Active pressure oversampling. */
    BMP388_Oversampling_t temp_osr;  /**< Active temperature oversampling. */

    BMP388_Calib_t calib;      /**< Parsed NVM trim coefficients. */
    bool     calib_loaded;     /**< True once calib has been read successfully. */

    bool     initialized;      /**< True once Init() has completed successfully. */
    uint32_t fault_count;      /**< Rolling bus-error counter for health reporting. */
    BMP388_Status_t last_error; /**< Most recent status returned by any call. */
} BMP388_Handle_t;

/*============================================================================*
 *                          PUBLIC API — REQUIRED SET                         *
 *============================================================================*/

/**
 * @brief Soft-resets the device, verifies CHIP_ID, loads NVM calibration,
 *        and starts continuous normal-mode measurement at the handle's
 *        configured oversampling.
 * @param[in,out] handle Device instance. press_osr/temp_osr/bus_context/
 *                       timeout_ms must already be set by the caller.
 * @retval BMP388_OK on success.
 * @retval BMP388_ERROR_WRONG_DEVICE_ID if CHIP_ID does not read 0x50.
 * @retval BMP388_ERROR_CALIBRATION if the NVM block fails a sanity check.
 * @retval BMP388_ERROR_BUS if any underlying transaction fails.
 */
BMP388_Status_t BMP388_Init(BMP388_Handle_t *handle);

/**
 * @brief Returns the device to sleep mode (both channels disabled).
 * @param[in,out] handle Initialized device instance.
 * @retval BMP388_OK on success; error code otherwise.
 */
BMP388_Status_t BMP388_DeInit(BMP388_Handle_t *handle);

/**
 * @brief Reads the CHIP_ID register.
 * @param[in,out] handle Device instance.
 * @param[out]    id     Destination for the raw CHIP_ID byte (expect 0x50).
 * @retval BMP388_OK on success; error code otherwise.
 */
BMP388_Status_t BMP388_ReadDeviceID(BMP388_Handle_t *handle, uint8_t *id);

/**
 * @brief Issues a software reset (CMD = 0xB6). Calibration must be reloaded
 *        (call Init() again) before the next ReadData().
 * @param[in,out] handle Device instance.
 * @retval BMP388_OK on success; error code otherwise.
 */
BMP388_Status_t BMP388_Reset(BMP388_Handle_t *handle);

/**
 * @brief Reads DATA_0..DATA_5, converts to raw 24-bit counts, and applies
 *        the Bosch compensation formula using the handle's cached trim.
 * @param[in,out] handle Initialized device instance.
 * @param[out]    data   Destination for the raw + compensated sample.
 * @retval BMP388_OK on success.
 * @retval BMP388_ERROR_NOT_INITIALIZED if Init() has not succeeded yet.
 * @retval BMP388_ERROR_NOT_READY if neither DRDY bit is set yet.
 * @retval BMP388_ERROR_BUS on transport failure.
 */
BMP388_Status_t BMP388_ReadData(BMP388_Handle_t *handle, BMP388_Data_t *data);

/*============================================================================*
 *                       PUBLIC API — SENSOR-SPECIFIC EXTRAS                  *
 *============================================================================*/

/**
 * @brief Changes pressure and temperature oversampling at runtime.
 * @param[in,out] handle    Initialized device instance.
 * @param[in]     press_osr New pressure oversampling.
 * @param[in]     temp_osr  New temperature oversampling.
 * @retval BMP388_OK on success; error code otherwise.
 */
BMP388_Status_t BMP388_SetOversampling(BMP388_Handle_t *handle,
                                        BMP388_Oversampling_t press_osr,
                                        BMP388_Oversampling_t temp_osr);

/**
 * @brief Polls STATUS and reports whether a fresh pressure sample is ready.
 * @param[in,out] handle Initialized device instance.
 * @param[out]    ready  Set true if DRDY_PRESS is set.
 * @retval BMP388_OK on success; error code otherwise.
 */
BMP388_Status_t BMP388_IsDataReady(BMP388_Handle_t *handle, bool *ready);

#ifdef __cplusplus
}
#endif

#endif /* BMP388_H */
