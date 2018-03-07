/*
 * drivers/soc/sunxi/pm_legacy/standby/common.c
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
*******************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : common.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 19:38
* Descript: common lib for standby
* Update  : date                auther      ver     notes
******************************************************************************
*/
#include "standby_i.h"

/*
******************************************************************************
*                           standby_memcpy
*
*Description: memory copy function for standby.
*
*Arguments  :
*
*Return     :
*
*Notes      :
*
******************************************************************************
*/
void standby_memcpy(void *dest, void *src, int n)
{
	char *tmp_src = (char *)src;
	char *tmp_dst = (char *)dest;

	if (!dest || !src) {
		/* parameter is invalid */
		return;
	}

	for (; n > 0; n--) {
		*tmp_dst++ = *tmp_src++;
	}

	return;
}
