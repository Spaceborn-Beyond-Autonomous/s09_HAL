# ANSA Coding Standards v0.1

Applies to `hal/`, `avhp/`, `mission-runtime/`, and any other shared layer.
Backend directories (`backends/<name>/`) may relax rules where a specific
vendor SDK forces it, but must document any deviation in that backend's
README.

## Language & style
- **Language:** C11. MISRA-C:2012 subset (mandatory + required rules) for
  `hal/` and `mission-runtime/safety_runtime.c` specifically; recommended
  elsewhere.
- **Naming:** `hal_<peripheral>_<verb>` for HAL functions (e.g.
  `hal_gpio_write`), `Virtual<Peripheral>` for Stage 3 virtual peripheral
  classes, `avhp_<subsystem>_<verb>` for AVHP Core functions.
- **Error codes:** every fallible function returns `hal_status_t`
  (`hal/hal_status.h`). No silent failure, no `errno`-style side channels.
- **No vendor leakage above the HAL:** see ADR-001. Enforced by CI.
- **Opaque handles:** HAL headers expose only opaque `typedef struct
  hal_foo hal_foo_t;` types above the HAL. Concrete struct layout is
  private to the backend that defines it.

## Build & warnings
- Build with `-Wall -Wextra -Werror -Wpedantic` (or the MSVC/other-compiler
  equivalent). Warnings are build failures, not backlog items.
- Every new `hal/*.h` header must compile standalone (`gcc -c
  -Ihal hal/foo.h`-style check) with no other project headers included
  first.

## Testing
- Unit test framework: plain C asserts / minimal custom runner for
  `tests/unit/`, matching the pattern in `tests/unit/test_null_backend.c`.
  (Unity/GoogleTest may be adopted later without changing this rule.)
- Every HAL header lands with a corresponding stub call-through in
  `tests/unit/test_null_backend.c` at the same time it is added (see
  Stage 1 exit criteria).
- Every backend must pass `docs/hal-compliance-checklist.md` before Mission
  Runtime is permitted to target it (ADR-003).

## Documentation
- Every backend directory has its own `README.md` describing what it does
  and does not validate (see `docs/backend-limitations/` for the
  QEMU/Cortex-M example established in Stage 4).
- Any architectural decision gets an ADR in `docs/adr/`, not just a code
  comment.
