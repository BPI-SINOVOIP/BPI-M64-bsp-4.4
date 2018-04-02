/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _SUNXI_KEY_H
#define _SUNXI_KEY_H

#include "asm/arch/platform.h"

struct sunxi_tpcontrol {
    volatile u32 ctrl0;      /* TP control Register0,add:0x00*/
    volatile u32 ctrl1;      /* TP control Register1,add:0x04*/
    volatile u32 ctrl2;      /* TP Pressure Measurement and touch sensitive Control Register,add:0x08*/
    volatile u32 ctrl3;      /* Median and averaging filter Controller Register,add:0x0c*/
    volatile u32 int_fifoc;  /* TP Interrupt FIFO Control Register,add:0x10*/
    volatile u32 int_fifos;  /* TP Interrupt FIFO Status Register,add:0x14*/
    volatile u32 empt1;      /* This register not use,add:0x18*/
    volatile u32 cdat;       /* TP Common Data,add:0x1c*/
    volatile u32 empt2;      /* This register not use,add:0x20*/
    volatile u32 data;       /* TP Data Register,add:0x24*/
};

/*SUNXI_TPCONTROL_BASE=0x01c24800*/
#define REG_TP_KEY_CTL0         (SUNXI_TPCONTROL_BASE + 0X00)
#define REG_TP_KEY_CTL1         (SUNXI_TPCONTROL_BASE + 0X04)
#define REG_TP_KEY_CTL2         (SUNXI_TPCONTROL_BASE + 0X08)
#define REG_TP_KEY_CTL3         (SUNXI_TPCONTROL_BASE + 0X0C)
#define REG_TP_KEY_INT_CTL      (SUNXI_TPCONTROL_BASE + 0X10)
#define REG_TP_KEY_INT_STS      (SUNXI_TPCONTROL_BASE + 0X14)
#define REG_TP_KEY_COM_DAT      (SUNXI_TPCONTROL_BASE + 0X1C)
#define REG_TP_KEY_ADC_DAT      (SUNXI_TPCONTROL_BASE + 0X24)


struct sunxi_lradc {
    volatile u32 ctrl;  /* lradc control     */
    volatile u32 intc;  /* interrupt control */
    volatile u32 ints;  /* interrupt status  */
    volatile u32 data0; /* lradc 0 data      */
    volatile u32 data1; /* lradc 1 data      */
};

#define SUNXI_KEY_ADC_CRTL        (SUNXI_KEYADC_BASE + 0x00)
#define SUNXI_KEY_ADC_INTC        (SUNXI_KEYADC_BASE + 0x04)
#define SUNXI_KEY_ADC_INTS        (SUNXI_KEYADC_BASE + 0x08)
#define SUNXI_KEY_ADC_DATA0       (SUNXI_KEYADC_BASE + 0x0C)


#define LRADC_EN                  (0x1)      /* LRADC enable       */
#define LRADC_SAMPLE_RATE         (0x2)      /* 32.25 Hz           */
#define LEVELB_VOL                (0x2)      /* 0x33(~1.6v)        */
#define LRADC_HOLD_EN             (0x1 << 6) /* sample hold enable */
#define KEY_MODE_SELECT           (0x0)      /* normal mode        */

#define ADC0_DATA_PENDING         (1 << 0)   /* adc0 has data      */
#define ADC0_KEYDOWN_PENDING      (1 << 1)   /* key down           */
#define ADC0_HOLDKEY_PENDING      (1 << 2)   /* key hold           */
#define ADC0_ALRDY_HOLD_PENDING   (1 << 3)   /* key already hold   */
#define ADC0_KEYUP_PENDING        (1 << 4)   /* key up             */

extern int sunxi_key_init(void);

extern int sunxi_key_exit(void);

extern int sunxi_key_read(void);

extern int sunxi_key_probe(void);

#endif
