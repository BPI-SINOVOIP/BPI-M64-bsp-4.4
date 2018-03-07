
/*
 * isp_platform_drv.c for isp reg config
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include "isp_platform_drv.h"
#include "sun8iw12p1/sun8iw12p1_isp_reg_cfg.h"
struct isp_platform_drv *isp_platform_curr;

int isp_platform_register(struct isp_platform_drv *isp_drv)
{
	int platform_id;
	platform_id = isp_drv->platform_id;
	isp_platform_curr = isp_drv;
	return 0;
}

int isp_platform_init(unsigned int platform_id)
{
	switch (platform_id) {
	case ISP_PLATFORM_SUN8IW12P1:
		sun8iw12p1_isp_platform_register();
		break;
	case ISP_PLATFORM_SUN8IW17P1:
		sun8iw12p1_isp_platform_register();
		break;
	default:
		sun8iw12p1_isp_platform_register();
		break;
	}
	return 0;
}

struct isp_platform_drv *isp_get_driver(void)
{
	if (NULL == isp_platform_curr)
		printk("[ISP ERR] isp platform curr have not init!\n");
	return isp_platform_curr;
}


