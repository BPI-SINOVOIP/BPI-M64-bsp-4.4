/*
 * drivers/soc/sunxi/pm_legacy/standby/super/resume/resume1_c_part.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* these code will be removed to sram.
 * function: open the mmu, and jump to dram, for continuing resume
 */
#include "./../super_i.h"

struct aw_mem_para mem_para_info;

extern char *__bss_start;
extern char *__bss_end;
#if (defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1)) && defined(CONFIG_SUNXI_TRUSTZONE)
extern void switch2normal(int entry_addr, unsigned int vector,
			  unsigned int mon_sp, unsigned int svc_sp);
#endif
static __u32 __tzasc_calc_2_power(__u32 data);
int (*func_entry) (void);
static __s32 dcdc2, dcdc3;
static __u32 sp_backup;
static __u32 *tmpPtr = (__u32 *) &__bss_start;
static __u32 status;

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#define MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#undef MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#if defined(ENTER_SUPER_STANDBY) || defined(ENTER_SUPER_STANDBY_WITH_NOMMU) || defined(WATCH_DOG_RESET)
#undef MMU_OPENED
#define POWER_OFF
#define FLUSH_TLB
#define SET_COPRO_REG
/* #define FLUSH_ICACHE */
#define INVALIDATE_DCACHE
#endif

#ifndef CONFIG_FPGA_V4_PLATFORM
#define IS_WFI_MODE(cpu)	(*(unsigned int *)((((0x01f01c00)) + (0x48 + (cpu)*0x40))) & (1<<2))
#else
#define IS_WFI_MODE(cpu)	(1)
#endif

int resume1_c_part(void)
{
#ifdef SET_COPRO_REG
	save_mem_status_nommu(RESUME1_START | 0x07);
	set_copro_default();
#endif

#ifdef MMU_OPENED
	save_mem_status(RESUME1_START | 0x08);

	/* move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state */
	mem_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1),
		   sizeof(mem_para_info));
#else
	save_mem_status_nommu(RESUME1_START | 0x08);

	/*restore freq from 408M to orignal freq. */
	mem_clk_setdiv(&mem_para_info.clk_div);
	mem_clk_set_pll_factor(&mem_para_info.pll_factor);
	change_runtime_env();
	delay_ms(mem_para_info.suspend_delay_ms);

	if (unlikely(mem_para_info.debug_mask & PM_STANDBY_PRINT_RESUME))
		serial_puts_nommu("resume1: 1. before restore mmu. \n");

	/*restore mmu configuration */
	save_mem_status_nommu(RESUME1_START | 0x09);
	/* save_mem_status(RESUME1_START |0x09); */

	/* after restore mmu, u need to re-init reg base address. */
	restore_mmu_state(&(mem_para_info.saved_mmu_state));
	save_mem_status(RESUME1_START | 0xA);

#endif

/* before jump to late_resume */
#ifdef FLUSH_TLB
	save_mem_status(RESUME1_START | 0xb);
	mem_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	save_mem_status(RESUME1_START | 0xc);
	flush_icache();
#endif

	mem_clk_init(1);
	if (unlikely(mem_para_info.debug_mask & PM_STANDBY_PRINT_RESUME)) {
		serial_puts("resume1: 3. after restore mmu, before jump.\n");
		printk("get_cur_cluster_id = %d.\n", get_cur_cluster_id());
		printk("mem_para_info.debug_mask = %x.\n",
		       mem_para_info.debug_mask);
		printk("delay mem_para_info.suspend_delay_ms = %d.\n",
		       mem_para_info.suspend_delay_ms);
		if (likely(mem_para_info.debug_mask & PM_STANDBY_PRINT_PORT)) {
			printk("addr PF: 0x%x = 0x%x.\n",
			       (IO_ADDRESS(AW_JTAG_PF_GPIO_PA)),
			       *(__u32
				 *)(IO_ADDRESS(AW_JTAG_PF_GPIO_PA)));
		} else {
			printk("addr PH: 0x%x = 0x%x.\n",
			       (IO_ADDRESS(AW_JTAG_PH_GPIO_PA)),
			       *(__u32
				 *)(IO_ADDRESS(AW_JTAG_PH_GPIO_PA)));
		}
	}

	save_mem_status(RESUME1_START | 0xe);
	jump_to_resume((void *)mem_para_info.resume_pointer,
		       mem_para_info.saved_runtime_context_svc);

	return 0;
}

/*****************************************************************************
* interface : set_pll
*	prototype		��void set_pll( void )
*	function		: adjust CPU frequence,
*					from 24M hosc to pll1 384M
*	input para	: void
*	return value	: void
*	note:
*****************************************************************************/
void set_pll(void)
{
	__ccmu_reg_list_t *CmuReg;
	__u32 cpu_id = 0;
	__u32 cpu1_reset = 0;
	__u32 cpu2_reset = 0;
	__u32 cpu3_reset = 0;
	__u32 pwr_reg = 0;
	__u32 mva_addr = 0x00000000;
	__u32 index = 0;

#if (defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)) && defined(CONFIG_SUNXI_TRUSTZONE)
	__u32 i = 0;
	__u32 reserved_dram_size = 16;
	__u32 dram_size = 0;	/* unit is Mbytes. */
	__u32 permission = 0;
	__u32 region_size = 0;
#endif
	/* Read CPU ID register */
	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (cpu_id));
	cpu_id &= 0x3;
	if (cpu_id == 0) {
		/* clear bss segment */
		do {
			*tmpPtr++ = 0;
		} while (tmpPtr <= (__u32 *) &__bss_end);

		/*when enter this func, state is as follow:
		 *      1. mmu is disable.
		 *      2. clk is 24M hosc (?)
		 *
		 */
		CmuReg = (__ccmu_reg_list_t *) mem_clk_init(0);
		/*init debug state */
		mem_status_init_nommu();

		save_mem_status_nommu(RESUME0_START | 0x0);
#ifdef CONFIG_ARCH_SUN8I
		/* switch to 24M */
		*(__u32 *)(&CmuReg->SysClkDiv) =
		    CPU_CLK_REST_DEFAULT_VAL;

		/* get mem para info */
		/* move other storage to sram:
		 * saved_resume_pointer(virtual addr), saved_mmu_state
		 */
		mem_memcpy((void *)&mem_para_info,
			   (void *)(DRAM_MEM_PARA_INFO_PA),
			   sizeof(mem_para_info));

		save_mem_status_nommu(RESUME0_START | 0x1);

		/* config jtag gpio */
		/* need to config gpio clk? apb1 clk gating? */
		if (unlikely(mem_para_info.debug_mask &
					PM_STANDBY_ENABLE_JTAG)) {
			/* notice: this will affect uart gpio config.
			 * for customer's reason, default is pf port.
			 */
			if (likely
			    (mem_para_info.
			     debug_mask & PM_STANDBY_PRINT_PORT)) {
				*(__u32 *)(AW_JTAG_PF_GPIO_PA) =
				    AW_JTAG_PF_CONFIG_VAL;
			} else {
				*(__u32 *)(AW_JTAG_PH_GPIO_PA) =
				    AW_JTAG_PH_CONFIG_VAL;
			}
		}
		/* config pll para */
		mem_clk_set_misc(&mem_para_info.clk_misc);
		save_mem_status_nommu(RESUME0_START | 0x3);
		/* enable pll1 and setting PLL1 to 408M */
#ifdef CONFIG_ARCH_SUN8IW1P1
		/* N = 17, P=1 -> pll1 = 24*N/P = 24*17 = 408M */
		/* N = 16, K=M=2 -> pll1 = 17*24 = 408M */
		*(__u32 *)(&CmuReg->Pll1Ctl) = (0x00001011) | (0x80000000);
#elif defined(CONFIG_ARCH_SUN8IW3P1)
		/* N = 16, K=M=1 -> pll1 = 17*24 = 408M */
		*(__u32 *)(&CmuReg->Pll1Ctl) = (0x00001000) | (0x80000000);
#elif defined(CONFIG_ARCH_SUN8IW5P1)
		/* N = 16, K=M=1 -> pll1 = 17*24 = 408M */
		*(__u32 *)(&CmuReg->Pll1Ctl) = (0x00001000) | (0x80000000);
#elif defined(CONFIG_ARCH_SUN8IW6P1)
		/* N = 17, P=1 -> pll1 = 17*24 = 408M */
		*(__u32 *)(&CmuReg->Pll1Ctl) = (0x02001100) | (0x80000000);
#elif defined(CONFIG_ARCH_SUN8IW9P1)
		/* N = 17, P=M=1 -> pll1 = 17*24 = 408M */
		*(__u32 *)(&CmuReg->Pll1Ctl) = (0x00001100) | (0x80000000);
#endif

		save_mem_status_nommu(RESUME0_START | 0x4);
#if 0				/* for a31? */
		/* setting pll6 to 600M */
		/* enable pll6 */
		*(__u32 *)(&CmuReg->Pll6Ctl) = 0x80041811;
#endif
#elif defined CONFIG_ARCH_SUN9IW1P1
		/* Make sure the clk src is 24M. */
		/* notice: when vdd_sys is not reset,
		 * this ops may be not stable.
		 */
		save_mem_status_nommu(RESUME0_START | 0x02);
		*(__u32 *)(&CmuReg->Cpu_Clk_Src) =
		    CPU_CLK_REST_DEFAULT_VAL;

		/* get mem para info */
		/* move other storage to sram:
		 * saved_resume_pointer(virtual addr), saved_mmu_state
		 */
		save_mem_status_nommu(RESUME0_START | 0x03);
		mem_memcpy((void *)&mem_para_info,
			   (void *)(DRAM_MEM_PARA_INFO_PA),
			   sizeof(mem_para_info));

		/* config jtag gpio */
		save_mem_status_nommu(RESUME0_START | 0x04);
		/* config gpio clk; */
		config_gpio_clk(0);

		if (unlikely(mem_para_info.debug_mask &
					PM_STANDBY_ENABLE_JTAG)) {
			/* notice: this will affect uart gpio config.
			 * for customer's reason, default is pf port.
			 */
			if (likely
			    (mem_para_info.
			     debug_mask & PM_STANDBY_PRINT_PORT)) {
				*(__u32 *)(AW_JTAG_PF_GPIO_PA) =
				    AW_JTAG_PF_CONFIG_VAL;
			} else {
				*(__u32 *)(AW_JTAG_PH_GPIO_PA) =
				    AW_JTAG_PH_CONFIG_VAL;
			}
		}
		/* config pll para: bias and tun para. */
		save_mem_status_nommu(RESUME0_START | 0x05);
		mem_clk_set_misc(&mem_para_info.clk_misc);
		/* enable pll1 and setting PLL1 to 408M */
		/* N = 17, P=1 -> pll1 = 24*N/P = 24*17 = 408M */
		/* Notice: the setting need be double
		 * checked before ic is ready. !!!!!!
		 */
		*(__u32 *)(&CmuReg->Pll_C0_Cfg) =
		    (CPU_PLL_REST_DEFAULT_VAL) | (0x80000000);
		*(__u32 *)(&CmuReg->Pll_C1_Cfg) =
		    (CPU_PLL_REST_DEFAULT_VAL) | (0x80000000);
		save_mem_status_nommu(RESUME0_START | 0x06);
#endif

		init_perfcounters(1, 0);	/* need double check.. */
		change_runtime_env();
		delay_ms(10);

		/* change apb2 src to pll6 */
		if (unlikely
		    (mem_para_info.debug_mask & PM_STANDBY_PRINT_RESUME)) {
			/* config uart */
			serial_init_nommu(mem_para_info.
					  debug_mask & PM_STANDBY_PRINT_PORT);
			serial_puts_nommu("resume1: start.\n");
		}
	} else {
		/* execute a TLBIMVAIS operation to addr: 0x0000,0000 */
		asm volatile ("mcr p15, 0, %0, c8, c3, 1" : : "r" (mva_addr));
		asm volatile ("dsb");
		/* printk_nommu("cpu_id = %x.\n", cpu_id); */
		/* set invalidation done flag */
		writel(CPUX_INVALIDATION_DONE_FLAG,
		       CPUX_INVALIDATION_DONE_FLAG_REG(cpu_id));
		/* dsb */
		asm volatile ("dsb");
		asm volatile ("SEV");
	}

#ifdef CONFIG_ARCH_SUN8IW1P1
	if (cpu_id == 0) {
		/* step2: clear completion flag. */
		writel(0, CPUX_INVALIDATION_COMPLETION_FLAG_REG);
		/* step3: clear completion done flag for each cpux */
		index = CPUCFG_CPU1;
		while (index < CPUCFG_CPU_NUMBER) {
			writel(0, CPUX_INVALIDATION_DONE_FLAG_REG(index));
			index++;
		}

		/* step4: dsb */
		asm volatile ("dsb");

		/* step5: power up other cpus. */
		index = CPUCFG_CPU1;
		while (index < CPUCFG_CPU_NUMBER) {
			super_enable_aw_cpu(index);
			index++;
		}

		/* step7: check cpux's invalidation done flag. */
		while (1) {
			/* step6 or 8: wfe */
			asm volatile ("wfe");
			if (ALL_CPUX_INVALIDATION_DONE) {
				/* step9: set completion flag. */
				writel(CPUX_INVALIDATION_DONE_FLAG,
				       CPUX_INVALIDATION_COMPLETION_FLAG_REG);

				/* step10: dsb */
				asm volatile ("dsb");
				/* sev */
				asm volatile ("sev");
				break;
			}
		}

		/* step 11: normal power down. */
		while (1) {
			if (ALL_CPUX_IS_WFI_MODE) {
			/* step9: set up cpu1+ power-off signal */
			/* printk_nommu("set up cpu1+ power-off signal.\n"); */
				pwr_reg =
				    (*(__u32 *)
				     ((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG));
				pwr_reg |= (0xe);	/* 0b1110 */
				(*(__u32 *)
				 ((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG)) =
		 pwr_reg;
				delay_ms(1);

			/* step10: active the power output clamp */
			/* printk_nommu("active the power output clamp.\n"); */
#ifndef CONFIG_FPGA_V4_PLATFORM
				index = CPUCFG_CPU1;
				while (index < CPUCFG_CPU_NUMBER) {
					(*(__u32 *)
					 ((AW_R_PRCM_BASE) +
					  AW_CPUX_PWR_CLAMP(index))) = 0xff;
					index++;
				}
#endif

				break;
			}
			if (unlikely
			    (mem_para_info.
			     debug_mask & PM_STANDBY_PRINT_RESUME)) {
				printk_nommu("cpu1+ wfi state as follow:\n");
				index = CPUCFG_CPU1;
				while (index < CPUCFG_CPU_NUMBER) {
					printk_nommu("cpu1 wfi = %d.\n",
							IS_WFI_MODE(index));
					index++;
				}
			}

		}
		/* printk_nommu("cpu1 go on wakeup the system...\n"); */

	} else {

		/* just waiting until the completion flag be seted.. */
		while (1) {
			/* step: execute a WFE instruction  */
			asm volatile ("wfe");
			if (CPUX_INVALIDATION_DONE_FLAG ==
			    readl(CPUX_INVALIDATION_COMPLETION_FLAG_REG)) {
				break;
			}
		}

		/* normal power down sequence. */
		while (1) {
			/* let the cpu1+ enter wfi state; */
			/* step3: execute a CLREX instruction */
			asm("clrex" : : : "memory", "cc");

			/* step5: execute an ISB instruction */
			asm volatile ("isb");
			/* step6: execute a DSB instruction  */
			asm volatile ("dsb");

			/* step7: execute a WFI instruction */
			while (1)
			      asm("wfi" : : : "memory", "cc");

		}
	}
#endif

	save_mem_status_nommu(RESUME0_START | 0x6);
	/* switch to PLL1 */
#ifdef CONFIG_ARCH_SUN8I

#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1)
	*(__u32 *)(&CmuReg->SysClkDiv) =
	    (0x1000) | (*(__u32 *)(&CmuReg->SysClkDiv));
#else
	*(__u32 *)(&CmuReg->SysClkDiv) =
	    (0x20000) | ((~0x30000) &
			 (*(__u32 *)(&CmuReg->SysClkDiv)));
#endif

	save_mem_status_nommu(RESUME0_START | 0x7);
#elif defined(CONFIG_ARCH_SUN9IW1P1)
	/* switch to PLL1 */
	if (get_cur_cluster_id())
		*(__u32 *)(&CmuReg->Cpu_Clk_Src) = 0x00000100;
	else
		*(__u32 *)(&CmuReg->Cpu_Clk_Src) = 0x00000001;
#endif
	change_runtime_env();
	delay_us(100);

#if (defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)) && defined(CONFIG_SUNXI_TRUSTZONE)

	/* busy_waiting(); */
	/* config gic to non-secure */
	/* id0-id1023:1024/32 = 0x20; */
	/* 2048? */
	/* how to config sgi interrupt? */
	for (i = 0; i < (0x40); i += 4)
		*(__u32 *)(SUNXI_GIC_DIST_PBASE + 0x80 + i) =
		    0xffffffff;
	/* busy_waiting(); */
	/* switch to normal world */
	/* after restore mmu, u need to re-init reg base address. */
	restore_secure_mmu_state(&(mem_para_info.saved_secure_mmu_state));
	func_entry = resume1_c_part;
	switch2normal((int)func_entry, mem_para_info.monitor_vector,
		      mem_para_info.saved_secure_mmu_state.mon_sp,
		      mem_para_info.saved_secure_mmu_state.svc_sp);
#endif
}

static __u32 __tzasc_calc_2_power(__u32 data)
{
	__u32 ret = 0;

	do {
		data >>= 1;
		ret++;
	} while (data);
	return ret - 1;
}
