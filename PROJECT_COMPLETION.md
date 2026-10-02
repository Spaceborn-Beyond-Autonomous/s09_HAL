# S09 ANSA HAL Portability & Emulator — Completion Record

This repository is the implementation of the concrete S09 project plan and the Stage 0–4 MVP foundation described by the supplied roadmap documents.

## Source-of-truth alignment

1. `S09-AVHP-ROADMAP-001` defines the long-term AVHP architecture: CPU identity stays below the HAL, QEMU is a backend rather than the platform foundation, and virtual peripherals provide the portable reference implementation.
2. `S09 PROJECT DEVELOPMENT PLAN` defines the concrete 3-engineer bare-metal deliverable: Cortex-M QEMU bring-up, three flight-sensor register models/drivers, VS Code/GDB workflow, pytest verification, and reproducible packaging.

## Implemented

### Stage 0 / architecture
- HAL interfaces and common status codes.
- ADR-001 through ADR-004.
- Coding standards and HAL compliance checklist.
- CI skeleton.

### Stage 1 / HAL framework
- 13 HAL interface headers.
- Null backend and link/stub compliance test.
- S09 sensor-facing `hal_imu_read`, `hal_baro_read`, and `hal_mag_read` shims.

### Stage 2 / AVHP Core
- Host-native virtual memory map with bounds checking.
- Virtual interrupt controller with registration, priority metadata, enable/trigger behavior.
- Host-native monotonic clock and deterministic delay API.
- Boot-sequence orchestration.
- F4/F7/H7 resource profiles.

### Stage 3 / Virtual backend
- Virtual GPIO, UART, SPI, I2C, timer, PWM, CAN, DMA, ADC, flash, sensor and power implementations.
- Reference backend behavior is independent of QEMU.
- Host compliance smoke tests cover representative read/write, callback, queue, DMA and flash behavior.

### Stage 4 / QEMU backend
- Cortex-M boot image, linker script and startup assembly.
- VS Code QEMU/GDB configuration.
- Register-level SPI1 bring-up and real ICM-42688 driver linkage.
- SysTick-based 1 ms interrupt timing in the boot demo.
- Machine substitution is explicitly documented: the supplied project plan names `olimex-stm32-p103`, but the current mainline QEMU target used by this repository is `stm32vldiscovery`.

### S09 peripheral simulation
- Register-level ICM-42688-P, BMP388 and HMC5883L Python models.
- FIFO, data-ready, calibration/compensation and identity behavior.
- JSON-lines TCP bridge for SPI/I2C register transactions.
- 20 passing Python tests, including bridge tests.

### Reproducibility / verification
- Root CMake build and CTest integration.
- Standalone sensor-driver Makefiles remain available.
- Docker toolchain definition for host + ARM + QEMU tooling.
- Regression script covering CMake/CTest, pytest and the socket bridge.

## Verification performed in this delivery environment

Host-side verification:

- CMake build: PASS
- CTest: **7/7 PASS**
- Peripheral Python tests: **20/20 PASS**
- Peripheral socket bridge identity checks: PASS

The delivery environment does **not** contain `arm-none-eabi-gcc` or `qemu-system-arm`, so the ARM cross-build and actual QEMU boot execution were not rerun here. The target files and CI/Docker path are included so the repository can be verified on the intended toolchain.

## Important engineering boundary

This repository should not claim that the complete 18-stage, 58-week AVHP program is finished. Stages 5–17 of the long-term roadmap remain future program work (sensor framework, mission runtime, Reality Engine SIL, fault injection, telemetry, real F7/H7 backends, hardware validation, multi-architecture expansion, Compute Manager, certification, Nano and future hardware). The concrete S09 project deliverable and the Stage 0–4/MVP-1 foundation are what this repository implements.

## Validation dashboard

A local dependency-free validation dashboard is included under `dashboard/`. It executes the existing `scripts/run_all_tests.sh` pipeline and displays its real stdout/results through a browser UI. It includes CTest, pytest, peripheral bridge and optional ARM/QEMU build status, plus the documented separation between QEMU CPU validation and Python sensor simulation.

Dashboard start command:

```bash
./dashboard/run_dashboard.sh
```

Browser endpoint:

```text
http://127.0.0.1:8765
```

The dashboard is a presentation/orchestration layer only; the terminal scripts remain the source of truth for validation.
