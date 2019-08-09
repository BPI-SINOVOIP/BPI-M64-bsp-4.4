/*
 * drivers\media\platform\aio.c
 * (C) Copyright 2016-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * zhangjingzhou<zhangjingzhou@allwinnertech.com>
 *
 * Driver for embedded vision engine(AIO).
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include "aio.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/signal.h>
#include <linux/mpp.h>

static struct dentry *debugfsP;

/* ai dev attr */
static char ai_info[] = "--------------------AI Dev ATTR(V1.0)------------------------\n";
static char ai_devenable[32] = "ai_enable: 0\n";
static char ai_channelcnt[32] = "ai_chncnt: 0\n";
static char ai_cardtype[64] = "ai_cardtype: 0(0-codec;1-linein)\n";
static char ai_samplerate[32] = "ai_samplerate: 0\n";
static char ai_bitwidth[32] = "ai_bitwidth: 0\n";
static char ai_trackcnt[32] = "ai_trackcnt: 0\n";
static char ai_bMute[32] = "ai_bMute: 0\n";
static char ai_volume[32] = "ai_volume: 0\n";

/* ao dev attr */
static char ao_info[] = "--------------------AO Dev ATTR(V1.0)------------------------\n";
static char ao_devenable[32] = "ao_enable: 0\n";
static char ao_channelcnt[32] = "ao_chncnt: 0\n";
static char ao_cardtype[64] = "ao_cardtype: 0(0-codec;1-hdmi)\n";
static char ao_samplerate[32] = "ao_samplerate: 0\n";
static char ao_bitwidth[32] = "ao_bitwidth: 0\n";
static char ao_trackcnt[32] = "ao_trackcnt: 0\n";
static char ao_bMute[32] = "ao_bMute: 0\n";
static char ao_volume[32] = "ao_volume: 0\n";

static int aio_write_to_usr(char __user *dst, char *src, size_t count, loff_t *ppos)
{
	int len = strlen(src);

	if (len) {
		if (*ppos >= len)
			return 0;

		if (count >= len)
			count = len;

		if (count > (len - *ppos))
			count = (len - *ppos);

		if (copy_to_user(dst, src, count)) {
			pr_warn("copy_to_user fail\n");
			return 0;
		}
		*ppos += count;
	} else {
		count = 0;
	}
	return count;
}

static int aio_info_get(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int wr_cnt = 0;
	char info[1024];

	sprintf(info, "%s%s%s%s%s%s%s%s%s\n%s%s%s%s%s%s%s%s%s",
			ai_info, ai_devenable, ai_channelcnt,
			ai_cardtype, ai_samplerate, ai_bitwidth,
			ai_trackcnt, ai_bMute, ai_volume,
			ao_info, ao_devenable, ao_channelcnt,
			ao_cardtype, ao_samplerate, ao_bitwidth,
			ao_trackcnt, ao_bMute, ao_volume);

	wr_cnt = aio_write_to_usr(buf, info, count, ppos);

	return wr_cnt;
}

static int aio_info_set(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char info[64] = {0};
	char *dst_ptr;
	int match_flag = 1;

	if (count > 64) {
		pr_warn("copy_from_user fail, count:%d too big!\n", count);
		return count;
	}

	if (copy_from_user(info, buf, count)) {
		pr_warn("copy_from_user fail\n");
		return 0;
	}

	switch (info[0]) {
	case '0':
		dst_ptr = ai_devenable;
		break;
	case '1':
		dst_ptr = ai_channelcnt;
		break;
	case '2':
		dst_ptr = ai_cardtype;
		break;
	case '3':
		dst_ptr = ai_samplerate;
		break;
	case '4':
		dst_ptr = ai_bitwidth;
		break;
	case '5':
		dst_ptr = ai_trackcnt;
		break;
	case '6':
		dst_ptr = ai_bMute;
		break;
	case '7':
		dst_ptr = ai_volume;
		break;

	case '8':
		dst_ptr = ao_devenable;
		break;
	case '9':
		dst_ptr = ao_channelcnt;
		break;
	case 'a':
		dst_ptr = ao_cardtype;
		break;
	case 'b':
		dst_ptr = ao_samplerate;
		break;
	case 'c':
		dst_ptr = ao_bitwidth;
		break;
	case 'd':
		dst_ptr = ao_trackcnt;
		break;
	case 'e':
		dst_ptr = ao_bMute;
		break;
	case 'f':
		dst_ptr = ao_volume;
		break;
	default:
		pr_warn("unknown aio debugfs setting[%s]!\n", info);
		pr_warn("MUST be: echo x[0,f] yz > path!\n");
		match_flag = 0;
		break;

	}
	while ((*dst_ptr != '\0') && !isdigit(*dst_ptr))
		++dst_ptr;

	if (match_flag && strlen(info) < 15)
		snprintf(dst_ptr, strlen(info), "%s", info + 2);
	else
		pr_warn("update fail! match_flag:%d, strlen: %d\n", match_flag, strlen(info));

	return count;
}

static int aio_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int aio_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations info_ops = {
	.write = aio_info_set,
	.read = aio_info_get,
	.open = aio_info_open,
	.release = aio_info_release,
};

static int __init debugfs_aio_init(void)
{
#ifdef CONFIG_SUNXI_MPP
	debugfsP = debugfs_create_file("sunxi-aio", 0644, debugfs_mpp_root, NULL, &info_ops);
#else
	debugfsP = NULL;
#endif
	return 0;
}

static void __exit debugfs_aio_exit(void)
{
	if (debugfsP)
		debugfs_remove_recursive(debugfsP);
}

module_init(debugfs_aio_init);
module_exit(debugfs_aio_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("zhangjingzhou<zhangjingzhou@allwinnertech.com>");
MODULE_DESCRIPTION("AIO debuginfo driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:AIO(AW1721)");
