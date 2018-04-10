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

#include <linux/shmem_fs.h>
#include "sunxi_drm_gem.h"
#include <drm/sunxi_drm.h>

struct page_info {
	struct page *page;
	unsigned int order;
	struct list_head list;
};

void sunxi_sync_buf(struct sunxi_drm_gem_buf *buf)
{
	dma_sync_sg_for_device(NULL, buf->sgt->sgl,
		buf->sgt->nents, DMA_BIDIRECTIONAL);

}

static int contig_buf_alloc(struct drm_device *dev,
	struct sunxi_drm_gem_buf *buf)
{

	unsigned int nr_pages;
	enum dma_attr attr;

	nr_pages = buf->size >> PAGE_SHIFT;
	init_dma_attrs(&buf->dma_attrs);

	dma_set_attr(DMA_ATTR_FORCE_CONTIGUOUS, &buf->dma_attrs);

	if (buf->flags & SUNXI_BO_WC || !(buf->flags & SUNXI_BO_CACHABLE))
		attr = DMA_ATTR_WRITE_COMBINE;
	else
		attr = DMA_ATTR_NON_CONSISTENT;
	dma_set_attr(attr, &buf->dma_attrs);
	dma_set_attr(DMA_ATTR_NO_KERNEL_MAPPING, &buf->dma_attrs);

	buf->kvaddr = dma_alloc_attrs(dev->dev, buf->size,
					&buf->dma_addr, GFP_KERNEL,
					&buf->dma_attrs);
	if (!buf->kvaddr) {
		DRM_ERROR("failed to allocate buffer.\n");
		goto err_free;
	}

	DRM_DEBUG_KMS("dma_addr(0x%lx), size(%llu)\n",
		(unsigned long)buf->dma_addr, buf->size);

	return 0;

err_free:
	dma_free_attrs(dev->dev, buf->size, buf->kvaddr,
			buf->dma_addr, &buf->dma_attrs);

	return -ENOMEM;

}

static inline int sunxi_order_gfp(unsigned long size, gfp_t *gfp_flags)
{
	int gem_order;

	gem_order = get_order(size);
	if (gem_order >= MAX_ORDER)
	gem_order = MAX_ORDER - 1;
	if (gem_order > 4) {
		*gfp_flags = GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN |
			__GFP_NORETRY;
	} else {
		*gfp_flags = GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN;
	}

	/*
	* no need to allocate another >= PAGE_SIZE buffer,
	* or it will cause ddk cannot
	* right PDE for this page;
	*/
	if ((1 << gem_order) * PAGE_SIZE > size)
		return gem_order - 1;
	else
		return gem_order;
}

static int nocontig_buf_alloc(struct drm_device *dev,
	struct sunxi_drm_gem_buf *buf)
{
	long tmp_size, count;
	int cur_order;
	struct list_head head;
	struct page_info *curs, *tmp_curs = NULL;
	gfp_t gfp_flags;
	struct page *page;
	struct scatterlist *sg;

	INIT_LIST_HEAD(&head);
	tmp_size = buf->size;
	if (buf->size / PAGE_SIZE > totalram_pages / 2)
		return -ENOMEM;

	cur_order = sunxi_order_gfp(tmp_size, &gfp_flags);

	if (unlikely(cur_order < 0)) {
		DRM_ERROR("bad order.\n");
		return -ENOMEM;
	}

	count = 0;
	while (tmp_size > 0 && cur_order >= 0) {

		page = alloc_pages(gfp_flags, cur_order);
		if (page == NULL) {
			cur_order--;
			if (cur_order < 0) {
				DRM_ERROR("bad alloc for 0 order.\n");
				goto err_alloc;
			}
			continue;
		}
		curs = kzalloc(sizeof(struct page_info), GFP_KERNEL);
		if (curs == NULL) {
			DRM_ERROR("bad alloc for page info.\n");
			goto err_alloc;
		}
		curs->order = cur_order;
		curs->page = page;
		list_add_tail(&curs->list, &head);
		tmp_size -= (1 << cur_order) * PAGE_SIZE;
		cur_order = sunxi_order_gfp(tmp_size, &gfp_flags);
		count++;

	}

	if (tmp_size > 0) {
		DRM_ERROR("bad alloc err for totle size.\n");
		goto err_alloc;
	}

	buf->sgt = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!buf->sgt) {
		DRM_ERROR("bad alloc for sgt.\n");
		goto err_alloc;
	}

	if (sg_alloc_table(buf->sgt, count, GFP_KERNEL)) {
		DRM_ERROR("bad alloc for sg.\n");
		goto err_alloc;
	}

	sg = buf->sgt->sgl;
	list_for_each_entry_safe(curs, tmp_curs, &head, list) {
		sg_set_page(sg, curs->page,
			(1 << curs->order) * PAGE_SIZE, 0);
		sg->dma_address = sg_phys(sg);
		list_del(&curs->list);
		kfree(curs);
		sg = sg_next(sg);
	}

	return 0;
err_alloc:
	list_for_each_entry_safe(curs, tmp_curs, &head, list) {
		__free_pages(curs->page, curs->order);
		list_del(&curs->list);
	}

	return -ENOMEM;
}

static int lowlevel_buffer_allocate(struct drm_device *dev,
	struct sunxi_drm_gem_buf *buf)
{
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	buf->size = PAGE_ALIGN(buf->size);
	/*
	* Make sure all the display buffer are all config,
	* it can fix destory buffer rightly when exit weston;
	*/
	buf->flags |= 0x01;
	if (buf->flags & SUNXI_BO_CONTIG) {
		return contig_buf_alloc(dev, buf);
	} else {
		return nocontig_buf_alloc(dev, buf);
	}
}

static void lowlevel_buffer_destroy(struct drm_device *dev,
		struct sunxi_drm_gem_buf *buf)
{

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	dma_free_attrs(dev->dev, buf->size, buf->kvaddr,
		(dma_addr_t)buf->dma_addr, &buf->dma_attrs);
	/* drm_free_large(buf->pages); */
}

static struct sunxi_drm_gem_buf *sunxi_buf_create(
	struct drm_mode_create_dumb *args)
{
	struct sunxi_drm_gem_buf *sunxi_buf;
	/*
	* The pitch decided by driver, but you must consult the gpu ddk.
	* "pitch should be 8 aligned" mali GPU said.
	*/
	uint64_t size;
	sunxi_buf =  kzalloc(sizeof(struct sunxi_drm_gem_buf), GFP_KERNEL);
	if (NULL == sunxi_buf) {
		DRM_ERROR("Bad alloc for sunxi_buf.\n");
		return NULL;
	}

	args->pitch = ((args->width * ((args->bpp + 7) / 8) + 7) / 8) * 8;
	size = args->pitch * args->height;
	/* GEM use the size for PAGE_ALIGN */
	args->size = PAGE_ALIGN(size);
	sunxi_buf->flags = args->flags;
	sunxi_buf->size = args->size;

	return sunxi_buf;
}

void sunxi_buf_destroy(struct sunxi_drm_gem_buf *sunxi_buf)
{
	kfree(sunxi_buf);
}

void sunxi_drm_gem_destroy(struct drm_gem_object *gem_obj)
{
	struct sunxi_drm_gem_buf *sunxi_buf;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_buf = to_sunxi_gem(gem_obj);

	DRM_DEBUG_KMS("handle count = %d\n", gem_obj->handle_count);

	if (gem_obj->import_attach) {
		drm_prime_gem_destroy(gem_obj, sunxi_buf->sgt);
	} else {
		lowlevel_buffer_destroy(gem_obj->dev, sunxi_buf);
	}

	sunxi_buf_destroy(sunxi_buf);

	drm_gem_object_release(gem_obj);
	kfree(gem_obj);
}

struct drm_gem_object *sunxi_drm_gem_creat(struct drm_device *dev,
	struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *sunxi_gem_obj;
	struct sunxi_drm_gem_buf *sunxi_buf;
	int ret;
	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	sunxi_buf = sunxi_buf_create(args);
	if (sunxi_buf == NULL) {
		DRM_ERROR("failed to delete sunxi_drm_gem_buf.\n");
		return NULL;
	}

	if (lowlevel_buffer_allocate(dev, sunxi_buf)) {
		sunxi_buf_destroy(sunxi_buf);
		return NULL;
	}

	drm_gem_private_object_init(dev, &sunxi_buf->obj, sunxi_buf->size);
/*
	ret = drm_gem_object_init(dev, &sunxi_buf->obj, sunxi_buf->size);
	if (ret < 0) {
		DRM_ERROR("failed to initialize gem object.\n");
		lowlevel_buffer_destroy(dev, sunxi_buf);
		sunxi_buf_destroy(sunxi_buf);
		return NULL;
	}
*/



	ret = drm_gem_create_mmap_offset(&sunxi_buf->obj);
	if (ret < 0) {
		DRM_ERROR("failed to create mmap offset.\n");
		lowlevel_buffer_destroy(dev, sunxi_buf);
		sunxi_buf_destroy(sunxi_buf);
		return NULL;
	}

	sunxi_gem_obj = &sunxi_buf->obj;

	DRM_DEBUG_KMS("obj:%p  addr:%llx\n", sunxi_gem_obj,
		sunxi_buf->dma_addr);

	return sunxi_gem_obj;
}

int sunxi_drm_gem_dumb_create(struct drm_file *file_priv,
	struct drm_device *dev,
	struct drm_mode_create_dumb *args)
{
	struct drm_gem_object *gem_obj;
	int ret;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	gem_obj = sunxi_drm_gem_creat(dev, args);
	if (NULL == gem_obj)
		return -ENOMEM;

	ret = drm_gem_handle_create(file_priv, gem_obj,
					&args->handle);
	/*
	* refcount = 2, so unreference;
	* refcount = 1, handle_count = 1
	*/
	drm_gem_object_unreference_unlocked(gem_obj);
	if (ret) {
		sunxi_drm_gem_destroy(gem_obj);
		return ret;
	}
	DRM_DEBUG_KMS("obj:%p handle:%u [%d  %d  %d] bpp:%d.\n",
		gem_obj, args->handle, args->width, args->height,
		args->pitch, args->bpp);
	return 0;
}

int sunxi_drm_gem_dumb_destroy(struct drm_file *file_priv,
	struct drm_device *dev,
	unsigned int handle)
{
	int ret;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);
	ret = drm_gem_handle_delete(file_priv, handle);
	if (ret < 0) {
		DRM_ERROR("failed to delete drm_gem_handle.\n");
		return ret;
	}

	return 0;
}

static void sunxi_update_vm_attr(struct sunxi_drm_gem_buf *sunxi_buf,
	struct vm_area_struct *vma)
{
	DRM_DEBUG_KMS("flags = 0x%x\n", sunxi_buf->flags);

	if (sunxi_buf->flags & SUNXI_BO_CACHABLE) {
		vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
	} else if (sunxi_buf->flags & SUNXI_BO_WC) {
		vma->vm_page_prot =
		pgprot_writecombine(vm_get_page_prot(vma->vm_flags));
	} else {
		vma->vm_page_prot =
		pgprot_noncached(vm_get_page_prot(vma->vm_flags));
	}
}

static int sunxi_drm_gem_mmap_buffer(struct sunxi_drm_gem_buf *sunxi_buf,
	struct vm_area_struct *vma)
{
	struct drm_device *drm_dev = sunxi_buf->obj.dev;
	unsigned long vm_size;
	int ret;

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

	vm_size = vma->vm_end - vma->vm_start;

	/* check if user-requested size is valid. */
	if (vm_size > sunxi_buf->size)
		return -EINVAL;

	ret = dma_mmap_attrs(drm_dev->dev, vma, sunxi_buf->kvaddr,
		sunxi_buf->dma_addr, sunxi_buf->size,
		&sunxi_buf->dma_attrs);
	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	return 0;
}


/*static int sunxi_drm_gem_map_buf(struct drm_gem_object *obj,
	struct vm_area_struct *vma,
	unsigned long f_vaddr,
	pgoff_t page_offset)
{
	struct sunxi_drm_gem_buf *buf = to_sunxi_gem(obj);
	struct scatterlist *sgl;
	unsigned long pfn;
	int i = 0;

	if (!buf->sgt)
		return -EINTR;

	if (page_offset >= (buf->size >> PAGE_SHIFT)) {
		DRM_ERROR("invalid page offset\n");
		return -EINVAL;
	}

	sgl = buf->sgt->sgl;
	for_each_sg(buf->sgt->sgl, sgl, buf->sgt->nents, i) {
		if (page_offset < (sgl->length >> PAGE_SHIFT))
			break;
		page_offset -=	(sgl->length >> PAGE_SHIFT);
	}

	pfn = __phys_to_pfn(sg_phys(sgl)) + page_offset;

	DRM_DEBUG_KMS("map:%llx  vm:[%lx,%lx] fault:%lx \
		pfn:%ld  flag:%lx, page:\n",
		buf->dma_addr, vma->vm_start, vma->vm_end, f_vaddr, pfn,
		vma->vm_flags);

	return vm_insert_pfn(vma, f_vaddr, pfn);
}

static unsigned int convert_to_vm_err_msg(int msg)
{
	unsigned int out_msg;

	switch (msg) {
	case 0:
	case -ERESTARTSYS:
	case -EINTR:
		out_msg = VM_FAULT_NOPAGE;
		break;

	case -ENOMEM:
		out_msg = VM_FAULT_OOM;
		break;

	default:
		out_msg = VM_FAULT_SIGBUS;
		break;
	}

	return out_msg;
}*/

int sunxi_drm_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	unsigned long pfn;
	pgoff_t page_offset;
	int ret;
	struct sunxi_drm_gem_buf *buf = to_sunxi_gem(obj);

	page_offset = ((unsigned long)vmf->virtual_address -
		vma->vm_start) >> PAGE_SHIFT;
	if (page_offset >= (buf->size >> PAGE_SHIFT)) {
		DRM_ERROR("invalid page offset\n");
		ret = -EINVAL;
		goto out;
	}
	pfn = page_to_pfn(buf->pages[page_offset]);
	ret = vm_insert_mixed(vma, (unsigned long)vmf->virtual_address, pfn);

out:
	switch (ret) {
	case 0:
	case -ERESTARTSYS:
	case -EINTR:
		return VM_FAULT_NOPAGE;
	case -ENOMEM:
		return VM_FAULT_OOM;
	default:
		return VM_FAULT_SIGBUS;
	}
}

static int check_gem_flags(unsigned int flags)
{
	if (flags & ~(SUNXI_BO_MASK)) {
		DRM_ERROR("invalid flags.\n");
		return -EINVAL;
	}

	return 0;
}

int sunxi_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_gem_object *obj;
	struct sunxi_drm_gem_buf *sunxi_buf;
	int ret;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	/* set vm_area_struct. */
	ret = drm_gem_mmap(filp, vma);
	if (ret < 0) {
		DRM_ERROR("failed to mmap., ret:%d\n", ret);
		return ret;
	}

	obj = vma->vm_private_data;
	sunxi_buf = to_sunxi_gem(obj);
	ret = check_gem_flags(sunxi_buf->flags);
	if (ret) {
		drm_gem_vm_close(vma);
		drm_gem_free_mmap_offset(obj);
		return ret;
	}

#if 0
	vma->vm_flags |= VM_PFNMAP | VM_DONTEXPAND |
			VM_DONTDUMP;
#endif
	sunxi_update_vm_attr(sunxi_buf, vma);
	sunxi_drm_gem_mmap_buffer(sunxi_buf, vma);

	return ret;
}

int sunxi_drm_gem_dumb_map_offset(struct drm_file *file_priv,
	struct drm_device *dev, uint32_t handle,
	uint64_t *offset)
{
	struct drm_gem_object *obj;
	int ret = 0;

	DRM_DEBUG_KMS("[%d]\n", __LINE__);

	/* protect drm_gem_create_mmap_offset */
	mutex_lock(&dev->struct_mutex);

	/*
	* get offset of memory allocated for drm framebuffer.
	* - this callback would be called by user application
	*	with DRM_IOCTL_MODE_MAP_DUMB command.
	*/

	obj = drm_gem_object_lookup(dev, file_priv, handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		ret = -EINVAL;
		goto unlock;
	}
/*
	ret = drm_gem_create_mmap_offset(obj);
	if (ret) {
		DRM_ERROR("failed to create mmap offset.\n");
		ret = -EINVAL;
		goto unlock;
	}
*/

	*offset = drm_vma_node_offset_addr(&obj->vma_node);
	DRM_DEBUG_KMS("offset = 0x%lx\n", (unsigned long)*offset);

	drm_gem_object_unreference(obj);
unlock:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

int sunxi_drm_gem_init_object(struct drm_gem_object *obj)
{
	/*TODO maybe*/
	return 0;
}

void sunxi_drm_gem_free_object(struct drm_gem_object *obj)
{
	struct sunxi_drm_gem_buf *buf = to_sunxi_gem(obj);

	drm_gem_free_mmap_offset(obj);
	lowlevel_buffer_destroy(obj->dev, buf);
	/*sunxi_drm_gem_destroy(obj);*/
	kfree(buf);
}

int sunxi_drm_gem_sync_ioctl(struct drm_device *dev, void *data,
	struct drm_file *file_priv)
{
	int ret = 0;
	struct sunxi_sync_gem_cmd *gem_cmd;
	struct drm_gem_object *obj;
	struct sunxi_drm_gem_buf *buf;

	gem_cmd = (struct sunxi_sync_gem_cmd *)data;
	obj = drm_gem_object_lookup(dev, file_priv, gem_cmd->gem_handle);
	if (!obj) {
		DRM_ERROR("failed to lookup gem object.\n");
		return -EINVAL;
	}
	buf = to_sunxi_gem(obj);
	if (!buf) {
		DRM_ERROR("failed to lookup sunxi_drm_gem_buf.\n");
		ret = -EINVAL;
		goto out;
	}

	/*sunxi_sync_buf(buf);*/
out:
	drm_gem_object_unreference(obj);

	return ret;
}

/*
 * Allocate a sg_table for this GEM object.
 * Note: Both the table's contents, and the sg_table itself must be freed by
 *       the caller.
 * Returns a pointer to the newly allocated sg_table, or an ERR_PTR() error.
 */
struct sg_table *sunxi_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct sunxi_drm_gem_buf *sunxi_buf = to_sunxi_gem(obj);
	struct drm_device *drm = obj->dev;
	struct sg_table *sgt;
	int ret;

	sgt = kzalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt)
		return ERR_PTR(-ENOMEM);

	ret = dma_get_sgtable_attrs(drm->dev, sgt, sunxi_buf->kvaddr,
		sunxi_buf->dma_addr, sunxi_buf->size,
		&sunxi_buf->dma_attrs);
	if (ret) {
		DRM_ERROR("failed to allocate sgt, %d\n", ret);
		kfree(sgt);
		return ERR_PTR(ret);
	}

	return sgt;
}

int sunxi_gem_mmap_buf(struct drm_gem_object *obj,
	struct vm_area_struct *vma)
{
	int ret;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret)
		return ret;

	return sunxi_drm_gem_mmap_buffer((struct sunxi_drm_gem_buf *)obj, vma);
}

void *sunxi_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct sunxi_drm_gem_buf *sunxi_buf = to_sunxi_gem(obj);

	if (dma_get_attr(DMA_ATTR_NO_KERNEL_MAPPING, &sunxi_buf->dma_attrs))
		return NULL;

	return sunxi_buf->kvaddr;
}

void sunxi_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	/* Nothing to do */
}
