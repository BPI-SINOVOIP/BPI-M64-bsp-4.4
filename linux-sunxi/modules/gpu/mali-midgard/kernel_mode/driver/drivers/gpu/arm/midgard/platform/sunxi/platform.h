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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

struct reg {
	unsigned long phys_addr;
	void __iomem *ioaddr;
};

struct sunxi_regs {
	struct reg poweroff_gating;
	struct reg drm;
};

struct sunxi_clks {
	
	struct clk *pll;
	struct clk *core;
};

#ifdef CONFIG_DEBUG_FS
struct sunxi_debug {
	bool enable;
	bool frequency;
	bool voltage;
	bool power;
	bool idle;
};
#endif /* CONFIG_DEBUG_FS */

struct sunxi_private {
	struct kbase_device *kbase_mali;
	struct sunxi_regs *regs;
	struct sunxi_clk *clks;
	struct device_node *np;
};

#endif /* _PLATFORM_H_ */
