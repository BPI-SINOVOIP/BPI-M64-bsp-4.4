/*
 * drivers/soc/sunxi/pm_legacy/mem_sram.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "pm_types.h"
#include "pm_i.h"

/*
*******************************************************************************
*                                       MEM SRAM SAVE
*
* Description: mem sram save.
*
* Arguments  : none.
*
* Returns    : 0/-1;
******************************************************************************
*/
__s32 mem_sram_save(struct sram_state *psram_state)
{
	int i = 0;

	/*save all the sram reg */
	for (i = 0; i < (SRAM_REG_LENGTH); i++) {
		psram_state->sram_reg_back[i] =
		    *(volatile __u32 *)(IO_ADDRESS(AW_SRAMCTRL_BASE) +
					i * 0x04);
	}
	return 0;
}

/*
******************************************************************************
*                                       MEM sram restore
*
* Description: mem sram restore.
*
* Arguments  : none.
*
* Returns    : 0/-1;
******************************************************************************
*/
__s32 mem_sram_restore(struct sram_state *psram_state)
{
	int i = 0;

	/*restore all the sram reg */
	for (i = 0; i < (SRAM_REG_LENGTH); i++) {
		*(volatile __u32 *)(IO_ADDRESS(AW_SRAMCTRL_BASE) + i * 0x04) =
		    psram_state->sram_reg_back[i];
	}

	return 0;
}
