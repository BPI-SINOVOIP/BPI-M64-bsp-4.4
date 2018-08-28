/*
 * (C) Copyright 2017
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <sunxi_flash.h>
#include <sys_partition.h>
#include <private_toc.h>

int get_boot0_extend_msg(struct boot0_extend_msg *out)
{
	int ret;
	int read_len=512;
	char temp[512];
	ret = sunxi_flash_phyread(BOOT0_EXTEND_MSG_START_SECTOR_IN_SDMMC, read_len/512, temp);
	if(!ret)
	{
		return -1;
	}
	memcpy(out,temp,sizeof(struct boot0_extend_msg));
	return 0;
}

int set_boot0_extend_msg(struct boot0_extend_msg *in)
{
	int ret;
	int length=512;
	char temp[512];
	memset(temp,0,512);
	memcpy(temp,in,sizeof(struct boot0_extend_msg));
	ret = sunxi_flash_phywrite(BOOT0_EXTEND_MSG_START_SECTOR_IN_SDMMC, length/512, temp);
	if(!ret)
	{
		return -1;
	}
	return 0;
}

int print_boot0_extend_msg(const struct boot0_extend_msg *boot0_extend_message)
{
	printf("magic:%x\n", boot0_extend_message->magic);
	printf("dtb index:%d\n", boot0_extend_message->dtb_index);
	return 0;
}

int do_read_boot0_extend_msg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  boot0_extend_args[2048];
	int ret;
	struct boot0_extend_msg *boot0_extend_message;
	boot0_extend_message = (struct boot0_extend_msg *)boot0_extend_args;
	memset(boot0_extend_args, 0x0, 2048);

	ret = get_boot0_extend_msg(boot0_extend_message);
	if (ret)
		return ret;
	if (argc > 1) {
		if (strcmp(argv[1], "magic") == 0)
			printf("%x\n", boot0_extend_message->magic);
		else if (strcmp(argv[1], "dtb_index") == 0)
			printf("%d\n", boot0_extend_message->dtb_index);
		else {
			printf("not support %s\n", argv[1]);
			return -1;
		}
	} else {
			print_boot0_extend_msg(boot0_extend_message);
	}
	return 0;
}

int do_write_boot0_extend_msg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  boot0_extend_args[2048];
	int ret, i = 1;
	struct boot0_extend_msg *boot0_extend_message;
	boot0_extend_message = (struct boot0_extend_msg *)boot0_extend_args;
	memset(boot0_extend_args, 0x0, 2048);

	ret = get_boot0_extend_msg(boot0_extend_message);
	if (ret)
		return ret;
	while (i+1 < argc) {
		if (strcmp(argv[i], "--dtb_index") == 0) {
			boot0_extend_message->dtb_index = (__u32)simple_strtoul(argv[i+1], NULL, 16);
			i += 2;
			continue;
		}
			printf("not support %s\n", argv[i]);
			return -1;
	}
	ret = set_boot0_extend_msg((struct boot0_extend_msg *)boot0_extend_args);
	if (ret)
		return ret;
	return 0;
}

U_BOOT_CMD(
		read_boot0_extend,      2,      1,      do_read_boot0_extend_msg,
		"read_boot0_extend   -  read boot0 extend message\n",
		"read_boot0_extend [dtb_index]"
);

U_BOOT_CMD(
		write_boot0_extend,     3,      1,      do_write_boot0_extend_msg,
		"write_boot0_extend   -  write boot0 extend message\n",
		"write_boot0_extend [--dtb_index xxx]"
);
