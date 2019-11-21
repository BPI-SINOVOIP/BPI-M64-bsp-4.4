.PHONY: all clean help
.PHONY: u-boot kernel kernel-config
.PHONY: linux pack

include chosen_board.mk

SUDO=sudo
U_CROSS_COMPILE=$(U_COMPILE_TOOL)/arm-linux-gnueabi-
K_CROSS_COMPILE=$(K_COMPILE_TOOL)/aarch64-linux-gnu-

LICHEE_KDIR=$(CURDIR)/linux-sunxi
LICHEE_MOD_DIR=$(LICHEE_KDIR)/output/lib/modules/4.4.89-BPI-M64-Kernel
ROOTFS=$(CURDIR)/rootfs/linux/default_linux_rootfs.tar.gz

Q=
J=$(shell expr `grep ^processor /proc/cpuinfo  | wc -l` \* 2)

all: bsp
bsp: u-boot kernel


# u-boot
u-boot: 
	$(Q)$(MAKE) -C u-boot-sunxi $(MACH)_config CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J
	$(Q)$(MAKE) -C u-boot-sunxi all CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J

u-boot-clean:
	$(Q)$(MAKE) -C u-boot-sunxi CROSS_COMPILE=$(U_CROSS_COMPILE) -j$J distclean

kernel: 
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 $(KERNEL_CONFIG)
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output Image.gz dtbs
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules
	$(Q)$(MAKE) -C linux-sunxi/modules/gpu/mali-utgard/kernel_mode/driver/src/devicedrv/mali CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm64 KDIR=${LICHEE_KDIR} LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=0
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J INSTALL_MOD_PATH=output modules_install
	$(Q)scripts/install_kernel_headers.sh $(K_CROSS_COMPILE)

kernel-clean:
	$(Q)$(MAKE) -C linux-sunxi/modules/gpu/mali-utgard/kernel_mode/driver/src/devicedrv/mali CROSS_COMPILE=$(K_CROSS_COMPILE) ARCH=arm64 KDIR=${LICHEE_KDIR} LICHEE_KDIR=${LICHEE_KDIR} USING_DT=1 BUILD=release USING_UMP=0 clean
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J distclean
	rm -rf linux-sunxi/output/

kernel-config:
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 $(KERNEL_CONFIG)
	$(Q)$(MAKE) -C linux-sunxi ARCH=arm64 CROSS_COMPILE=${K_CROSS_COMPILE} -j$J menuconfig
	cp linux-sunxi/.config linux-sunxi/arch/arm64/configs/$(KERNEL_CONFIG)

## clean all build
clean: u-boot-clean kernel-clean
	rm -rf out
	rm -f chosen_board.mk
	rm -f env.sh

## pack download files
pack: sunxi-pack
	$(Q)scripts/mk_pack.sh

## install to bpi image
install:
	$(Q)scripts/mk_install_sd.sh

## linux rootfs
linux: 
	$(Q)scripts/mk_linux.sh $(ROOTFS)

help:
	@echo ""
	@echo "Usage:"
	@echo ""
	@echo "  make bsp             - Default 'make'"
	@echo "  make u-boot          - Builds bootloader"
	@echo "  make kernel          - Builds linux kernel"
	@echo "  make kernel-config   - Menuconfig"
	@echo ""
	@echo "  make pack            - pack the images and rootfs to bpi tools download package."
	@echo "  make install         - download the build packages to SD device with bpi image flashed."
	@echo "  make clean           - clean all build."
	@echo ""
	@echo "Optional targets:"
	@echo ""
	@echo "  make linux           - Build target for linux platform, as ubuntu, need permisstion confirm during the build process"
	@echo "   Arguments:"
	@echo "    ROOTFS=            - Source rootfs (ie. rootfs.tar.gz with absolute path)"
	@echo ""

