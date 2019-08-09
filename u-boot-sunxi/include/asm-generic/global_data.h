/*
 * Copyright (c) 2012 The Chromium OS Authors.
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_GENERIC_GBL_DATA_H
#define __ASM_GENERIC_GBL_DATA_H
/*
 * The following data structure is placed in some memory which is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set GENERATED_GBL_DATA_SIZE > sizeof(gd_t)
 *
 * Each architecture has its own private fields. For now all are private
 */

#ifndef __ASSEMBLY__
#include <linux/list.h>

typedef struct global_data {
	bd_t *bd;
	unsigned long flags;
	unsigned int baudrate;
	unsigned long cpu_clk;	/* CPU clock in Hz!		*/
	unsigned long bus_clk;
	/* We cannot bracket this with CONFIG_PCI due to mpc5xxx */
	unsigned long pci_clk;
	unsigned long mem_clk;
#if defined(CONFIG_LCD) || defined(CONFIG_VIDEO) || defined(CONFIG_READ_LOGO_FOR_KERNEL)
	unsigned long fb_base;	/* Base address of framebuffer mem */
#endif
#if defined(CONFIG_POST) || defined(CONFIG_LOGBUFFER)
	unsigned long post_log_word;  /* Record POST activities */
	unsigned long post_log_res; /* success of POST test */
	unsigned long post_init_f_time;  /* When post_init_f started */
#endif
#ifdef CONFIG_BOARD_TYPES
	unsigned long board_type;
#endif
	unsigned long have_console;	/* serial_init() was called */
#ifdef CONFIG_PRE_CONSOLE_BUFFER
	unsigned long precon_buf_idx;	/* Pre-Console buffer index */
#endif
#ifdef CONFIG_MODEM_SUPPORT
	unsigned long do_mdm_init;
	unsigned long be_quiet;
#endif
	unsigned long env_addr;	/* Address  of Environment struct */
	unsigned long env_valid;	/* Checksum of Environment valid? */

	unsigned long ram_top;	/* Top address of RAM used by U-Boot */

	unsigned long relocaddr;	/* Start address of U-Boot in RAM */
	phys_size_t ram_size;	/* RAM size */
	unsigned long mon_len;	/* monitor len */
	unsigned long irq_sp;		/* irq stack pointer */
	unsigned long start_addr_sp;	/* start_addr_stackpointer */
	unsigned long secondary_cpu_svc_sp[4];		/* secondary cpu svc stack pointer */
	unsigned long secondary_cpu_irq_sp[4];		/* secondary cpu irq stack pointer */
	unsigned long reloc_off;
	struct global_data *new_gd;	/* relocated global data */

#ifdef CONFIG_DM
	struct udevice	*dm_root;/* Root instance for Driver Model */
	struct list_head uclass_root;	/* Head of core tree */
#endif

	const void *fdt_blob;	/* Our device tree, NULL if none */
	void *fdtfast_blob;
	void *new_fdt;		/* Relocated FDT */
	void *new_fdtfast;
	unsigned long fdt_size;	/* Space reserved for relocated FDT */
	unsigned long fdtfast_size;/* Space reserved for relocated FDT fast table */
	void **jt;		/* jump table */
	char env_buf[32];	/* buffer for getenv() before reloc. */
#ifdef CONFIG_TRACE
	void		*trace_buff;	/* The trace buffer */
#endif
#if defined(CONFIG_SYS_I2C)
	int		cur_i2c_bus;	/* current used i2c bus */
#endif
#ifdef CONFIG_SYS_I2C_MXC
	void *srdata[10];
#endif
	unsigned long timebase_h;
	unsigned long timebase_l;
#ifdef CONFIG_SUNXI
	long           securemode;
	void          *parameter_mod_buf;
	long           boot_card_num;
	int            keybox_installed;
	ulong          lockflag;
	ulong          chargemode;

	ulong          parameter_reloc_buf;
	ulong          parameter_reloc_size;

	ulong          malloc_noncache_start;

	long           key_pressd_value;
	long           axp_power_soft_id;
	long           power_step_level;
	long           pmu_suspend_chgcur;
	long           pmu_runtime_chgcur;
	long           limit_vol;
	long           limit_cur;
	long           limit_pcvol;
	long           limit_pccur;
	ulong          force_download_uboot;
	ulong          vbus_status;//0: unknow 1:exist 2:not exist
        ulong          debug_mode;
	long           force_shell;
	long           user_debug_mode;
	ulong          layer_para;
	ulong          layer_hd;
	ulong          bootfile_mode;
	int            pmu_saved_status;
	int 		   need_shutdown;
	int            logo_status_multiboot;
	int            ir_detect_status;

#endif
	struct arch_global_data arch;	/* architecture-specific data */
} gd_t;
#endif

/*
 * Global Data Flags
 */
#define GD_FLG_RELOC		0x00001	/* Code was relocated to RAM	   */
#define GD_FLG_DEVINIT		0x00002	/* Devices have been initialized   */
#define GD_FLG_SILENT		0x00004	/* Silent mode			   */
#define GD_FLG_POSTFAIL		0x00008	/* Critical POST test failed	   */
#define GD_FLG_POSTSTOP		0x00010	/* POST seqeunce aborted	   */
#define GD_FLG_LOGINIT		0x00020	/* Log Buffer has been initialized */
#define GD_FLG_DISABLE_CONSOLE	0x00040	/* Disable console (in & out)	   */
#define GD_FLG_ENV_READY	0x00080	/* Env. imported into hash table   */

#endif /* __ASM_GENERIC_GBL_DATA_H */
