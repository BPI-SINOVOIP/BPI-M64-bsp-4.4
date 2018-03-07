
/*
 * sunxi_scaler.h for scaler and osd v4l2 subdev
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _SUNXI_SCALER_H_
#define _SUNXI_SCALER_H_
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include "vipp_reg.h"

enum scaler_pad {
	SCALER_PAD_SINK,
	SCALER_PAD_SOURCE,
	SCALER_PAD_NUM,
};

struct scaler_para {
	u32 xratio;
	u32 yratio;
	u32 w_shift;
	u32 width;
	u32 height;
};

struct scaler_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct media_pad scaler_pads[SCALER_PAD_NUM];
	struct v4l2_event event;
	struct v4l2_mbus_framefmt formats[SCALER_PAD_NUM];
	struct platform_device *pdev;
	unsigned int is_empty;
	unsigned int is_osd_en;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	wait_queue_head_t wait;
	void __iomem *base;
	struct vin_mm vipp_reg;
	struct vin_mm osd_para;
	struct vin_mm osd_stat;
	struct {
		struct v4l2_rect request;
		struct v4l2_rect active;
	} crop;
	struct scaler_para para;
	struct list_head scaler_list;
};

struct v4l2_subdev *sunxi_scaler_get_subdev(int id);
int sunxi_vipp_get_osd_stat(int id, unsigned int *stat);
int sunxi_osd_change_sc_fmt(int id, enum vipp_format out_fmt, int en);
int sunxi_scaler_platform_register(void);
void sunxi_scaler_platform_unregister(void);

#endif /*_SUNXI_SCALER_H_*/
