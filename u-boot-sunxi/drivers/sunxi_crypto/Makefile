#
# (C) Copyright 2013-2016
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
#
# SPDX-License-Identifier:     GPL-2.0+
#

ifdef CONFIG_ARCH_SUN8IW6P1
obj-$(CONFIG_SUNXI_SECURE_SYSTEM)+= sunxi_crypto_legacy.o
else
obj-$(CONFIG_SUNXI_SECURE_SYSTEM)	+= sunxi_crypto.o
obj-$(CONFIG_SUNXI_SECURE_SYSTEM)	+= ss_op.o
endif
obj-y	+= sunxi_efuse.o
