/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 */
#include <common.h>
#include <asm/arch/gpio.h>
#include <private_boot0.h>
#include "spi_hal.h"
#include <asm/io.h>

static u32 g_cfg_mclk;

#if 0
#define SUNXI_DEBUG(fmt, args...)    printf(fmt, ##args)
#else
#define SUNXI_DEBUG(fmt, args...) do {} while (0)
#endif

#define set_wbit(addr, v) (*((unsigned long  *)(addr)) \
		|=  (unsigned long)(v))
#define clr_wbit(addr, v) (*((unsigned long  *)(addr)) \
		&= ~(unsigned long)(v))


void spic_config_dual_mode(u32 spi_no, u32 rxdual, u32 dbc, u32 stc)
{
	writel((rxdual<<28)|(dbc<<24)|(stc), SPI_BCC);
}

void ccm_module_disable_bak(u32 clk_id)
{
	switch (clk_id >> 8) {
	case AHB1_BUS0:
		clr_wbit(CCM_AHB1_RST_REG0, 0x1U<<(clk_id&0xff));
		SUNXI_DEBUG("\nread CCM_AHB1_RST_REG0[0x%x]\n",
				readl(CCM_AHB1_RST_REG0));
		break;
	}
}

void ccm_module_enable_bak(u32 clk_id)
{
	switch (clk_id>>8) {
	case AHB1_BUS0:
		set_wbit(CCM_AHB1_RST_REG0, 0x1U<<(clk_id&0xff));
		SUNXI_DEBUG("\nread enable CCM_AHB1_RST_REG0[0x%x]\n",
				readl(CCM_AHB1_RST_REG0));
		break;
	}
}

void ccm_clock_enable_bak(u32 clk_id)
{
	switch (clk_id>>8) {
	case AXI_BUS:
		set_wbit(CCM_AXI_GATE_CTRL, 0x1U<<(clk_id&0xff));
		break;
	case AHB1_BUS0:
		set_wbit(CCM_AHB1_GATE0_CTRL, 0x1U<<(clk_id&0xff));
		SUNXI_DEBUG("read s CCM_AHB1_GATE0_CTRL[0x%x]\n",
				readl(CCM_AHB1_GATE0_CTRL));
		break;
	}
}

void ccm_clock_disable_bak(u32 clk_id)
{
	switch (clk_id>>8) {
	case AXI_BUS:
		clr_wbit(CCM_AXI_GATE_CTRL, 0x1U<<(clk_id&0xff));
		break;
	case AHB1_BUS0:
		clr_wbit(CCM_AHB1_GATE0_CTRL, 0x1U<<(clk_id&0xff));
		SUNXI_DEBUG("read dis CCM_AHB1_GATE0_CTRL[0x%x]\n",
				readl(CCM_AHB1_GATE0_CTRL));
		break;
	}
}

void ccm_module_reset_bak(u32 clk_id)
{
	ccm_module_disable_bak(clk_id);
	ccm_module_enable_bak(clk_id);
}

u32 ccm_get_pll_periph_clk(void)
{
	unsigned int reg_val;
	u32 factor_n, factor_k, pll6;

	reg_val = readl(CCM_PLL6_MOD_CTRL);
	factor_n = ((reg_val >> 8) & 0x1f) + 1;
	factor_k = ((reg_val >> 4) & 0x03) + 1;
	pll6 = 24 * factor_n * factor_k/2;
	return pll6;
}

void pattern_goto(int pos)
{
    /* SUNXI_DEBUG("pos =%d\n",pos); */
}

u32 spi_cfg_mclk(u32 spi_no, u32 src, u32 mclk)
{
#ifdef CONFIG_FPGA
	g_cfg_mclk = 24000000;
	return g_cfg_mclk;
#else
	u32 mclk_base = CCM_SPI0_SCLK_CTRL;
	u32 source_clk;
	u32 rval;
	u32 m, n, div;

	src = 1;

	switch (src) {
	case 0:
		source_clk = 24000000;
		break;
	case 1:
		source_clk = ccm_get_pll_periph_clk()*1000000;
		break;
	default:
		SUNXI_DEBUG("Wrong SPI clock source :%x\n", src);
	}
	SUNXI_DEBUG("SPI clock source :0x%x\n", source_clk);
	div = (source_clk + mclk - 1) / mclk;
	div = (div == 0) ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		SUNXI_DEBUG("Source clock is too high\n");
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	rval = (1U << 31) | (src << 24) | (n << 16) | (m - 1);
	writel(rval, mclk_base);
	g_cfg_mclk = source_clk / (1 << n) / (m - 1);
	SUNXI_DEBUG("spi spic->sclk =0x%x\n", g_cfg_mclk);
	return g_cfg_mclk;
#endif
}

u32 spi_get_mlk(u32 spi_no)
{
	return g_cfg_mclk ; /* spicinfo[spi_no].sclk; */
}

void spi_gpio_cfg(int spi_no)
{
	uint reg_val = 0;
	uint reg_addr;

	/* PIO SETTING,SPI0_MOSI(PC2) SPI0_MISO(PC3) SPIO_CLK(PC0)  */
	reg_addr = SUNXI_PIO_BASE + 0x48;
	reg_val = readl(reg_addr);
	reg_val &= ~(0xffff);
	reg_val |= ((3 << 8) | (3 << 12) | (3 << 0) | (3 << 4));
	writel(reg_val, reg_addr);

	/* PIO SETTING,PortC SPI0 pull reg */
	reg_val = readl(SUNXI_PIO_BASE + 0x64);
	reg_val &= ~(0x03 << 6);
	reg_val |= (0x01 << 6);
	writel(reg_val, (SUNXI_PIO_BASE + 0x64));
	SUNXI_DEBUG("Reg pull reg_val=0x%x,read=0x%x\n",
			reg_val, readl((SUNXI_PIO_BASE + 0x64)));
}


void spi_onoff(u32 spi_no, u32 onoff)
{
	u32 clkid[] = {SPI0_CKID, SPI1_CKID};

	spi_no = 0;

	switch (spi_no) {
	case 0:
		spi_gpio_cfg(0);
		break;
	}
	ccm_module_reset_bak(clkid[spi_no]);
	if (onoff)
		ccm_clock_enable_bak(clkid[spi_no]);
	else
		ccm_clock_disable_bak(clkid[spi_no]);
}

void spic_set_clk(u32 spi_no, u32 clk)
{
	u32 mclk = spi_get_mlk(spi_no);
	u32 div;
	u32 cdr1 = 0;
	u32 cdr2 = 0;
	u32 cdr_sel = 0;

	div = mclk/(clk<<1);

	if (div == 0) {
		cdr1 = 0;

		cdr2 = 0;
		cdr_sel = 0;
	} else if (div <= 0x100) {
		cdr1 = 0;

		cdr2 = div-1;
		cdr_sel = 1;
	} else {
		div = 0;
		while (mclk > clk) {
			div++;
			mclk >>= 1;
		}
		cdr1 = div;

		cdr2 = 0;
		cdr_sel = 0;
	}

	writel((cdr_sel<<12)|(cdr1<<8)|cdr2, SPI_CCR);
	SUNXI_DEBUG("spic_set_clk:mclk=0x%x\n", mclk);
}

int spic_init(u32 spi_no)
{
	u32 rval;

	spi_no = 0;

	spi_onoff(spi_no, 1);
	spi_cfg_mclk(spi_no, SPI_CLK_SRC, SPI_MCLK);
	spic_set_clk(spi_no, SPI_DEFAULT_CLK);

	rval = SPI_SOFT_RST|SPI_TXPAUSE_EN|SPI_MASTER|SPI_ENABLE;
	writel(rval, SPI_GCR);
	rval = SPI_SET_SS_1|SPI_DHB|SPI_SS_ACTIVE0;
	/* set ss to high,discard unused burst */
	/* SPI select signal polarity(low,1=idle) */
	writel(rval, SPI_TCR);
	writel(SPI_TXFIFO_RST|(SPI_TX_WL<<16)|(SPI_RX_WL), SPI_FCR);

	return 0;
}

int spic_rw(u32 tcnt, void *txbuf, u32 rcnt, void *rxbuf)
{
	u32 i = 0, fcr;
	int timeout = 0xfffff;

	u8 *tx_buffer = txbuf;
	u8 *rx_buffer = rxbuf;

	writel(0, SPI_IER);
	writel(0xffffffff, SPI_ISR);/* clear status register */

	writel(tcnt, SPI_MTC);
	writel(tcnt+rcnt, SPI_MBC);
	writel(readl(SPI_TCR)|SPI_EXCHANGE, SPI_TCR);

	if (tcnt) {
		{
			i = 0;
			while (i < tcnt) {
				while ((readl(SPI_FSR)>>16) == SPI_FIFO_SIZE)
					;
				writeb(*(tx_buffer+i), SPI_TXD);
				i++;
			}
		}
#if 0
		else {
			spi_no = 0;
			writel((readl(SPI_FCR)|SPI_TXDMAREQ_EN), SPI_FCR);
			spi_dma_send_start(0, txbuf, tcnt);
			/* wait DMA finish */
			while ((timeout-- > 0) && spi_wait_dma_send_over(0))
				;
			if (timeout <= 0) {
				printf("tx wait_dma_send_over fail\n");
				return -1;
			}

			printf("timeout %d,count_send %d\n",
					timeout, count_send);
		}
#endif
	}
	timeout = 0xfffff;
	/* start transmit */
	if (rcnt) {
		{
			i = 0;
			while (i < rcnt) {
				/* receive valid data */
				while (((readl(SPI_FSR))&0x7f) == 0)
					;
				*(rx_buffer+i) = readb(SPI_RXD);
				i++;
			}
		}
#if 0
		else {
			pattern_goto(115);
			timeout = 0xfffff;
			writel((readl(SPI_FCR)|SPI_RXDMAREQ_EN), SPI_FCR);
			spi_dma_recv_start(0, rxbuf, rcnt);
			/* wait DMA finish */
			while ((timeout-- > 0) && spi_wait_dma_recv_over(0))
				;
			if (timeout <= 0) {
				printf("rx wait_dma_recv_over fail\n");
				return -1;
			}
		}
#endif
	}

	fcr = readl(SPI_FCR);
	fcr &= ~(SPI_TXDMAREQ_EN|SPI_RXDMAREQ_EN);
	writel(fcr, SPI_FCR);
	/* (1U << 11) | (1U << 10) | (1U << 9) | (1U << 8)) */
	if ((readl(SPI_ISR) & (0xf << 8)) || (timeout == 0))
		return RET_FAIL;

	if (readl(SPI_TCR)&SPI_EXCHANGE)
		printf("XCH Control Error!!\n");

	writel(0xfffff, SPI_ISR);  /* clear  flag */
	return RET_OK;

}

int spic_exit(u32 spi_no)
{
	return 0;
}

