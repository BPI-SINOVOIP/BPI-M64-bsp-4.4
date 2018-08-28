/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <linux/types.h>
#include <config.h>
//#include "bl_common.h"
#include <private_uboot.h>
#include <asm/io.h>
#include <private_toc.h>
#include <boot0_helper.h>
#include <private_boot0.h>
#include <sunxi_cfg.h>
#include <stdarg.h>
#include <vsprintf.h>

#define HEADER_OFFSET     (0x4000)

extern const boot0_file_head_t  BT0_head;

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

extern size_t strlen(const char * s);

int toc1_flash_read(u32 start_sector, u32 blkcnt, void *buff)
{
	memcpy(buff, (void *)(CONFIG_BOOTPKG_STORE_IN_DRAM_BASE + 512 * start_sector), 512 * blkcnt);

	return blkcnt;
}

#if 0
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
uint toc1_item_read(struct sbrom_toc1_item_info *p_toc_item, void * p_dest, u32 buff_len)
{
	u32 to_read_blk_start = 0;
	u32 to_read_blk_sectors = 0;
	s32 ret = 0;

	if( buff_len  < p_toc_item->data_len )
	{
		printf("PANIC : Toc1_item_read() error --1--,buff error\n");

		return 0;
	}

	to_read_blk_start   = (p_toc_item->data_offset)>>9;
	to_read_blk_sectors = (p_toc_item->data_len + 0x1ff)>>9;

	ret = toc1_flash_read(to_read_blk_start, to_read_blk_sectors, p_dest);
	if(ret != to_read_blk_sectors)
	{
		printf("PANIC: toc1_item_read() error --2--, read error\n");

		return 0;
	}

	return ret * 512;
}

#endif

int sunxi_deassert_arisc(void)
{
	printf("set arisc reset to de-assert state\n");
	{
		volatile unsigned long value;
		value = readl(SUNXI_RCPUCFG_BASE + 0x0);
		value &= ~1;
		writel(value, SUNXI_RCPUCFG_BASE + 0x0);
		value = readl(SUNXI_RCPUCFG_BASE + 0x0);
		value |= 1;
		writel(value, SUNXI_RCPUCFG_BASE + 0x0);
	}

	return 0;
}

int load_fip(int *use_monitor)
{
	int i;
	int dtb_loaded = 0, soccfg_loaded = 0;
	int default_dtb_offset = 0, default_dtb_len = 0;
	int default_soccfg_offset = 0, default_soccfg_len = 0;

#ifdef CONFIG_BOARD_ID_GPIO
	int dtb_index = 0;
#endif

#ifdef CONFIG_SUNXI_MOZART
	int dtb_index = 0;
#endif

	void *dram_para_addr = (void *)BT0_head.prvt_head.dram_para;

	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

#ifdef CONFIG_SUNXI_MOZART
	struct boot0_extend_msg  *boot0_extend = NULL;
#endif

	char item_dtb_name_with_index[64+4];
	char item_soccfg_name_with_index[64+4];
	sprintf(item_dtb_name_with_index, "%s", ITEM_DTB_NAME);
	sprintf(item_soccfg_name_with_index, "%s", ITEM_SOCCFG_NAME);

	toc1_head = (struct sbrom_toc1_head_info *)CONFIG_BOOTPKG_STORE_IN_DRAM_BASE;
	item_head = (struct sbrom_toc1_item_info *)(CONFIG_BOOTPKG_STORE_IN_DRAM_BASE + sizeof(struct sbrom_toc1_head_info));

#ifdef CONFIG_SUNXI_MOZART
	boot0_extend = (struct boot0_extend_msg *)CONFIG_BOOT0_EXTEND_MSG_STORE_IN_DRAM_BASE;
#ifdef BOOT_DEBUG
	printf("*******************BOOT0 EXTEND Message*************************\n");
	printf("Boot0_extend_magic      = %x\n",   boot0_extend->magic);
	printf("Boot0_extend_dtb_index      = %d\n",   boot0_extend->dtb_index);
	printf("****************************************************************\n");
#endif
	dtb_index = boot0_extend->dtb_index;
	if(dtb_index >= CONFIG_SUNXI_MOZART_DTB_COUNT)
	{
		dtb_index = 0;
	}
	if(dtb_index != 0)
	{
		sprintf(item_dtb_name_with_index, "%s-%d", ITEM_DTB_NAME, dtb_index);
		sprintf(item_soccfg_name_with_index, "%s-%d", ITEM_SOCCFG_NAME, dtb_index);
	}
#endif

#ifdef BOOT_DEBUG
	printf("*******************TOC1 Head Message*************************\n");
	printf("Toc_name          = %s\n",   toc1_head->name);
	printf("Toc_magic         = 0x%x\n", toc1_head->magic);
	printf("Toc_add_sum	      = 0x%x\n", toc1_head->add_sum);

	printf("Toc_serial_num    = 0x%x\n", toc1_head->serial_num);
	printf("Toc_status        = 0x%x\n", toc1_head->status);

	printf("Toc_items_nr      = 0x%x\n", toc1_head->items_nr);
	printf("Toc_valid_len     = 0x%x\n", toc1_head->valid_len);
	printf("TOC_MAIN_END      = 0x%x\n", toc1_head->end);
	printf("***************************************************************\n\n");
#endif

#ifdef CONFIG_BOARD_ID_GPIO

	int board_id_gpio_valid_num = 0;
	normal_gpio_cfg board_id_gpio[CONFIG_BOARD_ID_GPIO_MAX_NUM];
	for (i = 0; i < CONFIG_BOARD_ID_GPIO_MAX_NUM; i++)
	{
		if (!toc1_head->board_id_simple_gpio[i].port)
		{
			board_id_gpio_valid_num = i;
			break;
		}
		board_id_gpio[i].port = toc1_head->board_id_simple_gpio[i].port;
		board_id_gpio[i].port_num = toc1_head->board_id_simple_gpio[i].port_num;
		board_id_gpio[i].mul_sel = 0;   /* input */
		board_id_gpio[i].pull = 1;
		board_id_gpio[i].drv_level = -1;
		board_id_gpio[i].data = 1;
	}
	boot_set_gpio(&board_id_gpio, board_id_gpio_valid_num, 1);
	boot_get_gpio(&board_id_gpio, board_id_gpio_valid_num, 1);

	dtb_index = 0;
	for (i = 0; i < board_id_gpio_valid_num; i++)
		dtb_index = dtb_index * 2 + board_id_gpio[i].data;

	if (dtb_index) {
		sprintf(item_dtb_name_with_index, "%s-%d", ITEM_DTB_NAME, dtb_index);
		sprintf(item_soccfg_name_with_index, "%s-%d", ITEM_SOCCFG_NAME, dtb_index);
	}
	printf("board id enabled, will load %s\n", item_dtb_name_with_index);
#endif
	//init
	toc1_item = item_head;
	for(i=0;i<toc1_head->items_nr;i++,toc1_item++)
	{
#ifdef BOOT_DEBUG
		printf("\n*******************TOC1 Item Message*************************\n");
		printf("Entry_name        = %s\n",   toc1_item->name);
		printf("Entry_data_offset = 0x%x\n", toc1_item->data_offset);
		printf("Entry_data_len    = 0x%x\n", toc1_item->data_len);

		printf("encrypt	          = 0x%x\n", toc1_item->encrypt);
		printf("Entry_type        = 0x%x\n", toc1_item->type);
		printf("run_addr          = 0x%x\n", toc1_item->run_addr);
		printf("index             = 0x%x\n", toc1_item->index);
		printf("Entry_end         = 0x%x\n", toc1_item->end);
		printf("***************************************************************\n\n");
#endif
		printf("Entry_name        = %s\n",   toc1_item->name);

		if(strncmp(toc1_item->name, ITEM_UBOOT_NAME, sizeof(ITEM_UBOOT_NAME)) == 0)
		{
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_SYS_TEXT_BASE);
		}
		else if(strncmp(toc1_item->name, ITEM_MONITOR_NAME, sizeof(ITEM_MONITOR_NAME)) == 0)
		{
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)BL31_BASE);
			*use_monitor = 1;
		}
		else if(strncmp(toc1_item->name, ITEM_SCP_NAME, sizeof(ITEM_SCP_NAME)) == 0)
		{
			toc1_flash_read(toc1_item->data_offset/512, CONFIG_SYS_SRAMA2_SIZE/512, (void *)SCP_SRAM_BASE);
			toc1_flash_read((toc1_item->data_offset+0x18000)/512, SCP_DRAM_SIZE/512, (void *)SCP_DRAM_BASE);
			memcpy((void *)(SCP_SRAM_BASE+HEADER_OFFSET+SCP_DRAM_PARA_OFFSET),dram_para_addr,SCP_DARM_PARA_NUM * sizeof(int));
			sunxi_deassert_arisc();
		}
		else if(strncmp(toc1_item->name, ITEM_LOGO_NAME, sizeof(ITEM_LOGO_NAME)) == 0) {
			*(uint *)(SUNXI_LOGO_COMPRESSED_LOGO_SIZE_ADDR) = toc1_item->data_len;
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)SUNXI_LOGO_COMPRESSED_LOGO_BUFF);
		}
		else if(strncmp(toc1_item->name, ITEM_SHUTDOWNCHARGE_LOGO_NAME, sizeof(ITEM_SHUTDOWNCHARGE_LOGO_NAME)) == 0) {
			*(uint *)(SUNXI_SHUTDOWN_CHARGE_COMPRESSED_LOGO_SIZE_ADDR) = toc1_item->data_len;
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)SUNXI_SHUTDOWN_CHARGE_COMPRESSED_LOGO_BUFF);
		}
		else if(strncmp(toc1_item->name, ITEM_ANDROIDCHARGE_LOGO_NAME, sizeof(ITEM_ANDROIDCHARGE_LOGO_NAME)) == 0) {
			*(uint *)(SUNXI_ANDROID_CHARGE_COMPRESSED_LOGO_SIZE_ADDR) = toc1_item->data_len;
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)SUNXI_ANDROID_CHARGE_COMPRESSED_LOGO_BUFF);
		}
		else if(strlen(toc1_item->name) == strlen(item_dtb_name_with_index) &&
				strncmp(toc1_item->name, item_dtb_name_with_index, strlen(item_dtb_name_with_index)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_DTB_STORE_IN_DRAM_BASE);
			dtb_loaded = 1;
		}
		else if(strlen(toc1_item->name) == strlen(item_soccfg_name_with_index) &&
				strncmp(toc1_item->name, item_soccfg_name_with_index, strlen(item_soccfg_name_with_index)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_SOCCFG_STORE_IN_DRAM_BASE);
			soccfg_loaded = 1;
		}
		else if(strncmp(toc1_item->name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME)) == 0) {
			default_dtb_offset = toc1_item->data_offset/512;
			default_dtb_len = (toc1_item->data_len+511)/512;
		}
		else if(strncmp(toc1_item->name, ITEM_SOCCFG_NAME, sizeof(ITEM_SOCCFG_NAME)) == 0) {
			default_soccfg_offset = toc1_item->data_offset/512;
			default_soccfg_len = (toc1_item->data_len+511)/512;
		}


	}
	if(*use_monitor)
	{
		struct spare_boot_head_t* header;
		/* Obtain a reference to the image by querying the platform layer */
		header = (struct spare_boot_head_t* )CONFIG_SYS_TEXT_BASE;
		header->boot_data.monitor_exist = 1;
	}
	if(dtb_loaded == 0)
	{
		printf("Warning:can not find %s in toc1\n", item_dtb_name_with_index);
		if(default_dtb_offset != 0 && default_dtb_len != 0)
		{
			printf("default load %s\n", ITEM_DTB_NAME);
			toc1_flash_read(default_dtb_offset,default_dtb_len, (void *)CONFIG_DTB_STORE_IN_DRAM_BASE);
		}
	}
	if(soccfg_loaded == 0)
	{
		printf("Warning:can not find %s in toc1\n", item_soccfg_name_with_index);
		if(default_soccfg_offset != 0 && default_soccfg_len != 0)
		{
			printf("default load %s\n", ITEM_SOCCFG_NAME);
			toc1_flash_read(default_soccfg_offset,default_soccfg_len, (void *)CONFIG_SOCCFG_STORE_IN_DRAM_BASE);
		}
	}
	return 0;
}



