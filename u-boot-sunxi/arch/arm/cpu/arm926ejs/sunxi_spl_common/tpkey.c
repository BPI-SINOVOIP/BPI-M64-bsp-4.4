/*
 * (C) Copyright 2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhouhuacai <zhouhuacai@allwinnertech.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/key.h>
#include <asm/arch/ccmu.h>

__attribute__((section(".data")))
static uint32_t keyen_flag = 1;

__weak int sunxi_key_clock_open(void)
{
    return 0;
}

int sunxi_key_init(void)
{
    /* below code init tp controller*/
    struct sunxi_tpcontrol *sunxi_tpkey_base = (struct sunxi_tpcontrol *)SUNXI_TPCONTROL_BASE;
    u32 tp_reg_val = 0;
    u32 gpio_reg_val = 0;
    u32 gpio_reg_mask = 0;

    tp_reg_val = 0xf<<24;
    tp_reg_val |= 1<<23;
    tp_reg_val &= ~(1<<22); /*sellect HOSC(24MHz)*/
    tp_reg_val |= 0x3<<20;  /*00:CLK_IN/2,01:CLK_IN/3,10:CLK_IN/6,11:CLK_IN/1*/
    tp_reg_val |= 0x7f<<0;  /*FS DIV*/
    sunxi_tpkey_base->ctrl0 = tp_reg_val;

    tp_reg_val = 1<<4 | 1<<5; /* select adc mode and enable tp */
    sunxi_tpkey_base->ctrl1 = tp_reg_val;

    tp_reg_val = 1<<16; /* enable date irq */
    sunxi_tpkey_base->int_fifoc = tp_reg_val;

    tp_reg_val = sunxi_tpkey_base->ctrl1;
    tp_reg_val &= ~(0x0f);
    tp_reg_val |= 1<<0;   /* select X1 */
    sunxi_tpkey_base->ctrl1 = tp_reg_val;

    /*configuer gpio*/
    gpio_reg_mask = 0xf << 0;
    gpio_reg_val = *(volatile unsigned int *)(SUNXI_PIO_BASE);
    gpio_reg_val &= ~gpio_reg_mask;
    gpio_reg_val |= 0x2 << 0;
    *(volatile unsigned int *)(SUNXI_PIO_BASE) = gpio_reg_val;

    /* clear fifo */
    tp_reg_val = sunxi_tpkey_base->int_fifoc;
    tp_reg_val |= 1<<4;
    sunxi_tpkey_base->int_fifoc = tp_reg_val;

    return 0;
}

int sunxi_key_read(void)
{
    if(!keyen_flag)
    {
        return -1;
    }

    struct sunxi_tpcontrol *sunxi_tpkey_base = (struct sunxi_tpcontrol *)SUNXI_TPCONTROL_BASE;
    u32 tp_reg_val = 0 , tp_reg_data = 0;

    tp_reg_val = sunxi_tpkey_base->int_fifos;
    if(tp_reg_val & (1 << 16))
    {
        tp_reg_data = sunxi_tpkey_base->data;
        sunxi_tpkey_base->int_fifos = tp_reg_val;
        if( tp_reg_data < 4050 )
        {
            printf("The tp adc value is %d\n",tp_reg_data);
            return tp_reg_data;
        }
        else
        {
            return -1;
        }
    }

    return -1;
}

