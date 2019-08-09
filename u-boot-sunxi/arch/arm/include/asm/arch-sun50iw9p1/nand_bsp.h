/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef __BSP_NAND_H__
#define __BSP_NAND_H__

int nand_uboot_init(int boot_mode);

int nand_uboot_exit(void);

uint nand_uboot_read(uint start, uint sectors, void *buffer);

uint nand_uboot_write(uint start, uint sectors, void *buffer);

int nand_download_boot0(uint length, void *buffer);

int nand_download_uboot(uint length, void *buffer);

int nand_uboot_erase(int user_erase);

uint nand_uboot_get_flash_info(void *buffer, uint length);

uint nand_uboot_set_flash_info(void *buffer, uint length);

uint nand_uboot_get_flash_size(void);


#endif  /* ifndef __NAND_LOGIC_H__ */



