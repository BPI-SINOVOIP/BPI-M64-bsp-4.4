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

#ifndef _SUNXI_DRM_GEM_H_
#define _SUNXI_DRM_GEM_H_

#include <drm/drmP.h>
#include <drm/sunxi_drm.h>
#include <drm/drm_gem.h>

#define to_sunxi_gem(x)        container_of(x, struct sunxi_drm_gem_buf, obj)

struct sunxi_drm_gem_buf {
	struct drm_gem_object obj;
	void			*cookie;
	void __iomem *kvaddr;
	dma_addr_t dma_addr;
	struct dma_attrs dma_attrs;
	struct sg_table *sgt;
	struct page		**pages;
	uint64_t size;
	unsigned int flags;
};


int sunxi_drm_gem_dumb_map_offset(struct drm_file *file_priv,
	struct drm_device *dev, uint32_t handle,
	uint64_t *offset);

int sunxi_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma);

int sunxi_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf);

int sunxi_drm_gem_dumb_destroy(struct drm_file *file_priv,
	struct drm_device *dev, unsigned int handle);

int sunxi_drm_gem_dumb_create(struct drm_file *file_priv,
	struct drm_device *dev, struct drm_mode_create_dumb *args);

struct drm_gem_object *sunxi_drm_gem_creat(struct drm_device *dev,
	struct drm_mode_create_dumb *args);

void sunxi_drm_gem_destroy(struct drm_gem_object *gem_obj);

static inline int sunxi_check_gem_memory_type(
	struct drm_gem_object *sunxi_gem_obj,
	enum sunxi_drm_gem_buf_type need)
{
	struct sunxi_drm_gem_buf *buf = to_sunxi_gem(sunxi_gem_obj);
	return buf->flags & need;
}
void sunxi_sync_buf(struct sunxi_drm_gem_buf *buf);

void sunxi_drm_gem_free_object(struct drm_gem_object *obj);

int sunxi_drm_gem_init_object(struct drm_gem_object *obj);

int sunxi_drm_gem_sync_ioctl(struct drm_device *dev, void *data,
	struct drm_file *file_priv);

struct sg_table *sunxi_gem_prime_get_sg_table(
	struct drm_gem_object *obj);

int sunxi_gem_mmap_buf(struct drm_gem_object *obj,
	struct vm_area_struct *vma);

void *sunxi_gem_prime_vmap(struct drm_gem_object *obj);

void sunxi_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr);

#endif
