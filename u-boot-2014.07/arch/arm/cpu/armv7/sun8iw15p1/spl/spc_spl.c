/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
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
**********************************************************************************************************************
*/
#include <common.h>
#include <asm/io.h>
#include <asm/arch/spc.h>

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sunxi_spc_set_to_ns(uint type)
{
	writel(0x0207e1af, SPC_SET_REG(0));
	writel(0x00000007, SPC_SET_REG(2));
	writel(0x0250031d, SPC_SET_REG(4));
	writel(0x00400118, SPC_SET_REG(5));
	writel(0x00bc1703, SPC_SET_REG(6));
	writel(0x00000000, SPC_SET_REG(7));
	writel(0x0000071f, SPC_SET_REG(8));
	writel(0x00000000, SPC_SET_REG(9));
	writel(0x03fb0303, SPC_SET_REG(10));
	writel(0x000d010b, SPC_SET_REG(11));
	writel(0x042100d7, SPC_SET_REG(12));
	writel(0x00000879, SPC_SET_REG(13));
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/*void sunxi_spc_set_to_s(uint type)
{
	u8  sub_type0, sub_type1;

	sub_type0 = type & 0xff;
	sub_type1 = (type>>8) & 0xff;

	if(sub_type0)
	{
		writel(sub_type0, SPC_CLEAR_REG(0));
	}
	if(sub_type1)
	{
		writel(sub_type1, SPC_CLEAR_REG(1));
	}
}*/
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
/*uint sunxi_spc_probe_status(uint type)
{
	if(type < 8)
	{
		return (readl(SPC_STATUS_REG(0)) >> type) & 1;
	}
	if(type < 16)
	{
		return (readl(SPC_STATUS_REG(1)) >> type) & 1;
	}

	return 0;
}
*/
