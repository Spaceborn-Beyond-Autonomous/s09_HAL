# ANSA / AVHP — ANSA Virtual Hardware Platform

**ANSA Virtual Hardware Platform (AVHP)** is a software-based Hardware Abstraction Layer (HAL) and hardware-emulation foundation for embedded and autonomous systems.

The platform provides a common software interface between application/driver code and the underlying hardware implementation. This allows hardware-facing software to be developed, tested and validated using software backends before deployment on physical hardware.

The repository combines:

* Standardized HAL interfaces
* Virtual and null hardware backends
* Sensor drivers
* Host-side hardware simulation
* Cortex-M firmware execution through QEMU
* Python-based peripheral models
* Automated validation and regression testing
* Socket-based peripheral verification
* Documentation and architectural decision records

The concrete S09 implementation focuses on the flight-sensor driver stack while establishing the Stage 0–4 AVHP foundation.

---

# 1. Project Objective

The primary objective of ANSA/AVHP is to reduce direct dependency on physical hardware during embedded-system development.

A typical embedded application communicates directly with hardware peripherals such as:

```text
Application
    │
    ▼
Sensor Driver
    │
    ▼
HAL / Bus Interface
    │
    ▼
Hardware Peripheral
```

With AVHP, the hardware implementation can be replaced by a software backend:

```text
Application
    │
    ▼
Sensor Driver
    │
    ▼
HAL / Bus Interface
    │
    ├──────────────► Physical Hardware
    │
    ├──────────────► Virtual Backend
    │
    ├──────────────► Mock Backend
    │
    └──────────────► Simulated Peripheral
```

This allows the same driver architecture to be exercised in multiple environments.

---

# 2. Why a HAL is Required

Hardware-specific code normally depends heavily on a particular microcontroller, peripheral controller or vendor SDK.

For example, a sensor driver may need:

* SPI read/write operations
* I2C read/write operations
* GPIO control
* delays
* interrupt handling
* clock access

If these operations are directly implemented inside the sensor driver, moving the driver to another platform becomes difficult.

AVHP separates these responsibilities.

The driver knows **what hardware operation it needs**, while the HAL/backend determines **how that operation is performed**.

For example:

```text
ICM42688 Driver
       │
       │ SPI Read / Write
       ▼
ICM42688 Bus Interface
       │
       ├── STM32 implementation
       ├── Mock implementation
       └── Virtual implementation
```

The driver therefore remains independent of the underlying hardware implementation.

---

# 3. High-Level Architecture

The overall AVHP architecture is organized into several layers.

```text
┌─────────────────────────────────────────────────────────────┐
│                    APPLICATION / SYSTEM                     │
│        Mission Logic • Control • Runtime • Telemetry        │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                       SENSOR DRIVERS                        │
│     ICM-42688-P • BMP388 • HMC5883L • Future Sensors       │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    SENSOR BUS INTERFACE                     │
│          SPI Read/Write • I2C Read/Write • Delay            │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                         AVHP HAL                            │
│       GPIO • UART • SPI • I2C • Timer • Clock • IRQ        │
└──────────────────────────────┬──────────────────────────────┘
                               │
                 ┌─────────────┼──────────────┐
                 │             │              │
                 ▼             ▼              ▼
        ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
        │ Physical HW  │ │ Virtual HW   │ │ Mock / Test  │
        │ Backend      │ │ Backend      │ │ Backend      │
        └──────────────┘ └──────────────┘ └──────────────┘
                 │             │              │
                 ▼             ▼              ▼
             MCU / Board    AVHP Model     Host Tests
```

The important design principle is that **upper layers should not need to know which backend is providing the hardware behavior**.

---

# 4. How ANSA/AVHP Works

A typical sensor operation follows this path:

```text
Application
     │
     │ Request sensor data
     ▼
Sensor Driver
     │
     │ Sensor-specific register operation
     ▼
Sensor Bus Interface
     │
     │ SPI / I2C transaction
     ▼
AVHP HAL
     │
     ├──────── Physical backend
     │
     ├──────── Virtual backend
     │
     └──────── Mock / simulated backend
     │
     ▼
Sensor / Virtual Peripheral
     │
     │ Register response
     ▼
Sensor Driver
     │
     │ Decode / compensate / validate
     ▼
Application
```

For example, an ICM-42688 WHO_AM_I request can be conceptually represented as:

```text
ICM42688_Init()
      │
      ▼
ICM42688 Bus Read
      │
      ▼
SPI HAL
      │
      ▼
Backend
      │
      ▼
WHO_AM_I Register
      │
      ▼
0x47
      │
      ▼
ICM42688 Driver
      │
      ▼
Initialization successful
```

The same driver logic can therefore be tested without requiring the physical sensor.

---

# 5. Repository Structure

```text
s09_HAL/
│
├── hal/
│   └── Stage 1 HAL interfaces
│
├── avhp/
│   └── core/
│       └── Virtual CPU / memory / interrupt foundation
│
├── backends/
│   ├── virtual/
│   │   └── null/
│   │       └── Null HAL backend
│   │
│   └── qemu-cortex-m/
│       └── Cortex-M firmware / QEMU environment
│
├── Drivers/
│   ├── ICM42688/
│   ├── BMP388/
│   └── HMC5883L/
│
├── sensors/
│   └── Virtual Sensor API
│
├── peripheral-sim/
│   └── Python sensor peripheral models
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── virtual/
│   ├── qemu_icm42688/
│   ├── qemu_bmp388/
│   └── qemu_hmc5883l/
│
├── scripts/
│   ├── regression.py
│   ├── run_all_tests.sh
│   └── qemu_machine_survey.sh
│
├── docs/
│   ├── architecture.md
│   ├── adr/
│   └── coding-standards.md
│
├── mission-runtime/
├── compute-manager/
├── telemetry/
├── fault-injection/
├── certification/
│
├── CMakeLists.txt
├── PROJECT_COMPLETION.md
└── README.md
```

The repository contains the broader AVHP architecture as well as the concrete S09 sensor-driver implementation.

---

# 6. HAL Layer

The `hal/` directory contains the platform-independent hardware interfaces.

The HAL is designed to prevent higher-level software from depending directly on vendor-specific types or implementation details.

The Stage 1 HAL contains **13 interfaces** covering the basic hardware abstraction required by the platform.

The fundamental concept is:

```text
HAL Interface
     │
     ▼
Backend Implementation
     │
     ▼
Actual / Virtual Hardware
```

A backend can therefore implement the same interface for different execution environments.

---

# 7. Null Backend

The repository includes a null backend under:

```text
backends/virtual/null/
```

The null backend provides a minimal implementation used to verify that the HAL interfaces:

* compile correctly
* link correctly
* expose the expected API
* behave safely as stubs

It is primarily a structural and integration smoke test rather than a hardware simulator.

---

# 8. Sensor Driver Architecture

The S09 sensor-driver implementation contains three sensors:

| Driver             | Sensor                 | Interface |
| ------------------ | ---------------------- | --------- |
| `Drivers/ICM42688` | ICM-42688-P 6-axis IMU | SPI       |
| `Drivers/BMP388`   | BMP388 Barometer       | SPI / I2C |
| `Drivers/HMC5883L` | HMC5883L Magnetometer  | I2C       |

All three drivers follow the same general design philosophy.

```text
Application
     │
     ▼
Sensor Driver
     │
     ├── Register definitions
     ├── Configuration
     ├── Initialization
     ├── Data acquisition
     └── Sensor-specific processing
     │
     ▼
Bus abstraction
     │
     ├── BusRead
     ├── BusWrite
     └── DelayMs
     │
     ▼
Backend
```

---

# 9. Handle-Based Driver Design

The sensor drivers use handle-based APIs.

For example:

```text
<Sensor>_Handle_t
```

This avoids relying on mutable global driver state.

A handle contains the state required by a particular sensor instance.

Conceptually:

```text
Sensor Handle
├── Configuration
├── Device state
├── Bus context
└── Runtime information
```

This makes the driver architecture easier to reuse and test.

---

# 10. Bus Abstraction

The sensor `.c` files do not directly depend on a particular SPI or I2C implementation.

Instead, they communicate through the sensor-specific bus contract:

```text
<Sensor>_BusRead()
<Sensor>_BusWrite()
<Sensor>_DelayMs()
```

For example:

```text
ICM42688 Driver
      │
      ├── ICM42688_BusRead()
      ├── ICM42688_BusWrite()
      └── ICM42688_DelayMs()
```

The same driver can then be connected to different bus implementations.

```text
                 ┌── STM32 hardware bus
                 │
Driver ── Bus ───┼── Host mock bus
                 │
                 └── Simulated peripheral
```

This is one of the key mechanisms that makes the project hardware-independent.

---

# 11. Using the HAL for New Hardware

A new hardware device can follow the same architecture.

### Step 1 — Define the device interface

Create a device-specific driver API:

```text
Device_Handle_t
Device_Init()
Device_Read()
Device_Write()
Device_GetData()
```

### Step 2 — Define the register map

Keep device registers in a dedicated header:

```text
device_registers.h
```

### Step 3 — Define the bus contract

The driver should use abstract operations:

```text
Device_BusRead()
Device_BusWrite()
Device_DelayMs()
```

### Step 4 — Implement the driver

The driver contains:

* initialization
* configuration
* register operations
* data conversion
* status handling
* device-specific logic

### Step 5 — Implement a backend

Depending on the target:

```text
Physical hardware
        OR
Host mock
        OR
Virtual peripheral
```

### Step 6 — Add tests

The same driver should be validated using a controlled backend before physical deployment.

---

# 12. Python Peripheral Simulation

The directory:

```text
peripheral-sim/
```

contains Python-based register-level models for the S09 sensors.

The models currently cover:

```text
ICM-42688-P
BMP388
HMC5883L
```

The Python layer is useful when the goal is to test peripheral behavior without requiring the actual sensor IC.

Conceptually:

```text
Sensor Driver
      │
      ▼
Bus Transaction
      │
      ▼
Python Peripheral Model
      │
      ▼
Register Response
```

The BMP388 model also includes an independent implementation of the compensation calculation for cross-checking the driver behavior.

---

# 13. Socket-Based Peripheral Verification

The peripheral simulation layer also provides a JSON-lines socket transport.

The verification flow is:

```text
Test / Client
     │
     │ JSON request
     ▼
Socket Bridge
     │
     ▼
Python Peripheral Model
     │
     ▼
Register Response
     │
     ▼
JSON Response
```

The current regression test verifies representative sensor identities:

```text
ICM-42688-P
WHO_AM_I → 0x47

BMP388
CHIP_ID → 0x50

HMC5883L
ID → H 4 3
```

This transport is a verification mechanism for the Python peripheral layer.

It should not be interpreted as a claim that these sensor models are physically attached to the QEMU machine.

---

# 14. Cortex-M and QEMU

The project also contains a Cortex-M firmware environment:

```text
backends/qemu-cortex-m/
```

It provides:

* linker script
* startup assembly
* vector table
* Cortex-M firmware image
* QEMU execution
* GDB debugging configuration

The firmware includes the actual:

```text
Drivers/ICM42688/Src/icm42688.c
```

driver implementation.

The driver is compiled into the embedded firmware rather than being replaced with a simplified demonstration implementation.

---

# 15. QEMU Data Flow

The QEMU execution path is different from the Python peripheral simulation path.

```text
Cortex-M Firmware
       │
       ▼
Application / main()
       │
       ▼
ICM-42688 Driver
       │
       ▼
SPI1 Registers
       │
       ▼
QEMU Machine
       │
       ▼
Virtual MCU
```

The current QEMU machine provides the CPU and MCU environment, but the stock machine does **not** provide an emulated ICM-42688 sensor connected to SPI1.

Therefore:

```text
QEMU
  =
CPU / firmware execution environment

Python peripheral simulator
  =
Register-level sensor simulation
```

These are complementary parts of the project, not the same simulation layer.

---

# 16. What Can Be Tested Without Physical Hardware?

A major purpose of the platform is to allow development before hardware availability.

Without physical sensors, the project can validate:

### Software architecture

```text
HAL interfaces
Driver APIs
Backend contracts
```

### Driver behavior

```text
Initialization
Register access
Configuration
Data conversion
Error handling
```

### Sensor models

```text
Register responses
Device identity
Calibration behavior
Compensation calculations
```

### Embedded execution

```text
Cortex-M startup
Firmware linking
Driver integration
SPI register access
GDB debugging
```

### Automated validation

```text
CMake
CTest
Pytest
Socket bridge
Regression scripts
```

Physical hardware is still required for final hardware-level validation.

---

# 17. Complete Development Flow

A typical development workflow is:

```text
        ┌─────────────────────┐
        │ Design HAL Interface│
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ Implement Driver    │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ Implement Mock Bus  │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ Host Driver Tests   │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ Peripheral Model    │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ QEMU Firmware Build │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ GDB / Embedded Test │
        └──────────┬──────────┘
                   ▼
        ┌─────────────────────┐
        │ Physical Hardware   │
        └─────────────────────┘
```

This creates a progression from software-only validation toward hardware deployment.

---

# 18. Build and Test

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the complete regression:

```bash
./scripts/run_all_tests.sh
```

Or:

```bash
python3 scripts/regression.py
```

Run the Python peripheral tests independently:

```bash
python3 -m pytest -q peripheral-sim/tests
```

Run individual sensor-driver tests:

```bash
make -C tests/qemu_icm42688 run
make -C tests/qemu_bmp388 run
make -C tests/qemu_hmc5883l run
```

Build the Cortex-M firmware:

```bash
make -C backends/qemu-cortex-m/demo
```

The ARM/QEMU step requires:

```text
arm-none-eabi-gcc
qemu-system-arm
```

---

# 19. QEMU Debugging

The firmware can be executed under QEMU:

```bash
cd backends/qemu-cortex-m/demo

qemu-system-arm \
    -M stm32vldiscovery \
    -nographic \
    -kernel boot_demo.elf
```

For GDB debugging:

### Terminal 1

```bash
qemu-system-arm \
    -M stm32vldiscovery \
    -nographic \
    -kernel boot_demo.elf \
    -S \
    -gdb tcp::1234
```

### Terminal 2

```bash
gdb-multiarch boot_demo.elf
```

Then:

```gdb
target remote :1234
break main
continue
break ICM42688_Init
continue
bt
info locals
info args
```

This allows source-level debugging of the embedded driver execution.

---

# 20. Validation Architecture

The project has multiple independent validation layers:

```text
                         ANSA / AVHP
                              │
             ┌────────────────┼────────────────┐
             │                │                │
             ▼                ▼                ▼
       Host C Tests      Python Tests       QEMU Build
             │                │                │
             ▼                ▼                ▼
           CTest            Pytest         ARM GCC
             │                │                │
             └────────────────┼────────────────┘
                              ▼
                    Regression Validation
```

The layers have different purposes:

| Layer             | Purpose                                    |
| ----------------- | ------------------------------------------ |
| CTest             | Host-side C/C++ validation                 |
| Pytest            | Python peripheral-model validation         |
| Socket bridge     | Sensor register communication verification |
| QEMU              | Embedded firmware execution                |
| GDB               | Embedded debugging                         |
| Physical hardware | Final hardware validation                  |

---

# 21. Current Verification

The current delivery has been validated through:

```text
CTest
7 / 7 tests passing

Pytest
20 / 20 tests passing

Peripheral socket bridge
PASS
```

The ARM/QEMU validation depends on the local environment providing:

```text
arm-none-eabi-gcc
qemu-system-arm
```

If these tools are unavailable, the host and Python validation can still be executed independently.

---

# 22. Important Scope Boundary

ANSA/AVHP is designed as a broader multi-stage virtual hardware platform.

The repository contains the architectural foundation for the larger AVHP roadmap, while the concrete S09 implementation focuses on the flight-sensor driver stack and the Stage 0–4 foundation.

The current S09 implementation includes:

* ICM-42688-P driver
* BMP388 driver
* HMC5883L driver
* HAL interfaces
* Null backend
* Virtual backend foundation
* Cortex-M firmware environment
* QEMU boot/debug workflow
* Python peripheral models
* Socket-based verification
* Automated regression testing

The following remain outside the current concrete S09 implementation:

* Full hardware-accurate QEMU models for all three sensors
* A complete serial/named-pipe connection between the Python peripheral models and QEMU firmware
* BMP388 and HMC5883L integration into the current QEMU boot image
* Cycle-accurate QEMU timing
* The complete Stage 5–17 long-term AVHP roadmap

These boundaries are intentionally documented so that software simulation results are not presented as physical-hardware validation.

---

# 23. Known Simplifications

### ICM-42688 FIFO

The current FIFO implementation models the relevant FIFO registers and a fixed 12-byte accelerometer + gyroscope packet.

The real device supports additional FIFO packet formats and options that are not modeled here.

### BMP388 Configuration

The current oversampling and output-data-rate defaults are reasonable development values but should be reconsidered against the final airframe vibration and update-rate requirements.

### BMP388 Calibration

The host mock uses a synthetic but internally consistent calibration set.

For bit-exact physical-device behavior, calibration data from the actual sensor should be used.

---

# 24. Design Principles

The project follows several important design principles.

### Hardware independence

Drivers should not unnecessarily depend on a particular MCU or board.

### Explicit interfaces

Hardware operations are exposed through defined interfaces instead of hidden dependencies.

### Testability

Every hardware-facing component should have a way to be exercised without requiring physical hardware wherever practical.

### Reproducibility

Build and test operations should be executable through documented commands and automated scripts.

### Clear simulation boundaries

Software simulation results must not be presented as physical-hardware measurements.

### Replaceable backends

A HAL interface should be implementable by multiple backends without rewriting the higher-level driver.

---

# 25. Using ANSA/AVHP in a New Embedded Project

A new project can use the platform following this general pattern:

```text
1. Define hardware abstraction
          ↓
2. Implement device driver
          ↓
3. Connect driver to HAL
          ↓
4. Create host/mock backend
          ↓
5. Write automated tests
          ↓
6. Add virtual peripheral model
          ↓
7. Integrate with embedded firmware
          ↓
8. Execute under QEMU where supported
          ↓
9. Debug with GDB
          ↓
10. Validate on physical hardware
```

This allows much of the software development and validation to happen before the final hardware is available.

---

# 26. Documentation

Important project documentation is available under:

```text
docs/
```

Architecture:

```text
docs/architecture.md
```

Architectural decisions:

```text
docs/adr/
```

Coding standards:

```text
docs/coding-standards.md
```

HAL compliance checklist:

```text
docs/hal-compliance-checklist.md
```

QEMU documentation:

```text
backends/qemu-cortex-m/README.md
```

Python peripheral simulation:

```text
peripheral-sim/README.md
```

Project completion and verification:

```text
PROJECT_COMPLETION.md
```

---

# 27. Summary

ANSA/AVHP provides a structured software environment for developing hardware-facing embedded software with reduced dependence on physical hardware.

Its central architecture is:

```text
Application
     │
     ▼
Driver
     │
     ▼
HAL
     │
     ▼
Backend
     │
     ├── Physical Hardware
     ├── Virtual Hardware
     └── Simulation / Mock
```

For the S09 sensor stack:

```text
ICM-42688-P ── SPI ──┐
                     │
BMP388 ───── SPI/I2C ├──► HAL / Bus Abstraction
                     │
HMC5883L ──── I2C ───┘
                              │
                 ┌────────────┼────────────┐
                 ▼            ▼            ▼
             Host Tests   Peripheral    QEMU /
                          Simulation    Firmware
```

The result is a development workflow where sensor drivers and hardware-facing software can be designed, tested, simulated, debugged and integrated progressively—from host software to embedded firmware and finally to physical hardware.

