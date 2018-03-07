obj-$(CONFIG_SUNXI_SRAM) +=	sunxi_sram.o
obj-y +=	sunxi-sid.o

ifneq ($(strip $(CONFIG_ARCH_SUN8IW7)),)
obj-$(CONFIG_SUNXI_SUSPEND) += power/
else ifneq ($(strip $(CONFIG_ARCH_SUN8IW6)),)
obj-$(CONFIG_SUNXI_SUSPEND) +=  pm_legacy/
obj-$(CONFIG_SCENELOCK)     +=  pm_legacy/scenelock/scenelock.o
obj-$(CONFIG_USER_SCENELOCK) += pm_legacy/scenelock/userscenelock.o
else
obj-$(CONFIG_SUNXI_SUSPEND) += pm/
endif

obj-$(CONFIG_SUNXI_ARISC)       += arisc/
