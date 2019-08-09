/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */
#include <linux/types.h>
#include <config.h>
#include <private_uboot.h>
#include <asm/io.h>
#include <private_toc.h>
#include <boot0_helper.h>
#include <sunxi_cfg.h>
#include <stdarg.h>
#include <vsprintf.h>
#include <private_tee.h>
#include <private_boot0.h>
extern const boot0_file_head_t  BT0_head;
extern size_t strlen(const char * s);

int toc1_flash_read(u32 start_sector, u32 blkcnt, void *buff)
{
	memcpy_align16(buff,
		(void *)(CONFIG_BOOTPKG_STORE_IN_DRAM_BASE + 512 * start_sector),
		512 * blkcnt);
	return blkcnt;
}

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


int load_fip(int *use_monitor)
{
	int i;
#ifdef CONFIG_BOARD_ID_GPIO
	int dtb_index = 0;
#endif

	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;

	struct sbrom_toc1_item_info  *toc1_item = NULL;

	char item_dtb_name_with_index[64+4];
	char item_soccfg_name_with_index[64+4];
	sprintf(item_dtb_name_with_index, "%s", ITEM_DTB_NAME);
	sprintf(item_soccfg_name_with_index, "%s", ITEM_SOCCFG_NAME);

	toc1_head = (struct sbrom_toc1_head_info *)CONFIG_BOOTPKG_STORE_IN_DRAM_BASE;
	item_head = (struct sbrom_toc1_item_info *)(CONFIG_BOOTPKG_STORE_IN_DRAM_BASE + sizeof(struct sbrom_toc1_head_info));

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

		if (strncmp(toc1_item->name, ITEM_UBOOT_NAME, sizeof(ITEM_UBOOT_NAME)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_SYS_TEXT_BASE);
		} else if (strncmp(toc1_item->name, ITEM_OPTEE_NAME, sizeof(ITEM_OPTEE_NAME)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)OPTEE_BASE);
			struct spare_optee_head *tee_head = (struct spare_optee_head *)OPTEE_BASE;
			memcpy(tee_head->dram_para, BT0_head.prvt_head.dram_para, 32*sizeof(int));
			*use_monitor = 1;
		} else if(strlen(toc1_item->name) == strlen(item_dtb_name_with_index) &&
				strncmp(toc1_item->name, item_dtb_name_with_index, strlen(item_dtb_name_with_index)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_DTB_STORE_IN_DRAM_BASE);
		} else if(strlen(toc1_item->name) == strlen(item_soccfg_name_with_index) &&
				strncmp(toc1_item->name, item_soccfg_name_with_index, strlen(item_soccfg_name_with_index)) == 0) {
			toc1_flash_read(toc1_item->data_offset/512, (toc1_item->data_len+511)/512, (void *)CONFIG_SOCCFG_STORE_IN_DRAM_BASE);
		} else {
			printf("unknow boot package file \n");
			return -1;
		}

	}
	if (*use_monitor) {
		struct spare_boot_head_t *header;
		/* Obtain a reference to the image by querying the platform layer */
		header = (struct spare_boot_head_t *)CONFIG_SYS_TEXT_BASE;
		header->boot_data.secureos_exist = 1;
	}
	return 0;
}
