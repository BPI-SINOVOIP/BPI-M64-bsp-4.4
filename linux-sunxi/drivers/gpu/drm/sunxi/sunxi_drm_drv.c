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
#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>
#include <drm/sunxi_drm.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_core.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_drm_encoder.h"
#include "sunxi_drm_connector.h"
#include "sunxi_drm_fb.h"
#include "sunxi_drm_gem.h"
#include "sunxi_drm_fbdev.h"
#include "sunxi_drm_dmabuf.h"
#include "subdev/sunxi_rotate.h"

#define DRIVER_NAME	"sunxi"
#define DRIVER_DESC	"allwinnertech SoC DRM"
#define DRIVER_DATE	"20160530"
#define DRIVER_MAJOR	1
#define DRIVER_MINOR	0

#define VBLANK_OFF_DELAY	50000

struct device *sunxi_drm_dev;

void sunxi_drm_set_dev(struct device *dev)
{
	sunxi_drm_dev = dev;
}

struct device *sunxi_drm_get_dev(void)
{
	return sunxi_drm_dev;
}

static int sunxi_drm_load(struct drm_device *dev, unsigned long flags)
{
	struct sunxi_drm_private *private;
	int ret;

	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	private = kzalloc(sizeof(struct sunxi_drm_private), GFP_KERNEL);
	if (!private) {
		DRM_ERROR("failed to allocate private\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev->dev, dev);
	dev->dev_private = (void *)private;

	drm_mode_config_init(dev);

	/* init kms poll for handling hpd */
#if 0
	drm_kms_helper_poll_init(dev);
#endif

	platform_set_drvdata(dev->platformdev, dev);

	sunxi_drm_mode_config_init(dev);

	if (sunxi_drm_init(dev)) {
		DRM_ERROR("failed to initialize sunxi drm dev.\n");
		goto err_init_sunxi;
	}

	ret = drm_vblank_init(dev, dev->mode_config.num_crtc);
	if (ret) {
		DRM_ERROR("failed to init vblank.\n");
		goto err_init_sunxi;
	}
	/*
	* create and configure fb helper and also sunxi specific
	* fbdev object.
	*/
	ret = sunxi_drm_fbdev_creat(dev);
	if (ret) {
		DRM_ERROR("failed to initialize drm fbdev\n");
		goto err_vblank;
	}

	return 0;

err_vblank:
	drm_vblank_cleanup(dev);
err_init_sunxi:
	sunxi_drm_destroy(dev);
	kfree(private);

	return ret;
}

static int sunxi_drm_unload(struct drm_device *dev)
{
	struct sunxi_drm_private *private;

	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);
	private = (struct sunxi_drm_private *)dev->dev_private;
	sunxi_drm_fbdev_destroy(dev);
	drm_vblank_cleanup(dev);
	drm_kms_helper_poll_fini(dev);
	drm_mode_config_cleanup(dev);

#if 0
	sunxi_drm_rotate_destroy(private->rotate_private);
#endif
	kfree(dev->dev_private);

	dev->dev_private = NULL;

	return 0;
}

static int sunxi_drm_open(struct drm_device *dev, struct drm_file *file)
{

	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);
	return 0;
}

static void sunxi_drm_preclose(struct drm_device *dev,
				struct drm_file *file)
{
	struct drm_pending_vblank_event *e, *t;
	unsigned long flags;
	struct drm_crtc *crtc = NULL;
	struct sunxi_drm_crtc *sunxi_crtc;

	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	spin_lock_irqsave(&dev->event_lock, flags);

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		sunxi_crtc = to_sunxi_crtc(crtc);
		list_for_each_entry_safe(e, t,
			&sunxi_crtc->pageflip_event_list,
			base.link) {
			if (e->base.file_priv == file) {
				list_del(&e->base.link);
				e->base.destroy(&e->base);
			}
		}
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);

}

static void sunxi_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	if (!file->driver_priv)
		return;

	kfree(file->driver_priv);
	file->driver_priv = NULL;
}

static void sunxi_drm_lastclose(struct drm_device *dev)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	sunxi_drm_fbdev_restore_mode(dev);
}

static const struct vm_operations_struct sunxi_drm_gem_vm_ops = {
	.fault = sunxi_drm_gem_fault,
	.open = drm_gem_vm_open,
	.close = drm_gem_vm_close,
};

static struct drm_ioctl_desc sunxi_ioctls[] = {
	DRM_IOCTL_DEF_DRV(SUNXI_FLIP_SYNC, sunxi_drm_flip_ioctl,
		DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(SUNXI_ROTATE, sunxi_drm_rotate_ioctl,
		DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(SUNXI_SYNC_GEM, sunxi_drm_gem_sync_ioctl,
		DRM_UNLOCKED | DRM_AUTH),
	DRM_IOCTL_DEF_DRV(SUNXI_INFO_FB_PLANE, sunxi_drm_info_fb_ioctl,
		DRM_UNLOCKED | DRM_AUTH),
};

static const struct file_operations sunxi_drm_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.mmap = sunxi_drm_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
	.unlocked_ioctl	= drm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.release = drm_release,
};

static struct drm_driver sunxi_drm_driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_MODESET |
		DRIVER_GEM | DRIVER_PRIME,
	.load = sunxi_drm_load,
	.unload = sunxi_drm_unload,
	.open = sunxi_drm_open,
	.preclose = sunxi_drm_preclose,
	.lastclose = sunxi_drm_lastclose,
	.postclose = sunxi_drm_postclose,
	.get_vblank_counter	= drm_vblank_no_hw_counter,
	.enable_vblank = sunxi_drm_enable_vblank,
	.disable_vblank = sunxi_drm_disable_vblank,
	.gem_free_object = sunxi_drm_gem_free_object,
	.gem_vm_ops = &sunxi_drm_gem_vm_ops,
	.dumb_create = sunxi_drm_gem_dumb_create,
	.dumb_map_offset = sunxi_drm_gem_dumb_map_offset,
	.dumb_destroy = sunxi_drm_gem_dumb_destroy,
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_get_sg_table	= sunxi_gem_prime_get_sg_table,
	.gem_prime_vmap 	= sunxi_gem_prime_vmap,
	.gem_prime_vunmap	= sunxi_gem_prime_vunmap,
	.gem_prime_mmap		= sunxi_gem_mmap_buf,
	.ioctls = sunxi_ioctls,
	.fops = &sunxi_drm_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
};

static int sunxi_drm_platform_probe(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	sunxi_drm_driver.num_ioctls = ARRAY_SIZE(sunxi_ioctls);

	sunxi_drm_set_dev(&pdev->dev);
	return drm_platform_init(&sunxi_drm_driver, pdev);
}

static int sunxi_drm_platform_remove(struct platform_device *pdev)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	drm_put_dev(dev_get_drvdata(&pdev->dev));

	return 0;
}

static const struct of_device_id sunxi_of_match[] = {
	{ .compatible = "allwinner,sun50i-disp", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_of_match);


static struct platform_driver sunxi_drm_platform_driver = {
	.probe = sunxi_drm_platform_probe,
	.remove = sunxi_drm_platform_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sunxi-drm",
		.of_match_table = sunxi_of_match,
	},
};

static int __init sunxi_drm_dev_init(void)
{
	int ret;

	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	ret = platform_driver_register(&sunxi_drm_platform_driver);
	if (ret < 0)
		goto out;

	return 0;

out:
	platform_driver_unregister(&sunxi_drm_platform_driver);
	return ret;
}

static void __exit sunxi_drm_dev_exit(void)
{
	DRM_DEBUG_DRIVER("[%d]\n", __LINE__);

	platform_driver_unregister(&sunxi_drm_platform_driver);
}

module_init(sunxi_drm_dev_init);
module_exit(sunxi_drm_dev_exit);

MODULE_AUTHOR("Jet Cui");
MODULE_DESCRIPTION("Allwinnertech SoC DRM Driver");
MODULE_LICENSE("GPL");
