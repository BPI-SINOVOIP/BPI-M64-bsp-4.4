/*
 * sound\soc\sunxi\sunxi-daudio.h
 * (C) Copyright 2014-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_I2S_H_
#define	__SUNXI_I2S_H_

/*
* DAUDIO register definition
* DA0/DA1/DA2(HDMI)
*/
#define	SUNXI_DAUDIO_CTL		0x00
#define	SUNXI_DAUDIO_FMT0		0x04
#define	SUNXI_DAUDIO_FMT1		0x08
#define	SUNXI_DAUDIO_INTSTA		0x0C
#define	SUNXI_DAUDIO_RXFIFO		0x10
#define	SUNXI_DAUDIO_FIFOCTL	0x14
#define	SUNXI_DAUDIO_FIFOSTA	0x18
#define	SUNXI_DAUDIO_INTCTL		0x1C
#define	SUNXI_DAUDIO_TXFIFO		0x20
#define	SUNXI_DAUDIO_CLKDIV		0x24
#define	SUNXI_DAUDIO_TXCNT		0x28
#define	SUNXI_DAUDIO_RXCNT		0x2C
#define	SUNXI_DAUDIO_TXCHSEL	0x30
#define	SUNXI_DAUDIO_TXCHMAP	0x34
#define	SUNXI_DAUDIO_RXCHSEL	0x38
#define	SUNXI_DAUDIO_RXCHMAP	0x3C

/* SUNXI_DAUDIO_CTL:0x00 */
#define	SDO3_EN			11
#define	SDO2_EN			10
#define	SDO1_EN			9
#define	SDO0_EN			8
#define ASS_TX_UR		6
#define MS_SEL			5
#define PCM_INTERFACE	4
#define	LOOP_EN			3
#define	CTL_TXEN		2
#define	CTL_RXEN		1
#define	GLOBAL_EN		0

/* SUNXI_DAUDIO_FMT0:0x04 */
#define	LR_CLK_PARITY		7
#define BCLK_PARITY			6
#define	SAMPLE_RESOLUTION	4
#define WORD_SEL_SIZE		2
#define SD_FMT				0

/* SUNXI_DAUDIO_FMT1:0x08 */
#define PCM_SYNC_PERIOD	12
#define PCM_SYNC_OUT	11
#define PCM_OUT_MUTE	10
#define MLS_SEL			9
#define SEXT			8
#define SLOT_INDEX		6
#define SLOT_WIDTH		5
#define SHORT_SYNC_SEL	4
#define	RX_PDM			2
#define	TX_PDM			0

/* SUNXI_DAUDIO_INTSTA:0x0C */
#define	TXU_INT			6
#define	TXO_INT			5
#define	TXE_INT			4
#define	RXU_INT			2
#define RXO_INT			1
#define	RXA_INT			0

/* SUNXI_DAUDIO_FIFOCTL:0x14 */
#define	HUB_EN			31
#define	FIFO_CTL_FTX	25
#define	FIFO_CTL_FRX	24
#define	TXTL			12
#define	RXTL			4
#define	TXIM			2
#define	RXOM			0

/* SUNXI_DAUDIO_FIFOSTA:0x18 */
#define	FIFO_TXE		28
#define	FIFO_TX_CNT		16
#define	FIFO_RXA		8
#define	FIFO_RX_CNT		0

/* SUNXI_DAUDIO_INTCTL:0x1C */
#define	TXDRQEN			7
#define	TXUI_EN			6
#define	TXOI_EN			5
#define	TXEI_EN			4
#define	RXDRQEN			3
#define	RXUI_EN			2
#define	RXOI_EN			1
#define	RXAI_EN			0

/* SUNXI_DAUDIO_CLKDIV:0x24 */
#define	MCLKOUT_EN		7
#define	BCLK_DIV		4
#define	MCLK_DIV		0

/* SUNXI_DAUDIO_TXCHSEL:0x30 */
#define	TX_CHSEL		0

/* SUNXI_DAUDIO_TXCHSEL:0x38 */
#define	RX_CHSEL		0

/* CHMAP default setting */
#define	SUNXI_DEFAULT_TXCHMAP	0x76543210
#define SUNXI_DEFAULT_RXCHMAP	0x3210

/* SUNXI_DAUDIO_CTL:0x00 */
#define	SUNXI_DAUDIO_MODE_CTL_MASK		1
#define	SUNXI_DAUDIO_MODE_CTL_PCM		1
#define	SUNXI_DAUDIO_MODE_CTL_I2S		0

#define	SUNXI_DAUDIO_MODE_CTL_LEFT		1
#define	SUNXI_DAUDIO_MODE_CTL_RIGHT		2
#define	SUNXI_DAUDIO_MODE_CTL_REVD		3

/* Master Slave setting */
#define SUNXI_DAUDIO_MS_SEL_MASK		1
#define SUNXI_DAUDIO_MS_SEL_MASTER		0
#define SUNXI_DAUDIO_MS_SEL_SLAVE		1

/* combine LRCK_CLK & BCLK setting */
#define	SUNXI_DAUDIO_LRCK_OUT_MASK		3
#define	SUNXI_DAUDIO_LRCK_OUT_DISABLE	0
#define	SUNXI_DAUDIO_LRCK_OUT_ENABLE	3

/* SUNXI_DAUDIO_FMT0 */
#define	SUNXI_DAUDIO_WORD_SIZE_MASK		3
#define	SUNXI_DAUDIO_WORD_SIZE_16BIT	0
#define	SUNXI_DAUDIO_WORD_SIZE_20BIT	1
#define	SUNXI_DAUDIO_WORD_SIZE_24BIT	2
#define	SUNXI_DAUDIO_WORD_SIZE_32BIT	3

#define SUNXI_DAUDIO_TX_DATA_MODE_MASK		3
#define SUNXI_DAUDIO_RX_DATA_MODE_MASK		3
#define SUNXI_DAUDIO_SLOT_WIDTH_MASK	1
#define	SUNXI_DAUDIO_PCM_SYNC_PERIOD_MASK	7
/* Left Blank */
#define	SUNXI_DAUDIO_SR_MASK			3
#define	SUNXI_DAUDIO_SR_16BIT			0
#define	SUNXI_DAUDIO_SR_20BIT			1
#define	SUNXI_DAUDIO_SR_24BIT			2

#define	SUNXI_DAUDIO_LRCK_POLARITY_NOR	0
#define	SUNXI_DAUDIO_LRCK_POLARITY_INV	1
#define	SUNXI_DAUDIO_BCLK_POLARITY_NOR	0
#define	SUNXI_DAUDIO_BCLK_POLARITY_INV	1

#define SUNXI_DAUDIO_FMT_CTL_MASK		3
#define SUNXI_DAUDIO_FMT_CTL_I2S		0
#define SUNXI_DAUDIO_FMT_CTL_LEFT		1
#define SUNXI_DAUDIO_FMT_CTL_RIGHT		2

/* SUNXI_DAUDIO_FMT1 */
#define	SUNXI_DAUDIO_FMT1_DEF			0x4020
#define SUNXI_DAUDIO_FMT_LONG_FRAME		0
#define SUNXI_DAUDIO_FMT_SHORT_FRAME	1

/* SUNXI_DAUDIO_FIFOCTL */
#define	SUNXI_DAUDIO_TXIM_MASK			1
#define	SUNXI_DAUDIO_TXIM_VALID_MSB		0
#define	SUNXI_DAUDIO_TXIM_VALID_LSB		1
/* Left Blank */
#define	SUNXI_DAUDIO_RXOM_MASK			3
/* Expanding 0 at LSB of RX_FIFO */
#define	SUNXI_DAUDIO_RXOM_EXP0			0
/* Expanding sample bit at MSB of RX_FIFO */
#define	SUNXI_DAUDIO_RXOM_EXPH			1
/* Fill RX_FIFO low word be 0 */
#define	SUNXI_DAUDIO_RXOM_TUNL			2
/* Fill RX_FIFO high word be higher sample bit */
#define	SUNXI_DAUDIO_RXOM_TUNH			3

/* SUNXI_DAUDIO_CLKDIV */
#define	SUNXI_DAUDIO_BCLK_DIV_MASK		0x7
#define	SUNXI_DAUDIO_BCLK_DIV_0			0
#define	SUNXI_DAUDIO_BCLK_DIV_1			1
#define	SUNXI_DAUDIO_BCLK_DIV_2			2
#define	SUNXI_DAUDIO_BCLK_DIV_3			3
#define	SUNXI_DAUDIO_BCLK_DIV_4			4
#define	SUNXI_DAUDIO_BCLK_DIV_5			5
#define	SUNXI_DAUDIO_BCLK_DIV_6			6
#define	SUNXI_DAUDIO_BCLK_DIV_7			7


/* Left Blank */
#define	SUNXI_DAUDIO_MCLK_DIV_MASK		0xF
#define	SUNXI_DAUDIO_MCLK_DIV_0			0
#define	SUNXI_DAUDIO_MCLK_DIV_1			1
#define	SUNXI_DAUDIO_MCLK_DIV_2			2
#define	SUNXI_DAUDIO_MCLK_DIV_3			3
#define	SUNXI_DAUDIO_MCLK_DIV_4			4
#define	SUNXI_DAUDIO_MCLK_DIV_5			5
#define	SUNXI_DAUDIO_MCLK_DIV_6			6
#define	SUNXI_DAUDIO_MCLK_DIV_7			7
#define	SUNXI_DAUDIO_MCLK_DIV_8			8
#define	SUNXI_DAUDIO_MCLK_DIV_9			9
#define	SUNXI_DAUDIO_MCLK_DIV_10		0xA
/* SUNXI_DAUDIO_CHCFG */
#ifdef SUNXI_DAUDIO_MODE_B
#define	SUNXI_DAUDIO_TX_SLOT_MASK		0XF
#define	SUNXI_DAUDIO_RX_SLOT_MASK		0XF
#else
#define	SUNXI_DAUDIO_TX_SLOT_MASK		7
#define	SUNXI_DAUDIO_RX_SLOT_MASK		7
#endif
/* SUNXI_DAUDIO_TXCHSEL: */
#define SUNXI_DAUDIO_TX_CHSEL_MASK		7

/* SUNXI_DAUDIO_RXCHSEL */
#define	SUNXI_DAUDIO_RX_CHSEL_MASK		7

#define	SND_SOC_DAIFMT_SIG_SHIFT		8
#define	SND_SOC_DAIFMT_MASTER_SHIFT		12

/* define for HDMI audio drq type number */
/*i2s0->daudio1, i2s1-> daudio2, i2s2-> daudio3*/
#define DRQDST_HDMI_TX		DRQDST_DAUDIO_2_TX

#define SUNXI_DAUDIO_BCLK			0
#define SUNXI_DAUDIO_LRCK			1
#define SUNXI_DAUDIO_MCLK			2
#define SUNXI_DAUDIO_GEN			3

/*to clear FIFO*/
#define SUNXI_DAUDIO_FTX_TIMES	1

extern int i2s_set_clk_onoff(struct snd_soc_dai *dai, u32 mask, u32 onoff);

#ifndef	CONFIG_SND_SUNXI_SOC_HDMI
static inline int sunxi_hdmi_codec_hw_params(
		struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

static inline int sunxi_hdmi_codec_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	return 0;
}

static inline void sunxi_hdmi_codec_shutdown(
		struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
}

#else	/* !CONFIG_SND_SUNXI_SOC_HDMI */
extern int sunxi_hdmi_codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai);
extern void sunxi_hdmi_codec_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai);
extern int sunxi_hdmi_codec_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai);
#endif	/* CONFIG_SND_SUNXI_SOC_HDMIAUDIO */

#endif	/* __SUNXI_I2S_H_ */
