/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef	__dram_head_h__
#define	__dram_head_h__

typedef struct __DRAM_PARA {
	/*normal configuration*/
	unsigned int		dram_clk;
	unsigned int		dram_type;  /*dram_type			DDR2: 2				DDR3: 3		LPDDR2: 6	LPDDR3: 7	DDR3L: 31*/
	unsigned int		dram_dx_odt;
	unsigned int		dram_dx_dri;
	unsigned int		dram_ca_dri;
	unsigned int		dram_odt_en;
	/*control configuration*/
	unsigned int		dram_para1;
	unsigned int		dram_para2;
	/*timing configuration*/
	unsigned int		dram_mr0;
	unsigned int		dram_mr1;
	unsigned int		dram_mr2;
	unsigned int		dram_mr3;
	unsigned int		dram_mr4;
	unsigned int		dram_mr5;
	unsigned int		dram_mr6;
	unsigned int		dram_mr11;
	unsigned int		dram_mr12;
	unsigned int		dram_mr13;
	unsigned int		dram_mr14;
	unsigned int		dram_mr16;
	unsigned int		dram_mr17;
	unsigned int		dram_mr22;
	unsigned int		dram_tpr0;
	unsigned int		dram_tpr1;
	unsigned int		dram_tpr2;
	unsigned int		dram_tpr3;
	unsigned int		dram_tpr6;
	unsigned int		dram_tpr10;
	unsigned int		dram_tpr11;
	unsigned int		dram_tpr12;
	unsigned int		dram_tpr13;
	unsigned int		dram_tpr14;
} __dram_para_t;

#ifdef FPGA_PLATFORM
unsigned int mctl_init(void *para);
#else
extern int init_DRAM(int type, __dram_para_t *buff);
#endif
extern int update_fdt_dram_para(void *dtb_base);
#endif


