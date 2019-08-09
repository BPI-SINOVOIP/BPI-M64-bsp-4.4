/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
/*
**********************************************************************************************************************
*
*	 the Embedded Secure Bootloader System
*
*
*	 Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*        All Rights Reserved
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
#define SID_TIMING_REG			(SUNXI_SID_BASE + 0x90)
#define SID_DBG_REG				(SUNXI_SID_BASE + 0x94)

#define SID_EFUSE               (SUNXI_SID_BASE + 0x200)
#define SID_SECURE_MODE         (SUNXI_SID_BASE + 0xA0)

#define SID_OP_LOCK              (0xAC)

#define EFUSE_CHIPD			(0x00)/* 128 bits */
#define EFUSE_BROM_CONFIG		(0x10)/* 16 bits config, 16 bits try */
#define EFUSE_THERMAL_SENSOR		(0x14)/* 64 bits */
#define EFUSE_TF_ZONE			(0x1C)/* 128 bits */
#define EFUSE_OEM_PROGRAM		(0x2C)/* 128bits :tvout 32+reserv 96 */

#define EFUSE_WRITE_PROTECT		(0x30)/* 32 bits */
#define EFUSE_READ_PROTECT		(0x34)/* 32 bits */
#define EFUSE_LCJS			(0x38)/* 32 bits */
#define EFUSE_ATTR			(0x3C)/* 32 bits */
#define EFUSE_IN			(0x50)/* 192 bits */
#define EFUSE_HUK			(0x50)/* 192 bits */
#define EFUSE_INDENTIFICATION		(0x68)/* 32 bits */
#define EFUSE_ID			(0x6C)/* 32 bits */
#define EFUSE_ROTPK			(0x40)/* 256 bits */
#define EFUSE_SSK			(0x60)/* 128 bits */
#define EFUSE_RSSK			(0xA0)/* 256 bits */
#define EFUSE_HDCP_HASH         (0xC0) /* 0xC0-0xCf, 128bits */
#define EFUSE_EK_HASH			(0xD0)/* 128 bits */
#define EFUSE_SN			(0xE0)/* 192 bits */
#define EFUSE_NV1			(0x74)/* 32 bits */
#define EFUSE_NV2			(0x78)/* 224 bits */
#define EFUSE_TCON_PRAM         (0x118) /* 0x118-0x11B,32bits */
#define EFUSE_BACKUP_KEY        (0x11C)/* 0x11C-0x13F,288bits */

/* size (bit)*/
#define SID_CHIPID_SIZE			(128)
#define SID_OEM_PROGRAM_SIZE	(32)
#define SID_NV1_SIZE			(32)
#define SID_NV2_SIZE			(32)
#define SID_THERMAL_SIZE		(64)
#define SID_IN_SIZE			    (0)
#define SID_IDENTIFY_SIZE		(0)
#define SID_ID_SIZE			    (0)
#define SID_ROTPK_SIZE			(256)
#define SID_SSK_SIZE			(128)
#define SID_RSSK_SIZE			(0)
#define SID_EK_HASH_SIZE		(0)
#define SID_SN_SIZE			    (0)


extern void sid_set_security_mode(void);
extern int  sid_probe_security_mode(void);

#endif    /*  #ifndef __EFUSE_H__  */
