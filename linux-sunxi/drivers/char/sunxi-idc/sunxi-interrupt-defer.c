/*******************************************************************************
 * Copyright (C) 2016-2018, Allwinner Technology CO., LTD.
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 ******************************************************************************/
/* #define DEBUG */
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "sunxi-interrupt-defer.h"

struct sunxi_idc *g_idc_dev;

void idc_print_regs(unsigned int *_data, int _len, unsigned int *_addr)
{
	int i;

	pr_debug("---------------- The valid len = %d ----------------\n",
		_len);
	for (i = 0; i < _len/32; i++) {
		pr_debug("0x%p: %08X %08X %08X %08X %08X %08X %08X %08X\n",
			i*8 + _addr,
			_data[i*8+0], _data[i*8+1], _data[i*8+2], _data[i*8+3],
			_data[i*8+4], _data[i*8+5], _data[i*8+6], _data[i*8+7]);
	}
	pr_debug("----------------------------------------------------\n");
}

static inline unsigned int __sunxi_idc_read(unsigned int offset)
{
	return readl(g_idc_dev->base + offset);
}

static inline void __sunxi_idc_write(unsigned int offset,
						unsigned int value)
{
	writel(value, g_idc_dev->base + offset);
}

static inline void __idc_enable(void)
{
	unsigned int reg;

	reg = __sunxi_idc_read(IDC_EN_REG);
	reg |= IDC_EN;
	__sunxi_idc_write(IDC_EN_REG, reg);
}

static inline void __idc_disable(void)
{
	unsigned int reg;

	reg = __sunxi_idc_read(IDC_EN_REG);
	reg &= ~IDC_EN;
	__sunxi_idc_write(IDC_EN_REG, reg);
}

static inline int __idc_du_enable(unsigned int du, bool en)
{
	unsigned int reg;

	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	if (en)
		reg |= DU_EN;
	else
		reg &= ~DU_EN;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	return 0;
}

static inline bool __idc_du_get_enabled(unsigned int du)
{
	return __sunxi_idc_read(DU_CTRL_REG(du)) & DU_EN ? true : false;
}

static inline int __idc_du_enable_wfi_check(unsigned int du)
{
	unsigned int reg;

	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	reg |= DU_WFI_CHECK_EN;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	return 0;
}

static inline int __idc_du_disable_wfi_check(unsigned int du)
{
	unsigned int reg;

	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	reg &= ~DU_WFI_CHECK_EN;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	return 0;
}

static inline int __idc_du_clk_32k(unsigned int du)
{
	unsigned int reg;

	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	reg &= ~DU_CLK_SRC_SEL_MASK;
	reg |= DU_CLK_32K;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	return 0;
}

static inline int __idc_du_clk_24m(unsigned int du)
{
	unsigned int reg;

	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	reg &= ~DU_CLK_SRC_SEL_MASK;
	reg |= DU_CLK_24M;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	return 0;
}

static inline void __idc_du_set_delay_time(unsigned int du,
							unsigned int cycle)
{
	unsigned int reg;
	int retval;

	__sunxi_idc_write(DU_INTV_VALUE(du), cycle);
	reg = __sunxi_idc_read(DU_CTRL_REG(du));
	reg |= DU_RELOAD_MODE;
	__sunxi_idc_write(DU_CTRL_REG(du), reg);
	retval = sunxi_wait_when((__sunxi_idc_read(DU_CTRL_REG(du))
			& DU_RELOAD_MODE), 2);
	if (retval)
		pr_err("change delay cycle timed out\n");
}

static inline unsigned int __idc_du_get_delay_time(unsigned int du)
{
	return __sunxi_idc_read(DU_CUR_VALUE(du));
}

static inline int __idc_irq_enable(unsigned int irq)
{
	unsigned int reg;

	reg = __sunxi_idc_read(IRQ_DLY_EN(irq));
	reg |= IRQ_DLY_EN_MASK(irq);
	__sunxi_idc_write(IRQ_DLY_EN(irq), reg);
	return 0;
}

static inline int __idc_irq_disable(unsigned int irq)
{
	unsigned int reg;

	reg = __sunxi_idc_read(IRQ_DLY_EN(irq));
	reg &= ~IRQ_DLY_EN_MASK(irq);
	__sunxi_idc_write(IRQ_DLY_EN(irq), reg);
	return 0;
}

static inline int __idc_irq_select_du(unsigned int irq, unsigned int du)
{
	unsigned int reg;

	reg = __sunxi_idc_read(IRQ_SEL_REG(irq));
	reg &= ~IRQ_SEL_REG_MASK(irq);
	reg |= IRQ_SEL_REG_DU(irq, du);
	__sunxi_idc_write(IRQ_SEL_REG(irq), reg);
	return 0;
}

static inline int __idc_dbg_enable(unsigned int dbg, bool en)
{
	unsigned int reg;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	if (en)
		reg |= INT_DU_EN;
	else
		reg &= ~INT_DU_EN;
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	return 0;
}

static inline bool __idc_dbg_get_enabled(unsigned int dbg)
{
	return __sunxi_idc_read(INT_DU_CTRL(dbg)) & INT_DU_EN ? true : false;
}

static inline int __idc_dbg_select_irq(unsigned int dbg, unsigned int irq)
{
	unsigned int reg;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	reg &= ~INT_SEL_MASK;
	reg |= INT_SEL(irq);
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	return 0;
}

static inline int __idc_dbg_clk_32k(unsigned int dbg)
{
	unsigned int reg;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	reg &= ~INT_DU_CLK_SEL_MASK;
	reg |= INT_DU_CLK_SEL_32K;
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	return 0;
}

static inline int __idc_dbg_clk_24m(unsigned int dbg)
{
	unsigned int reg;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	reg &= ~INT_DU_CLK_SEL_MASK;
	reg |= INT_DU_CLK_SEL_24M;
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	return 0;
}

static inline unsigned int __idc_dbg_restart(unsigned int dbg)
{
	unsigned int reg;
	int retval;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	reg |= INT_DU_RESTART;
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	retval = sunxi_wait_when((__sunxi_idc_read(INT_DU_CTRL(dbg))
			& INT_DU_RESTART), 2);
	if (retval)
		pr_err("restart dbg%d timed out\n", dbg);
	return 0;
}

static inline int __idc_dbg_ovf_clear(unsigned int dbg)
{
	unsigned int reg;

	reg = __sunxi_idc_read(INT_DU_CTRL(dbg));
	reg |= OVF_STA;
	__sunxi_idc_write(INT_DU_CTRL(dbg), reg);
	return 0;
}

static inline bool __idc_dbg_ovf(unsigned int dbg)
{
	return __sunxi_idc_read(INT_DU_CTRL(dbg)) & OVF_STA ? true : false;
}

static inline unsigned int __idc_dbg_get_current_value(unsigned int dbg)
{
	return __sunxi_idc_read(INT_DU_CUR_VALUE(dbg));
}

unsigned int __idc_save_regs(struct sunxi_idc *idc_dev)
{
	unsigned int i;

	for (i = 0; i < IRQ_DELAY_EN_REGS; i++)
		idc_dev->idc_regs.irq_delay_en[i] =
		__sunxi_idc_read(IRQ_DLY_EN_BASE + (i * sizeof(int)));
	for (i = 0; i < IRQ_SELECT_REGS; i++)
		idc_dev->idc_regs.irq_select_reg[i] =
		__sunxi_idc_read(IRQ_SEL_REG_BASE + (i * sizeof(int)));
	for (i = 0; i < CHANNEL_NUMBERS; i++)
		idc_dev->idc_regs.delay_interval_reg[i] =
		__sunxi_idc_read(DU_INTV_VALUE_BASE + (i * sizeof(int)));
	for (i = 0; i < CHANNEL_NUMBERS; i++)
		idc_dev->idc_regs.delay_ctrl_reg[i] =
			__sunxi_idc_read(DU_CTRL_REG(i));
	for (i = 0; i < INT_DU_CTRL_NUMBERS; i++)
		idc_dev->idc_regs.irq_debug_ctrl_reg[i] =
			__sunxi_idc_read(INT_DU_CTRL(i));
	return 0;
}

unsigned int __idc_restore_regs(struct sunxi_idc *idc_dev)
{
	unsigned int i, reg;
	int retval;

	for (i = 0; i < IRQ_DELAY_EN_REGS; i++)
		__sunxi_idc_write(IRQ_DLY_EN_BASE + (i * sizeof(int)),
			idc_dev->idc_regs.irq_delay_en[i]);
	for (i = 0; i < IRQ_SELECT_REGS; i++)
		__sunxi_idc_write(IRQ_SEL_REG_BASE + (i * sizeof(int)),
			idc_dev->idc_regs.irq_select_reg[i]);
	for (i = 0; i < CHANNEL_NUMBERS; i++)
		__sunxi_idc_write(DU_INTV_VALUE_BASE + (i * sizeof(int)),
			idc_dev->idc_regs.delay_interval_reg[i]);
	for (i = 0; i < CHANNEL_NUMBERS; i++) {
		if (idc_dev->idc_regs.delay_ctrl_reg[i] & DU_EN) {
			__sunxi_idc_write(DU_CTRL_REG(i),
			idc_dev->idc_regs.delay_ctrl_reg[i] | DU_RELOAD_MODE);
			retval = sunxi_wait_when(
				(__sunxi_idc_read(DU_CTRL_REG(i))
				& DU_RELOAD_MODE), 2);
			if (retval)
				pr_err("resume delay cycle timed out\n");
		} else {
			__sunxi_idc_write(DU_CTRL_REG(i),
				idc_dev->idc_regs.delay_ctrl_reg[i]);
		}
	}
	for (i = 0; i < INT_DU_CTRL_NUMBERS; i++) {
		if (idc_dev->idc_regs.irq_debug_ctrl_reg[i] & INT_DU_EN) {
			__sunxi_idc_write(INT_DU_CTRL(i),
			idc_dev->idc_regs.irq_debug_ctrl_reg[i] |
				INT_DU_RESTART);
			retval = sunxi_wait_when(
				(__sunxi_idc_read(INT_DU_CTRL(i))
				& INT_DU_RESTART), 2);
			if (retval)
				pr_err("restart dbg%d timed out\n", i);
		} else {
			__sunxi_idc_write(INT_DU_CTRL(i),
				idc_dev->idc_regs.irq_debug_ctrl_reg[i]);
		}
	}
	reg = __sunxi_idc_read(DU_STA_REG);
	reg |= 0xffff;
	__sunxi_idc_write(DU_STA_REG, reg);

	return 0;
}

static ssize_t sunxi_du_enable_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	bool enabled;
	unsigned int du = index->index;

	WARN_ON(du >= CHANNEL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	enabled = __idc_du_get_enabled(du);
	spin_unlock(&g_idc_dev->lock);
	return scnprintf(buf, PAGE_SIZE,
		"%d\n", enabled);
}

static ssize_t sunxi_du_enable_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int du = index->index;
	bool en;

	WARN_ON(du >= CHANNEL_NUMBERS);
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	en = val == 0 ? false : true;
	spin_lock(&g_idc_dev->lock);
	__idc_du_enable(du, en);
	spin_unlock(&g_idc_dev->lock);
	return count;
}

static ssize_t sunxi_du_irq_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int reg[IRQ_SELECT_REGS];  /* 28 */
	unsigned int bitmap[IRQ_DELAY_EN_REGS]; /* 7 */
	unsigned int du = index->index;
	unsigned int i;

	WARN_ON(du >= CHANNEL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	for (i = 0; i < IRQ_SELECT_REGS; i++)
		reg[i] = __sunxi_idc_read(IRQ_SEL_REG_BASE + (i * sizeof(int)));
	spin_unlock(&g_idc_dev->lock);

	for (i = 0; i < IRQ_NUMBERS; i++) {
		if (((reg[i/8] >> ((i%8) * 4)) & 0xf) == du)
			bitmap[i/32] |= 1 << (i % 32);
		else
			bitmap[i/32] &= ~(1 << (i % 32));
	}

	return scnprintf(buf, PAGE_SIZE,
		"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
			bitmap[0], bitmap[1], bitmap[2], bitmap[3],
				bitmap[4], bitmap[5], bitmap[6]);
}

static ssize_t sunxi_du_irq_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int du = index->index;
	bool en;
	bool du_enabled;

	WARN_ON(du >= CHANNEL_NUMBERS);
	if (*buf == '!') {
		en = false;
		if (kstrtoul(buf+1, 0, &val))
			return -EINVAL;
	} else {
		en = true;
		if (kstrtoul(buf, 0, &val))
			return -EINVAL;
	}

	if (val >= IRQ_NUMBERS)
		return -EINVAL;

	spin_lock(&g_idc_dev->lock);
	du_enabled = __idc_du_get_enabled(du);
	__idc_du_enable(du, false);
	if (en) {
		__idc_irq_select_du(val, du);
		__idc_irq_enable(val);
	} else {
		__idc_irq_disable(val);
	}
	__idc_du_enable(du, du_enabled);
	spin_unlock(&g_idc_dev->lock);

	return count;
}

static ssize_t sunxi_du_delay_cycles_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int data;
	unsigned int du = index->index;

	WARN_ON(du >= CHANNEL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	data = __idc_du_get_delay_time(du);
	spin_unlock(&g_idc_dev->lock);
	return scnprintf(buf, PAGE_SIZE,
		"%d\n", data);
}

static ssize_t sunxi_du_delay_cycles_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int du = index->index;
	bool en;

	WARN_ON(du >= CHANNEL_NUMBERS);
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	spin_lock(&g_idc_dev->lock);
	en = __idc_du_get_enabled(du);
	__idc_du_enable(du, false);
	__idc_du_set_delay_time(du, val);
	__idc_du_enable(du, en);
	spin_unlock(&g_idc_dev->lock);
	return count;
}

static ssize_t sunxi_du_wfi_check_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int data;
	unsigned int du = index->index;

	WARN_ON(du >= CHANNEL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	data = __sunxi_idc_read(DU_CTRL_REG(du));
	spin_unlock(&g_idc_dev->lock);
	return scnprintf(buf, PAGE_SIZE,
		"%d\n", data & DU_WFI_CHECK_EN ? 1 : 0);
}

static ssize_t sunxi_du_wfi_check_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int du = index->index;
	bool en;

	WARN_ON(du >= CHANNEL_NUMBERS);
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	spin_lock(&g_idc_dev->lock);
	en = __idc_du_get_enabled(du);
	__idc_du_enable(du, false);
	if (val)
		__idc_du_enable_wfi_check(du);
	else
		__idc_du_disable_wfi_check(du);
	__idc_du_enable(du, en);
	spin_unlock(&g_idc_dev->lock);
	return count;
}

static struct index_attribute sunxi_du_enable_attr =
	__ATTR(enable, S_IWUSR | S_IRUGO, sunxi_du_enable_show,
	sunxi_du_enable_store);
static struct index_attribute sunxi_du_irq_attr =
	__ATTR(irq, S_IWUSR | S_IRUGO, sunxi_du_irq_show,
	sunxi_du_irq_store);
static struct index_attribute sunxi_du_delay_cycles_attr =
	__ATTR(delay_cycles, S_IWUSR | S_IRUGO, sunxi_du_delay_cycles_show,
	sunxi_du_delay_cycles_store);
static struct index_attribute sunxi_du_wfi_check_attr =
	__ATTR(wfi_check, S_IWUSR | S_IRUGO, sunxi_du_wfi_check_show,
	sunxi_du_wfi_check_store);

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *du_default_attrs[] = {
	&sunxi_du_enable_attr.attr,
	&sunxi_du_irq_attr.attr,
	&sunxi_du_delay_cycles_attr.attr,
	&sunxi_du_wfi_check_attr.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static ssize_t sunxi_dbg_enable_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int dbg = index->index;
	bool en;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	en = __idc_dbg_get_enabled(dbg);
	spin_unlock(&g_idc_dev->lock);
	return scnprintf(buf, PAGE_SIZE,
		"%d\n", en);
}

static ssize_t sunxi_dbg_enable_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int dbg = index->index;
	bool en;

	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	en = val == 0 ? false : true;
	spin_lock(&g_idc_dev->lock);
	__idc_dbg_enable(dbg, en);
	spin_unlock(&g_idc_dev->lock);
	return count;
}

static ssize_t sunxi_dbg_irq_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int dbg = index->index;
	unsigned int data;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	data = __sunxi_idc_read(INT_DU_CTRL(dbg));
	spin_unlock(&g_idc_dev->lock);

	return scnprintf(buf, PAGE_SIZE, "0x%x\n",
		INT_REG_TO_IRQ(data));
}

static ssize_t sunxi_dbg_irq_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int dbg = index->index;
	bool en;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val >= IRQ_NUMBERS)
		return -EINVAL;

	spin_lock(&g_idc_dev->lock);
	en = __idc_dbg_get_enabled(dbg);
	__idc_dbg_enable(dbg, false);
	__idc_dbg_select_irq(dbg, val);
	__idc_irq_enable(val);
	__idc_dbg_enable(dbg, en);
	spin_unlock(&g_idc_dev->lock);

	return count;
}

static ssize_t sunxi_dbg_delay_cycles_show(struct index_obj *index,
		struct index_attribute *attr, char *buf)
{
	unsigned int data;
	unsigned int dbg = index->index;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	spin_lock(&g_idc_dev->lock);
	data = __idc_dbg_get_current_value(dbg);
	spin_unlock(&g_idc_dev->lock);
	return scnprintf(buf, PAGE_SIZE,
		"%d\n", data);
}

static ssize_t sunxi_dbg_delay_cycles_store(struct index_obj *index,
					    struct index_attribute *attr,
					    const char *buf, size_t count)
{
	unsigned long val;
	unsigned int dbg = index->index;
	bool en;

	WARN_ON(dbg >= INT_DU_CTRL_NUMBERS);
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val != 0)
		return -EINVAL;

	spin_lock(&g_idc_dev->lock);
	en = __idc_dbg_get_enabled(dbg);
	__idc_dbg_enable(dbg, false);
	__idc_dbg_ovf_clear(dbg);
	__idc_dbg_restart(dbg);
	__idc_dbg_enable(dbg, en);
	spin_unlock(&g_idc_dev->lock);
	return count;
}


static struct index_attribute sunxi_dbg_enable_attr =
	__ATTR(enable, S_IWUSR | S_IRUGO, sunxi_dbg_enable_show,
	sunxi_dbg_enable_store);
static struct index_attribute sunxi_dbg_irq_attr =
	__ATTR(irq, S_IWUSR | S_IRUGO, sunxi_dbg_irq_show,
	sunxi_dbg_irq_store);
static struct index_attribute sunxi_dbg_delay_cycles_attr =
	__ATTR(delay_cycles, S_IWUSR | S_IRUGO, sunxi_dbg_delay_cycles_show,
	sunxi_dbg_delay_cycles_store);

static struct attribute *dbg_default_attrs[] = {
	&sunxi_dbg_enable_attr.attr,
	&sunxi_dbg_irq_attr.attr,
	&sunxi_dbg_delay_cycles_attr.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static ssize_t index_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
	struct index_attribute *attribute;
	struct index_obj *foo;

	attribute = to_index_attr(attr);
	foo = to_index_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(foo, attribute, buf);
}

static ssize_t index_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct index_attribute *attribute;
	struct index_obj *foo;

	attribute = to_index_attr(attr);
	foo = to_index_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(foo, attribute, buf, len);
}

static void index_release(struct kobject *kobj)
{
	struct index_obj *foo;

	foo = to_index_obj(kobj);
	kfree(foo);
}

static const struct sysfs_ops index_sysfs_ops = {
	.show	= index_attr_show,
	.store	= index_attr_store,
};

static struct kobj_type du_ktype = {
	.sysfs_ops	= &index_sysfs_ops,
	.release	= index_release,
	.default_attrs = du_default_attrs,
};

static struct kobj_type dbg_ktype = {
	.sysfs_ops	= &index_sysfs_ops,
	.release	= index_release,
	.default_attrs = dbg_default_attrs,
};

static struct index_obj *create_du_index_obj(struct platform_device *pdev,
						const char *name, int index)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);
	struct index_obj *foo;
	int retval;

	foo = kzalloc(sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return NULL;

	foo->kobj.kset = idc_dev->du_kset;
	retval = kobject_init_and_add(&foo->kobj, &du_ktype,
						NULL, "%s%d", name, index);
	if (retval) {
		kobject_put(&foo->kobj);
		return NULL;
	}
	kobject_uevent(&foo->kobj, KOBJ_ADD);
	foo->index = index;

	return foo;
}

static struct index_obj *create_dbg_index_obj(struct platform_device *pdev,
						const char *name, int index)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);
	struct index_obj *foo;
	int retval;

	foo = kzalloc(sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return NULL;

	foo->kobj.kset = idc_dev->dbg_kset;
	retval = kobject_init_and_add(&foo->kobj, &dbg_ktype,
						NULL, "%s%d", name, index);
	if (retval) {
		kobject_put(&foo->kobj);
		return NULL;
	}
	kobject_uevent(&foo->kobj, KOBJ_ADD);
	foo->index = index;

	return foo;
}

static void destroy_index_obj(struct index_obj *foo)
{
	kobject_put(&foo->kobj);
}

static int sunxi_idc_sysfs_create(struct platform_device *pdev)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);
	int i, j;

	for (i = 0; i < CHANNEL_NUMBERS; i++) {
		idc_dev->du_index[i] = create_du_index_obj(pdev, "du", i);
		if (!idc_dev->du_index[i])
			goto du_err;
	}
	i--;

	for (j = 0; j < INT_DU_CTRL_NUMBERS; j++) {
		idc_dev->dbg_index[j] = create_dbg_index_obj(pdev, "dbg", j);
		if (!idc_dev->dbg_index[j])
			goto dbg_err;
	}
	return 0;

dbg_err:
	for (; j >= 0; j--)
		destroy_index_obj(idc_dev->dbg_index[j]);
du_err:
	for (; i >= 0; i--)
		destroy_index_obj(idc_dev->du_index[i]);
	return -EINVAL;
}

static void sunxi_idc_sysfs_remove(struct platform_device *pdev)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);
	int i, j;

	for (i = 0; i < CHANNEL_NUMBERS; i++)
		destroy_index_obj(idc_dev->du_index[i]);
	for (j = 0; j < INT_DU_CTRL_NUMBERS; j++)
		destroy_index_obj(idc_dev->dbg_index[j]);
}

static int sunxi_idc_parse_cfg(struct platform_device *pdev)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);
	struct device_node *np;
	unsigned int i, j, irq, ret, wfi_check_en, du_en = 0;

	np = pdev->dev.of_node;
	ret = of_property_read_u32_array(np, "irq_delay_en",
			idc_dev->idc_regs.irq_delay_en, IRQ_DELAY_EN_REGS);
	if (ret) {
		pr_err("get irq_delay_en error.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "irq_channel_array",
			idc_dev->idc_regs.irq_select_reg, IRQ_SELECT_REGS);
	if (ret) {
		pr_err("get irq_channel_array error.\n");
		return -EINVAL;
	}

	for (i = 0; i < IRQ_DELAY_EN_REGS; i++) {
		if (idc_dev->idc_regs.irq_delay_en[i] == 0)
			continue;
		for (j = 0; j < 32; j++) {
			if ((idc_dev->idc_regs.irq_delay_en[i]>>j) & 0x1) {
				irq = i*32+j;
				du_en |= 0x1 <<
				((idc_dev->idc_regs.irq_select_reg[irq/8]
				>>((irq%8)*4))&0xf);
			}
		}
	}

	ret = of_property_read_u32_array(np, "channel_delay_cycle",
			idc_dev->idc_regs.delay_interval_reg, CHANNEL_NUMBERS);
	if (ret) {
		pr_err("get channel_delay_cycle error.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "wfi_check_en",
			&wfi_check_en);
	if (ret) {
		pr_err("get wfi_check_en error.\n");
		return -EINVAL;
	}

	for (i = 0; i < CHANNEL_NUMBERS; i++) {
		idc_dev->idc_regs.delay_ctrl_reg[i] |= DU_CLK_24M;
		if ((wfi_check_en >> i) & 0x1)
			idc_dev->idc_regs.delay_ctrl_reg[i] |= DU_WFI_CHECK_EN;
		if ((du_en >> i) & 0x1)
			idc_dev->idc_regs.delay_ctrl_reg[i] |= DU_EN;
	}

	return 0;
}

void idc_init(struct platform_device *pdev)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);

	spin_lock(&idc_dev->lock);
	sunxi_idc_parse_cfg(pdev);
	__idc_restore_regs(idc_dev);
	__idc_enable();
	spin_unlock(&idc_dev->lock);
	idc_print_regs(idc_dev->base, 0x338, idc_dev->base);
}

static int sunxi_idc_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct sunxi_idc *idc_dev;
	struct resource *res;

	idc_dev = kzalloc(sizeof(*idc_dev), GFP_KERNEL);
	if (!idc_dev)
		return  -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_dbg(dev, "Unable to find resource region\n");
		ret = -ENOENT;
		goto err_res;
	}

	idc_dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (!idc_dev->base) {
		dev_dbg(dev, "Unable to map IOMEM @ PA:%#x\n",
				(unsigned int)res->start);
		ret = -ENOENT;
		goto err_res;
	}

	platform_set_drvdata(pdev, idc_dev);
	idc_dev->dev = dev;
	idc_dev->du_kset = kset_create_and_add("du", NULL, &dev->kobj);
	if (!idc_dev->du_kset)
		goto err_res;

	idc_dev->dbg_kset = kset_create_and_add("dbg", NULL, &dev->kobj);
	if (!idc_dev->dbg_kset)
		goto err_kset;

	spin_lock_init(&idc_dev->lock);
	g_idc_dev = idc_dev;

	sunxi_idc_sysfs_create(pdev);
	idc_init(pdev);
	return 0;

err_kset:
	kset_unregister(idc_dev->du_kset);
err_res:
	kfree(idc_dev);
	dev_err(dev, "Failed to initialize\n");
	return ret;
}

static int sunxi_idc_remove(struct platform_device *pdev)
{
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);

	sunxi_idc_sysfs_remove(pdev);
	kset_unregister(idc_dev->du_kset);
	kset_unregister(idc_dev->dbg_kset);
	devm_iounmap(idc_dev->dev, idc_dev->base);
	kfree(idc_dev);

	return 0;
}

static int sunxi_idc_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);

	spin_lock(&idc_dev->lock);
	__idc_save_regs(idc_dev);
	__idc_disable();
	spin_unlock(&idc_dev->lock);
	return 0;
}

static int sunxi_idc_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_idc *idc_dev = platform_get_drvdata(pdev);

	spin_lock(&idc_dev->lock);
	__idc_restore_regs(idc_dev);
	__idc_enable();
	spin_unlock(&idc_dev->lock);

	return 0;
}

const struct dev_pm_ops sunxi_idc_pm_ops = {
	.suspend	= sunxi_idc_suspend,
	.resume	= sunxi_idc_resume,
};

static const struct of_device_id sunxi_idc_dt[] = {
	{ .compatible = "allwinner,sunxi-idc", },
};
MODULE_DEVICE_TABLE(of, sunxi_idc_dt);

static struct platform_driver sunxi_idc_driver = {
	.probe		= sunxi_idc_probe,
	.remove		= sunxi_idc_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "idc",
		.pm	= &sunxi_idc_pm_ops,
		.of_match_table = sunxi_idc_dt,
	}
};

static int __init sunxi_idc_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_matching_node(NULL, sunxi_idc_dt);
	if (!np)
		return 0;

	of_node_put(np);
	ret = platform_driver_register(&sunxi_idc_driver);

	return ret;
}

static void __exit sunxi_idc_exit(void)
{
	platform_driver_unregister(&sunxi_idc_driver);
}

subsys_initcall(sunxi_idc_init);
module_exit(sunxi_idc_exit);

MODULE_DESCRIPTION("IDC Driver for Allwinner");
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_ALIAS("platform:allwinner");
MODULE_LICENSE("GPL v2");
