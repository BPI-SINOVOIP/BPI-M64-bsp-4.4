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
/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dc1sw(int onoff)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff) {
		reg_value |= (1 << 7);
	} else {
		reg_value &= ~(1 << 7);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to set dc1sw\n");

		return -1;
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dc4ldo(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 1400) {
			set_vol = 1400;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC4LDO_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		/*  dldoï¼š 0.5v-1.4v  20mv/step */
		reg_value |= (set_vol - 500) / 20;
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC4LDO_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dldo2\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 6);
	} else {
		reg_value |= (1 << 6);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff dldo2\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dc4ldo(void)
{
	u8 reg_value;
	int vol;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 6))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC4LDO_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	vol = 500 + 20 * reg_value;

	return vol;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc1(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 1500) {
			set_vol = 1500;
		} else if (set_vol > 3400) {
			set_vol = 3400;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC1OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= ~0x1f;
		reg_value |= ((set_vol - 1500) / 100);

		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC1OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dcdc1\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 0);
	} else {
		reg_value |= (1 << 0);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, reg_value)) {
		printf("sunxi pmu error : unable to onoff dcdc1\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dcdc1(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 0))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC1OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 1500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc2(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 1540) {
			set_vol = 1540;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC2OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= ~0x7f;
		/*  dcdc2ï¼š 0.5v-1.2v  10mv/step   1.22v-1.540v  20mv/step */
		if (set_vol > 1200) {
			reg_value |= (70 + (set_vol - 1200) / 20);
		} else {
			reg_value |= (set_vol - 500) / 10;
		}

		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC2OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dcdc2\n");
			return -1;
		}
#if 0
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC2OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		printf("BOOT_POWER21_DC2OUT_VOL=%d\n", reg_value);
#endif
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 1);
	} else {
		reg_value |= (1 << 1);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, reg_value)) {
		printf("sunxi pmu error : unable to onoff dcdc2\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dcdc2(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 1))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC2OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x7f;
	/*  dcdc2ï¼š 0.5v-1.2v  10mv/step   1.22v-1.3v  20mv/step */
	if (reg_value > 70) {
		return 1200 + 20 * (reg_value - 70);
	} else {
		return 500 + 10 * reg_value;
	}
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc3(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3400) {
			set_vol = 3400;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC3OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		reg_value &= (~0x7f);
		/*  dcdc3ï¼š 0.5v-1.2v  10mv/step   1.22v-1.54v  20mv/step  1.6v-3.4v 100mv/step */
		if (set_vol > 1540) {
			reg_value |= (88 + (set_vol - 1540) / 100);
		} else if (set_vol > 1200) {
			reg_value |= (70 + (set_vol - 1200) / 20);
		} else {
			reg_value |= (set_vol - 500) / 10;
		}
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC3OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dcdc3\n");

			return -1;
		}
#if 0
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC3OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		printf("BOOT_POWER21_DC3OUT_VOL=%d\n", reg_value);
#endif
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 2);
	} else {
		reg_value |= (1 << 2);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, reg_value)) {
		printf("sunxi pmu error : unable to onoff dcdc3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dcdc3(void)
{
	u8 reg_value;

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 2))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC3OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x7f;
	/*  dcdc3ï¼š 0.5v-1.2v  10mv/step   1.22v-1.3v  20mv/step 1.6v-3.4v 100mv/step*/
	if (reg_value >= 88) {
		return 1600 + 100 * (reg_value - 88);
	} else if (reg_value > 70) {
		return 1200 + 20 * (reg_value - 70);
	} else {
		return 500 + 10 * reg_value;
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc4(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 1840) {
			set_vol = 1840;
		}

		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC4OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		reg_value &= (~0x7f);
		/*  dcdc4ï¼š 0.5v-1.2v  10mv/step   1.22v-1.84v  20mv/step */
		if (set_vol > 1200) {
			reg_value |= (70 + (set_vol - 1200) / 20);
		} else {
			reg_value |= (set_vol - 500) / 10;
		}
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC4OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dcdc4\n");

			return -1;
		}
#if 0
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC4OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		printf("BOOT_POWER21_DC4OUT_VOL=%d\n", reg_value);
#endif
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 3);
	} else {
		reg_value |= (1 << 3);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, reg_value)) {
		printf("sunxi pmu error : unable to onoff dcdc4\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dcdc4(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 3))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC4OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x7f;
	/*  dcdc4ï¼š 0.5v-1.2v  10mv/step   1.22v-1.84v  20mv/step */
	if (reg_value > 70) {
		return 1200 + 20 * (reg_value - 70);
	} else {
		return 500 + 10 * reg_value;
	}
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc5(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 1400) {
			set_vol = 1400;
		} else if (set_vol > 3700) {
			set_vol = 3700;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC5OUT_VOL,
				 &reg_value)) {
			debug("%d\n", __LINE__);

			return -1;
		}
		reg_value &= (~0x1f);
		/*  dcdc5ï¼š 1.4v-3.7v  100mv/step */
		reg_value |= (set_vol - 1400) / 100;

		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DC5OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dcdc5\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 4);
	} else {
		reg_value |= (1 << 4);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, reg_value)) {
		printf("sunxi pmu error : unable to onoff dcdc5\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dcdc5(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL0, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 4))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DC5OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;
	/*  dcdc5ï¼š 1.4v-3.7v  100mv/step */

	return 1400 + 100 * (reg_value - 24);
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_aldo1(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO1OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_ALDO1OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo1\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 0);
	} else {
		reg_value |= (1 << 0);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo1\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_aldo1(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 0))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO1OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_aldo2(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO2OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_ALDO2OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo2\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 1);
	} else {
		reg_value |= (1 << 1);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo2\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_aldo2(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 1))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO2OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_aldo3(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO3OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_ALDO3OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo3\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 2);
	} else {
		reg_value |= (1 << 2);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_aldo3(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 2))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO3OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_aldo4(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO4OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_ALDO4OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo3\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 3);
	} else {
		reg_value |= (1 << 3);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_aldo4(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 3))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_ALDO4OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_bldo1(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BLDO1OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_BLDO1OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo3\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 4);
	} else {
		reg_value |= (1 << 4);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_bldo1(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 4))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BLDO1OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_bldo2(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BLDO2OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_BLDO2OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo3\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 5);
	} else {
		reg_value |= (1 << 5);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_bldo2(void)
{
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 5))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_BLDO2OUT_VOL,
			 &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	return 500 + 100 * reg_value;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dldo1(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 3500) {
			set_vol = 3500;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DLDO1OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		reg_value |= ((set_vol - 500) / 100);
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DLDO1OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set aldo3\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 7);
	} else {
		reg_value |= (1 << 7);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, reg_value)) {
		printf("sunxi pmu error : unable to onoff aldo3\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dldo1(void)
{
	int vol;
	u8 reg_value;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL2, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 7))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DLDO1OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;
	vol = 500 + 100 * reg_value;

	return vol;
}

/*
************************************************************************************************************
*
*                                             function
*
*    º¯ÊýÃû³Æ£º
*
*    ²ÎÊýÁÐ±í£º
*
*    ·µ»ØÖµ  £º
*
*    ËµÃ÷    £º
*
*
************************************************************************************************************
*/
static int axp21_set_dldo2(int set_vol, int onoff)
{
	u8 reg_value;

	if (set_vol > 0) {
		if (set_vol < 500) {
			set_vol = 500;
		} else if (set_vol > 1400) {
			set_vol = 1400;
		}
		if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DLDO2OUT_VOL,
				 &reg_value)) {
			return -1;
		}
		reg_value &= (~0x1f);
		/*  dldoï¼š 0.5v-1.4v  50mv/step */
		reg_value |= (set_vol - 500) / 50;
		if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_DLDO2OUT_VOL,
				  reg_value)) {
			printf("sunxi pmu error : unable to set dldo2\n");

			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL3, &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << 0);
	} else {
		reg_value |= (1 << 0);
	}
	if (axp_i2c_write(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL3, reg_value)) {
		printf("sunxi pmu error : unable to onoff dldo2\n");

		return -1;
	}

	return 0;
}

static int axp21_probe_dldo2(void)
{
	u8 reg_value;
	int vol;
	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_OUTPUT_CTL3, &reg_value)) {
		return -1;
	}
	if (!(reg_value & (0x01 << 0))) {
		return 0;
	}

	if (axp_i2c_read(AXP21_CHIP_ID, BOOT_POWER21_DLDO2OUT_VOL, &reg_value)) {
		return -1;
	}
	reg_value &= 0x1f;

	vol = 500 + 50 * reg_value;

	return vol;
}


/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int axp21_set_dcdc_output(int sppply_index, int vol_value, int onoff)
{
	switch (sppply_index) {
	case 1:
		return axp21_set_dcdc1(vol_value, onoff);
	case 2:
		return axp21_set_dcdc2(vol_value, onoff);
	case 3:
		return axp21_set_dcdc3(vol_value, onoff);
	case 4:
		return axp21_set_dcdc4(vol_value, onoff);
	case 5:
		return axp21_set_dcdc5(vol_value, onoff);
	}

	return -1;
}

static int axp21_set_aldo_output(int sppply_index, int vol_value, int onoff)
{
	switch (sppply_index) {
	case 1:
		return axp21_set_aldo1(vol_value, onoff);
	case 2:
		return axp21_set_aldo2(vol_value, onoff);
	case 3:
		return axp21_set_aldo3(vol_value, onoff);
	case 4:
		return axp21_set_aldo4(vol_value, onoff);
	}

	return -1;
}

static int axp21_set_bldo_output(int sppply_index, int vol_value, int onoff)
{
	switch (sppply_index) {
	case 1:
		return axp21_set_bldo1(vol_value, onoff);
	case 2:
		return axp21_set_bldo2(vol_value, onoff);
	}

	return -1;
}

static int axp21_set_dldo_output(int sppply_index, int vol_value, int onoff)
{
	switch (sppply_index) {
	case 1:
		return axp21_set_dldo1(vol_value, onoff);
	case 2:
		return axp21_set_dldo2(vol_value, onoff);
	}

	return -1;
}


static int axp21_set_misc_output(int sppply_index, int vol_value, int onoff)
{
	switch (sppply_index) {
	case PMU_SUPPLY_DC4LDO:
		return axp21_set_dc4ldo(vol_value, onoff);
	case PMU_SUPPLY_DC1SW:
		return axp21_set_dc1sw(onoff);
	}

	return -1;
}

int axp21_set_supply_status(int vol_name, int vol_value, int onoff)
{
	int supply_type;
	int sppply_index;

	supply_type = vol_name & 0xffff0000;
	sppply_index = vol_name & 0x0000ffff;

	switch (supply_type) {
	case PMU_SUPPLY_DCDC_TYPE:
		return axp21_set_dcdc_output(sppply_index, vol_value, onoff);

	case PMU_SUPPLY_ALDO_TYPE:
		return axp21_set_aldo_output(sppply_index, vol_value, onoff);

	case PMU_SUPPLY_BLDO_TYPE:
		return axp21_set_bldo_output(sppply_index, vol_value, onoff);

	case PMU_SUPPLY_DLDO_TYPE:
		return axp21_set_dldo_output(sppply_index, vol_value, onoff);

	case PMU_SUPPLY_MISC_TYPE:
		return axp21_set_misc_output(vol_name, vol_value, onoff);

		break;

	default:
		return -1;
	}
}

int axp21_set_supply_status_byname(char *vol_name, int vol_value, int onoff)
{
	int sppply_index;

	if (!strncmp(vol_name, "dcdc", 4)) {
		sppply_index = simple_strtoul(vol_name + 4, NULL, 10);

		return axp21_set_dcdc_output(sppply_index, vol_value, onoff);
	} else if (!strncmp(vol_name, "aldo", 4)) {
		sppply_index = simple_strtoul(vol_name + 4, NULL, 10);

		return axp21_set_aldo_output(sppply_index, vol_value, onoff);
	} else if (!strncmp(vol_name, "bldo", 4)) {
		sppply_index = simple_strtoul(vol_name + 4, NULL, 10);

		return axp21_set_bldo_output(sppply_index, vol_value, onoff);
	} else if (!strncmp(vol_name, "dldo", 4)) {
		sppply_index = simple_strtoul(vol_name + 4, NULL, 10);

		return axp21_set_dldo_output(sppply_index, vol_value, onoff);
	} else if (!strncmp(vol_name, "dc4ldo", 6)) {
		return axp21_set_dc4ldo(vol_value, onoff);
	} else if (!strncmp(vol_name, "dc1sw", 5)) {
		return axp21_set_dc1sw(onoff);
	}

	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static int axp21_probe_dcdc_output(int sppply_index)
{
	switch (sppply_index) {
	case 1:
		return axp21_probe_dcdc1();
	case 2:
		return axp21_probe_dcdc2();
	case 3:
		return axp21_probe_dcdc3();
	case 4:
		return axp21_probe_dcdc4();
	case 5:
		return axp21_probe_dcdc5();
	}

	return -1;
}

static int axp21_probe_aldo_output(int sppply_index)
{
	switch (sppply_index) {
	case 1:
		return axp21_probe_aldo1();
	case 2:
		return axp21_probe_aldo2();
	case 3:
		return axp21_probe_aldo3();
	case 4:
		return axp21_probe_aldo4();
	}

	return -1;
}

static int axp21_probe_bldo_output(int sppply_index)
{
	switch (sppply_index) {
	case 1:
		return axp21_probe_bldo1();
	case 2:
		return axp21_probe_bldo2();
	}

	return -1;
}

static int axp21_probe_dldo_output(int sppply_index)
{
	switch (sppply_index) {
	case 1:
		return axp21_probe_dldo1();
	case 2:
		return axp21_probe_dldo2();
	}

	return -1;
}

int axp21_probe_supply_status(int vol_name, int vol_value, int onoff)
{
	return 0;
}

int axp21_probe_supply_status_byname(char *vol_name)
{
	int sppply_index;

	sppply_index = 1 + vol_name[4] - '1';

	if (!strncmp(vol_name, "dcdc", 4)) {
		return axp21_probe_dcdc_output(sppply_index);
	} else if (!strncmp(vol_name, "aldo", 4)) {
		return axp21_probe_aldo_output(sppply_index);
	} else if (!strncmp(vol_name, "bldo", 4)) {
		return axp21_probe_bldo_output(sppply_index);
	} else if (!strncmp(vol_name, "dldo", 4)) {
		return axp21_probe_dldo_output(sppply_index);
	} else if (!strncmp(vol_name, "dc4ldo", 6)) {
		return axp21_probe_dc4ldo();
	}
	return -1;
}
