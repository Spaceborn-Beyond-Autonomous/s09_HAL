#!/usr/bin/env python3
"""Tests for peripheral_sim.py. Run with: python3 -m unittest discover -s tests -v
(or just: python3 tests/test_peripheral_sim.py)"""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from peripheral_sim import Bmp388Model, Hmc5883lModel, Icm42688Model, PeripheralSim


class TestIcm42688Model(unittest.TestCase):
    def test_who_am_i(self):
        m = Icm42688Model()
        self.assertEqual(m.read(Icm42688Model.WHO_AM_I, 1), bytes([0x47]))

    def test_soft_reset_preserves_identity(self):
        m = Icm42688Model()
        m.write(Icm42688Model.PWR_MGMT0, 0x0F)
        m.write(Icm42688Model.DEVICE_CONFIG, Icm42688Model.DEVICE_CONFIG_SOFT_RESET)
        self.assertEqual(m.pwr_mgmt0, 0, "reset should clear pwr_mgmt0")
        self.assertEqual(m.read(Icm42688Model.WHO_AM_I, 1), bytes([0x47]),
                          "reset should not change the chip's identity")

    def test_primary_block_changes_across_reads(self):
        m = Icm42688Model(seed=1)
        a = m.read(Icm42688Model.TEMP_DATA1, 14)
        b = m.read(Icm42688Model.TEMP_DATA1, 14)
        self.assertNotEqual(a, b)

    def test_accel_z_near_one_g_at_rest(self):
        m = Icm42688Model(seed=2)
        raw = m.read(Icm42688Model.TEMP_DATA1, 14)
        az = int.from_bytes(raw[6:8], "big", signed=True)
        self.assertTrue(1800 < az < 2300, f"az={az} not plausibly near +1g (2048 LSB)")

    def test_fifo_fill_and_drain(self):
        m = Icm42688Model(seed=3)
        m.write(Icm42688Model.FIFO_CONFIG, Icm42688Model.FIFO_MODE_STREAM)
        for _ in range(5):
            m.tick()
        count_before = int.from_bytes(m.read(Icm42688Model.FIFO_COUNTH, 2), "big")
        self.assertGreater(count_before, 0)
        packet = m.read(Icm42688Model.FIFO_DATA, Icm42688Model.FIFO_PACKET_BYTES)
        self.assertEqual(len(packet), 12)
        count_after = int.from_bytes(m.read(Icm42688Model.FIFO_COUNTH, 2), "big")
        self.assertLess(count_after, count_before)

    def test_fifo_underflow_raises(self):
        m = Icm42688Model()
        with self.assertRaises(ValueError):
            m.read(Icm42688Model.FIFO_DATA, Icm42688Model.FIFO_PACKET_BYTES)


class TestBmp388Model(unittest.TestCase):
    def test_chip_id(self):
        m = Bmp388Model()
        self.assertEqual(m.read(Bmp388Model.CHIP_ID, 1), bytes([0x50]))

    def test_calibration_block_length(self):
        m = Bmp388Model()
        self.assertEqual(len(m.read(Bmp388Model.CALIB_DATA, 21)), 21)

    def test_data_ready_handshake(self):
        m = Bmp388Model()
        status = m.read(Bmp388Model.STATUS, 1)[0]
        self.assertEqual(status & Bmp388Model.STATUS_DRDY_PRESS, 0, "not ready before power-on")

        m.write(Bmp388Model.PWR_CTRL, Bmp388Model.PWR_PRESS_EN | Bmp388Model.PWR_TEMP_EN)
        status = m.read(Bmp388Model.STATUS, 1)[0]
        self.assertTrue(status & Bmp388Model.STATUS_DRDY_PRESS)
        self.assertTrue(status & Bmp388Model.STATUS_DRDY_TEMP)

        m.read(Bmp388Model.DATA_0, 6)  # consumes the ready flags
        status = m.read(Bmp388Model.STATUS, 1)[0]
        self.assertEqual(status & Bmp388Model.STATUS_DRDY_PRESS, 0)

        m.tick()
        status = m.read(Bmp388Model.STATUS, 1)[0]
        self.assertTrue(status & Bmp388Model.STATUS_DRDY_PRESS)

    def test_compensation_is_physically_plausible(self):
        """Cross-checks the independent Python compensation formula (which
        mirrors bmp388.c's) against the same calibration bytes the C mock
        uses -- this is the number that matters, not the raw counts."""
        m = Bmp388Model()
        m.write(Bmp388Model.PWR_CTRL, Bmp388Model.PWR_PRESS_EN | Bmp388Model.PWR_TEMP_EN)
        raw6 = m.read(Bmp388Model.DATA_0, 6)
        pressure_pa, temperature_c = Bmp388Model.compensate(raw6)
        self.assertTrue(95000 < pressure_pa < 108000, f"pressure {pressure_pa} implausible")
        self.assertTrue(15 < temperature_c < 35, f"temperature {temperature_c} implausible")

    def test_soft_reset(self):
        m = Bmp388Model()
        m.write(Bmp388Model.OSR, 0x2B)
        m.write(Bmp388Model.CMD, Bmp388Model.CMD_SOFT_RESET)
        self.assertEqual(m.osr, 0)


class TestHmc5883lModel(unittest.TestCase):
    def test_identity_spells_h43(self):
        m = Hmc5883lModel()
        ida, idb, idc = m.read(Hmc5883lModel.REG_IDA, 3)
        self.assertEqual((ida, idb, idc), (ord("H"), ord("4"), ord("3")))

    def test_data_changes_across_reads(self):
        m = Hmc5883lModel()
        a = m.read(Hmc5883lModel.REG_DATA_X_MSB, 6)
        b = m.read(Hmc5883lModel.REG_DATA_X_MSB, 6)
        self.assertNotEqual(a, b)


class TestPeripheralSimRouter(unittest.TestCase):
    def test_spi_routes_by_chip_select(self):
        sim = PeripheralSim()
        imu_id = sim.spi_transfer("imu", Icm42688Model.WHO_AM_I | 0x80, None, 1)
        baro_id = sim.spi_transfer("baro", Bmp388Model.CHIP_ID | 0x80, None, 1)
        self.assertEqual(imu_id, bytes([0x47]))
        self.assertEqual(baro_id, bytes([0x50]))

    def test_spi_unknown_chip_select_raises(self):
        sim = PeripheralSim()
        with self.assertRaises(ValueError):
            sim.spi_transfer("nope", 0x00, None, 1)

    def test_i2c_routes_by_address(self):
        sim = PeripheralSim()
        ida = sim.i2c_transaction(Hmc5883lModel.I2C_ADDRESS, Hmc5883lModel.REG_IDA, None, 3)
        self.assertEqual(ida, bytes([ord("H"), ord("4"), ord("3")]))

    def test_i2c_unknown_address_raises(self):
        sim = PeripheralSim()
        with self.assertRaises(ValueError):
            sim.i2c_transaction(0x50, 0x00, None, 1)

    def test_tick_advances_both_ticking_devices(self):
        sim = PeripheralSim()
        sim.spi_transfer("imu", Icm42688Model.FIFO_CONFIG, Icm42688Model.FIFO_MODE_STREAM, 0)
        sim.spi_transfer("baro", Bmp388Model.PWR_CTRL,
                          Bmp388Model.PWR_PRESS_EN | Bmp388Model.PWR_TEMP_EN, 0)
        sim.spi_transfer("baro", Bmp388Model.DATA_0 | 0x80, None, 6)  # consume ready flags
        sim.tick()
        count = int.from_bytes(sim.spi_transfer("imu", Icm42688Model.FIFO_COUNTH | 0x80, None, 2), "big")
        self.assertGreater(count, 0)
        status = sim.spi_transfer("baro", Bmp388Model.STATUS | 0x80, None, 1)[0]
        self.assertTrue(status & Bmp388Model.STATUS_DRDY_PRESS)


if __name__ == "__main__":
    unittest.main()
