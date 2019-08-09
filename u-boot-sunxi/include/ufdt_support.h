/*
 * (C) Copyright 2007
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __UFDT_SUPPORT_H
#define __UFDT_SUPPORT_H

#ifdef CONFIG_OF_LIBUFDT
void *sunxi_support_ufdt(void *dtb_base, u32 dtb_len);
int sunxi_dto_merge_test(void);
#endif

#endif /* ifndef __UFDT_SUPPORT_H */
