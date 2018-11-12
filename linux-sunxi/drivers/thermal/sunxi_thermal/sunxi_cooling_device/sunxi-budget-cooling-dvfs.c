/*
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: JiaRui Xiao <xiaojiarui@allwinnertech.com>
 *
 * allwinner sunxi thermal cooling device driver for dvfs .
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/sunxi-sid.h>
#include "sunxi-budget-cooling.h"

#ifdef CONFIG_SUNXI_BUDGET_COOLING_FREQTBL

#define IS_STATE_VALUE_VALID(target, max) ((target < max) ? 1 : 0)
static int sunxi_cpu_budget_get_freqtable(struct device_node *of_node,
				struct sunxi_cpufreq_cooling_table *tbl,
				int cluster_num, int state, int state_value)
{
	int max_level = 0;
	struct cpufreq_frequency_table *freq_table, *pos;
	struct device *dev = container_of(&of_node, struct device, of_node);
	struct sunxi_budget_cooling_device *bcd = container_of(&dev,
				struct sunxi_budget_cooling_device, dev);

	freq_table = cpufreq_frequency_get_table(
				cpumask_first(&bcd->cluster_cpus[cluster_num]));
	if (!freq_table) {
		pr_debug("%s: CPUFreq table not found\n", __func__);
		return -1;
	}
	cpufreq_for_each_valid_entry(pos, freq_table)
		max_level++;
	if (IS_STATE_VALUE_VALID(state_value, max_level)) {
		tbl[state].cluster_freq[cluster_num] =
			freq_table[max_level - state_value - 1].frequency;
	}

	return 0;
}
#endif

static int is_cpufreq_valid(unsigned int cpu)
{
	struct cpufreq_policy policy;

	return !cpufreq_get_policy(&policy, cpu);
}

int sunxi_cpufreq_update_state(
	struct sunxi_budget_cooling_device *cooling_device,
	u32 cluster)
{
	unsigned long flags;
	s32 ret = 0;
	u32 cpuid;
	u32 cooling_state = cooling_device->cooling_state;
	struct cpufreq_policy policy;
	struct sunxi_budget_cpufreq *cpufreq = cooling_device->cpufreq;

	if (cpufreq == NULL)
		return 0;

	spin_lock_irqsave(&cpufreq->lock, flags);

	cpufreq->cluster_freq_limit[cluster] =
			cpufreq->tbl[cooling_state].cluster_freq[cluster];

	spin_unlock_irqrestore(&cpufreq->lock, flags);

	for_each_cpu(cpuid, &cooling_device->cluster_cpus[cluster]) {
		if (is_cpufreq_valid(cpuid)) {
			if ((cpufreq_get_policy(&policy, cpuid) == 0) &&
							policy.governor) {
				ret = cpufreq_update_policy(cpuid);
				break;
			}
		}
	}

	return ret;
}
EXPORT_SYMBOL(sunxi_cpufreq_update_state);

int sunxi_cpufreq_get_roomage(
	struct sunxi_budget_cooling_device *cooling_device,
	u32 *freq_floor,
	u32 *freq_roof,
	u32 cluster)
{
	struct sunxi_budget_cpufreq *cpufreq = cooling_device->cpufreq;

	if (cpufreq == NULL)
		return 0;
	*freq_floor = cpufreq->cluster_freq_floor[cluster];
	*freq_roof = cpufreq->cluster_freq_roof[cluster];
	return 0;
}
EXPORT_SYMBOL(sunxi_cpufreq_get_roomage);

int sunxi_cpufreq_set_roomage(struct sunxi_budget_cooling_device *cooling_device,
				u32 freq_floor, u32 freq_roof, u32 cluster)
{
	unsigned long flags;
	struct sunxi_budget_cpufreq *cpufreq = cooling_device->cpufreq;

	if (cpufreq == NULL)
		return 0;
	spin_lock_irqsave(&cpufreq->lock, flags);

	cpufreq->cluster_freq_floor[cluster] = freq_floor;
	cpufreq->cluster_freq_roof[cluster] = freq_roof;

	spin_unlock_irqrestore(&cpufreq->lock, flags);
	sunxi_cpufreq_update_state(cooling_device, cluster);
	return 0;
}
EXPORT_SYMBOL(sunxi_cpufreq_set_roomage);

static int cpufreq_thermal_notifier(struct notifier_block *nb,
					unsigned long event, void *data)
{
	struct sunxi_budget_cpufreq *cpufreq = container_of(nb,
					struct sunxi_budget_cpufreq, notifer);
	struct sunxi_budget_cooling_device *bcd = cpufreq->bcd;
	struct cpufreq_policy *policy = data;
	int cluster = 0;
	unsigned long limit_freq = 0, base_freq = 0, head_freq = 0;
	unsigned long max_freq = 0, min_freq = 0;

	if (event != CPUFREQ_ADJUST || cpufreq == NOTIFY_INVALID)
		return 0;

	while (cluster < bcd->cluster_num) {
		if (cpumask_test_cpu(policy->cpu,
						&bcd->cluster_cpus[cluster])) {
			limit_freq = cpufreq->cluster_freq_limit[cluster];
			base_freq = cpufreq->cluster_freq_floor[cluster];
			head_freq = cpufreq->cluster_freq_roof[cluster];
			break;
		}
		cluster++;
	}

	if (cluster == bcd->cluster_num)
		return 0;

	if (limit_freq && limit_freq != INVALID_FREQ) {
		max_freq = (head_freq >= limit_freq)?limit_freq:head_freq;
		min_freq = base_freq;
		/* Never exceed policy.max*/
		if (max_freq > policy->max)
			max_freq = policy->max;
		if (min_freq < policy->min)
			min_freq = policy->min;

		min_freq = (min_freq < max_freq)?min_freq:max_freq;

		if (policy->max != max_freq || policy->min != min_freq) {
			cpufreq_verify_within_limits(policy,
						     min_freq,
						     max_freq);
			policy->user_policy.max = policy->max;
			pr_debug
			("CPU Budget:update CPU%d cpufreq max to min %lu~%lu\n",
			policy->cpu,
			max_freq,
			min_freq);
		}
	}
	return 0;
}

/**
 * sunxi_cpufreq_cooling_parse - parse the cpufreq limit value and
 * fill in struct sunxi_cpufreq_cooling_table
 */
static struct sunxi_cpufreq_cooling_table *
	sunxi_cpufreq_cooling_parse(struct device_node *np,
				    u32 tbl_num,
				    u32 cluster_num)
{
	struct sunxi_cpufreq_cooling_table *tbl;
	u32 i, j, ret = 0;
	char name[32];

	tbl = kcalloc(tbl_num, sizeof(*tbl), GFP_KERNEL);
	if (IS_ERR_OR_NULL(tbl)) {
		pr_err("cooling_dev: not enough memory for cpufreq cooling table\n");
		return NULL;
	}
	for (i = 0; i < tbl_num; i++) {
		sprintf(name, "state%d", i);
		for (j = 0; j < cluster_num; j++) {
#ifdef CONFIG_SUNXI_BUDGET_COOLING_FREQTBL
			u32 value = 0;
			if (of_property_read_u32_index(np, (const char *)&name,
							(j * 2), &value)) {
				pr_err("node %s get failed!\n", name);
				ret = -EBUSY;
			}
			sunxi_cpu_budget_get_freqtable(np, tbl, j, i, value);
#else
			if (of_property_read_u32_index(np, (const char *)&name,
					(j * 2), &(tbl[i].cluster_freq[j]))) {
				pr_err("node %s get failed!\n", name);
				ret = -EBUSY;
			}
#endif
		}
	}
	if (ret) {
		kfree(tbl);
		tbl = NULL;
	}
	return tbl;
}

struct sunxi_budget_cpufreq *
sunxi_cpufreq_cooling_register(struct sunxi_budget_cooling_device *bcd)
{
	struct sunxi_budget_cpufreq *cpufreq;
	struct sunxi_cpufreq_cooling_table *tbl;
	struct device_node *np = bcd->dev->of_node;
	u32 cluster;

	cpufreq = kzalloc(sizeof(*cpufreq), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpufreq)) {
		pr_err(
		"cooling_dev: not enough memory for cpufreq cooling data\n");
		goto fail;
	}

	tbl = sunxi_cpufreq_cooling_parse(np, bcd->state_num, bcd->cluster_num);
	if (!tbl) {
		kfree(cpufreq);
		goto fail;
	}
	cpufreq->tbl_num = bcd->state_num;
	cpufreq->tbl = tbl;
	spin_lock_init(&cpufreq->lock);
	for (cluster = 0; cluster < bcd->cluster_num; cluster++) {
		cpufreq->cluster_freq_limit[cluster] =
					cpufreq->tbl[0].cluster_freq[cluster];
		cpufreq->cluster_freq_roof[cluster] =
					cpufreq->cluster_freq_limit[cluster];
	}

	cpufreq->notifer.notifier_call = cpufreq_thermal_notifier;
	cpufreq_register_notifier(&(cpufreq->notifer),
						CPUFREQ_POLICY_NOTIFIER);
	cpufreq->bcd = bcd;
	pr_info("CPU freq cooling register Success\n");
	return cpufreq;
fail:
	return NULL;
}
EXPORT_SYMBOL(sunxi_cpufreq_cooling_register);

void sunxi_cpufreq_cooling_unregister(struct sunxi_budget_cooling_device *bcd)
{
	if (!bcd->cpufreq)
		return;
	cpufreq_unregister_notifier(&(bcd->cpufreq->notifer),
						CPUFREQ_POLICY_NOTIFIER);
	kfree(bcd->cpufreq->tbl);
	kfree(bcd->cpufreq);
	bcd->cpufreq = NULL;
}
EXPORT_SYMBOL(sunxi_cpufreq_cooling_unregister);

