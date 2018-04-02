#
# Copyright (C) 2013-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(BUILD_DIR)/image.mk
include $(BUILD_DIR)/host.mk

FAT32_BLOCK_SIZE=1024
ifneq ($(CONFIG_SUNXI_SD_BOOT_PARTSIZE),)
	FAT32_BLOCKS=$(shell echo $$(($(CONFIG_SUNXI_SD_BOOT_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))
endif
ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_PARTSIZE),)
	RECOVERY_INITRAMFS_BLOCKS=$(shell echo $$(($(CONFIG_SUNXI_RECOVERY_INITRAMFS_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))
endif
LOAD_ADDRESS:=0x40008000
ENTRY_POINT:=0x40008000
BASE_ADDRESS:=0x40000000
KERNEL_OFFSET:=0x8000
RAMDISK_OFFSET:=0x01000000
IMAGE_DATA:=zImage
BOOTIMG_IMAGE_DATA:=zImage
RAMDISK:=ramdisk.img
UIMAGE_NAME:=uImage
BOOTIMG_NAME:=boot.img
TARCH:=$(ARCH)
COMP:=none
ifeq ($(ARCH), aarch64)
TARCH=arm64
endif
# max size of board name is 16
BOOTIMG_BOARD:=$(IMG_PREFIX)

ifneq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),)
UIMAGE_NAME=uImage-initramfs
BOOTIMG_NAME=boot-initramfs.img
endif

ifeq ($(TARGET_BOARD_PLATFORM), tulip)
LOAD_ADDRESS=0x40080000
ENTRY_POINT=0x40080000
BASE_ADDRESS=0x40000000
KERNEL_OFFSET=0x80000
IMAGE_DATA=Image.gz
BOOTIMG_IMAGE_DATA=bImage
COMP=gzip
ifneq ($(CONFIG_KERNEL_KERNEL_LZO),)
IMAGE_DATA=Image.lzo
COMP=lzo
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), koto)
LOAD_ADDRESS=0x40080000
ENTRY_POINT=0x40080000
BASE_ADDRESS=0x40000000
KERNEL_OFFSET=0x80000
IMAGE_DATA=Image.gz
BOOTIMG_IMAGE_DATA=bImage
COMP=gzip
ifneq ($(CONFIG_KERNEL_KERNEL_LZO),)
IMAGE_DATA=Image.lzo
COMP=lzo
endif
endif

ifeq ($(TARGET_BOARD_PLATFORM), sitar)
LOAD_ADDRESS=0x80008000
ENTRY_POINT=0x80008000
BASE_ADDRESS=0x80000000
KERNEL_OFFSET=0x8000
endif

define mkBootimg
mkbootimg --kernel $(1) \
	--ramdisk $(2) \
	--board $(3) \
	--base $(4) \
	--kernel_offset $(5) \
	--ramdisk_offset $(6) \
	-o $(7)
endef

define mkuImage
	mkimage -A $(TARCH) -O linux -T kernel -C $(1) -a $(2) -e $(3) \
		-n '$(call toupper,$(ARCH)) OpenWrt Linux-$(LINUX_VERSION)' -d $(4) $(5)
endef

define Image/BuildKernel

ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_UIMAGE),)
	$(call mkuImage, \
		$(COMP), $(LOAD_ADDRESS), $(ENTRY_POINT), \
		$(KDIR)/$(IMAGE_DATA), $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) \
		)
else ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_BOOTIMG),)
	$(call mkBootimg, \
		$(KDIR)/$(BOOTIMG_IMAGE_DATA), \
		$(RAMDISK), \
	        ${BOOTIMG_BOARD}, \
		${BASE_ADDRESS}, \
		${KERNEL_OFFSET}, \
		${RAMDISK_OFFSET}, \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME),\
		)
else
	# Compatible with old config
	$(call mkuImage, \
		$(COMP), $(LOAD_ADDRESS), $(ENTRY_POINT), \
		$(KDIR)/$(IMAGE_DATA), $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) \
		)
endif

endef

define Image/Build/Initramfs

ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_UIMAGE),)
	$(CP) $(KDIR)/$(IMAGE_DATA)-initramfs $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs
	echo -ne '\x00\x00\x00\x00' >> $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs
	$(call mkuImage, \
		$(COMP), $(LOAD_ADDRESS), $(ENTRY_POINT), \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs, \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) \
		)
else ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_BOOTIMG),)
	$(CP) $(KDIR)/$(BOOTIMG_IMAGE_DATA)-initramfs $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_IMAGE_DATA)-initramfs
	echo -ne '\x00\x00\x00\x00' >> $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_IMAGE_DATA)-initramfs
	$(call mkBootimg, \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_IMAGE_DATA)-initramfs, \
		$(RAMDISK), \
	        ${BOOTIMG_BOARD}, \
		${BASE_ADDRESS}, \
		${KERNEL_OFFSET}, \
		${RAMDISK_OFFSET}, \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME),\
		)
else
	# Compatible with old config
	$(CP) $(KDIR)/$(IMAGE_DATA)-initramfs $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs
	echo -ne '\x00\x00\x00\x00' >> $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs
	$(call mkuImage, \
		$(COMP), $(LOAD_ADDRESS), $(ENTRY_POINT), \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(IMAGE_DATA)-initramfs, \
		$(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) \
		)

endif
endef

ifneq ($(CONFIG_TARGET_ROOTFS_INITRAMFS),)
define Image/InstallKernel
	rm -f $(TARGET_OUT_DIR)/boot.img
	rm -f $(TARGET_OUT_DIR)/boot_initramfs.img
ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_PARTSIZE),)
	mkdosfs $(TARGET_OUT_DIR)/boot_initramfs.img -C $(RECOVERY_INITRAMFS_BLOCKS)

ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_KERNEL_FORMAT_UIMAGE),)
	mcopy -i $(TARGET_OUT_DIR)/boot_initramfs.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) ::uImage
else ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_KERNEL_FORMAT_BOOTIMG),)
	mcopy -i $(TARGET_OUT_DIR)/boot_initramfs.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME) ::boot.img
else
	# Compatible with old config
	mcopy -i $(TARGET_OUT_DIR)/boot_initramfs.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) ::uImage
endif

else

ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_KERNEL_FORMAT_UIMAGE),)
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) $(TARGET_OUT_DIR)/boot_initramfs.img
else ifneq ($(CONFIG_SUNXI_RECOVERY_INITRAMFS_KERNEL_FORMAT_BOOTIMG),)
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME) $(TARGET_OUT_DIR)/boot_initramfs.img
else
	# Compatible with old config
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) $(TARGET_OUT_DIR)/boot_initramfs.img
endif

endif
endef

else

define Image/InstallKernel
	rm -f $(TARGET_OUT_DIR)/boot.img
	rm -f $(TARGET_OUT_DIR)/boot_initramfs.img

ifneq ($(CONFIG_SUNXI_SD_BOOT_PARTSIZE),)
	mkdosfs $(TARGET_OUT_DIR)/boot.img -C $(FAT32_BLOCKS)

ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_UIMAGE),)
	mcopy -i $(TARGET_OUT_DIR)/boot.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) ::uImage
else ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_BOOTIMG),)
	mcopy -i $(TARGET_OUT_DIR)/boot.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME) ::boot.img
else
	# Compatible with old config
	mcopy -i $(TARGET_OUT_DIR)/boot.img $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) ::uImage
endif

else

ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_UIMAGE),)
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) $(TARGET_OUT_DIR)/boot.img
else ifneq ($(CONFIG_SUNXI_SD_BOOT_KERNEL_FORMAT_BOOTIMG),)
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(BOOTIMG_NAME) $(TARGET_OUT_DIR)/boot.img
else
	# Compatible with old config
	$(CP) $(TARGET_OUT_DIR)/$(IMG_PREFIX)-$(UIMAGE_NAME) $(TARGET_OUT_DIR)/boot.img
endif

endif
endef
endif

define Image/Build
	rm -f $(TARGET_OUT_DIR)/rootfs.img
	rm -f $(TARGET_OUT_DIR)/usr.img
	$(call Image/Build/$(1),$(1))
	dd if=$(KDIR)/root.$(1) of=$(TARGET_OUT_DIR)/rootfs.img bs=128k conv=sync
ifneq ($(CONFIG_SUNXI_SMALL_STORAGE_OTA),)
	dd if=$(KDIR)/usr.$(1) of=$(TARGET_OUT_DIR)/usr.img bs=128k conv=sync
endif
endef

$(eval $(call BuildImage))
