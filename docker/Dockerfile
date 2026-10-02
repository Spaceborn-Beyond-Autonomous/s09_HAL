FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build python3 python3-pytest \
    gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch qemu-system-arm \
    ca-certificates git && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace/ansa
COPY . .
CMD ["bash", "scripts/run_all_tests.sh"]
