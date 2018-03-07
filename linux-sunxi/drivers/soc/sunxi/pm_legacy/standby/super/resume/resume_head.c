/*
 * drivers/soc/sunxi/pm_legacy/standby/super/resume/resume_head.c
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

/*
*****************************************************************************
*                                                  eGON
*                                 the Embedded GO-ON Resumeloader System
*
*              Copyright(C), 2006-2008, SoftWinners Microelectronic Co., Ltd.
*					All Rights Reserved
*
* File Name : Resume_head.c
*
* Author : Gary.Wang
*
* Version : 1.1.0
*
* Date : 2007.11.06
*
* Description : This file defines the file head part of Resume,
*		which contains some important infomations such as magic,
*		platform information and so on, and MUST be allocted in the head
*		of Resume.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Gary.Wang       2007.11.06      1.1.0        build the file
*
******************************************************************************
*/
#include "./../super_i.h"

#pragma  arm section  rodata = "head"

const resume_file_head_t  resume_head = {
		/* jump_instruction */
		(0xEA000000 | (((sizeof(resume_file_head_t) +
			sizeof(int) - 1) / sizeof(int) - 2)
				 & 0x00FFFFFF)),
		/*0xe3a0f000,*/
		RESUMEX_MAGIC,
		STAMP_VALUE,
		RESUMEX_ALIGN_SIZE,
		sizeof(resume_file_head_t),
		RESUME_PUB_HEAD_VERSION,
		RESUMEX_FILE_HEAD_VERSION,
		RESUMEX_VERSION,
		EGON_VERSION,
};

