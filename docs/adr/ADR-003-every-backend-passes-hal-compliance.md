# ADR-003: Every Backend Must Pass the HAL Compliance Checklist

**Status:** Accepted

## Context
Without a shared, enforced bar, "the Virtual backend works" and "the F7
backend works" could quietly mean different things, undermining trust that
application code tested on one backend behaves the same on another.

## Decision
Every backend — Virtual, QEMU Cortex-M, STM32F7, STM32H7, ESP32, RP2040,
Linux, Raspberry Pi, Jetson, RK3588, ANSA Nano, and any future backend —
must pass the same versioned HAL Compliance Checklist (see
`docs/hal-compliance-checklist.md`) before Mission Runtime is permitted to
target it. This checklist matures into the Stage 15 OEM certification suite.

## Consequences
- **Positive:** "HAL-compliant" is a single, testable, versioned bar, not a
  per-backend judgment call.
- **Negative:** New backends cannot be declared usable until compliance
  tooling exists for the relevant HAL surface area; this can block a
  backend's "done" status on infrastructure work, not just driver work.

## Enforcement
CI backend matrix (Stage 13 onward) runs the compliance suite against every
registered backend on every relevant commit.
