/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
 /* this is the user layer ioctl and struct */
#ifndef _SUNXI_DRM_UAPI_H_
#define _SUNXI_DRM_UAPI_H_
#include <linux/sunxi_tr.h>

#define SUNXI_MAX_PLANE 16
/*
enum tr_mode {
	TR_ROT_0        = 0x0,//rotate clockwise 0 ROTgree
	TR_ROT_90       = 0x1,//rotate clockwise 90 ROTgree
	TR_ROT_180      = 0x2,//rotate clockwise 180 ROTgree
	TR_ROT_270      = 0x3,//rotate clockwise 270 ROTgree
	TR_HFLIP        = 0x4,//horizontal flip
	TR_HFLIP_ROT_90 = 0x5,//first rotate clockwise 90 ROTgree then horizontal flip
	TR_VFLIP        = 0x6,//vertical flip
	TR_VFLIP_ROT_90 = 0x7,//first rotate clockwise 90 ROTgree then vertical flip
};
*/
enum tr_cmd {
    TR_CMD_AQUIRE = 1,
	TR_CMD_COMMIT,
	TR_CMD_QUERY,
	TR_CMD_RELEASE,
};

enum sunxi_drm_gem_buf_type {
	/* Physically Continuous memory and used as default. */
	SUNXI_BO_CONTIG	= 1 << 0,
	/* Physically Non-Continuous memory. */
	SUNXI_BO_NONCONTIG	= 0 << 0,
	/* non-cachable mapping. */
	SUNXI_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	SUNXI_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	SUNXI_BO_WC		= 1 << 2,
	SUNXI_BO_MASK		= SUNXI_BO_CONTIG | SUNXI_BO_CACHABLE |
					SUNXI_BO_WC
};

struct sunxi_flip_user_date {
	 int32_t   plane_id;
	__u32		zorder; /*cancel*/
	 /* unsigned int sync_id; */
	 unsigned int crtc_id;

};

struct sunxi_rotate_info {
    /* user must promise the gem handle hold for rotate */
	union{
		uint32_t src_gem_handle;
		int status;
	};
	uint32_t dst_gem_handle;
	int width;
	int pitch;
	int height;
	int depth;
	int bpp;
	unsigned int set_time;
	tr_mode mode;
	bool sleep_mode;
};

struct sunxi_rotate_cmd {
    int handle;
    enum tr_cmd cmd;
    void  *private;
};

struct sunxi_sync_gem_cmd {
    uint32_t gem_handle;
};

struct sunxi_fb_info {
	    bool in_used;
		unsigned int width;
		unsigned int height;
		unsigned int pixel_format;
		unsigned int depth;
		int bpp;
};

struct sunxi_fb_info_cmd {
	unsigned int crtc_id;
	unsigned int plane_id;
	unsigned int zorder;
	bool set_get;
};

struct sunxi_drm_get_wb_info {
	unsigned int crtc_base_id;
	unsigned int fb_base_id;
	unsigned long long data_ptr;

	/*return value*/
	unsigned int width;
	unsigned int height;
	unsigned int pitch;
	unsigned int out_format;
	unsigned int data_count;
};

struct sunxi_drm_get_fb_info {
	unsigned int base_id;
	unsigned long long data_ptr;

	unsigned int isfbdev;

	unsigned int width;
	unsigned int height;
	unsigned int pitch;
	unsigned int bpp;
	unsigned int depth;
	unsigned int pixel_format;
};

struct sunxi_drm_get_plane_info {
	unsigned int base_id;

	/*primary, overlay, cursor*/
	unsigned int type;

	unsigned int plane_id;
	unsigned int isvideo;
	unsigned int chn_id;

	unsigned int fb_base_id;
};

struct sunxi_drm_get_crtc_info {
	unsigned int crtc_base_id;

	/*crtc0/1*/
	unsigned int crtc_id;

	/*include primary, overlay, curser*/
	unsigned int plane_count;
	unsigned long long plane_id_ptr;
};

#define DRM_SUNXI_FLIP_SYNC		0x00
#define DRM_SUNXI_FENCE_SYNC	0x01
#define DRM_SUNXI_ROTATE	0x02
#define DRM_SUNXI_SYNC_GEM	0x03
#define DRM_SUNXI_INFO_FB_PLANE	0x04
#define DRM_SUNXI_GET_CRTCINFO	0x05
#define DRM_SUNXI_GET_PLANEINFO	0x06
#define DRM_SUNXI_GET_FBINFO	0x07
#define DRM_SUNXI_GET_WBINFO	0x08


#define DRM_IOCTL_SUNXI_FLIP_SYNC		DRM_IOW(DRM_COMMAND_BASE + \
		DRM_SUNXI_FLIP_SYNC, struct sunxi_flip_user_date)

#define DRM_IOCTL_SUNXI_FENCE_SYNC		DRM_IO(DRM_COMMAND_BASE + \
		DRM_SUNXI_FENCE_SYNC)

#define DRM_IOCTL_SUNXI_ROTATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SUNXI_ROTATE, struct sunxi_rotate_cmd)

#define DRM_IOCTL_SUNXI_SYNC_GEM		DRM_IOW(DRM_COMMAND_BASE + \
		DRM_SUNXI_SYNC_GEM, struct sunxi_sync_gem_cmd)

#define DRM_IOCTL_SUNXI_INFO_FB_PLANE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SUNXI_INFO_FB_PLANE, struct sunxi_fb_info_cmd)

#define DRM_IOCTL_SUNXI_GET_CRTCINFO		DRM_IOWR(DRM_COMMAND_BASE + \
			DRM_SUNXI_GET_CRTCINFO, struct sunxi_drm_get_crtc_info)

#define DRM_IOCTL_SUNXI_GET_PLANEINFO		DRM_IOWR(DRM_COMMAND_BASE + \
				DRM_SUNXI_GET_PLANEINFO, struct sunxi_drm_get_plane_info)

#define DRM_IOCTL_SUNXI_GET_FBINFO		DRM_IOWR(DRM_COMMAND_BASE + \
				DRM_SUNXI_GET_FBINFO, struct sunxi_drm_get_fb_info)

#define DRM_IOCTL_SUNXI_GET_WBINFO		DRM_IOWR(DRM_COMMAND_BASE + \
				DRM_SUNXI_GET_WBINFO, struct sunxi_drm_get_wb_info)

#endif
