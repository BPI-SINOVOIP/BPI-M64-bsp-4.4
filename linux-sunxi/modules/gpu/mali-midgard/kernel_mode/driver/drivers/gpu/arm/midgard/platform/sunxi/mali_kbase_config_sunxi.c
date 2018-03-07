/*
 * Copyright (C) 2016-2017 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk-provider.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>

#include "platform.h"

#define GPU_DEFAULT_FREQ (624 * 1000 * 1000) /* Hz */
#define GPU_DEFAULT_VOL (900 * 1000) /* uV */

#define SMC_REG_BASE 0x3007000
#define SMC_GPU_DRM_REG (SMC_REG_BASE) + 0x54
#define GPU_POWEROFF_GATING_REG (0x07010000 + 0x0254)

#define sunxi_err(format, args...) \
	do { \
		if (sunxi_kbdev->dev == NULL) \
			dev_err(sunxi_kbdev->dev, format, ## args); \
	} while (false)

static struct kbase_device *sunxi_kbdev;
static struct device_node *sunxi_np;
static struct sunxi_regs *sunxi_regs;
static struct sunxi_clks *sunxi_clks;
#if defined(CONFIG_REGULATOR)
static struct regulator *sunxi_regulator;
#endif
#ifdef CONFIG_DEBUG_FS
static struct sunxi_debug *sunxi_debug;
static struct dentry *sunxi_debugfs;
#endif /* CONFIG_DEBUG_FS */

static bool sunxi_initialized;

/* GPU relevant functionality */
static bool sunxi_idle_enabled;

static inline void set_reg_bit(unsigned char bit,
                        bool val, void __iomem *ioaddr)
{
	int data;

	if (ioaddr == NULL)
		return;

	data = readl(ioaddr);
	if (!val)
		data &= ~(1 << bit);
	else
		data |= (1 << bit);

	writel(data, ioaddr);
}

static inline void *alloc_os_mem(size_t size)
{
	void *vaddr;

	vaddr = kzalloc(sizeof(struct sunxi_private), GFP_KERNEL);

	return vaddr;
}

static inline void free_os_mem(void *vaddr)
{
	if (vaddr == NULL)
		return;

	kfree(vaddr);
}

static inline void ioremap_regs(void)
{
	struct reg *drm;
	struct reg *poweroff_gating;

	sunxi_regs = alloc_os_mem(sizeof(struct sunxi_regs));
	if (sunxi_regs == NULL)
		return;

	drm = &sunxi_regs->drm;
	poweroff_gating = &sunxi_regs->poweroff_gating;

	drm->phys_addr = SMC_GPU_DRM_REG;
	poweroff_gating->phys_addr = GPU_POWEROFF_GATING_REG;

	drm->ioaddr = ioremap(drm->phys_addr, 1);
	poweroff_gating->ioaddr = ioremap(poweroff_gating->phys_addr, 1);
}

static inline void iounmap_regs(void)
{
	struct reg *drm;
	struct reg *poweroff_gating;

	if (sunxi_regs == NULL)
		return;

	drm = &sunxi_regs->drm;
	poweroff_gating = &sunxi_regs->poweroff_gating;

	iounmap(drm->ioaddr);
	iounmap(poweroff_gating->ioaddr);

	free_os_mem(sunxi_regs);
}

static inline int set_poweroff_gating(bool status)
{
	void __iomem *ioaddr;

	ioaddr = sunxi_regs->poweroff_gating.ioaddr;
	if (ioaddr == NULL)
		return 0;

	set_reg_bit(0, status, ioaddr);

	return 0;
}

static inline int set_gpu_drm_mode(bool status)
{
	void __iomem *ioaddr;

	ioaddr = sunxi_regs->drm.ioaddr;
	if (ioaddr == NULL)
		return 0;

	set_reg_bit(0, status, ioaddr);

	return 0;
}

static int get_gpu_clk(void)
{
	sunxi_clks = alloc_os_mem(sizeof(struct sunxi_clks));
	if (sunxi_clks == NULL) {
		sunxi_err("failed to allocate memory for sunxi_clks!\n");
		return -ENOMEM;
	}

	sunxi_clks->pll = of_clk_get(sunxi_np, 0);
	if (IS_ERR_OR_NULL(sunxi_clks->pll)) {
		sunxi_err("failed to get GPU pll clock!\n");
		return -EPERM;
	}

	sunxi_clks->core = of_clk_get(sunxi_np, 1);
	if (IS_ERR_OR_NULL(sunxi_clks->core)) {
		sunxi_err("failed to get GPU core clock!\n");
		return -EPERM;
	}

	return 0;
}

static void put_gpu_clk(void)
{
	if (sunxi_clks == NULL)
		return;

	free_os_mem(sunxi_clks);
}

static int enable_gpu_clk(void)
{
	int err;

	if (sunxi_clks == NULL) {
		sunxi_err("sunxi_clks is invalid!\n");
		return 0;
	}

	if (!__clk_is_enabled(sunxi_clks->pll)) {
		err = clk_prepare_enable(sunxi_clks->pll);
		if (err)
			return err;
	}

	if (!__clk_is_enabled(sunxi_clks->core)) {
		err = clk_prepare_enable(sunxi_clks->core);
		if (err)
			return err;
	}

	return 0;
}

static void disable_gpu_clk(void)
{
	if (__clk_is_enabled(sunxi_clks->core))
		clk_disable_unprepare(sunxi_clks->core);
	if (__clk_is_enabled(sunxi_clks->pll))
		clk_disable_unprepare(sunxi_clks->pll);
}

int set_gpu_freq(unsigned long freq /* Hz */)
{
	int err;
	unsigned long current_freq;

	if (sunxi_clks == NULL)
		return 0;

	current_freq = clk_get_rate(sunxi_clks->core);
	if (freq == current_freq)
		return 0;

	err = clk_set_rate(sunxi_clks->pll, freq);
	if (err) {
		sunxi_err("failed to set GPU pll clock to %lu MHz!\n",
					freq/1000/1000);
		return err;
	}

	err = clk_set_rate(sunxi_clks->core, freq);
	if (err) {
		sunxi_err("failed to set GPU core clock to %lu MHz!\n",
					freq/1000/1000);
		return err;
	}

	return 0;
}

#if defined(CONFIG_REGULATOR)
static int get_gpu_power(void)
{
	sunxi_regulator = regulator_get(NULL, "vdd-gpu");
	if (sunxi_regulator == NULL) {
		sunxi_err("failed to get the GPU regulator!\n");
		return -EPERM;
	}

	return 0;
}

static void put_gpu_power(void)
{
	sunxi_regulator = NULL;
}

static int enable_gpu_power(void)
{
	int err;

	if (sunxi_regulator == NULL) {
		sunxi_err("sunxi_regulator is invalid!\n");
		return 0;
	}

	/*
	 * regulator_is_enabled is the status for the GPU DCDC hardware. And
	 * GPU power is always enabled during the system boot. at the same
	 * time, however, the refcount of GPU regulator is 0. So here we force
	 * calling regulator_enable at the first time to make the result of
	 * regulator_is_enabled and regulator refcount match or else kernel
	 * crash may occur.
	 */
	if (sunxi_initialized && regulator_is_enabled(sunxi_regulator))
		return 0;

	err = regulator_enable(sunxi_regulator);
	if (err) {
		sunxi_err("failed to enable GPU power!\n");
		return err;
	}

	set_poweroff_gating(0);

	return err;
}

static void disable_gpu_power(void)
{
	int err = 0;

	/* If GPU is not initialized, regulator_disable must not be used. */
	if (!sunxi_initialized)
		return;

	if (sunxi_regulator == NULL) {
		sunxi_err("sunxi_regulator is invalid!\n");
		return;
	}

	if (!regulator_is_enabled(sunxi_regulator))
		return;

	set_poweroff_gating(1);

	err = regulator_disable(sunxi_regulator);
	if (err)
		sunxi_err("failed to disable GPU power!\n");
}

static int set_gpu_voltage(int vol /* uV */)
{
	int err;
	int current_vol;

	if (sunxi_regulator == NULL) {
		sunxi_err("sunxi_regulator is invalid!\n");
		return 0;
	}

	current_vol = regulator_get_voltage(sunxi_regulator);
	if (vol == current_vol)
		return 0;

	err = regulator_set_voltage(sunxi_regulator, vol, vol);
	if (err)
		sunxi_err("failed to set GPU voltage to %d mV!\n",
					vol/1000);

	return err;
}
#endif /* CONFIG_REGULATOR */

#ifdef CONFIG_DEBUG_FS
static ssize_t write_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *offp)
{
	int i, err;
	int freq, vol;
	unsigned long val;
	bool semicolon = false;
	char buffer[50], data[32];
	int head_size, data_size;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	/* The command format is '<head>:<data>' or '<head>:<data>;' */
	for (i = 0; i < count; i++) {
		if (*(buffer+i) == ':')
			head_size = i;
		if (*(buffer+i) == ';' && head_size) {
			data_size = count - head_size - 3;
			semicolon = true;
			break;
		}
	}

	if (!head_size)
		goto err_out;

	if (!semicolon)
		data_size = count - head_size - 2;

	if (data_size > 32)
		goto err_out;

	memcpy(data, buffer + head_size + 1, data_size);
	data[data_size] = '\0';

	err = kstrtoul(data, 10, &val);
	if (err)
		goto err_out;

	if (!strncmp("enable", buffer, head_size)) {
		sunxi_debug->enable = val ? true : false;
	} else if (!strncmp("frequency", buffer, head_size)) {
		if (val == 0 || val == 1)
			sunxi_debug->frequency = val ? true : false;
		else
			freq = val;
	} else if (!strncmp("voltage", buffer, head_size)) {
		if (val == 0 || val == 1)
			sunxi_debug->voltage = val ? true : false;
		else
			vol = val;
	} else if (!strncmp("power", buffer, head_size)) {
		sunxi_debug->power = val ? true : false;
	} else if (!strncmp("idle", buffer, head_size)) {
		if (val == 0 || val == 1)
			sunxi_debug->idle = val ? true : false;
		else
			sunxi_idle_enabled = (val - 2) ? true : false;
	} else {
		goto err_out;
	}

	return count;

err_out:
	sunxi_err("Invalid parameter!\n");
	return -EINVAL;
}

static const struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.write = write_write,
};

static int dump_debugfs_show(struct seq_file *s, void *private_data)
{
	int vol;
	unsigned long freq;

	if (!sunxi_debug->enable)
		return 0;

	if (sunxi_debug->frequency) {
		freq = clk_get_rate(sunxi_clks->core) / (1000 * 1000);
		seq_printf(s, "frequency:%luMHz;", freq);
	}

	if (sunxi_debug->voltage) {
		vol = regulator_get_voltage(sunxi_regulator) / 1000;
		seq_printf(s, "voltage:%dmV;", vol);
	}

	if (sunxi_debug->power)
		seq_printf(s, "power:%s;",
			regulator_is_enabled(sunxi_regulator) ? "on" : "off");

	if (sunxi_debug->idle)
		seq_printf(s, "idle:%s;", sunxi_idle_enabled ? "on" : "off");

	seq_puts(s, "\n");

	return 0;
}

static int dump_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_debugfs_show, inode->i_private);
}

static const struct file_operations dump_fops = {
	.owner = THIS_MODULE,
	.open = dump_debugfs_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sunxi_create_debugfs(void)
{
	struct dentry *node;

	sunxi_debugfs = debugfs_create_dir("sunxi_gpu", NULL);
	if (IS_ERR_OR_NULL(sunxi_debugfs))
		return -ENOMEM;

	node = debugfs_create_file("write", 0644,
				sunxi_debugfs, NULL, &write_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	node = debugfs_create_file("dump", 0644,
				sunxi_debugfs, NULL, &dump_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	sunxi_debug = alloc_os_mem(sizeof(struct sunxi_debug));
	if (sunxi_debug == NULL)
		return -ENOMEM;

	return 0;
}
#endif /* CONFIG_DEBUG_FS */

static int parse_dts_and_fex(void)
{
#ifdef CONFIG_OF
	int val;

	if (!of_property_read_u32(sunxi_np, "gpu_idle", &val))
		sunxi_idle_enabled = val ? true : false;
#endif /* CONFIG_OF */

	return 0;
}

int sunxi_platform_init(struct kbase_device *kbdev)
{
	int err;

	sunxi_kbdev = kbdev;

	sunxi_np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (sunxi_np == NULL) {
		sunxi_err("of_find_compatible_node failed!\n");
		return -EINVAL;
	}

	err = parse_dts_and_fex();
	if (err)
		return err;

#if defined(CONFIG_REGULATOR)
	get_gpu_power();

	err = set_gpu_voltage(GPU_DEFAULT_VOL);
	if (err)
		return err;
#endif /* CONFIG_REGULATOR */

	ioremap_regs();

	err = get_gpu_clk();
	if (err)
		return err;

	err = set_gpu_freq(GPU_DEFAULT_FREQ);
	if (err)
		return err;

#if defined(CONFIG_REGULATOR)
	err = enable_gpu_power();
	if (err)
		return err;
#endif /* CONFIG_REGULATOR */

	err = enable_gpu_clk();
	if (err)
		return err;

#ifdef CONFIG_DEBUG_FS
	err = sunxi_create_debugfs();
	if (err)
		return err;
#endif /* CONFIG_DEBUG_FS */

	sunxi_initialized = true;

	return 0;
}

void sunxi_platform_term(struct kbase_device *kbdev)
{
	disable_gpu_clk();

	put_gpu_clk();

#if defined(CONFIG_REGULATOR)
	disable_gpu_power();

	put_gpu_power();
#endif /* CONFIG_REGULATOR */

	iounmap_regs();

	sunxi_initialized = false;
}

struct kbase_platform_funcs_conf sunxi_platform_conf = {
	.platform_init_func = sunxi_platform_init,
	.platform_term_func = sunxi_platform_term,
};

int kbase_platform_early_init(void)
{
	return 0;
}

static int sunxi_power_on(struct kbase_device *kbdev)
{
	int err;

#if defined(CONFIG_REGULATOR)
	err = enable_gpu_power();
	if (err)
		return err;
#endif /* CONFIG_REGULATOR */

	err = enable_gpu_clk();
	if (err)
		return err;

	return 0;
}

static void sunxi_power_off(struct kbase_device *kbdev)
{
	disable_gpu_clk();

	if (!sunxi_idle_enabled)
		return;

#if defined(CONFIG_REGULATOR)
	disable_gpu_power();
#endif /* CONFIG_REGULATOR */
}

struct kbase_pm_callback_conf sunxi_pm_callbacks = {
	.power_on_callback      = sunxi_power_on,
	.power_off_callback     = sunxi_power_off,
	.power_suspend_callback = NULL,
	.power_resume_callback  = NULL
};
