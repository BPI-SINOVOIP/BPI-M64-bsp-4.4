/*
 * sunxi_load_jpeg/sunxi_load_jpeg.c
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#include <common.h>
#include <malloc.h>
#include <sys_config.h>
#include <fdt_support.h>

#include <sys_partition.h>
#include <tinyjpeg.h>
#include <bmp_layout.h>

struct boot_fb_private {
	char *base;
	int width;
	int height;
	int bpp;
	int stride;
	int id;
};

static int read_jpeg(const char *filename, char *buf, unsigned int buf_size)
{
#ifndef BOOTLOGO_PARTITION_NAME
#define BOOTLOGO_PARTITION_NAME "bootloader"
#endif

#ifdef CONFIG_CMD_FAT
	char *argv[6];
	char part_num[16] = {0};
	char len[16] = {0};
	char read_addr[32] = {0};
	char file_name[32] = {0};

	snprintf(part_num, 16, "%d:0",
		 sunxi_partition_get_partno_byname(BOOTLOGO_PARTITION_NAME));
	snprintf(len, 16, "%ld", (ulong)buf_size);
	snprintf(read_addr, 32, "%lx", (ulong)buf);
	strncpy(file_name, filename, 32);
	argv[0] = "fatload";
	argv[1] = "sunxi_flash";
	argv[2] = part_num;
	argv[3] = read_addr;
	argv[4] = file_name;
	argv[5] = len;
	return do_fat_fsload(0, 0, 5, argv);
#else
	int size;
	unsigned int start_block, nblock;

	start_block =
	    sunxi_partition_get_offset_byname(BOOTLOGO_PARTITION_NAME);
	nblock = sunxi_partition_get_size_byname(BOOTLOGO_PARTITION_NAME);
	size = sunxi_flash_read(start_block, nblock, (void *)buf);
	return size > 0 ? (size << 10) : 0;
#endif
}

static int get_disp_fdt_node(void)
{
	static int fdt_node = -1;

	if (0 <= fdt_node)
		return fdt_node;
	/* notice: make sure we use the only one nodt "disp". */
	fdt_node = fdt_path_offset(working_fdt, "disp");
	assert(fdt_node >= 0);
	return fdt_node;
}

static void disp_getprop_by_name(int node, const char *name, uint32_t *value,
				 uint32_t defval)
{
	if (fdt_getprop_u32(working_fdt, node, name, value) < 0) {
		printf("fetch script data disp.%s fail. using defval=%d\n",
		       name, defval);
		*value = defval;
	}
}

static int malloc_buf(char **buff, int size)
{
	int ret;
#if defined(CONFIG_SUNXI_LOGBUFFER)
	*buff = (void *)(CONFIG_SYS_SDRAM_BASE + gd->ram_size -
			 SUNXI_DISPLAY_FRAME_BUFFER_SIZE);
	ret = 1;
#elif defined(SUNXI_DISPLAY_FRAME_BUFFER_ADDR)
	*buff = (void *)(SUNXI_DISPLAY_FRAME_BUFFER_ADDR);
	ret = 2;
#else
	*buff = malloc((size + 512 + 0x1000));
	if (NULL == bmp_buff) {
		pr_error("bmp buffer malloc error!\n");
		return -1;
	}
	ret = 3;
#endif
	return ret;
}

static int request_fb(struct boot_fb_private *fb, unsigned int width,
		      unsigned int height)
{
	int format = 0;
	int fbsize = 0;

	disp_getprop_by_name(get_disp_fdt_node(), "fb0_format",
			     (uint32_t *)(&format), 0);
	if (8 == format) /* DISP_FORMAT_RGB_888 = 8; */
		fb->bpp = 24;
	else
		fb->bpp = 32;
	fb->width = width;
	fb->height = height;
	fb->stride = width * fb->bpp >> 3;
	fbsize = fb->stride * fb->height + sizeof(bmp_header_t); /*add bmp header len*/
	fb->id = 0;
	malloc_buf(&fb->base, fbsize);

	if (NULL != fb->base)
		memset((void *)(fb->base), 0x0, fbsize);

	return 0;
}

static int release_fb(struct boot_fb_private *fb)
{
	flush_cache((uint)fb->base, fb->stride * fb->height);
	return 0;
}

static int add_bmp_header(struct boot_fb_private *fb)
{
	bmp_header_t bmp_header;
	memset(&bmp_header, 0, sizeof(bmp_header_t));
	bmp_header.signature[0] = 'B';
	bmp_header.signature[1] = 'M';
	bmp_header.data_offset = sizeof(bmp_header_t);
	bmp_header.image_size = fb->stride * fb->height;
	bmp_header.file_size = bmp_header.image_size + sizeof(bmp_header_t);
	bmp_header.size = sizeof(bmp_header_t) - 14;
	bmp_header.width = fb->width;
	bmp_header.height = -fb->height;
	bmp_header.planes = 1;
	bmp_header.bit_count = fb->bpp;
	memcpy(fb->base, &bmp_header, sizeof(bmp_header_t));
	return 0;
}

static int board_display_update_para_for_kernel(char *name, int value)
{
#ifndef CONFIG_SUNXI_MULITCORE_BOOT
	int node;
	int ret = -1;

	node = fdt_path_offset(working_fdt, "disp");
	if (node < 0) {
		pr_error("%s:disp_fdt_nodeoffset %s fail\n", __func__, "disp");
		goto exit;
	}

	ret = fdt_setprop_u32(working_fdt, node, name, (uint32_t)value);
	if (ret < 0)
		pr_error("fdt_setprop_u32 %s.%s(0x%x) fail.err code:%s\n",
				       "disp", name, value, fdt_strerror(ret));
	else
		ret = 0;

exit:
	return ret;
#else
	return sunxi_fdt_getprop_store(working_fdt, "disp", name, value);
#endif
}

int sunxi_load_jpeg(const char *filename)
{
	int length_of_file;
	unsigned int width, height;
	unsigned char *buf = (unsigned char *)CONFIG_SYS_SDRAM_BASE;
	;
	struct jdec_private *jdec;
	struct boot_fb_private fb;
	int output_format = -1;

	/* Load the Jpeg into memory */
	length_of_file =
	    read_jpeg(filename, (char *)buf, SUNXI_DISPLAY_FRAME_BUFFER_SIZE);
	if (0 >= length_of_file) {
		return -1;
	}

	/* tinyjpeg: init and parse header */
	jdec = tinyjpeg_init();
	if (jdec == NULL) {
		printf("tinyjpeg_init failed\n");
		return -1;
	}
	if (tinyjpeg_parse_header(jdec, buf, (unsigned int)length_of_file) <
	    0) {
		printf("tinyjpeg_parse_header failed: %s\n",
		       tinyjpeg_get_errorstring(jdec));
		return -1;
	}

	/* Get the size of the image, request the same size fb */
	tinyjpeg_get_size(jdec, &width, &height);
	memset((void *)&fb, 0x0, sizeof(fb));
	request_fb(&fb, width, height);
	if (NULL == fb.base) {
		printf("fb.base is null !!!");
		return -1;
	}
	char *tmp = fb.base + sizeof(bmp_header_t);
	tinyjpeg_set_components(jdec, (unsigned char **)&(tmp), 1);

	if (32 == fb.bpp)
		output_format = TINYJPEG_FMT_BGRA32;
	else if (24 == fb.bpp)
		output_format = TINYJPEG_FMT_BGR24;
	if (tinyjpeg_decode(jdec, output_format) < 0) {
		printf("tinyjpeg_decode failed: %s\n",
		       tinyjpeg_get_errorstring(jdec));
		return -1;
	}

	add_bmp_header(&fb);

	release_fb(&fb);
	board_display_update_para_for_kernel("fb_base", (uint)fb.base);
	free(jdec);

	return 0;
}
