#!/usr/bin/env python3
"""peripheral_sim.py -- ANSA S09 HAL Verification Layer (Engineer 2 workstream).

Python-side register-accurate models of the three S09 flight sensors
(ICM-42688-P, BMP388, HMC5883L), independent of the C toolchain. Built to
process register-level read/write "payloads" the way a real SPI/I2C
transaction would carry them -- see PeripheralSim.spi_transfer() and
.i2c_transaction() below -- for use as:

  - a fast, dependency-free reference to check a driver's or a bus
    analyzer capture's register-level behavior against, without building
    or running any C code;
  - a scriptable/CLI tool for engineers poking at register maps by hand;
  - a starting point for a future serial/named-pipe bridge into the
    QEMU-hosted firmware in backends/qemu-cortex-m/ (NOT built here --
    see "Scope" in the README in this folder for exactly what that would
    still need).

Design note: every register address, reset value, and identity byte here
is deliberately kept identical to the corresponding C mock
(tests/qemu_<sensor>/mock_<sensor>_bus.c) -- including the same synthetic
BMP388 calibration bytes -- so a fact checked against one is a fact
checked against the other. Where the two diverge, that is a bug in one of
them, not an acceptable "two valid models" situation.
"""

from __future__ import annotations

import argparse
import math
import random
import sys
from dataclasses import dataclass, field
from typing import Optional


# =============================================================================
# ICM-42688-P (6-axis IMU, SPI)
# =============================================================================

class Icm42688Model:
    """Register-level ICM-42688-P model. Mirrors
    tests/qemu_icm42688/mock_icm42688_bus.c register-for-register."""

    WHO_AM_I = 0x75
    WHO_AM_I_VALUE = 0x47
    DEVICE_CONFIG = 0x11
    DEVICE_CONFIG_SOFT_RESET = 0x01
    TEMP_DATA1 = 0x1D
    FIFO_CONFIG = 0x16
    FIFO_COUNTH = 0x2E
    FIFO_COUNTL = 0x2F
    FIFO_DATA = 0x30
    PWR_MGMT0 = 0x4E
    GYRO_CONFIG0 = 0x4F
    ACCEL_CONFIG0 = 0x50
    FIFO_PACKET_BYTES = 12
    FIFO_MODE_MASK = 0xC0
    FIFO_MODE_BYPASS = 0x00
    FIFO_MODE_STREAM = 0x40

    def __init__(self, seed: int = 0x1234ABCD) -> None:
        self._rng = random.Random(seed)
        self.reset()

    def reset(self) -> None:
        self.device_config = 0
        self.pwr_mgmt0 = 0
        self.accel_config0 = 0
        self.gyro_config0 = 0
        self.fifo_config = 0
        self.fifo_count_bytes = 0

    def _gauss(self, sigma: float) -> float:
        return self._rng.gauss(0.0, sigma)

    def _primary_block(self) -> bytes:
        """14 bytes: temp, accel x/y/z, gyro x/y/z -- matches
        TEMP_DATA1..GYRO_DATA_Z0 being contiguous on real silicon."""
        lsb_per_g = 2048.0     # +-16 g default range
        lsb_per_dps = 16.384   # +-2000 dps default range

        temp = int(self._gauss(0.05) * 132.48)
        ax = int(self._gauss(0.01) * lsb_per_g)
        ay = int(self._gauss(0.01) * lsb_per_g)
        az = int((1.0 + self._gauss(0.01)) * lsb_per_g)   # resting: +1 g on Z
        gx = int(self._gauss(0.5) * lsb_per_dps)
        gy = int(self._gauss(0.5) * lsb_per_dps)
        gz = int(self._gauss(0.5) * lsb_per_dps)

        out = bytearray()
        for v in (temp, ax, ay, az, gx, gy, gz):
            v &= 0xFFFF
            out += bytes([(v >> 8) & 0xFF, v & 0xFF])
        return bytes(out)

    def _fifo_packet(self) -> bytes:
        """12 bytes: accel x/y/z, gyro x/y/z -- simplified packet, see the
        note in Drivers/ICM42688/Inc/icm42688_registers.h."""
        block = self._primary_block()
        return block[2:]  # drop the leading 2-byte temp field

    def read(self, reg: int, length: int) -> bytes:
        if reg == self.WHO_AM_I and length == 1:
            return bytes([self.WHO_AM_I_VALUE])
        if reg == self.TEMP_DATA1 and length == 14:
            return self._primary_block()
        if reg == self.FIFO_COUNTH and length == 2:
            n = self.fifo_count_bytes
            return bytes([(n >> 8) & 0xFF, n & 0xFF])
        if reg == self.FIFO_DATA and length == self.FIFO_PACKET_BYTES:
            if self.fifo_count_bytes < self.FIFO_PACKET_BYTES:
                raise ValueError("FIFO underflow: check FIFO_COUNT before draining")
            self.fifo_count_bytes -= self.FIFO_PACKET_BYTES
            return self._fifo_packet()
        raise ValueError(f"unmodeled ICM42688 read: reg=0x{reg:02X} length={length}")

    def write(self, reg: int, data: int) -> None:
        if reg == self.DEVICE_CONFIG:
            self.device_config = data
            if data & self.DEVICE_CONFIG_SOFT_RESET:
                who = self.WHO_AM_I_VALUE
                self.reset()
                self.WHO_AM_I_VALUE = who
            return
        if reg == self.PWR_MGMT0:
            self.pwr_mgmt0 = data
            return
        if reg == self.ACCEL_CONFIG0:
            self.accel_config0 = data
            return
        if reg == self.GYRO_CONFIG0:
            self.gyro_config0 = data
            return
        if reg == self.FIFO_CONFIG:
            self.fifo_config = data
            if (data & self.FIFO_MODE_MASK) == self.FIFO_MODE_BYPASS:
                self.fifo_count_bytes = 0
            return
        raise ValueError(f"unmodeled ICM42688 write: reg=0x{reg:02X}")

    def tick(self) -> None:
        """Simulated time passing: one more FIFO packet becomes available,
        if streaming is enabled. Mirrors mock_icm42688_bus_tick()."""
        if (self.fifo_config & self.FIFO_MODE_MASK) == self.FIFO_MODE_STREAM:
            self.fifo_count_bytes += self.FIFO_PACKET_BYTES


# =============================================================================
# BMP388 (barometer, SPI/I2C)
# =============================================================================

class Bmp388Model:
    """Register-level BMP388 model. Mirrors
    tests/qemu_bmp388/mock_bmp388_bus.c register-for-register, including
    the same synthetic-but-self-consistent 21-byte calibration block."""

    CHIP_ID = 0x00
    CHIP_ID_VALUE = 0x50
    STATUS = 0x03
    STATUS_CMD_RDY = 0x10
    STATUS_DRDY_PRESS = 0x20
    STATUS_DRDY_TEMP = 0x40
    DATA_0 = 0x04
    PWR_CTRL = 0x1B
    PWR_PRESS_EN = 0x01
    PWR_TEMP_EN = 0x02
    OSR = 0x1C
    ODR = 0x1D
    CALIB_DATA = 0x31
    CALIB_DATA_LEN = 21
    CMD = 0x7E
    CMD_SOFT_RESET = 0xB6

    # Identical bytes to kCalibrationBytes in mock_bmp388_bus.c -- see that
    # file's header comment for how these were derived (inverting the real
    # Bosch compensation formula's own scale factors against target trim
    # coefficients, then verifying they decode to a plausible ~25 C /
    # ~101.3 kPa reading).
    CALIBRATION_BYTES = bytes([
        0x71, 0x72, 0xAA, 0x64, 0x00, 0x6D, 0x27, 0x16, 0x01, 0x00,
        0x00, 0x0A, 0x32, 0x00, 0x02, 0xFF, 0x00, 0x7E, 0x10, 0x08, 0xFE,
    ])
    BASELINE_TEMP_RAW = 8_540_000
    BASELINE_PRESS_RAW = 198_000

    def __init__(self, seed: int = 0x9E3779B9) -> None:
        self._rng = random.Random(seed)
        self.reset()

    def reset(self) -> None:
        self.pwr_ctrl = 0
        self.osr = 0
        self.odr = 0
        self.press_ready = False
        self.temp_ready = False

    def read(self, reg: int, length: int) -> bytes:
        if reg == self.CHIP_ID and length == 1:
            return bytes([self.CHIP_ID_VALUE])
        if reg == self.STATUS and length == 1:
            v = self.STATUS_CMD_RDY
            if self.press_ready:
                v |= self.STATUS_DRDY_PRESS
            if self.temp_ready:
                v |= self.STATUS_DRDY_TEMP
            return bytes([v])
        if reg == self.DATA_0 and length == 6:
            p = int(self.BASELINE_PRESS_RAW + self._rng.gauss(0.0, 40.0)) & 0xFFFFFF
            t = int(self.BASELINE_TEMP_RAW + self._rng.gauss(0.0, 400.0)) & 0xFFFFFF
            self.press_ready = False
            self.temp_ready = False
            return bytes([
                p & 0xFF, (p >> 8) & 0xFF, (p >> 16) & 0xFF,
                t & 0xFF, (t >> 8) & 0xFF, (t >> 16) & 0xFF,
            ])
        if reg == self.CALIB_DATA and length == self.CALIB_DATA_LEN:
            return self.CALIBRATION_BYTES
        raise ValueError(f"unmodeled BMP388 read: reg=0x{reg:02X} length={length}")

    def write(self, reg: int, data: int) -> None:
        if reg == self.CMD:
            if data == self.CMD_SOFT_RESET:
                self.reset()
            return
        if reg == self.OSR:
            self.osr = data
            return
        if reg == self.ODR:
            self.odr = data
            return
        if reg == self.PWR_CTRL:
            self.pwr_ctrl = data
            if (data & (self.PWR_PRESS_EN | self.PWR_TEMP_EN)) == (
                self.PWR_PRESS_EN | self.PWR_TEMP_EN
            ):
                self.press_ready = True
                self.temp_ready = True
            return
        raise ValueError(f"unmodeled BMP388 write: reg=0x{reg:02X}")

    def tick(self) -> None:
        """One more conversion cycle completes. Mirrors mock_bmp388_bus_tick()."""
        self.press_ready = True
        self.temp_ready = True

    @staticmethod
    def compensate(raw6: bytes) -> tuple[float, float]:
        """Runs the real Bosch floating-point compensation formula against
        a raw 6-byte DATA_0..DATA_5 block and CALIBRATION_BYTES, returning
        (pressure_pa, temperature_c). Independent re-implementation of the
        same formula bmp388.c uses -- kept here so this module is a
        standalone check, not something that has to link the C driver to
        be useful."""
        raw = Bmp388Model.CALIBRATION_BYTES

        def u16(off: int) -> int:
            return raw[off] | (raw[off + 1] << 8)

        def s16(off: int) -> int:
            v = u16(off)
            return v - 65536 if v >= 32768 else v

        def s8(off: int) -> int:
            v = raw[off]
            return v - 256 if v >= 128 else v

        par_t1 = u16(0) * 256.0
        par_t2 = u16(2) / 1073741824.0
        par_t3 = s8(4) / 281474976710656.0
        par_p1 = (s16(5) - 16384.0) / 1048576.0
        par_p2 = (s16(7) - 16384.0) / 536870912.0
        par_p3 = s8(9) / 4294967296.0
        par_p4 = s8(10) / 137438953472.0
        par_p5 = u16(11) * 8.0
        par_p6 = u16(13) / 64.0
        par_p7 = s8(15) / 256.0
        par_p8 = s8(16) / 32768.0
        par_p9 = s16(17) / 281474976710656.0
        par_p10 = s8(19) / 281474976710656.0
        par_p11 = s8(20) / 36893488147419103232.0

        press_raw = raw6[0] | (raw6[1] << 8) | (raw6[2] << 16)
        temp_raw = raw6[3] | (raw6[4] << 8) | (raw6[5] << 16)

        p1 = temp_raw - par_t1
        p2 = p1 * par_t2
        t_lin = p2 + (p1 * p1) * par_t3

        po1 = par_p5 + par_p6 * t_lin + par_p7 * t_lin**2 + par_p8 * t_lin**3
        po2 = press_raw * (par_p1 + par_p2 * t_lin + par_p3 * t_lin**2 + par_p4 * t_lin**3)
        pd = (press_raw**2) * (par_p9 + par_p10 * t_lin) + (press_raw**3) * par_p11
        pressure_pa = po1 + po2 + pd

        return pressure_pa, t_lin


# =============================================================================
# HMC5883L (magnetometer, I2C) -- retained design
# =============================================================================

class Hmc5883lModel:
    """Register-level HMC5883L model (I2C address 0x1E). Datasheet
    identification bytes spell 'H43' in ASCII across three registers;
    data registers are read out in X, Z, Y order (a well-known quirk of
    this specific chip, not a typo)."""

    I2C_ADDRESS = 0x1E
    REG_CRA = 0x00
    REG_CRB = 0x01
    REG_MODE = 0x02
    REG_DATA_X_MSB = 0x03  # then X_LSB, Z_MSB, Z_LSB, Y_MSB, Y_LSB
    REG_STATUS = 0x09
    REG_IDA = 0x0A
    REG_IDB = 0x0B
    REG_IDC = 0x0C
    IDA_VALUE, IDB_VALUE, IDC_VALUE = ord("H"), ord("4"), ord("3")

    def __init__(self, seed: int = 42) -> None:
        self._rng = random.Random(seed)
        self._angle = 0.0
        self.cra = 0x10
        self.crb = 0x20
        self.mode = 0x01

    def read(self, reg: int, length: int) -> bytes:
        if reg == self.REG_IDA and length == 3:
            return bytes([self.IDA_VALUE, self.IDB_VALUE, self.IDC_VALUE])
        if reg == self.REG_DATA_X_MSB and length == 6:
            self._angle += 0.087  # ~5 degrees per sample, matches the C mock
            x = 0.25 + 0.10 * math.sin(self._angle) + self._rng.gauss(0, 0.01)
            y = 0.15 + 0.10 * math.cos(self._angle) + self._rng.gauss(0, 0.01)
            z = -0.40 + self._rng.gauss(0, 0.01)
            scale = 390.0  # LSB/Gauss @ +-4.7 Ga range
            out = bytearray()
            for v in (x, z, y):  # datasheet order: X, Z, Y
                counts = max(-32768, min(32767, int(v * scale)))
                counts &= 0xFFFF
                out += bytes([(counts >> 8) & 0xFF, counts & 0xFF])
            return bytes(out)
        raise ValueError(f"unmodeled HMC5883L read: reg=0x{reg:02X} length={length}")

    def write(self, reg: int, data: int) -> None:
        if reg == self.REG_CRA:
            self.cra = data
        elif reg == self.REG_CRB:
            self.crb = data
        elif reg == self.REG_MODE:
            self.mode = data
        else:
            raise ValueError(f"unmodeled HMC5883L write: reg=0x{reg:02X}")


# =============================================================================
# Bus router -- "process serial, SPI, and I2C register payloads"
# =============================================================================

@dataclass
class PeripheralSim:
    """Routes register transactions to whichever attached model owns the
    given chip-select line (SPI) or 7-bit address (I2C). This is the piece
    a serial/pipe bridge into real firmware would sit behind -- see the
    README's "Scope" section for what that bridge itself would still need
    (framing, timing, a transport)."""

    icm42688: Icm42688Model = field(default_factory=Icm42688Model)
    bmp388: Bmp388Model = field(default_factory=Bmp388Model)
    hmc5883l: Hmc5883lModel = field(default_factory=Hmc5883lModel)

    def spi_transfer(self, cs_line: str, reg: int, write_byte: Optional[int], length: int) -> bytes:
        """cs_line selects which SPI-attached device this transaction is
        for ('imu' or 'baro' in this project's wiring). reg's bit 7 is the
        real ICM-42688-P/BMP388 SPI read convention (1 = read)."""
        device = {"imu": self.icm42688, "baro": self.bmp388}.get(cs_line)
        if device is None:
            raise ValueError(f"unknown SPI chip-select: {cs_line!r}")
        is_read = bool(reg & 0x80)
        real_reg = reg & 0x7F
        if is_read:
            return device.read(real_reg, length)
        device.write(real_reg, write_byte if write_byte is not None else 0)
        return b""

    def i2c_transaction(self, address: int, reg: int, write_byte: Optional[int], length: int) -> bytes:
        if address != self.hmc5883l.I2C_ADDRESS:
            raise ValueError(f"no device modeled at I2C address 0x{address:02X}")
        if write_byte is not None:
            self.hmc5883l.write(reg, write_byte)
            return b""
        return self.hmc5883l.read(reg, length)

    def tick(self) -> None:
        """Advances simulated time for every device that models one
        (FIFO fill, new conversion available, ...)."""
        self.icm42688.tick()
        self.bmp388.tick()


# =============================================================================
# CLI: quick manual register poking without writing a script
# =============================================================================

def _main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("sensor", nargs="?", choices=["icm42688", "bmp388", "hmc5883l"])
    parser.add_argument("--read", metavar="REG_HEX", help="register to read, e.g. 0x75")
    parser.add_argument("--length", type=int, default=1)
    parser.add_argument("--tick", action="store_true", help="advance simulated time first")
    parser.add_argument("--serve", action="store_true", help="start the JSON-lines TCP bridge")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args(argv)

    sim = PeripheralSim()
    if args.tick:
        sim.tick()

    if args.serve:
        serve_socket(args.host, args.port)
        return 0
    if args.read is None:
        parser.error("nothing to do -- pass --read REG_HEX")

    if args.sensor is None:
        parser.error("sensor is required unless --serve is used")
    reg = int(args.read, 16)
    model = {"icm42688": sim.icm42688, "bmp388": sim.bmp388, "hmc5883l": sim.hmc5883l}[args.sensor]
    data = model.read(reg, args.length)
    print(" ".join(f"0x{b:02X}" for b in data))
    return 0


# =============================================================================
# Local socket bridge -- JSON-lines, deterministic and dependency-free
# =============================================================================

def serve_socket(host: str = "127.0.0.1", port: int = 8765) -> None:
    """Serve register transactions over a newline-delimited JSON TCP socket.

    Request examples:
      {"op":"spi","cs":"imu","reg":245,"len":1}
      {"op":"spi","cs":"baro","reg":128,"len":1}
      {"op":"i2c","addr":30,"reg":10,"len":3}
      {"op":"tick"}

    Responses are {"ok":true,"data":[...]} or {"ok":false,"error":"..."}.
    The protocol is intentionally simple so a bare-metal UART/host adapter or
    a QEMU monitor-side bridge can implement it without a Python dependency.
    """
    import json
    import socket
    sim = PeripheralSim()
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((host, port))
        srv.listen(4)
        print(f"peripheral_sim socket bridge listening on {host}:{port}", flush=True)
        while True:
            conn, _ = srv.accept()
            with conn:
                buf = b""
                while True:
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    buf += chunk
                    while b"\n" in buf:
                        line, buf = buf.split(b"\n", 1)
                        if not line.strip():
                            continue
                        try:
                            req = json.loads(line.decode("utf-8"))
                            op = req.get("op")
                            if op == "tick":
                                sim.tick(); resp = {"ok": True, "data": []}
                            elif op == "spi":
                                data = sim.spi_transfer(str(req["cs"]), int(req["reg"]), req.get("write"), int(req.get("len", 1)))
                                resp = {"ok": True, "data": list(data)}
                            elif op == "i2c":
                                data = sim.i2c_transaction(int(req["addr"]), int(req["reg"]), req.get("write"), int(req.get("len", 1)))
                                resp = {"ok": True, "data": list(data)}
                            else:
                                raise ValueError("op must be spi, i2c, or tick")
                        except Exception as exc:  # bridge must never crash on malformed client data
                            resp = {"ok": False, "error": str(exc)}
                        conn.sendall((json.dumps(resp) + "\n").encode("utf-8"))


if __name__ == "__main__":
    raise SystemExit(_main(sys.argv[1:]))
