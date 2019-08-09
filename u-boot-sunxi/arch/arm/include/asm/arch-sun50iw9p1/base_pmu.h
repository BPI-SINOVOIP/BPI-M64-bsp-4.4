/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef __BASE_PMU_H_
#define __BASE_PMU_H_


void i2c_init_cpus(int speed, int slaveaddr);

int axp_i2c_read(unsigned char chip, unsigned char addr, unsigned char *buffer);
int axp_i2c_write(unsigned char chip, unsigned char addr, unsigned char data);


int pmu_init(u8 power_mode);
int set_ddr_voltage(int set_vol);
int set_pll_voltage(int set_vol);


#endif  /* __BASE_PMU_H_ */

