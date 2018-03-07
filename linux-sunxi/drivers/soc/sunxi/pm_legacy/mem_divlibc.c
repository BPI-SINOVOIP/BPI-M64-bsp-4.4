/*
 * drivers/soc/sunxi/pm_legacy/mem_divlibc.c
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

#include "pm_i.h"

void __mem_div0(void)
{
	printk("Attempting division by 0!");
}

__u32 raw_lib_udiv(__u32 dividend, __u32 divisior)
{
	__u32 tmpDiv = (__u32) divisior;
	__u32 tmpQuot = 0;
	__s32 shift = 0;

	if (!divisior) {
		/* divide 0 error abort */
		return 0;
	}

	while (!(tmpDiv & ((__u32) 1 << 31))) {
		tmpDiv <<= 1;
		shift++;
	}

	do {
		if (dividend >= tmpDiv) {
			dividend -= tmpDiv;
			tmpQuot = (tmpQuot << 1) | 1;
		} else {
			tmpQuot = (tmpQuot << 1) | 0;
		}
		tmpDiv >>= 1;
		shift--;
	} while (shift >= 0);

	return tmpQuot;
}
