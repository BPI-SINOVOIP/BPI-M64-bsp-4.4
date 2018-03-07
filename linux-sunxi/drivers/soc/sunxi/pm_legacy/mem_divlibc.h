/*
 * drivers/soc/sunxi/pm_legacy/mem_divlibc.h
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

#ifndef _MEM_DIVLIBC_H
#define _MEM_DIVLIBC_H

void __mem_div0(void);
__u32 raw_lib_udiv(__u32 dividend, __u32 divisior);
extern void __aeabi_idiv(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_uidiv(void);
extern void __aeabi_uidivmod(void);

#endif /*_MEM_DIVLIBC_H*/
