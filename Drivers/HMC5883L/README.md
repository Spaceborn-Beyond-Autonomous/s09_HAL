# Honeywell HMC5883L 3-Axis Digital Compass ANSA Driver

This directory contains the production-ready ANSA Hardware Abstraction Layer (HAL) driver for the **Honeywell HMC5883L 3-Axis Digital Compass IC**. 

The implementation strictly enforces a **stateless, handle-based architecture** and a **raw-only measurement pipeline**, fully conforming to the ANSA HAL Driver Framework Specifications.

---

## 1. Datasheet & Reference Metadata

*   **Manufacturer:** Honeywell Microelectronics & Precision Sensors
*   **Part Number:** HMC5883L 3-Axis Digital Compass IC
*   **Datasheet Reference:** Honeywell HMC5883L Datasheet, Form # 900405 Rev E, February 2013
*   **Operating Voltage:** 2.16V to 3.6V VDD (analog core), 1.71V to VDD VDDIO (digital I/O) (*Honeywell HMC5883L Datasheet, "Absolute Maximum Ratings" page 9, and "Pin Configurations" page 9*)
*   **Sensor Type:** Anisotropic Magnetoresistive (AMR) thin-film nickel-iron (Permalloy) bridge array (*Honeywell HMC5883L Datasheet, "Anisotropic Magneto-Resistive Sensors", page 11*)

---

## 2. Directory Layout & Manifest

The driver and testing resources are organized within the repository as follows
(paths below reflect this driver's place in the integrated `ansa/` repo —
flattened to `Drivers/HMC5883L/`, matching the other two S09 sensor drivers,
rather than the original submission's `drivers/mag/hmc5883l/` nesting):

```text
ansa/
├── Drivers/
│   └── HMC5883L/
│       ├── Inc/
│       │   ├── hmc5883l.h             # Public interface: API declarations, handle, and status enums
│       │   └── hmc5883l_registers.h   # Private hardware interface: register offsets and register bitmasks
│       ├── Src/
│       │   └── hmc5883l.c             # Core chip-control logic and Device Manager vtable
│       └── README.md                  # This documentation file (updated version)
│
└── tests/
    └── qemu_hmc5883l/                 # Standalone QEMU/host testing suite
        ├── Makefile                   # Plain-make build (DRIVER_STANDARD.md's literal ask)
        ├── CMakeLists.txt             # Original build option, path/filename bugs fixed
        ├── mock_hmc5883l_bus.c        # High-fidelity headerless register state simulator
        └── main.c                     # Automated test runner with fault injection
```

See the repo root `README.md`'s "Sensor drivers" section for the two
functional bugs (an `Init()`/`Reset()` config-clobbering order issue, and a
self-test simulation gap) and the build-script issue fixed when integrating
this driver alongside the two new ones.

---

## 3. Physical Wiring & Bus Specifications

### 3.1 I²C Bus Abstraction
Control of the HMC5883L is carried out over a standard two-wire I²C serial bus (*Honeywell HMC5883L Datasheet, "I2C Interface", pages 11–12*):
*   **Physical 7-bit slave address:** `0x1E` (*Honeywell HMC5883L Datasheet, "Specifications Table", page 8*)
*   **8-bit Write Address equivalent:** `0x3C` (*Honeywell HMC5883L Datasheet, "Specifications Table" page 8, and "Register Access" page 12*)
*   **8-bit Read Address equivalent:** `0x3D` (*Honeywell HMC5883L Datasheet, "Specifications Table" page 8, and "Register Access" page 12*)
*   **Bus rates supported:** Standard Mode (up to 100 kHz) and Fast Mode (up to 400 kHz) (*Honeywell HMC5883L Datasheet, "I2C Interface", page 11*).
*   **High-Speed (Hs) mode is explicitly NOT supported by the chip** (*Honeywell HMC5883L Datasheet, "I2C Interface", page 11*).

### 3.2 Required Pins and External Support Components
To ensure stable measurements and accommodate the high switching currents of the set/reset strap driver, several external capacitors are mandatory (*Honeywell HMC5883L Datasheet, "Features & Benefits" page 7, and "Pin Configurations" page 9*):

1.  **SCL (Pin 1) & SDA (Pin 16):** I²C clock and data lines. Require external pull-up resistors ($R_p$) of 2.2 k$\Omega$ to 10 k$\\Omega$ tied to the $V_{DDIO}$ voltage domain (*Honeywell HMC5883L Datasheet, "I2C Communication Protocol", page 15*).
2.  **$V_{DD}$ (Pin 2):** Main analog power supply. Requires a $0.1\\,\mu\\text{F}$ decoupling ceramic capacitor to ground (*Honeywell HMC5883L Datasheet, "Pin Configurations" page 9, and "Single/Dual Supply Reference Design" page 10*).
3.  **$V_{DDIO}$ (Pin 13) & S1 (Pin 4):** Digital interface power and tie-high. $S1$ must be tied high to $V_{DDIO}$ (*Honeywell HMC5883L Datasheet, "Pin Configurations", page 9*).
4.  **C1 (Pin 10):** Reservoir capacitor. Connect a $4.7\\,\mu\\text{F}$ ceramic capacitor with very low equivalent series resistance (ESR $< 200\\,\text{m}\\Omega$) between C1 and ground (*Honeywell HMC5883L Datasheet, "External Capacitors" page 9, and "Single/Dual Supply Reference Design" page 10*). This limits the current draw on $V_{DD}$ during capacitor recharge (*Honeywell HMC5883L Datasheet, "Charge Current Limit", page 11*).
5.  **SETP (Pin 8) & SETC (Pin 12):** Set/Reset strap connections. Connect a $0.22\\,\mu\\text{F}$ low-ESR ceramic capacitor between Pin 8 and Pin 12 (*Honeywell HMC5883L Datasheet, "External Capacitors" page 9, and "Single/Dual Supply Reference Design" page 10*). The internal H-bridge switches large current pulses (${\sim}1\\,\text{A}$) through this strap on every measurement to degauss the sensor elements and eliminate thermal offset drift (*Honeywell HMC5883L Datasheet, "Reflow Assembly" page 8, and "H-Bridge for Set/Reset Strap Drive" page 11*).
6.  **DRDY (Pin 15):** Data Ready hardware interrupt (optional). Pulled low for $250\\,\mu\\text{s}$ when fresh measurement data is available in the output registers (*Honeywell HMC5883L Datasheet, "Pin Configurations", page 9*).

---

## 4. Driver Architecture & Standard Public APIs

The driver uses a **stateless design pattern**. No global variables exist. Every functional callback takes a pointer to a mutable `HMC5883L_Handle_t` as its first parameter to enable multi-instance scaling and thread-safety on real-time targets (*ANSA HAL Driver Standard, Section 2: "Required driver architecture"*):

### 4.1 Required Baseline Functions

1.  **`HMC5883L_Init(handle)`** (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*)
    *   *Purpose:* Verifies the chip identity string (`H43`) in Identification Registers A, B, and C, soft-resets the registers, and configures the target Output Data Rate (ODR), sample averaging, magnetic gain, and operating mode (*ANSA HAL Driver Standard, Section 3: "Required public API surface" and Honeywell HMC5883L Datasheet, "Register List", page 11*).
2.  **`HMC5883L_DeInit(handle)`** (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*)
    *   *Purpose:* Puts the device in the lowest-power safe state (Idle Mode) (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*).
3.  **`HMC5883L_ReadDeviceID(handle, id_a, id_b, id_c)`** (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*)
    *   *Purpose:* Conducts a 3-byte burst read starting at register `0x0A` to query the ASCII identification signature (expected: `id_a='H'`, `id_b='4'`, `id_c='3'`) (*ANSA HAL Driver Standard, Section 3: "Required public API surface", and Honeywell Datasheet "Register List" page 11, and "Identification Register A, B, and C" pages 13-14*).
4.  **`HMC5883L_Reset(handle)`** (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*)
    *   *Purpose:* Forces the device configuration and mode registers back to their physical power-on defaults (`CRA=0x10`, `CRB=0x20`, `MODE=0x01`) (*ANSA HAL Driver Standard, Section 3: "Required public API surface" and Honeywell Datasheet "Configuration Register A" page 12*).
5.  **`HMC5883L_ReadData(handle, data)`** (*ANSA HAL Driver Standard, Section 3: "Required public API surface"*)
    *   *Purpose:* Conducts a **6-byte block burst read** starting from the raw X MSB register `0x03` (*Honeywell HMC5883L Datasheet, "Data Output X, Y, and Z Registers" page 12, and "Operational Examples" page 14*). It decodes the register layout sequence in the exact order output by the physical Honeywell ASIC: **X MSB, X LSB, Z MSB, Z LSB, Y MSB, Y LSB** (*Honeywell HMC5883L Datasheet, "Data Output X, Y, and Z Registers", pages 12-13, and "Operational Examples" page 14*) and timestamps the sample at the lowest boundary using the free-running hardware counter (*Honeywell HMC5883L Datasheet, "Data Output Register Operation" page 13, and ANSA HAL Sensor Stack Specification, Section 9.1: "Stage-by-Stage"*).

### 4.2 Raw-Only Execution Rule
Per Section 10.4 of the ANSA engineering roadmap, the driver performs **zero unit conversion, zero offset subtraction, and zero calibration**. It reports measurement readings as raw, 12-bit, big-endian 2's complement integers (*Honeywell Datasheet "Data Output X, Y, and Z Registers", page 12*). The processed conversions ($LSB \\rightarrow \\text{SI Gauss}$ units) and coordinate frame calibrations are applied downstream in the Sensor Stack Processing and Calibration layers (*ANSA HAL Sensor Stack Specification, Section 9.1: "Stage-by-Stage" page 18, Honeywell Datasheet "Specifications Table" page 8, and ANSA HAL Sensor Stack Specification, Section 6.2: "Layer 2 — Driver Framework" page 15*).

---

## 5. Built-In Self-Test (BIST) & Calibration Support

### 5.1 Self-Test Diagnostic Logic (`HMC5883L_RunSelfTest`)
To verify full hardware functionality after assembly without an external magnetic turntable, the driver implements the Honeywell datasheet self-test sequence (*Honeywell HMC5883L Datasheet, "Features & Benefits" page 7, and "Self Test" page 11*):

1.  **State Caching:** Caches the current run-configurations (averaging, ODR, bias, gain, and mode) (*Honeywell HMC5883L Datasheet, "Self Test Operation", page 14*).
2.  **Test Configuration:** Configures Configuration Register A for Positive Bias excitation mode (`MS[1:0] = 01`) with 8-sample averaging and 15 Hz ODR (`CRA = 0x71`) (*Honeywell HMC5883L Datasheet, "Self Test Operation", pages 14-15*).
3.  **Gain Matching:** Sets register B to Gain Setting 5 (`CRB = 0xA0`, which corresponds to $\\pm 4.7\\,\\text{Ga}$ range and a sensitivity of $390\\,\\text{LSB}/\\text{Gauss}$) (*Honeywell HMC5883L Datasheet, "Self Test Operation", pages 14-15*).
4.  **Double-Acquisition Cycle:** To bypass physical gain settings change latency (see Section 6 Errata), the driver issues two consecutive single-measurement triggers. The first read is discarded; the second read is captured (*Honeywell HMC5883L Datasheet, "Configuration Register B" page 12, and "Self Test Operation" page 14*).
5.  **Limits Checking:** The internal current source applies a nominal excitation field of $\\pm 1.16\\,\\text{Ga}$ on the X/Y axes and $\\pm 1.08\\,\\text{Ga}$ on the Z axis (*Honeywell HMC5883L Datasheet, "Specifications Table" page 8, and "Self Test" page 11*). Under Gain Setting 5 ($390\\,\\text{LSB}/\\text{Gauss}$), the driver verifies that the retrieved raw measurements fall strictly within the datasheet boundaries of **243 to 575 LSBs on all three axes** (*Honeywell HMC5883L Datasheet, "Specifications Table" page 8, and "Self Test Operation" page 14*).
6.  **State Restoration:** Fully restores original operational parameters to the chip on exit (*Honeywell HMC5883L Datasheet, "Self Test Operation", page 14*).

### 5.2 Dynamic Temperature Compensation Support
The physical self-test output can be utilized by the downstream Calibration Manager to periodically compensate for thermal sensitivity drift without requiring a separate physical temperature sensor (*Honeywell HMC5883L Datasheet, "Self Test" page 11, and "Scale Factor Temperature Compensation" page 15*). 

By comparing the live self-test outputs ($X_{ST\\_current}$) with baseline test values recorded at calibration time ($X_{ST\\_cal}$), the Calibration layer applies a sensitivity scaling correction factor (*Honeywell HMC5883L Datasheet, "Scale Factor Temperature Compensation", page 15*):

$$X_{comp} = X_{raw} \\times \\left(\\frac{X_{ST\\_cal}}{X_{ST\\_current}}\\right)$$

---

## 6. Known Hardware Errata & Layout Constraints

### 6.1 Gain Change Latency Erratum
*   **Erratum Description:** As documented in the HMC5883L datasheet (page 12), the very first measurement completed immediately after modifying Configuration Register B (`CRB`) retains the *previous* gain scale factor (*Honeywell HMC5883L Datasheet, "Configuration Register B", page 12*).
*   **Workaround:** The new gain setting is only physically effective starting from the second measurement onward (*Honeywell HMC5883L Datasheet, "Configuration Register B", page 12*). The driver's `HMC5883L_RunSelfTest` API automatically handles this by executing a double single-measurement acquisition sequence to safely flush the stale configuration (*Honeywell HMC5883L Datasheet, "Self Test Operation" page 14*).

### 6.2 Eddy-Current and Ferrous Layout Constraints
*   **Errata Description:** Anisotropic Magnetoresistive sensors are highly sensitive to microscopic magnetic deviations and stray DC fields. 
*   **PCB Constraints:** 
    *   All trace pathways routing power to the set/reset strap capacitors ($C1$ and $C2$) must be sized generously to support the transient $1\\,\text{A}$ current spikes without inducing substantial voltage drops (*Honeywell HMC5883L Datasheet, "PCB Pad Definition and Traces", page 9*).
    *   No copper fill or conducting planes are permitted on *any layer* of the PCB directly under or near the sensor footprint to prevent eddy-current loop induced noise (*Honeywell HMC5883L Datasheet, "Layout Considerations", page 9*).
    *   Keep all components that contain ferrous materials (such as nickel) away from the sensor to prevent static hard-iron biases (*Honeywell HMC5883L Datasheet, "Layout Considerations", page 9*).
