/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include "common.h"
#include "asm/io.h"
#include "asm/arch/efuse.h"

#define SID_OP_LOCK  (0xAC)
#define SBROM_ACCELERATION_ENABLE_BIT	29

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
uint sid_read_key(uint key_index)
{
	uint reg_val;

	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff << 16) | 0x3);
	reg_val |= key_index << 16;
	writel(reg_val, SID_PRCTL);

	reg_val &= ~((0xff << 8) | 0x3);
	reg_val |= (SID_OP_LOCK << 8) | 0x2;
	writel(reg_val, SID_PRCTL);

	while(readl(SID_PRCTL) & 0x2) {
		;
	}

	reg_val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	writel(reg_val, SID_PRCTL);

	reg_val = readl(SID_RDKEY);

	return reg_val;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sid_program_key(uint key_index, uint key_value)
{
	uint reg_val;

	writel(key_value, SID_PRKEY);

	reg_val = readl(SID_PRCTL);
	reg_val &= ~((0x1ff<<16)|0x3);
	reg_val |= key_index<<16;
	writel(reg_val, SID_PRCTL);

	reg_val &= ~((0xff<<8)|0x3);
	reg_val |= (SID_OP_LOCK<<8) | 0x1;
	writel(reg_val, SID_PRCTL);

	while(readl(SID_PRCTL)&0x1){};

	reg_val &= ~((0x1ff<<16)|(0xff<<8)|0x3);
	writel(reg_val, SID_PRCTL);

	return;
}
/*
*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void sid_read_rotpk(void *dst)
{
	uint chipid_index = 0x64;
	uint id_length = 32;
	uint i = 0;
	for(i = 0 ; i < id_length ;i+=4 )
	{
		*(u32*)dst  = sid_read_key(chipid_index + i );
		dst += 4 ;
	}
	return ;
}

/*
*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sid_write_rotpk_hash(uint* dst)
{
	uint chipid_index = EFUSE_ROTPK;
	uint id_length = (SID_ROTPK_SIZE >> 3);
	uint i = 0;
	uint val = 0;

	printf("%s entry\n",__func__);
	if (dst == NULL) {
		printf("%s : dst is NULL\n",__func__);
		return -1;
	}

	for(i = 0 ; i < id_length ; i+=4) {
		printf("chipid_inde[0x%X] =0x%X\n",chipid_index + i, dst[i >> 2]);
		sid_program_key(chipid_index + i , dst[i >> 2]);
	}

	for(i = 0 ; i < id_length ; i+=4) {
		val = sid_read_key(chipid_index + i);
		printf("chipid_inde[0x%X] =0x%X\n", chipid_index + i, val);
		if (val != dst [i >> 2]) {
			printf("the %d chipid_index(0x%X) !=dst(0x%X)\n", i, val, dst[i >> 2]);
			return -1;
		}
	}

	return 0;
}

/*
*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sid_enable_security_mode(void)
{
    uint reg_val;

    reg_val  = sid_read_key(EFUSE_LCJS);
    reg_val |= ((0x1 << 11) | (0x1 << SBROM_ACCELERATION_ENABLE_BIT));		//使能securebit
    sid_program_key(EFUSE_LCJS, reg_val);
    reg_val  = sid_read_key(EFUSE_LCJS);
	pr_force("secure bit :0x%X\n", reg_val);
	reg_val &= ((0x1 << 11) | (0x1 << SBROM_ACCELERATION_ENABLE_BIT));
	if (!reg_val) {
		reg_val  = sid_read_key(EFUSE_LCJS);
		printf("sid_enable_security_mode erro: 0x%X\n", reg_val);
		return -1;
	}
    return 0;
}

/*
*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sid_write_ssk(uint *dst)
{
	uint chipid_index = EFUSE_SSK;
	uint id_length = (SID_SSK_SIZE >> 3);
	uint i = 0;
	uint val = 0;

	printf("%s entry\n",__func__);
	if (dst == NULL) {
		printf("%s : dst is NULL\n",__func__);
		return -1;
	}

	for(i = 0 ; i < id_length ; i+=4) {
		printf("chipid_inde[0x%X] =0x%X\n",chipid_index + i, dst[i >> 2]);
		sid_program_key(chipid_index + i , dst[i >> 2]);
	}

	for(i = 0 ; i < id_length ; i+=4) {
		val = sid_read_key(chipid_index + i);
		printf("chipid_inde[0x%X] =0x%X\n", chipid_index + i, val);
		if (val != dst [i >> 2]) {
			printf("the %d chipid_index(0x%X) !=dst(0x%X)\n", i, val, dst[i >> 2]);
			return -1;
		}
	}

	return 0;
}

int handler_offline_burn_key(u32 *buf)
{

	pr_force("%s : enter\n",__func__);
	if (sid_enable_security_mode() < 0) {
		pr_force("sid_enable_security_mode is erro\n");
		return -1;
	}

	if (sid_write_rotpk_hash(buf) < 0) {
		pr_force("sid_write_rotpk is erro\n");
		return -1;
	}

	if (sid_write_ssk(buf + 8) < 0) {
		pr_force("sid_write_ssk is erro\n");
		return -1;
	}

	return 0;
}

void printf_key_info(void)
{
	uint i = 0;
	uint val = 0;

	pr_force("%s : enter\n", __func__);
	val = sid_read_key(EFUSE_LCJS);
	pr_force("secure bit :0x%X\n", val);

	pr_force("rotpk :\n");
	for(i = 0 ; i < 32 ; i+=4 )
	{
		val = sid_read_key(EFUSE_ROTPK + i );
		printf("chipid_inde[0x%X] =0x%X\n", EFUSE_ROTPK + i, val);
	}

	pr_force("ssk :\n");
	for(i = 0 ; i < 16 ; i+=4 )
	{
		val = sid_read_key(EFUSE_SSK + i );
		printf("chipid_inde[0x%X] =0x%X\n", EFUSE_SSK + i, val);
	}

}
