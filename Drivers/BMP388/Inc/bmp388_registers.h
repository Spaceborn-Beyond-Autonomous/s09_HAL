/**
 * @file    bmp388_registers.h
 * @brief   Private register map and calibration-NVM layout for the Bosch
 *          BMP388 barometric pressure sensor.
 * @details Sourced from the Bosch BMP388 Register Map / Trimming Coefficient
 *          sections. Private header: include only from bmp388.c and the
 *          matching bus implementations.
 */

#ifndef BMP388_REGISTERS_H
#define BMP388_REGISTERS_H

#include <stdint.h>

/*============================================================================*
 *                          REGISTER ADDRESS MAP                              *
 *============================================================================*/

#define BMP388_REG_CHIP_ID     ((uint8_t)0x00U) /**< Fixed chip identifier. */
#define BMP388_REG_ERR_REG     ((uint8_t)0x02U) /**< Fatal/cmd/config error flags. */
#define BMP388_REG_STATUS      ((uint8_t)0x03U) /**< cmd_rdy / drdy_press / drdy_temp. */

/** @brief Raw pressure (3 bytes) then raw temperature (3 bytes), 6 bytes
 *         total, contiguous and burst-readable in one transaction. */
#define BMP388_REG_DATA_0      ((uint8_t)0x04U) /**< PRESS_XLSB */
#define BMP388_REG_DATA_1      ((uint8_t)0x05U) /**< PRESS_LSB */
#define BMP388_REG_DATA_2      ((uint8_t)0x06U) /**< PRESS_MSB */
#define BMP388_REG_DATA_3      ((uint8_t)0x07U) /**< TEMP_XLSB */
#define BMP388_REG_DATA_4      ((uint8_t)0x08U) /**< TEMP_LSB */
#define BMP388_REG_DATA_5      ((uint8_t)0x09U) /**< TEMP_MSB */

#define BMP388_REG_PWR_CTRL    ((uint8_t)0x1BU) /**< press_en / temp_en / mode. */
#define BMP388_REG_OSR         ((uint8_t)0x1CU) /**< Oversampling: osr_p[2:0], osr_t[5:3]. */
#define BMP388_REG_ODR         ((uint8_t)0x1DU) /**< Output data rate divider. */
#define BMP388_REG_CONFIG      ((uint8_t)0x1FU) /**< IIR filter coefficient. */

/** @brief 21-byte NVM trimming-coefficient block (par_t1..par_p11, plus
 *         reserved), little-endian, burst-readable in one transaction. */
#define BMP388_REG_CALIB_DATA  ((uint8_t)0x31U)
#define BMP388_CALIB_DATA_LEN  ((uint16_t)21U)

#define BMP388_REG_CMD         ((uint8_t)0x7EU) /**< Command register (soft reset). */

/*============================================================================*
 *                        IDENTITY / RESET CONSTANTS                          *
 *============================================================================*/

#define BMP388_CHIP_ID_VALUE       ((uint8_t)0x50U)
#define BMP388_CMD_SOFT_RESET      ((uint8_t)0xB6U)

/*============================================================================*
 *                            STATUS (0x03) BITS                              *
 *============================================================================*/

#define BMP388_STATUS_CMD_RDY      ((uint8_t)0x10U) /**< Ready for a new command. */
#define BMP388_STATUS_DRDY_PRESS   ((uint8_t)0x20U) /**< Fresh pressure sample available. */
#define BMP388_STATUS_DRDY_TEMP    ((uint8_t)0x40U) /**< Fresh temperature sample available. */

/*============================================================================*
 *                            PWR_CTRL (0x1B) BITS                            *
 *============================================================================*/

#define BMP388_PWR_PRESS_EN        ((uint8_t)0x01U)
#define BMP388_PWR_TEMP_EN         ((uint8_t)0x02U)

#define BMP388_PWR_MODE_MASK       ((uint8_t)0x30U)
#define BMP388_PWR_MODE_SLEEP      ((uint8_t)0x00U)
#define BMP388_PWR_MODE_FORCED     ((uint8_t)0x10U)
#define BMP388_PWR_MODE_NORMAL     ((uint8_t)0x30U)

/** @brief Default operating point applied by BMP388_Init(): both
 *         measurement channels enabled, continuous normal mode. */
#define BMP388_PWR_CTRL_DEFAULT_RUN \
    ((uint8_t)(BMP388_PWR_PRESS_EN | BMP388_PWR_TEMP_EN | BMP388_PWR_MODE_NORMAL))

/*============================================================================*
 *                              OSR (0x1C) FIELD                              *
 *============================================================================*/

#define BMP388_OSR_P_SHIFT     (0U)
#define BMP388_OSR_T_SHIFT     (3U)

/** @brief x1/x2/x4/x8/x16/x32 oversampling selectors, shared field encoding
 *         for both osr_p[2:0] and osr_t[5:3]. */
typedef enum {
    BMP388_OSR_X1  = 0x00,
    BMP388_OSR_X2  = 0x01,
    BMP388_OSR_X4  = 0x02,
    BMP388_OSR_X8  = 0x03,
    BMP388_OSR_X16 = 0x04,
    BMP388_OSR_X32 = 0x05
} Bmp388Osr_t;

/** @brief Init() default: Bosch "high resolution" preset (pressure x8,
 *         temperature x1 — temperature does not need heavy oversampling). */
#define BMP388_OSR_DEFAULT \
    ((uint8_t)((BMP388_OSR_X8 << BMP388_OSR_P_SHIFT) | (BMP388_OSR_X1 << BMP388_OSR_T_SHIFT)))

/*============================================================================*
 *                              ODR (0x1D) FIELD                              *
 *============================================================================*/

/** @brief odr_sel selects a divider from the 200 Hz base rate:
 *         ODR = 200 Hz / 2^odr_sel. Init() default (0x03) -> 25 Hz, a
 *         comfortable rate for altitude-hold on a non-racing airframe. */
#define BMP388_ODR_25HZ        ((uint8_t)0x03U)

#endif /* BMP388_REGISTERS_H */
