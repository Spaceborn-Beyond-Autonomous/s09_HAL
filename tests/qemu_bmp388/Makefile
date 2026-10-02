# Makefile for the BMP388 QEMU/host driver test suite.
# Same pattern as tests/qemu_icm42688/Makefile — see that file's header
# comment for the rationale (host-native build, no board/QEMU binary needed
# to validate driver logic).

CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 \
           -I../../Drivers/BMP388/Inc -I.
LDFLAGS ?= -lm

SRCS := main.c \
        mock_bmp388_bus.c \
        ../../Drivers/BMP388/Src/bmp388.c

TARGET := test_bmp388

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
