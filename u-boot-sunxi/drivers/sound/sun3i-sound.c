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
#include <asm/arch/dma.h>
#include <sunxi_board.h>
#include <sunxi_cfg.h>
#include <sys_config.h>
#include <sys_config_old.h>

static  sunxi_dma_setting_t * codec_tx_dma;
static  uint  codec_tx_dma_hd;
u16*  tone_buf_global;
int tone_size_global = 0;
int tone_skip_size_global = 44;
int codec_play_audio_enable = 1;
int set_factory_tone_size(void);
int set_boot_tone_size(void);
int set_sample_rate(int sample_rate);

void sun3i_sound_mdelay(int i)
{
	mdelay(i);
}

void sun3i_sound_udelay(int i)
{
	udelay(i);
}

typedef struct wav_header {
    /* RIFF Header */
    char riff_header[4]; /* Contains "RIFF" */
    int wav_size; /* Size of the wav portion of the file, which follows the first 8 bytes. File size - 8 */
    char wave_header[4]; /* Contains "WAVE" */

    /* Format Header */
    char fmt_header[4]; /* Contains "fmt " (includes trailing space) */
    int fmt_chunk_size; /* Should be 16 for PCM */
    short audio_format; /* Should be 1 for PCM. 3 for IEEE Float */
    short num_channels;
    int sample_rate;
    int byte_rate; /* Number of bytes per second. sample_rate * num_channels * Bytes Per Sample */
    short sample_alignment; /* num_channels * Bytes Per Sample */
    short bit_depth; /* Number of bits per sample */

    /* Data */
    char data_header[4]; /* Contains "data" */
    int data_bytes; /* Number of bytes in data. Number of samples * num_channels * sample byte size */
    /* uint8_t bytes[]; */  /* Remainder of wave file is bytes */
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

int is_valid_wav_header(struct wav_header* wav)
{
	if( strncmp("RIFF", wav->riff_header, 4) == 0)
		return 1;

	return 0;
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
	tmp_val = 0x41555556;
	writel(tmp_val, cfg_reg);


	cfg_reg = _ADDA_PHY_BASE + 0x04;  //0x04 	0x20600f40
	val = *((volatile unsigned int *)cfg_reg);
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

int set_reg_by_wav_header(struct wav_header* wav)
{
	if (is_valid_wav_header(wav))
	{
		printf("set sample_rate:%d\n",wav->sample_rate);
		set_sample_rate(wav->sample_rate);
	}
	return 0;
}

#if 0
int load_tone_from_partition(u16 **tone_buf_p, int *tone_size_p)
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
int load_tone_from_boot0(u16 **tone_buf_p, int *tone_size_p)
{
	*tone_buf_p = (u16*)CONFIG_TONE_STORE_IN_DRAM_BASE;
	return 0;
}

int play_factory_tone(void)
{
	if (codec_play_audio_enable == 0)
	{
		printf("codec_play_audio_enable is 0\n");
		return 0;
	}
#ifdef CONFIG_LOAD_FACTORY_TONE_BY_ENV
	int ret = 0;
	/* here used to run load_factory_tone, but now just use load_boot_tone */
	printf("play same tone as boot, run load_boot_tone:%s\n", getenv("load_boot_tone"));
	ret = run_command(getenv("load_boot_tone"), 0);
	if (ret == 0)
	{
		printf("load factory tone success\n");
		set_factory_tone_size();
		printf("play factory tone\n");
		codec_play_audio();
		mdelay(1000);
	}
	else
	{
		printf("load factory tone fail\n");
	}
#else
	mdelay(50); /* make sure there is enough time between init regs and play, or the play may not complete */
	set_factory_tone_size();
	printf("play factory tone\n");
	codec_play_audio();
#endif
	codec_play_audio_enable = 0;
	return 0;
}

int play_boot_tone(void)
{
	if (codec_play_audio_enable == 0)
	{
		printf("codec_play_audio_enable is 0\n");
		return 0;
	}
#ifdef CONFIG_LOAD_BOOT_TONE_BY_ENV
	int ret = 0;
	printf("run load_boot_tone:%s\n", getenv("load_boot_tone"));
	ret = run_command(getenv("load_boot_tone"), 0);
	if (ret == 0)
	{
		printf("load boot tone success\n");
		set_boot_tone_size();
		printf("play boot tone\n");
		codec_play_audio();
	}
	else
	{
		printf("load boot tone fail\n");
	}
#else
	mdelay(50); /* make sure there is enough time between init regs and play, or the play may not complete */
	set_boot_tone_size();
	printf("play boot tone\n");
	codec_play_audio();
#endif
	codec_play_audio_enable = 0;
	return 0;
}

int codec_play_audio(void)
{
	int ret = 0;
	int boottone_used;
	int workmode = get_boot_work_mode();
	u16* buf_start;
	int buf_size = 0;

	printf("codec_play_audio\n");
	if (workmode != WORK_MODE_BOOT)
		return 0;

	if (codec_play_audio_enable == 0)
	{
		printf("codec_play_audio_enable is 0\n");
		return 0;
	}

	if (!script_parser_fetch("boottone", "boottone_used", &boottone_used, 1)) {
		printf("boottone_used = %d\n", boottone_used);
		if (boottone_used == 0)
			return 0;
	} else {
		printf("can not get boottone_used\n");
		return 0;
	}
	set_reg_by_wav_header((struct wav_header*)tone_buf_global);
	printf("codec_play_audio start\n");
	buf_start = tone_buf_global + tone_skip_size_global;
	buf_size = tone_size_global - tone_skip_size_global;
	buf_size &= 0x3fc00;   /* limit to 256k , align to 1024bytes */
	printf("dma buff size:%d 0x%x\n", buf_size, buf_size);
	ret = codec_dma_send_start(buf_start, buf_size);
	if(ret < 0) {
		printf("codec_dma_send_start error = %d\n",ret);
	}

//      exit will cause some error, so just let it go
//	codec_dma_exit();


	printf("codec_play_audio end\n");
	return 0;
}


int set_sample_rate(int sample_rate)
{
	volatile int tmp_val = 0;
	volatile int cfg_reg = 0;
	cfg_reg = _ADDA_PHY_BASE + 0x04;  //0x04 	0x20600f40
	int reg_sample = 0;
	if (sample_rate == 48000)
		reg_sample = (0x0 << 29);  //011 48k
	else if(sample_rate == 24000)
		reg_sample = (0x2 << 29);   //010  24k
	else if(sample_rate == 12000)
		reg_sample = (0x4 << 29);   //100  12k
	else if(sample_rate == 192000)
		reg_sample = (0x6 << 29);   //110  192k
	else if(sample_rate == 32000)
		reg_sample = (0x1 << 29);   //001  32k
	else if(sample_rate == 16000)
		reg_sample = (0x3 << 29);   //011  16k
	else if(sample_rate == 8000)
		reg_sample = (0x5 << 29);   //101  8k
	else if(sample_rate == 96000)
		reg_sample = (0x7 << 29);   //111  96k

	tmp_val = 0x00600f50 | reg_sample ;
	writel(tmp_val, cfg_reg);

	return 0;
}



int get_tone_size_from_wav_header(struct wav_header* wav)
{
	int tone_size = -1;
	//dump_wav_header(wav);
	if (is_valid_wav_header(wav))
	{
		tone_size = wav->wav_size + 8;
		printf("get size from wav header:%d\n", tone_size);
	}
	else
	{
		printf("not a valid wav header\n");
	}

	return tone_size;
}

int get_boot_tone_size_from_sysconfig(void)
{
	int boottone_size = -1;
	if (!script_parser_fetch("boottone", "boottone_size", &boottone_size, 1))
		printf("get size from sys_config:%d\n", boottone_size);

	return boottone_size;
}

int get_boot_tone_size_from_boot0(void)
{
	int tone_size = uboot_spare_head.boot_ext[1].data[0];
	printf("get size from boot0 :%d\n", tone_size);
	return tone_size;
}

int set_boot_tone_size(void)
{
	tone_size_global = get_boot_tone_size_from_sysconfig();

	if (tone_size_global <= 0)
	{
		tone_size_global = get_boot_tone_size_from_boot0();
	}

	if (tone_size_global <= 0)
	{
		tone_size_global = get_tone_size_from_wav_header((struct wav_header*)tone_buf_global);
	}

	printf("tone_size_global:%d\n", tone_size_global);
	return 0;
}

int set_factory_tone_size(void)
{
	tone_size_global = get_tone_size_from_wav_header((struct wav_header*)tone_buf_global);

	printf("tone_size_global:%d\n", tone_size_global);
	return 0;
}

int need_play_tone(void)
{
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

	return 1;
}

/* Call me early enough to make sure codec have enough time to prepare */
int codec_play_audio_prepare_step1(void)
{
	if ( need_play_tone() == 0 )
		return 0;

	printf("codec_play_audio start\n");
	codec_audio_init();
	return 0;
}

/* Call me early, but after dma is ready */
int codec_play_audio_prepare_step2(void)
{
	int ret = 0;

	if ( need_play_tone() == 0 )
		return 0;

	tone_buf_global = NULL;

	load_tone_from_boot0(&tone_buf_global, &tone_size_global);

	if(tone_buf_global == NULL) {
		printf("tone_buf is NULL\n");
		return 0;
	}

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

	return 0;
}

/* Call me to enable speaker, enable too early may cause pop */
int codec_play_audio_prepare_step3(void)
{
       gpio_request_simple("boottone", NULL);
       return 0;
}

int do_boottone (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
//	sun3i_sound_mdelay(800);
	u16* tone_buf_global_tmp;
	int tone_size_global_tmp;
	int codec_play_audio_enable_tmp;

	if (argc >= 2)
	{
		tone_buf_global_tmp = tone_buf_global;
		tone_buf_global = (u16 *)simple_strtoul(argv[1], NULL, 16);
	}
	if (argc >= 3)
	{
		tone_size_global_tmp  = tone_size_global;
		tone_size_global = simple_strtoul(argv[2], NULL, 16);
	}
	printf("play tone buffer %d\n",(int)tone_buf_global);
	printf("play tone size %d\n", tone_size_global);

	codec_play_audio_enable_tmp = codec_play_audio_enable;
	codec_play_audio_enable = 1;
	codec_play_audio();
	codec_play_audio_enable = codec_play_audio_enable_tmp;

	if (argc >= 2)
		tone_buf_global = tone_buf_global_tmp;

	if (argc >= 3)
		tone_size_global = tone_size_global_tmp;
	return 0;
}

int do_play_boot_tone (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	play_boot_tone();
	return 0;
}

int do_play_factory_tone (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	play_factory_tone();
	return 0;
}

U_BOOT_CMD(
	boottone,	4,	1,	do_boottone,
	"boottone   - play boot tone\n",
	"argv1: tone_buff\n argv2: tone_size"
);

U_BOOT_CMD(
	play_boot_tone,	1,	1,	do_play_boot_tone,
	"play_boot_tone   - play boot tone\n",
);

U_BOOT_CMD(
	play_factory_tone,	1,	1,	do_play_factory_tone,
	"play_factory_tone   - play factory tone\n",
);






