LINUX_OUT=linux-build
LINUX_VER=5.16.5
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-${LINUX_VER}.tar.xz
LINUX_SRC=$(LINUX_OUT)/linux-${LINUX_VER}

OPENSBI_OUT=$(LINUX_OUT)
OPENSBI_VER=1.1
OPENSBI_SRC_URL=https://github.com/riscv-software-src/opensbi/archive/refs/tags/v$(OPENSBI_VER).tar.gz
OPENSBI_SRC=$(OPENSBI_OUT)/opensbi-$(OPENSBI_VER)

BUSYBOX_OUT=$(LINUX_OUT)
BUSYBOX_VER=1.35.0
BUSYBOX_SRC_URL = https://busybox.net/downloads/busybox-${BUSYBOX_VER}.tar.bz2
BUSYBOX_SRC = $(BUSYBOX_OUT)/busybox-${BUSYBOX_VER}

define download-n-extract
$(eval $(T)_SRC_ARCHIVE = $($(T)_OUT)/$(shell basename $($(T)_SRC_URL)))
$($(T)_SRC_ARCHIVE):
	$(Q)echo "  1. Download $$@..."
	$(Q)mkdir -p $($(T)_OUT)
	$(Q)curl --progress-bar -o $$@ -L -C - "$(strip $($(T)_SRC_URL))"
$($(T)_SRC): $($(T)_SRC_ARCHIVE)
	$(Q)echo "  2. Extract $$@..."
	$(Q)tar -xf $$< -C $($(T)_OUT)
endef

EXTERNAL_SRC = LINUX OPENSBI BUSYBOX
$(foreach T,$(EXTERNAL_SRC),$(eval $(download-n-extract)))

LINUX_IMG=$(OPENSBI_SRC)/build/platform/generic/firmware/fw_payload.elf
$(LINUX_IMG): $(OPENSBI_SRC) $(LINUX_SRC)
	make -C $(LINUX_SRC) \
		ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- defconfig
	make -C $(LINUX_SRC) \
		ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j 2
	make -C $(OPENSBI_SRC) \
		PLATFORM=generic PLATFORM_RISCV_XLEN=64 \
		CROSS_COMPILE=riscv64-unknown-linux-gnu- \
		FW_PAYLOAD_PATH=../../$(LINUX_SRC)/arch/riscv/boot/Image

BUSYBOX_BIN=$(LINUX_OUT)/rootfs/bin/busybox
$(BUSYBOX_BIN): $(BUSYBOX_SRC)
	make -C $(BUSYBOX_SRC) defconfig
	make -C $(BUSYBOX_SRC) -j 2
	make -C $(BUSYBOX_SRC) CONFIG_PREFIX='../rootfs' install

LINUX_RFS_IMG=$(LINUX_OUT)/rootfs/rootfs.cpio
$(LINUX_RFS_IMG): $(BUSYBOX_BIN)
	cd $(LINUX_OUT)/rootfs; \
	ls; \
	mv linuxrc init; \
	mkdir -p etc/init.d; \
	cp -f ../../configs/rc-startup etc/init.d/rcS; \
	chmod 755 etc/init.d/rcS; \
	find . | cpio -o --format=newc > $(abspath $@)