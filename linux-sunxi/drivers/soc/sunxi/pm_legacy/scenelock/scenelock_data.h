/*
 * drivers/soc/sunxi/pm_legacy/scenelock/scenelock_data.h
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


#ifndef _LINUX_SCENELOCK_DATA_H
#define _LINUX_SCENELOCK_DATA_H

#include <linux/power/axp_depend.h>
#ifdef CONFIG_ARCH_SUN50IW1P1
#include "scenelock_data_sun50iw1p1.h"
#elif defined(CONFIG_ARCH_SUN50IW2P1)
#include "scenelock_data_sun50iw2p1.h"
#elif defined(CONFIG_ARCH_SUN50IW3P1)
#include "scenelock_data_sun50iw3p1.h"
#elif defined(CONFIG_ARCH_SUN50IW6P1)
#include "scenelock_data_sun50iw6p1.h"
#elif defined(CONFIG_ARCH_SUN8IW5P1)
#include "scenelock_data_sun8iw5p1.h"
#elif defined(CONFIG_ARCH_SUN8IW6P1)
#include "scenelock_data_sun8iw6p1.h"
#elif defined(CONFIG_ARCH_SUN8IW7P1)
#include "scenelock_data_sun8iw7p1.h"
#elif defined(CONFIG_ARCH_SUN8IW8P1)
#include "scenelock_data_sun8iw8p1.h"
#elif defined(CONFIG_ARCH_SUN8IW10P1)
#include "scenelock_data_sun8iw10p1.h"
#elif defined(CONFIG_ARCH_SUN8IW11P1)
#include "scenelock_data_sun8iw11p1.h"
#elif defined(CONFIG_ARCH_SUN8IW17P1)
#include "scenelock_data_sun8iw17p1.h"
#elif defined(CONFIG_ARCH_SUN9IW1P1)
#include "scenelock_data_sun9iw1p1.h"
#endif

int extended_standby_cnt = sizeof(extended_standby)/sizeof(extended_standby[0]);

#endif
