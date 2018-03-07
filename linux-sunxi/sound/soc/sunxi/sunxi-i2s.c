/* sound\soc\sunxi\sunxi-daudio.c
 * (C) Copyright 2015-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wolfgang huang <huangjinhui@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <linux/delay.h>

#include "sunxi-i2s.h"
#include "sunxi-pcm.h"

#define	DRV_NAME	"sunxi-i2s"

#define	SUNXI_I2S_EXTERNAL_TYPE	1
#define	SUNXI_I2S_TDMHDMI_TYPE	2

#define SUNXI_I2S_DRQDST(sunxi_i2s, x)			\
	((sunxi_i2s)->playback_dma_param.dma_drq_type_num =	\
				DRQDST_DAUDIO_##x##_TX)
#define SUNXI_I2S_DRQSRC(sunxi_i2s, x)			\
	((sunxi_i2s)->capture_dma_param.dma_drq_type_num =	\
				DRQSRC_DAUDIO_##x##_RX)


struct sunxi_i2s_platform_data {
	unsigned int daudio_type;
	unsigned int external_type;
	unsigned int daudio_master;
	unsigned int pcm_lrck_period;
	unsigned int pcm_lrckr_period;
	unsigned int slot_width_select;
	unsigned int word_select_size;
	unsigned int slot_width;
	unsigned int slot_index;
	unsigned int pcm_lsb_first;
	unsigned int tx_data_mode;
	unsigned int rx_data_mode;
	unsigned int audio_format;
	unsigned int signal_inversion;
	unsigned int frame_type;
	unsigned int tdm_config;
	unsigned int tdm_num;
	unsigned int mclk_div;
	unsigned int over_sample_rate;
	unsigned int sample_resolution;
	unsigned int pcm_sync_period;
};

struct sunxi_i2s_info {
	struct device *dev;
	struct regmap *regmap;
	struct clk *pllclk;
	struct clk *moduleclk;
	struct mutex mutex;
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;
	struct sunxi_i2s_platform_data *pdata;
	unsigned int hub_mode;
	unsigned int hdmi_en;
};

static bool i2s_loop_en;
module_param(i2s_loop_en, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(i2s_loop_en,
		"SUNXI I2S audio loopback debug(Y=enable, N=disable)");

/*
*	Some codec on electric timing need debugging
*/
int i2s_set_clk_onoff(struct snd_soc_dai *dai, u32 mask, u32 onoff)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	switch (mask) {
	case SUNXI_DAUDIO_BCLK:
		break;
	case SUNXI_DAUDIO_LRCK:
		break;
	case SUNXI_DAUDIO_MCLK:
		if (onoff)
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (1<<MCLKOUT_EN));
		else
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (0<<MCLKOUT_EN));
		break;
	case SUNXI_DAUDIO_GEN:
		if (onoff)
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (1<<GLOBAL_EN));
		else
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (0<<GLOBAL_EN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(i2s_set_clk_onoff);


static int sunxi_i2s_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_i2s_info *sunxi_i2s =
					snd_soc_codec_get_drvdata(codec);
	unsigned int reg_val;

	regmap_read(sunxi_i2s->regmap, SUNXI_DAUDIO_FIFOCTL, &reg_val);

	ucontrol->value.integer.value[0] = ((reg_val & (1<<HUB_EN)) ? 2 : 1);
	return 0;
}

static int sunxi_i2s_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct sunxi_i2s_info *sunxi_i2s =
					snd_soc_codec_get_drvdata(codec);

	switch (ucontrol->value.integer.value[0]) {
	case	0:
	case	1:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<HUB_EN), (0<<HUB_EN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));
		break;
	case	2:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<HUB_EN), (1<<HUB_EN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const char *i2s_format_function[] = {"null",
			"hub_disable", "hub_enable"};
static const struct soc_enum i2s_format_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(i2s_format_function),
			i2s_format_function),
};

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_i2s_controls[] = {
	SOC_ENUM_EXT("sunxi i2s audio hub mode", i2s_format_enum[0],
		sunxi_i2s_get_hub_mode, sunxi_i2s_set_hub_mode),
};

static void sunxi_i2s_txctrl_enable(struct sunxi_i2s_info *sunxi_i2s,
					int enable)
{
	pr_debug("Enter %s, enable %d\n", __func__, enable);
	if (enable) {
		/* HDMI audio Transmit Clock just enable at startup */
		if (sunxi_i2s->pdata->daudio_type
			!= SUNXI_I2S_TDMHDMI_TYPE)
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_CTL,
					(1<<CTL_TXEN), (1<<CTL_TXEN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_INTCTL,
					(1<<TXDRQEN), (1<<TXDRQEN));
	} else {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_INTCTL,
					(1<<TXDRQEN), (0<<TXDRQEN));
		if (sunxi_i2s->pdata->daudio_type
			!= SUNXI_I2S_TDMHDMI_TYPE)
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_CTL,
					(1<<CTL_TXEN), (0<<CTL_TXEN));
	}
	pr_debug("End %s, enable %d\n", __func__, enable);
}

static void sunxi_i2s_rxctrl_enable(struct sunxi_i2s_info *sunxi_i2s,
					int enable)
{
	if (enable) {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_RXEN), (1<<CTL_RXEN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_INTCTL,
				(1<<RXDRQEN), (1<<RXDRQEN));
	} else {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_INTCTL,
				(1<<RXDRQEN), (0<<RXDRQEN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_RXEN), (0<<CTL_RXEN));
	}
}

static int sunxi_i2s_global_enable(struct sunxi_i2s_info *sunxi_i2s,
					int enable)
{
	if (enable) {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (1<<SDO0_EN));
		if (sunxi_i2s->hdmi_en) {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO1_EN), (1<<SDO1_EN));
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO2_EN), (1<<SDO2_EN));
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO3_EN), (1<<SDO3_EN));
		}
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (1<<GLOBAL_EN));
	} else {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<GLOBAL_EN), (0<<GLOBAL_EN));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (0<<SDO0_EN));
		if (sunxi_i2s->hdmi_en) {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO1_EN), (0<<SDO1_EN));
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO2_EN), (0<<SDO2_EN));
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_CTL, (1<<SDO3_EN), (0<<SDO3_EN));
		}
	}
	return 0;
}

static int sunxi_i2s_mclk_setting(struct sunxi_i2s_info *sunxi_i2s)
{
#if 0
	unsigned int mclk_div;

	if (sunxi_i2s->pdata->mclk_div) {

		switch (sunxi_i2s->pdata->mclk_div) {
		case	1:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_0;
			break;
		case	2:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_1;
			break;
		case	4:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_2;
			break;
		case	6:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_3;
			break;
		case	8:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_4;
			break;
		case	12:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_5;
			break;
		case	16:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_6;
			break;
		case	24:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_7;
			break;
		case	32:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_8;
			break;
		case	48:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_9;
			break;
		case	64:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_10;
			break;
		default:
			dev_err(sunxi_i2s->dev, "unsupport  mclk_div\n");
			return -EINVAL;
		}
		/* setting Mclk as external codec input clk */
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_MCLK_DIV_MASK<<MCLK_DIV),
			(mclk_div<<MCLK_DIV));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (1<<MCLKOUT_EN));
	} else {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CLKDIV,
				(1<<MCLKOUT_EN), (0<<MCLKOUT_EN));
	}
#endif
	return 0;
}

static int sunxi_i2s_init_fmt(struct sunxi_i2s_info *sunxi_i2s,
				unsigned int fmt)
{
	unsigned int offset, mode;
	unsigned int lrck_polarity, brck_polarity;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case	SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_MS_SEL_MASK << MS_SEL),
				(SUNXI_DAUDIO_MS_SEL_SLAVE << MS_SEL));
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_MS_SEL_MASK << MS_SEL),
				(SUNXI_DAUDIO_MS_SEL_MASTER << MS_SEL));
		break;
	default:
		dev_err(sunxi_i2s->dev, "unknown maser/slave format\n");
		return -EINVAL;
	}
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
		offset = SUNXI_DAUDIO_FMT_CTL_I2S;
		mode = SUNXI_DAUDIO_MODE_CTL_I2S;
		break;
	case	SND_SOC_DAIFMT_RIGHT_J:
		offset = SUNXI_DAUDIO_FMT_CTL_RIGHT;
		mode = SUNXI_DAUDIO_MODE_CTL_I2S;
		break;
	case	SND_SOC_DAIFMT_LEFT_J:
		offset = SUNXI_DAUDIO_FMT_CTL_LEFT;
		mode = SUNXI_DAUDIO_MODE_CTL_I2S;
		break;
	case	SND_SOC_DAIFMT_DSP_A:
		offset = SUNXI_DAUDIO_FMT_CTL_MASK;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
					(1 << LR_CLK_PARITY), (0 << LR_CLK_PARITY));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
					(1 << SHORT_SYNC_SEL),
					(SUNXI_DAUDIO_FMT_LONG_FRAME << SHORT_SYNC_SEL));
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		offset = SUNXI_DAUDIO_FMT_CTL_MASK;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
					(1 << LR_CLK_PARITY), (1 << LR_CLK_PARITY));
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
					(1 << SHORT_SYNC_SEL),
					(SUNXI_DAUDIO_FMT_SHORT_FRAME << SHORT_SYNC_SEL));
		break;
	default:
		dev_err(sunxi_i2s->dev, "format setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
			(SUNXI_DAUDIO_MODE_CTL_MASK << PCM_INTERFACE),
			(mode << PCM_INTERFACE));
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_FMT_CTL_MASK << SD_FMT),
			(offset << SD_FMT));

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case	SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	case	SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	default:
		dev_err(sunxi_i2s->dev, "invert clk setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
			(1 << LR_CLK_PARITY), (lrck_polarity << LR_CLK_PARITY));
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
			(1 << BCLK_PARITY), (brck_polarity << BCLK_PARITY));
	return 0;
}

static int sunxi_i2s_init(struct sunxi_i2s_info *sunxi_i2s)
{
	struct sunxi_i2s_platform_data *pdata = sunxi_i2s->pdata;
	/*
	 * MSB on the transmit format, always be first.
	 * default using Linear-PCM, without no companding.
	 * A-law<Eourpean standard> or U-law<US-Japan> not working ok.
	 */
/*
	regmap_write(sunxi_i2s->regmap,
			SUNXI_DAUDIOs_FMT1, SUNXI_DAUDIO_FMT1_DEF);*/
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
				(1 << SHORT_SYNC_SEL),
				(pdata->frame_type << SHORT_SYNC_SEL));

	sunxi_i2s_init_fmt(sunxi_i2s, (pdata->audio_format
		| (pdata->signal_inversion << SND_SOC_DAIFMT_SIG_SHIFT)
		| (pdata->daudio_master << SND_SOC_DAIFMT_MASTER_SHIFT)));

	return sunxi_i2s_mclk_setting(sunxi_i2s);
}

static int sunxi_i2s_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_card *card = rtd->card;
	struct sunxi_hdmi_priv *sunxi_hdmi = snd_soc_card_get_drvdata(card);
	unsigned int reg_val;


	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		/*
		 * Special procesing for hdmi, HDMI card name is
		 * "sndhdmi" or sndhdmiraw. if card not HDMI,
		 * strstr func just return NULL, jump to right section.
		 * Not HDMI card, sunxi_hdmi maybe a NULL pointer.
		 */
		if (sunxi_i2s->pdata->daudio_type ==
				SUNXI_I2S_TDMHDMI_TYPE
				&& (sunxi_hdmi->hdmi_format > 1)) {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK << SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT << SAMPLE_RESOLUTION));
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_MSB << TXIM));
		} else {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK << SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_16BIT << SAMPLE_RESOLUTION));
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
			else
				regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
		}
		break;
	case	SNDRV_PCM_FORMAT_S20_3LE:
	case	SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK << SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT << SAMPLE_RESOLUTION));
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
		else
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK << SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT << SAMPLE_RESOLUTION));
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
		else
			regmap_update_bits(sunxi_i2s->regmap,
					SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
		break;
	default:
		dev_err(sunxi_i2s->dev, "unrecognized format\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_TXCHSEL,
				(SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL),
				((params_channels(params)-1) << TX_CHSEL));
		if (sunxi_i2s->hdmi_en == 0) {
			if (params_channels(params) == 1) {
				regmap_write(sunxi_i2s->regmap,
						SUNXI_DAUDIO_TXCHMAP, 0x76543200);
			} else {
				regmap_write(sunxi_i2s->regmap,
						SUNXI_DAUDIO_TXCHMAP, 0x76542310);
			}
		} else {
			/* HDMI multi-channel processing */
			if (params_channels(params) == 1) {
				regmap_write(sunxi_i2s->regmap,
						SUNXI_DAUDIO_TXCHMAP, 0x76543200);
			} else if (params_channels(params) == 8) {
				regmap_write(sunxi_i2s->regmap,
						SUNXI_DAUDIO_TXCHMAP, 0x54762310);
			} else {
				regmap_write(sunxi_i2s->regmap,
						SUNXI_DAUDIO_TXCHMAP, 0x76542310);
			}
		}
	} else {
		regmap_write(sunxi_i2s->regmap,
			SUNXI_DAUDIO_RXCHMAP, SUNXI_DEFAULT_RXCHMAP);
		regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_RXCHSEL,
				(SUNXI_DAUDIO_RX_CHSEL_MASK << RX_CHSEL),
				((params_channels(params)-1) << RX_CHSEL));
	}

	/* Special processing for HDMI hub playback to enable hdmi module */
	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE) {
		mutex_lock(&sunxi_i2s->mutex);
		regmap_read(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FIFOCTL, &reg_val);
		sunxi_i2s->hub_mode = (reg_val & (1<<HUB_EN));
		if (sunxi_i2s->hub_mode) {
			sunxi_hdmi_codec_hw_params(substream, params, NULL);
			sunxi_hdmi_codec_prepare(substream, NULL);
		}
		mutex_unlock(&sunxi_i2s->mutex);
	}

	return 0;
}

static int sunxi_i2s_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	sunxi_i2s_init_fmt(sunxi_i2s, fmt);
	return 0;
}

static int sunxi_i2s_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	if (clk_set_rate(sunxi_i2s->pllclk, freq)) {
		dev_err(sunxi_i2s->dev, "set pllclk rate failed\n");
		printk("set sysclk error");
		return -EBUSY;
	}
	return 0;
}

static int sunxi_i2s_set_clkdiv(struct snd_soc_dai *dai,
				int clk_id, int sample_rate)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int bclk_div, mclk_div;
	unsigned int mclk;
	unsigned int word_select_size;
	/*unsigned int sample_resolution = sunxi_i2s->pdata->sample_resolution;*/

	mclk = sunxi_i2s->pdata->over_sample_rate;

	if (sunxi_i2s->pdata->tdm_config) {
		/* I2S/TDM two channel mode */
		switch (sample_rate) {
		case 8000: {
				switch (mclk) {
				case 128:
								mclk_div = 24;
								break;
				case 192:
								mclk_div = 16;
								break;
				case 256:
								mclk_div = 12;
								break;
				case 384:
								mclk_div = 8;
								break;
				case 512:
								mclk_div = 6;
								break;
				case 768:
								mclk_div = 4;
								break;
				}
				break;
		}

		case 16000: {
				switch (mclk) {
				case 128:
								mclk_div = 12;
								break;
				case 192:
								mclk_div = 8;
								break;
				case 256:
								mclk_div = 6;
								break;
				case 384:
								mclk_div = 4;
								break;
				case 768:
								mclk_div = 2;
								break;
				}
				break;
		}

		case 32000: {
				switch (mclk) {
				case 128:
								mclk_div = 6;
								break;
				case 192:
								mclk_div = 4;
								break;
				case 384:
								mclk_div = 2;
								break;
				case 768:
								mclk_div = 1;
								break;
				}
				break;
		}

		case 64000: {
				switch (mclk) {
				case 192:
								mclk_div = 2;
								break;
				case 384:
								mclk_div = 1;
								break;
				}
				break;
		}

		case 128000: {
/* if it is to be supported, you need addtional configuration.*/
				switch (mclk) {
				case 192:
								mclk_div = 1;
								break;
				}
				break;
		}

		case 11025:
		case 12000: {
				switch (mclk) {
				case 128:
								mclk_div = 16;
								break;
				case 256:
								mclk_div = 8;
								break;
				case 512:
								mclk_div = 4;
								break;
				}
				break;
		}

		case 22050:
		case 24000: {
				switch (mclk) {
				case 128:
								mclk_div = 8;
								break;
				case 256:
								mclk_div = 4;
								break;
				case 512:
								mclk_div = 2;
								break;
				}
				break;
		}

		case 44100:
		case 48000: {
				switch (mclk) {
				case 128:
								mclk_div = 4;
								break;
				case 256:
								mclk_div = 2;
								break;
				case 512:
								mclk_div = 1;
								break;
				}
				break;
		}

		case 88200:
		case 96000: {
				switch (mclk) {
				case 128:
								mclk_div = 2;
								break;
				case 256:
								mclk_div = 1;
								break;
				}
				break;
		}

		case 176400:
		case 192000: {
								mclk_div = 1;
								break;
		}

		}

/*bclk div caculate*/
		bclk_div = mclk/(2 * sunxi_i2s->pdata->word_select_size);

	} else {
		/* PCM mode */
		mclk_div = 2;
		bclk_div = 6;
	}

/*calculate MCLK Divide Ratio*/
	switch (mclk_div) {
	case 1:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_0;
				break;
	case 2:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_1;
				break;
	case 4:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_2;
				break;
	case 6:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_3;
				break;
	case 8:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_4;
				break;
	case 12:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_5;
				break;
	case 16:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_6;
				break;
	case 24:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_7;
				break;
	case 32:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_8;
				break;
	case 48:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_9;
				break;
	case 64:
				mclk_div = SUNXI_DAUDIO_MCLK_DIV_10;
				break;
	default:
		dev_err(sunxi_i2s->dev, "unsupport mclk_div\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_MCLK_DIV_MASK<<MCLK_DIV),
			(mclk_div<<MCLK_DIV));

/* word_select_size configuration */
	switch (sunxi_i2s->pdata->word_select_size) {
	case 16:
			word_select_size = SUNXI_DAUDIO_WORD_SIZE_16BIT;
	case 20:
			word_select_size = SUNXI_DAUDIO_WORD_SIZE_20BIT;
		break;
	case 24:
			word_select_size = SUNXI_DAUDIO_WORD_SIZE_24BIT;
		break;
	case 32:
			word_select_size = SUNXI_DAUDIO_WORD_SIZE_32BIT;
		break;
	default:
		dev_err(sunxi_i2s->dev, "word_select_size setting failed\n");
		return -EINVAL;
	}
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_WORD_SIZE_MASK << WORD_SEL_SIZE),
			word_select_size << WORD_SEL_SIZE);

/*calculate BCLK Divide Ratio*/
	switch (bclk_div) {
	case 2:
				bclk_div = 0;
				break;
	case 4:
				bclk_div = 1;
				break;
	case 6:
				bclk_div = 2;
				break;
	case 8:
				bclk_div = 3;
				break;
	case 12:
				bclk_div = 4;
				break;
	case 16:
				bclk_div = 5;
				break;
	case 32:
				bclk_div = 6;
				break;
	case 64:
				bclk_div = 7;
				break;
	default:
		dev_err(sunxi_i2s->dev, "unsupport bclk_div\n");
		return -EINVAL;
	}
/* setting bclk to driver external codec bit clk */
	regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_BCLK_DIV_MASK<<BCLK_DIV),
			(bclk_div<<BCLK_DIV));
	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_EXTERNAL_TYPE) {
/*set the tx/rx_data_mode*/
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
			(SUNXI_DAUDIO_RX_DATA_MODE_MASK << RX_PDM),
			sunxi_i2s->pdata->rx_data_mode << RX_PDM);
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
			(SUNXI_DAUDIO_TX_DATA_MODE_MASK << TX_PDM),
			sunxi_i2s->pdata->tx_data_mode << TX_PDM);

		if (sunxi_i2s->pdata->pcm_lsb_first) {
/* lsb first */
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT1, 1 << MLS_SEL, 1);
		} else {
/* msb first */
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT1, 1 << MLS_SEL, 0);
		}

		if (sunxi_i2s->pdata->slot_width == 16) {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT1, 1 << SLOT_WIDTH, 1 << SLOT_WIDTH);
		}

		if (!(sunxi_i2s->pdata->slot_index)) {
			regmap_update_bits(sunxi_i2s->regmap,
				SUNXI_DAUDIO_FMT1, 1 << SLOT_INDEX, 0 << SLOT_INDEX);
		}

		if (sunxi_i2s->pdata->pcm_sync_period == 256) {
			regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
				SUNXI_DAUDIO_PCM_SYNC_PERIOD_MASK << PCM_SYNC_PERIOD,
				0x04 << PCM_SYNC_PERIOD);
		} else if (sunxi_i2s->pdata->pcm_sync_period == 128) {
			regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
				SUNXI_DAUDIO_PCM_SYNC_PERIOD_MASK << PCM_SYNC_PERIOD,
				0x03 << PCM_SYNC_PERIOD);
		} else if (sunxi_i2s->pdata->pcm_sync_period == 64) {
			regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
				SUNXI_DAUDIO_PCM_SYNC_PERIOD_MASK << PCM_SYNC_PERIOD,
					0x02 << PCM_SYNC_PERIOD);
		} else if (sunxi_i2s->pdata->pcm_sync_period == 32) {
			regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1,
				SUNXI_DAUDIO_PCM_SYNC_PERIOD_MASK << PCM_SYNC_PERIOD,
				0x01 << PCM_SYNC_PERIOD);
		} else {
			dev_err(sunxi_i2s->dev, "pcm_sync_period setting failed\n");
			return -EINVAL;
		}

/* signext */
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FMT1, 1 << SEXT, 0);
	}

	return 0;
}

static int sunxi_i2s_dai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	/* FIXME: As HDMI module to play audio, it need at least 1100ms to sync.
	 * if we not wait we lost audio data to playback, or we wait for 1100ms
	 * to playback, user experience worst than you can imagine. So we need
	 * to cutdown that sync time by keeping clock signal on. we just enable
	 * it at startup and resume, cutdown it at remove and suspend time.
	 */
	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_i2s->playback_dma_param);
	else
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_i2s->capture_dma_param);

	return 0;
}

static int sunxi_i2s_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (i2s_loop_en)
				regmap_update_bits(sunxi_i2s->regmap,
						SUNXI_DAUDIO_CTL,
						(1<<LOOP_EN), (1<<LOOP_EN));
			else
				regmap_update_bits(sunxi_i2s->regmap,
						SUNXI_DAUDIO_CTL,
						(1<<LOOP_EN), (0<<LOOP_EN));
			sunxi_i2s_txctrl_enable(sunxi_i2s, 1);
		} else {
			sunxi_i2s_rxctrl_enable(sunxi_i2s, 1);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sunxi_i2s_txctrl_enable(sunxi_i2s, 0);
		else
			sunxi_i2s_rxctrl_enable(sunxi_i2s, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_i2s_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	unsigned int i;
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0 ; i < SUNXI_DAUDIO_FTX_TIMES ; i++) {
			regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FTX), (1<<FIFO_CTL_FTX));
			mdelay(1);
		}
		regmap_write(sunxi_i2s->regmap, SUNXI_DAUDIO_TXCNT, 0);
	} else {
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FRX), (1<<FIFO_CTL_FRX));
		regmap_write(sunxi_i2s->regmap, SUNXI_DAUDIO_RXCNT, 0);
	}

	return 0;
}

static int sunxi_i2s_probe(struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	mutex_init(&sunxi_i2s->mutex);

	sunxi_i2s_init(sunxi_i2s);
	return 0;
}

static void sunxi_i2s_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	/* Special processing for HDMI hub playback to shutdown hdmi module */
	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE) {
		mutex_lock(&sunxi_i2s->mutex);
		if (sunxi_i2s->hub_mode)
			sunxi_hdmi_codec_shutdown(substream, NULL);
		mutex_unlock(&sunxi_i2s->mutex);
	}
}

static int sunxi_i2s_remove(struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);

	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));
	return 0;
}

static int sunxi_i2s_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	pr_debug("[i2s] suspend .%s\n", dev_name(sunxi_i2s->dev));

	/* Global disable I2S/TDM module */
	sunxi_i2s_global_enable(sunxi_i2s, 0);

	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));

	clk_disable_unprepare(sunxi_i2s->moduleclk);
	clk_disable_unprepare(sunxi_i2s->pllclk);

	if (sunxi_i2s->pdata->external_type) {
		ret = pinctrl_select_state(sunxi_i2s->pinctrl,
				sunxi_i2s->pinstate_sleep);
		if (ret) {
			pr_warn("[i2s]select pin sleep state failed\n");
			return ret;
		}
		devm_pinctrl_put(sunxi_i2s->pinctrl);
	}

	return 0;
}

static int sunxi_i2s_resume(struct snd_soc_dai *dai)
{
	struct sunxi_i2s_info *sunxi_i2s = snd_soc_dai_get_drvdata(dai);
	int ret;

	pr_debug("[i2s] resume .%s\n", dev_name(sunxi_i2s->dev));

	if (clk_prepare_enable(sunxi_i2s->pllclk)) {
		dev_err(sunxi_i2s->dev, "pllclk resume failed\n");
		ret = -EBUSY;
		goto err_resume_out;
	}

	if (clk_prepare_enable(sunxi_i2s->moduleclk)) {
		dev_err(sunxi_i2s->dev, "moduleclk resume failed\n");
		ret = -EBUSY;
		goto err_pllclk_disable;
	}

	if (sunxi_i2s->pdata->external_type) {
		sunxi_i2s->pinctrl = devm_pinctrl_get(sunxi_i2s->dev);
		if (IS_ERR_OR_NULL(sunxi_i2s)) {
			dev_err(sunxi_i2s->dev, "pinctrl resume get fail\n");
			ret = -ENOMEM;
			goto err_moduleclk_disable;
		}

		sunxi_i2s->pinstate = pinctrl_lookup_state(
				sunxi_i2s->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_i2s->pinstate)) {
			dev_err(sunxi_i2s->dev,
				"pinctrl default state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		sunxi_i2s->pinstate_sleep = pinctrl_lookup_state(
				sunxi_i2s->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_i2s->pinstate_sleep)) {
			dev_err(sunxi_i2s->dev,
				"pinctrl sleep state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		ret = pinctrl_select_state(sunxi_i2s->pinctrl,
					sunxi_i2s->pinstate);
		if (ret)
			dev_warn(sunxi_i2s->dev,
				"daudio set pinctrl default state fail\n");
	}

	sunxi_i2s_init(sunxi_i2s);

	/* Global enable I2S/TDM module */
	sunxi_i2s_global_enable(sunxi_i2s, 1);

	if (sunxi_i2s->pdata->daudio_type == SUNXI_I2S_TDMHDMI_TYPE)
		regmap_update_bits(sunxi_i2s->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));

	return 0;

err_pinctrl_put:
	devm_pinctrl_put(sunxi_i2s->pinctrl);
err_moduleclk_disable:
	clk_disable_unprepare(sunxi_i2s->moduleclk);
err_pllclk_disable:
	clk_disable_unprepare(sunxi_i2s->pllclk);
err_resume_out:
	return ret;
}

#define	SUNXI_I2S_RATES	(SNDRV_PCM_RATE_8000_192000 \
				| SNDRV_PCM_RATE_KNOT)

static struct snd_soc_dai_ops sunxi_i2s_dai_ops = {
	.hw_params = sunxi_i2s_hw_params,
	.set_sysclk = sunxi_i2s_set_sysclk,
	.set_clkdiv = sunxi_i2s_set_clkdiv,
	.set_fmt = sunxi_i2s_set_fmt,
	.startup = sunxi_i2s_dai_startup,
	.trigger = sunxi_i2s_trigger,
	.prepare = sunxi_i2s_prepare,
	.shutdown = sunxi_i2s_shutdown,
};

static struct snd_soc_dai_driver sunxi_i2s_dai = {
	.probe = sunxi_i2s_probe,
	.suspend = sunxi_i2s_suspend,
	.resume = sunxi_i2s_resume,
	.remove = sunxi_i2s_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_i2s_dai_ops,
};

static const struct snd_soc_component_driver sunxi_i2s_component = {
	.name		= DRV_NAME,
	.controls	= sunxi_i2s_controls,
	.num_controls	= ARRAY_SIZE(sunxi_i2s_controls),
};

static struct sunxi_i2s_platform_data sunxi_i2s = {
	.daudio_type = SUNXI_I2S_EXTERNAL_TYPE,
	.external_type = 1,
};

static struct sunxi_i2s_platform_data sunxi_tdmhdmi = {
	.daudio_type = SUNXI_I2S_TDMHDMI_TYPE,
	.external_type = 0,
	.audio_format = 1,
	.signal_inversion = 1,
	.daudio_master = 4,
	.pcm_lrck_period = 32,
	.pcm_lrckr_period = 1,
	.slot_width_select = 32,
	.over_sample_rate = 128,
	.tx_data_mode = 0,
	.rx_data_mode = 0,
	.tdm_config = 1,
	.sample_resolution = 16,
	.word_select_size = 32,
	.mclk_div = 0,
};

static const struct of_device_id sunxi_i2s_of_match[] = {
	{
		.compatible = "allwinner,sunxi-i2s",
		.data = &sunxi_i2s,
	},
	{
		.compatible = "allwinner,sunxi-tdmhdmi",
		.data = &sunxi_tdmhdmi,
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_i2s_of_match);

static const struct regmap_config sunxi_i2s_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_RXCHMAP,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_i2s_dev_probe(struct platform_device *pdev)
{
	struct resource res, *memregion;
	const struct of_device_id *match;
	void __iomem *sunxi_i2s_membase;
	struct sunxi_i2s_info *sunxi_i2s;
	struct device_node *np = pdev->dev.of_node;
	unsigned int temp_val;
	int ret;

	match = of_match_device(sunxi_i2s_of_match, &pdev->dev);
	if (match) {
		sunxi_i2s = devm_kzalloc(&pdev->dev,
					sizeof(struct sunxi_i2s_info),
					GFP_KERNEL);
		if (!sunxi_i2s) {
			dev_err(&pdev->dev, "alloc sunxi_i2s failed\n");
			ret = -ENOMEM;
			goto err_node_put;
		}
		dev_set_drvdata(&pdev->dev, sunxi_i2s);
		sunxi_i2s->dev = &pdev->dev;

		sunxi_i2s->pdata = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_i2s_platform_data),
				GFP_KERNEL);
		if (!sunxi_i2s->pdata) {
			dev_err(&pdev->dev,
				"alloc sunxi i2s platform data failed\n");
			ret = -ENOMEM;
			goto err_devm_kfree;
		}

		memcpy(sunxi_i2s->pdata, match->data,
			sizeof(struct sunxi_i2s_platform_data));
	} else {
		dev_err(&pdev->dev, "node match failed\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&pdev->dev, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	memregion = devm_request_mem_region(&pdev->dev, res.start,
					resource_size(&res), DRV_NAME);
	if (!memregion) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	sunxi_i2s_membase = ioremap(res.start, resource_size(&res));
	if (!sunxi_i2s_membase) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_kfree;
	}

	sunxi_i2s->regmap = devm_regmap_init_mmio(&pdev->dev,
					sunxi_i2s_membase,
					&sunxi_i2s_regmap_config);
	if (IS_ERR(sunxi_i2s->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = PTR_ERR(sunxi_i2s->regmap);
		goto err_iounmap;
	}

	sunxi_i2s->pllclk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_i2s->pllclk)) {
		dev_err(&pdev->dev, "pllclk get failed\n");
		ret = PTR_ERR(sunxi_i2s->pllclk);
		goto err_iounmap;
	}

	sunxi_i2s->moduleclk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(sunxi_i2s->moduleclk)) {
		dev_err(&pdev->dev, "moduleclk get failed\n");
		ret = PTR_ERR(sunxi_i2s->moduleclk);
		goto err_pllclk_put;
	}

	if (clk_set_parent(sunxi_i2s->moduleclk, sunxi_i2s->pllclk)) {
		dev_err(&pdev->dev, "set parent of moduleclk to pllclk fail\n");
		ret = -EBUSY;
		goto err_moduleclk_put;
	}
	clk_prepare_enable(sunxi_i2s->pllclk);
	clk_prepare_enable(sunxi_i2s->moduleclk);

	if (sunxi_i2s->pdata->external_type) {
		sunxi_i2s->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(sunxi_i2s->pinctrl)) {
			dev_err(&pdev->dev, "pinctrl get failed\n");
			ret = -EINVAL;
			goto err_moduleclk_put;
		}

		sunxi_i2s->pinstate = pinctrl_lookup_state(
				sunxi_i2s->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(sunxi_i2s->pinstate)) {
			dev_err(&pdev->dev, "pinctrl default state get fail\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}

		sunxi_i2s->pinstate_sleep = pinctrl_lookup_state(
				sunxi_i2s->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(sunxi_i2s->pinstate_sleep)) {
			dev_err(&pdev->dev, "pinctrl sleep state get failed\n");
			ret = -EINVAL;
			goto err_pinctrl_put;
		}
	}

	switch (sunxi_i2s->pdata->daudio_type) {
	case	SUNXI_I2S_EXTERNAL_TYPE:
		ret = of_property_read_u32(np, "tdm_num", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm configuration missing\n");
			/*
			 * warnning just continue,
			 * making tdm_num as default setting
			 */
			sunxi_i2s->pdata->tdm_num = 0;
		} else {
			/*
			 * FIXME, for channel number mess,
			 * so just not check channel overflow
			 */
			sunxi_i2s->pdata->tdm_num = temp_val;
		}

		sunxi_i2s->playback_dma_param.dma_addr =
					res.start + SUNXI_DAUDIO_TXFIFO;
		sunxi_i2s->playback_dma_param.src_maxburst = 4;
		sunxi_i2s->playback_dma_param.dst_maxburst = 4;

		sunxi_i2s->capture_dma_param.dma_addr =
					res.start + SUNXI_DAUDIO_RXFIFO;
		sunxi_i2s->capture_dma_param.src_maxburst = 4;
		sunxi_i2s->capture_dma_param.dst_maxburst = 4;


		switch (sunxi_i2s->pdata->tdm_num) {
		case 0:
			/*i2s0*/
			SUNXI_I2S_DRQDST(sunxi_i2s, 0);
			SUNXI_I2S_DRQSRC(sunxi_i2s, 0);
			break;
		case 1:
			/*i2s1*/
			SUNXI_I2S_DRQDST(sunxi_i2s, 1);
			SUNXI_I2S_DRQSRC(sunxi_i2s, 1);
			break;
		default:
			dev_warn(&pdev->dev, "tdm_num setting overflow\n");
			break;
		}

		ret = of_property_read_u32(np, "daudio_master", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "daudio_master configuration missing or invalid\n");
			/*
			 * default setting SND_SOC_DAIFMT_CBS_CFS mode
			 * codec clk & FRM slave
			 */
			sunxi_i2s->pdata->daudio_master = 4;
		} else {
			sunxi_i2s->pdata->daudio_master = temp_val;
		}

		ret = of_property_read_u32(np, "pcm_lrck_period", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "pcm_lrck_period configuration missing or invalid\n");
			sunxi_i2s->pdata->pcm_lrck_period = 0;
		} else {
			sunxi_i2s->pdata->pcm_lrck_period = temp_val;
		}

		ret = of_property_read_u32(np, "slot_width", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "slot_width_select configuration missing or invalid\n");
			sunxi_i2s->pdata->slot_width = 0;
		} else {
			sunxi_i2s->pdata->slot_width = temp_val;
		}

		ret = of_property_read_u32(np, "slot_index", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "slot_index configuration missing or invalid\n");
			sunxi_i2s->pdata->slot_index = 0;
		} else {
			sunxi_i2s->pdata->slot_index = temp_val;
		}


		ret = of_property_read_u32(np, "pcm_lsb_first", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "pcm_lsb_first config missing or invalid\n");
			sunxi_i2s->pdata->pcm_lsb_first = 1;
		} else {
			sunxi_i2s->pdata->pcm_lsb_first = temp_val;
			dev_dbg(&pdev->dev, "pcm_lsb_first:%d\n", temp_val);
		}

		ret = of_property_read_u32(np, "tx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tx_data_mode configuration missing or invalid\n");
			sunxi_i2s->pdata->tx_data_mode = 0;
		} else {
			sunxi_i2s->pdata->tx_data_mode = temp_val;
			dev_dbg(&pdev->dev, "tx_data_mode:%d\n", temp_val);
		}

		ret = of_property_read_u32(np, "rx_data_mode", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "rx_data_mode configuration missing or invalid\n");
			sunxi_i2s->pdata->rx_data_mode = 0;
		} else {
			sunxi_i2s->pdata->rx_data_mode = temp_val;
			dev_dbg(&pdev->dev, "rx_data_mode:%d\n", temp_val);
		}

		ret = of_property_read_u32(np, "audio_format", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "audio_format configuration missing or invalid\n");
			sunxi_i2s->pdata->audio_format = 1;
		} else {
			sunxi_i2s->pdata->audio_format = temp_val;
		}

		ret = of_property_read_u32(np, "signal_inversion", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "signal_inversion configuration missing or invalid\n");
			sunxi_i2s->pdata->signal_inversion = 1;
		} else {
			sunxi_i2s->pdata->signal_inversion = temp_val;
		}

		ret = of_property_read_u32(np, "pcm_sync_period", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "pcm_sync_period configuration missing or invalid\n");
			sunxi_i2s->pdata->pcm_sync_period = 64;
		} else {
			sunxi_i2s->pdata->pcm_sync_period = temp_val;
		}

		ret = of_property_read_u32(np, "frametype", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "frametype configuration missing or invalid\n");
			sunxi_i2s->pdata->frame_type = 0;
		} else {
			sunxi_i2s->pdata->frame_type = temp_val;
		}

		ret = of_property_read_u32(np, "word_select_size", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "word_select_size configuration missing or invalid\n");
			sunxi_i2s->pdata->word_select_size = 32;
		} else {
			sunxi_i2s->pdata->word_select_size = temp_val;
		}
		ret = of_property_read_u32(np, "tdm_config", &temp_val);
		if (ret < 0) {
			dev_warn(&pdev->dev, "tdm_config configuration missing or invalid\n");
			sunxi_i2s->pdata->tdm_config = 1;
		} else {
			sunxi_i2s->pdata->tdm_config = temp_val;
		}

		ret = of_property_read_u32(np, "mclk_div", &temp_val);
		if (ret < 0)
			sunxi_i2s->pdata->mclk_div = 0;
		else
			sunxi_i2s->pdata->mclk_div = temp_val;

		ret = of_property_read_u32(np, "over_sample_rate", &temp_val);
		if (ret < 0) {
			sunxi_i2s->pdata->over_sample_rate = 128;
		} else {
			sunxi_i2s->pdata->over_sample_rate = temp_val;
		}

		ret = of_property_read_u32(np, "sample_resolution", &temp_val);
		if (ret < 0) {
			sunxi_i2s->pdata->sample_resolution = 16;
		} else {
			sunxi_i2s->pdata->sample_resolution = temp_val;
		}

		break;

	case	SUNXI_I2S_TDMHDMI_TYPE:
		sunxi_i2s->playback_dma_param.dma_addr =
				res.start + SUNXI_DAUDIO_TXFIFO;
		sunxi_i2s->playback_dma_param.dma_drq_type_num =
					DRQDST_HDMI_TX;
		sunxi_i2s->playback_dma_param.src_maxburst = 8;
		sunxi_i2s->playback_dma_param.dst_maxburst = 8;
		sunxi_i2s->hdmi_en = 1;
		break;
	default:
		dev_err(&pdev->dev, "missing digital audio type\n");
		ret = -EINVAL;
		goto err_devm_kfree;
	}

	ret = snd_soc_register_component(&pdev->dev, &sunxi_i2s_component,
					&sunxi_i2s_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "component register failed\n");
		ret = -ENOMEM;
		goto err_pinctrl_put;
	}

	switch (sunxi_i2s->pdata->daudio_type) {
	case	SUNXI_I2S_EXTERNAL_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev, 0);
		printk("platform has been register!\n");
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_unregister_component;
		}
		break;
	case	SUNXI_I2S_TDMHDMI_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev,
				SND_DMAENGINE_PCM_FLAG_NO_DT);
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_unregister_component;
		}
		break;
	default:
		dev_err(&pdev->dev, "missing i2s audio type\n");
		ret = -EINVAL;
		goto err_unregister_component;
	}

	sunxi_i2s_global_enable(sunxi_i2s, 1);

	return 0;

err_unregister_component:
	snd_soc_unregister_component(&pdev->dev);
err_pinctrl_put:
	devm_pinctrl_put(sunxi_i2s->pinctrl);
err_moduleclk_put:
	clk_put(sunxi_i2s->moduleclk);
err_pllclk_put:
	clk_put(sunxi_i2s->pllclk);
err_iounmap:
	iounmap(sunxi_i2s_membase);
err_devm_kfree:
	devm_kfree(&pdev->dev, sunxi_i2s);
err_node_put:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_i2s_dev_remove(struct platform_device *pdev)
{
	struct sunxi_i2s_info *sunxi_i2s = dev_get_drvdata(&pdev->dev);

	snd_soc_unregister_component(&pdev->dev);
	clk_put(sunxi_i2s->moduleclk);
	clk_put(sunxi_i2s->pllclk);
	devm_kfree(&pdev->dev, sunxi_i2s);
	return 0;
}

static struct platform_driver sunxi_i2s_driver = {
	.probe = sunxi_i2s_dev_probe,
	.remove = __exit_p(sunxi_i2s_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_i2s_of_match,
	},
};

module_platform_driver(sunxi_i2s_driver);

MODULE_AUTHOR("wolfgang huang <huangjinhui@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI I2S DAI AUDIO ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-i2s");
