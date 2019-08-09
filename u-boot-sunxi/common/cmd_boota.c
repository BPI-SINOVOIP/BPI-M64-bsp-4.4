/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * Boot an image which is generated by android mkbootimg tool
 *
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>
//#include <sunxi_board.h>
#include <android_image.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <fdt_support.h>
#include <image.h>
#include <sunxi_board.h>
#include <power/sunxi/pmu.h>
#include <smc.h>
#include <cputask.h>
#include <sys_config_old.h>
#include <sys_partition.h>
#include <sunxi_mbr.h>
#include <ufdt_support.h>
DECLARE_GLOBAL_DATA_PTR;


enum {
	ARCH_ARM = 0,
	ARCH_ARM64,
};

typedef void (*Kernel_Entry)(int zero,int machine_id,void *fdt_addr);
__weak void clean_timestamp_counter(void)
{
    pr_msg("weak clean_timestamp_counter\n");
}

static void announce_and_cleanup(int fake)
{
	pr_msg("prepare for kernel\n");
#ifndef CONFIG_ARCH_SUN3IW1P1
        sunxi_board_prepare_kernel();
#endif
#ifdef CONFIG_BOOTSTAGE_FDT
	bootstage_fdt_add_report();
#endif
#ifdef CONFIG_BOOTSTAGE_REPORT
	bootstage_report();
#endif

#ifdef CONFIG_USB_DEVICE
	udc_disconnect();
#endif
	cleanup_before_linux();
	pr_force("\nStarting kernel ...%s\n\n", fake ?
		"(fake run for tracing)" : "");
}


uint get_arch_type(struct andr_img_hdr *hdr)
{
	if ((hdr->kernel_addr & 0xfffff) == 0x80000)
		return ARCH_ARM64;
	else
		return ARCH_ARM;
}

#ifdef CONFIG_SUNXI_SECURE_SYSTEM
static int android_image_get_signature(const struct andr_img_hdr *hdr,
									ulong *sign_data, ulong *sign_len)
{
	struct boot_img_hdr_ex  *hdr_ex;
	ulong addr=0;

	hdr_ex = (struct boot_img_hdr_ex *)hdr;
	if(strncmp((void *)(hdr_ex->cert_magic),AW_CERT_MAGIC,strlen(AW_CERT_MAGIC))){
		printf("No cert image embeded, image %s\n",hdr_ex->cert_magic);
		return 0;
	}

	addr = (unsigned long)hdr;
	addr += hdr->page_size;
	addr += ALIGN(hdr->kernel_size, hdr->page_size);
	if (hdr->ramdisk_size)
		addr += ALIGN(hdr->ramdisk_size, hdr->page_size);
	if (hdr->second_size)
		addr += ALIGN(hdr->second_size, hdr->page_size);
	if (hdr->recovery_dtbo_size)
		addr += ALIGN(hdr->recovery_dtbo_size, hdr->page_size);

	*sign_data = (ulong)addr;
	*sign_len = hdr_ex->cert_size;
	memset(hdr_ex->cert_magic,0,ANDR_BOOT_MAGIC_SIZE+sizeof(unsigned));
	return 1;
}
#endif

int do_boota_linux (void *kernel_addr, void *dtb_base, uint arch_type)
{
	int fake = 0;
	Kernel_Entry kernel_entry = NULL;


	kernel_entry = (Kernel_Entry)(ulong)(kernel_addr);

	debug("## Transferring control to Linux (at address %lx)...\n",
		(ulong) kernel_entry);

#ifdef CONFIG_SUNXI_FINS_FUNC
	extern int __attribute__((__no_instrument_function__))
		ff_move_reloc_data(void);
	ff_move_reloc_data();
#endif

#ifdef CONFIG_SUNXI_MULITCORE_BOOT
	sunxi_secondary_cpu_poweroff();
	sunxi_fdt_reflush_all();
#endif
#ifdef CONFIG_OF_LIBUFDT
	void *dtbo_base = NULL;
	dtbo_base = sunxi_support_ufdt((void *)gd->fdt_blob, fdt_totalsize(gd->fdt_blob));
	if (dtbo_base == NULL) {
		printf("sunxi dto merge fail\n");
		return -1;
	}
	memcpy((void *)dtb_base, dtbo_base, fdt_totalsize(dtbo_base));
#else

	debug("moving platform.dtb from %lx to: %lx, size 0x%lx\n",
		(ulong)dtb_base,
		(ulong)(gd->fdt_blob),gd->fdt_size);
	//fdt_blob  save the addree of  working_fdt
	memcpy((void*)dtb_base, gd->fdt_blob,gd->fdt_size);
#endif
	if(fdt_check_header(dtb_base) !=0 )
	{
		printf("fdt header check error befor boot\n");
		return -1;
	}


	announce_and_cleanup(fake);
	if (sunxi_probe_secure_monitor()) {
		arm_svc_run_os((ulong)kernel_addr, (ulong)dtb_base,  arch_type);
	} else {
		clean_timestamp_counter();
		kernel_entry(0,0xffffffff,dtb_base);
	}

	return 0;
}

void * memcpy2(void * dest,const void * src,__kernel_size_t n)
{
	if (src == dest)
		return dest;
	if (src < dest) {
		if (src + n > dest) {
			memcpy((void*) (dest + (dest - src)), dest, src + n - dest);
			n = dest - src;
		}
		memcpy(dest, src, n);
	} else {
		memcpy(dest, src, n);
	}
	return dest;
}

void update_bootargs(void)
{
	int dram_clk = 0;
	char *str;
	char cmdline[1024] = {0};
	char tmpbuf[128] = {0};
	char *verifiedbootstate_info = getenv("verifiedbootstate");
	str = getenv("bootargs");
	strcpy(cmdline,str);

	if ((!strcmp(getenv("bootcmd"), "run setargs_mmc boot_normal")) \
			|| !strcmp(getenv("bootcmd"), "run setargs_nand boot_normal")) {
		if (gd->chargemode == 0) {
			printf ("in boot normal mode,pass normal para to cmdline\n");
			strcat(cmdline, " androidboot.mode=normal");
		} else {
			printf("in charger mode, pass charger para to cmdline\n");
			strcat(cmdline, " androidboot.mode=charger");
		}
	}
#ifdef CONFIG_SUNXI_SERIAL
	//serial info
	str = getenv("snum");
	sprintf(tmpbuf," androidboot.serialno=%s",str);
	strcat(cmdline,tmpbuf);
#endif
	//harware info
	sprintf(tmpbuf," androidboot.hardware=%s",board_hardware_info());
	strcat(cmdline,tmpbuf);
	/*boot type*/
	sprintf(tmpbuf," boot_type=%d",get_boot_storage_type_ext());
	strcat(cmdline,tmpbuf);

	/*dram_clk*/
	script_parser_fetch("dram_para", "dram_clk", (int *)&dram_clk, 1);
#ifdef CONFIG_ARCH_SUN8IW15P1
	sprintf(tmpbuf, " dram_clk=%d", dram_clk);
	strcat(cmdline, tmpbuf);
#endif
	/* gpt support */
	if(PART_TYPE_GPT == sunxi_partition_get_type())
	{
		sprintf(tmpbuf," gpt=1");
		strcat(cmdline,tmpbuf);
	}


	if (gd->securemode) {
		/* verified boot state info */
		sprintf(tmpbuf, " androidboot.verifiedbootstate=%s",
				verifiedbootstate_info);
		strcat(cmdline, tmpbuf);
	}

	if (gd->keybox_installed) {
		sprintf(tmpbuf, " androidboot.trustchain=%s", "true");
	} else {
		sprintf(tmpbuf, " androidboot.trustchain=%s", "false");
	}
	strcat(cmdline, tmpbuf);

	setenv("bootargs", cmdline);
	printf("android.hardware = %s\n", board_hardware_info());
}

extern int switch_boot_partition(void);
extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

int do_boota (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong os_load_addr;
	ulong os_data = 0,os_len = 0;
	ulong rd_data,rd_len;
	uint arch_type = 0;
	void *kernel_addr = NULL;
	struct  andr_img_hdr *fb_hdr = NULL;
	void *dtb_base = (void*)CONFIG_SUNXI_FDT_ADDR;

	if (argc < 2)
		return cmd_usage(cmdtp);

	os_load_addr = simple_strtoul(argv[1], NULL, 16);
	fb_hdr = (struct andr_img_hdr *)os_load_addr;

	if(android_image_check_header(fb_hdr))
	{
		puts("boota: bad boot image magic, maybe not a boot.img?\n");
		switch_boot_partition();
		do_reset(NULL, 0, 0,NULL);
		return -1;
	}
	android_image_get_kernel(fb_hdr,0,&os_data,&os_len);
	android_image_get_ramdisk(fb_hdr,&rd_data,&rd_len);
	arch_type = get_arch_type(fb_hdr);
	kernel_addr = (void *)fb_hdr->kernel_addr;
#ifdef CONFIG_SUNXI_SECURE_SYSTEM
	uint count = 0;
	uint wait_for_power_key = 1;
	if(gd->securemode)
	{
		/* ORANGE, indicating a device may be freely modified.
		 * Device integrity is left to the user to verify out-of-band.
		 * The bootloader displays a warning to the user before allowing
		 * the boot process to continue.*/
		if (gd->lockflag == SUNXI_UNLOCKED) {
			setenv("verifiedbootstate", "orange");
			printf("Your device software can't be checked for corruption.\n");
			printf("Please lock the bootloader.\n");
			sunxi_bmp_display("orange_warning.bmp");
			do {
				if (axp_probe_key()) {
					/* PAUSE BOOT */
					printf("pause boot,shutdown machine\n");
					sunxi_board_shutdown();
				}
				__msdelay(1000);
				count++;
			} while	(count < 5);
			printf("orange state:start to display bootlogo\n");
			sunxi_bmp_display("bootlogo.bmp");
		} else {
			ulong total_len = 0;
			ulong sign_data , sign_len;
			int ret;

			total_len += fb_hdr->page_size;
			total_len += ALIGN(fb_hdr->kernel_size, fb_hdr->page_size);
			if (fb_hdr->second_size)
				total_len += ALIGN(fb_hdr->second_size, fb_hdr->page_size);
			if (fb_hdr->ramdisk_size)
				total_len += ALIGN(fb_hdr->ramdisk_size, fb_hdr->page_size);
			if (fb_hdr->recovery_dtbo_size)
				total_len += ALIGN(fb_hdr->recovery_dtbo_size, fb_hdr->page_size);

			printf("total_len=%ld\n", total_len);

			if (android_image_get_signature(fb_hdr, &sign_data, &sign_len))
				ret = sunxi_verify_embed_signature((void *)os_load_addr,
					(unsigned int)total_len,
					argv[2], (void *)sign_data, sign_len);
			else
				ret = sunxi_verify_signature((void *)os_load_addr,
					(unsigned int)total_len, argv[2]);

			/* YELLOW, indicating the boot partition has been verified using the
			 * embedded certificate, and the signature is valid. The bootloader
			 * displays a warning and the fingerprint of the public key before
			 * allowing the boot process to continue.*/
			if (!strcmp(getenv("verifiedbootstate"), "yellow")) {
				printf("Your device has loaded a different operating system.\n");
				printf("stop booting until the power key is pressed\n");
				sunxi_bmp_display("yellow_pause_warning.bmp");
				do {
					if (axp_probe_key()) {
						/* PAUSE BOOT */
						printf("pause boot,waiting for press power key again\n");
						sunxi_bmp_display("yellow_continue_warning.bmp");
						do {
							if (axp_probe_key()) {
								/* CONTINUE BOOT */
								wait_for_power_key = 0;
								/* timeout,continue to bootup */
								count = 5;
							}
						} while (wait_for_power_key);
					}
					__msdelay(1000);
					count++;
				} while	(count < 5);
				printf("yellow state:start to display bootlogo\n");
				sunxi_bmp_display("bootlogo.bmp");
			} else {
				/* GREEN, indicating a full chain of trust extending from the
				 * bootloader to verified partitions, including the bootloader,
				 * boot partition, and all verified partitions.*/
				setenv("verifiedbootstate", "green");
			}

			/* RED, indicating the device has failed verification. The bootloader
			 * displays an error and stops the boot process.*/
			if (ret) {
				printf("boota: verify the %s failed\n", argv[2]);
				setenv("verifiedbootstate", "red");
				printf("Your device is corrupt.It can't be truseted and will not boot.\n");
				sunxi_bmp_display("red_warning.bmp");
				__msdelay(30000);
				sunxi_board_shutdown();
			}
		}
	}
#endif
	memcpy2((void*) (long)fb_hdr->kernel_addr, (const void *)os_data, os_len);
	if (fb_hdr->ramdisk_size) {
		memcpy2((void *)(long)fb_hdr->ramdisk_addr, (const void *)rd_data, rd_len);
	}

#ifdef SYS_CONFIG_MEMBASE
	debug("moving sysconfig.bin from %lx to: %lx, size 0x%lx\n",
		get_script_reloc_buf(SOC_SCRIPT),
		(ulong)(SYS_CONFIG_MEMBASE),
		get_script_reloc_size(SOC_SCRIPT));

	memcpy((void*)SYS_CONFIG_MEMBASE, (void*)get_script_reloc_buf(SOC_SCRIPT),get_script_reloc_size(SOC_SCRIPT));
#endif
	update_bootargs();

	//update fdt bootargs from env config
	fdt_chosen(working_fdt);
	if (fb_hdr->ramdisk_size) {
		fdt_initrd(working_fdt, (ulong)fb_hdr->ramdisk_addr, (ulong)(fb_hdr->ramdisk_addr+rd_len));
	}

	pr_msg("ready to boot\n");
#if 1
	do_boota_linux(kernel_addr, dtb_base, arch_type);
	puts("Boot linux failed, control return to monitor\n");
#else
	puts("Boot linux test ok, control return to monitor\n");
#endif

	return 0;
}

U_BOOT_CMD(
	boota,	3,	1,	do_boota,
	"boota   - boot android bootimg from memory\n",
	"<addr>\n    - boot application image stored in memory\n"
	"\t'addr' should be the address of boot image which is kernel+ramdisk.img\n"
);

