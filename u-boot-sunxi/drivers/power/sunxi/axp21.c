/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Berg Xing <bergxing@allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Sunxi platform dram register definition.
 */

#include <common.h>
#include <power/sunxi/axp21_reg.h>
#include <power/sunxi/axp.h>
#include <power/sunxi/pmu.h>

extern int axp21_set_supply_status(int vol_name, int vol_value, int onoff);
extern int axp21_set_supply_status_byname(char *vol_name, int vol_value,
					  int onoff);
extern int axp21_probe_supply_status(int vol_name, int vol_value, int onoff);
extern int axp21_probe_supply_status_byname(char *vol_name);
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe(void)
{
	u8 pmu_type;

	axp_i2c_config(SUNXI_AXP_21X, AXP21_CHIP_ID);
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_VERSION, &pmu_type)) {
		printf("axp read error\n");

		return -1;
	}

	pmu_type &= 0xCF;
	if (pmu_type == 0x47) {
		/* pmu type AXP21 */
		tick_printf("PMU: AXP21\n");

		return 0;
	}

	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_coulombmeter_onoff(int onoff)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_FUEL_GAUGE_CTL,
			 &reg_value)) {
		return -1;
	}
	if (!onoff)
		reg_value &= ~(0x01 << 3);
	else
		reg_value |= (0x01 << 3);

	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_FUEL_GAUGE_CTL,
			  reg_value)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_charge_control(void)
{
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_battery_ratio(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BAT_PERCEN_CAL,
			 &reg_value)) {
		return -1;
	}

	return reg_value & 0x7f;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_power_status(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_STATUS, &reg_value)) {
		return -1;
	}
	/* vbus exist */
	if (reg_value & 0x20) {
		return AXP_VBUS_EXIST;
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_battery_exist(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_STATUS, &reg_value))
		return -1;

	if (reg_value & 0x08)
		return 1;

	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_battery_vol(void)
{
	u8 reg_value_h, reg_value_l;
	int bat_vol;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BAT_AVERVOL_H8,
			 &reg_value_h)) {
		return -1;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BAT_AVERVOL_L4,
			 &reg_value_l)) {
		return -1;
	}
	bat_vol = (reg_value_h << 6) | reg_value_l;

	return bat_vol;
}

int axp21_set_led_control(int flag)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_CHGLED_SET, &reg_value)) {
		return -1;
	}

	if (flag == 0x00) { /*Hi-z ;return controled by charger*/
		reg_value = reg_value | 0x01;
		reg_value = reg_value & 0xcf;
	} else if (flag ==
		   0x01) { /*25%/75% 1Hz toggle;get led control capacity*/
		reg_value = reg_value & 0xfE;
		reg_value = reg_value & 0xcf;
		reg_value = reg_value | 0x10;
	} else if (flag ==
		   0x02) { /*25%/75% 4Hz toggle;get led control capacity*/
		reg_value = reg_value & 0xfE;
		reg_value = reg_value & 0xcf;
		reg_value = reg_value | 0x20;
	} else if (flag == 0x03) { /*drive low;get led control capacity*/
		reg_value = reg_value & 0xfE;
		reg_value = reg_value & 0xcf;
		reg_value = reg_value | 0x30;
	} else {
		printf("not support model\n");
		return -1;
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_CHGLED_SET, reg_value)) {
		return -1;
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_key(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_INTSTS1, &reg_value)) {
		return -1;
	}
	reg_value &= (0x03 << 2);
	if (reg_value) {
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_INTSTS1,
				  reg_value)) {
			return -1;
		}
	}

	return (reg_value >> 2) & 3;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_pre_sys_mode(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DATA_BUFFER3,
			 &reg_value)) {
		return -1;
	}

	return reg_value;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_next_sys_mode(int data)
{
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DATA_BUFFER3, (u8)data)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_this_poweron_cause(void)
{
	uchar reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_PWRON_STATUS,
			 &reg_value)) {
		return -1;
	}

	if ((reg_value & 0x01) | (reg_value & 0x04))
		return 1;
	else
		return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_power_off(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OFF_CTL, &reg_value)) {
		return -1;
	}
	reg_value |= 1;
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OFF_CTL, reg_value)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_power_onoff_vol(int set_vol, int stage)
{
	u8 reg_value;

	if (!set_vol) {
		if (!stage) {
			set_vol = 3300;
		} else {
			set_vol = 2900;
		}
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_VOFF_SET, &reg_value)) {
		return -1;
	}
	reg_value &= 0xf8;
	if (set_vol >= 2600 && set_vol <= 3300) {
		reg_value |= (set_vol - 2600) / 100;
	} else if (set_vol <= 2600) {
		reg_value |= 0x00;
	} else {
		reg_value |= 0x07;
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_VOFF_SET, reg_value)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_charge_current(int current)
{
	u8 reg_value;
	int step;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_CHARGE1, &reg_value)) {
		return -1;
	}
	reg_value &= ~0x1f;
	if (current > 2000) {
		current = 2000;
	}
	if (current <= 200)
		step = current / 25;
	else
		step = (current / 100) + 6;

	reg_value |= (step & 0x1f);
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_CHARGE1, reg_value)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_charge_current(void)
{
	uchar reg_value;
	int current;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_CHARGE1, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;
	if (reg_value <= 8)
		current = reg_value * 25;
	else
		current = 200 + 100 * (reg_value - 8);
	return current;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_vbus_cur_limit(int current)
{
	uchar reg_value;

	/* set bus current limit off */
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_VBUS_CUR_SET,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0xf8;

	if (current >= 2000) {
		/*limit to 2000mA */
		reg_value |= 0x05;
	} else if (current >= 1500) {
		/* limit to 1500mA */
		reg_value |= 0x04;
	} else if (current >= 1000) {
		/* limit to 1000mA */
		reg_value |= 0x03;
	} else if (current >= 900) {
		/*limit to 900mA */
		reg_value |= 0x02;
	} else if (current >= 500) {
		/*limit to 500mA */
		reg_value |= 0x01;
	} else if (current >= 100) {
		/*limit to 100mA */
		reg_value |= 0x0;
	} else
		reg_value |= 0x01;

	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_VBUS_CUR_SET,
			  reg_value)) {
		return -1;
	}

	return 0;
}

int axp21_probe_vbus_cur_limit(void)
{
	uchar reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_VBUS_CUR_SET,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x07;
	if (reg_value == 0x05) {
		printf("limit to 2000mA \n");
		return 2000;
	} else if (reg_value == 0x04) {
		printf("limit to 1500mA \n");
		return 1500;
	} else if (reg_value == 0x03) {
		printf("limit to 1000mA \n");
		return 1000;
	} else if (reg_value == 0x02) {
		printf("limit to 900mA \n");
		return 900;
	} else if (reg_value == 0x01) {
		printf("limit to 500mA \n");
		return 500;
	} else if (reg_value == 0x00) {
		printf("limit to 100mA \n");
		return 100;
	} else {
		printf("do not limit current \n");
		return 0;
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_vbus_vol_limit(int vol)
{
	uchar reg_value;

	if (!vol)
		return 0;

	/* set bus vol limit off */
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_VBUS_VOL_SET,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0xf0;

	if (vol < 3880) {
		vol = 3880;
	} else if (vol > 5080) {
		vol = 5080;
	}
	reg_value |= (vol - 3880) / 80;
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_VBUS_VOL_SET,
			  reg_value)) {
		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_int_pending(uchar *addr)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_INTSTS0 + i,
				 addr + i)) {
			return -1;
		}
	}

	for (i = 0; i < 3; i++) {
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_INTSTS0 + i,
				  0xff)) {
			return -1;
		}
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_probe_int_enable(uchar *addr)
{
	int i;
	uchar int_reg = BOOT_POWER21_INTEN0;

	for (i = 0; i < 3; i++) {
		if (axp_i2c_read(AXP21_CHIP_ID, int_reg, addr + i)) {
			return -1;
		}
		int_reg++;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
int axp21_set_int_enable(uchar *addr)
{
	int i;
	uchar int_reg = BOOT_POWER21_INTEN0;

	for (i = 0; i < 3; i++) {
		if (axp_i2c_write(AXP21_CHIP_ID, int_reg, addr[i])) {
			return -1;
		}
		int_reg++;
	}

	return 0;
}

sunxi_axp_module_init("axp21", SUNXI_AXP_21X);

s32 axp21_usb_vbus_output(int hight)
{
	return 0;
}
