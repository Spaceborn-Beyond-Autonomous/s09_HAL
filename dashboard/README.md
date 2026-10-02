# S09 Validation Dashboard

A dependency-free local web dashboard for the ANSA S09 HAL Emulator. It is a **presentation and orchestration layer over the repository's existing validation scripts**; it does not replace or fabricate test results.

## Start

From the repository root:

```bash
./dashboard/run_dashboard.sh
```

Open:

```text
http://127.0.0.1:8765
```

Stop with `Ctrl+C`.

Optional host/port override:

```bash
ANSA_DASHBOARD_HOST=127.0.0.1 ANSA_DASHBOARD_PORT=9000 ./dashboard/run_dashboard.sh
```

## What happens when "Run Full Validation" is pressed

The browser calls the local Python dashboard API. The dashboard starts:

```text
scripts/run_all_tests.sh
        |
        +--> scripts/regression.py
        |      |
        |      +--> CMake configure/build
        |      +--> CTest
        |      +--> pytest peripheral-sim/tests
        |      +--> Python JSON-lines peripheral socket bridge check
        |
        +--> ARM/QEMU demo build (only when arm-none-eabi-gcc and
             qemu-system-arm are available)
```

The dashboard streams the subprocess stdout into the execution log and derives its summary cards from that output.

## Dashboard sections

- **CTest** — host C/C++ test count reported by CTest.
- **Pytest** — Python peripheral verification count.
- **Bridge** — confirms the regression script completed its socket bridge identity checks.
- **QEMU Build** — confirms the ARM demo was cross-compiled when the required tools are installed.
- **Validation flow** — visualizes the six pipeline phases.
- **Sensor simulation layer** — shows the documented register identities of the three Python models.
- **Execution log** — raw pipeline output, useful during demonstrations and debugging.
- **Data flow** — explains how the UI, dashboard server, test orchestrator, CTest and Python peripheral layer relate.

## Important simulation boundary

The dashboard does **not** claim that stock QEMU contains an emulated ICM-42688-P, BMP388 or HMC5883L. The repository has two complementary validation layers:

1. **QEMU/ARM layer:** validates the Cortex-M firmware build/boot/debug path. The selected QEMU machine is documented in `backends/qemu-cortex-m/README.md`.
2. **Python peripheral layer:** validates register-level sensor behavior and the JSON-lines verification bridge independently of the QEMU board model.

This separation is intentional and matches the repository's documented scope.
