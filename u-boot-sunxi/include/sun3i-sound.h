/*
 * Copyright (C) 2016 Allwinner Electronics
 * Version  :  V1.00
 *  XXW
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __SUN3I_SOUND_H__
#define __SUN3I_SOUND_H__

#define _ADDA_CCU_BASE		(0X01C20000)




#define _ADDA_PHY_BASE		(0X01C23C00)
#define _ADDA_MEM_SZ		(1024)

#define _ADDA_R_OFST(n)		(0X00 + (n)*4U)

#if 1//offset of regs
#define ADDA_PLL_CPU_CTRL		_ADDA_R_OFST(0)
#define ADDA_PLL_AUDIO_CTRL		_ADDA_R_OFST(2)

#endif


#define ADDA_o_DAC_DPC			_ADDA_R_OFST(0)//0x00
#define ADDA_o_DAC_FIFOC		_ADDA_R_OFST(1)//0x04
#define ADDA_o_DAC_FIFOS		_ADDA_R_OFST(2)//0x08
#define ADDA_o_DAC_TXDATA		_ADDA_R_OFST(3)//0x0c
#define ADDA_o_DAC_MIXER_CTRL		_ADDA_R_OFST(8)//0x20
#define ADDA_o_DAC_TX_CNT		_ADDA_R_OFST(16)//0x40


//Anolog Register R/W Register
#define ADDA_PR_CFG_REG			(_ADDA_PHY_BASE + 0x400)

//Anolog Register
#define	HP_PA_VOL			(0x00)
#define LOMIXSC				(0x01)
#define ROMIXSC				(0x02)
#define DAC_PA_SCR			(0x03)
#define LINEIN_GCTR			(0x05)
#define MIC_GCTR			(0x06)
#define PAEN_CTR			(0x07)
#define SPKL_CTR			(0x08)
#define SPKR_CTR			(0x09)
#define MIC2G_LINEOUT_CTR		(0x0A)
#define MIC1G_MICBAIS_CTR		(0x0B)
#define LADCMIXSC			(0x0C)
#define RADCMIXSC			(0x0D)
#define RES_REG				(0x0E)
#define ADC_AP_EN			(0x0F)
#define ADDA_APT0			(0x10)
#define ADDA_APT1			(0x11)
#define ADDA_APT2			(0x12)
#define BIAS_DA16_CTR0			(0x13)
#define BIAS_DA16_CTR1			(0x34)
#define DA16CAL				(0x15)
#define DA16VERIFY			(0x16)
#define BIASCALI			(0x17)
#define BIASVERIFY			(0x18)



#define get_bvalue(addr)	(*((volatile unsigned char  *)(addr)))
#define put_bvalue(addr, v)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define get_hvalue(addr)	(*((volatile unsigned short *)(addr)))
#define put_hvalue(addr, v)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define get_wvalue(addr)	(*((volatile unsigned long  *)(addr)))
#define put_wvalue(addr, v)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

int codec_play_audio_prepare_step1(void);
int codec_play_audio_prepare_step2(void);
int codec_play_audio_prepare_step3(void);
int codec_play_audio(void);
int play_factory_tone(void);
int play_boot_tone(void);

#endif /*__SUN3I_SOUND_H__*/

