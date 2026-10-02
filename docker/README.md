# Reproducible S09 toolchain

Build and run the same host + ARM/QEMU regression environment with:

```bash
docker build -f docker/Dockerfile -t ansa-s09 .
docker run --rm ansa-s09
```

The image pins the environment to Debian Bookworm package repositories and installs CMake, GCC, pytest, `arm-none-eabi-gcc`, `gdb-multiarch`, and `qemu-system-arm`.
