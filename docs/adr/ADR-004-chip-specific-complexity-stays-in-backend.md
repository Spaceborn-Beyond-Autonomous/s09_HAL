# ADR-004: Chip-Specific Complexity Is Absorbed Entirely Inside the Backend

**Status:** Accepted

## Context
Advanced chips (e.g. STM32H7's multi-domain clock tree and I-cache/D-cache
behavior) have real complexity that cannot simply be wished away. The
question is *where* that complexity lives.

## Decision
Chip-specific complexity — cache maintenance, clock-tree configuration,
DMA/peripheral errata workarounds, dual-core coordination, and similar
concerns — is implemented entirely inside that backend's own implementation
directory (e.g. `backends/stm32h7/`) and is never leaked upward into
`avhp/`, `hal/`, `mission-runtime/`, or any other shared layer.

## Consequences
- **Positive:** Adding a new, more complex chip never requires touching
  code shared by other backends.
- **Negative:** Some logic (e.g. cache-maintenance-around-DMA patterns) may
  be duplicated across backends that each need it, rather than shared.

## Enforcement
Code review checklist item, same mechanism as ADR-001: any change to a
shared layer (`hal/`, `avhp/`, `mission-runtime/`, etc.) that encodes a
single chip's quirks is rejected and redirected into `backends/<name>/`.
