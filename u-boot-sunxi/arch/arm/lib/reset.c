/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
 *
 * (C) Copyright 2004
 * DAVE Srl
 * http://www.dave-tech.it
 * http://www.wawnet.biz
 * mailto:info@wawnet.biz
 *
 * (C) Copyright 2004 Texas Insturments
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <sunxi_board.h>

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	puts ("resetting ...\n");

	udelay (50000);				/* wait 50 ms */
/*
*Saveenv must call sunxi_flash_flush
*to synchronize the data in the FIFO
*/
#if 1
	sunxi_board_restart(0);
#else
	disable_interrupts();
	reset_cpu(0);
#endif
	/*NOTREACHED*/
	return 0;
}
