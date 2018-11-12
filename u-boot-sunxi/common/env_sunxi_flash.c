/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <mmc.h>
#include <search.h>
#include <errno.h>
#include <nand.h>
#ifdef CONFIG_ALLWINNER
#include <boot_type.h>
#include <sys_partition.h>
#include <sunxi_board.h>
#endif

char * env_name_spec = "SUNXI";

#ifdef ENV_IS_EMBEDDED
env_t *env_ptr = &environment;
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr;
#endif /* ENV_IS_EMBEDDED */

/* local functions */
#if !defined(ENV_IS_EMBEDDED)
static void use_default(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

const uchar sunxi_sprite_environment[] = {
#ifdef  CONFIG_SUNXI_SPRITE_ENV_SETTINGS
	CONFIG_SUNXI_SPRITE_ENV_SETTINGS
#endif
	"\0"
};


#if !defined(CONFIG_ENV_OFFSET)
#define CONFIG_ENV_OFFSET 0
#endif

//loff_t env_offset = (loff_t)CONFIG_ENV_ADDR;
size_t env_size = (size_t)CONFIG_ENV_SIZE;

uchar env_get_char_spec(int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}

int env_init(void)
{
	/* use default */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}

#ifdef CONFIG_ENV_OFFSET_REDUND
#error No support for redundant environment on sunxi nand yet!
#endif

static void flash_use_efex_env(void)
{
	if (himport_r(&env_htab, (char *)sunxi_sprite_environment,
		    sizeof(sunxi_sprite_environment), '\0', H_INTERACTIVE,
		    0, NULL) == 0) {
		error("Environment import failed: errno = %d\n", errno);
	}
	gd->flags |= GD_FLG_ENV_READY;

	return ;
}

int saveenv(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	int     ret;
	u32     start;

	printf("saveenv storage_type = %d\n", get_boot_storage_type());
	start = sunxi_partition_get_offset_byname(CONFIG_SUNXI_ENV_PARTITION);
	if(!start){
		printf("fail to find part named %s\n", CONFIG_SUNXI_ENV_PARTITION);
		return -1;
	}

	ret = env_export(env_new);
	if(ret)
		goto fini;

#ifndef CONFIG_SUNXI_ENV_NOT_BACKUP
	uint env_size = 0;
	env_size = sunxi_partition_get_size_byname(CONFIG_SUNXI_ENV_PARTITION);
	if(env_size >= ((CONFIG_ENV_SIZE * 2)/512))
	{
		char backup_buf[CONFIG_ENV_SIZE*2];
		memcpy(backup_buf,env_new,CONFIG_ENV_SIZE);
		memcpy((backup_buf+CONFIG_ENV_SIZE),env_new,CONFIG_ENV_SIZE);
		ret = sunxi_flash_write(start, (CONFIG_ENV_SIZE*2)/512, backup_buf);
	}
	else
	{
		printf("env size is %u\n",env_size);
		puts("env partition is too small!\n");
		puts("can't enabled backup env functions\n");
		ret = sunxi_flash_write(start, CONFIG_ENV_SIZE/512, env_new);
	}
#else
	ret = sunxi_flash_write(start, CONFIG_ENV_SIZE/512, env_new);
#endif
	sunxi_flash_flush();
fini:
	return ret;
}

#ifndef CONFIG_SUNXI_ENV_NOT_BACKUP
//if index is 0,recover the normal env area
//if index is 1,recover the backup env area
int recover_env(char *buf,int index)
{
	int ret;
	u32 start;

	start = sunxi_partition_get_offset_byname(CONFIG_SUNXI_ENV_PARTITION);
	if(!start){
		printf("fail to find part named %s\n", CONFIG_SUNXI_ENV_PARTITION);
		return -1;
	}

	if(index == 0)
	{
		ret = sunxi_flash_write(start, CONFIG_ENV_SIZE/512, buf);
	}
	else if(index == 1)
	{
		ret = sunxi_flash_write(start+(CONFIG_ENV_SIZE/512), CONFIG_ENV_SIZE/512, buf);
	}
	else
	{
		puts("Input recover env index isn't 0 or 1\n");
		return -1;
	}

	sunxi_flash_flush();
	return ret;
}
#endif

static void flash_env_relocate_spec(int workmode)
{
#if !defined(ENV_IS_EMBEDDED)
#ifndef CONFIG_SUNXI_ENV_NOT_BACKUP
	char env_buf[CONFIG_ENV_SIZE*2];
#else
	char buf[CONFIG_ENV_SIZE];
#endif
	u32 start;

	if((workmode & WORK_MODE_PRODUCT) && (!(workmode & WORK_MODE_UPDATE)))
	{
		flash_use_efex_env();
	}
	else
	{
		start = sunxi_partition_get_offset_byname(CONFIG_SUNXI_ENV_PARTITION);
		if(!start){
			printf("fail to find part named %s\n", CONFIG_SUNXI_ENV_PARTITION);
			use_default();
			return;
		}

#ifndef CONFIG_SUNXI_ENV_NOT_BACKUP
		if(!sunxi_flash_read(start, (CONFIG_ENV_SIZE*2)/512, env_buf))
#else
		if(!sunxi_flash_read(start, CONFIG_ENV_SIZE/512, buf))
#endif
		{
			use_default();
			return;
		}

#ifndef CONFIG_SUNXI_ENV_NOT_BACKUP
		uint env_size = 0;
		char * normal_buf_p = env_buf;
		char * backup_buf_p = env_buf;
		//point to backup area
		backup_buf_p += CONFIG_ENV_SIZE;

		env_t *ep = (env_t *)normal_buf_p;
		env_t *ep_b = (env_t *)backup_buf_p;

		uint32_t crc;
		uint32_t crc_b;
		memcpy(&crc, &ep->crc, sizeof(crc));
		memcpy(&crc_b, &ep_b->crc, sizeof(crc));
		uint32_t crc_value = crc32(0, ep->data, ENV_SIZE);
		uint32_t crc_b_value = crc32(0, ep_b->data, ENV_SIZE);

		env_size = sunxi_partition_get_size_byname(CONFIG_SUNXI_ENV_PARTITION);

		if(env_size >= ((CONFIG_ENV_SIZE * 2)/512))
		{
			if((crc_value == crc) && (crc_b_value == crc_b))
			{
				if(crc != crc_b)
				{
					puts("env and backup env are not synchronized,now to synchronize\n");
					recover_env(normal_buf_p,1);
				}
				env_import(normal_buf_p, 0);
			}
			else if((crc_value != crc) && (crc_b_value == crc_b))
			{
				puts("env check CRC fail\n");
				puts("Now use backup env\n");
				env_import(backup_buf_p,0);
				recover_env(backup_buf_p,0);
			}
			else if((crc_value == crc) && (crc_b_value != crc_b))
			{
				puts("backup env check CRC fail\n");
				puts("Now update backup env\n");
				env_import(normal_buf_p, 0);
				recover_env(normal_buf_p,1);
			}
			else if((crc_value != crc) && (crc_b_value != crc_b))
			{
				puts("env and backup env all check CRC fail\n");
				puts("Now set env to default\n");
				set_default_env("!bad CRC");
				saveenv();
				return;
			}
		}
		else
		{
			printf("env size is %u\n",env_size);
			puts("env partition is too small!\n");
			puts("can't enabled backup env functions\n");
			env_import(normal_buf_p,1);
		}
#else
		env_import(buf, 1);
#endif
	}

#endif
}


void env_relocate_spec(void)
{
	debug("env_relocate_spec storage_type = %d\n", get_boot_storage_type());
	flash_env_relocate_spec(uboot_spare_head.boot_data.work_mode);
}

#if !defined(ENV_IS_EMBEDDED)
static void use_default()
{
	set_default_env(NULL);
}
#endif

/*
 *  This is called before nand_init() so we can't read NAND to
 *  validate env data.
 */

