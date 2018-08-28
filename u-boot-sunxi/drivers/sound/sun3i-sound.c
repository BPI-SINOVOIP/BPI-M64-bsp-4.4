#include <common.h>
#include <private_boot0.h>
#include <private_uboot.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include <asm/arch/uart.h>
#include <asm/arch/dram.h>
#include <asm/arch/rtc_region.h>
#include <private_toc.h>
#include "sun3i-sound.h"

#include <malloc.h>
#include <sys_partition.h>


#include <asm/arch/ccmu.h>
//#include <asm/arch/ccmu.h>
#include <asm/arch/dma.h>
#include <sunxi_board.h>
#include <sunxi_cfg.h>
#include <sys_config.h>
#include <sys_config_old.h>

static  sunxi_dma_setting_t * codec_tx_dma;
static  uint  codec_tx_dma_hd;
u16*  tone_buf_global;
uint  tone_size_global = 0;  //bytes
uint  tone_skip_size_global = 44;  //bytes


void sun3i_sound_mdelay(int i)
{
	mdelay(i);
}

void sun3i_sound_udelay(int i)
{
	udelay(i);
}


void put_adda_wvalue(u32 addr, u32 data)
{
	u32 reg;
	reg=get_wvalue(ADDA_PR_CFG_REG);
	reg|=(0x1<<28);
	reg=put_wvalue(ADDA_PR_CFG_REG,reg);


	reg=get_wvalue(ADDA_PR_CFG_REG);
	reg&=~(0x1f<<16);
	reg|=(addr<<16);
	reg=put_wvalue(ADDA_PR_CFG_REG,reg);


	reg=get_wvalue(ADDA_PR_CFG_REG);
	reg&=~(0xff<<8);
	reg|=(data<<8);
	reg=put_wvalue(ADDA_PR_CFG_REG,reg);

	reg=get_wvalue(ADDA_PR_CFG_REG);
	reg|=(0x1<<24);
	reg=put_wvalue(ADDA_PR_CFG_REG,reg);


	reg=get_wvalue(ADDA_PR_CFG_REG);
	reg&=~(0x1<<24);
	reg=put_wvalue(ADDA_PR_CFG_REG,reg);
}

int printf_codec(void)
{
	volatile int val = 0;
	volatile int cfg_reg = 0;

	cfg_reg = _ADDA_PHY_BASE + 0x00;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x00  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x04;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x04  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x08;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x08  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x0c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x0c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x10;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x10  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x14;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x14  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x18;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x18  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x1c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x1c  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x20;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x20  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x24;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x24  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x28;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x28  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x2c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x2c  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x30;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x30  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x34;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x34  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x38;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x38  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x3c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x3c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x40;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x40  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x44;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x44  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x48;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x48  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x4c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x4c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x50;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x50  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x54;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x54  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x5c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x5c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x60;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x60  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x64;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x64  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x68;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x68  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x6c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x6c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x70;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x70  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x74;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x74  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x78;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x78  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x7c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x7c  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x80;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x80  = %x\n",val);



	cfg_reg = _ADDA_PHY_BASE + 0x84;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x84  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x88;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x88  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x8c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x8c  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x90;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x90  = %x\n",val);


	cfg_reg = _ADDA_PHY_BASE + 0x94;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x94  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x98;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x98  = %x\n",val);

	cfg_reg = _ADDA_PHY_BASE + 0x9c;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	printf("_ADDA_PHY_BASE + 0x9c  = %x\n",val);

	return 0;
}


int codec_audio_init(void)
{
	volatile int val = 0;
	volatile int tmp_val = 0;
	volatile int cfg_reg = 0;


	cfg_reg = _ADDA_CCU_BASE + ADDA_PLL_AUDIO_CTRL;
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0x90005514 ;
	writel(tmp_val, cfg_reg);


	cfg_reg = _ADDA_CCU_BASE + 0x68;
	tmp_val = 0x00241001;
	writel(tmp_val, cfg_reg);


	cfg_reg = _ADDA_CCU_BASE + 0x2D0;
	tmp_val = 0x00241001;
	writel(tmp_val, cfg_reg);

	cfg_reg = _ADDA_CCU_BASE + 0x140;
	tmp_val = 0x80000000;
	writel(tmp_val, cfg_reg);

	cfg_reg = _ADDA_CCU_BASE + 0x224;
	tmp_val = 0x10100000;
	writel(tmp_val, cfg_reg);

	cfg_reg = _ADDA_PHY_BASE + 0x2c;  //	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0x00000000;
	writel(tmp_val, cfg_reg);
	sun3i_sound_udelay(1000*10);

	cfg_reg = _ADDA_PHY_BASE + 0x34;  //	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0x90000000;
	writel(tmp_val, cfg_reg);
	sun3i_sound_udelay(1000*10);

	cfg_reg = _ADDA_PHY_BASE + 0x28;  //	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0x44555556;
	writel(tmp_val, cfg_reg);


	cfg_reg = _ADDA_PHY_BASE + 0x04;  //0x04 	0x20600f40
	val = *((volatile unsigned int *)cfg_reg);
//	tmp_val = 0x20600f50;   //32k
	tmp_val = 0x60600f50;   //16k
	writel(tmp_val, cfg_reg);

	cfg_reg = _ADDA_PHY_BASE + 0x00;	//	0x80000000
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = val & (~(0xffffffff));			//clear bit28~bit30
	tmp_val = tmp_val | (0x01 << 31);			//set bit28~bit30 to 1
	writel(tmp_val, cfg_reg);

	cfg_reg = _ADDA_PHY_BASE + 0x20;  //0x20	0xcce38337
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0xcc230337;
	writel(tmp_val, cfg_reg);

	sun3i_sound_mdelay(10);

	cfg_reg = _ADDA_PHY_BASE + 0x20;  //0x20	   xcce38337
	val = *((volatile unsigned int *)cfg_reg);
	tmp_val = 0xcc238337;
	writel(tmp_val, cfg_reg);

	return 0;
}

static int codec_dma_send_start(u16 * pbuf, uint byte_cnt)
{
	int ret =0;
	flush_cache((uint)pbuf, byte_cnt);

	ret = sunxi_dma_start(codec_tx_dma_hd, (uint)pbuf, _ADDA_PHY_BASE + ADDA_o_DAC_TXDATA, byte_cnt);

	//sunxi_dma_start(codec_tx_dma_hd, (uint)pbuf, _ADDA_PHY_BASE + 0x40, byte_cnt);
	return ret;
}


int codec_dma_exit(void)
{
	if(codec_tx_dma)
		free(codec_tx_dma);

	sunxi_dma_release(codec_tx_dma_hd);

	return 0;
}

static void sunxi_dma_tx(void *data)
{
	static int  i = 0;
//	i++;
	if (i < 100)
		return;

	sunxi_dma_stop(codec_tx_dma_hd);
	codec_dma_exit();
	printf("codec sunxi_dma_tx\n");
}

int printf_dma1(void)
{

	volatile int cfg_reg = 0;

	volatile int val = 0;

	printf("***************printf_dma1****************\n");
	cfg_reg = 0x01c02100;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02100 = %x\n",val);


	cfg_reg = 0x01c02104;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02104 = %x\n",val);

	cfg_reg = 0x01c02108;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02108 = %x\n",val);

	cfg_reg = 0x01c0210c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0210c = %x\n",val);

	printf("*******************************\n");


	cfg_reg = 0x01c02120;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02120 = %x\n",val);

	cfg_reg = 0x01c02124;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02124 = %x\n",val);

	cfg_reg = 0x01c02128;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02128 = %x\n",val);



	cfg_reg = 0x01c0212c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0212c = %x\n",val);

	printf("*******************************\n");

	cfg_reg = 0x01c02140;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02140 = %x\n",val);

	cfg_reg = 0x01c02144;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02144 = %x\n",val);

	cfg_reg = 0x01c02148;
	val = *((volatile unsigned int *)cfg_reg);
	//printf("0x01c02148 = %x\n",val);

	cfg_reg = 0x01c0214c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0214c = %x\n\n",val);

	printf("*******************************\n\n");
	return 0;
}


int printf_dma(void)
{

	volatile int cfg_reg = 0;

	volatile int val = 0;
	volatile int tmp_val = 0;


	cfg_reg = 0x01c02100;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02100 = %x\n",val);
	sun3i_sound_udelay(1000*100);


	cfg_reg = 0x01c02104;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02104 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02108;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02108 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c0210c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0210c = %x\n",val);
	sun3i_sound_udelay(1000*100);


	cfg_reg = 0x01c02120;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02120 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02124;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02124 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02128;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02128 = %x\n",val);
	sun3i_sound_udelay(1000*100);



	cfg_reg = 0x01c0212c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0212c = %x\n\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02140;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02140 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02144;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02144 = %x\n",val);
	tmp_val = 0x8096b1e8;
	writel(tmp_val, cfg_reg);
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02144x2 = %x\n",val);

	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c02148;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02148 = %x\n",val);
	tmp_val = 0x01c23c0c;
	writel(tmp_val, cfg_reg);
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c02148x2 = %x\n",val);
	sun3i_sound_udelay(1000*100);

	cfg_reg = 0x01c0214c;
	val = *((volatile unsigned int *)cfg_reg);
	printf("0x01c0214c = %x\n\n",val);
	sun3i_sound_udelay(1000*100);
	return 0;
}

#if 0
int load_tone_from_partition(u16 **tone_buf_p, uint *tone_size_p)
{

	uint   tone_start, tone_size;
	u16*  tone_buf ;
	sunxi_partition_init();
	tone_start = sunxi_partition_get_offset_byname("tone"); //tone partition
//	tone_size =  sunxi_partition_get_size_byname("tone");
	tone_size = 128;  //TODO:fix me, why 512 not ok
	printf("tone_start = 0x%x\n", tone_start);
	printf("tone_size  =  %u\n", tone_size);
	if(tone_start && tone_size) {
		tone_buf = (u16*)malloc(tone_size*512);
		memset(tone_buf,0,tone_size*512);
		sunxi_flash_read(tone_start, tone_size, tone_buf);
		//sunxi_flash_read(tone_start + tone_size - (8192+512)/512, 1, buffer);
		sunxi_flash_flush();
		*tone_size_p = tone_size;
		*tone_buf_p = tone_buf;
		printf("tone_buf : %p tone_size  :  %u\n",tone_buf, tone_size);
		return 0;
	} else{
		*tone_buf_p = NULL;
		printf("fail to get tone partition\n");
		return -1;
	}

}
#endif
int load_tone_from_boot0(u16 **tone_buf_p, uint *tone_size_p)
{
	int boottone_size = 0;
	if (!script_parser_fetch("boottone", "boottone_size", &boottone_size, 1))
		printf("tone_size = %d\n", boottone_size);
	else
		printf("can not get tone_size\n");

	*tone_buf_p = (u16*)CONFIG_TONE_STORE_IN_DRAM_BASE;
	*tone_size_p = boottone_size;
	return 0;
}

int codec_play_audio(void)
{
	int ret = 0;
	int boottone_used;
	int workmode = get_boot_work_mode();
	int tone_data_size;
	printf("codec_play_audio\n");
	if (workmode != WORK_MODE_BOOT)
		return 0;

	if (!script_parser_fetch("boottone", "boottone_used", &boottone_used, 1)) {
		printf("boottone_used = %d\n", boottone_used);
		if (boottone_used == 0)
			return 0;
	} else {
		printf("can not get boottone_used\n");
		return 0;
	}

	tone_data_size = tone_size_global - tone_skip_size_global;
	if(tone_data_size <= 0)
	{
		printf("abort, tone_data_size=%d\n", tone_data_size);
		return 0;
	}

	printf("codec_play_audio start\n");
	ret = codec_dma_send_start(tone_buf_global + tone_skip_size_global, tone_data_size);
	if(ret < 0) {
		printf("codec_dma_send_start error = %d\n",ret);
	}

//      exit will cause some error, so just let it go
//	codec_dma_exit();


	printf("codec_play_audio end\n");
	return 0;
}

typedef struct wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"

    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    int fmt_chunk_size; // Should be 16 for PCM
    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    short num_channels;
    int sample_rate;
    int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    short sample_alignment; // num_channels * Bytes Per Sample
    short bit_depth; // Number of bits per sample

    // Data
    char data_header[4]; // Contains "data"
    int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
} wav_header;

int dump_wav_header(struct wav_header* wav)
{
	printf("*****************wav header*******************\n");
	printf("riff:%c%c%c%c\n",wav->riff_header[0],wav->riff_header[1],wav->riff_header[2],wav->riff_header[3]);
	printf("wav_size:%d\n",wav->wav_size);
	printf("wave_header:%c%c%c%c\n",wav->wave_header[0],wav->wave_header[1],wav->wave_header[2],wav->wave_header[3]);
	printf("fmt_header:%c%c%c%c\n",wav->fmt_header[0],wav->fmt_header[1],wav->fmt_header[2],wav->fmt_header[3]);
	printf("fmt_chunk_size:%d\n",wav->fmt_chunk_size);
	printf("audio_format:%d\n",wav->audio_format);
	printf("num_channels:%d\n",wav->num_channels);
	printf("sample_rate:%d\n",wav->sample_rate);
	printf("byte_rate:%d\n",wav->byte_rate);
	printf("sample_alignment:%d\n",wav->sample_alignment);
	printf("bit_depth:%d\n",wav->bit_depth);
	printf("data_header:%c%c%c%c\n",wav->data_header[0],wav->data_header[1],wav->data_header[2],wav->data_header[3]);
	printf("data_bytes:%d\n",wav->data_bytes);
	printf("**********************************************\n");
	return 0;
}

int get_tone_size_from_wav_header(struct wav_header* wav)
{
	int tone_size = wav->wav_size + 8;
	//dump_wav_header(wav);
	printf("get size from wav header:%d\n", tone_size);
	return tone_size;
}

int get_tone_size_from_boot0(void)
{
	int tone_size = uboot_spare_head.boot_ext[1].data[0];
	printf("get size from boot0 :%d\n", tone_size);
	return tone_size;
}


int codec_play_audio_prepare(void)
{
	int ret = 0;
	int boottone_used = 0;

	int workmode = get_boot_work_mode();
	if (workmode != WORK_MODE_BOOT)
		return 0;

	if (!script_parser_fetch("boottone", "boottone_used", &boottone_used, 1)) {
		printf("boottone_used = %d\n", boottone_used);
		if (boottone_used == 0)
			return 0;
	} else {
		printf("can not get boottone_used\n");
		return 0;
	}

	printf("codec_play_audio start\n");
	tone_buf_global = NULL;

//	load_tone_from_partition(&tone_buf_global, &tone_size_global);
	load_tone_from_boot0(&tone_buf_global, &tone_size_global);
	if(tone_size_global <= 0)
	{

//		tone_size_global = get_tone_size_from_wav_header((struct wav_header*)tone_buf_global);
		tone_size_global = get_tone_size_from_boot0();
	}
	if(tone_buf_global == NULL) {
		printf("tone_buf is NULL\n");
		return 0;
	}

	codec_audio_init();

	codec_tx_dma  = malloc_noncache(sizeof(sunxi_dma_setting_t));
	if(!(codec_tx_dma)) {
		printf("no enough memory to malloc\n");
		return -1;
	}
	memset(codec_tx_dma , 0 , sizeof(sunxi_dma_setting_t));

	//codec_tx_dma_hd = sunxi_dma_request(DMAC_DMATYPE_NORMAL);
	codec_tx_dma_hd = sunxi_dma_request_from_last(DMAC_DMATYPE_NORMAL);
	if((codec_tx_dma_hd == 0)) {
		printf("codec_tx_dma_hd request dma failed\n");
		return -1;
	}

	codec_tx_dma->cfg.src_drq_type     = DMAC_CFG_TYPE_DRAM;
	codec_tx_dma->cfg.src_addr_mode    = DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE;
	codec_tx_dma->cfg.src_burst_length = DMAC_CFG_SRC_1_BURST;
	codec_tx_dma->cfg.src_data_width   = DMAC_CFG_SRC_DATA_WIDTH_32BIT;//DMAC_CFG_SRC_DATA_WIDTH_32BIT;

	codec_tx_dma->cfg.dst_drq_type     = 0x0C;                           //DMAC_CFG_DEST_TYPE_CODEC//
	codec_tx_dma->cfg.dst_addr_mode    = DMAC_CFG_DEST_ADDR_TYPE_IO_MODE;//DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	codec_tx_dma->cfg.dst_burst_length = DMAC_CFG_DEST_1_BURST;
	codec_tx_dma->cfg.dst_data_width   = DMAC_CFG_DEST_DATA_WIDTH_16BIT;//DMAC_CFG_DEST_DATA_WIDTH_32BIT
	codec_tx_dma->cfg.wait_state       = 4;
	codec_tx_dma->cfg.continuous_mode  = 0;

	ret = sunxi_dma_setting(codec_tx_dma_hd, (void *)codec_tx_dma);
	if(ret < 0) {
		printf("sunxi_dma_setting error = %d\n",ret);
	}

	ret = sunxi_dma_enable_int(codec_tx_dma_hd);
	if(ret < 0) {
		printf("sunxi_dma_enable_int error = %d\n",ret);
	}

	sunxi_dma_install_int(codec_tx_dma_hd,sunxi_dma_tx,0);

	gpio_request_simple("boottone", NULL);

#if 0  //move to another function, to avoid call delay
	sun3i_sound_mdelay(800);
	tone_size = 35 * 1024 / 512;  //debug: hard code tone_size
	ret = codec_dma_send_start(tone_buf, tone_size*512);
	if(ret < 0){
		printf("codec_dma_send_start error = %d\n",ret);

	}

//	codec_dma_exit();


	printf("codec_play_audio end\n");
#endif
	return 0;
}



int i2s_play_audio(void)
{
	return 0;
}

int do_boottone (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
//	codec_play_audio_prepare();
//	sun3i_sound_mdelay(800);
	codec_play_audio();
	return 0;
}

U_BOOT_CMD(
	boottone,	2,	1,	do_boottone,
	"boottone   - play boot tone\n",
);






