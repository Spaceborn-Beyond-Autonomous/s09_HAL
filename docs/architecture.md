# ANSA / AVHP Architecture (Stage 0 canonical reference)

This is the canonical architecture reference for all interns and new hires,
per Stage 0's deliverables. Full program context lives in
`S09_HAL_SIMULATOR.pdf` (S09-AVHP-ROADMAP-001); this file is the living,
in-repo summary that stays in sync with the code.

## Layered architecture

```
Applications
 |
Mission Runtime
 |
Control Logic
 |
Safety Runtime
 |
Compute Manager
 |
HAL API
 |
-------------------------------------------------
AVHP (ANSA Virtual Hardware Platform)
-------------------------------------------------
CPU | Memory | Interrupts | DMA | Bus
Peripherals | Sensors | Telemetry
Fault Injection | Power
-------------------------------------------------
Backends
-------------------------------------------------
Virtual (software-only, reference implementation)
QEMU Cortex-M (generic core: instruction/IRQ fidelity)
STM32F7 (real hardware)
STM32H7 (real hardware)
ESP32 | RP2040 | Linux | Raspberry Pi | Jetson | RK3588
Future: RISC-V | ANSA Nano | Custom Silicon
```

Nothing above the `AVHP` line may contain CPU-specific or chip-specific
code (ADR-001).

## Key rules (ADR summary)
- **ADR-001** — CPU identity is never visible above the HAL.
- **ADR-002** — QEMU is a backend, not a foundation.
- **ADR-003** — Every backend must pass the HAL Compliance Checklist.
- **ADR-004** — Chip-specific complexity is absorbed entirely inside that
  backend.

Full text: `docs/adr/`.

## Repository layout

See `README.md` for the directory-by-directory breakdown and which stage
introduces each piece.

## Status

This repo currently implements **Stage 0** (this document + ADRs +
skeleton) and **Stage 1** (`hal/` interfaces + null backend +
`tests/unit/test_null_backend.c`). See `docs/hal-compliance-checklist.md`
for backend-by-backend status as later stages land.
