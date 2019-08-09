/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include <common.h>
#include <asm/io.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/timer.h>

void enable_watchdog(void);
void feed_watchdog(void);
void disable_watchdog(void);

void enable_watchdog(void)
{
	struct sunxi_timer_reg *timer_reg;
	struct sunxi_wdog *wdog;

	timer_reg = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	wdog = &timer_reg->wdog[0];

	printf("enable watchdog\n");
	wdog->cfg = 1;
	wdog->mode = 0xb0;   //0xb0 --> 16s
	wdog->mode = (wdog->mode | 0x1);   //enable

	return;
}

void dump_watchdog(void)
{
	struct sunxi_timer_reg *timer_reg;
	struct sunxi_wdog *wdog;

	timer_reg = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	wdog = &timer_reg->wdog[0];

	printf("dump watchdog\n");
	printf("wdog->ctrl:%x %x\n", (int)&(wdog->ctrl),wdog->ctrl);
	printf("wdog->mode:%x %x\n", (int)&(wdog->mode),wdog->mode);
	printf("wdog->cfg:%x %x\n", (int)&(wdog->cfg),wdog->cfg);

	return;
}


void feed_watchdog(void)
{
	struct sunxi_timer_reg *timer_reg;
	struct sunxi_wdog *wdog;

	timer_reg = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	wdog = &timer_reg->wdog[0];

	printf("feed watchdog\n");
	wdog->ctrl = (wdog->ctrl|(0xa57<<1)|(0x1<<0));
}

void disable_watchdog(void)
{
	struct sunxi_timer_reg *timer_reg;
	struct sunxi_wdog *wdog;

	timer_reg = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;
	wdog = &timer_reg->wdog[0];

	printf("disable watchdog\n");
	wdog->mode = (wdog->mode & ~0x1);   //disable

	return;
}

void reboot(void)
{
	struct sunxi_timer_reg *timer_reg;
	struct sunxi_wdog *wdog;

	timer_reg = (struct sunxi_timer_reg *)SUNXI_TIMER_BASE;

	 /* enable watchdog */
	wdog = &timer_reg->wdog[0];
	printf("system will reboot\n");
	wdog->cfg = 1;
	wdog->mode = 1;
	while(1);

	return;
}