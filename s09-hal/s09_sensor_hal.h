#ifndef S09_SENSOR_HAL_H
#define S09_SENSOR_HAL_H
#include "icm42688.h"
#include "bmp388.h"
#include "hmc5883l.h"

typedef ICM42688_RawData_t s09_imu_sample_t;
typedef BMP388_Data_t s09_baro_sample_t;
typedef HMC5883L_RawData_t s09_mag_sample_t;

ICM42688_Status_t hal_imu_read(ICM42688_Handle_t *imu, s09_imu_sample_t *sample);
BMP388_Status_t hal_baro_read(BMP388_Handle_t *baro, s09_baro_sample_t *sample);
hal_status_t hal_mag_read(HMC5883L_Handle_t *mag, s09_mag_sample_t *sample);
#endif
