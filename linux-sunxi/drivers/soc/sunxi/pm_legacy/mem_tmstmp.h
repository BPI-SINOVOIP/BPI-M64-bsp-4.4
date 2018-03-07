/*
 * drivers/soc/sunxi/pm_legacy/mem_tmstmp.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2011-2015, gq.yang China
*                                             All Rights Reserved
*
* File    : mem_tmstmp.h
* By      : gq.yang
* Version : v1.0
* Date    : 2012-11-31 15:23
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_TMSTMP_H__
#define __MEM_TMSTMP_H__

typedef struct __MEM_TMSTMP_REG {
	/*  offset:0x00 */
	volatile __u32 Ctl;
	volatile __u32 reserved0[7];
	/*  offset:0x20 */
	volatile __u32 Cluster0CtrlReg1;

} __mem_tmstmp_reg_t;

/* for super standby; */
__s32 mem_tmstmp_save(__mem_tmstmp_reg_t *ptmstmp_state);
__s32 mem_tmstmp_restore(__mem_tmstmp_reg_t *ptmstmp_state);

#endif /* __MEM_TMSTMP_H__ */
