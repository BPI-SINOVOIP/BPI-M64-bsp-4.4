/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <malloc.h>
#include <stdarg.h>
#include <asm/arch/dma.h>
#include <sys_config.h>
#include <smc.h>
#include <sys_config_old.h>

//#define get_wvalue(addr)	(*((volatile unsigned long  *)(addr)))
//#define put_wvalue(addr, v)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
extern __u32 SPIC_IO_BASE;
//#define SPI0_BASE   					SPIC_IO_BASE
#define SPI_TX_IO_DATA             (SPIC_IO_BASE + 0x200)
#define SPI_RX_IO_DATA             (SPIC_IO_BASE + 0x300)

//__u32 DMAC_BASE_ADDR = 0x01c02000;
//#define DMAC_STATUS_REG  		(DMAC_BASE_ADDR + 0x30)

#define  NAND_DRV_VERSION_0		0x2
#define  NAND_DRV_VERSION_1		0x08
#define  NAND_DRV_DATE			0x20190408
#define  NAND_DRV_TIME			0x1958


extern int sunxi_get_securemode(void);
__u32 NAND_Print_level(void);

__u32 get_wvalue(__u32 addr)
{
	//return (smc_readl(addr));
	return(*((volatile unsigned long  *)(addr)));
}

void put_wvalue(__u32 addr,__u32 v)
{
	//smc_writel(v, addr);
	*((volatile unsigned long  *)(addr)) = (unsigned long)(v);
}

void * NAND_Malloc(unsigned int Size);
void NAND_Free(void *pAddr, unsigned int Size);
static __u32 boot_mode;
//static __u32 gpio_hdl;

int NAND_set_boot_mode(__u32 boot)
{
	boot_mode = boot;
	return 0;
}

int NAND_Print(const char * str, ...)
{
	//if((boot_mode)||(1==NAND_Print_level()))
	//	return 0;
	//else
	{
		static char _buf[1024];
		va_list args;

		va_start(args, str);
		vsprintf(_buf, str, args);
		va_end(args);

		printf(_buf);
		return 0;
	}

}

int NAND_Print_DBG(const char * str, ...)
{
	//	if((boot_mode)||(1==NAND_Print_level()))
	//		return 0;
	//	else
	{
		static char _buf[1024];
		va_list args;

		va_start(args, str);
		vsprintf(_buf, str, args);
		va_end(args);

		printf(_buf);
		return 0;
	}

}

__s32 NAND_CleanFlushDCacheRegion(__u32 buff_addr, __u32 len)
{
	flush_cache(buff_addr, len);
	return 0;
}

__u32 NAND_DMASingleMap(__u32 rw, __u32 buff_addr, __u32 len)
{
	return buff_addr;
}

__u32 NAND_DMASingleUnmap(__u32 rw, __u32 buff_addr, __u32 len)
{
	return buff_addr;
}

__s32 NAND_AllocMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{
	void *p = NULL;

#if 0

	__u32 physical_addr  = 0;
	//void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *handle, gfp_t gfp);
	p = (void *)dma_alloc_coherent(NULL, PAGE_SIZE,
			(dma_addr_t *)&physical_addr, GFP_KERNEL);

	if (p == NULL) {
		printf("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = physical_addr;
		printf("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
				*cpu_addr, *phy_addr);
	}

#else

	p = (void *)NAND_Malloc(1024);

	if (p == NULL) {
		printf("NAND_AllocMemoryForDMADescs(): alloc dma des failed\n");
		return -1;
	} else {
		*cpu_addr = (__u32)p;
		*phy_addr = (__u32)p;
		printf("NAND_AllocMemoryForDMADescs(): cpu: 0x%x    physic: 0x%x\n",
				*cpu_addr, *phy_addr);
	}

#endif

	return 0;
}

__s32 NAND_FreeMemoryForDMADescs(__u32 *cpu_addr, __u32 *phy_addr)
{

#if 0

	//void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle);
	dma_free_coherent(NULL, PAGE_SIZE, (void *)(*cpu_addr), *phy_addr);

#else

	NAND_Free((void *)(*cpu_addr), 1024);

#endif

	*cpu_addr = 0;
	*phy_addr = 0;

	return 0;
}

__u32 _Getpll6Clk(void)
{
	__u32 reg_val;
	__u32 factor_n;
	__u32 factor_k;
	//__u32 div_m;
	__u32 clock;

	reg_val  = *(volatile __u32 *)(0x01c20000 + 0x28);
	factor_n = ((reg_val >> 8) & 0x1f) + 1;
	factor_k = ((reg_val >> 4) & 0x3) + 1;
	//div_m = ((reg_val >> 0) & 0x3) + 1;

	clock = 24000000 * factor_n * factor_k/2;
	//NAND_Print("pll6 clock is %d Hz\n", clock);
	//if(clock != 600000000)
	//printf("pll6 clock rate error, %d!!!!!!!\n", clock);

	return clock;
}

__s32 _get_spic_clk(__u32 nand_index, __u32 *pdclk)
{
	__u32 sclk0_reg_adr = 0;
	__u32 sclk_src, sclk_src_sel;
	__u32 sclk_pre_ratio_n, sclk_ratio_m;
	__u32 reg_val, sclk0;

	if (nand_index > 1) {
		printf("wrong nand id: %d\n", nand_index);
		return -1;
	}

	if (nand_index == 0) {
		sclk0_reg_adr = (0x01c20000 + 0xa0); //CCM_SPI0_CLK0_REG;
	}

	// sclk0
	reg_val = get_wvalue(sclk0_reg_adr);
	sclk_src_sel     = (reg_val>>24) & 0x3;
	sclk_pre_ratio_n = (reg_val>>16) & 0x3;;
	sclk_ratio_m     = (reg_val) & 0xf;
	if (sclk_src_sel == 0)
		sclk_src = 24;
	else
		sclk_src = _Getpll6Clk()/1000000;
	sclk0 = (sclk_src >> sclk_pre_ratio_n) / (sclk_ratio_m+1);

	if (nand_index == 0) {
		//NAND_Print("Reg 0x01c20080: 0x%x\n", get_wvalue(0x01c20080));
	} else {
		//NAND_Print("Reg 0x01c20084: 0x%x\n", get_wvalue(0x01c20084));
	}
	//NAND_Print("NDFC%d:  sclk0(2*dclk): %d MHz\n", nand_index, sclk0);

	*pdclk = sclk0/2;

	return 0;
}

__s32 _change_spic_clk(__u32 nand_index, __u32 dclk_src_sel, __u32 dclk)
{
	u32 reg_val;
	u32 sclk0_src_sel, sclk0, sclk0_src, sclk0_pre_ratio_n, sclk0_src_t, sclk0_ratio_m;
	u32 sclk0_reg_adr;

	if (nand_index == 0) {
		sclk0_reg_adr = (0x01c20000 + 0xa0); //CCM_SPI0_CLK0_REG;
	}  else {
		printf("_change_ndfc_clk error, wrong nand index: %d\n", nand_index);
		return -1;
	}

	/*close dclk and cclk*/
	if (dclk == 0)
	{
		reg_val = get_wvalue(sclk0_reg_adr);
		reg_val &= (~(0x1U<<31));
		put_wvalue(sclk0_reg_adr, reg_val);

		//printf("_change_ndfc_clk, close sclk0 and sclk1\n");
		return 0;
	}

	sclk0_src_sel = dclk_src_sel;
	sclk0 = dclk*2; //set sclk0 to 2*dclk.

	if(sclk0_src_sel == 0x0) {
		//osc pll
		sclk0_src = 24;
	} else {
		//pll6 for ndfc version 1
		sclk0_src = _Getpll6Clk()/1000000;
	}

	//////////////////// sclk0: 2*dclk
	//sclk0_pre_ratio_n
	sclk0_pre_ratio_n = 3;
	if(sclk0_src > 4*16*sclk0)
		sclk0_pre_ratio_n = 3;
	else if (sclk0_src > 2*16*sclk0)
		sclk0_pre_ratio_n = 2;
	else if (sclk0_src > 1*16*sclk0)
		sclk0_pre_ratio_n = 1;
	else
		sclk0_pre_ratio_n = 0;

	sclk0_src_t = sclk0_src>>sclk0_pre_ratio_n;

	//sclk0_ratio_m
	sclk0_ratio_m = (sclk0_src_t/(sclk0)) - 1;
	if( sclk0_src_t%(sclk0) )
		sclk0_ratio_m +=1;

	/////////////////////////////// close clock
	reg_val = get_wvalue(sclk0_reg_adr);
	reg_val &= (~(0x1U<<31));
	put_wvalue(sclk0_reg_adr, reg_val);

	///////////////////////////////configure
	//sclk0 <--> 2*dclk
	reg_val = get_wvalue(sclk0_reg_adr);
	//clock source select
	reg_val &= (~(0x3<<24));
	reg_val |= (sclk0_src_sel&0x3)<<24;
	//clock pre-divide ratio(N)
	reg_val &= (~(0x3<<16));
	reg_val |= (sclk0_pre_ratio_n&0x3)<<16;
	//clock divide ratio(M)
	reg_val &= ~(0xf<<0);
	reg_val |= (sclk0_ratio_m&0xf)<<0;
	put_wvalue(sclk0_reg_adr, reg_val);

	/////////////////////////////// open clock
	reg_val = get_wvalue(sclk0_reg_adr);
	reg_val |= 0x1U<<31;
	put_wvalue(sclk0_reg_adr, reg_val);

	//NAND_Print("NAND_SetClk for nand index %d \n", nand_index);
	if (nand_index == 0) {
		//NAND_Print("Reg 0x01c20080: 0x%x\n", get_wvalue(0x01c20080));
	} else {
		//NAND_Print("Reg 0x01c20084: 0x%x\n", get_wvalue(0x01c20084));
	}

	return 0;
}

__s32 _close_spic_clk(__u32 nand_index)
{
	u32 reg_val;
	u32 sclk0_reg_adr;

	if (nand_index == 0) {
		sclk0_reg_adr = (0x01c20000 + 0xa0); //CCM_SPI0_CLK0_REG;

	} else {
		printf("close_ndfc_clk error, wrong nand index: %d\n", nand_index);
		return -1;
	}

	/*close dclk and cclk*/
	reg_val = get_wvalue(sclk0_reg_adr);
	reg_val &= (~(0x1U<<31));
	put_wvalue(sclk0_reg_adr, reg_val);

	reg_val = get_wvalue(sclk0_reg_adr);
	//printf("ndfc clk release,sclk reg adr %x:%x\n",sclk0_reg_adr,reg_val);

	//printf(" close sclk0 and sclk1\n");
	return 0;
}


__s32 _open_ahb_gate_and_reset(__u32 nand_index)
{
	__u32 cfg=0;

	/*
	   1. release ahb reset and open ahb clock gate for spic and gdmac.
	   */
	if (nand_index == 0) {
		//reset
		cfg = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		cfg &= (~((0x1<<20)|(0x1<<6)));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = cfg;

		cfg = *(volatile __u32 *)(0x01c20000 + 0x2c0);
		cfg |= ((0x1<<20)|(0x1<<6));
		*(volatile __u32 *)(0x01c20000 + 0x2c0) = cfg;

		//open ahb
		cfg = *(volatile __u32 *)(0x01c20000 + 0x60);
		cfg |= ((0x1<<20)|(0x1<<6));
		*(volatile __u32 *)(0x01c20000 + 0x60) = cfg;


	} else {
		printf("open ahb gate and reset, wrong nand index: %d\n", nand_index);
		return -1;
	}

	return 0;
}

__s32 _close_spic_ahb_gate_and_reset(__u32 nand_index)
{
	__u32 reg_val=0;

	/*
	   1. release ahb reset and open ahb clock gate for spic.
	   */
	if (nand_index == 0) {
		// reset
		reg_val = get_wvalue(0x01c20000 + 0x2c0);
		reg_val &= (~(0x1U<<20));
		//		*(volatile __u32 *)(0x01c20000 + 0x2c0) = reg_val;
		put_wvalue((0x01c20000 + 0x2c0),reg_val);

		// ahb clock gate
		reg_val = get_wvalue(0x01c20000 + 0x60);
		reg_val &= (~(0x1U<<20));
		//		*(volatile __u32 *)(0x01c20000 + 0x60) = reg_val;
		put_wvalue((0x01c20000 + 0x60),reg_val);
	} else {
		printf("close ahb gate and reset, wrong nand index: %d\n", nand_index);
		return -1;
	}
	reg_val = get_wvalue(0x01c20000 + 0x2c0);
	//	printf("0x01c202c0:%x\n",reg_val);
	reg_val = get_wvalue(0x01c20000 + 0x60);
	//	printf("0x01c20060:%x\n",reg_val);

	return 0;
}


__s32 _cfg_spic_gpio(__u32 nand_index)
{
	if(0 == nand_index)
	{
		*(volatile __u32 *)(0x01c20800 + 0x48) = 0x3333;
		*(volatile __u32 *)(0x01c20800 + 0x5c) = 0x55;
		*(volatile __u32 *)(0x01c20800 + 0x64) = 0x10;
		NAND_Print("Reg 0x01c20848: 0x%x\n", *(volatile __u32 *)(0x01c20848));
		NAND_Print("Reg 0x01c2085c: 0x%x\n", *(volatile __u32 *)(0x01c2085c));
		NAND_Print("Reg 0x01c20864: 0x%x\n", *(volatile __u32 *)(0x01c20864));
	}
	else
	{

	}

	return 0;
}

int NAND_ClkRequest(__u32 spi_no)
{
	__s32 ret = 0;

	// 1. release ahb reset and open ahb clock gate
	_open_ahb_gate_and_reset(spi_no);

	// 2. configure clk
	ret = _change_spic_clk(spi_no, 1, 20);
	if (ret<0) {
		printf("NAND_ClkRequest, set dclk failed!\n");
		return -1;
	}

	return 0;
}

void NAND_ClkRelease(__u32 nand_index)
{
	_close_spic_clk(nand_index);

	_close_spic_ahb_gate_and_reset(nand_index);

	return;

}

/*
 **********************************************************************************************************************
 *
 *             NAND_GetCmuClk
 *
 *  Description:
 *
 *
 *  Parameters:
 *
 *
 *  Return value:
 *
 *
 **********************************************************************************************************************
 */
int NAND_SetClk(__u32 nand_index, __u32 nand_clk)
{
	__u32 dclk_src_sel, dclk;
	__s32 ret = 0;

	////////////////////////////////////////////////
	dclk_src_sel = 1;
	dclk = nand_clk;
	////////////////////////////////////////////////
	ret = _change_spic_clk(nand_index, dclk_src_sel, dclk);
	if (ret < 0) {
		printf("NAND_SetClk, change ndfc clock failed\n");
		return -1;
	}

	return 0;
}

int NAND_GetClk(__u32 nand_index, __u32 *pnand_clk)
{
	__s32 ret;

	//	NAND_Print("NAND_GetClk for nand index %d \n", nand_index);
	ret = _get_spic_clk(nand_index, pnand_clk);
	if (ret < 0) {
		printf("NAND_GetClk, failed!\n");
		return -1;
	}

	return 0;
}

void NAND_PIORequest(__u32 spi_no)
{
#if 1
	__s32 ret = 0;

	ret = _cfg_spic_gpio(spi_no);
	if (ret < 0) {
		printf("NAND_PIORequest, failed!\n");
		return;
	}

#else
	gpio_hdl = gpio_request_ex("spi0", NULL);
	if(!gpio_hdl)
		printf("NAND_PIORequest, failed!\n");
#endif
	return;
}


void NAND_PIORelease(__u32 spi_no)
{

	return;
}

void NAND_Memset(void* pAddr, unsigned char value, unsigned int len)
{
	memset(pAddr, value, len);
}

void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len)
{
	memcpy(pAddr_dst, pAddr_src, len);
}

#if 0
#define NAND_MEM_BASE  0x59000000

void * NAND_Malloc(unsigned int Size)
{
	__u32 mem_addr;

	mem_addr = NAND_MEM_BASE+malloc_size;

	malloc_size += Size;
	if(malloc_size%4)
		malloc_size += (4-(malloc_size%4));

	//NAND_Print("NAND_Malloc: 0x%x\n", NAND_MEM_BASE + malloc_size);

	if(malloc_size>0x4000000)
		return NULL;
	else
		return (void *)mem_addr;
}

void NAND_Free(void *pAddr, unsigned int Size)
{
	//free(pAddr);
}

#else
void * NAND_Malloc(unsigned int Size)
{
	return (void *)malloc(Size);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
	free(pAddr);
}
#endif




void  OSAL_IrqUnLock(unsigned int  p)
{
	;
}
void  OSAL_IrqLock  (unsigned int *p)
{
	;
}

int NAND_WaitRbReady(void)
{
	return 0;
}

void *NAND_IORemap(unsigned int base_addr, unsigned int size)
{
	return (void *)base_addr;
}

__u32 NAND_VA_TO_PA(__u32 buff_addr)
{
	return buff_addr;
}

__u32 NAND_GetIOBaseAddr(void)
{
	return 0x01c68000;
}

__u32 NAND_GetNdfcDmaMode(void)
{
	/*
0: General DMA;
1: MBUS DMA

Only support General DMA!!!!
*/
	return 0;
}

int NAND_PhysicLockInit(void)
{
	return 0;
}

int NAND_PhysicLock(void)
{
	return 0;
}

int NAND_PhysicUnLock(void)
{
	return 0;
}

int NAND_PhysicLockExit(void)
{
	return 0;
}

__u32 NAND_GetMaxChannelCnt(void)
{
	return 1;
}

__u32 NAND_GetPlatform(void)
{
	return 1681;
}

unsigned int tx_dma_chan = 0;
unsigned int rx_dma_chan = 0;


/* request dma channel and set callback function */
int nand_request_tx_dma(void)
{
	tx_dma_chan = sunxi_dma_request(0);
	if (tx_dma_chan == 0) {
		printf("uboot nand_request_tx_dma: request genernal dma failed!\n");
		return -1;
	} else
		NAND_Print("uboot nand_request_tx_dma: reqest genernal dma for nand success, 0x%x\n", tx_dma_chan);

	return 0;
}
int nand_request_rx_dma(void)
{
	rx_dma_chan = sunxi_dma_request(0);
	if (rx_dma_chan == 0) {
		printf("uboot nand_request_tx_dma: request genernal dma failed!\n");
		return -1;
	} else
		NAND_Print("uboot nand_request_tx_dma: reqest genernal dma for nand success, 0x%x\n", rx_dma_chan);

	return 0;
}

int NAND_ReleaseTxDMA(void)
{
	printf("nand release dma:%x\n",tx_dma_chan);
	if(tx_dma_chan)
	{
		sunxi_dma_release(tx_dma_chan);
		tx_dma_chan = 0;
	}
	return 0;
}
int NAND_ReleaseRxDMA(void)
{
	printf("nand release dma:%x\n",tx_dma_chan);
	if(rx_dma_chan)
	{
		sunxi_dma_release(rx_dma_chan);
		rx_dma_chan = 0;
	}
	return 0;
}

int nand_dma_config_start(__u32 tx_mode, dma_addr_t addr,__u32 length)
{
	int ret = 0;
	sunxi_dma_setting_t dma_set;

	dma_set.loop_mode = 0;
	dma_set.wait_cyc = 8;
	dma_set.data_block_size = 0;

	if (tx_mode) {
		dma_set.cfg.src_drq_type = DMAC_CFG_SRC_TYPE_DRAM;
		dma_set.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
		dma_set.cfg.src_burst_length = DMAC_CFG_SRC_8_BURST; //8
		dma_set.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_32BIT;
		dma_set.cfg.reserved0 = 0;

		dma_set.cfg.dst_drq_type = DMAC_CFG_TYPE_SPI0;
		dma_set.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_IO_MODE;
		dma_set.cfg.dst_burst_length = DMAC_CFG_DEST_8_BURST; //8
		dma_set.cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_32BIT;
		dma_set.cfg.reserved1 = 0;
	} else {
		dma_set.cfg.src_drq_type = DMAC_CFG_TYPE_SPI0;
		dma_set.cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;
		dma_set.cfg.src_burst_length = DMAC_CFG_SRC_8_BURST; //8
		dma_set.cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_32BIT;
		dma_set.cfg.reserved0 = 0;

		dma_set.cfg.dst_drq_type = DMAC_CFG_DEST_TYPE_DRAM;
		dma_set.cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
		dma_set.cfg.dst_burst_length = DMAC_CFG_DEST_8_BURST; //8
		dma_set.cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_32BIT;
		dma_set.cfg.reserved1 = 0;
	}

	if (tx_mode){
		if ( sunxi_dma_setting(tx_dma_chan, &dma_set) < 0) {
			printf("uboot: sunxi_dma_setting for tx faild!\n");
			return -1;
		}
	}
	else{
		if ( sunxi_dma_setting(rx_dma_chan, &dma_set) < 0) {
			printf("uboot: sunxi_dma_setting for rx faild!\n");
			return -1;
		}
	}

	if (tx_mode)
	{
		flush_cache((uint)addr, length);
		ret = sunxi_dma_start(tx_dma_chan, addr, (__u32)SPI_TX_IO_DATA, length);
	}
	else
	{
		flush_cache((uint)addr, length);
		ret = sunxi_dma_start(rx_dma_chan, (__u32)SPI_RX_IO_DATA, addr, length);
	}
	if (ret < 0) {
		printf("uboot: sunxi_dma_start for spi nand faild!\n");
		return -1;
	}

	return 0;
}
int NAND_WaitDmaFinish(__u32 tx_flag, __u32 rx_flag)
{
	__u32 timeout = 0xffffff;

	if(tx_flag)
	{
		timeout = 0xffffff;
		while(sunxi_dma_querystatus(tx_dma_chan))
		{
			timeout--;
			if (!timeout)
				break;
		}

		if(timeout <= 0)
		{
			NAND_Print("TX DMA wait status timeout!\n");
			return -1;
		}
	}

	if(rx_flag)
	{
		timeout = 0xffffff;
		while(sunxi_dma_querystatus(rx_dma_chan))
		{
			timeout--;
			if (!timeout)
				break;
		}

		if(timeout <= 0)
		{
			NAND_Print("RX DMA wait status timeout!\n");
			return -1;
		}
	}
	return 0;
}

int Nand_Dma_End(__u32 rw, __u32 addr,__u32 length)
{

	return 0;
}

__u32 NAND_GetNandExtPara(__u32 para_num)
{
	int nand_para;
	script_parser_value_type_t ret;

	if (para_num == 0) {
		ret = script_parser_fetch("spinand", "spinand_p0", &nand_para, 1);
		if(ret!=SCRIPT_PARSER_OK)
		{
			printf("NAND_GetNandExtPara: get spinand_p0 fail, %x\n", nand_para);
			return 0xffffffff;
		}
		else
			return nand_para;
	} else if (para_num == 1) {
		ret = script_parser_fetch("spinand", "spinand_p1", &nand_para, 1);
		if(ret!=SCRIPT_PARSER_OK)
		{
			printf("NAND_GetNandExtPara: get spinand_p1 fail, %x\n", nand_para);
			return 0xffffffff;
		}
		else
			return nand_para;
	} else {
		printf("NAND_GetNandExtPara: wrong para num: %d\n", para_num);
		return 0xffffffff;
	}
}

__u32 NAND_GetNandIDNumCtrl(void)
{
	int id_number_ctl;
	script_parser_value_type_t ret;

	ret = script_parser_fetch("spinand", "spinand_id_number_ctl", &id_number_ctl, 1);
	if(ret!=SCRIPT_PARSER_OK) {
		printf("nand : get id_number_ctl fail, %x\n",id_number_ctl);
		return 0x0;
	} else {
		printf("nand : get id_number_ctl from script, %x\n",id_number_ctl);
		return id_number_ctl;
	}
}

void nand_cond_resched(void)
{
	//	return 0;
}

__u32 NAND_GetNandCapacityLevel(void)
{
	int CapacityLevel;
	script_parser_value_type_t ret;

	ret = script_parser_fetch("spinand", "spinand_capacity_level", &CapacityLevel, 1);
	if(ret!=SCRIPT_PARSER_OK) {
		printf("nand : get CapacityLevel fail, %x\n",CapacityLevel);
		return 0x0;
	} else {
		printf("nand : get CapacityLevel from script, %x\n",CapacityLevel);
		return CapacityLevel;
	}
}

__u32 NAND_Print_level(void)
{
	int print_level;
	script_parser_value_type_t ret;

	ret = script_parser_fetch("spinand", "spinand_print_level", &print_level, 1);
	if(ret!=SCRIPT_PARSER_OK) {
		//printf("nand : get print_Level fail, %x\n",print_level);
		return 0x0;
	} else {
		//printf("nand : get print_Level from script, %x\n",print_level);
		return print_level;
	}
}


static void dumphex32(char *name, char *base, int len)
{
	__u32 i;

	if ((unsigned int)base&0xf0000000) {
		printf("dumphex32: err para in uboot, %s 0x%x\n", name, (unsigned int)base);
		return ;
	}

	printf("dump %s registers:", name);
	for (i=0; i<len*4; i+=4) {
		if (!(i&0xf))
			printf("\n0x%p : ", base + i);
		printf("0x%08x ", *((volatile unsigned int *)(base + i)));
	}
	printf("\n");
}

void NAND_DumpReg(void)
{
	dumphex32("nand0 reg", (char*)0x01c68000, 40);
	dumphex32("gpio reg", (char*)0x01c20848, 40);
	dumphex32("clk reg", (char*)0x01c200a0, 8);
	dumphex32("dma reg part0", (char*)0x01c02000, 8);
	dumphex32("dma reg part1", (char*)0x01c02100, 50);
}

void NAND_ShowEnv(__u32 type, char *name, __u32 len, __u32 *val)
{
	int i;

	if (len && (val==NULL)) {
		printf("uboot:NAND_ShowEnv, para error!\n");
		return ;
	}

	if (type == 0)
	{
		printf("uboot:%s: ", name);
		for (i=0; i<len; i++)
		{
			if (i && (i%8==0))
				printf("\n");
			printf("%x ", val[i]);
		}
		printf("\n");
	}
	else
	{
		printf("uboot:NAND_ShowEnv, type error, %d!\n", type);
	}

	return ;
}

void NAND_Print_Version(void)
{
	__u32 val[4] = {0};

	val[0] = NAND_DRV_VERSION_0;
	val[1] = NAND_DRV_VERSION_1;
	val[2] = NAND_DRV_DATE;
	val[3] = NAND_DRV_TIME;
	NAND_ShowEnv(0, "nand version", 4, val);

}

int NAND_GetVoltage(void)
{



	return 0;


}

int NAND_ReleaseVoltage(void)
{
	int ret = 0;


	return ret;

}

//int sunxi_get_securemode(void)
//return 0:normal mode
//rerurn 1:secure mode ,but could change clock
//return !0 && !1: secure mode ,and couldn't change clock=
#if 0
int NAND_IS_Secure_sys(void)
{
#if 1
	int mode=0;

	mode = sunxi_get_securemode();
	if(mode==0) //normal mode,could change clock
		return 0;
	else if(mode==1)//secure boot,could change clock
		return 1;
	else
		return -1;//secure mode,can't change clock
#else
	return 0;
#endif
}
#endif
