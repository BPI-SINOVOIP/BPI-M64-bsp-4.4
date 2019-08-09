/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef _TWIC_REGS_H_
#define _TWIC_REGS_H_

#include <linux/types.h>
/*
*********************************************************************************************************
*   Interrupt controller define
*********************************************************************************************************
*/
#define TWI_CONTROL_OFFSET             0x400
#define SUNXI_I2C_CONTROLLER             3

struct sunxi_twi_reg {
    volatile unsigned int addr;        /* slave address     */
    volatile unsigned int xaddr;       /* extend address    */
	volatile unsigned int data;        /* data              */
    volatile unsigned int ctl;         /* control           */
    volatile unsigned int status;      /* status            */
    volatile unsigned int clk;         /* clock             */
    volatile unsigned int srst;        /* soft reset        */
    volatile unsigned int eft;         /* enhanced future   */
    volatile unsigned int lcr;         /* line control      */
    volatile unsigned int dvfs;        /* dvfs control      */
};


#endif  /* _TWIC_REGS_H_ */

