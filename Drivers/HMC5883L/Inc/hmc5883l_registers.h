/**
 * @file hmc5883l_registers.h
 * @brief Private Register Map and Bitfield Definitions for the HMC5883L 3-Axis Digital Compass.
 * @details This header defines register addresses, bitmasks, shift counts, and configuration 
 *          value constants for the Honeywell HMC5883L. All values are sourced directly from the 
 *          official Honeywell HMC5883L datasheet (Form # 900405 Rev E, February 2013).
 *
 * @note This file is part of the ANSA Hardware Abstraction Layer (HAL) Driver Framework.
 *       It is a private interface meant only to be included by the core driver source files 
 *       (e.g., hmc5883l.c) and host-side mock drivers (e.g., mock_hmc5883l_bus.c).
 *
 * 
 */

#ifndef HMC5883L_REGISTERS_H
#define HMC5883L_REGISTERS_H

#include <stdint.h>

/*============================================================================*
 *                        REGISTER ADDRESS MAP                                *
 *          Sourced from Table 2: Register List (Datasheet Page 11)           *
 *============================================================================*/

/** 
 * @brief Configuration Register A (Read/Write)
 * @details Used to set sample averaging, data output rate, and measurement bias configuration.
 *          Default value is 0x10 (no averaging, 15 Hz output rate, normal measurement bias).
 *          See Datasheet Pages 11-12.
 */
#define HMC5883L_REG_CRA            ((uint8_t)0x00)

/** 
 * @brief Configuration Register B (Read/Write)
 * @details Used to set the device gain (magnetic field range and resolution).
 *          Default value is 0x20 (Gain 1090 LSB/Gauss, dynamic range of ±1.3 Gauss).
 *          See Datasheet Pages 12-13.
 */
#define HMC5883L_REG_CRB            ((uint8_t)0x01)

/** 
 * @brief Mode Register (Read/Write)
 * @details Used to select the operating mode (Continuous, Single-Measurement, or Idle).
 *          Default value is 0x01 (Single-Measurement Mode).
 *          See Datasheet Pages 13-14.
 */
#define HMC5883L_REG_MODE           ((uint8_t)0x02)

/** 
 * @brief Data Output X MSB Register (Read Only)
 * @details Stores the most significant byte of the X-axis magnetic measurement.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_X_MSB     ((uint8_t)0x03)

/** 
 * @brief Data Output X LSB Register (Read Only)
 * @details Stores the least significant byte of the X-axis magnetic measurement.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_X_LSB     ((uint8_t)0x04)

/** 
 * @brief Data Output Z MSB Register (Read Only)
 * @details Stores the most significant byte of the Z-axis magnetic measurement.
 *          @note The HMC5883L layout sequences registers as X, Z, Y instead of X, Y, Z.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_Z_MSB     ((uint8_t)0x05)

/** 
 * @brief Data Output Z LSB Register (Read Only)
 * @details Stores the least significant byte of the Z-axis magnetic measurement.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_Z_LSB     ((uint8_t)0x06)

/** 
 * @brief Data Output Y MSB Register (Read Only)
 * @details Stores the most significant byte of the Y-axis magnetic measurement.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_Y_MSB     ((uint8_t)0x07)

/** 
 * @brief Data Output Y LSB Register (Read Only)
 * @details Stores the least significant byte of the Y-axis magnetic measurement.
 *          See Datasheet Page 14.
 */
#define HMC5883L_REG_DATA_Y_LSB     ((uint8_t)0x08)

/** 
 * @brief Status Register (Read Only)
 * @details Indicates the internal data state (Ready, Locked).
 *          See Datasheet Page 15.
 */
#define HMC5883L_REG_STATUS         ((uint8_t)0x09)

/** 
 * @brief Identification Register A (Read Only)
 * @details Contains the ASCII character 'H' (0x48).
 *          See Datasheet Page 15.
 */
#define HMC5883L_REG_IDA            ((uint8_t)0x0A)

/** 
 * @brief Identification Register B (Read Only)
 * @details Contains the ASCII character '4' (0x34).
 *          See Datasheet Page 15.
 */
#define HMC5883L_REG_IDB            ((uint8_t)0x0B)

/** 
 * @brief Identification Register C (Read Only)
 * @details Contains the ASCII character '3' (0x33).
 *          See Datasheet Page 15.
 */
#define HMC5883L_REG_IDC            ((uint8_t)0x0C)


/*============================================================================*
 *                    BITFIELD & CONFIGURATION DEFINITIONS                    *
 *============================================================================*/

/*----------------------------------------------------------------------------*
 * 1. Configuration Register A (CRA) - Sourced from Pages 11-12
 *----------------------------------------------------------------------------*/

/** @brief CRA Bit 7: Reserved bit. Must always be written as 0. */
#define HMC5883L_CRA_RESERVED_MASK  ((uint8_t)0x80)

/** @brief CRA Bits 6-5: Sample Averaging (MA1:MA0) */
#define HMC5883L_CRA_MA_MASK        ((uint8_t)0x60)
#define HMC5883L_CRA_MA_SHIFT       (5U)
#define HMC5883L_CRA_MA_1           ((uint8_t)(0x00 << HMC5883L_CRA_MA_SHIFT)) /**< Default: 1 sample averaged */
#define HMC5883L_CRA_MA_2           ((uint8_t)(0x01 << HMC5883L_CRA_MA_SHIFT)) /**< 2 samples averaged */
#define HMC5883L_CRA_MA_4           ((uint8_t)(0x02 << HMC5883L_CRA_MA_SHIFT)) /**< 4 samples averaged */
#define HMC5883L_CRA_MA_8           ((uint8_t)(0x03 << HMC5883L_CRA_MA_SHIFT)) /**< 8 samples averaged */

/** @brief CRA Bits 4-2: Data Output Rate (DO2:DO0) in Continuous Mode */
#define HMC5883L_CRA_DO_MASK        ((uint8_t)0x1C) //00011100
#define HMC5883L_CRA_DO_SHIFT       (2U)
#define HMC5883L_CRA_ODR_0_75_HZ    ((uint8_t)(0x00 << HMC5883L_CRA_DO_SHIFT)) /**< 0.75 Hz Output Rate */
#define HMC5883L_CRA_ODR_1_5_HZ     ((uint8_t)(0x01 << HMC5883L_CRA_DO_SHIFT)) /**< 1.5 Hz Output Rate */
#define HMC5883L_CRA_ODR_3_HZ       ((uint8_t)(0x02 << HMC5883L_CRA_DO_SHIFT)) /**< 3.0 Hz Output Rate */
#define HMC5883L_CRA_ODR_7_5_HZ     ((uint8_t)(0x03 << HMC5883L_CRA_DO_SHIFT)) /**< 7.5 Hz Output Rate */
#define HMC5883L_CRA_ODR_15_HZ      ((uint8_t)(0x04 << HMC5883L_CRA_DO_SHIFT)) /**< Default: 15 Hz Output Rate */
#define HMC5883L_CRA_ODR_30_HZ      ((uint8_t)(0x05 << HMC5883L_CRA_DO_SHIFT)) /**< 30 Hz Output Rate */
#define HMC5883L_CRA_ODR_75_HZ      ((uint8_t)(0x06 << HMC5883L_CRA_DO_SHIFT)) /**< 75 Hz Output Rate */

/** @brief CRA Bits 1-0: Measurement Configuration Mode (MS1:MS0) */
#define HMC5883L_CRA_MS_MASK        ((uint8_t)0x03)
#define HMC5883L_CRA_MS_SHIFT       (0U)
#define HMC5883L_CRA_BIAS_NORMAL    ((uint8_t)(0x00 << HMC5883L_CRA_MS_SHIFT)) /**< Default: Normal Measurement Flow */
#define HMC5883L_CRA_BIAS_POSITIVE  ((uint8_t)(0x01 << HMC5883L_CRA_MS_SHIFT)) /**< Positive bias across sensor elements (Self-Test) */
#define HMC5883L_CRA_BIAS_NEGATIVE  ((uint8_t)(0x02 << HMC5883L_CRA_MS_SHIFT)) /**< Negative bias across sensor elements (Self-Test) */

/*----------------------------------------------------------------------------*
 * 2. Configuration Register B (CRB) - Sourced from Pages 12-13
 *----------------------------------------------------------------------------*/

/** @brief CRB Bits 7-5: Gain Configuration Bits (GN2:GN0) */
#define HMC5883L_CRB_GN_MASK        ((uint8_t)0xE0) //11100000
#define HMC5883L_CRB_GN_SHIFT       (5U)
#define HMC5883L_CRB_GAIN_0_88_GA   ((uint8_t)(0x00 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±0.88 Gauss, Gain: 1370 LSB/Gauss */
#define HMC5883L_CRB_GAIN_1_3_GA    ((uint8_t)(0x01 << HMC5883L_CRB_GN_SHIFT)) /**< Default: ±1.3 Gauss, Gain: 1090 LSB/Gauss */
#define HMC5883L_CRB_GAIN_1_9_GA    ((uint8_t)(0x02 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±1.9 Gauss, Gain: 820 LSB/Gauss */
#define HMC5883L_CRB_GAIN_2_5_GA    ((uint8_t)(0x03 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±2.5 Gauss, Gain: 660 LSB/Gauss */
#define HMC5883L_CRB_GAIN_4_0_GA    ((uint8_t)(0x04 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±4.0 Gauss, Gain: 440 LSB/Gauss */
#define HMC5883L_CRB_GAIN_4_7_GA    ((uint8_t)(0x05 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±4.7 Gauss, Gain: 390 LSB/Gauss (Self-Test Recommended) */
#define HMC5883L_CRB_GAIN_5_6_GA    ((uint8_t)(0x06 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±5.6 Gauss, Gain: 330 LSB/Gauss */
#define HMC5883L_CRB_GAIN_8_1_GA    ((uint8_t)(0x07 << HMC5883L_CRB_GN_SHIFT)) /**< Dynamic range: ±8.1 Gauss, Gain: 230 LSB/Gauss */

/** @brief CRB Bits 4-0: Reserved. These bits must be cleared to 0 for correct operation. */
#define HMC5883L_CRB_RESERVED_MASK  ((uint8_t)0x1F)

/*----------------------------------------------------------------------------*
 * 3. Mode Register (MR) - Sourced from Pages 13-14
 *----------------------------------------------------------------------------*/

/** @brief MR Bit 7: High Speed I2C Enable (HS) */
#define HMC5883L_MR_HS_MASK         ((uint8_t)0x80) //10000000
#define HMC5883L_MR_HS_SHIFT        (7U)
#define HMC5883L_MR_HS_DISABLE      ((uint8_t)(0x00 << HMC5883L_MR_HS_SHIFT)) /**< Default: Standard I2C up to 400kHz */ //12c info in page 9
#define HMC5883L_MR_HS_ENABLE       ((uint8_t)(0x01 << HMC5883L_MR_HS_SHIFT)) /**< High-speed I2C up to 3400kHz */

/** @brief MR Bits 6-2: Reserved. Must be written as 0. */
#define HMC5883L_MR_RESERVED_MASK   ((uint8_t)0x7C) //01111100

/** @brief MR Bits 1-0: Operating Mode Selection (MD1:MD0) */
#define HMC5883L_MR_MD_MASK         ((uint8_t)0x03)
#define HMC5883L_MR_MD_SHIFT        (0U)
#define HMC5883L_MR_MODE_CONTINUOUS ((uint8_t)(0x00 << HMC5883L_MR_MD_SHIFT)) /**< Continuous-Measurement Mode */
#define HMC5883L_MR_MODE_SINGLE     ((uint8_t)(0x01 << HMC5883L_MR_MD_SHIFT)) /**< Default: Single-Measurement Mode */
#define HMC5883L_MR_MODE_IDLE       ((uint8_t)(0x02 << HMC5883L_MR_MD_SHIFT)) /**< Idle Mode (reduced power) */

/*----------------------------------------------------------------------------*
 * 4. Status Register (SR) - Sourced from Page 16
 *----------------------------------------------------------------------------*/

/** @brief SR Bits 7-2: Reserved. Read as 0. */
#define HMC5883L_SR_RESERVED_MASK   ((uint8_t)0xFC)

/** 
 * @brief SR Bit 1: Data Output Register Lock (LOCK)
 * @details Set when some but not all of the six data output registers have been read.
 *          Locked registers prevent raw data update until all 6 bytes are read or 
 *          mode/configuration changes.
 */
#define HMC5883L_SR_LOCK_MASK       ((uint8_t)0x02)
#define HMC5883L_SR_LOCK_SHIFT      (1U)

/** 
 * @brief SR Bit 0: Ready Bit (RDY)
 * @details Set when new data has been written to all six data registers.
 *          Cleared when device initiates a write/read of the data output registers.
 */
#define HMC5883L_SR_RDY_MASK        ((uint8_t)0x01)
#define HMC5883L_SR_RDY_SHIFT       (0U)

/*----------------------------------------------------------------------------*
 * 5. Device Identification Values - Sourced from Pages 15-16
 *----------------------------------------------------------------------------*/

#define HMC5883L_IDA_EXPECTED_VAL   ((uint8_t)'H') /**< Expected ID Register A value (0x48) */
#define HMC5883L_IDB_EXPECTED_VAL   ((uint8_t)'4') /**< Expected ID Register B value (0x34) */
#define HMC5883L_IDC_EXPECTED_VAL   ((uint8_t)'3') /**< Expected ID Register C value (0x33) */

/*----------------------------------------------------------------------------*
 * 6. Hard-Wired Physical Constants - Sourced from Page 11, 17
 *----------------------------------------------------------------------------*/

#define HMC5883L_I2C_ADDRESS        ((uint8_t)0x1E) /**< 7-bit physical slave address of the chip */ //page 11
#define HMC5883L_I2C_WRITE_ADDR     ((uint8_t)0x3C) /**< 8-bit equivalent write address */ //(page 11, 17)
#define HMC5883L_I2C_READ_ADDR      ((uint8_t)0x3D) /**< 8-bit equivalent read address */  //(page 11, 17)

/** 
 * @brief ADC saturation or error value
 * @details Sourced from Datasheet Page 15. If the ADC overflows, underflows,
 *          or experiences a math overflow during bias measurement, the corresponding
 *          registers will contain -4096 (represented as 0xF000 in 2's complement).
 */
#define HMC5883L_DATA_OVERFLOW_VAL  ((int16_t)-4096)

#endif /* HMC5883L_REGISTERS_H */
