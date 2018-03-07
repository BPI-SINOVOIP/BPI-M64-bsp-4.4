/*
 * linux/drivers/cpufreq/sunxi-iks-cpufreq.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sunxi iks cpufreq driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/of.h>
#include <asm/bL_switcher.h>
#include <linux/arisc/arisc.h>
#include <linux/regulator/consumer.h>
#include <linux/sunxi-sid.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/hrtimer.h>
#endif

#include "sunxi-iks-cpufreq.h"

#ifdef CONFIG_DEBUG_FS
static unsigned long long c0_set_time_usecs;
static unsigned long long c0_get_time_usecs;
static unsigned long long c1_set_time_usecs;
static unsigned long long c1_get_time_usecs;
#endif

static cpumask_var_t cpu_mask_1, cpu_mask_2;
#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1) || defined(CONFIG_ARCH_SUN8IW17P1)
static struct cpufreq_dvfs_table cpufreq_dvfs_table[DVFS_VF_TABLE_MAX] = {
	{0, 0, 3},
};
/*
 * Notice:
 * The the definition of the minimum frequnecy should be a valid value
 * in the frequnecy table, otherwise, there may be some power efficiency
 * lost in the interactive governor, when the cpufreq_interactive_idle_start
 * try to check the frequency status:
 * if (pcpu->target_freq != pcpu->policy->min) {}
 * the target_freq will never equal to the policy->min !!! Then, the timer
 * will wakeup the cpu frequently
*/
/* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MAX           (2016000000)
/* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MIN           (480000000)
/* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MAX           SUNXI_CPUFREQ_L_MAX
/* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MIN           SUNXI_CPUFREQ_L_MIN

#else
/*
 * Notice:
 * The the definition of the minimum frequnecy should be a valid value
 * in the frequnecy table, otherwise, there may be some power efficiency
 * lost in the interactive governor, when the cpufreq_interactive_idle_start
 * try to check the frequency status:
 * if (pcpu->target_freq != pcpu->policy->min) {}
 * the target_freq will never equal to the policy->min !!! Then, the timer
 * will wakeup the cpu frequently
*/
/* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MAX           (1200000000)
/* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MIN           (480000000)
/* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MAX           (1800000000)
/* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MIN           (600000000)

#endif

#ifdef CONFIG_BL_SWITCHER
bool bL_switching_enabled;
#endif

int sunxi_dvfs_debug;

static struct clk  *cluster_clk[MAX_CLUSTERS];
static unsigned int cluster_pll[MAX_CLUSTERS];
static struct cpufreq_frequency_table *freq_table[MAX_CLUSTERS + 1];

static DEFINE_PER_CPU(unsigned int, physical_cluster);
static DEFINE_PER_CPU(unsigned int, cpu_last_req_freq);

static struct mutex cluster_lock[MAX_CLUSTERS];
static unsigned int sunxi_cpufreq_set_rate(u32 cpu, u32 old_cluster,
						u32 new_cluster, u32 rate);

static unsigned int find_cluster_maxfreq(int cluster)
{
	int j;
	u32 max_freq = 0, cpu_freq;

	for_each_online_cpu(j) {
		cpu_freq = per_cpu(cpu_last_req_freq, j);
		if ((cluster == per_cpu(physical_cluster, j)) && (max_freq < cpu_freq))
			max_freq = cpu_freq;
	}

	if (unlikely(sunxi_dvfs_debug))
		CPUFREQ_DBG("%s: cluster:%d, max freq:%d\n", __func__, cluster, max_freq);

	return max_freq;
}

static unsigned int sunxi_clk_get_cpu_rate(unsigned int cpu)
{
	u32 cur_cluster = per_cpu(physical_cluster, cpu), rate;
	u32 other_cluster_cpu_num = 0;

#ifdef CONFIG_SCHED_SMP_DCMP
	u32 cpu_id;
	u32 other_cluster = MAX_CLUSTERS;
	u32 other_cluster_cpu = nr_cpu_ids;
	u32 other_cluster_rate;
	u32 tmp_cluster;
#endif

#ifdef CONFIG_DEBUG_FS
	ktime_t calltime = ktime_set(0, 0), delta, rettime;
#endif

#ifdef CONFIG_SCHED_SMP_DCMP
	for_each_online_cpu(cpu_id) {
		tmp_cluster = cpu_to_cluster(cpu_id);
		if (tmp_cluster != cur_cluster) {
			other_cluster_cpu_num++;
			if (other_cluster == MAX_CLUSTERS) {
				other_cluster_cpu = cpu_id;
				other_cluster = tmp_cluster;
			}
		}
	}
#endif

	if (other_cluster_cpu_num) {
		mutex_lock(&cluster_lock[A15_CLUSTER]);
		mutex_lock(&cluster_lock[A7_CLUSTER]);
	} else {
		mutex_lock(&cluster_lock[cur_cluster]);
	}

#ifdef CONFIG_DEBUG_FS
	calltime = ktime_get();
#endif

	rate = clk_get_rate(cluster_clk[cur_cluster]) / 1000;
	/* For switcher we use virtual A15 clock rates */
	if (is_bL_switching_enabled())
		rate = VIRT_FREQ(cur_cluster, rate);

#ifdef CONFIG_SCHED_SMP_DCMP
	if (other_cluster_cpu_num) {
		other_cluster_rate = clk_get_rate(cluster_clk[other_cluster]);
		other_cluster_rate /= 1000;
		/* For switcher we use virtual A15 clock rates */
		if (is_bL_switching_enabled())
			other_cluster_rate = VIRT_FREQ(other_cluster,
							other_cluster_rate);
	}
#endif

#ifdef CONFIG_DEBUG_FS
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	if (cur_cluster == A7_CLUSTER)
		c0_get_time_usecs = ktime_to_ns(delta) >> 10;
	else if (cur_cluster == A15_CLUSTER)
		c1_get_time_usecs = ktime_to_ns(delta) >> 10;
#endif

	if (unlikely(sunxi_dvfs_debug))
		CPUFREQ_DBG("cpu:%d, cur_cluster:%d,  cur_freq:%d\n",
					cpu, cur_cluster, rate);

#ifdef CONFIG_SCHED_SMP_DCMP
	if (other_cluster_cpu_num) {
		mutex_unlock(&cluster_lock[A7_CLUSTER]);
		mutex_unlock(&cluster_lock[A15_CLUSTER]);
		if (other_cluster_rate != rate) {
			sunxi_cpufreq_set_rate(other_cluster_cpu,
					other_cluster, other_cluster, rate);
		}
	} else
#endif
		mutex_unlock(&cluster_lock[cur_cluster]);

	return rate;

}

#ifdef CONFIG_SUNXI_ARISC
static int clk_set_fail_notify(void *arg)
{
	CPUFREQ_ERR("%s: cluster: %d\n", __func__, (u32)arg);

	/* maybe should do others */

	return 0;
}
#endif

static int sunxi_clk_set_cpu_rate(unsigned int cluster, unsigned int cpu, unsigned int rate)
{
	int ret;
#ifdef CONFIG_DEBUG_FS
	ktime_t calltime = ktime_set(0, 0), delta, rettime;
#endif

	if (unlikely(sunxi_dvfs_debug))
		CPUFREQ_DBG("cpu:%d, cluster:%d, set freq:%u\n", cpu, cluster, rate);

#ifdef CONFIG_DEBUG_FS
	calltime = ktime_get();
#endif

#ifndef CONFIG_SUNXI_ARISC
	ret = 0;
#else
	/* the rate is base on khz */
	ret = arisc_dvfs_set_cpufreq(rate, cluster_pll[cluster],
					ARISC_MESSAGE_ATTR_SOFTSYN,
					clk_set_fail_notify, (void *)cluster);
#endif

#ifdef CONFIG_DEBUG_FS
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	if (cluster == A7_CLUSTER)
		c0_set_time_usecs = ktime_to_ns(delta) >> 10;
	else if (cluster == A15_CLUSTER)
		c1_set_time_usecs = ktime_to_ns(delta) >> 10;
#endif

	return ret;
}

static unsigned int sunxi_cpufreq_set_rate(u32 cpu, u32 old_cluster,
						u32 new_cluster, u32 rate)
{
	u32 new_rate, prev_rate;
	int ret;

	mutex_lock(&cluster_lock[new_cluster]);

	prev_rate = per_cpu(cpu_last_req_freq, cpu);
	per_cpu(cpu_last_req_freq, cpu) = rate;
	per_cpu(physical_cluster, cpu) = new_cluster;

	if (is_bL_switching_enabled()) {
		new_rate = find_cluster_maxfreq(new_cluster);
		new_rate = ACTUAL_FREQ(new_cluster, new_rate);
	} else
	    new_rate = rate;

	if (unlikely(sunxi_dvfs_debug))
		CPUFREQ_DBG("cpu:%d, old cluster:%d, new cluster:%d, target freq:%d\n",
				cpu, old_cluster, new_cluster, new_rate);
	ret = sunxi_clk_set_cpu_rate(new_cluster, cpu, new_rate);
	if (WARN_ON(ret)) {
		CPUFREQ_ERR("clk_set_rate failed:%d, new cluster:%d\n", ret, new_cluster);
		per_cpu(cpu_last_req_freq, cpu) = prev_rate;
		per_cpu(physical_cluster, cpu) = old_cluster;

		mutex_unlock(&cluster_lock[new_cluster]);

		return ret;
	}
	mutex_unlock(&cluster_lock[new_cluster]);

	if (is_bL_switching_enabled()) {
		/* Recalc freq for old cluster when switching clusters */
		if (old_cluster != new_cluster) {
			if (unlikely(sunxi_dvfs_debug))
				CPUFREQ_DBG("cpu:%d, switch from cluster-%d to cluster-%d\n",
						cpu, old_cluster, new_cluster);

			bL_switch_request(cpu, new_cluster);

			mutex_lock(&cluster_lock[old_cluster]);
			/* Set freq of old cluster if there are cpus left on it */
			new_rate = find_cluster_maxfreq(old_cluster);
			new_rate = ACTUAL_FREQ(old_cluster, new_rate);
			if (new_rate) {
				if (unlikely(sunxi_dvfs_debug))
					CPUFREQ_DBG("Updating rate of old cluster:%d, to freq:%d\n",
							old_cluster, new_rate);

				if (sunxi_clk_set_cpu_rate(old_cluster, cpu, new_rate))
					CPUFREQ_ERR("clk_set_rate failed: %d, old cluster:%d\n",
							ret, old_cluster);
			}
			mutex_unlock(&cluster_lock[old_cluster]);
		}
	}

	return 0;
	}

/* Validate policy frequency range */
static int sunxi_cpufreq_verify_policy(struct cpufreq_policy *policy)
{
	u32 cur_cluster = cpu_to_cluster(policy->cpu);

	return cpufreq_frequency_table_verify(policy, freq_table[cur_cluster]);
}

static int sunxi_cpufreq_set_target_index(struct cpufreq_policy *policy,
				unsigned int index)
{
	unsigned int cur_cluster, new_cluster, actual_cluster;
	unsigned int pre_freq, new_freq, i;
	unsigned int other_cluster;
	unsigned int ret = 0;

	cur_cluster = cpu_to_cluster(policy->cpu);
	new_freq = freq_table[cur_cluster][index].frequency;

	pre_freq = sunxi_clk_get_cpu_rate(policy->cpu);
	if (pre_freq == new_freq)
		return 0;

	new_cluster = actual_cluster = per_cpu(physical_cluster, policy->cpu);

	if (is_bL_switching_enabled()) {
		if ((actual_cluster == A15_CLUSTER) &&
			(new_freq < SUNXI_BL_SWITCH_THRESHOLD))
			new_cluster = A7_CLUSTER;
		else if ((actual_cluster == A7_CLUSTER) &&
			(new_freq > SUNXI_BL_SWITCH_THRESHOLD))
			new_cluster = A15_CLUSTER;
	}

	ret = sunxi_cpufreq_set_rate(policy->cpu, actual_cluster, new_cluster, new_freq);
#ifdef CONFIG_SCHED_SMP_DCMP
	for_each_online_cpu(i) {
		other_cluster = cpu_to_cluster(i);
		if (other_cluster != actual_cluster) {
			ret |= sunxi_cpufreq_set_rate(i, other_cluster, other_cluster, new_freq);
			break;
		}
	}
#endif
	return ret;

}

static inline u32 get_table_count(struct cpufreq_frequency_table *table)
{
	int count;

	for (count = 0; table[count].frequency != CPUFREQ_TABLE_END; count++)
		;

	return count;
}


static int merge_cluster_tables(void)
{
	int i, j, k = 0, count = 1;
	struct cpufreq_frequency_table *table;

	for (i = 0; i < MAX_CLUSTERS; i++)
		count += get_table_count(freq_table[i]);

	table = kzalloc(sizeof(*table) * count, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	freq_table[MAX_CLUSTERS] = table;

	/* Add in reverse order to get freqs in increasing order */
	for (i = MAX_CLUSTERS - 1; i >= 0; i--) {
		for (j = 0; freq_table[i][j].frequency != CPUFREQ_TABLE_END; j++) {
			table[k].frequency = VIRT_FREQ(i, freq_table[i][j].frequency);
			CPUFREQ_DBG("%s: index: %d, freq: %d\n", __func__, k, table[k].frequency);
			k++;
		}
	}

	table[k].driver_data = k;
	table[k].frequency = CPUFREQ_TABLE_END;

	CPUFREQ_DBG("%s: End, table: %p, count: %d\n", __func__, table, k);

	return 0;
}

static int __get_cpufreq_table(unsigned int cpu, unsigned int soc_bin,
		cpumask_var_t *cpumask_1, cpumask_var_t *cpumask_2)
{
	struct device *cpu_dev;
	unsigned int pcpu, pcluster, pcluster_tmp;
	int ret;

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		CPUFREQ_ERR("Failed to get cpu%d device\n", cpu);
		return -ENODEV;
	}

	cpu_to_pcpu(cpu, &pcpu, &pcluster);
	pcluster_tmp = pcluster;


	if (!zalloc_cpumask_var(cpumask_1, GFP_KERNEL)) {
		CPUFREQ_ERR("Failed to alloc cpumask, %s:%d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	if (!zalloc_cpumask_var(cpumask_2, GFP_KERNEL)) {
		CPUFREQ_ERR("Failed to alloc cpumask, %s:%d\n", __func__, __LINE__);
		goto err_0;;
	}

	ret = dev_pm_opp_of_get_sharing_cpus_by_soc_bin(cpu_dev,
							*cpumask_1, soc_bin);
	if (ret) {
		CPUFREQ_ERR("OPP-v2 opp-shared Error, %d, %d\n", __LINE__, __LINE__);
		goto err_1;
	}

	ret = dev_pm_opp_of_cpumask_add_table_by_soc_bin(*cpumask_1, soc_bin);
	if (ret) {
		CPUFREQ_ERR("Failed to add opp table for cluster:%d\n", pcluster);
		goto err_1;
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table[pcluster]);
	if (ret) {
		CPUFREQ_ERR("Failed to init cpufreq table for cluster:%d\n", pcluster);
		goto err_2;
	}

	cluster_clk[pcluster] = clk_get(cpu_dev, NULL);
	if (IS_ERR_OR_NULL(cluster_clk[pcluster])) {
		CPUFREQ_ERR("get cluster:%d's clk error\n", pcluster);
		goto err_3;
	}

	for_each_possible_cpu(cpu) {
		cpu_to_pcpu(cpu, &pcpu, &pcluster);
		if (pcluster_tmp != pcluster) {
			cpu_dev = get_cpu_device(cpu);
			if (!cpu_dev) {
				CPUFREQ_ERR("Failed to get cpu%d device\n", cpu);
				goto err_4;
			}
			break;
		}
	}

	ret = dev_pm_opp_of_get_sharing_cpus_by_soc_bin(cpu_dev,
							*cpumask_2, soc_bin);
	if (ret) {
		CPUFREQ_ERR("OPP-v2 opp-shared Error, %d, %d\n", __LINE__, __LINE__);
		goto err_4;
	}

	ret = dev_pm_opp_of_cpumask_add_table_by_soc_bin(*cpumask_2, soc_bin);
	if (ret) {
		CPUFREQ_ERR("Failed to add opp table for cluster:%d\n", pcluster);
		goto err_4;
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table[pcluster]);
	if (ret) {
		CPUFREQ_ERR("Failed to init cpufreq table for cluster:%d\n", pcluster);
		goto err_5;
	}

	cluster_clk[pcluster] = clk_get(cpu_dev, NULL);
	if (IS_ERR_OR_NULL(cluster_clk[pcluster])) {
		CPUFREQ_ERR("get cluster:%d's clk error\n", pcluster);
		goto err_6;
	}

	return 0;

err_6:
	kfree(freq_table[pcluster]);
err_5:
	dev_pm_opp_of_cpumask_remove_table(*cpumask_2);
err_4:
	clk_put(cluster_clk[pcluster_tmp]);
err_3:
	kfree(freq_table[pcluster_tmp]);
err_2:
	dev_pm_opp_of_cpumask_remove_table(*cpumask_1);
err_1:
	free_cpumask_var(*cpumask_2);
err_0:
	free_cpumask_var(*cpumask_1);
	return -1;
}

/* Export freq_table to sysfs */
static struct freq_attr *sunxi_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
	NULL,
};

static int sunxi_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	struct device_node *dvfs_main_np;
	struct dev_pm_opp *opp;
	struct device *dev;
	unsigned int table_count, soc_bin = 0;
	unsigned int pcpu;
	unsigned int cur_cluster;
	int ret, cpu;
	int i, j;
	unsigned long temp_freq;

	dvfs_main_np = of_find_node_by_path("/opp_dvfs_table");
	if (!dvfs_main_np) {
		CPUFREQ_ERR("No opp dvfs table node found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dvfs_main_np, "opp_table_count",
				&table_count)) {
		CPUFREQ_ERR("get vf_table_count failed\n");
		return -ENODEV;
	}

	if (table_count == 1) {
		pr_err("%s: only one opp_table\n", __func__);
		soc_bin = 0;
	} else {
		soc_bin = sunxi_get_soc_bin();
		pr_err("%s: support more opp_table and soc bin is %d\n",
					__func__, soc_bin);
	}

	ret = __get_cpufreq_table(policy->cpu, soc_bin, &cpu_mask_1, &cpu_mask_2);
	if (ret) {
		CPUFREQ_ERR("get cpufreq table error\n");
		return -EINVAL;
	}

	cpu_to_pcpu(policy->cpu, &pcpu, &cur_cluster);

	merge_cluster_tables();

	dev = get_cpu_device(policy->cpu);

	/* set cpu freq-table */
	if (is_bL_switching_enabled()) {
		/* bL switcher use the merged freq table */
		cpufreq_table_validate_and_show(policy, freq_table[MAX_CLUSTERS]);
		/* TODO: for this case should we transmit the vf table
		 * to cpus ?
		 */
	} else {
		/* HMP use the per-cluster freq table */
		cpufreq_table_validate_and_show(policy, freq_table[cur_cluster]);
		/* for this case, we should only transmit the cur cluster's
		 * vf table
		 */
		j = get_table_count(freq_table[cur_cluster]);

		for (i = j - 1; i >= 0; --i) {
			temp_freq =
				 freq_table[cur_cluster][i].frequency * 1000;
			rcu_read_lock();
			opp = dev_pm_opp_find_freq_ceil(dev, &temp_freq);
			rcu_read_unlock();
			cpufreq_dvfs_table[j-1-i].voltage =
				dev_pm_opp_get_voltage(opp) / 1000;
			cpufreq_dvfs_table[j-1-i].freq =
				dev_pm_opp_get_freq(opp);
			cpufreq_dvfs_table[j-1-i].axi_div =
				dev_pm_opp_axi_bus_divide_ratio(opp);
			pr_info("index:%d, volatge:%d, freq:%d, axi_div:%d ,%s\n",
					(j-1-i), cpufreq_dvfs_table[j-1-i].voltage,
					cpufreq_dvfs_table[j-1-i].freq,
					cpufreq_dvfs_table[j-1-i].axi_div,
					__func__);
		}

		/* send the vf table to the cpus*/
		arisc_dvfs_cfg_vf_table(0, j, cpufreq_dvfs_table);
	}

	/* Support turbo/boost mode */
	if (policy_has_boost_freq(policy)) {
		/* This gets disabled by core on driver unregister */
		ret = cpufreq_enable_boost_support();
		if (ret) {
			CPUFREQ_ERR("cpufreq enable boost support error\n");
			dev_pm_opp_of_cpumask_remove_table(cpu_mask_1);
			dev_pm_opp_of_cpumask_remove_table(cpu_mask_2);
			kfree(freq_table[A7_CLUSTER]);
			kfree(freq_table[A15_CLUSTER]);
			free_cpumask_var(cpu_mask_1);
			free_cpumask_var(cpu_mask_2);
			clk_put(cluster_clk[0]);
			clk_put(cluster_clk[1]);
			return -EINVAL;
		};
		sunxi_cpufreq_attr[1] = &cpufreq_freq_attr_scaling_boost_freqs;
	}
	/* set the target cluster of cpu */
	if (cur_cluster < MAX_CLUSTERS) {
		/* set cpu masks */
#ifdef CONFIG_SCHED_SMP_DCMP
		cpumask_copy(policy->cpus, cpu_possible_mask);
#else
		cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));
#endif
		/* set core sibling */
		for_each_cpu(cpu, topology_core_cpumask(policy->cpu)) {
			per_cpu(physical_cluster, cpu) = cur_cluster;
		}
	} else {
		/* Assumption: during init, we are always running on A7 */
		per_cpu(physical_cluster, policy->cpu) = A7_CLUSTER;
	}
	/* initialize current cpu freq */
	policy->cur = sunxi_clk_get_cpu_rate(policy->cpu);
	per_cpu(cpu_last_req_freq, policy->cpu) = policy->cur;

	if (unlikely(sunxi_dvfs_debug))
		CPUFREQ_DBG("cpu:%d, cluster:%d, init freq:%d\n", policy->cpu, cur_cluster, policy->cur);


	/* set the transition latency value */
	policy->cpuinfo.transition_latency = SUNXI_FREQTRANS_LATENCY;

	return 0;
}

static int sunxi_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{

	dev_pm_opp_of_cpumask_remove_table(cpu_mask_1);
	dev_pm_opp_of_cpumask_remove_table(cpu_mask_2);
	kfree(freq_table[A7_CLUSTER]);
	kfree(freq_table[A15_CLUSTER]);
	free_cpumask_var(cpu_mask_1);
	free_cpumask_var(cpu_mask_2);
	clk_put(cluster_clk[0]);
	clk_put(cluster_clk[1]);

	return 0;
}

static struct cpufreq_driver sunxi_cpufreq_driver = {
	.name			= "sunxi-iks",
	.flags			= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify			= sunxi_cpufreq_verify_policy,
	.target_index		= sunxi_cpufreq_set_target_index,
	.get			= sunxi_clk_get_cpu_rate,
	.init			= sunxi_cpufreq_cpu_init,
	.exit			= sunxi_cpufreq_cpu_exit,
	.attr			= sunxi_cpufreq_attr,
};

static int sunxi_cpufreq_switcher_notifier(struct notifier_block *nfb,
						unsigned long action, void *_arg)
{
	pr_info("%s: action:%lu\n", __func__, action);

	switch (action) {
	case BL_NOTIFY_PRE_ENABLE:
	case BL_NOTIFY_PRE_DISABLE:
		cpufreq_unregister_driver(&sunxi_cpufreq_driver);
		break;

	case BL_NOTIFY_POST_ENABLE:
		set_switching_enabled(true);
		cpufreq_register_driver(&sunxi_cpufreq_driver);
		break;

	case BL_NOTIFY_POST_DISABLE:
		set_switching_enabled(false);
		cpufreq_register_driver(&sunxi_cpufreq_driver);
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sunxi_switcher_notifier = {
	.notifier_call = sunxi_cpufreq_switcher_notifier,
};

static int  __init sunxi_cpufreq_init(void)
{
	int ret, i;

	ret = bL_switcher_get_enabled();
	set_switching_enabled(ret);

	cluster_pll[A7_CLUSTER] = ARISC_DVFS_PLL1;
	cluster_pll[A15_CLUSTER] = ARISC_DVFS_PLL2;

	for (i = 0; i < MAX_CLUSTERS; i++)
		mutex_init(&cluster_lock[i]);

	ret = cpufreq_register_driver(&sunxi_cpufreq_driver);
	if (ret) {
		CPUFREQ_ERR("sunxi register cpufreq driver fail, err: %d\n", ret);
	} else {
		CPUFREQ_DBG("sunxi register cpufreq driver succeed\n");
		ret = bL_switcher_register_notifier(&sunxi_switcher_notifier);
		if (ret) {
			CPUFREQ_ERR("sunxi register bL notifier fail, err: %d\n", ret);
			cpufreq_unregister_driver(&sunxi_cpufreq_driver);
		} else {
			CPUFREQ_DBG("sunxi register bL notifier succeed\n");
		}
	}

	bL_switcher_put_enabled();

	pr_debug("%s: done!\n", __func__);
	return ret;
}
module_init(sunxi_cpufreq_init);

static void __exit sunxi_cpufreq_exit(void)
{
	int i;

	for (i = 0; i < MAX_CLUSTERS; i++)
		mutex_destroy(&cluster_lock[i]);

	bL_switcher_get_enabled();
	bL_switcher_unregister_notifier(&sunxi_switcher_notifier);
	cpufreq_unregister_driver(&sunxi_cpufreq_driver);
	bL_switcher_put_enabled();
	CPUFREQ_DBG("%s: done!\n", __func__);
}
module_exit(sunxi_cpufreq_exit);

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_cpufreq_root;

static int get_c0_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%llu\n", c0_get_time_usecs);
	return 0;
}

static int get_c0_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_c0_time_show, inode->i_private);
}

static const struct file_operations get_c0_time_fops = {
	.open = get_c0_time_open,
	.read = seq_read,
};

static int set_c0_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%llu\n", c0_set_time_usecs);
	return 0;
}

static int set_c0_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_c0_time_show, inode->i_private);
}

static const struct file_operations set_c0_time_fops = {
	.open = set_c0_time_open,
	.read = seq_read,
};

static int get_c1_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%llu\n", c1_get_time_usecs);
	return 0;
}

static int get_c1_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, get_c1_time_show, inode->i_private);
}

static const struct file_operations get_c1_time_fops = {
	.open = get_c1_time_open,
	.read = seq_read,
};

static int set_c1_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%llu\n", c1_set_time_usecs);
	return 0;
}

static int set_c1_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_c1_time_show, inode->i_private);
}

static const struct file_operations set_c1_time_fops = {
	.open = set_c1_time_open,
	.read = seq_read,
};

static int __init debug_init(void)
{
	int err = 0;

	debugfs_cpufreq_root = debugfs_create_dir("cpufreq", 0);
	if (!debugfs_cpufreq_root)
		return -ENOMEM;

	if (!debugfs_create_file("c0_get_time", 0444, debugfs_cpufreq_root, NULL, &get_c0_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	if (!debugfs_create_file("c0_set_time", 0444, debugfs_cpufreq_root, NULL, &set_c0_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	if (!debugfs_create_file("c1_get_time", 0444, debugfs_cpufreq_root, NULL, &get_c1_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	if (!debugfs_create_file("c1_set_time", 0444, debugfs_cpufreq_root, NULL, &set_c1_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	return 0;

out:
	debugfs_remove_recursive(debugfs_cpufreq_root);
	return err;
}

static void __exit debug_exit(void)
{
	debugfs_remove_recursive(debugfs_cpufreq_root);
}

late_initcall(debug_init);
module_exit(debug_exit);
#endif /* CONFIG_DEBUG_FS */

MODULE_AUTHOR("sunny <sunny@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi iks cpufreq driver");
MODULE_LICENSE("GPL");
