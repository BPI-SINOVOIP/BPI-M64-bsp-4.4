/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#include  <asm/io.h>
#include  <asm/arch/ccmu.h>
#include  <asm/arch/timer.h>
#include <common.h>
#include <asm/arch/usb.h>

#define HOSC_19M	1
#define HOSC_38M	2
#define HOSC_24M	3

void otg_phy_config(void)
{
    u32 reg_val = 0;
    reg_val = readl(SUNXI_USBOTG_BASE+USBC_REG_o_PHYCTL);
	reg_val &= ~(0x01<<USBC_PHY_CTL_SIDDQ);
	reg_val |= 0x01<<USBC_PHY_CTL_VBUSVLDEXT;
	writel(reg_val, SUNXI_USBOTG_BASE+USBC_REG_o_PHYCTL);
}

static void disable_otg_clk_reset_gating(void)
{
	u32 reg_temp = 0;

	reg_temp = readl(CCMU_USB_BGR_REG);
	reg_temp &= ~((0x1 << USBOTG_RESET_BIT) | (0x1 << USBOTG_CLK_ONOFF_BIT));
	writel(reg_temp, CCMU_USB_BGR_REG);
}

static void disable_phy_clk_reset_gating(void)
{
	u32 reg_value = 0;

	reg_value = readl(CCMU_USB0_CLK_REG);
	reg_value &= ~((0x1 << USB0_PHY_CLK_ONOFF_BIT) | (0x1 << USB0_PHY_RESET_BIT));
	writel(reg_value, CCMU_USB0_CLK_REG);
}

static void enable_otg_clk_reset_gating(void)
{
	u32 reg_value = 0;

	reg_value = readl(CCMU_USB_BGR_REG);
	reg_value |= (1 << USBOTG_RESET_BIT);
	writel(reg_value, CCMU_USB_BGR_REG);

	__usdelay(500);

	reg_value = readl(CCMU_USB_BGR_REG);
	reg_value |= (1 << USBOTG_CLK_ONOFF_BIT);
	writel(reg_value, CCMU_USB_BGR_REG);

	__usdelay(500);
}

static void enable_phy_clk_reset_gating(void)
{
	u32 reg_value = 0;

	reg_value = readl(CCMU_USB0_CLK_REG);
	reg_value |= (1 << USB0_PHY_CLK_ONOFF_BIT);
	writel(reg_value, CCMU_USB0_CLK_REG);

	__usdelay(500);

	reg_value = readl(CCMU_USB0_CLK_REG);
	reg_value |= (1 << USB0_PHY_RESET_BIT);
	writel(reg_value, CCMU_USB0_CLK_REG);
	__usdelay(500);
}

static u32 prode_which_osc(void)
{
	u32 val = 0;

	val = (readl(XO_CTRL_REG) & (0x3 << 14));
	val = (val >> 14);

	return val;
}


int usb_open_clock(void)
{
	u32 reg_value = 0;
	u32 hosc_type = 0;

	disable_otg_clk_reset_gating();
	disable_phy_clk_reset_gating();
	/*sel HOSC*/
	reg_value = readl(CCMU_USB0_CLK_REG);
	reg_value &= (~(0x1 << USB0_PHY_CLK_SEL_BIT));
	writel(reg_value, CCMU_USB0_CLK_REG);

	hosc_type = prode_which_osc();
	if (hosc_type == HOSC_24M) {
		reg_value = readl(CCMU_USB0_CLK_REG);
		reg_value |= (0x1 << USB0_PHY_CLK_DIV_BIT);
		writel(reg_value, CCMU_USB0_CLK_REG);

		reg_value = readl(SUNXI_USBOTG_BASE + 0x410);
		reg_value &= (~(0x1 << 16));
		writel(reg_value, SUNXI_USBOTG_BASE + 0x410);

	} else if (hosc_type == HOSC_19M) {
		reg_value = readl(CCMU_USB0_CLK_REG);
		reg_value |= (0x1 << USB0_PHY_CLK_DIV_BIT);
		writel(reg_value, CCMU_USB0_CLK_REG);

		reg_value = readl(SUNXI_USBOTG_BASE + 0x410);
		reg_value |= (0x1 << 16);
		writel(reg_value, SUNXI_USBOTG_BASE + 0x410);

	} else if (hosc_type == HOSC_38M) {
		reg_value = readl(CCMU_USB0_CLK_REG);
		reg_value &= (~(0x1 << USB0_PHY_CLK_DIV_BIT));
		writel(reg_value, CCMU_USB0_CLK_REG);

		reg_value = readl(SUNXI_USBOTG_BASE + 0x410);
		reg_value |= (0x1 << 16);
		writel(reg_value, SUNXI_USBOTG_BASE + 0x410);
	}

	reg_value = readl(SUNXI_USBOTG_BASE + 0x420);
	reg_value |= (0x01 << 0);
	writel(reg_value, (SUNXI_USBOTG_BASE + 0x420));
	__msdelay(1);

	enable_otg_clk_reset_gating();
	enable_phy_clk_reset_gating();

	return 0;
}

int usb_close_clock(void)
{
	disable_otg_clk_reset_gating();
	disable_phy_clk_reset_gating();

	return 0;
}
