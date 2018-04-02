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
	writel(0x0016EAAF, SPC_SET_REG(0));
	writel(0x00000007, SPC_SET_REG(2));
	writel(0x00000001, SPC_SET_REG(3));
    writel(0xDBD5033D, SPC_SET_REG(4));
    writel(0x0040113F, SPC_SET_REG(5));
    writel(0x007D0503, SPC_SET_REG(6));
    writel(0x00323F1F, SPC_SET_REG(8));
    writel(0x0BFB0303, SPC_SET_REG(10));
    writel(0x000F011F, SPC_SET_REG(11));
    writel(0x05FF2ED5, SPC_SET_REG(12));
    writel(0x0000187C, SPC_SET_REG(13));
}

