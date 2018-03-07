/*
 * drivers/char/sunxi-idc/sunxi-idc-debug.c
 *
 * Copyright (C) 2015-2017 Allwinnertech Co., Ltd
 *
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Sunxi IDC test module
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEBUG
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/seq_file.h>
#include <linux/freezer.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/sizes.h>
#include <linux/err.h>
#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/version.h>
#include "sunxi-interrupt-defer.h"

/*
 * sunxi_idc/
 * |-enable
 * |-result
 * |-threshold_value
 */

#define DRV_VERSION	"0.1"
#define BUF_LEN	(1UL << 20)
struct sunxi_idctest {
	bool				enable;
	bool				result;
	long long			threshold_value;
	struct mutex		mutex;
	struct task_struct	*task[4];
};

struct callback_done {
	bool			done;
	wait_queue_head_t	*wait;
};

static DEFINE_KFIFO(idc_ring, struct timespec, 128);
static DEFINE_SPINLOCK(idc_ring_lock);

static struct sunxi_idctest idc_gd;

static void dma_callback(void *arg)
{
	struct callback_done *done = arg;
	struct timespec now;
	static struct timespec prev = {0, 0};
	unsigned long flags;

	now = current_kernel_time();
	if (timespec_sub(now, prev).tv_nsec > 1000) {
		spin_lock_irqsave(&idc_ring_lock, flags);
		kfifo_put(&idc_ring, now);
		spin_unlock_irqrestore(&idc_ring_lock, flags);
		prev = now;
	}
	done->done = true;
	wake_up_all(done->wait);
}

static int memcpy_exec(struct dma_chan *chan, char *buf_src, char *buf_dst)
{
	DECLARE_WAIT_QUEUE_HEAD_ONSTACK(done_wait);
	int err = 0;
	dma_cookie_t	cookie;
	enum dma_status	status;
	unsigned short len_seed = 0;
	unsigned int len;
	struct dma_async_tx_descriptor *tx = NULL;
	struct callback_done	back_done = { .wait = &done_wait };
	dma_addr_t dma_src, dma_dst;

	/*
	 * Get the random len for the random drq times.
	 */
	while ((len_seed == 0) || ((len_seed << 4) > BUF_LEN))
		get_random_bytes(&len_seed, sizeof(len_seed));

	len = len_seed << 4;

	dma_src = dma_map_single(chan->device->dev, buf_src, len,
			DMA_TO_DEVICE);
	dma_dst = dma_map_single(chan->device->dev, buf_dst, len,
			DMA_FROM_DEVICE);

	tx = chan->device->device_prep_dma_memcpy(chan,
			dma_dst, dma_src, len, 0);

	if (!tx) {
		dma_unmap_single(chan->device->dev, dma_src, len,
				DMA_TO_DEVICE);
		dma_unmap_single(chan->device->dev, dma_dst, len,
				DMA_FROM_DEVICE);
		pr_warn("[sunxi_idctest]: device_prep_dma_memcpy failed!\n");
		goto error;
	}

	back_done.done = false;

	tx->callback = dma_callback;
	tx->callback_param = &back_done;
	cookie = tx->tx_submit(tx);

	dma_async_issue_pending(chan);

	wait_event_freezable_timeout(done_wait, back_done.done,
			msecs_to_jiffies(3000));

	status = dma_async_is_tx_complete(chan, cookie, NULL, NULL);

	dma_unmap_single(chan->device->dev, dma_src, len,
			DMA_TO_DEVICE);
	dma_unmap_single(chan->device->dev, dma_dst, len,
			DMA_FROM_DEVICE);

	if (!back_done.done) {
		pr_warn("[sunxi_idctest]: Test timed out!\n");
		err += 1;
	} else if (status == DMA_ERROR) {
		pr_warn("[sunxi_idctest]: status is DMA_ERROR!\n");
		err += 1;
	}

error:
	return err;
}

static int memcpy_thread(void *data)
{
	int		ret = 0;
	unsigned char len = 0;
	struct dma_chan	*chan;
	dma_cap_mask_t mask;
	enum dma_transaction_type type = DMA_MEMCPY;
	char *buf_src, *buf_dst;

	dma_cap_zero(mask);
	dma_cap_set(type, mask);

	chan = dma_request_channel(mask, NULL, NULL);
	if (!chan) {
		pr_warn("[sunxi_idctest]: No more dma channels available\n");
		ret = -ENODEV;
		goto err;
	}

	buf_src = kmalloc(BUF_LEN, GFP_KERNEL);
	if (!buf_src) {
		ret = -ENOMEM;
		goto err_src;
	}

	buf_dst = kmalloc(BUF_LEN, GFP_KERNEL);
	if (!buf_dst) {
		ret = -ENOMEM;
		goto err_dst;
	}

	while (!kthread_should_stop()) {
		ret = memcpy_exec(chan, buf_src, buf_dst);
		while (len == 0)
			get_random_bytes(&len, sizeof(len));
		msleep(len);
	}

	dmaengine_terminate_all(chan);
	kfree(buf_dst);

err_dst:
	kfree(buf_src);
err_src:
	dma_release_channel(chan);
err:
	return ret;
}

/**********************Sunxi Test BUS****************************/
static struct kset *test_kset;
static struct kobject *idctest_obj;

ssize_t enable_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	bool enabled;

	mutex_lock(&idc_gd.mutex);
	enabled = idc_gd.enable;
	mutex_unlock(&idc_gd.mutex);

	return scnprintf(buf, PAGE_SIZE,
		"%d\n", idc_gd.enable);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	ssize_t ret = count;
	unsigned long val;
	int i;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	mutex_lock(&idc_gd.mutex);
	idc_gd.enable = val == 0 ? false : true;
	if (idc_gd.enable) {
		for (i = 0;  i < 4; i++) {
			idc_gd.task[i] = kthread_run(memcpy_thread, NULL,
					"dma-%d", i);
			if (IS_ERR(idc_gd.task[i]))
				pr_err("create task:%d failed\n", i);
		}
	} else {
		for (i = 0;  i < 4; i++) {
			if (idc_gd.task[i]) {
				kthread_stop(idc_gd.task[i]);
				idc_gd.task[i] = NULL;
			}
		}
	}
	mutex_unlock(&idc_gd.mutex);

	return ret;
}

struct kobj_attribute enable_attr = {
	.attr = {
		.name = "enable",
		.mode = 0666,
	},
	.show = enable_show,
	.store = enable_store,
};

ssize_t threshold_value_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	long long	threshold;

	mutex_lock(&idc_gd.mutex);
	threshold = idc_gd.threshold_value;
	mutex_unlock(&idc_gd.mutex);

	return scnprintf(buf, PAGE_SIZE,
		"0x%llx\n", idc_gd.threshold_value);
}

static ssize_t threshold_value_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	ssize_t ret = count;
	long int threshold;

	if (kstrtol(buf, 0, &threshold))
		return -EINVAL;

	mutex_lock(&idc_gd.mutex);
	idc_gd.threshold_value = threshold;
	mutex_unlock(&idc_gd.mutex);

	return ret;
}

struct kobj_attribute threshold_value_attr = {
	.attr = {
		.name = "threshold_value",
		.mode = 0666,
	},
	.show = threshold_value_show,
	.store = threshold_value_store,
};

ssize_t result_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	char *cp = buf;
	bool result = true;
	struct timespec entry;
	long long prev_ns = 0, cur_ns;
	unsigned long flags;
	unsigned int ret = 1, i = 0;

	spin_lock_irqsave(&idc_ring_lock, flags);
	if (kfifo_get(&idc_ring, &entry))
		prev_ns = timespec_to_ns(&entry);

	while (i < 128) {
		ret = kfifo_get(&idc_ring, &entry);
		if (ret == 0)
			break;
		cur_ns = timespec_to_ns(&entry);
		cp += sprintf(cp, "%lld\n", cur_ns);
		if ((cur_ns - prev_ns < idc_gd.threshold_value) &&
					(idc_gd.enable == true)) {
			pr_err("err, prev_ns:0x%llx, cur_ns:0x%llx\n",
					prev_ns, cur_ns);
			result = false;
		}
		prev_ns = cur_ns;
		i++;
	}
	spin_unlock_irqrestore(&idc_ring_lock, flags);

	mutex_lock(&idc_gd.mutex);
	idc_gd.result = result;
	mutex_unlock(&idc_gd.mutex);
	cp += sprintf(cp, "%s", idc_gd.result ? "Success\n" : "Failed\n");
	return cp - buf;
}

struct kobj_attribute result_attr = {
		.attr = {
			.name = "result",
			.mode = 0666,
		},
		.show = result_show,
};

struct kobj_attribute *attr_group[] = {
	&enable_attr,
	&result_attr,
	&threshold_value_attr,
	NULL,
};

static int create_test_bus(void)
{
	int ret = 0;
	struct kobj_attribute **gropu_ptr = NULL;

	test_kset = kset_create_and_add("sunxi", NULL, NULL);
	if (!test_kset)
		return -ENOMEM;
	idctest_obj = kobject_create_and_add("idctest", &test_kset->kobj);
	if (!idctest_obj) {
		ret = -ENOMEM;
		goto err;
	}

	for (gropu_ptr = attr_group; (*gropu_ptr) != NULL; gropu_ptr++) {
		ret = sysfs_create_file(idctest_obj, &(*gropu_ptr)->attr);
		if (ret)
			goto err;
	}

	return 0;
err:
	kobject_put(idctest_obj);
	kset_unregister(test_kset);
	return ret;
}

static int __init sunxi_idc_test_init(void)
{
	int ret = 0;

	pr_debug("[sunxi_test]: Create sunxi test sysfs!\n");
	ret = create_test_bus();
	mutex_init(&idc_gd.mutex);

	return ret;
}

static void __exit sunxi_idc_test_exit(void)
{
	kobject_put(idctest_obj);
	kset_unregister(test_kset);
	pr_debug("[sunxi_test]: Unload sunxi idc sysfs!\n");
}

module_init(sunxi_idc_test_init);
module_exit(sunxi_idc_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("fanqinghua");
MODULE_VERSION(DRV_VERSION);
MODULE_DESCRIPTION("Sunxi IDC Test Module");
