# Makefile for the HMC5883L QEMU/host driver test suite — same pattern as
# tests/qemu_icm42688/Makefile and tests/qemu_bmp388/Makefile. Provided
# alongside CMakeLists.txt (which this driver shipped with originally); both
# build the identical test binary.

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 \
           -I../../Drivers/HMC5883L/Inc -I.
LDFLAGS ?= -lm

SRCS := main.c \
        mock_hmc5883l_bus.c \
        ../../Drivers/HMC5883L/Src/hmc5883l.c

TARGET := test_hmc5883l

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
