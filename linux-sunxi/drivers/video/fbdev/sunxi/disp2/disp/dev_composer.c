/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef DEV_COMPOSER_C_C
#define DEV_COMPOSER_C_C

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include "dev_disp.h"
#include <video/sunxi_display2.h>

enum {
	HWC_NEW_CLIENT = 1,
	HWC_DESTROY_CLIENT,
	HWC_ACQUIRE_FENCE,
	HWC_SUBMIT_FENCE,
};

struct sync_info {
	int fd;
	unsigned int count;
};

struct display_sync {
	unsigned int timeline_count;
	unsigned int submmit_count;
	unsigned int current_count;
	struct sw_sync_timeline *timeline;
};

struct composer_private_data {
	struct disp_drv_info *dispopt;
	struct work_struct post2_cb_work;
	bool b_no_output;
	struct mutex sync_lock;
	struct display_sync display_sync[DISP_SCREEN_NUM];
};

static struct composer_private_data composer_priv;

static void imp_finish_cb(int force)
{
	int i = 0;
	struct display_sync *dsync = NULL;

	for (i = 0; i < DISP_SCREEN_NUM; i++) {
		dsync = &composer_priv.display_sync[i];
		while (dsync->timeline != NULL
		       && (dsync->timeline->value
					!= dsync->timeline_count)) {
			if (!(force & 1 << i)
				&& (dsync->timeline->value + 1) >= dsync->current_count)
				break;
			sw_sync_timeline_inc(dsync->timeline, 1);
		}
	}
}

static bool hwc_aquire_fence(int disp, void *user_fence)
{
	struct sync_fence *fence = NULL;
	struct sync_pt *pt = NULL;
	struct display_sync *dispsync = NULL;
	struct sync_info sync;
	sync.fd = -1;
	sync.count = 0;

	dispsync = &composer_priv.display_sync[disp];

	if (dispsync->timeline == NULL) {
		printk(KERN_ERR "must first new a client.\n");
		goto err_quire;
	}

	sync.count = dispsync->timeline_count;

	pt = sw_sync_pt_create(dispsync->timeline,
			       dispsync->timeline_count);
	if (pt == NULL) {
		printk(KERN_ERR "creat display pt fail\n");
		goto err_quire;
	}
	fence = sync_fence_create("display", pt);
	if (fence == NULL) {
		printk(KERN_ERR "creat dispay fence fail\n");
		goto err_quire;
	}
	sync.fd = get_unused_fd_flags(O_CLOEXEC);
	if (sync.fd < 0) {
		printk(KERN_ERR "get unused fd fail\n");
		goto err_quire;
	}

	sync_fence_install(fence, sync.fd);

	if (copy_to_user((void __user *)user_fence, &sync, sizeof(sync)))
		pr_warn("copy_to_user fail\n");

	dispsync->timeline_count++;
	return 0;
err_quire:
	return -ENOMEM;
}

static inline int hwc_submit(int disp, unsigned int sbcount)
{
	composer_priv.display_sync[disp].submmit_count = sbcount;
	return 0;
}

static int get_de_clk_rate(unsigned int disp, int *usr)
{
	struct disp_manager *disp_mgr;
	int rate = 254000000;
	if (DISP_SCREEN_NUM <= disp) {
		printk("%s: disp=%d\n", __func__, disp);
		return -1;
	}

	disp_mgr = composer_priv.dispopt->mgr[disp];
	if (disp_mgr && disp_mgr->get_clk_rate)
		rate = disp_mgr->get_clk_rate(disp_mgr);

	put_user(rate, usr);

	return 0;
}

static int hwc_new_client(int disp, int *user)
{
	char buf[20];
	if (composer_priv.display_sync[disp].timeline != NULL) {
		printk("has initial.\n");
		return 0;
	}

	sprintf(buf, "disp_%d", disp);
	composer_priv.display_sync[disp].timeline =
				    sw_sync_timeline_create(buf);
	if (composer_priv.display_sync[disp].timeline == NULL) {
		printk(KERN_ERR "creat timelie err.");
		return -1;
	}
	composer_priv.display_sync[disp].timeline_count = 0;
	composer_priv.display_sync[disp].submmit_count = 0;
	composer_priv.display_sync[disp].current_count = 0;
	get_de_clk_rate(disp, (int *)user);

	return 0;
}

static int hwc_destroy_client(int disp)
{
	/* releace for all */
	mutex_lock(&composer_priv.sync_lock);
	imp_finish_cb(1<<disp);
	sync_timeline_destroy(&composer_priv.display_sync[disp].timeline->obj);
	composer_priv.display_sync[disp].timeline = NULL;
	mutex_unlock(&composer_priv.sync_lock);
	return 0;
}

static int hwc_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -EFAULT;

	if (cmd == DISP_HWC_COMMIT) {
		unsigned long *ubuffer;
		ubuffer = (unsigned long *)arg;
		switch (ubuffer[1]) {
		case HWC_NEW_CLIENT:
			ret = hwc_new_client((int)ubuffer[0], (int *)ubuffer[2]);
			break;
		case HWC_DESTROY_CLIENT:
			ret = hwc_destroy_client((int)ubuffer[0]);
			break;
		case HWC_ACQUIRE_FENCE:
			ret = hwc_aquire_fence(ubuffer[0], (void *)ubuffer[2]);
			break;
		case HWC_SUBMIT_FENCE:
			ret = hwc_submit(ubuffer[0], ubuffer[2]);
			break;
		default:
			pr_warn("hwc give a err iotcl.\n");
		}
	}
	return ret;
}

static void disp_composer_proc(u32 sel)
{
	if (sel < 2) {
		composer_priv.display_sync[sel].current_count =
			composer_priv.display_sync[sel].submmit_count;
	}
	schedule_work(&composer_priv.post2_cb_work);
}

static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.sync_lock);
	imp_finish_cb(composer_priv.b_no_output ? 3 : 0);
	mutex_unlock(&composer_priv.sync_lock);
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	schedule_work(&composer_priv.post2_cb_work);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	return 0;
}

s32 composer_init(struct disp_drv_info *psg_disp_drv)
{
	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.sync_lock);
	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_ioctl);
#if defined(CONFIG_COMPAT)
	disp_register_compat_ioctl_func(DISP_HWC_COMMIT, hwc_ioctl);
#endif
	disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
	composer_priv.dispopt = psg_disp_drv;
	return 0;
}
#endif
