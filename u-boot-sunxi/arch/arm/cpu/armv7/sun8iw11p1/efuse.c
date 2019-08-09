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
#include "common.h"
#include "asm/io.h"
#include "asm/arch/sid.h"

#define SID_OP_LOCK  (0xAC)
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
static void sid_program_key(uint key_index, uint key_value)
{
	uint reg_val;

	writel(key_value, SID_PRKEY);

	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff<<16)|0x3);
	reg_val |= key_index<<16;
	writel(reg_val, SID_PRCTL);

	reg_val &= ~((0xff<<8)|0x3);
	reg_val |= (SID_OP_LOCK<<8) | 0x1;
	writel(reg_val, SID_PRCTL);

	while(readl(SID_PRCTL)&0x1){};

	reg_val &= ~((0x1ff<<16)|(0xff<<8)|0x3);
	writel(reg_val, SID_PRCTL);

	return;
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
static uint sid_read_key(uint key_index)
{
	uint reg_val;

	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff<<16)|0x3);
	reg_val |= key_index<<16;
	writel(reg_val, SID_PRCTL);

	reg_val &= ~((0xff<<8)|0x3);
	reg_val |= (SID_OP_LOCK<<8) | 0x2;
	writel(reg_val, SID_PRCTL);

	while(readl(SID_PRCTL)&0x2){};

	reg_val &= ~((0x1ff<<16)|(0xff<<8)|0x3);
	writel(reg_val, SID_PRCTL);

	reg_val = readl(SID_RDKEY);

	return reg_val;
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
void sid_write_customer_id (void)
{
	 uint reg_val;
	 int val = 0x2D;

	 reg_val = sid_read_key(EFUSE_CUSTOMER_ID);
	 if (reg_val == 0) {
		 sid_program_key(EFUSE_CUSTOMER_ID, val);
	}

	reg_val = sid_read_key(EFUSE_CUSTOMER_ID);
	if (reg_val != val) {
		printf("write customer id err0 =%d\n", reg_val);
	}

	if (reg_val == val) {
		printf("write customer id sucess =%d\n", reg_val);
	}
	return;
 }
