/*
 * include/linux/secure/te_rpc.h
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


/**
 * tee RPC implementation
 */

#ifndef __TE_RPC_H__
#define __TE_RPC_H__
#include <mach/sunxi-smc.h>

/* RPC defines */
#define TEE_SMC_RPC_PREFIX_MASK	 0xFFFF0000
#define TEE_SMC_RPC_PREFIX	 0xFAFA0000
#define TEE_SMC_RPC_FUNC_MASK	 0x0000FFFF
#define TEE_SMC_RPC_FUNC(val)	 ((val) & TEE_SMC_RPC_FUNC_MASK)
#define TEE_SMC_IS_RPC_CALL(val) (((val) & TEE_SMC_RPC_PREFIX_MASK) == \
							TEE_SMC_RPC_PREFIX)
#define TEE_SMC_RPC_VAL(func)    ((func) | TEE_SMC_RPC_PREFIX)

/* RPC services command define */
#define TEE_SMC_RPC_GET_TIME     (0x00A0)
#define TEE_SMC_RPC_SST_COMMAND  (0x00B0)

int te_shmem_init(void);
unsigned int te_shmem_pa2va(unsigned int paddr);
unsigned int te_shmem_va2pa(unsigned int vaddr);

int te_rpc_handle(struct smc_param *param);

#endif /* __TE_RPC_H__ */
