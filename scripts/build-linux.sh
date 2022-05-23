LINUX_VER=5.16.5

make -C linux-build/linux-${LINUX_VER} \
    ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- defconfig
make -C linux-build/linux-${LINUX_VER} \
    ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j $(nproc)
make -C linux-build/opensbi \
    PLATFORM=generic CROSS_COMPILE=riscv64-unknown-linux-gnu- \
    PLATFORM_RISCV_XLEN=64 FW_PAYLOAD_PATH=../linux-${LINUX_VER}/arch/riscv/boot/Image
