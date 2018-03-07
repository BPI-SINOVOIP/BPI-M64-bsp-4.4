/*
 * linux/arch/arm/mach-vexpress/mcpm_platsmp.c
 *
 * Created by:  Nicolas Pitre, November 2012
 * Copyright:   (C) 2012-2013  Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Code to handle secondary CPU bringup and hotplug for the cluster power API.
 */

#include <linux/init.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <asm/mcpm.h>
#include <asm/smp.h>
#include <asm/smp_plat.h>
#include <linux/irqchip/arm-gic.h>

static void cpu_to_pcpu(unsigned int cpu,
			unsigned int *pcpu, unsigned int *pcluster)
{
	unsigned int mpidr;

	mpidr = cpu_logical_map(cpu);
	*pcpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	*pcluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
}

static int mcpm_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned int pcpu, pcluster, ret;
	extern void secondary_startup(void);

	cpu_to_pcpu(cpu, &pcpu, &pcluster);

	pr_debug("%s: logical CPU %d is physical CPU %d cluster %d\n",
		 __func__, cpu, pcpu, pcluster);

	mcpm_set_entry_vector(pcpu, pcluster, NULL);
	ret = mcpm_cpu_power_up(pcpu, pcluster);
	if (ret)
		return ret;
	mcpm_set_entry_vector(pcpu, pcluster, secondary_startup);
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	dsb_sev();
	return 0;
}

extern int gic_secondary_init(struct notifier_block *nfb, unsigned long action, void *hcpu);

static void mcpm_secondary_init(unsigned int cpu)
{
	/*0x000A ======>CPU_STARTING*/
	/*mcpm_cpu_powered_up();*/
	gic_secondary_init(NULL, 0x000A, NULL);
}

#ifdef CONFIG_HOTPLUG_CPU

static int mcpm_cpu_kill(unsigned int cpu)
{
	unsigned int pcpu, pcluster;

	cpu_to_pcpu(cpu, &pcpu, &pcluster);

	return !mcpm_wait_for_cpu_powerdown(pcpu, pcluster);
}

static bool mcpm_cpu_can_disable(unsigned int cpu)
{
	/* We assume all CPUs may be shut down. */
	return true;
}

static int  mcpm_cpu_disable(unsigned int cpu)
{
	unsigned int pcpu, pcluster;

	cpu_to_pcpu(cpu, &pcpu, &pcluster);
	return mcpm_cpu_clear_status(cpu, pcpu, pcluster);
}

static void mcpm_cpu_die(unsigned int cpu)
{
	unsigned int mpidr, pcpu, pcluster;
	mpidr = read_cpuid_mpidr();
	pcpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	pcluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	mcpm_set_entry_vector(pcpu, pcluster, NULL);
	mcpm_cpu_power_down();
}

extern int sun8i_mcpm_cpu_map_init(void);
extern void gic_raise_softirq(const struct cpumask *mask, unsigned int irq);

static void __ref mcpm_smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores = MAX_NR_CLUSTERS * MAX_CPUS_PER_CLUSTER;

	/*
	 * sanity check, the cr_cpu_ids is configured form CONFIG_NR_CPUS
	 */
	if (ncores > nr_cpu_ids) {
		printk("SMP: %u cores greater than maximum (%u), clipping\n",
				ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}
	printk("[%s] ncores=%d\n", __func__, ncores);

	for (i = 0; i < ncores; i++) {
	    set_cpu_possible(i, true);
	}

	sun8i_mcpm_cpu_map_init();

#if defined(CONFIG_ARM_SUNXI_CPUIDLE)
	set_smp_cross_call(sunxi_raise_softirq);
#else
	set_smp_cross_call(gic_raise_softirq);
#endif
}

#endif

static struct smp_operations __initdata mcpm_smp_ops = {
	.smp_init_cpus		= mcpm_smp_init_cpus,
	.smp_boot_secondary	= mcpm_boot_secondary,
	.smp_secondary_init	= mcpm_secondary_init,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_kill		= mcpm_cpu_kill,
	.cpu_can_disable	= mcpm_cpu_can_disable,
	.cpu_disable		= mcpm_cpu_disable,
	.cpu_die		= mcpm_cpu_die,
#endif
};

void __init mcpm_smp_set_ops(void)
{
	smp_set_ops(&mcpm_smp_ops);
}
