#!/usr/bin/env bash
set -euo pipefail
python3 scripts/regression.py
if command -v arm-none-eabi-gcc >/dev/null 2>&1 && command -v qemu-system-arm >/dev/null 2>&1; then
  make -C backends/qemu-cortex-m/demo
  echo 'Cross-compiled QEMU demo successfully.'
else
  echo 'NOTE: arm-none-eabi-gcc/qemu-system-arm not installed; QEMU target test skipped.'
fi
