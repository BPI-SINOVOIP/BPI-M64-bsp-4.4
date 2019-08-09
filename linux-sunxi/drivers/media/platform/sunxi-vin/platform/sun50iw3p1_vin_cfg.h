
/*
 * Hawkview ISP - sun50iw3p1_vin_cfg.h module
 *
 * Copyright (c) 2018 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _SUN50IW3P1_VIN_CFG_H_
#define _SUN50IW3P1_VIN_CFG_H_

#define CSI0_REGS_BASE          		0x06621000
#define CSI1_REGS_BASE          		0x06621000
#define CSI0_CCI_REG_BASE			0x0662e000
#define VIPP0_REGS_BASE          		0x02101000
#define VIPP1_REGS_BASE				0x02101400

#define ISP_REGS_BASE				0x02100000
#define GPIO_REGS_VBASE				0x0300b000
#define CPU_DRAM_PADDR_ORG 			0x40000000
#define HW_DMA_OFFSET				0x00000000

/*set vin core clk base on sensor size*/
#define CORE_CLK_RATE_FOR_2M (108*1000*1000)
#define CORE_CLK_RATE_FOR_3M (216*1000*1000)
#define CORE_CLK_RATE_FOR_5M (216*1000*1000)
#define CORE_CLK_RATE_FOR_8M (432*1000*1000)
#define CORE_CLK_RATE_FOR_16M (432*1000*1000)

/*CSI & ISP size configs*/

#define CSI0_REG_SIZE			0x1000
#define MIPI_CSI_REG_SIZE		0x1000
#define MIPI_DPHY_REG_SIZE		0x1000
#define CSI0_CCI_REG_SIZE		0x1000
#define CSI1_REG_SIZE			0x1000
#define CSI1_CCI_REG_SIZE		0x1000
#define VIPP0_REG_SIZE			0x400
#define VIPP1_REG_SIZE			0x400

#define VIN_MAX_DEV			2
#define VIN_MAX_CSI			1
#define VIN_MAX_CCI			1
#define VIN_MAX_MIPI			0
#define VIN_MAX_ISP			1
#define VIN_MAX_SCALER			2

#define MAX_CH_NUM      4

#endif /*_SUN50IW3P1_VIN_CFG_H_*/
