obj-$(CONFIG_HDMI_DISP2_SUNXI) += hdmi.o

hdmi-y := drv_hdmi.o hdmi_core.o hdmi_edid.o hdmi_bsp.o

ifeq ($(CONFIG_ARM),y)
hdmi-$(CONFIG_ARCH_SUN8IW12) += hdmi_bsp_sun8iw12.o
hdmi-$(CONFIG_ARCH_SUN8IW7) += hdmi_bsp_sun8iw7.o
hdmi-$(CONFIG_ARCH_SUN8IW6) += hdmi_bsp_sun8iw6.o
endif

