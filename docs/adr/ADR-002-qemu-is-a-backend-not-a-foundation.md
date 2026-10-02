# ADR-002: QEMU Is a Backend, Not a Foundation

**Status:** Accepted

## Context

Mainline QEMU's built-in STM32 support is limited to older Cortex-M3/M4-class
boards (e.g. `netduino2`, `netduinoplus2`, `stm32vldiscovery`-class
machines). There is no maintained, cycle-accurate STM32F7 or STM32H7 machine
model in mainline QEMU. If AVHP's build, CI, and testing infrastructure were
built to *require* QEMU, every future non-ARM backend (Linux, Jetson,
RK3588, eventual RISC-V) would inherit an unnecessary and misleading
dependency, and CI would be slower than necessary for day-to-day development
that doesn't touch CPU-instruction-level concerns at all.

## Decision

AVHP must run fully host-native — zero QEMU dependency — for the Virtual
backend (Stage 2/3) and for any backend that is not itself an ARM Cortex-M
target. QEMU (Stage 4) is registered as **one interchangeable backend**
among several, used specifically to validate ARM Cortex-M
instruction-set/interrupt-controller correctness. It is never a prerequisite
for building, testing, or running AVHP in general.

## Consequences

- **Positive:** Fast host-native CI (Stage 2 onward) does not wait on an
  emulator.
- **Positive:** Non-ARM backends (Linux, Jetson, RK3588, future RISC-V) are
  first-class citizens, not QEMU workarounds.
- **Negative:** Two parallel validation paths (host-native and QEMU) must be
  kept in sync and diffed (see Stage 6's timing-diff CI check) rather than
  relying on a single canonical emulated environment.

## Enforcement

- CI must contain a host-native build/test job that has no QEMU invocation
  anywhere in its dependency graph.
- The QEMU CI job (Stage 4 onward) is additive, not a gate that blocks
  earlier, non-CPU-specific work.
