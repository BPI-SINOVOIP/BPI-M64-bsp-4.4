/*
 * Copyright (C) 2016 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN8IW15_H
#define __MACH_SUNXI_CLK_SUN8IW15_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#include "clk-factors.h"

/* CCMU Register List */
#define PLL_CPU             0x0000
#define PLL_DDR             0x0010
#define PLL_PERIPH0         0x0020
#define PLL_PERIPH1         0x0028
#define PLL_GPU             0x0030
#define PLL_VIDEO0          0x0040
#define PLL_VIDEO1          0x0048
#define PLL_VE              0x0058
#define PLL_DE              0x0060
#define PLL_AUDIO           0x0078
#define PLL_24M             0x00b8

#define PLL_DDRPAT          0x0110
#define PLL_PERI0PAT0       0x0120
#define PLL_PERI0PAT1       0x0124
#define PLL_PERI1PAT0       0x0128
#define PLL_PERI1PAT1       0x012C
#define PLL_GPUPAT0         0x0130
#define PLL_GPUPAT1         0x0134
#define PLL_VIDEO0PAT0      0x0140
#define PLL_VIDEO0PAT1      0x0144
#define PLL_VIDEO1PAT0      0x0148
#define PLL_VIDEO1PAT1      0x014C
#define PLL_VEPAT0          0x0158
#define PLL_VEPAT1          0x015C
#define PLL_DEPAT0          0x0160
#define PLL_DEPAT1          0x0164
#define PLL_AUDIOPAT0       0x0178
#define PLL_AUDIOPAT1       0x017C


#define CPU_CFG             0x0500
#define PSI_CFG             0x0510
#define AHB3_CFG            0x051C
#define APB1_CFG            0x0520
#define APB2_CFG            0x0524
#define MBUS_CFG            0x0540

/* Accelerator */
#define DE_CFG              0x0600
#define DE_GATE             0x060C
#define EE_CFG              0x0610
#define EDMA_CFG            0x0640
#define EINK_GATE           0x061C
#define G2D_CFG             0x0630
#define G2D_GATE            0x063C
#define GPU_CFG             0x0670
#define GPU_GATE            0x067C
#define CE_CFG              0x0680
#define CE_GATE             0x068C
#define VE_CFG              0x0690
#define VE_GATE             0x069C
#define EMCE_CFG            0x06B0
#define EMCE_GATE           0x06BC

/* SYS Resource */
#define DMA_GATE            0x070C
#define MSGBOX_GATE         0x071C
#define SPINLOCK_GATE       0x072C
#define HSTIMER_GATE        0x073C
#define AVS_CFG             0x0740
#define DBGSYS_GATE         0x078C
#define PSI_GATE            0x079C
#define PWM_GATE            0x07AC
#define IOMMU_GATE          0x07BC

/* Storage Medium */
#define DRAM_CFG            0x0800
#define MBUS_GATE           0x0804
#define DRAM_GATE           0x080C
#define NAND0_CFG           0x0810
#define NAND1_CFG           0x0814
#define NAND_GATE           0x082C
#define SMHC0_CFG           0x0830
#define SMHC1_CFG           0x0834
#define SMHC2_CFG           0x0838
#define SMHC_GATE           0x084C

/* Common Interface */
#define UART_GATE           0x090C
#define TWI_GATE            0x091C
#define SCR_GATE            0x093C
#define SPI0_CFG            0x0940
#define SPI1_CFG            0x0944
#define SPI_GATE            0x096C
#define THS_GATE            0x09FC
#define I2S0_CFG            0x0A10
#define I2S1_CFG            0x0A14
#define I2S_GATE            0x0A1C
#define DMIC_CFG            0x0A40
#define DMIC_GATE           0x0A4C
#define CODEC_1X_CFG        0x0A50
#define CODEC_4X_CFG        0x0A54
#define CODEC_GATE          0x0A5C
#define USB0_CFG            0x0A70
#define USB1_CFG            0x0A74
#define USB_GATE            0x0A8C

/* Display Interface */
#define MIPI_DPHY0_CFG      0x0B20
#define MIPI_HOST0_CFG      0x0B24
#define MIPI_GATE           0x0B4C
#define DISPLAY_TOP_GATE    0x0B5C
#define TCON_LCD0_CFG       0x0B60
#define TCON_LCD_GATE       0x0B7C
#define LVDS_GATE           0x0BAC
#define CSI_MISC_CFG        0x0C00
#define CSI_TOP_CFG         0x0C04
#define CSI_MASTER_CFG      0x0C08
#define CSI_GATE            0x0C2C
#define SUNXI_CLK_MAX_REG   0x0C2C

/* DISPLL Register List */
#define PLL_DIS_CFG0        0x0000
#define PLL_DIS_CFG1        0x0004
#define PLL_DIS_TUN0        0x0008
#define PLL_DIS_TUN1        0x000C
#define PLL_DIS_PAT0        0x0010
#define PLL_DIS_PAT1        0x0014
#define PLL_DIS_SSC         0x0018
#define PLL_DIS_DIV         0x0020

/* PRCM Register List */
#define CPUS_CFG            0x0000
#define CPUS_APBS1_CFG      0x000C
#define CPUS_APBS2_CFG      0x0010
#define CPUS_TIMER_GATE     0x011C
#define CPUS_TWDOG_GATE     0x012C
#define CPUS_PWM_CFG        0x0130
#define CPUS_PWM_GATE       0x013C
#define CPUS_UART_GATE      0x018C
#define CPUS_TWI_GATE       0x019C
#define CPUS_RSB_GATE       0x01BC
#define CPUS_RTC_GATE       0x020C
#define CPUS_CLK_MAX_REG    0x020C

/* RTC Register List */
#define LOSC_OUT_GATE       0x0060

#define F_N8X8_P16x2(nv, pv)      (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, pv, 16, 2, 0, 0, 0, 0, 0, 0))
#define F_N8X8_D1V1X1_D2V0X1(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, d2v, 0, 1))
#define F_N8X8_D1V1X1(nv, d1v)             (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 1, 1, 0, 0, 0))
#define F_N8X8_D1V4X2_D2V0X2(nv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, d1v, 4, 2, d2v, 0, 2))
#define F_N8X8_P16X6_D1V1X1_D2V0X1(nv, pv, d1v, d2v) (FACTOR_ALL(nv, 8, 8, 0, 0, 0, 0, 0, 0, pv, 16, 6, d1v, 1, 1, d2v, 0, 1))

#define PLLCPU(n, p, freq)        {F_N8X8_P16x2(n, p), freq}
#define PLLDDR(n, d1, d2, freq)     {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLPERIPH0(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLPERIPH1(n, d1, d2, freq)  {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLGPU(n, d1, d2, freq)      {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLVIDEO0(n, d1, freq)       {F_N8X8_D1V1X1(n, d1), freq}
#define PLLVIDEO1(n, d1, freq)       {F_N8X8_D1V1X1(n, d1), freq}
#define PLLVE(n, d1, d2, freq)       {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLDE(n, d1, d2, freq)       {F_N8X8_D1V1X1_D2V0X1(n, d1, d2), freq}
#define PLLAUDIO(n, p, d1, d2, freq) {F_N8X8_P16X6_D1V1X1_D2V0X1(n, p, d1, d2), freq}

#endif
