/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef _SUNXI_RTC_H_
#define _SUNXI_RTC_H_


/* Rtc */
struct sunxi_rtc_regs {
	volatile u32 losc_ctrl;
	volatile u32 losc_auto_swt_status;
	volatile u32 clock_prescalar;
	volatile u32 res0[1];
	volatile u32 yymmdd;
	volatile u32 hhmmss;
	volatile u32 res1[2];
	volatile u32 alarm0_counter;
	volatile u32 alarm0_current_value;
	volatile u32 alarm0_enable;
	volatile u32 alarm0_irq_enable;
	volatile u32 alarm0_irq_status;
	volatile u32 res2[3];
	volatile u32 alarm1_wk_hms;
	volatile u32 alarm1_enable;
	volatile u32 alarm1_irq_enable;
	volatile u32 alarm1_irq_status;
	volatile u32 alarm_config;
};



#endif

