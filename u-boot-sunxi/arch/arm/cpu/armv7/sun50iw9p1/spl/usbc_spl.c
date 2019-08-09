/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#include "common.h"
#include "asm/io.h"
#include  <asm/arch/platform.h>

int usb_open_clock(void)
{
    u32 reg_value = 0;

#ifdef FPGA_PLATFORM
    /* change interfor on  fpga */
    reg_value = USBC_Readl(SUNXI_SYSCRL_BASE+0x04);
    reg_value |= 0x01;
    USBC_Writel(reg_value, SUNXI_SYSCRL_BASE+0x04);
#endif

    /*Enable module clock for USB phy0  */
    reg_value = readl(CCMU_USB0_CLK_REG);
    reg_value |= (1 << 29)|(1 << 30);
    writel(reg_value, CCMU_USB0_CLK_REG);
    __msdelay(10);

    /*Gating AHB clock for USB_phy0  */
    reg_value = readl(CCMU_USB_BGR_REG);
    reg_value |= (1 << 8);
    writel(reg_value, CCMU_USB_BGR_REG);
    __msdelay(10);

    /* AHB reset */
    reg_value = readl(CCMU_USB_BGR_REG);
    reg_value |= (1 << 24);
    writel(reg_value, CCMU_USB_BGR_REG);
    __msdelay(10);

    return 0;
}

int usb_close_clock(void)
{
    u32 reg_value = 0;

    /* AHB reset */
    reg_value = readl(CCMU_USB_BGR_REG);
    reg_value &= ~(1 << 24);
    writel(reg_value, CCMU_USB_BGR_REG);
    __msdelay(10);

    /*close usb ahb clock  */
    reg_value = readl(CCMU_USB_BGR_REG);
    reg_value &= ~(1 << 8);
    writel(reg_value, CCMU_USB_BGR_REG);
    __msdelay(10);

    /*close usb phy clock  */
    reg_value = readl(CCMU_USB0_CLK_REG);
    reg_value &= ~((1 << 29) | (1 << 30));
    writel(reg_value, CCMU_USB0_CLK_REG);
    __msdelay(10);

    return 0;
}

