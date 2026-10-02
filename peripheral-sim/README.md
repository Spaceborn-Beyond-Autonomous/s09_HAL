# ANSA S09 Peripheral Verification Layer

`peripheral_sim.py` is the independent Python register model for ICM-42688-P, BMP388 and HMC5883L. It now includes a dependency-free JSON-lines TCP bridge so host tooling can exercise the same register model through a stable transport.

## Socket bridge

```bash
python3 peripheral_sim.py --serve --host 127.0.0.1 --port 8765
```

Request/response examples:

```json
{"op":"spi","cs":"imu","reg":245,"len":1}
{"ok":true,"data":[71]}
```

```json
{"op":"i2c","addr":30,"reg":10,"len":3}
{"ok":true,"data":[72,52,51]}
```

This is a **verification transport**, not a claim that upstream QEMU exposes the sensor's SPI/I2C bus to a TCP socket. A QEMU-side adapter must translate firmware transactions into this protocol. The QEMU board limitations remain documented in `backends/qemu-cortex-m/README.md`.
