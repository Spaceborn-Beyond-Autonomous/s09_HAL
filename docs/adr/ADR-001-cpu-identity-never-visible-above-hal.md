# ADR-001: CPU Identity Is Never Visible Above the HAL

**Status:** Accepted

## Context

Spaceborn's flight controllers target STM32F7 and STM32H7, both more advanced
than the Cortex-M4-class machines available in mainline QEMU. Early
prototyping approaches attempted to find or build an exact QEMU model of
F7/H7, which is not available upstream and would be a continuously moving
target even if partially built. This created repeated stalls in the HAL
emulator effort and put the project at risk of becoming a collection of
disconnected, chip-specific demos rather than a durable platform.

## Decision

No code above the HAL API layer (Applications, Mission Runtime, Control
Logic, Safety Runtime, Compute Manager) may reference a CPU-specific type,
register, memory address, or vendor macro, directly or indirectly. All CPU
and chip-specific behavior is implemented exclusively inside a **Backend**,
behind the fixed HAL interface defined in Stage 1. Backends include, at
minimum: Virtual (pure software), QEMU Cortex-M (generic core,
instruction/interrupt-only fidelity), STM32F7, and STM32H7.

## Consequences

- **Positive:** QEMU's lack of an exact F7/H7 machine model stops being a
  blocker; it becomes an intentionally scoped, documented limitation of one
  backend among several.
- **Positive:** New hardware targets (ESP32, Jetson, ANSA Nano, future
  silicon) are additive — new backends — rather than requiring a redesign.
- **Negative:** Requires up-front discipline and code review rigor in
  Stage 1 and beyond to prevent chip-specific leakage into shared layers; a
  single violation can quietly reintroduce coupling.

## Enforcement

- Code review checklist item: "Does this change introduce a
  vendor-specific type, macro, or register reference outside `/backends/`?"
- CI static analysis flags known vendor header includes (e.g.
  `stm32f7xx.h`, `stm32h7xx.h`) anywhere outside the `backends/` directory.
