#ifndef _PRIVATE_TEE_HEAD_
#define _PRIVATE_TEE_HEAD_

struct spare_optee_head {
	unsigned int jump_instruction;
	unsigned char magic[8];
	unsigned int dram_size;
	unsigned int drm_size;
	unsigned int length;
	unsigned int optee_length;
	unsigned char version[8];
	unsigned char platform[8];
	unsigned int dram_para[32];
};

#endif
