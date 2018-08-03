#
# Makefile for the touchscreen drivers.
#

# Each configuration option enables a list of files.

EXTRA_CFLAGS +=-Wno-error=date-time

obj-$(CONFIG_TOUCHSCREEN_GT9XX)          += gt9xxnew_ts.o
gt9xxnew_ts-objs := goodix_tool.o gt9xx_update.o gt9xx.o

