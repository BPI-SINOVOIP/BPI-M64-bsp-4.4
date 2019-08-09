/*
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, d.mueller@elsoft.ch
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

/* This code should work for both the S3C2400 and the S3C2410
 * as they seem to have the same I2C controller inside.
 * The different address mapping is handled by the s3c24xx.h files below.
 */

#include <common.h>
#include <asm/arch/base_pmu.h>
#include <power/sunxi/axp21_reg.h>
#include <power/sunxi/axp809_reg.h>
#include <private_boot0.h>

u8 pmu_type;
__weak boot0_file_head_t BT0_head;
__weak boot0_file_head_t fes1_head;

#define pmu_err(format, arg...)	printf("[pmu]: "format, ##arg)
#define pmu_info(format, arg...)	/*printf("[pmu]: "format,##arg)*/

#define RSB_SLAVE_ADDR   (0x3a3)
#define RSB_RUNTIME_ADDR (0x2d)

#define PMU_IC_TYPY_REG (0x3)

#define VDD_SYS_VOL	(920)
#define VOL_ON			(1)

typedef struct _axp_contrl_info {
	char name[8];

	u16 min_vol;
	u16 max_vol;
	u16 cfg_reg_addr;
	u16 cfg_reg_mask;

	u16 step0_val;
	u16 split1_val;
	u16 step1_val;
	u16 ctrl_reg_addr;

	u16 ctrl_bit_ofs;
	u16 res;
} axp_contrl_info;

extern s32 rsb_init(u32 saddr, u32 rtaddr);
extern s32 rsb_exit(void);
extern s32 rsb_write(u32 rtsaddr, u32 daddr, u8 *data, u32 len);
extern s32 rsb_read(u32 rtsaddr, u32 daddr, u8 *data, u32 len);
static int pmu_set_vol(char *name, int set_vol, int onoff);
s32 pmu_bus_read(u32 rtaddr, u32 daddr, u8 *data);
s32 pmu_bus_init(void);
s32 pmu_reset_enable(void);
s32 pmu_disable_soften3_signal(void);
s32 enable_vbus_adc_channel(void);
s32 set_dcdc1_pwm_mode(void);
s32 set_chgcur_limit(void);




axp_contrl_info axp21_ctrl_tbl[] = { \
/*   name     min,  max,   reg,   mask,  step0,
split1_val,  step1,   ctrl_reg,     ctrl_bit */

	{"dcdc2",   500,  1540, BOOT_POWER21_DC2OUT_VOL,  0x7f,   10,
	1220,   20,   BOOT_POWER21_OUTPUT_CTL0, 1},

	{"dcdc3",   500,  1540, BOOT_POWER21_DC3OUT_VOL,  0x7f,   10,
	1220,   20,   BOOT_POWER21_OUTPUT_CTL0, 2},

	{"dcdc4",   500,  1840, BOOT_POWER21_DC4OUT_VOL,  0x7f,   10,
	1220,   20,   BOOT_POWER21_OUTPUT_CTL0, 3},

	{"bldo1",   500,  3500, BOOT_POWER21_BLDO1OUT_VOL,  0x7f,   100,
	0,   0,   BOOT_POWER21_OUTPUT_CTL2, 4},
};

axp_contrl_info axp809_ctrl_tbl[] = { \
/*   name     min,  max,   reg,   mask,  step0,
split1_val,  step1,   ctrl_reg,     ctrl_bit */

	{"dcdc2",   600,  1540, 0x22,  0x7f,   20,
	0,   0,   BOOT_POWER809_OUTPUT_CTL1, 2},

	{"dcdc3",   600,  1860, 0x23,  0x7f,   20,
	0,   0,   BOOT_POWER809_OUTPUT_CTL1, 3},

	{"dcdc5",   1000,  2550, 0x25,  0x7f,   50,
	0,   0,   BOOT_POWER809_OUTPUT_CTL1, 5},

	{"aldo3",   700,  3300, 0x2a,  0x7f,   100,
	0,   0,   BOOT_POWER809_OUTPUT_CTL2, 5},
};

int pmu_init(u8 power_mode)
{
	if (fes1_head.prvt_head.reserve[0] != 0)
		BT0_head.prvt_head.reserve[0] = fes1_head.prvt_head.reserve[0];
	else if (BT0_head.prvt_head.reserve[0] == 0)
		BT0_head.prvt_head.reserve[0] = VDD_SYS_VOL / 10;

	if (pmu_bus_init()) {
		pmu_err("bus init error\n");
		return -1;
	}

	if (pmu_bus_read(RSB_RUNTIME_ADDR, PMU_IC_TYPY_REG, &pmu_type)) {
		pmu_err("bus read error\n");
		return -1;
	}

	pmu_type &= 0xCF;

	if (pmu_type == 0x47) {
		/* pmu type AXP2101 */
		printf("PMU: AXP2101\n");
		/* limit charge current to 300mA */
		set_chgcur_limit();
		/* keep LDO stable */
		enable_vbus_adc_channel();
		set_dcdc1_pwm_mode();
		/* keep DCDC4 stable by disable soften3_signal */
		pmu_disable_soften3_signal();
		pmu_reset_enable();
		set_sys_voltage(BT0_head.prvt_head.reserve[0] * 10, VOL_ON);
		return AXP21_CHIP_ID;
	} else if (pmu_type == 0x42) {
		/* pmu type AXP809 */
		printf("PMU: AXP809\n");
		set_sys_voltage(BT0_head.prvt_head.reserve[0] * 10, VOL_ON);
		return AXP809_CHIP_ID;
	}

	printf("unknow PMU\n");
	return -1;

}

s32 pmu_bus_init(void)
{
	return rsb_init(RSB_SLAVE_ADDR, RSB_RUNTIME_ADDR);
}

s32 pmu_bus_read(u32 rtaddr, u32 daddr, u8 *data)
{
	return rsb_read(rtaddr, daddr, data, 1);
}

s32 pmu_bus_write(u32 rtaddr, u32 daddr, u8 data)
{
	return rsb_write(rtaddr, daddr, &data, 1);
}

s32 set_chgcur_limit(void)
{
	/* limit charge current to 300mA */
	return pmu_bus_write(RSB_RUNTIME_ADDR, BOOT_POWER21_CHARGE1, 0x9);
}

s32 enable_vbus_adc_channel(void)
{
	return pmu_bus_write(RSB_RUNTIME_ADDR, BOOT_POWER21_BAT_AVERVOL_H8, 0x40);
}

s32 set_dcdc1_pwm_mode(void)
{
	uchar reg_value;

	if (pmu_bus_read(RSB_RUNTIME_ADDR, BOOT_POWER21_OUTPUT_CTL1, &reg_value))
		return -1;
	reg_value |= (1 << 2);
	if (pmu_bus_write(RSB_RUNTIME_ADDR, BOOT_POWER21_BAT_AVERVOL_H8, reg_value))
		return -1;
	return 0;
}

s32 pmu_disable_soften3_signal(void)
{
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xFF, 0x00))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xF0, 0x06))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xF1, 0x04))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xFF, 0x01))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0X26, 0x30))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xFF, 0x00))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xF1, 0x00))
		return -1;
	if (pmu_bus_write(RSB_RUNTIME_ADDR, 0xF0, 0x00))
		return -1;
	return 0;
}

s32 pmu_reset_enable(void)
{
	uchar reg_value;
	if (pmu_bus_read(RSB_RUNTIME_ADDR, BOOT_POWER21_OFF_CTL, &reg_value))
			return -1;
	reg_value |= (1 << 3);

	if (pmu_bus_write(RSB_RUNTIME_ADDR, BOOT_POWER21_OFF_CTL, reg_value))
		return -1;
	return 0;
}

int probe_power_key(void)
{
	u8 reg_value;
	u8 pok_status = 0, pok_offset = 0;

	if (pmu_type == 0x47) {
		pok_status = BOOT_POWER21_INTSTS1;
		pok_offset = 0x02;
	} else if (pmu_type == 0x42) {
		pok_status = BOOT_POWER809_INTSTS5;
		pok_offset =  0x03;
	}

	if (pmu_bus_read(RSB_RUNTIME_ADDR, pok_status, &reg_value)) {
		return -1;
	}
	/* POKLIRQ,POKSIRQ */
	reg_value &= (0x03 << pok_offset);
	if (reg_value) {
		if (pmu_bus_write(RSB_RUNTIME_ADDR, pok_status,
				  reg_value)) {
			return -1;
		}
	}

	return  (reg_value >> pok_offset) & 3;
}

int set_ddr_voltage(int set_vol)
{
	if (pmu_type == 0x47)
		return pmu_set_vol("dcdc4", set_vol, 1);
	else if (pmu_type == 0x42)
		return pmu_set_vol("dcdc5", set_vol, 1);

	return -1;
}

int set_pll_voltage(int set_vol)
{
	return pmu_set_vol("dcdc2", set_vol, 1);
}

int set_efuse_voltage(int set_vol)
{
	if (pmu_type == 0x47)
		return pmu_set_vol("bldo1", set_vol, 1);
	else if (pmu_type == 0x42)
		return pmu_set_vol("aldo3", set_vol, 1);
	return -1;
}

int set_sys_voltage(int set_vol, int onoff)
{
	return pmu_set_vol("dcdc3", set_vol, onoff);
}

static axp_contrl_info *get_ctrl_info_from_tbl(axp_contrl_info *axp_ctrl_tbl, int size, char *name)
{
	int i = 0;

	for (i = 0; i < size; i++) {
		if (!strncmp(name, axp_ctrl_tbl[i].name,
			     strlen(axp_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		return NULL;
	}
	return axp_ctrl_tbl + i;
}

static int pmu_set_vol(char *name, int set_vol, int onoff)
{
	u8   reg_value;
	axp_contrl_info *p_item = NULL;
	u8 base_step;

	if (set_vol <= 0)
		return 0;

	if (pmu_type == 0x47)
		p_item = get_ctrl_info_from_tbl(axp21_ctrl_tbl, ARRAY_SIZE(axp21_ctrl_tbl), name);
	else if (pmu_type == 0x42)
		p_item = get_ctrl_info_from_tbl(axp809_ctrl_tbl, ARRAY_SIZE(axp809_ctrl_tbl), name);
	if (!p_item) {
		return -1;
	}

	pmu_info("name %s, min_vol %dmv, max_vol %d, cfg_reg 0x%x, cfg_mask 0x%x \
		step0_val %d, split1_val %d, step1_val %d, ctrl_reg_addr 0x%x, ctrl_bit_ofs %d\n",
	p_item->name,
	p_item->min_vol,
	p_item->max_vol,
	p_item->cfg_reg_addr,
	p_item->cfg_reg_mask,
	p_item->step0_val,
	p_item->split1_val,
	p_item->step1_val,
	p_item->ctrl_reg_addr,
	p_item->ctrl_bit_ofs);

	if (set_vol < p_item->min_vol) {
		set_vol = p_item->min_vol;
	} else if (set_vol > p_item->max_vol) {
		set_vol = p_item->max_vol;
	}
	if (pmu_bus_read(RSB_RUNTIME_ADDR, p_item->cfg_reg_addr, &reg_value)) {
		return -1;
	}

	reg_value &= ~p_item->cfg_reg_mask;
	if (p_item->split1_val && (set_vol > p_item->split1_val)) {
		if (p_item->split1_val < p_item->min_vol) {
			pmu_err("bad split val(%d) for %s\n",
				p_item->split1_val, name);
			return -1;
		}

		base_step =
		    (p_item->split1_val - p_item->min_vol) / p_item->step0_val;
		reg_value |=
		    (base_step +
		     (set_vol - p_item->split1_val) / p_item->step1_val);
	} else {
		reg_value |= (set_vol - p_item->min_vol) / p_item->step0_val;
	}

	if (pmu_bus_write(RSB_RUNTIME_ADDR, p_item->cfg_reg_addr, reg_value)) {
		pmu_err("unable to set %s\n", name);
		return -1;
	}

#if 0
	if (onoff < 0) {
		return 0;
	}
	if (pmu_bus_read(RSB_RUNTIME_ADDR, p_item->ctrl_reg_addr, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (pmu_bus_write(RSB_RUNTIME_ADDR, p_item->ctrl_reg_addr, reg_value)) {
		pmu_err("unable to onoff %s\n", name);
		return -1;
	}
#endif
	return 0;
}
