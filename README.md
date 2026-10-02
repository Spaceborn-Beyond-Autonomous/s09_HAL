# ANSA / AVHP — ANSA Virtual Hardware Platform

Implements Stage 0 (architecture/ADRs) and Stage 1 (HAL interfaces +
null backend) of `S09-AVHP-ROADMAP-001`. See `docs/architecture.md` for
the full picture and `docs/adr/` for the binding decisions.

## Quick start

```sh
# Compile-and-link check (no build system required):
gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Ihal -Ibackends/virtual/null \
  -c backends/virtual/null/null_backend.c -o /tmp/null_backend.o
gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Ihal -Ibackends/virtual/null \
  tests/unit/test_null_backend.c /tmp/null_backend.o -o /tmp/test_null_backend
/tmp/test_null_backend

# Or, with CMake:
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

Both should print:
```
PASS: all HAL headers link and null-backend stubs behave as expected (13/13 headers)
```

## Repository layout

```
ansa/
  hal/                    Stage 1 — pure HAL interfaces (13 headers, zero
                           vendor-specific types; see ADR-001)
  avhp/
    core/                 Stage 2 — virtual CPU/memory/interrupt substrate
    peripherals/          Stage 3 — VirtualGPIO/UART/SPI/... implementations
  backends/
    virtual/
      null/               Stage 1 — null backend: stub, link-only smoke test
    qemu-cortex-m/         Stage 4 (also holds the S09 boot demo — see
                           "Sensor drivers" below)
    stm32f7/                Stage 10
    stm32h7/                 Stage 11
    esp32/ rp2040/ linux/     Stage 13
    raspberry-pi/ jetson/      Stage 13
    rk3588/                     Stage 13
    ansa-nano/                   Stage 16
  sensors/                Stage 5 — Virtual Sensor API (GPS/IMU/baro/...)
  Drivers/                S09 project-plan scope — see "Sensor drivers" below
  peripheral-sim/         S09 project-plan scope — Python HAL verification
                           layer, see "Sensor drivers" below
  fault-injection/        Stage 8
  telemetry/               Stage 9
  mission-runtime/          Stage 6 — FSM, scheduler, control logic, safety
  compute-manager/           Stage 14 — CPU/GPU/NPU/DSP/FPGA task abstraction
  certification/               Stage 15 — OEM certification pipeline
  tests/
    hal_compliance/       Stage 3 onward — behavioral compliance suite,
                           run against every registered backend
    unit/                 Stage 1 — test_null_backend.c (this repo's
                           current exit-criteria test)
    integration/
    qemu_icm42688/         S09 — see "Sensor drivers" below
    qemu_bmp388/            S09
    qemu_hmc5883l/          S09
  docs/
    adr/                  ADR-001..004 (full text)
    architecture.md        Stage 0 canonical architecture reference
    coding-standards.md     Stage 0 deliverable
    hal-compliance-checklist.md   Stage 1 deliverable / Stage 15 seed
    backend-limitations/    One file per backend (populated from Stage 4
                             onward)
  .github/workflows/       CI skeleton (Stage 0 task: "set up CI skeleton")
```

## Sensor drivers (S09 project-plan scope)

`Drivers/` implements the sensor-driver slice of `s09_project_plan.pdf`
(QEMU Olimex STM32-P103 HAL emulator for the flight sensor stack), built to
`DRIVER_STANDARD.md`. This is a narrower, concrete deliverable, not a claim
that Stage 2+ of the full AVHP roadmap is done — see "Scope" below.

| Driver | Chip | Bus | Status |
|---|---|---|---|
| `Drivers/ICM42688` | ICM-42688-P (6-axis IMU) | SPI | New |
| `Drivers/BMP388` | BMP388 (barometer) | SPI/I2C | New |
| `Drivers/HMC5883L` | HMC5883L (magnetometer) | I2C | Retained (integrated from the existing submission, two functional bugs fixed — see below) |

**Run everything:**
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
This builds and runs all four registered tests (Stage 1's null-backend test
plus the three sensor drivers) exactly the way `.github/workflows/ci.yml`
does. Each driver's `tests/qemu_<sensor>/` also builds standalone
(`make run` — plain Makefile, no CMake needed; `tests/qemu_hmc5883l/` also
still has its original `CMakeLists.txt`, fixed, as a second option).

**Architecture**, applied identically across all three:
- Handle-based (`<Sensor>_Handle_t`), no static/global mutable driver state.
- Bus-agnostic: driver `.c` files call only `<S>_BusRead` / `<S>_BusWrite` /
  `<S>_DelayMs` (DRIVER_STANDARD.md Section 4). Exactly one implementation
  of that contract is linked per binary — `<sensor>_bus_stm32.c` for real
  hardware (gated behind an `_TARGET_STM32` macro so it's inert unless
  explicitly built against STM32Cube HAL) or `mock_<sensor>_bus.c` for the
  host/QEMU test.
- Granular per-driver status enum (`ICM42688_Status_t`, `BMP388_Status_t`),
  not a shared/generic ok-or-error type.
- Register maps live in their own `<sensor>_registers.h`, Doxygen'd.

**Known simplifications** (flagged here rather than presented as more
precise than they are):
- ICM-42688-P FIFO support (`ICM42688_FifoEnable/GetCount/Read`) models
  `FIFO_CONFIG` / `FIFO_COUNTH` / `FIFO_COUNTL` / `FIFO_DATA` at their real
  register addresses with a fixed 12-byte accel+gyro packet. The real chip
  supports several selectable packet formats (8/16/20 bytes, optional
  header + timestamp, via `FIFO_CONFIG1`, not modeled here).
- BMP388's oversampling/ODR defaults (x8 pressure / x1 temperature, 25 Hz)
  are a reasonable non-racing-airframe choice, not a value taken from a
  specific integration spec — reconsider against the actual airframe's
  vibration/update-rate needs before flight.
- `mock_bmp388_bus.c`'s 21-byte calibration block is a synthetic-but-
  internally-consistent trim set (generated by inverting `bmp388.c`'s own
  `parse_calibration()` scale factors against target coefficients, not
  copied from one physical unit) — decoding it through the real
  compensation formula reproduces a plausible ~25 °C / ~101.3 kPa reading.
  Swap in a real unit's calibration dump if bit-exact hardware behavior
  needs to be reproduced.

**HMC5883L integration fixes.** The retained submission's driver logic and
register map were correct, but three bugs kept it from actually passing its
own test suite or building via its own `CMakeLists.txt`:
1. `HMC5883L_Init()` called `HMC5883L_Reset()` — which intentionally snaps
   `handle->averaging/odr/bias/gain/mode` back to power-on defaults, to
   mirror the real device post-reset — and then read those *just-defaulted*
   fields when applying the caller's requested configuration, silently
   discarding it. Fixed by capturing the caller's requested config into
   locals before calling `Reset()`.
2. `tests/qemu_hmc5883l/CMakeLists.txt` pointed at `../../drivers/mag/
   hmc5883l/Inc` (wrong case, and the extra `mag/` nesting this repo drops
   for consistency with the other two drivers) and at source files
   (`test_hmc5883l-v2.c`, `mock_hmc5883l_bus-v2.c`) that don't exist in this
   submission — it could not have built as committed. Paths and filenames
   fixed; a plain `Makefile` (matching `DRIVER_STANDARD.md`'s literal ask)
   is provided alongside it.
3. `mock_hmc5883l_bus.c`'s self-test simulation (bias mode) always returned
   the ideal datasheet excitation response regardless of
   `mock_hmc5883l_set_static_field()`, so `test_self_test_verification_
   failure_path` could never observe a failing self-test. Added a
   `sim_self_test_override_active` flag so an explicit static-field
   override also applies during self-test simulation.

Also fixed in passing: a doxygen comment in `hmc5883l_registers.h` was
missing its closing `*/` and silently swallowed the next line, dropping
the `HMC5883L_CRA_MA_2` macro (unused by the driver, so harmless, but worth
not shipping); and a stray top-level `;` after the `DRIVER_REGISTER(...)`
macro invocation (a `-Wpedantic` warning, not a bug).

**Also implemented (Engineer 1 + Engineer 2 workstreams):**
- `backends/qemu-cortex-m/` — a real Cortex-M3 boot image (linker script,
  startup assembly, vector table) running under QEMU, with
  `Drivers/ICM42688/Src/icm42688.c` linked in **completely unmodified**
  from the host-mock-tested version, talking to real (empirically-verified,
  not assumed) SPI1 hardware registers. VS Code `launch.json`/`tasks.json`
  for QEMU+GDB debugging, checked working end-to-end (breakpoints,
  source-line stepping, variable inspection). Read that folder's own
  README before trusting the output as more than it is — in particular,
  QEMU has no machine named "Olimex STM32-P103" (checked directly), so
  this targets the closest real match instead, and there is no emulated
  chip on the other end of SPI1 to answer with real sensor data.
- `peripheral-sim/peripheral_sim.py` — the Python-driven HAL verification
  layer named in `s09_project_plan.pdf` §2: register-accurate, dependency-
  free Python models of all three sensors (18 passing tests), including an
  independent re-implementation of the BMP388 compensation formula for
  cross-checking. Not wired to a live serial/QEMU bridge — see that
  folder's README for exactly where that boundary sits.

**Scope — what this is not.** `S09 HAL SIMULATOR.pdf` describes an 18-stage
(Stage 0–17), multi-team, multi-month roadmap for the full AVHP platform.
This delivers the sensor-driver slice of the *3-engineer S09 project*
(`s09_project_plan.pdf`) specifically — not Stage 2 onward of that larger
roadmap. Still open, even within the S09 project's own scope:
- A serial/named-pipe bridge actually connecting `peripheral_sim.py` to
  the QEMU boot demo (or to real firmware) — the register models exist,
  the transport doesn't.
- BMP388 and HMC5883L wired into the QEMU boot image alongside the IMU —
  BMP388 could reuse the existing SPI1 wiring; HMC5883L cannot, because
  I2C1 is not usably modeled in this QEMU machine (see
  `backends/qemu-cortex-m/README.md`).
- Cycle-accurate timing on the QEMU target (`ICM42688_DelayMs()` there is
  an uncalibrated busy-loop; no SysTick is configured).


## Current implementation status

The repository now implements the concrete S09 project plan plus the Stage 0–4 MVP foundation:

- [x] Stage 0 — architecture, ADRs, repository skeleton, coding standards, CI
- [x] Stage 1 — 13 HAL interfaces, common status enum, null backend and compliance smoke test
- [x] Stage 2 — host-native AVHP memory, interrupt, clock and boot substrate
- [x] Stage 3 — reference virtual peripheral backend and compliance smoke tests
- [x] Stage 4 foundation — Cortex-M boot image, VS Code/GDB setup, SPI1 register-level path and SysTick timing
- [x] S09 sensor drivers — ICM-42688-P, BMP388 and HMC5883L with dedicated mock-bus suites
- [x] Python peripheral verification — register models, BMP388 compensation cross-check and JSON-lines socket bridge
- [x] Reproducible tooling — Docker definition and host regression script

### Verification in the delivery environment

`cmake --build build` and `ctest --test-dir build --output-on-failure` pass **7/7** tests. `pytest -q peripheral-sim/tests` passes **20/20** tests.

The current delivery environment does not provide `arm-none-eabi-gcc` or `qemu-system-arm`, so the ARM cross-build/QEMU execution is intentionally reported as pending rather than falsely marked as verified. Run `scripts/run_all_tests.sh` on the target Ubuntu/VS Code workstation or inside the supplied Docker image.

### Scope boundary

The long-term AVHP roadmap contains Stages 5–17 beyond this concrete S09 deliverable. Those stages are not silently represented as completed. See `PROJECT_COMPLETION.md` for the exact boundary and verification record.

## Recommended verification

```sh
./scripts/run_all_tests.sh

# Or reproducibly:
docker build -f docker/Dockerfile -t ansa-s09 .
docker run --rm ansa-s09
```

## Validation and demonstration dashboard

This repository includes a local, dependency-free web dashboard under `dashboard/`. The dashboard is intentionally an orchestration and visualization layer over the existing test pipeline; it does not replace the C/Python tests or manufacture PASS values.

### Option A — terminal validation

From the repository root:

```bash
# Full host regression + peripheral bridge + optional ARM/QEMU build
bash scripts/run_all_tests.sh
```

The regression portion can also be run directly:

```bash
python3 scripts/regression.py
```

For the host C/C++ layer only:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

For the independent Python peripheral layer:

```bash
python3 -m pytest -q peripheral-sim/tests
```

For the standalone sensor-driver smoke tests:

```bash
make -C tests/qemu_icm42688 run
make -C tests/qemu_bmp388 run
make -C tests/qemu_hmc5883l run
```

If the ARM toolchain and QEMU are installed, the complete script additionally builds the firmware demo:

```bash
make -C backends/qemu-cortex-m/demo
```

The QEMU boot/debug workflow and its machine limitations are documented in `backends/qemu-cortex-m/README.md`.

### Option B — dashboard validation

Start the local dashboard from the repository root:

```bash
./dashboard/run_dashboard.sh
```

Then open:

```text
http://127.0.0.1:8765
```

Press **Run Full Validation**. The browser shows the live execution log and summarizes the real results returned by `scripts/run_all_tests.sh`.

The dashboard has four validation metrics:

| Dashboard metric | Source of truth | Meaning |
|---|---|---|
| CTest | CTest output from `scripts/regression.py` | Host C/C++ test suite result |
| Pytest | `peripheral-sim/tests` pytest output | Python register-model verification |
| Bridge | `regression.py` socket identity checks | JSON-lines SPI/I2C verification transport |
| QEMU Build | `run_all_tests.sh` ARM build step | ARM firmware build, when required tools exist |

### Dashboard architecture and data flow

```text
┌───────────────────────┐
│ Browser Dashboard     │
│ HTML / CSS / JS       │
└───────────┬───────────┘
            │ HTTP / JSON polling
            ▼
┌───────────────────────┐
│ dashboard/server.py   │
│ Local Python server   │
└───────────┬───────────┘
            │ starts subprocess
            ▼
┌───────────────────────┐
│ scripts/run_all_tests │
│ Existing orchestrator │
└───────────┬───────────┘
            │
     ┌──────┴───────────────┐
     ▼                      ▼
┌──────────────┐     ┌──────────────────┐
│ regression.py│     │ ARM/QEMU demo     │
└──────┬───────┘     │ build (optional)  │
       │             └──────────────────┘
 ┌─────┼───────────┐
 ▼     ▼           ▼
CMake CTest     pytest + bridge
 │                 │
 ▼                 ▼
Host tests     Python sensor models
               ICM-42688 / BMP388 /
               HMC5883L
```

The browser never directly executes compiler or test commands. `dashboard/server.py` is the controlled local orchestration boundary, while `scripts/run_all_tests.sh` remains the canonical validation entry point. This keeps terminal CI/reproducibility and the presentation dashboard aligned.

### Dashboard scope boundary

The dashboard visualizes two different but complementary simulation/validation layers:

- **Cortex-M/QEMU:** CPU firmware build, boot and debugging path. Stock QEMU does not provide the physical flight-sensor models used by the Python layer.
- **Python peripheral simulation:** register-level ICM-42688-P, BMP388 and HMC5883L models plus the JSON-lines socket verification transport.

Therefore a dashboard card showing `ICM-42688-P → WHO_AM_I 0x47` represents the Python peripheral model/bridge verification, not a claim that a physical ICM-42688 is attached to stock QEMU SPI.

For the complete dashboard implementation details, see `dashboard/README.md`.
