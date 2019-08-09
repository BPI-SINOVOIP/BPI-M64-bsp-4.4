/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Berg Xing <bergxing@allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Sunxi platform dram register definition.
 */


#ifndef __AXP21_REGS_H__
#define __AXP21_REGS_H__

#define   AXP21_CHIP_ID              (0x34)

/* define AXP21 REGISTER */
#define   BOOT_POWER21_STATUS              			(0x00)
#define   BOOT_POWER21_MODE_CHGSTATUS      		(0x01)
#define   BOOT_POWER21_OTG_STATUS          			(0x02)
#define   BOOT_POWER21_VERSION         	   			(0x03)
#define   BOOT_POWER21_DATA_BUFFER0        			(0x04)
#define   BOOT_POWER21_DATA_BUFFER1        			(0x05)
#define   BOOT_POWER21_DATA_BUFFER2       			(0x06)
#define   BOOT_POWER21_DATA_BUFFER3       			(0x07)
#define   BOOT_POWER21_OUTPUT_CTL0    	   		(0x80)
#define   BOOT_POWER21_OUTPUT_CTL1     	   		(0x81)
#define   BOOT_POWER21_OUTPUT_CTL2     	   		(0x90)
#define   BOOT_POWER21_OUTPUT_CTL3     	   		(0x91)

#define   BOOT_POWER21_DC1OUT_VOL				(0x82)
#define   BOOT_POWER21_DC2OUT_VOL          			(0x83)
#define   BOOT_POWER21_DC3OUT_VOL          			(0x84)
#define   BOOT_POWER21_DC4OUT_VOL          			(0x85)
#define   BOOT_POWER21_DC5OUT_VOL          			(0x86)
#define   BOOT_POWER21_ALDO1OUT_VOL				(0x92)
#define   BOOT_POWER21_ALDO2OUT_VOL				(0x93)
#define   BOOT_POWER21_ALDO3OUT_VOL				(0x94)
#define   BOOT_POWER21_ALDO4OUT_VOL				(0x95)
#define   BOOT_POWER21_BLDO1OUT_VOL				(0x96)
#define   BOOT_POWER21_BLDO2OUT_VOL				(0x97)
#define   BOOT_POWER21_DC4LDO_VOL				(0x98)
#define   BOOT_POWER21_DLDO1OUT_VOL				(0x99)
#define   BOOT_POWER21_DLDO2OUT_VOL				(0x9A)

#define   BOOT_POWER21_PWRON_STATUS              	(0x20)
#define   BOOT_POWER21_VBUS_VOL_SET             		(0x15)
#define   BOOT_POWER21_VBUS_CUR_SET             		(0x16)
#define   BOOT_POWER21_VOFF_SET            			(0x24)
#define   BOOT_POWER21_OFF_CTL             			(0x10)
#define   BOOT_POWER21_CHARGE1             			(0x62)
#define   BOOT_POWER21_CHGLED_SET             		(0x69)

#define   BOOT_POWER21_BAT_AVERVOL_H8                  (0x34)
#define   BOOT_POWER21_BAT_AVERVOL_L4                  (0x35)

#define   BOOT_POWER21_INTEN0              			(0x40)
#define   BOOT_POWER21_INTEN1              			(0x41)
#define   BOOT_POWER21_INTEN2              			(0x42)

#define   BOOT_POWER21_INTSTS0             			(0x48)
#define   BOOT_POWER21_INTSTS1             			(0x49)
#define   BOOT_POWER21_INTSTS2             			(0x4a)

#define   BOOT_POWER21_FUEL_GAUGE_CTL         			(0x18)
#define   BOOT_POWER21_BAT_PERCEN_CAL					(0xA4)

#endif /* __AXP21_REGS_H__ */
