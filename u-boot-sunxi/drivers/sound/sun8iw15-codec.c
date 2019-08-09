/*
 * (C) Copyright 2014-2019
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * guoyingyang <guoyingyang@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <sys_config.h>
#include <asm/global_data.h>
#include <asm/arch/dma.h>

#include "sun8iw15-codec.h"
#include "sunxi_rw_func.h"

DECLARE_GLOBAL_DATA_PTR;

struct sunxi_codec {
	void __iomem *addr_clkbase;
	void __iomem *addr_dbase;
	void __iomem *addr_abase;
	user_gpio_set_t spk_cfg;
	u32 headphone_vol;
	u32 spk_handle;
	u32 pa_ctl_level;
};

static struct sunxi_codec g_codec;

struct aif1_fs {
	unsigned int samplerate;
	int aif1_bclk_div;
	int aif1_srbit;
};

struct aif1_lrck {
	int aif1_lrlk_div;
	int aif1_lrlk_bit;
};

struct aif1_word_size {
	int aif1_wsize_val;
	int aif1_wsize_bit;
};


static u32 sunxi_codec_read(struct sunxi_codec *codec, u32 reg)
{
	u32 val;
	if (reg >= ANALOG_FLAG) {
		reg = reg - ANALOG_FLAG;
		val = read_prcm_wvalue(reg, codec->addr_abase);
	} else
		val = codec_rdreg(codec->addr_dbase + reg);
	return val;
}

static int sunxi_codec_write(struct sunxi_codec *codec, u32 reg, u32 val)
{
	if (reg >= ANALOG_FLAG) {
		/* Analog part */
		reg = reg - ANALOG_FLAG;
		write_prcm_wvalue(reg, val, codec->addr_abase);
	} else {
		codec_wrreg(codec->addr_dbase + reg, val);
	}
	return 0;
}

static int sunxi_codec_update_bits(struct sunxi_codec *codec, u32 reg, u32 mask,
				   u32 val)
{
	if (reg >= ANALOG_FLAG) {
		/* Analog part */
		reg = reg - ANALOG_FLAG;
		codec_wrreg_prcm_bits(codec->addr_abase, reg, mask, val);
	} else {
		codec_wrreg_bits(codec->addr_dbase + reg, mask, val);
	}

	return 0;
}

static struct label reg_labels[] = {
	LABEL(SUNXI_DA_CTL),
	LABEL(SUNXI_DA_FAT0),
	LABEL(SUNXI_DA_FAT1),
	LABEL(SUNXI_DA_FCTL),
	LABEL(SUNXI_DA_INT),
	LABEL(SUNXI_DA_CLKD),
	LABEL(SUNXI_DA_TXCNT),
	LABEL(SUNXI_DA_RXCNT),
	LABEL(SUNXI_DA_TXCHSEL),
	LABEL(SUNXI_DA_TXCHMAP),
	LABEL(SUNXI_DA_RXCHSEL),
	LABEL(SUNXI_DA_RXCHMAP),
	LABEL(SUNXI_SYSCLK_CTL),
	LABEL(SUNXI_MOD_CLK_ENA),
	LABEL(SUNXI_MOD_RST_CTL),
	LABEL(SUNXI_SYS_SR_CTRL),
	/*LABEL(SUNXI_SYS_SRC_CLK),*/
	LABEL(SUNXI_SYS_DVC_MOD),

	LABEL(SUNXI_AIF1_CLK_CTRL),
	LABEL(SUNXI_AIF1_ADCDAT_CTRL),
	LABEL(SUNXI_AIF1_DACDAT_CTRL),
	LABEL(SUNXI_AIF1_MXR_SRC),
	LABEL(SUNXI_AIF1_VOL_CTRL1),
	LABEL(SUNXI_AIF1_VOL_CTRL2),
	LABEL(SUNXI_AIF1_VOL_CTRL3),
	LABEL(SUNXI_AIF1_VOL_CTRL4),
	LABEL(SUNXI_AIF1_MXR_GAIN),
	LABEL(SUNXI_AIF1_RXD_CTRL),
	LABEL(SUNXI_AIF2_CLK_CTRL),
	LABEL(SUNXI_AIF2_ADCDAT_CTRL),
	LABEL(SUNXI_AIF2_DACDAT_CTRL),
	LABEL(SUNXI_AIF2_MXR_SRC),
	LABEL(SUNXI_AIF2_VOL_CTRL1),
	LABEL(SUNXI_AIF2_VOL_CTRL2),
	LABEL(SUNXI_AIF2_MXR_GAIN),
	LABEL(SUNXI_AIF2_RXD_CTRL),
	LABEL(SUNXI_AIF3_CLK_CTRL),
	LABEL(SUNXI_AIF3_ADCDAT_CTRL),
	LABEL(SUNXI_AIF3_DACDAT_CTRL),
	LABEL(SUNXI_AIF3_SGP_CTRL),
	LABEL(SUNXI_AIF3_RXD_CTRL),
	LABEL(SUNXI_ADC_DIG_CTRL),
	LABEL(SUNXI_ADC_VOL_CTRL),
	LABEL(SUNXI_ADC_DBG_CTRL),
	LABEL(SUNXI_HMIC_CTRL1),
	LABEL(SUNXI_HMIC_CTRL2),
	LABEL(SUNXI_HMIC_STS),
	LABEL(SUNXI_DAC_DIG_CTRL),
	LABEL(SUNXI_DAC_VOL_CTRL),
	LABEL(SUNXI_DAC_DBG_CTRL),
	LABEL(SUNXI_DAC_MXR_SRC),
	LABEL(SUNXI_DAC_MXR_GAIN),
	LABEL(SUNXI_AC_DAC_DAPCTRL),
	LABEL(SUNXI_DRC_ENA),

	LABEL(HP_CTRL),
	LABEL(OL_MIX_CTRL),
	LABEL(OR_MIX_CTRL),
	LABEL(MIC2_CTRL),
	LABEL(LINEIN_CTRL),
	LABEL(MIX_DAC_CTRL),
	LABEL(R_ADCMIX_SRC),
	LABEL(ADC_CTRL),
	LABEL(HS_MBIAS_CTRL),
	LABEL(APT_REG),
	LABEL(OP_BIAS_CTRL0),
	LABEL(OP_BIAS_CTRL1),
	LABEL(ZC_VOL_CTRL),
	LABEL(BIAS_CAL_DATA),
	LABEL(BIAS_CAL_SET),
	LABEL(BD_CAL_CTRL),
	LABEL(HP_PA_CTRL),
	LABEL(HP_CAL_CTRL),
	/* LABEL(RHP_CAL_DAT), */
	LABEL(RHP_CAL_SET),
	/* LABEL(LHP_CAL_DAT), */
	LABEL(LHP_CAL_SET),
	LABEL(MDET_CTRL),
	LABEL(JACK_MIC_CTRL),
	LABEL(CP_LDO_CTRL),
	LABEL_END,
};

void sunxi_codec_dump_reg(void)
{
	struct sunxi_codec *codec = &g_codec;
	int i = 0;

	while (reg_labels[i].name != NULL) {
		printf("%s 0x%08x: 0x%x\n",
			reg_labels[i].name,
			reg_labels[i].value,
			sunxi_codec_read(codec, reg_labels[i].value));
		i++;
	}
#if 0
 {

		if ((reg_labels[i].value & (~ANALOG_FLAG)) == 0)
			reg_group++;
		if (reg_grounp == 1) {
			printf("%s 0x%p: 0x%x\n",
				reg_labels[i].name,
				reg_labels[i].addr,
				sunxi_codec_read(codec, reg_labels[i].reg));
		} else if (reg_group == 2) {

		}
	}
#endif
}
static const struct aif1_fs codec_aif1_fs[] = {
	{44100, 4, 7},
	{48000, 4, 8},
	{8000, 9, 0},
	{11025, 8, 1},
	{12000, 8, 2},
	{16000, 7, 3},
	{22050, 6, 4},
	{24000, 6, 5},
	{32000, 5, 6},
	{96000, 2, 9},
	{192000, 1, 10},
};

static const struct aif1_lrck codec_aif1_lrck[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

static const struct aif1_word_size codec_aif1_wsize[] = {
	{8, 0},
	{16, 1},
	{20, 2},
	{24, 3},
};


int sunxi_internal_i2s_clk_init(u32 sb, u32 sr, u32 ch)
{
	struct sunxi_codec *codec = &g_codec;
	u32 mclk_div = 0;
	u32 bclk_div = 0;
	int wss_value = 0;
	u32 over_sample_rate = 0;
	u32 word_select_size = 32;
	/*mclk div calculate*/
	switch (sr) {
	case 8000:
	{
		over_sample_rate = 128;
		mclk_div = 24;
		break;
	}
	case 16000:
	{
		over_sample_rate = 128;
		mclk_div = 12;
		break;
	}
	case 32000:
	{
		over_sample_rate = 128;
		mclk_div = 6;
		break;
	}
	case 64000:
	{
		over_sample_rate = 384;
		mclk_div = 1;
		break;
	}
	case 11025:
	case 12000:
	{
		over_sample_rate = 128;
		mclk_div = 16;
		break;
	}
	case 22050:
	case 24000:
	{
		over_sample_rate = 128;
		mclk_div = 8;
		break;
	}
	case 44100:
	case 48000:
	{
		over_sample_rate = 128;
		mclk_div = 4;
		break;
	}
	case 88200:
	case 96000:
	{
		over_sample_rate = 128;
		mclk_div = 2;
		break;
	}
	case 176400:
	case 192000:
	{
		over_sample_rate = 128;
		mclk_div = 1;
		break;
	}

	}

	 /*bclk div caculate*/
	 bclk_div = over_sample_rate/(2*word_select_size);
	 /*calculate MCLK Divide Ratio*/
	switch (mclk_div) {
	case 1:
		mclk_div = 0;
	break;
	case 2:
		mclk_div = 1;
	break;
	case 4:
		mclk_div = 2;
	break;
	case 6:
		mclk_div = 3;
	break;
	case 8:
		mclk_div = 4;
	break;
	case 12:
		mclk_div = 5;
	break;
	case 16:
		mclk_div = 6;
	break;
	case 24:
		mclk_div = 7;
	break;
	case 32:
		mclk_div = 8;
	break;
	case 48:
		mclk_div = 9;
	break;
	case 64:
		mclk_div = 0xA;
	break;
	default:
	break;
	}
	mclk_div &= 0xf;

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
	break;
	}
	bclk_div &= 0x7;

	 /*confige mclk and bclk dividor register*/
	sunxi_codec_update_bits(codec, SUNXI_DA_CLKD,
				0x7 << BCLKDIV, bclk_div << BCLKDIV);
	sunxi_codec_update_bits(codec, SUNXI_DA_CLKD,
				0xf << MCLKDIV, mclk_div << MCLKDIV);
	sunxi_codec_update_bits(codec, SUNXI_DA_CLKD,
				0x1 << 7, 1 << 7);

	/* word select size */
	switch (word_select_size) {
	case 16:
		wss_value = 0;
	break;
	case 20:
		wss_value = 1;
	break;
	case 24:
		wss_value = 2;
	break;
	case 32:
		wss_value = 3;
	break;
	}
	sunxi_codec_update_bits(codec, SUNXI_DA_FAT0,
					0x3 << WSS, wss_value << WSS);
	return 0;
}

int sunxi_codec_aif1_clk_enable(void)
{
		struct sunxi_codec *codec = &g_codec;
		sunxi_codec_update_bits(codec, SUNXI_SYSCLK_CTL,
			   (0x1 << AIF1CLK_ENA),
			   (0x1 << AIF1CLK_ENA));
			sunxi_codec_update_bits(codec, SUNXI_MOD_CLK_ENA,
				 (0x1 << AIF1_MOD_CLK_EN),
				 (0x1 << AIF1_MOD_CLK_EN));
			sunxi_codec_update_bits(codec, SUNXI_MOD_RST_CTL,
				 (0x1 << AIF1_MOD_RST_CTL),
				 (0x1 << AIF1_MOD_RST_CTL));
	sunxi_codec_update_bits(codec, SUNXI_SYSCLK_CTL,
	  (0x1 << SYSCLK_ENA),
		 (0x1 << SYSCLK_ENA));
				sunxi_codec_update_bits(codec, SUNXI_MOD_CLK_ENA,
					(0x1 << DAC_DIGITAL_MOD_CLK_EN),
					(0x1 << DAC_DIGITAL_MOD_CLK_EN));
			sunxi_codec_update_bits(codec, SUNXI_MOD_RST_CTL,
					(0x1 << DAC_DIGITAL_MOD_RST_CTL),
					(0x1 << DAC_DIGITAL_MOD_RST_CTL));
			sunxi_codec_update_bits(codec, SUNXI_AIF1_CLK_CTRL,
					(0x1 << AIF1_MSTR_MOD),
					(0x1 << AIF1_MSTR_MOD));
			sunxi_codec_update_bits(codec, SUNXI_SYSCLK_CTL,
				(0x3 << AIF1CLK_SRC), (0x3 << AIF1CLK_SRC));

		return 0;
}

/*
 * sample bits
 * sample rate
 * channels
 *
 */

int sunxi_codec_hw_params(u32 sb, u32 sr, u32 ch)
{
	int i;
	struct sunxi_codec *codec = &g_codec;
	uint rs_value;
	int aif1_lrlk_div = 64;
	int bclk_div_factor;
	sunxi_internal_i2s_clk_init(sb, sr,ch);

	/* sample rate */
	switch (sb) {
	case 16:
		rs_value = 0;
	break;
	case 20:
		rs_value = 1;
	break;
	case 24:
		rs_value = 2;
	break;
	default:
		return -1;
	}
	sunxi_codec_update_bits(codec, SUNXI_DA_FAT0, 0x3<< SR, rs_value << SR);

	if (sb == 24) {
		/* FIXME
		 * should set expanding 0 at LSB of RXFIFO.
		 * so that the valid data at the MSB of TXFIFO.
		 */
		sunxi_codec_update_bits(codec , SUNXI_DA_FCTL,
						0xf << RXOM, 0x0 << RXOM);
	} else
		sunxi_codec_update_bits(codec , SUNXI_DA_FCTL,
						0xf << RXOM, 0x5 << RXOM);

	/*
	 * FIXME make up the codec_aif1_lrck factor
	 * adjust for more working scene
	 */
	switch (aif1_lrlk_div) {
	case 16:
		bclk_div_factor = 4;
		break;
	case 32:
		bclk_div_factor = 2;
		break;
	case 64:
		bclk_div_factor = 0;
		break;
	case 128:
		bclk_div_factor = -2;
		break;
	case 256:
		bclk_div_factor = -4;
		break;
	default:
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].aif1_lrlk_div == aif1_lrlk_div) {
			sunxi_codec_update_bits(codec, SUNXI_AIF1_CLK_CTRL,
					(0x7 << AIF1_LRCK_DIV),
					((codec_aif1_lrck[i].aif1_lrlk_bit)
						<< AIF1_LRCK_DIV));
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samplerate ==  sr) {
			sunxi_codec_update_bits(codec, SUNXI_SYS_SR_CTRL,
				(0xf << AIF1_FS),
				((codec_aif1_fs[i].aif1_srbit) << AIF1_FS));
			sunxi_codec_update_bits(codec, SUNXI_SYS_SR_CTRL,
				(0xf << AIF2_FS),
				((codec_aif1_fs[i].aif1_srbit) << AIF2_FS));
			bclk_div_factor += codec_aif1_fs[i].aif1_bclk_div;
			sunxi_codec_update_bits(codec, SUNXI_AIF1_CLK_CTRL,
				(0xf << AIF1_BCLK_DIV),
				((bclk_div_factor) << AIF1_BCLK_DIV));
			break;
		}
	}

	sunxi_codec_update_bits(codec, SUNXI_AIF1_CLK_CTRL,
		(0x1 << DSP_MONO_PCM), (0x0 << DSP_MONO_PCM));

	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].aif1_wsize_val == sb) {
			sunxi_codec_update_bits(codec, SUNXI_AIF1_CLK_CTRL,
				(0x3<<AIF1_WORD_SIZ),
				((codec_aif1_wsize[i].aif1_wsize_bit)
					<< AIF1_WORD_SIZ));
			break;
		}
	}

	/* Tx channel select */
	sunxi_codec_update_bits(codec, SUNXI_DA_TXCHSEL,
			(0x7 << TX_CHSEL), ((ch-1) << TX_CHSEL));
	/* Tx channel map(default value) */
#if 0
	sunxi_codec_update_bits(codec, SUNXI_DA_TXCHMAP,
			(0x7 << TX_CH0_MAP), (0 << TX_CH0_MAP));
	sunxi_codec_update_bits(codec, SUNXI_DA_TXCHMAP,
			(0x7 << TX_CH1_MAP), (1 << TX_CH1_MAP));
	sunxi_codec_update_bits(codec, SUNXI_DA_TXCHMAP,
			(0x7 << TX_CH2_MAP), (2 << TX_CH2_MAP));
	sunxi_codec_update_bits(codec, SUNXI_DA_TXCHMAP,
			(0x7 << TX_CH3_MAP), (3 << TX_CH3_MAP));
#endif
	return 0;
}


static int sunxi_codec_init(struct sunxi_codec *codec)
{
	void __iomem *cfg_reg;
	volatile u32 tmp_val = 0;

	/*
	 * CCMU register
	 * BASE				0x01c2 0000
	 * PLL_AUDIO_CTRL_REG		0x78
	 * PLL_AUDIO_BIAS_REG		0x378
	 * PLL_AUDIO_PAT_CTRL_REG	0x178
	 * BUS_SOFT_RST_REG3		0xa5c
	 * BUS_CLK_GATING_REG2		0x68
	 * AC_DIG_CLK_REG		0x140
	 *
	 */
	cfg_reg = codec->addr_clkbase + 0x78;
	tmp_val = 0x898b1701;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x178;
	tmp_val = 0xc00126e9;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0xa5c;
	tmp_val = 0x00;
	writel(tmp_val, cfg_reg);
	tmp_val = 0x10000;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0xa5c;
	tmp_val = 0x10001;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0xa50;
	tmp_val = 0x80000000;
	writel(tmp_val, cfg_reg);

	/*
	 * Audio Codec
	 *
	 */

	sunxi_codec_update_bits(codec, SUNXI_DA_CTL, 0x1 << GEN, 1 << GEN);

	sunxi_codec_aif1_clk_enable();

	sunxi_codec_update_bits(codec, HP_CTRL,
				(0x1 << HPPA_EN), (0x1 << HPPA_EN));

	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
				(0x1 << LHPPAMUTE), (0x1 << LHPPAMUTE));
	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
				(0x1 << RHPPAMUTE), (0x1 << RHPPAMUTE));

	/* set headphone volume */
	sunxi_codec_update_bits(codec, HP_CTRL,
			(0x3f << HPVOL), (codec->headphone_vol << HPVOL));


	/*
	 * codec route
	 * AIF10DACL>DAC_L -> Left Output Mixer  -> SPK_LR Adder -> SPK_L Mux -> SPKL -> External Speaker
	 * AIF10DACR->DAC_R -> Right Output Mixer -> SPK_LR Adder -> SPK_R Mux -> SPKR -> External Speaker
	 */
	/*
	* enable aif1
	*/

	sunxi_codec_update_bits(codec, SUNXI_AIF1_DACDAT_CTRL,
			(0x1 << AIF1_DA0L_ENA), (1 << AIF1_DA0L_ENA));
	sunxi_codec_update_bits(codec, SUNXI_AIF1_DACDAT_CTRL,
			(0x1 << AIF1_DA0R_ENA), (1 << AIF1_DA0R_ENA));
	sunxi_codec_update_bits(codec, SUNXI_AIF1_DACDAT_CTRL,
			(0x1 << AIF1_DA0L_SRC), (0 << AIF1_DA0L_SRC));
	sunxi_codec_update_bits(codec, SUNXI_AIF1_DACDAT_CTRL,
			(0x1 << AIF1_DA0R_SRC), (0 << AIF1_DA0R_SRC));
	sunxi_codec_update_bits(codec, SUNXI_DAC_MXR_SRC,
			(0x1 << DACL_MXR_SRC_AIF1DA0L), (1 << DACL_MXR_SRC_AIF1DA0L));
	sunxi_codec_update_bits(codec, SUNXI_DAC_MXR_SRC,
			(0x1 << DACR_MXR_SRC_AIF1DA0R), (1 << DACR_MXR_SRC_AIF1DA0R));

	/* enable DAC */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DIG_CTRL,
			(0x1 << ENDA), (1 << ENDA));
	/* enable Analog Right channel DAC */
	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << DACAREN), (1 << DACAREN));
	/* enable Analog Right channel DAC */
	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << DACALEN), (1 << DACALEN));
	/* enable Right Output Mixer */
	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << RMIXEN), (1 << RMIXEN));
	/* set DRCR --> Right Output Mixer */
	sunxi_codec_update_bits(codec, OR_MIX_CTRL,
			(0x1 << RMIXMUTEDACR), (1 << RMIXMUTEDACR));
	/* enable Left Output Mixer */
	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << LMIXEN), (1 << LMIXEN));
	/* set DRCL --> Left Output Mixer */
	sunxi_codec_update_bits(codec, OL_MIX_CTRL,
			(0x1 << LMIXMUTEDACL), (1 << LMIXMUTEDACL));

	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << RHPPAMUTE), (1 << RHPPAMUTE));

	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << LHPPAMUTE), (1 << LHPPAMUTE));

	sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << RHPIS), (1 << RHPIS));
		sunxi_codec_update_bits(codec, MIX_DAC_CTRL,
			(0x1 << LHPIS), (1 << LHPIS));

	return 0;
}

#if 0
fdt_addr_t new_fdtdec_get_addr_size_fixed(const void *blob, int node,
		const char *prop_name, int index, int na,
		int ns, fdt_size_t *sizep,
		bool translate)
{
	const fdt32_t *prop, *prop_end;
	const fdt32_t *prop_addr, *prop_size, *prop_after_size;
	int len;
	fdt_addr_t addr;

	prop = fdt_getprop(blob, node, prop_name, &len);
	if (!prop) {
		debug("(not found)\n");
		return FDT_ADDR_T_NONE;
	}
	prop_end = prop + (len / sizeof(*prop));

	prop_addr = prop + (index * (na + ns));
	prop_size = prop_addr + na;
	prop_after_size = prop_size + ns;
	if (prop_after_size > prop_end) {
		debug("(not enough data: expected >= %d cells, got %d cells)\n",
		      (u32)(prop_after_size - prop), ((u32)(prop_end - prop)));
		return FDT_ADDR_T_NONE;
	}

#if CONFIG_IS_ENABLED(OF_TRANSLATE)
	if (translate)
		addr = fdt_translate_address(blob, node, prop_addr);
	else
#endif
		addr = fdtdec_get_number(prop_addr, na);

	if (sizep) {
		*sizep = fdtdec_get_number(prop_size, ns);
		debug("addr=%08llx, size=%llx\n", (unsigned long long)addr,
		      (unsigned long long)*sizep);
	} else {
		debug("addr=%08llx\n", (unsigned long long)addr);
	}

	return addr;
}
#endif

static int get_sunxi_codec_values(struct sunxi_codec *codec, const void *blob)
{
	int node, ret;
#if 0
	fdt_addr_t addr = 0;
	fdt_size_t size = 0;
#endif

	node = fdt_node_offset_by_compatible(blob, 0, "allwinner,clk-init");
	if (node < 0) {
		printf("unable to find allwinner,clk-init node in device tree.\n");
		return node;
	}
#if 0
	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 0, 2, 2, &size, false);
	codec->addr_clkbase = map_sysmem(addr, size);
#else
	codec->addr_clkbase = (void *)0x03001000;
#endif

	node = fdt_node_offset_by_compatible(blob, 0, "allwinner,sunxi-internal-codec");
	if (node < 0) {
		printf("unable to find allwinner,sunxi-internal-codec node in device tree.\n");
		return node;
	}
	if (!fdtdec_get_is_enabled(blob, node)) {
		printf("sunxi-internal-codec disabled in device tree.\n");
		return -1;
	}
#if 0
	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 0, 2, 2, &size, false);
	codec->addr_dbase = map_sysmem(addr, size);

	addr = new_fdtdec_get_addr_size_fixed(blob, node, "reg", 1, 2, 2, &size, false);
	codec->addr_abase = map_sysmem(addr, size);
#else
	codec->addr_dbase = (void *)(0x05096000);
	codec->addr_abase = (void *)(0x050967c0);
#endif

	/*printf("dbase:0x%p, abase:0x%p\n", codec->addr_dbase, codec->addr_abase);*/
	ret = fdt_getprop_u32(blob, node, "headphonevol", &codec->headphone_vol);
	if (ret < 0) {
		printf("headphone_vol not set. default 0x3b\n");
		codec->headphone_vol = 0x3b;
	}
	ret = fdt_getprop_u32(blob, node, "pa_ctl_level", &codec->pa_ctl_level);
	if (ret < 0) {
		printf("pa_ctl_level not set. default 1\n");
		codec->pa_ctl_level = 1;
	}

	ret = fdt_get_one_gpio_by_offset(node, "gpio-spk", &codec->spk_cfg);
	if (ret < 0 || codec->spk_cfg.port == 0) {
		printf("parser gpio-spk failed, ret:%d, port:%u\n", ret, codec->spk_cfg.port);
	} else {
		codec->spk_handle = gpio_request(&codec->spk_cfg, 1);
		if (!codec->spk_handle) {
			printf("reuqest gpio for spk failed\n");
			return -1;
		}
		gpio_set_one_pin_io_status(codec->spk_handle, 1, "spk-gpio");
	}

	return 0;
}

int sunxi_codec_playback_prepare(void)
{
	struct sunxi_codec *codec = &g_codec;

	sunxi_codec_update_bits(codec, SUNXI_DA_TXCNT,0xffffffff << TX_CNT, 0 << TX_CNT);
		/* flush TX FIFO*/
	sunxi_codec_update_bits(codec, SUNXI_DA_FCTL, 0x1 << FTX, 1 << FTX);

	/*SDO ON*/
	sunxi_codec_update_bits(codec , SUNXI_DA_CTL, 0x1 << SDO_EN, 1 << SDO_EN);
	/* I2S0 TX ENABLE */
	sunxi_codec_update_bits(codec, SUNXI_DA_CTL, 0x1 << TXEN, 1 << TXEN);
	/* enable DMA DRQ mode for play */
	sunxi_codec_update_bits(codec, SUNXI_DA_INT, 0x1 << TX_DRQ, 1 << TX_DRQ);


	/*sunxi_codec_dump_reg();*/
	/* increase delay time, if Pop occured */
	__msdelay(10);
	gpio_write_one_pin_value(codec->spk_handle, codec->pa_ctl_level, "spk-gpio");

	return 0;
}

int sunxi_codec_probe(void)
{

	int ret;
	struct sunxi_codec *codec = &g_codec;
	const void *blob = gd->fdt_blob;
	if (get_sunxi_codec_values(codec, blob) < 0) {
		printf("sys_config Codec values failed\n");
		return -1;
	}
	ret = sunxi_codec_init(codec);

	return ret;
}

void sunxi_codec_fill_txfifo(u32 *data)
{
	struct sunxi_codec *codec = &g_codec;

	sunxi_codec_write(codec, SUNXI_DA_TXFIFO, *data);
}

int sunxi_codec_playback_start(ulong handle, u32 *srcBuf, u32 cnt)
{
	int ret = 0;
	struct sunxi_codec *codec = &g_codec;

	flush_cache((unsigned long)srcBuf, cnt);

#if 0
	printf("start dma: form 0x%x to 0x%x  total 0x%x(%u)byte\n",
	       (uint)srcBuf, (uint)codec->addr_dbase + SUNXI_DA_TXFIFO, cnt, cnt);
#endif
	/*ret = sunxi_dma_start(handle, (uint)srcBuf, codec->addr_dbase + SUNXI_DAC_TXDATA);*/
	ret = sunxi_dma_start(handle, (uint)srcBuf,
			      (uint)(codec->addr_dbase + SUNXI_DA_TXFIFO), cnt);

	return ret;
}
