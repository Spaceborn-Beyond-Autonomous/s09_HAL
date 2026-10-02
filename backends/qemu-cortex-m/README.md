# QEMU Cortex-M3 boot demo (S09 project-plan scope, Engineer 1 workstream)

This folder proves the sensor-driver architecture in `Drivers/` actually
boots and runs on target-representative hardware under QEMU — not just the
host-native mock-bus tests. Read this whole file before trusting any of its
output as more than what it actually is; the honest limitations below
matter as much as what works.

## The board substitution — read this first

`s09_project_plan.pdf` names "QEMU Olimex STM32-P103" as the target. That
exact machine **does not exist in mainline QEMU** (checked directly:
`qemu-system-arm -machine help` lists no `olimex-stm32-p103`). The closest
real match for the same chip family (STM32F1, Cortex-M3) is QEMU's
`stm32vldiscovery` machine, which models an STM32F100RB (128 KB flash /
8 KB RAM — smaller than the P103's STM32F103RB's 20 KB RAM, but the same
peripheral generation). Everything here targets `stm32vldiscovery`. If the
team needs the literal Olimex P103 memory map, only `linker/stm32f100.ld`'s
`LENGTH` values need adjusting — the STM32F103RB has more RAM (20K vs 8K)
at the same base addresses.

## What's actually verified here, and how

Every claim below was checked by actually running it in this environment,
not assumed from documentation:

| Peripheral | Verified behavior | What that means |
|---|---|---|
| Boot / vector table / linker | `.data` copied from flash, `.bss` zeroed, jumps to `main()` | A real, working bring-up — not a stub |
| USART1 (PA9 TX) | Polled TX fully functional; boot banner prints correctly | Fully usable for real output |
| SPI1 | `CR1` read/write persists correctly; writing `DR` sets `RXNE` in `SR` (a real transaction reaction, not a static register) | Enough to drive a real bus protocol — see caveat below |
| I2C1 | Writes to `CR1` do **not** persist (reads back 0 regardless) | Not usably modeled in this QEMU machine — **HMC5883L cannot be bus-tested this way** |
| GDB remote debugging | `qemu-system-arm -S -gdb tcp::1234` + `gdb-multiarch`: breakpoints, source-line stepping, and variable inspection all confirmed working | The VS Code debug config below is real, not aspirational |

**The SPI1 caveat, spelled out:** QEMU's SPI1 model reacts to transactions
(`TXE`/`RXNE` flags update, data register cycles), but nothing drives the
MISO line — there's no emulated ICM-42688-P (or any chip) actually sitting
on the other end. A real transaction happens; the byte that comes back is
just always `0x00`, because that's genuinely what's on the (unconnected)
wire. `Drivers/ICM42688/Src/icm42688.c` — completely unmodified from the
host-mock-tested version — correctly reads that back as
`ICM42688_ERROR_WRONG_DEVICE_ID`. **That's the correct answer, not a bug**:
it's exactly what a real board with the SPI header wired but no sensor
populated would report too.

## Layout

```
backends/qemu-cortex-m/
  linker/stm32f100.ld               memory map: 128K flash @ 0x08000000,
                                     8K RAM @ 0x20000000
  startup/startup_stm32f100.s       vector table + Reset_Handler
                                     (.data copy, .bss zero, jump to main)
  demo/
    main.c                          brings up UART, then calls the real
                                     ICM42688 driver over real SPI1
    icm42688_bus_baremetal_qemu.c   3rd bus backend for Drivers/ICM42688
                                     (register-level, no STM32Cube HAL —
                                     see its own header comment for why a
                                     3rd backend exists alongside
                                     icm42688_bus_stm32.c and the mock)
    Makefile                        build + `make run` (QEMU, no debugger)
```

## Build and run

```sh
cd backends/qemu-cortex-m/demo
make            # needs arm-none-eabi-gcc (apt: gcc-arm-none-eabi)
make run        # needs qemu-system-arm (apt: qemu-system-arm)
                # Ctrl-A, X to quit QEMU
```

Expected output (verbatim, from an actual run):
```
=== ANSA S09 HAL emulator: QEMU Cortex-M3 boot demo ===
Board model: QEMU stm32vldiscovery (closest available match
to the Olimex STM32-P103 named in s09_project_plan.pdf --
see backends/qemu-cortex-m/README.md).

SPI1 bus brought up (register-level, no HAL).
Calling the real ICM42688_Init() (Drivers/ICM42688/Src/icm42688.c,
unmodified from the host-test build)...
  -> ICM42688_Init() returned ICM42688_ERROR_WRONG_DEVICE_ID
  -> WHO_AM_I byte actually read over SPI1: 0x00 (expected 0x47 ...)

=== boot demo complete ===
```

## Debugging (VS Code or plain GDB)

**VS Code:** open the repo root, `F5` with the "QEMU: debug boot_demo.elf"
configuration (`.vscode/launch.json`). It runs the build task, launches
QEMU paused (`-S -gdb tcp::1234`, via `.vscode/tasks.json`), and attaches
`gdb-multiarch`. Requires the `gcc-arm-none-eabi` and `qemu-system-arm`
packages, `gdb-multiarch` (or substitute `arm-none-eabi-gdb` if that's
what's installed — edit `miDebuggerPath` in `launch.json`), and the C/C++
extension (`ms-vscode.cpptools`) for the `cppdbg` debug type.

**Plain GDB**, the same thing without VS Code:
```sh
qemu-system-arm -M stm32vldiscovery -nographic \
  -kernel boot_demo.elf -serial mon:stdio -S -gdb tcp::1234 &
gdb-multiarch -q -ex "file boot_demo.elf" -ex "target remote localhost:1234" \
  -ex "break main" -ex "continue"
```

## Timing and interrupt implementation

The boot image now configures the Cortex-M SysTick for a 1 ms interrupt at the STM32F100 8 MHz HSI setting. `SysTick_Handler` drives the sensor-driver delay contract, replacing the earlier uncalibrated busy-loop. The QEMU target still uses the `stm32vldiscovery` substitution rather than an Olimex P103 machine.

## What this does not prove

- **No real sensor data.** Nothing here validates register maps or
  compensation math against a *real chip's* behavior on target — that's
  what `tests/qemu_icm42688` (the host-native mock-bus suite) already does
  thoroughly. This demo proves the *wiring*, not the *sensor model*.
- **Not cycle- or timing-accurate.** `ICM42688_DelayMs()` here is an
  uncalibrated busy-loop; there's no SysTick configured.
- **BMP388 and HMC5883L aren't wired into this boot image.** BMP388 could
  be (it also uses SPI1 in this project's wiring) — not done here for
  time. HMC5883L (I2C) can't be, per the I2C1 finding above, without a
  different QEMU machine model that actually implements I2C.
- **Board substitution.** See the top of this file — this is
  `stm32vldiscovery`, not literally an Olimex STM32-P103.
