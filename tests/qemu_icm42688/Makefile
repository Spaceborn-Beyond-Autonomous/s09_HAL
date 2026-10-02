# Makefile for the ICM-42688-P QEMU/host driver test suite.
#
# Builds icm42688.c against mock_icm42688_bus.c (NOT icm42688_bus_stm32.c —
# that file only compiles against the real STM32Cube HAL) so the driver
# logic can be verified with nothing but a host C compiler: no board, no
# QEMU binary, no cross toolchain. Matches the pattern already established
# by tests/qemu_hmc5883l/.
#
# Usage:
#   make        # build ./test_icm42688
#   make run    # build and run it (nonzero exit on any failed check)
#   make clean

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 \
           -I../../Drivers/ICM42688/Inc -I.
LDFLAGS ?= -lm

SRCS := main.c \
        mock_icm42688_bus.c \
        ../../Drivers/ICM42688/Src/icm42688.c

TARGET := test_icm42688

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
