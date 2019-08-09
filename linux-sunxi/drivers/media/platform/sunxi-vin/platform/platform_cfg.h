
/*
 * vin platform config header file
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PLATFORM_CFG__H__
#define __PLATFORM_CFG__H__

/*#define FPGA_VER*/
#define SUNXI_MEM
/* #define VIN_IN_UBOOT */

#ifndef FPGA_VER
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/sh_clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/regulator/consumer.h>
#endif

#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/slab.h>
#include <linux/kernel.h>

#include "../utility/vin_os.h"
#include "../vin-mipi/combo_common.h"

#ifdef FPGA_VER
#define DPHY_CLK (48*1000*1000)
#else
#define DPHY_CLK (150*1000*1000)
#endif

#if defined CONFIG_ARCH_SUN50IW1P1
#include "sun50iw1p1_vfe_cfg.h"
#elif defined CONFIG_ARCH_SUN8IW11P1
#include "sun8iw11p1_vfe_cfg.h"
#elif defined CONFIG_ARCH_SUN50IW3P1
#include "sun50iw3p1_vin_cfg.h"
#define CROP_AFTER_SCALER
#elif defined CONFIG_ARCH_SUN50IW6P1
#include "sun50iw6p1_vin_cfg.h"
#define CROP_AFTER_SCALER
#elif defined CONFIG_ARCH_SUN8IW12P1
#include "sun8iw12p1_vin_cfg.h"
#elif defined CONFIG_ARCH_SUN8IW17P1
#include "sun8iw12p1_vin_cfg.h"
#endif

#define ALIGN_4K(x)	(((x) + (4095)) & ~(4095))
#define ALIGN_32B(x)	(((x) + (31)) & ~(31))
#define ALIGN_16B(x)	(((x) + (15)) & ~(15))
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define CLIP(a, i, s)	(((a) > (s)) ? (s) : MAX(a, i))

struct mbus_framefmt_res {
	u32 res_pix_fmt;
	u32 res_mipi_bps;
	u8 res_combo_mode;
	u8 res_wdr_mode;
};

enum steam_on_seq {
	SENSOR_BEFORE_MIPI = 0,
	MIPI_BEFORE_SENSOR,
};

#define CSI_CH_0	(1 << 20)
#define CSI_CH_1	(1 << 21)
#define CSI_CH_2	(1 << 22)
#define CSI_CH_3	(1 << 23)

#define MAX_DETECT_NUM	3

/*
 * The subdevices' group IDs.
 */
#define VIN_GRP_ID_SENSOR	(1 << 8)
#define VIN_GRP_ID_MIPI		(1 << 9)
#define VIN_GRP_ID_CSI		(1 << 10)
#define VIN_GRP_ID_ISP		(1 << 11)
#define VIN_GRP_ID_SCALER	(1 << 12)
#define VIN_GRP_ID_CAPTURE	(1 << 13)
#define VIN_GRP_ID_STAT		(1 << 14)

#define VIN_ALIGN_WIDTH 16
#define VIN_ALIGN_HEIGHT 16

#endif /*__PLATFORM_CFG__H__*/
