
/*
 * Hawkview ISP - sun8iw11p1_vin_cfg.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SUN8IW11P1_VIN_CFG_H_
#define _SUN8IW11P1_VIN_CFG_H_

#define MIPI_CSI_NOT_EXIST

#define CSI0_REGS_BASE        			0x01c09000	/* 0x01c09000 */
#define CSI1_REGS_BASE        			0X01c1D000	/* 0X01c1D000 */

#define CSI0_CCI_REG_BASE			0x01cb3000
#define MIPI_CSI0_REGS_BASE    			0x01cb1000

#define ISP_REGS_BASE           		0x01cb8000

/*set vin core clk base on sensor size*/
#define CORE_CLK_RATE_FOR_2M (108*1000*1000)
#define CORE_CLK_RATE_FOR_3M (216*1000*1000)
#define CORE_CLK_RATE_FOR_5M (216*1000*1000)
#define CORE_CLK_RATE_FOR_8M (432*1000*1000)
#define CORE_CLK_RATE_FOR_16M (432*1000*1000)

/*CSI & ISP size configs*/

#define CSI0_REG_SIZE               0x1000
#define MIPI_CSI_REG_SIZE           0x1000
#define MIPI_DPHY_REG_SIZE          0x1000
#define CSI0_CCI_REG_SIZE          0x1000
#define CSI1_REG_SIZE               0x1000
#define CSI1_CCI_REG_SIZE          0x1000

#define VIN_MAX_DEV			2
#define VIN_MAX_CSI			2
#define VIN_MAX_CCI			2
#define VIN_MAX_MIPI			0
#define VIN_MAX_ISP			2
#define VIN_MAX_SCALER			2

#define MAX_CH_NUM      4

#endif /*_SUN8IW11P1_VIN_CFG_H_*/
