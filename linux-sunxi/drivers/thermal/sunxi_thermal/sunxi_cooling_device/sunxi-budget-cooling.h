/*
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: JiaRui Xiao <xiaojiarui@allwinnertech.com>
 *
 * allwinner sunxi thermal cooling device driver struct data,
 * it include hotplug and dvfs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SUNXI_BUDGET_COOLING_H
#define SUNXI_BUDGET_COOLING_H

#include <linux/thermal.h>
#include <linux/cpufreq.h>

#define NOTIFY_INVALID NULL
#define INVALID_FREQ    (-1)
#define CLUSTER_MAX	(2)

struct sunxi_cpufreq_cooling_table {
	u32 cluster_freq[CLUSTER_MAX];
};

struct sunxi_hotplug_cooling_table {
	u32 cluster_num[CLUSTER_MAX];
};

struct sunxi_budget_cpufreq {
	u32 cluster_freq_limit[CLUSTER_MAX];
	u32 cluster_freq_roof[CLUSTER_MAX];
	u32 cluster_freq_floor[CLUSTER_MAX];
	u32 tbl_num;
	struct sunxi_cpufreq_cooling_table *tbl;
	struct notifier_block notifer;
	struct sunxi_budget_cooling_device *bcd;
	spinlock_t lock;
};

struct sunxi_budget_hotplug {
	u32 cluster_num_limit[CLUSTER_MAX];
	u32 cluster_num_roof[CLUSTER_MAX];
	u32 cluster_num_floor[CLUSTER_MAX];
	u32 tbl_num;
	int budget_take_count;
	struct sunxi_hotplug_cooling_table *tbl;
	struct notifier_block notifer;
	struct sunxi_budget_cooling_device *bcd;
	spinlock_t lock;
};

struct sunxi_budget_cooling_device {
	struct device *dev;
	struct thermal_cooling_device *cool_dev;
	u32 cooling_state;
	u32 state_num;
	u32 cluster_num;
	struct cpumask cluster_cpus[CLUSTER_MAX];
	struct sunxi_budget_cpufreq *cpufreq;
	struct sunxi_budget_hotplug *hotplug;
	struct list_head node;
};

#endif /* SUNXI_BUDGET_COOLING_H */
