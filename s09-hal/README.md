# S09 Bare-Metal Sensor HAL shims

These three functions are the explicit application-facing shims requested by the S09 build specification: `hal_imu_read()`, `hal_baro_read()`, and `hal_mag_read()`. They do not own bus state; each concrete sensor driver continues to own its opaque bus contract and handle state.
