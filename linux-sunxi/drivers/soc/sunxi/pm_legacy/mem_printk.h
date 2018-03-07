/*
 * drivers/soc/sunxi/pm_legacy/mem_printk.h
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
*                                    (c) Copyright 2012-2015,  China
*                                             All Rights Reserved
*
* File    : mem_printk.h
* By      : young
* Version : v1.0
* Date    : 2011-5-30 19:50
* Descript: intterupt bsp for platform standby.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_PRINTK_H__
#define __MEM_PRINTK_H__

#define DEBUG_BUFFER_SIZE (256)
/* other module may define printk, while its declaration may not be the same with this. */
/* so, it not proper to export this symbols to global. */
__s32 printk(const char *format, ...);
__s32 printk_nommu(const char *format, ...);

#endif /* __MEM_PRINTK_H__ */
