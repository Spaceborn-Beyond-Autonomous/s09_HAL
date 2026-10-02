# HAL Compliance Checklist v0.1

Every backend (Virtual, QEMU Cortex-M, STM32F7, STM32H7, ESP32, RP2040,
Linux, Raspberry Pi, Jetson, RK3588, ANSA Nano, and beyond) must satisfy
this checklist before Mission Runtime is permitted to target it (ADR-003).
This checklist is the seed of the Stage 15 OEM certification suite — write
entries here with that eventual audience in mind.

## 1. Build & link
- [ ] Backend implements every function declared in every `hal/*.h`
      header (no missing symbols at link time).
- [ ] Backend compiles with the project's standard warning flags
      (`-Wall -Wextra -Werror -Wpedantic`) with zero warnings.
- [ ] Backend introduces no vendor-specific symbol, macro, or include
      visible outside its own `backends/<name>/` directory (ADR-001).

## 2. Behavioral (see also `tests/hal_compliance/`, Section 18 of the
   roadmap for illustrative test-case granularity — HAL-GPIO-001,
   HAL-UART-001, etc.)
- [ ] `hal_gpio_*`: write/read/toggle/mode/interrupt behave per spec.
- [ ] `hal_uart_*`: sync + async write, read, loopback integrity.
- [ ] `hal_spi_*`: full-duplex transfer correctness, CS control.
- [ ] `hal_i2c_*`: write/read/mem_write/mem_read correctness, NACK
      handling.
- [ ] `hal_timer_*`: start/stop/period/callback accuracy within the
      backend's documented tolerance.
- [ ] `hal_pwm_*`: duty cycle/frequency accuracy within tolerance.
- [ ] `hal_can_*`: send/receive/filter correctness.
- [ ] `hal_dma_*`: configure/start/stop/status correctness (where the
      backend supports DMA at all — document if not).
- [ ] `hal_adc_*`: read accuracy within tolerance, continuous-mode
      callback correctness.
- [ ] `hal_flash_*`: erase/write/read integrity, erased-value semantics.
- [ ] `hal_clock_*`: tick_ms/tick_us monotonicity and drift within
      tolerance.
- [ ] `hal_power_*`: reasonable/documented behavior for battery/current
      queries and low-power mode transitions.
- [ ] `hal_sensor_*`: init/read/subscribe/health-status correctness.

## 3. Documentation
- [ ] Backend has a `README.md` stating what it validates and what it
      explicitly does *not* (see the QEMU Cortex-M backend's limitations
      doc, Stage 4, as the reference example).
- [ ] Any known deviation from full behavioral compliance is documented,
      not silently accepted.

## 4. Fault injection (Stage 8 onward)
- [ ] Backend supports, or explicitly documents that it cannot support,
      each fault type in the fault injection scenario catalogue.

## Status by backend
| Backend | Stage introduced | Status |
|---|---|---|
| Null (stub) | 1 | N/A — not a real backend, link-only smoke test |
| Virtual | 3 | Not started |
| QEMU Cortex-M | 4 | Not started |
| STM32F7 | 10 | Not started |
| STM32H7 | 11 | Not started |
| ESP32 / RP2040 / Linux / RPi / Jetson / RK3588 | 13 | Not started |
| ANSA Nano | 16 | Not started |
