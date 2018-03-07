/*
 * drivers/soc/sunxi/pm_legacy/mem_usb.c
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
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_usb.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 15:18
* Descript:
* Update  : date                auther      ver     notes
*****************************************************************************
*/
#include "pm_i.h"

/* ================================================================== */
/*  USB CHECK FOR WAKEUP SYSTEM FROM MEM */
/* ================================================================== */

/*
*************************************************************************
*                                     mem_usb_init
*
* Description: init usb for mem.
*
* Arguments  : none;
*
* Returns    : none;
*************************************************************************
*/
__s32 mem_usb_init(void)
{
	return 0;
}

/*
**************************************************************************
*                                     mem_usb_exit
*
* Description: exit usb for mem.
*
* Arguments  : none;
*
* Returns    : none;
**************************************************************************
*/
__s32 mem_usb_exit(void)
{
	return 0;
}

/*
***************************************************************************
*                           mem_is_usb_status_change
*
*Description: check if usb status is change.
*
*Arguments  : port  usb port number;
*
*Return     : result, 0 status not change, !0 status changed;
*
*Notes      :
*
****************************************************************************
*/
__s32 mem_is_usb_status_change(__u32 port)
{
	return 0;
}

/*
****************************************************************************
*                                     mem_query_usb_event
*
* Description: query usb event for wakeup system from mem.
*
* Arguments  : none;
*
* Returns    : result;
*               EPDK_TRUE,  some usb event happenned;
*               EPDK_FALSE, none usb event;
*************************************************************************
*/
__s32 mem_query_usb_event(void)
{
	return -1;
}
