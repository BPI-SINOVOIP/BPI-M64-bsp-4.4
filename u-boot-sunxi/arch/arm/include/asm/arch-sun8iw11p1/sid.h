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

#ifndef __EFUSE_H__
#define __EFUSE_H__

#include "asm/arch/platform.h"

#define SID_PRCTL				(SUNXI_SID_BASE + 0x40)
#define SID_PRKEY				(SUNXI_SID_BASE + 0x50)
#define SID_RDKEY				(SUNXI_SID_BASE + 0x60)
#define SJTAG_AT0				(SUNXI_SID_BASE + 0x80)
#define SJTAG_AT1				(SUNXI_SID_BASE + 0x84)
#define SJTAG_S					(SUNXI_SID_BASE + 0x88)
#define SID_RF(n)               (SUNXI_SID_BASE + (n) * 4 + 0x80)

#define SID_EFUSE               (SUNXI_SID_BASE + 0x200)
#define SID_OP_LOCK  (0xAC)

#define EFUSE_CHIPD             (0x00)
#define EFUSE_HUK       		(0x10)
#define EFUSE_SSK               (0x30)
#define EFUSE_THERMAL_SENSOR    (0x40)
#define EFUSE_FT_ZONE    		(0x44)
#define EFUSE_TVOUT    			(0x4C)
#define EFUSE_RSSK              (0x5C)
#define EFUSE_HDCP_HASH         (0x7C)
#define EFUSE_CHIPCONFIG        (0x8C)
#define EFUSE_CUSTOMER_ID      	(0x90)

#define CUSTOMER_ID_BIT_SIZE	(112)


extern void sid_write_customer_id(void);

#endif    /*  #ifndef __EFUSE_H__  */
