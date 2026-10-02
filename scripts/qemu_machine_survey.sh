#!/usr/bin/env bash
set -euo pipefail
if ! command -v qemu-system-arm >/dev/null 2>&1; then echo 'qemu-system-arm not installed'; exit 2; fi
qemu-system-arm -machine help | tee qemu-machines.txt
if grep -q '^stm32vldiscovery' qemu-machines.txt; then echo 'Selected machine: stm32vldiscovery (Cortex-M3 STM32F100-class)'; fi
if grep -qi 'olimex-stm32-p103' qemu-machines.txt; then echo 'Olimex STM32-P103 is available'; else echo 'Olimex STM32-P103 is not listed by this QEMU build'; fi
