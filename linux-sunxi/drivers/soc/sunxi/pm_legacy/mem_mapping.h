/*
 * drivers/soc/sunxi/pm_legacy/mem_mapping.h
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
******************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, gq.yang China
*                                             All Rights Reserved
*
* File    : mem_mapping.h
* By      :
* Version : v1.0
* Date    : 2012-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
******************************************************************************
*/
#ifndef __MEM_MAPPING_H__
#define __MEM_MAPPING_H__

/*mem_mapping.c*/
extern void create_mapping(void);
extern void save_mapping(unsigned long vaddr);
extern void restore_mapping(unsigned long vaddr);

#endif /* __MEM_MAPPING_H__ */
