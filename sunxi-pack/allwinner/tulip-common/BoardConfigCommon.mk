-include target/allwinner/generic/common.mk

TARGET_LINUX_VERSION:=3.10
TARGET_ARCH := aarch64
TARGET_ARCH_VARIANT :=
TARGET_CPU_VARIANT := cortex-a53

TARGET_ARCH_PACKAGES := sunxi

TARGET_BOARD_PLATFORM := tulip

PRODUCT_PACKAGE += \
	adb
