/*
 * (C) Copyright 2017
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <android_misc.h>
#include <sys_partition.h>

int get_bootloader_message(struct bootloader_message *out)
{
	u32   misc_offset = 0;

	misc_offset = sunxi_partition_get_offset_byname("misc");
	if (!misc_offset) {
		pr_msg("no misc partition is found\n");
		return -1;
	} else {
		pr_msg("misc partition found\n");
		sunxi_flash_read(misc_offset, 2048/512, (void *)out);
	}
	return 0;
}

int set_bootloader_message(const struct bootloader_message *in)
{
	u32   misc_offset = 0;

	misc_offset = sunxi_partition_get_offset_byname("misc");
	if (!misc_offset) {
		pr_msg("no misc partition is found\n");
		return -1;
	} else {
		pr_msg("misc partition found\n");
		sunxi_flash_write(misc_offset, 2048/512, (void *)in);
	}
	return 0;
}

int print_bootloader_message(const struct bootloader_message *misc_message)
{
	printf("command:%s\n", misc_message->command);
	printf("status:%s\n", misc_message->status);
	printf("vesion:%s\n", misc_message->recovery);
	return 0;
}

int do_read_misc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  misc_args[2048];
	int ret;
	struct bootloader_message *misc_message;
	misc_message = (struct bootloader_message *)misc_args;
	memset(misc_args, 0x0, 2048);

	ret = get_bootloader_message(misc_message);
	if (ret)
		return ret;
	if (argc > 1) {
		if (strcmp(argv[1], "command") == 0)
			printf("%s\n", misc_message->command);
		else if (strcmp(argv[1], "status") == 0)
			printf("%s\n", misc_message->status);
		else if (strcmp(argv[1], "version") == 0)
			printf("%s\n", misc_message->recovery);
		else {
			printf("not support %s\n", argv[1]);
			return -1;
		}
	} else {
		print_bootloader_message(misc_message);
	}
	return 0;
}

int do_write_misc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char  misc_args[2048];
	int ret, i = 1;
	struct bootloader_message *misc_message;
	misc_message = (struct bootloader_message *)misc_args;
	memset(misc_args, 0x0, 2048);

	ret = get_bootloader_message(misc_message);
	if (ret)
		return ret;
	while (i+1 < argc) {
		if (strcmp(argv[i], "-c") == 0) {
			strcpy(misc_message->command, argv[i+1]);
			i += 2;
			continue;
		}

		if (strcmp(argv[i], "-s") == 0) {
			strcpy(misc_message->status, argv[i+1]);
			i += 2;
			continue;
		}

		if (strcmp(argv[i], "-v") == 0) {
			strcpy(misc_message->recovery, argv[i+1]);
			i += 2;
			continue;
		}
		printf("not support %s\n", argv[i]);
		return -1;
	}
	ret = set_bootloader_message((struct bootloader_message *)misc_args);
	if (ret)
		return ret;
	return 0;
}

U_BOOT_CMD(
	read_misc,	2,	1,	do_read_misc,
	"read_misc   -  read misc message\n",
	"read_misc [command] [status] [version]"
);

U_BOOT_CMD(
	write_misc,	7,	1,	do_write_misc,
	"write_misc   -  write misc message\n",
	"write_misc [-c command] [-s status] [-v version]"
);
