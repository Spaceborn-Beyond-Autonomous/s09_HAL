#include "s09_sensor_hal.h"
ICM42688_Status_t hal_imu_read(ICM42688_Handle_t *imu,s09_imu_sample_t *sample){return ICM42688_ReadData(imu,sample);}
BMP388_Status_t hal_baro_read(BMP388_Handle_t *baro,s09_baro_sample_t *sample){return BMP388_ReadData(baro,sample);}
hal_status_t hal_mag_read(HMC5883L_Handle_t *mag,s09_mag_sample_t *sample){return HMC5883L_ReadData(mag,sample);}
