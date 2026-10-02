/**
 * @file hmc5883l.h
 * @brief Public Abstraction Interface for the HMC5883L 3-Axis Digital Compass.
 * @details This header defines public enums, structures, and function prototypes
 *          to initialize, configure, and retrieve raw data from the Honeywell HMC5883L.
 *          Every API enforces the handle-based, stateless contract to ensure multi-instance
 *          safety on bare-metal and RTOS platforms.
 *
 * @note This file is part of the ANSA Hardware Abstraction Layer (HAL) Driver Framework.
 *       It depends only on standard library types and the common HAL types.
 *
 * 
 */

#ifndef HMC5883L_H
#define HMC5883L_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================*
 *                        COMMON STATUS & STATE TYPEDEFS                      *
 *     (Mocked or declared here if not loaded from main ansa framework)      *
 *============================================================================*/

#ifndef ANSA_HAL_TYPES_H
/**
 * @brief Normalized ANSA HAL Status return codes.
 * @details Sourced from Section 2.2 of the ANSA HAL Specification.
 */
typedef enum {
    HAL_OK                  = 0,   /**< Operation successful */
    HAL_ERR_TIMEOUT         = -1,  /**< Bus timeout expired */
    HAL_ERR_BUSY            = -2,  /**< Bus is busy or contested */
    HAL_ERR_INVALID_PARAM   = -3,  /**< Null pointers or invalid boundaries */
    HAL_ERR_NOT_INITIALIZED = -4,  /**< Device handle not configured or probed */
    HAL_ERR_HW_FAULT        = -5,  /**< Identity mismatch or self-test failure */
    HAL_ERR_UNSUPPORTED     = -6,  /**< Selected feature not supported by chip */
    HAL_ERR_DMA             = -7,  /**< DMA transfer failure */
    HAL_ERR_NACK            = -8,  /**< Device failed to acknowledge I2C byte */
    HAL_ERR_OVERRUN         = -9,  /**< Data registers read slower than ODR */
    HAL_ERR_UNKNOWN         = -99  /**< Unspecified driver or system fault */
} hal_status_t;
#endif

#ifndef ANSA_DRIVER_IF_H
/**
 * @brief Unified Driver State Machine representation.
 * @details Sourced from Section 3.2 and 3.4 of the ANSA HAL Specification.
 */
typedef enum {
    DRV_STATE_UNINIT,         /**< Device has not been configured */
    DRV_STATE_INITIALIZING,   /**< Probe and self-test are currently executing */
    DRV_STATE_READY,          /**< Initialized and passed self-test, but not active */
    DRV_STATE_ACTIVE,         /**< Successfully writing or reading registers */
    DRV_STATE_FAULT,          /**< Communication failure, watchdog timeout, or BIT fault */
    DRV_STATE_SHUTDOWN        /**< Driver gracefully put in ultra-low power state */
} driver_state_t;
#endif

/*============================================================================*
 *                       HMC5883L CONFIGURATION ENUMS                        *
 *============================================================================*/

/**
 * @brief Selectable average counts per measurement output (MA1:MA0).
 * @details Sourced from Register CRA bits [6:5] (Datasheet Page 12).
 */
typedef enum {
    HMC5883L_AVG_1 = 0x00, /**< Default: No internal sample averaging */
    HMC5883L_AVG_2 = 0x01, /**< Averaging of 2 internal samples */
    HMC5883L_AVG_4 = 0x02, /**< Averaging of 4 internal samples */
    HMC5883L_AVG_8 = 0x03  /**< Averaging of 8 internal samples */
} HMC5883L_Averages_t;

/**
 * @brief Selectable typical Output Data Rates (DO2:DO0) in Continuous Mode.
 * @details Sourced from Register CRA bits [4:2] (Datasheet Page 12).
 */
typedef enum {
    HMC5883L_ODR_0_75_HZ = 0x00, /**< 0.75 Hz typical output rate */
    HMC5883L_ODR_1_5_HZ  = 0x01, /**< 1.5 Hz typical output rate */
    HMC5883L_ODR_3_0_HZ  = 0x02, /**< 3.0 Hz typical output rate */
    HMC5883L_ODR_7_5_HZ  = 0x03, /**< 7.5 Hz typical output rate */
    HMC5883L_ODR_15_HZ   = 0x04, /**< Default: 15 Hz typical output rate */
    HMC5883L_ODR_30_HZ   = 0x05, /**< 30 Hz typical output rate */
    HMC5883L_ODR_75_HZ   = 0x06  /**< 75 Hz typical output rate */
} HMC5883L_Odr_t;

/**
 * @brief Measurement configuration biases (MS1:MS0).
 * @details Sourced from Register CRA bits [1:0] (Datasheet Page 12). Used to
 *          enable positive/negative current loops across offset straps for self-test.
 */
typedef enum {
    HMC5883L_BIAS_NORMAL   = 0x00, /**< Default: Normal magnetic measurement flow */
    HMC5883L_BIAS_POSITIVE = 0x01, /**< Excites internal positive offset test field */
    HMC5883L_BIAS_NEGATIVE = 0x02  /**< Excites internal negative offset test field */
} HMC5883L_Bias_t;

/**
 * @brief Selectable gain configurations (GN2:GN0).
 * @details Sourced from Register CRB bits [7:5] (Datasheet Page 13).
 *          GN value controls the dynamic range and digital scale factor.
 */
typedef enum {
    HMC5883L_GAIN_0_88_GA = 0x00, /**< Range ±0.88 Gauss, 1370 LSB/Gauss (0.73 mG/LSB) */
    HMC5883L_GAIN_1_3_GA  = 0x01, /**< Default: Range ±1.3 Gauss, 1090 LSB/Gauss (0.92 mG/LSB) */
    HMC5883L_GAIN_1_9_GA  = 0x02, /**< Range ±1.9 Gauss, 820 LSB/Gauss (1.22 mG/LSB) */
    HMC5883L_GAIN_2_5_GA  = 0x03, /**< Range ±2.5 Gauss, 660 LSB/Gauss (1.52 mG/LSB) */
    HMC5883L_GAIN_4_0_GA  = 0x04, /**< Range ±4.0 Gauss, 440 LSB/Gauss (2.27 mG/LSB) */
    HMC5883L_GAIN_4_7_GA  = 0x05, /**< Range ±4.7 Gauss, 390 LSB/Gauss (2.56 mG/LSB). Recommended for Self-Test. */
    HMC5883L_GAIN_5_6_GA  = 0x06, /**< Range ±5.6 Gauss, 330 LSB/Gauss (3.03 mG/LSB) */
    HMC5883L_GAIN_8_1_GA  = 0x07  /**< Range ±8.1 Gauss, 230 LSB/Gauss (4.35 mG/LSB) */
} HMC5883L_Gain_t;

/**
 * @brief Operating modes (MD1:MD0).
 * @details Sourced from Mode Register bits [1:0] (Datasheet Page 14).
 */
typedef enum {
    HMC5883L_MODE_CONTINUOUS = 0x00, /**< Continuously samples data at the configured ODR */
    HMC5883L_MODE_SINGLE     = 0x01, /**< Samples once, updates registers, transitions to IDLE */
    HMC5883L_MODE_IDLE       = 0x02  /**< Powers down analog circuits, accessible via I2C */
} HMC5883L_Mode_t;

/*============================================================================*
 *                       DATA STRUCTURES & HANDLE                             *
 *============================================================================*/

/**
 * @brief Structure containing the raw measurement values.
 * @details This is the baseline driver output. Raw LSB counts are strictly
 *          output directly with zero unit conversion, scaling, or calibration.
 */
typedef struct {
    int16_t x_raw;            /**< Raw X-axis magnetic field reading (2's complement) */
    int16_t y_raw;            /**< Raw Y-axis magnetic field reading (2's complement) */
    int16_t z_raw;            /**< Raw Z-axis magnetic field reading (2's complement) */
    uint64_t timestamp_us;    /**< Hardware timestamp at moment read completes (captured in HAL) */
} HMC5883L_RawData_t;

/**
 * @brief Master handle structure representing one unique HMC5883L instance.
 * @details Enforces a stateless driver architecture. All runtime parameters,
 *          state indicators, and virtual bus bindings live in this structure.
 */
typedef struct {
    /* Peripheral Interface Context */
    void *bus_handle;               /**< Pointer to platform-specific peripheral (e.g., I2C handle context) */
    uint16_t dev_address;           /**< Bus physical 7-bit slave address (usually 0x1E) */
    uint32_t timeout_ms;            /**< Bus transaction timeout constraint */

    /* Active Run configurations */
    HMC5883L_Averages_t averaging;  /**< Number of samples to average */
    HMC5883L_Odr_t odr;             /**< Output data rate configuration */
    HMC5883L_Bias_t bias;           /**< Bias configuration (self-test polarity) */
    HMC5883L_Gain_t gain;           /**< Hardware amplification dynamic range */
    HMC5883L_Mode_t mode;           /**< Chip operating mode state */

    /* Internal State Machine tracking */
    driver_state_t state;           /**< Current lifecycle state */
    uint32_t fault_count;           /**< Rolling I2C bus error counter */
    hal_status_t last_error;        /**< Last returned error from the bus */

    /* Standard Bus Abstraction Interface */
    hal_status_t (*bus_read)(void *bus, uint8_t dev_addr, uint8_t reg_addr, uint8_t *dest, size_t len, uint32_t timeout);
    hal_status_t (*bus_write)(void *bus, uint8_t dev_addr, uint8_t reg_addr, const uint8_t *src, size_t len, uint32_t timeout);
} HMC5883L_Handle_t;

/*============================================================================*
 *                         PUBLIC API DECLARATIONS                            *
 *============================================================================*/

/**
 * @brief Resets the HMC5883L, verifies its identity, and applies configuration parameters.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @return hal_status_t HAL_OK on successful initialization; error enum otherwise.
 * @pre Function pointers `bus_read` and `bus_write` must be bound in the handle.
 */
hal_status_t HMC5883L_Init(HMC5883L_Handle_t *handle);

/**
 * @brief Gracefully shuts down the device, putting it into lowest power state (Idle mode).
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_DeInit(HMC5883L_Handle_t *handle);

/**
 * @brief Performs a hardware check of the chip's hardwired Identification Registers.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[out] id_a Destination pointer to store Identification Register A (expected 'H').
 * @param[out] id_b Destination pointer to store Identification Register B (expected '4').
 * @param[out] id_c Destination pointer to store Identification Register C (expected '3').
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_ReadDeviceID(HMC5883L_Handle_t *handle, uint8_t *id_a, uint8_t *id_b, uint8_t *id_c);

/**
 * @brief Soft-resets the configuration and mode registers of the physical device.
 * @details Resets register values on the chip to power-up defaults by setting MR and Configuration
 *          registers, and updates the driver state structure.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_Reset(HMC5883L_Handle_t *handle);

/**
 * @brief Retrieves the latest raw magnetic measurements from the hardware registers.
 * @details Reads all six data registers (0x03 to 0x08) in a single burst read. Enforces the
 *          register layout order of X, Z, Y as defined by the Honeywell ASIC.
 *          Does NOT apply scale, offset, temperature compensation, or unit conversions.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[out] data Target data struct to populate with raw results and timestamp.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 * @note If the reading overflows or underflows, X/Y/Z variables are set to -4096.
 */
hal_status_t HMC5883L_ReadData(HMC5883L_Handle_t *handle, HMC5883L_RawData_t *data);

/**
 * @brief Updates the sensor gain register directly and modifies LSB scaling targets.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[in] gain Selected gain enumeration value.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_SetGain(HMC5883L_Handle_t *handle, HMC5883L_Gain_t gain);

/**
 * @brief Sets the operating mode (Continuous, Single, or Idle) on the chip.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[in] mode Selected operating mode.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_SetMode(HMC5883L_Handle_t *handle, HMC5883L_Mode_t mode);

/**
 * @brief Configures data rate and sample averaging in CRA.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[in] odr Target output rate in Hz.
 * @param[in] averaging Target internal averaging counts.
 * @return hal_status_t HAL_OK on success; error enum otherwise.
 */
hal_status_t HMC5883L_SetOdrAndAveraging(HMC5883L_Handle_t *handle, HMC5883L_Odr_t odr, HMC5883L_Averages_t averaging);

/**
 * @brief Triggers and evaluates the device's internal built-in Self-Test.
 * @details Sourced from Self-Test Operation (Datasheet Page 16). Configures positive bias
 *          and Gain=5, samples data, and evaluates if results fall within the specified
 *          datasheet bounds (243 to 575 LSBs on all three axes). Restores normal configuration
 *          after execution.
 * @param[in,out] handle Pointer to the stateless device instance handle.
 * @param[out] pass_out Pointer to boolean, set to true if self-test passes, false if it fails.
 * @return hal_status_t HAL_OK on successful test execution (even if device fails parameters);
 *                      error enum on transaction failures.
 */
hal_status_t HMC5883L_RunSelfTest(HMC5883L_Handle_t *handle, bool *pass_out);

#endif /* HMC5883L_H */
