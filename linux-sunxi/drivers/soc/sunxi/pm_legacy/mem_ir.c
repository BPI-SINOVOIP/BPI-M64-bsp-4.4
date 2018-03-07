/*
 * drivers/soc/sunxi/pm_legacy/mem_ir.c
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
* File    : mem_ir.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:36
* Descript:
* Update  : date                auther      ver     notes
******************************************************************************
*/
#include  "pm_i.h"

/*
******************************************************************************
*                           INIT IR FOR MEM
*
*Description: init ir for mem;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    init ir successed;
*               EPDK_FAIL,  init ir failed;
******************************************************************************
*/
__s32 mem_ir_init(void)
{
	return 0;
}

/*
******************************************************************************
*                           EXIT IR FOR MEM
*
*Description: exit ir for mem;
*
*Arguments  : none;
*
*Return     : result.
*               EPDK_OK,    exit ir successed;
*               EPDK_FAIL,  exit ir failed;
******************************************************************************
*/
__s32 mem_ir_exit(void)
{
	return 0;
}

/*
******************************************************************************
*                           DETECT IR FOR MEM
*
*Description: detect ir for mem;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    receive some signal;
*               EPDK_FAIL,  no signal;
******************************************************************************
*/
__s32 mem_ir_detect(void)
{
	return 0;
}

/*
******************************************************************************
*                           VERIFY IR SIGNAL FOR MEM
*
*Description: verify ir signal for mem;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    valid ir signal;
*               EPDK_FAIL,  invalid ir signal;
******************************************************************************
*/
__s32 mem_ir_verify(void)
{
	return -1;
}
