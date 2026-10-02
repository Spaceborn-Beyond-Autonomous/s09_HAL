/**
 * @file    icm42688_registers.h
 * @brief   Private register map and bitfield definitions for the TDK InvenSense
 *          ICM-42688-P 6-axis IMU (3-axis accel + 3-axis gyro).
 *
 * @details All addresses below are User Bank 0 registers (the bank the driver
 *          selects by default and never leaves, since this project does not
 *          use the auxiliary banks 1-4 used for self-test trim / APEX
 *          features). Sourced from the ICM-42688-P Register Map, User Bank 0.
 *
 * @note    Private header: include only from icm42688.c and the matching
 *          bus implementations (icm42688_bus_stm32.c, mock_icm42688_bus.c).
 */

#ifndef ICM42688_REGISTERS_H
#define ICM42688_REGISTERS_H

#include <stdint.h>

/*============================================================================*
 *                    BANK 0 REGISTER ADDRESS MAP                            *
 *============================================================================*/

#define ICM42688_REG_DEVICE_CONFIG     ((uint8_t)0x11U) /**< Soft reset (bit0). */
#define ICM42688_REG_DRIVE_CONFIG      ((uint8_t)0x13U) /**< I/O drive strength. */
#define ICM42688_REG_INT_CONFIG        ((uint8_t)0x14U) /**< Interrupt pin config. */
#define ICM42688_REG_FIFO_CONFIG       ((uint8_t)0x16U) /**< FIFO mode select. */

#define ICM42688_REG_TEMP_DATA1        ((uint8_t)0x1DU) /**< Temperature MSB. */
#define ICM42688_REG_TEMP_DATA0        ((uint8_t)0x1EU) /**< Temperature LSB. */

#define ICM42688_REG_ACCEL_DATA_X1     ((uint8_t)0x1FU)
#define ICM42688_REG_ACCEL_DATA_X0     ((uint8_t)0x20U)
#define ICM42688_REG_ACCEL_DATA_Y1     ((uint8_t)0x21U)
#define ICM42688_REG_ACCEL_DATA_Y0     ((uint8_t)0x22U)
#define ICM42688_REG_ACCEL_DATA_Z1     ((uint8_t)0x23U)
#define ICM42688_REG_ACCEL_DATA_Z0     ((uint8_t)0x24U)

#define ICM42688_REG_GYRO_DATA_X1      ((uint8_t)0x25U)
#define ICM42688_REG_GYRO_DATA_X0      ((uint8_t)0x26U)
#define ICM42688_REG_GYRO_DATA_Y1      ((uint8_t)0x27U)
#define ICM42688_REG_GYRO_DATA_Y0      ((uint8_t)0x28U)
#define ICM42688_REG_GYRO_DATA_Z1      ((uint8_t)0x29U)
#define ICM42688_REG_GYRO_DATA_Z0      ((uint8_t)0x2AU)

/**
 * @brief FIFO byte count and burst-read data port.
 * @details Same Bank-0 offsets used across the whole InvenSense ICM-4xxxx /
 *          MPU-9xxx family register map (FIFO_COUNTH/L + FIFO_DATA).
 */
#define ICM42688_REG_FIFO_COUNTH       ((uint8_t)0x2EU)
#define ICM42688_REG_FIFO_COUNTL       ((uint8_t)0x2FU)
#define ICM42688_REG_FIFO_DATA         ((uint8_t)0x30U)

#define ICM42688_REG_PWR_MGMT0         ((uint8_t)0x4EU) /**< Sensor power modes. */
#define ICM42688_REG_GYRO_CONFIG0      ((uint8_t)0x4FU) /**< Gyro FS + ODR. */
#define ICM42688_REG_ACCEL_CONFIG0     ((uint8_t)0x50U) /**< Accel FS + ODR. */

#define ICM42688_REG_WHO_AM_I          ((uint8_t)0x75U)
#define ICM42688_REG_REG_BANK_SEL      ((uint8_t)0x76U)

/*============================================================================*
 *                       IDENTITY / RESET CONSTANTS                          *
 *============================================================================*/

#define ICM42688_WHO_AM_I_VALUE        ((uint8_t)0x47U) /**< Fixed WHO_AM_I value. */

/** @brief DEVICE_CONFIG bit0: software reset, self-clearing on the device. */
#define ICM42688_DEVICE_CONFIG_SOFT_RESET  ((uint8_t)0x01U)

/*============================================================================*
 *                       PWR_MGMT0 (0x4E) BITFIELDS                          *
 *============================================================================*/

#define ICM42688_PWR_TEMP_DIS_MASK     ((uint8_t)0x20U) /**< 1 = temperature sensor disabled. */

#define ICM42688_PWR_GYRO_MODE_MASK    ((uint8_t)0x0CU)
#define ICM42688_PWR_GYRO_MODE_SHIFT   (2U)
#define ICM42688_PWR_GYRO_MODE_OFF     ((uint8_t)(0x00U << ICM42688_PWR_GYRO_MODE_SHIFT))
#define ICM42688_PWR_GYRO_MODE_STANDBY ((uint8_t)(0x01U << ICM42688_PWR_GYRO_MODE_SHIFT))
#define ICM42688_PWR_GYRO_MODE_LOWNOISE ((uint8_t)(0x03U << ICM42688_PWR_GYRO_MODE_SHIFT))

#define ICM42688_PWR_ACCEL_MODE_MASK   ((uint8_t)0x03U)
#define ICM42688_PWR_ACCEL_MODE_OFF    ((uint8_t)0x00U)
#define ICM42688_PWR_ACCEL_MODE_LOWPWR ((uint8_t)0x02U)
#define ICM42688_PWR_ACCEL_MODE_LOWNOISE ((uint8_t)0x03U)

/** @brief Default operating point applied by ICM42688_Init(): temp enabled,
 *         gyro + accel both in low-noise mode. */
#define ICM42688_PWR_MGMT0_DEFAULT_RUN \
    ((uint8_t)(ICM42688_PWR_GYRO_MODE_LOWNOISE | ICM42688_PWR_ACCEL_MODE_LOWNOISE))

/** @brief PWR_MGMT0 value that parks the device in its lowest-power state. */
#define ICM42688_PWR_MGMT0_SLEEP       ((uint8_t)0x00U)

/*============================================================================*
 *                 ACCEL_CONFIG0 / GYRO_CONFIG0 (0x50 / 0x4F)                *
 *============================================================================*/

#define ICM42688_FS_SEL_MASK           ((uint8_t)0xE0U)
#define ICM42688_FS_SEL_SHIFT          (5U)
#define ICM42688_ODR_SEL_MASK          ((uint8_t)0x0FU)

/*============================================================================*
 *                          FIFO_CONFIG (0x16)                               *
 *============================================================================*/

#define ICM42688_FIFO_MODE_MASK        ((uint8_t)0xC0U)
#define ICM42688_FIFO_MODE_SHIFT       (6U)
#define ICM42688_FIFO_MODE_BYPASS      ((uint8_t)(0x00U << ICM42688_FIFO_MODE_SHIFT))
#define ICM42688_FIFO_MODE_STREAM      ((uint8_t)(0x01U << ICM42688_FIFO_MODE_SHIFT))
#define ICM42688_FIFO_MODE_STOP_ON_FULL ((uint8_t)(0x02U << ICM42688_FIFO_MODE_SHIFT))

/**
 * @note Simplified FIFO packet model used by this driver and its mock bus:
 *       each FIFO entry is a fixed 12-byte packet of raw accel (6 bytes) +
 *       raw gyro (6 bytes), in the same byte order as the direct data
 *       registers above. The real ICM-42688-P supports several selectable
 *       packet formats (8/16/20 bytes, optional header byte and 16-bit
 *       timestamp, governed by FIFO_CONFIG1) which this driver does not
 *       model — treat FIFO support here as "byte-count and burst-drain
 *       plumbing is real and testable," not as a certified bit-exact
 *       reproduction of every InvenSense packet layout.
 */
#define ICM42688_FIFO_PACKET_BYTES     ((uint16_t)12U)

#endif /* ICM42688_REGISTERS_H */
