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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/key.h>
#include <asm/arch/sys_proto.h>
#include <sys_config.h>
#include <power/sunxi/pmu.h>
#include <fdt_support.h>

__attribute__((section(".data")))
static uint32_t keyen_flag = 1;

__weak int sunxi_key_clock_open(void)
{
	return 0;
}

__weak int sunxi_key_clock_close(void)
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
    tp_reg_val |= 0x3<<20; /*00:CLK_IN/2,01:CLK_IN/3,10:CLK_IN/6,11:CLK_IN/1*/
    tp_reg_val |= 0x7f<<0; /*FS DIV*/
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

    int nodeoffset;
    nodeoffset = fdt_path_offset(working_fdt,FDT_PATH_KEY_DETECT);
    if(nodeoffset > 0)
    {
        fdt_getprop_u32(working_fdt,nodeoffset,"keyen_flag",&keyen_flag);
    }

	return 0;
}

int sunxi_key_exit(void)
{
    /* below code exit tp controller*/
	struct sunxi_tpcontrol *sunxi_tpkey_base = (struct sunxi_tpcontrol *)SUNXI_TPCONTROL_BASE;
    u32 tp_reg_val = 0;

    /* select tp mode and disable tp */
    tp_reg_val = ~(1<<4 | 1<<5);
    sunxi_tpkey_base->ctrl1 &= tp_reg_val;

    /* disable date irq */
    tp_reg_val = ~(1<<16);
    sunxi_tpkey_base->int_fifoc &= tp_reg_val;

    /* clear fifo */
    tp_reg_val = sunxi_tpkey_base->int_fifoc;
    tp_reg_val |= 1<<4;
    sunxi_tpkey_base->int_fifoc = tp_reg_val;

	sunxi_key_clock_close();

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

int do_key_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int nodeoffset;

    nodeoffset = fdt_path_offset(working_fdt,FDT_PATH_KEY_DETECT);
    if(nodeoffset > 0)
    {
        fdt_getprop_u32(working_fdt,nodeoffset,"keyen_flag",&keyen_flag);
    }
    if(!keyen_flag)
    {
        puts("warnning: not support,please set keyen_flag=1 in sys_config.fex\n");
        return -1;
    }

	struct sunxi_tpcontrol *sunxi_tpkey_base = (struct sunxi_tpcontrol *)SUNXI_TPCONTROL_BASE;
    u32 tp_reg_val = 0;

    puts("press a key:\n");

    /* clear tp fifo */
    tp_reg_val = sunxi_tpkey_base->int_fifoc;
    tp_reg_val |= 1<<4;
    sunxi_tpkey_base->int_fifoc = tp_reg_val;

    while(!ctrlc())
    {
        sunxi_key_read();
    }

	return 0;
}

U_BOOT_CMD(
	tpkey_test, 1, 0,	do_key_test,
	"Test the key value(use tp control)\n",
	""
);
