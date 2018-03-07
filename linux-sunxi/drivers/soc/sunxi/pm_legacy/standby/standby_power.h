/*
 * drivers/soc/sunxi/pm_legacy/standby/standby_power.h
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
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_power.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_POWER_H__
#define __STANDBY_POWER_H__

#include "standby_cfg.h"
#include "axp209_power.h"

extern int standby_set_power(unsigned int pmu_id, unsigned int bitmap,
			     unsigned int (*power_tree)[VCC_MAX_INDEX],
			     unsigned short volt);

extern int standby_recovery_power(unsigned int pmu_id);
#endif /* __STANDBY_POWER_H__ */
