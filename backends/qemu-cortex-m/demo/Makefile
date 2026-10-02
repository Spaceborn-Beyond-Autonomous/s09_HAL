# Builds the QEMU Cortex-M3 boot demo and (optionally) runs it under
# qemu-system-arm. Links the REAL Drivers/ICM42688/Src/icm42688.c —
# completely unmodified from the host-mock-tested build — against this
# folder's bare-metal SPI1 bus implementation, so the same driver source
# runs both as a host unit test (tests/qemu_icm42688) and as boot-image
# code here.
#
# Requires: arm-none-eabi-gcc / -objcopy (package gcc-arm-none-eabi) and
# qemu-system-arm to run it. Both installed for free via apt on Debian/
# Ubuntu; see the repo root README's toolchain note if `make` can't find
# them.

CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy
SIZE     := arm-none-eabi-size

CPU_FLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS    := $(CPU_FLAGS) -std=c11 -Wall -Wextra -O0 -g \
             -ffreestanding -nostdlib -fno-builtin \
             -I../../../Drivers/ICM42688/Inc
LDFLAGS   := $(CPU_FLAGS) -T ../linker/stm32f100.ld -Wl,-Map=boot_demo.map

SRCS := ../startup/startup_stm32f100.s \
        main.c \
        icm42688_bus_baremetal_qemu.c \
        ../../../Drivers/ICM42688/Src/icm42688.c

TARGET := boot_demo.elf

.PHONY: all run size clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS)

size: $(TARGET)
	$(SIZE) $(TARGET)

# -nographic + a plain serial-to-stdio mapping is enough to see the UART
# banner without a GUI; Ctrl-A X quits QEMU. CI / non-interactive runs
# should wrap this in `timeout` (see README) since the demo intentionally
# spins forever after printing.
run: $(TARGET)
	qemu-system-arm -M stm32vldiscovery -nographic -kernel $(TARGET) -serial mon:stdio

clean:
	rm -f $(TARGET) boot_demo.map
