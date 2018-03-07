# Makefile for the Linux idc drivers on sunxi platform.
# 30 june 2017, fanqinghua, <mailto:fanqinghua@allwinnertech.com>

obj-$(CONFIG_SUNXI_IDC) = idc.o
idc-y 							+= sunxi-interrupt-defer.o
idc-$(CONFIG_SUNXI_IDC_DEBUG) 	+= sunxi-interrupt-defer-debug.o
