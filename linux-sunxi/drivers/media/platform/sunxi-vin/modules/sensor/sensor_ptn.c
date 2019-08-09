/*
 * A V4L2 driver for pattern Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for pattern sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR 0xFFFF

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The IMX317 i2c address
 */
#define I2C_ADDR 0x34

#define SENSOR_NUM 0x2
#define SENSOR_NAME "sensor_ptn"
#define SENSOR_NAME_2 "sensor_ptn_2"

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};


/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = HD1080_WIDTH;
	info->height = HD1080_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	 {
	 .width = 4224,
	 .height = 128,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 3840,
	 .height = 256,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 3264,
	 .height = 256,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 2688,
	 .height = 128,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 1920,
	 .height = 1088,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	{
	 .width = 1920,
	 .height = 256,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 1920,
	 .height = 128,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 1024,
	 .height = 128,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
	 {
	 .width = 256,
	 .height = 128,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 2288,
	 .vts = 8400,
	 .pclk = 288 * 1000 * 1000,
	 .mipi_bps = 720 * 1000 * 1000,
	 .fps_fixed = 30,
	 .bin_factor = 1,
	 .intg_min = 4 << 4,
	 .intg_max = (4200 - 12) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 1440 << 4,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 2816 * 16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 1, 65536 * 16, 1, 1);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0)
		return ret;

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum)
		return -ERANGE;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_dev_id;

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

module_init(init_sensor);
module_exit(exit_sensor);
