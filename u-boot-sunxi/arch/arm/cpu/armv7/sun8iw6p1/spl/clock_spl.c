/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
* SPDX-License-Identifier: GPL-2.0+
**********************************************************************************************************************
*/
#include "common.h"
#include "asm/io.h"
#include "asm/armv7.h"
#include "asm/arch/cpu.h"
#include "asm/arch/ccmu.h"
#include "asm/arch/timer.h"

static bool ahb_secure;

void open_timestamp(void)
{
	u32 reg_val = 0;

	reg_val = readl(0x01720000);
	reg_val |= (1 << 0);
    writel(reg_val, 0x01720000);
}

static void set_pll_cpux(__u32 reg)
{
	__u32 reg_val;

	/*set PLL_C0CPUX, the  default  clk is 408M  ,PLL_OUTPUT= 24M*N/P*/
	reg_val = readl(reg);
	reg_val &= ~(1 << 31);
	writel(reg_val, reg);
	__usdelay(5);

	reg_val = readl(reg);
	reg_val &= ~((0x1 << 16) | (0xff << 8));
	reg_val |= (0x11 << 8);
	writel(reg_val, reg);
	__usdelay(10);

	reg_val = readl(reg);
	reg_val |= (0x1 << 31);
	writel(reg_val, reg);
	__usdelay(10);
}

static void set_pll_hsic(__u32 reg)
{
	__u32 reg_val;

	/*pll_hsic = 432M*/
	reg_val = readl(reg);
	reg_val &= ~(1 << 31);
	writel(reg_val, reg);
	__usdelay(5);

	reg_val = readl(reg);
	reg_val &= ~((0x1 << 18) | (0x1 << 16) | (0xff << 8));
	reg_val |= ((0x1 << 18) | (0x24 << 8));
	writel(reg_val, reg);
	__usdelay(10);

	reg_val = readl(reg);
	reg_val |= (0x1 << 31);
	writel(reg_val, reg);
	__usdelay(10);
}

static void set_pll_perih(void)
{
	__u32 reg_val;
	/*pll_periph=600M*/
	reg_val = readl(CCMU_PLL_PERIPH_CTRL_REG);
	reg_val &= ~(1 << 31);
	writel(reg_val, CCMU_PLL_PERIPH_CTRL_REG);
	__usdelay(10);

	reg_val = readl(CCMU_PLL_PERIPH_CTRL_REG);
	reg_val &= ~((0x1 << 18) | (0x1 << 16) | (0xff << 8));
	reg_val |= (0x32 << 8);
	writel(reg_val, CCMU_PLL_PERIPH_CTRL_REG);
	__usdelay(10);

	reg_val = readl(CCMU_PLL_PERIPH_CTRL_REG);
	reg_val |= (0x1 << 31);
	writel(reg_val, CCMU_PLL_PERIPH_CTRL_REG);
	__usdelay(10);
}

static void set_cci400(void)
{
	__u32 reg_val;

 /*cci400 clk src is pll_hsic: 480M*/
	CP15DMB;
	CP15ISB;

	reg_val = readl(CCMU_CCI400_CFG_REG);
	reg_val &= ~(0x3 << 24);
	writel(reg_val, CCMU_CCI400_CFG_REG);
	__usdelay(10);

	reg_val = readl(CCMU_CCI400_CFG_REG);
	reg_val &= ~(0x3 << 0);
	reg_val |= (0x0 << 0);
	writel(reg_val, CCMU_CCI400_CFG_REG);
	__usdelay(10);

	reg_val = readl(CCMU_CCI400_CFG_REG);
	reg_val |= (0x2 << 24);
	writel(reg_val, CCMU_CCI400_CFG_REG);
	__usdelay(100);

	CP15DMB;
	CP15ISB;
}

static void set_dma(void)
{
	/* dma reset */
	writel(readl(CCMU_BUS_SOFT_RST_REG0)  | (1 << 6), CCMU_BUS_SOFT_RST_REG0);
	__usdelay(20);
	/* gating clock for dma pass */
	writel(readl(CCMU_BUS_CLK_GATING_REG0) | (1 << 6), CCMU_BUS_CLK_GATING_REG0);
    /* auto gating disable */
	writel(7, (SUNXI_DMA_BASE+0x20));
}

static void switch_ahb1_apb1_cpu_to_pll(void)
{
	__u32 reg_val;
	__u32 ahb_pre_div;
	__u32 ahb_div;

	/*ahb1 clock src is PLL6,                           (0x02 << 12)
	apb1 clk src is ahb1 clk src, divide  ratio is 2  (1<<8)
	ahb1 pre divide  ratio is 2:    0:1  , 1:2,  2:3,   3:4 (2<<6)
	PLL6:AHB1:APB1 = 1200M:200M:100M */
	if (ahb_secure) {
		ahb_pre_div = 2;
		ahb_div = 2;
	} else {
		ahb_pre_div = 2;
		ahb_div = 1;
	}
	reg_val = readl(CCMU_AHB1_APB1_CFG_REG);
	reg_val &= (0x3 << 8) | (0x3 << 6) | (0x3 << 4);
	reg_val |= (1 << 8) | (ahb_pre_div << 6) | (ahb_div << 4);
	writel(reg_val, CCMU_AHB1_APB1_CFG_REG);
	__usdelay(10);

	reg_val = readl(CCMU_AHB1_APB1_CFG_REG);
	reg_val &= ~(0x3 << 12);
	reg_val |= (0x02 << 12);
	writel(reg_val, CCMU_AHB1_APB1_CFG_REG);
	__usdelay(10);

}

static void switch_mbus_to_pll(void)
{
	__u32 reg_val;

	writel(0x80000000, CCMU_MBUS_RST_REG);

    /* open MBUS,clk src is pll6 , pll6/(m+1) = 400M */
	/* writel(0x81000002, CCMU_MBUS_CLK_REG); */
	reg_val = readl(CCMU_MBUS_CLK_REG);
	reg_val &= ~(0x1 << 31);
	writel(reg_val, CCMU_MBUS_CLK_REG);
	__usdelay(5);

	reg_val = readl(CCMU_MBUS_CLK_REG);
	reg_val &= ~(0x3 << 0);
	reg_val |= (0x02 << 0);
	writel(reg_val, CCMU_MBUS_CLK_REG);
	 __usdelay(1);

	reg_val = readl(CCMU_MBUS_CLK_REG);
	reg_val &= ~(0x3 << 24);
	reg_val |= (0x01 << 24);
	writel(reg_val, CCMU_MBUS_CLK_REG);
	__usdelay(5);

	reg_val = readl(CCMU_MBUS_CLK_REG);
	reg_val |= (0x1 << 31);
	writel(reg_val, CCMU_MBUS_CLK_REG);
	__usdelay(10);

}

static void switch_cpu_to_pll(void)
{
	__u32 reg_val;

	/*set and change cpu clk src to pll1,  PLL1:AXI0 = 408M:204M*/
	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(3 << 0);
	reg_val |=  (1 << 0);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(10);

	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(1 << 12);
	reg_val |=  (1 << 12);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(100);

	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(3 << 16);
	reg_val |=  (1 << 16);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(10);

	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(1 << 28);
	reg_val |=  (1 << 28);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(100);

}

static void switch_cpu_to_osc(void)
{
	__u32 reg_val;

	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(1 << 12);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(5);

	reg_val = readl(CCMU_CPUX_AXI_CFG_REG);
	reg_val &= ~(3 << 16);
	writel(reg_val, CCMU_CPUX_AXI_CFG_REG);
	__usdelay(5);

}

static void switch_ahb1_apb1_to_osc(void)
{
	__u32 reg_val;

	reg_val = readl(CCMU_AHB1_APB1_CFG_REG);
	reg_val &= ~(0x3 << 12);
	reg_val |= (0x01 << 12);
	writel(reg_val, CCMU_AHB1_APB1_CFG_REG);
	__usdelay(10);

}

static void switch_cci_to_osc(void)
{
	__u32 reg_val;

	reg_val = readl(CCMU_CCI400_CFG_REG);
	reg_val &= (0x3 << 24);
	writel(reg_val, CCMU_CCI400_CFG_REG);
	__usdelay(10);

}

static void switch_mbus_to_osc(void)
{
	__u32 reg_val;

	reg_val = readl(CCMU_MBUS_CLK_REG);
	reg_val &= ~(0x3 << 24);
	writel(reg_val, CCMU_MBUS_CLK_REG);
	__usdelay(10);

}

/*******************************************************************************
*函数名称: set_pll
*函数原型：void set_pll( void )
*函数功能: 调整CPU频率
*入口参数: void
*返 回 值: void
*备    注:
*******************************************************************************/
void set_pll(void)
{
	__u32 reg_val;

	open_timestamp();

	/* switch cpu/ahb/apb/cci/mbus to osc*/
	switch_cpu_to_osc();
	switch_cci_to_osc();
	switch_ahb1_apb1_to_osc();
	switch_mbus_to_osc();

    /*set pll_c0cpux*/
	set_pll_cpux(CCMU_PLL_C0CPUX_CTRL_REG);
#ifndef CONFIG_SUNXI_FPGA
	do {
		reg_val = readl(CCMU_PLL_STB_STATUS_REG);
	} while (!(reg_val & 0x1));
#endif

	/* set PLL_C1CPUX, the  default  clk is 408M  ,PLL_OUTPUT= 24M*N/P */
	set_pll_cpux(CCMU_PLL_C1CPUX_CTRL_REG);
	/* wait pll1 stable */
#ifndef CONFIG_SUNXI_FPGA
	do {
		reg_val = readl(CCMU_PLL_STB_STATUS_REG);
	} while (!(reg_val & 0x2));
#endif
	/* enable pll_hsic, default is 480M */
	set_pll_hsic(CCMU_PLL_HSIC_CTRL_REG);
#ifndef CONFIG_SUNXI_FPGA
	do {
		reg_val = readl(CCMU_PLL_STB_STATUS_REG);
	} while (!(reg_val & (1 << 8)));
#endif

	/* cci400 clk src is pll_hsic: (2<<24) , div is 1:(0<<0) */
	set_cci400();

    /* enable PLL6 */
	set_pll_perih();
#ifndef CONFIG_SUNXI_FPGA
    do {
	    reg_val = readl(CCMU_PLL_STB_STATUS_REG);
    } while (!(reg_val & (1 << 6)));
#endif

	/* ahb1_clk/apb1_clk*/
	switch_ahb1_apb1_cpu_to_pll();

	/* set and change cpu clk src to pll1,  PLL1:AXI0 = 408M:204M */
	switch_cpu_to_pll();

	/* ----DMA function-------- */
	set_dma();

	/* set mbus*/
	switch_mbus_to_pll();

	return ;
}

void set_pll_in_secure_mode(void)
{
    ahb_secure = 1;
    set_pll();
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
void reset_pll( void )
{
    /*switch cpu/ahb/apb to osc*/
	switch_cpu_to_osc();
	switch_cci_to_osc();
	switch_ahb1_apb1_to_osc();
	switch_mbus_to_osc();
    /* 还原PLL_C0CPUX为默认值 */
	writel(0x02001100, CCMU_PLL_C0CPUX_CTRL_REG);

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*函数名称：
*
*参数列表：
*
*返回值  ：
*
*说明    ：
*
*
************************************************************************************************************
*/
void set_gpio_gate(void)
{

}
