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
	unsigned char expand_magic[8];
	unsigned char expand_version[8];
	unsigned int vdd_cpua;
	unsigned int vdd_cpub;
	unsigned int vdd_sys;
	unsigned int vcc_pll;
	unsigned int vcc_io;
	unsigned int vdd_res1;
	unsigned int vdd_res2;
	unsigned int pmu_count;
	unsigned int pmu_port;
	unsigned int pmu_para;
	unsigned char pmu_id[4];
	unsigned char pmu_addr[4];
};

#endif
