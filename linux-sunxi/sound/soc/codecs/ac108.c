/*
 * ac108.c  --  ac108 ALSA Soc Audio driver
 *
 * Author: panjunwen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/of.h>
#include <sound/tlv.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include "ac108.h"

#define AC108_DEBUG_EN			0

#if AC108_DEBUG_EN
#define AC108_DEBUG(...)	printk(KERN_DEBUG __VA_ARGS__)
#else
#define AC108_DEBUG(...)
#endif

/* //test enable config */
#define AC108_DAPM_TEST_EN			0
#define AC108_CODEC_RW_TEST_EN		0

#define AC108_NUM_MAX 			4
#define AC108_SLOT_WIDTH		24
#define AC108_DMIC_EN			0
/* //0:ADC  1:DMIC */
#define AC108_ENCODING_EN		0
/* //TX Encoding mode enable */

#define AC108_REGULATOR_NAME	"voltage_enable"

#define AC108_RATES	(SNDRV_PCM_RATE_8000_96000 | SNDRV_PCM_RATE_KNOT)
#define AC108_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
		SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

struct i2c_client *i2c_clt[AC108_NUM_MAX];
int regulator_en;
unsigned int ac108_bclk_div_val;
struct ac108_public_config ac108_pub_cfg;
EXPORT_SYMBOL(ac108_pub_cfg);

struct ac108_priv {
	struct i2c_client *i2c;
	struct snd_soc_codec *codec;
};

static const struct regmap_config ac108_regmap_config = {
	.reg_bits = 8,
	/* //Number of bits in a register address */
	.val_bits = 8,
	/* //Number of bits in a register value */
};

struct real_val_to_reg_val {
	unsigned int real_val;
	unsigned int reg_val;
};

struct pll_div {
	u32 freq_in;
	u32 freq_out;
	u32 m1;
	u32 m2;
	u32 n;
	u32 k1;
	u32 k2;
};

static const struct real_val_to_reg_val ac108_sample_rate[] = {
	{8000, 0},
	{11025, 1},
	{12000, 2},
	{16000, 3},
	{22050, 4},
	{24000, 5},
	{32000, 6},
	{44100, 7},
	{48000, 8},
	{96000, 9},
};

static const struct real_val_to_reg_val ac108_sample_resolution[] = {
	{8, 1},
	{12, 2},
	{16, 3},
	{20, 4},
	{24, 5},
	{28, 6},
	{32, 7},
};

static const struct real_val_to_reg_val ac108_bclk_div[] = {
	{0, 0},
	{1, 1},
	{2, 2},
	{4, 3},
	{6, 4},
	{8, 5},
	{12, 6},
	{16, 7},
	{24, 8},
	{32, 9},
	{48, 10},
	{64, 11},
	{96, 12},
	{128, 13},
	{176, 14},
	{192, 15},
};

/* //FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ; \
 * M1[0,31],  M2[0,1],  N[0,1023],  K1[0,31],  K2[0,1] */
static const struct pll_div ac108_pll_div[] = {
	{400000, 24576000, 0, 0, 614, 4, 1},
	{512000, 24576000, 0, 0, 960, 9, 1},
	/* //24576000/48 */
	{768000, 24576000, 0, 0, 640, 9, 1},
	/* //24576000/32 */
	{800000, 24576000, 0, 0, 614, 9, 1},
	{1024000, 24576000, 0, 0, 480, 9, 1},
	/* //24576000/24 */
	{1600000, 24576000, 0, 0, 307, 9, 1},
	{2048000, 24576000, 0, 0, 240, 9, 1},
	/* //24576000/12 */
	{3072000, 24576000, 0, 0, 160, 9, 1},
	/* //24576000/8 */
	{4096000, 24576000, 2, 0, 360, 9, 1},
	/* //24576000/6 */
	{6000000, 24576000, 4, 0, 410, 9, 1},
	{12000000, 24576000, 9, 0, 410, 9, 1},
	{13000000, 24576000, 8, 0, 340, 9, 1},
	{15360000, 24576000, 12, 0, 415, 9, 1},
	{16000000, 24576000, 12, 0, 400, 9, 1},
	{19200000, 24576000, 15, 0, 410, 9, 1},
	{19680000, 24576000, 15, 0, 400, 9, 1},
	{24000000, 24576000, 9, 0, 205, 9, 1},

	{400000, 22579200, 0, 0, 566, 4, 1},
	{512000, 22579200, 0, 0, 880, 9, 1},
	{768000, 22579200, 0, 0, 587, 9, 1},
	{800000, 22579200, 0, 0, 567, 9, 1},
	{1024000, 22579200, 0, 0, 440, 9, 1},
	{1600000, 22579200, 1, 0, 567, 9, 1},
	{2048000, 22579200, 0, 0, 220, 9, 1},
	{3072000, 22579200, 0, 0, 148, 9, 1},
	{4096000, 22579200, 2, 0, 330, 9, 1},
	{6000000, 22579200, 2, 0, 227, 9, 1},
	{12000000, 22579200, 8, 0, 340, 9, 1},
	{13000000, 22579200, 9, 0, 350, 9, 1},
	{15360000, 22579200, 10, 0, 325, 9, 1},
	{16000000, 22579200, 11, 0, 340, 9, 1},
	{19200000, 22579200, 13, 0, 330, 9, 1},
	{19680000, 22579200, 14, 0, 345, 9, 1},
	{24000000, 22579200, 16, 0, 320, 9, 1},

	{12288000, 24576000, 9, 0, 400, 9, 1},
	/* //24576000/2 */
	{11289600, 22579200, 9, 0, 400, 9, 1},
	/* g//22579200/2 */

	{24576000 / 1, 24576000, 9, 0, 200, 9, 1},
	/* //24576000 */
	{24576000 / 4, 24576000, 4, 0, 400, 9, 1},
	/* //6144000 */
	{24576000 / 16, 24576000, 0, 0, 320, 9, 1},
	/* //1536000 */
	{24576000 / 64, 24576000, 0, 0, 640, 4, 1},
	/* //384000 */
	{24576000 / 96, 24576000, 0, 0, 960, 4, 1},
	/* //256000 */
	{24576000 / 128, 24576000, 0, 0, 512, 1, 1},
	/* //192000 */
	{24576000 / 176, 24576000, 0, 0, 880, 4, 0},
	/* //140000 */
	{24576000 / 192, 24576000, 0, 0, 960, 4, 0},
	/* //128000 */

	{22579200 / 1, 22579200, 9, 0, 200, 9, 1},
	/* //22579200 */
	{22579200 / 4, 22579200, 4, 0, 400, 9, 1},
	/* //5644800 */
	{22579200 / 16, 22579200, 0, 0, 320, 9, 1},
	/* //1411200 */
	{22579200 / 64, 22579200, 0, 0, 640, 4, 1},
	/* //352800 */
	{22579200 / 96, 22579200, 0, 0, 960, 4, 1},
	/* //235200 */
	{22579200 / 128, 22579200, 0, 0, 512, 1, 1},
	/* //176400 */
	{22579200 / 176, 22579200, 0, 0, 880, 4, 0},
	/* //128290 */
	{22579200 / 192, 22579200, 0, 0, 960, 4, 0},
	/* //117600 */

	{22579200 / 6, 22579200, 2, 0, 360, 9, 1},
	/* //3763200 */
	{22579200 / 8, 22579200, 0, 0, 160, 9, 1},
	/* //2822400 */
	{22579200 / 12, 22579200, 0, 0, 240, 9, 1},
	/* //1881600 */
	{22579200 / 24, 22579200, 0, 0, 480, 9, 1},
	/* //940800 */
	{22579200 / 32, 22579200, 0, 0, 640, 9, 1},
	/* //705600 */
	{22579200 / 48, 22579200, 0, 0, 960, 9, 1},
	/* //470400 */
};

static const DECLARE_TLV_DB_SCALE(adc1_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc2_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc3_pga_gain_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc4_pga_gain_tlv, 0, 100, 0);

static const DECLARE_TLV_DB_SCALE(ch1_digital_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(ch2_digital_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(ch3_digital_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(ch4_digital_vol_tlv, -11925, 75, 0);

static const DECLARE_TLV_DB_SCALE(channel1_ch1_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel1_ch2_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel1_ch3_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel1_ch4_dig_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(channel2_ch1_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel2_ch2_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel2_ch3_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel2_ch4_dig_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(channel3_ch1_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel3_ch2_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel3_ch3_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel3_ch4_dig_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(channel4_ch1_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel4_ch2_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel4_ch3_dig_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(channel4_ch4_dig_mix_vol_tlv, -600, 600, 0);

/* //static const DECLARE_TLV_DB_SCALE(adc_pga_gain_tlv,0,100,0); */
/* //static const DECLARE_TLV_DB_SCALE(digital_vol_tlv,-11925,75,0); */
/* //static const DECLARE_TLV_DB_SCALE(digital_mix_vol_tlv,-600,600,0); */

/*************************************** General(volume) controls ****
 * ***************************************/
/* //ac108 common controls */
static const struct snd_kcontrol_new ac108_controls[] = {
	SOC_SINGLE_TLV("ADC1 PGA gain", ANA_PGA1_CTRL, ADC1_ANALOG_PGA, 0x1f, 0,
			adc1_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC2 PGA gain", ANA_PGA2_CTRL, ADC2_ANALOG_PGA, 0x1f, 0,
			adc2_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC3 PGA gain", ANA_PGA3_CTRL, ADC3_ANALOG_PGA, 0x1f, 0,
			adc3_pga_gain_tlv),
	SOC_SINGLE_TLV("ADC4 PGA gain", ANA_PGA4_CTRL, ADC4_ANALOG_PGA, 0x1f, 0,
			adc4_pga_gain_tlv),

	SOC_SINGLE_TLV("CH1 digital volume", ADC1_DVOL_CTRL, 0, 0xff, 0,
			ch1_digital_vol_tlv),
	SOC_SINGLE_TLV("CH2 digital volume", ADC2_DVOL_CTRL, 0, 0xff, 0,
			ch2_digital_vol_tlv),
	SOC_SINGLE_TLV("CH3 digital volume", ADC3_DVOL_CTRL, 0, 0xff, 0,
			ch3_digital_vol_tlv),
	SOC_SINGLE_TLV("CH4 digital volume", ADC4_DVOL_CTRL, 0, 0xff, 0,
			ch4_digital_vol_tlv),

	SOC_SINGLE_TLV("CH1 ch1 mixer gain", ADC1_DMIX_SRC, ADC1_ADC1_DMXL_GC,
			1, 0, channel1_ch1_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH1 ch2 mixer gain", ADC1_DMIX_SRC, ADC1_ADC2_DMXL_GC,
			1, 0, channel1_ch2_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH1 ch3 mixer gain", ADC1_DMIX_SRC, ADC1_ADC3_DMXL_GC,
			1, 0, channel1_ch3_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH1 ch4 mixer gain", ADC1_DMIX_SRC, ADC1_ADC4_DMXL_GC,
			1, 0, channel1_ch4_dig_mix_vol_tlv),

	SOC_SINGLE_TLV("CH2 ch1 mixer gain", ADC2_DMIX_SRC, ADC2_ADC1_DMXL_GC,
			1, 0, channel2_ch1_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH2 ch2 mixer gain", ADC2_DMIX_SRC, ADC2_ADC2_DMXL_GC,
			1, 0, channel2_ch2_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH2 ch3 mixer gain", ADC2_DMIX_SRC, ADC2_ADC3_DMXL_GC,
			1, 0, channel2_ch3_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH2 ch4 mixer gain", ADC2_DMIX_SRC, ADC2_ADC4_DMXL_GC,
			1, 0, channel2_ch4_dig_mix_vol_tlv),

	SOC_SINGLE_TLV("CH3 ch1 mixer gain", ADC3_DMIX_SRC, ADC3_ADC1_DMXL_GC,
			1, 0, channel3_ch1_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH3 ch2 mixer gain", ADC3_DMIX_SRC, ADC3_ADC2_DMXL_GC,
			1, 0, channel3_ch2_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH3 ch3 mixer gain", ADC3_DMIX_SRC, ADC3_ADC3_DMXL_GC,
			1, 0, channel3_ch3_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH3 ch4 mixer gain", ADC3_DMIX_SRC, ADC3_ADC4_DMXL_GC,
			1, 0, channel3_ch4_dig_mix_vol_tlv),

	SOC_SINGLE_TLV("CH4 ch1 mixer gain", ADC4_DMIX_SRC, ADC4_ADC1_DMXL_GC,
			1, 0, channel4_ch1_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH4 ch2 mixer gain", ADC4_DMIX_SRC, ADC4_ADC2_DMXL_GC,
			1, 0, channel4_ch2_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH4 ch3 mixer gain", ADC4_DMIX_SRC, ADC4_ADC3_DMXL_GC,
			1, 0, channel4_ch3_dig_mix_vol_tlv),
	SOC_SINGLE_TLV("CH4 ch4 mixer gain", ADC4_DMIX_SRC, ADC4_ADC4_DMXL_GC,
			1, 0, channel4_ch4_dig_mix_vol_tlv),

	/* //SOC_SINGLE_TLV("CH1 mixer gain", ADC1_DMIX_SRC,
	 * ADC1_ADC1_DMXL_GC, 0x0f, 0, digital_mix_vol_tlv), */
	/* //SOC_SINGLE_TLV("CH2 mixer gain", ADC2_DMIX_SRC,
	 * ADC2_ADC1_DMXL_GC, 0x0f, 0, digital_mix_vol_tlv), */
	/* //SOC_SINGLE_TLV("CH3 mixer gain", ADC3_DMIX_SRC,
	 * ADC3_ADC1_DMXL_GC, 0x0f, 0, digital_mix_vol_tlv), */
	/* //SOC_SINGLE_TLV("CH4 mixer gain", ADC4_DMIX_SRC,
	 * ADC4_ADC1_DMXL_GC, 0x0f, 0, digital_mix_vol_tlv), */
};

/*************************************** DAPM controls ************************/
/* //ADC12 DMIC1 Source Select Mux */
static const char * const adc12_dmic1_src_mux_text[] = {
	"ADC12 switch", "DMIC1 switch"
};

static const struct soc_enum adc12_dmic1_src_mux_enum =
SOC_ENUM_SINGLE(DMIC_EN, DMIC1_EN, 2, adc12_dmic1_src_mux_text);
static const struct snd_kcontrol_new adc12_dmic1_src_mux =
SOC_DAPM_ENUM("ADC12 DMIC1 MUX", adc12_dmic1_src_mux_enum);

/* //ADC34 DMIC2 Source Select Mux */
static const char * const  const adc34_dmic2_src_mux_text[] = {
	"ADC34 switch", "DMIC2 switch"
};

static const struct soc_enum adc34_dmic2_src_mux_enum =
SOC_ENUM_SINGLE(DMIC_EN, DMIC2_EN, 2, adc34_dmic2_src_mux_text);
static const struct snd_kcontrol_new adc34_dmic2_src_mux =
SOC_DAPM_ENUM("ADC34 DMIC2 MUX", adc34_dmic2_src_mux_enum);

/* //ADC1 Digital Source Select Mux */
static const char * const adc1_digital_src_mux_text[] = {
	"ADC1 switch", "ADC2 switch", "ADC3 switch", "ADC4 switch"
};

static const struct soc_enum adc1_digital_src_mux_enum =
SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC1_SRS, 4, adc1_digital_src_mux_text);
static const struct snd_kcontrol_new adc1_digital_src_mux =
SOC_DAPM_ENUM("ADC1 DIG MUX", adc1_digital_src_mux_enum);

/* //ADC2 Digital Source Select Mux */
static const char * const adc2_digital_src_mux_text[] = {
	"ADC1 switch", "ADC2 switch", "ADC3 switch", "ADC4 switch"
};

static const struct soc_enum adc2_digital_src_mux_enum =
SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC2_SRS, 4, adc2_digital_src_mux_text);
static const struct snd_kcontrol_new adc2_digital_src_mux =
SOC_DAPM_ENUM("ADC2 DIG MUX", adc2_digital_src_mux_enum);

/* //ADC3 Digital Source Select Mux */
static const char * const adc3_digital_src_mux_text[] = {
	"ADC1 switch", "ADC2 switch", "ADC3 switch", "ADC4 switch"
};

static const struct soc_enum adc3_digital_src_mux_enum =
SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC3_SRS, 4, adc3_digital_src_mux_text);
static const struct snd_kcontrol_new adc3_digital_src_mux =
SOC_DAPM_ENUM("ADC3 DIG MUX", adc3_digital_src_mux_enum);

/* //ADC4 Digital Source Select Mux */
static const char * const adc4_digital_src_mux_text[] = {
	"ADC1 switch", "ADC2 switch", "ADC3 switch", "ADC4 switch"
};

static const struct soc_enum adc4_digital_src_mux_enum =
SOC_ENUM_SINGLE(ADC_DSR, DIG_ADC4_SRS, 4, adc4_digital_src_mux_text);
static const struct snd_kcontrol_new adc4_digital_src_mux =
SOC_DAPM_ENUM("ADC4 DIG MUX", adc4_digital_src_mux_enum);

/* //ADC1 Digital Source Control Mixer */
static const struct snd_kcontrol_new adc1_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 DAT switch", ADC1_DMIX_SRC, ADC1_ADC1_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC2 DAT switch", ADC1_DMIX_SRC, ADC1_ADC2_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC3 DAT switch", ADC1_DMIX_SRC, ADC1_ADC3_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC4 DAT switch", ADC1_DMIX_SRC, ADC1_ADC4_DMXL_SRC, 1,
			0),
};

/* //ADC2 Digital Source Control Mixer */
static const struct snd_kcontrol_new adc2_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 DAT switch", ADC2_DMIX_SRC, ADC2_ADC1_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC2 DAT switch", ADC2_DMIX_SRC, ADC2_ADC2_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC3 DAT switch", ADC2_DMIX_SRC, ADC2_ADC3_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC4 DAT switch", ADC2_DMIX_SRC, ADC2_ADC4_DMXL_SRC, 1,
			0),
};

/* //ADC3 Digital Source Control Mixer */
static const struct snd_kcontrol_new adc3_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 DAT switch", ADC3_DMIX_SRC, ADC3_ADC1_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC2 DAT switch", ADC3_DMIX_SRC, ADC3_ADC2_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC3 DAT switch", ADC3_DMIX_SRC, ADC3_ADC3_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC4 DAT switch", ADC3_DMIX_SRC, ADC3_ADC4_DMXL_SRC, 1,
			0),
};

/* //ADC4 Digital Source Control Mixer */
static const struct snd_kcontrol_new adc4_digital_src_mixer[] = {
	SOC_DAPM_SINGLE("ADC1 DAT switch", ADC4_DMIX_SRC, ADC4_ADC1_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC2 DAT switch", ADC4_DMIX_SRC, ADC4_ADC2_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC3 DAT switch", ADC4_DMIX_SRC, ADC4_ADC3_DMXL_SRC, 1,
			0),
	SOC_DAPM_SINGLE("ADC4 DAT switch", ADC4_DMIX_SRC, ADC4_ADC4_DMXL_SRC, 1,
			0),
};

/* //I2S TX1 Ch1 Mapping Mux */
static const char * const i2s_tx1_ch1_map_mux_text[] = {
	"ADC1 Sample switch", "ADC2 Sample switch", "ADC3 Sample switch",
	"ADC4 Sample switch"
};

static const struct soc_enum i2s_tx1_ch1_map_mux_enum =
SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH1_MAP, 4, i2s_tx1_ch1_map_mux_text);
static const struct snd_kcontrol_new i2s_tx1_ch1_map_mux =
SOC_DAPM_ENUM("I2S TX1 CH1 MUX", i2s_tx1_ch1_map_mux_enum);

/* //I2S TX1 Ch2 Mapping Mux */
static const char * const i2s_tx1_ch2_map_mux_text[] = {
	"ADC1 Sample switch", "ADC2 Sample switch", "ADC3 Sample switch",
	"ADC4 Sample switch"
};

static const struct soc_enum i2s_tx1_ch2_map_mux_enum =
SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH2_MAP, 4, i2s_tx1_ch2_map_mux_text);
static const struct snd_kcontrol_new i2s_tx1_ch2_map_mux =
SOC_DAPM_ENUM("I2S TX1 CH2 MUX", i2s_tx1_ch2_map_mux_enum);

/* //I2S TX1 Ch3 Mapping Mux */
static const char * const i2s_tx1_ch3_map_mux_text[] = {
	"ADC1 Sample switch", "ADC2 Sample switch", "ADC3 Sample switch",
	"ADC4 Sample switch"
};

static const struct soc_enum i2s_tx1_ch3_map_mux_enum =
SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH3_MAP, 4, i2s_tx1_ch3_map_mux_text);
static const struct snd_kcontrol_new i2s_tx1_ch3_map_mux =
SOC_DAPM_ENUM("I2S TX1 CH3 MUX", i2s_tx1_ch3_map_mux_enum);

/* //I2S TX1 Ch4 Mapping Mux */
static const char * const i2s_tx1_ch4_map_mux_text[] = {
	"ADC1 Sample switch", "ADC2 Sample switch", "ADC3 Sample switch",
	"ADC4 Sample switch"
};

static const struct soc_enum i2s_tx1_ch4_map_mux_enum =
SOC_ENUM_SINGLE(I2S_TX1_CHMP_CTRL1, TX1_CH4_MAP, 4, i2s_tx1_ch4_map_mux_text);
static const struct snd_kcontrol_new i2s_tx1_ch4_map_mux =
SOC_DAPM_ENUM("I2S TX1 CH4 MUX", i2s_tx1_ch4_map_mux_enum);

/******************* DAPM widgets *******************************************/
/* //ac108 dapm widgets */
static const struct snd_soc_dapm_widget ac108_dapm_widgets[] = {
	/* //input widgets */
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),

	SND_SOC_DAPM_INPUT("MIC2P"),
	SND_SOC_DAPM_INPUT("MIC2N"),

	SND_SOC_DAPM_INPUT("MIC3P"),
	SND_SOC_DAPM_INPUT("MIC3N"),

	SND_SOC_DAPM_INPUT("MIC4P"),
	SND_SOC_DAPM_INPUT("MIC4N"),

	SND_SOC_DAPM_INPUT("DMIC1"),
	SND_SOC_DAPM_INPUT("DMIC2"),

	/* //MIC PGA */
	SND_SOC_DAPM_PGA("MIC1 PGA", ANA_ADC1_CTRL1, ADC1_PGA_ENABLE, 0, NULL,
			0),
	SND_SOC_DAPM_PGA("MIC2 PGA", ANA_ADC2_CTRL1, ADC2_PGA_ENABLE, 0, NULL,
			0),
	SND_SOC_DAPM_PGA("MIC3 PGA", ANA_ADC3_CTRL1, ADC3_PGA_ENABLE, 0, NULL,
			0),
	SND_SOC_DAPM_PGA("MIC4 PGA", ANA_ADC4_CTRL1, ADC4_PGA_ENABLE, 0, NULL,
			0),

	/* //DMIC PGA */
	SND_SOC_DAPM_PGA("DMIC1L PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC1R PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_PGA("DMIC2L PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("DMIC2R PGA", SND_SOC_NOPM, 0, 0, NULL, 0),

	/* //ADC1 DIG MUX */
	SND_SOC_DAPM_MUX("ADC1 DIG MUX", ADC_DIG_EN, ENAD1, 0,
			&adc1_digital_src_mux),

	/* //ADC2 DIG MUX */
	SND_SOC_DAPM_MUX("ADC2 DIG MUX", ADC_DIG_EN, ENAD2, 0,
			&adc2_digital_src_mux),

	/* //ADC3 DIG MUX */
	SND_SOC_DAPM_MUX("ADC3 DIG MUX", ADC_DIG_EN, ENAD3, 0,
			&adc3_digital_src_mux),

	/* //ADC4 DIG MUX */
	SND_SOC_DAPM_MUX("ADC4 DIG MUX", ADC_DIG_EN, ENAD4, 0,
			&adc4_digital_src_mux),

	/* //ADC12 DMIC1 MUX */
	SND_SOC_DAPM_MUX("ADC12 DMIC1 MUX", SND_SOC_NOPM, 0, 0,
			&adc12_dmic1_src_mux),

	/* //ADC34 DMIC2 MUX */
	SND_SOC_DAPM_MUX("ADC34 DMIC2 MUX", SND_SOC_NOPM, 0, 0,
			&adc34_dmic2_src_mux),

	/* //ADC1 VIR PGA */
	SND_SOC_DAPM_PGA("ADC1 VIR PGA", ANA_ADC1_CTRL1, ADC1_DSM_ENABLE, 0,
			NULL, 0),

	/* //ADC2 VIR PGA */
	SND_SOC_DAPM_PGA("ADC2 VIR PGA", ANA_ADC2_CTRL1, ADC2_DSM_ENABLE, 0,
			NULL, 0),

	/* //ADC3 VIR PGA */
	SND_SOC_DAPM_PGA("ADC3 VIR PGA", ANA_ADC3_CTRL1, ADC3_DSM_ENABLE, 0,
			NULL, 0),

	/* //ADC4 VIR PGA */
	SND_SOC_DAPM_PGA("ADC4 VIR PGA", ANA_ADC4_CTRL1, ADC4_DSM_ENABLE, 0,
			NULL, 0),

	/* //ADC1 DIG MIXER */
	SND_SOC_DAPM_MIXER("ADC1 DIG MIXER", SND_SOC_NOPM, 0, 0,
			adc1_digital_src_mixer,
			ARRAY_SIZE(adc1_digital_src_mixer)),

	/* //ADC2 DIG MIXER */
	SND_SOC_DAPM_MIXER("ADC2 DIG MIXER", SND_SOC_NOPM, 0, 0,
			adc2_digital_src_mixer,
			ARRAY_SIZE(adc2_digital_src_mixer)),

	/* //ADC3 DIG MIXER */
	SND_SOC_DAPM_MIXER("ADC3 DIG MIXER", SND_SOC_NOPM, 0, 0,
			adc3_digital_src_mixer,
			ARRAY_SIZE(adc3_digital_src_mixer)),

	/* //ADC4 DIG MIXER */
	SND_SOC_DAPM_MIXER("ADC4 DIG MIXER", SND_SOC_NOPM, 0, 0,
			adc4_digital_src_mixer,
			ARRAY_SIZE(adc4_digital_src_mixer)),

	/* //I2S TX1 CH1 MUX */
	SND_SOC_DAPM_MUX("I2S TX1 CH1 MUX", SND_SOC_NOPM, 0, 0,
			&i2s_tx1_ch1_map_mux),

	/* //I2S TX1 CH2 MUX */
	SND_SOC_DAPM_MUX("I2S TX1 CH2 MUX", SND_SOC_NOPM, 0, 0,
			&i2s_tx1_ch2_map_mux),

	/* //I2S TX1 CH3 MUX */
	SND_SOC_DAPM_MUX("I2S TX1 CH3 MUX", SND_SOC_NOPM, 0, 0,
			&i2s_tx1_ch3_map_mux),

	/* //I2S TX1 CH4 MUX */
	SND_SOC_DAPM_MUX("I2S TX1 CH4 MUX", SND_SOC_NOPM, 0, 0,
			&i2s_tx1_ch4_map_mux),

	/* //AIF OUT -> (stream widget, stname must be same with codec \
	 * dai_driver stream_name, which will be used to build dai widget) */
	SND_SOC_DAPM_AIF_OUT("AIF ADC OUT", "Capture", 0, SND_SOC_NOPM, 0, 0),
};

/***************** DAPM routes *******************************************/
/* //ac108 dapm routes */
static const struct snd_soc_dapm_route ac108_dapm_routes[] = {
	/* //MIC */
	{"MIC1 PGA", NULL, "MIC1P"},
	{"MIC1 PGA", NULL, "MIC1N"},

	{"MIC2 PGA", NULL, "MIC2P"},
	{"MIC2 PGA", NULL, "MIC2N"},

	{"MIC3 PGA", NULL, "MIC3P"},
	{"MIC3 PGA", NULL, "MIC3N"},

	{"MIC4 PGA", NULL, "MIC4P"},
	{"MIC4 PGA", NULL, "MIC4N"},

	/* //DMIC */
	{"DMIC1L PGA", NULL, "DMIC1"},
	{"DMIC1R PGA", NULL, "DMIC1"},

	{"DMIC2L PGA", NULL, "DMIC2"},
	{"DMIC2R PGA", NULL, "DMIC2"},

	/* //ADC1 DIG MUX */
	{"ADC1 DIG MUX", "ADC1 switch", "MIC1 PGA"},
	{"ADC1 DIG MUX", "ADC2 switch", "MIC2 PGA"},
	{"ADC1 DIG MUX", "ADC3 switch", "MIC3 PGA"},
	{"ADC1 DIG MUX", "ADC4 switch", "MIC4 PGA"},

	/* //ADC2 DIG MUX */
	{"ADC2 DIG MUX", "ADC1 switch", "MIC1 PGA"},
	{"ADC2 DIG MUX", "ADC2 switch", "MIC2 PGA"},
	{"ADC2 DIG MUX", "ADC3 switch", "MIC3 PGA"},
	{"ADC2 DIG MUX", "ADC4 switch", "MIC4 PGA"},

	/* //ADC3 DIG MUX */
	{"ADC3 DIG MUX", "ADC1 switch", "MIC1 PGA"},
	{"ADC3 DIG MUX", "ADC2 switch", "MIC2 PGA"},
	{"ADC3 DIG MUX", "ADC3 switch", "MIC3 PGA"},
	{"ADC3 DIG MUX", "ADC4 switch", "MIC4 PGA"},

	/* //ADC4 DIG MUX */
	{"ADC4 DIG MUX", "ADC1 switch", "MIC1 PGA"},
	{"ADC4 DIG MUX", "ADC2 switch", "MIC2 PGA"},
	{"ADC4 DIG MUX", "ADC3 switch", "MIC3 PGA"},
	{"ADC4 DIG MUX", "ADC4 switch", "MIC4 PGA"},

	/* //ADC12 DMIC1 MUX */
	{"ADC12 DMIC1 MUX", "ADC12 switch", "ADC1 DIG MUX"},
	{"ADC12 DMIC1 MUX", "ADC12 switch", "ADC2 DIG MUX"},
	{"ADC12 DMIC1 MUX", "DMIC1 switch", "DMIC1L PGA"},
	{"ADC12 DMIC1 MUX", "DMIC1 switch", "DMIC1R PGA"},

	/* //ADC34 DMIC2 MUX */
	{"ADC34 DMIC2 MUX", "ADC34 switch", "ADC3 DIG MUX"},
	{"ADC34 DMIC2 MUX", "ADC34 switch", "ADC4 DIG MUX"},
	{"ADC34 DMIC2 MUX", "DMIC2 switch", "DMIC2L PGA"},
	{"ADC34 DMIC2 MUX", "DMIC2 switch", "DMIC2R PGA"},

	/* //ADC1 VIR PGA */
	{"ADC1 VIR PGA", NULL, "ADC12 DMIC1 MUX"},

	/* //ADC2 VIR PGA */
	{"ADC2 VIR PGA", NULL, "ADC12 DMIC1 MUX"},

	/* //ADC3 VIR PGA */
	{"ADC3 VIR PGA", NULL, "ADC34 DMIC2 MUX"},

	/* //ADC4 VIR PGA */
	{"ADC4 VIR PGA", NULL, "ADC34 DMIC2 MUX"},

	/* //ADC1 DIG MIXER */
	{"ADC1 DIG MIXER", "ADC1 DAT switch", "ADC1 VIR PGA"},
	{"ADC1 DIG MIXER", "ADC2 DAT switch", "ADC2 VIR PGA"},
	{"ADC1 DIG MIXER", "ADC3 DAT switch", "ADC3 VIR PGA"},
	{"ADC1 DIG MIXER", "ADC4 DAT switch", "ADC4 VIR PGA"},

	/* //ADC2 DIG MIXER */
	{"ADC2 DIG MIXER", "ADC1 DAT switch", "ADC1 VIR PGA"},
	{"ADC2 DIG MIXER", "ADC2 DAT switch", "ADC2 VIR PGA"},
	{"ADC2 DIG MIXER", "ADC3 DAT switch", "ADC3 VIR PGA"},
	{"ADC2 DIG MIXER", "ADC4 DAT switch", "ADC4 VIR PGA"},

	/* //ADC3 DIG MIXER */
	{"ADC3 DIG MIXER", "ADC1 DAT switch", "ADC1 VIR PGA"},
	{"ADC3 DIG MIXER", "ADC2 DAT switch", "ADC2 VIR PGA"},
	{"ADC3 DIG MIXER", "ADC3 DAT switch", "ADC3 VIR PGA"},
	{"ADC3 DIG MIXER", "ADC4 DAT switch", "ADC4 VIR PGA"},

	/* //ADC4 DIG MIXER */
	{"ADC4 DIG MIXER", "ADC1 DAT switch", "ADC1 VIR PGA"},
	{"ADC4 DIG MIXER", "ADC2 DAT switch", "ADC2 VIR PGA"},
	{"ADC4 DIG MIXER", "ADC3 DAT switch", "ADC3 VIR PGA"},
	{"ADC4 DIG MIXER", "ADC4 DAT switch", "ADC4 VIR PGA"},

	/* //I2S TX1 CH1 MUX */
	{"I2S TX1 CH1 MUX", "ADC1 Sample switch", "ADC1 DIG MIXER"},
	{"I2S TX1 CH1 MUX", "ADC2 Sample switch", "ADC2 DIG MIXER"},
	{"I2S TX1 CH1 MUX", "ADC3 Sample switch", "ADC3 DIG MIXER"},
	{"I2S TX1 CH1 MUX", "ADC4 Sample switch", "ADC4 DIG MIXER"},

	/* //I2S TX1 CH2 MUX */
	{"I2S TX1 CH2 MUX", "ADC1 Sample switch", "ADC1 DIG MIXER"},
	{"I2S TX1 CH2 MUX", "ADC2 Sample switch", "ADC2 DIG MIXER"},
	{"I2S TX1 CH2 MUX", "ADC3 Sample switch", "ADC3 DIG MIXER"},
	{"I2S TX1 CH2 MUX", "ADC4 Sample switch", "ADC4 DIG MIXER"},

	/* //I2S TX1 CH3 MUX */
	{"I2S TX1 CH3 MUX", "ADC1 Sample switch", "ADC1 DIG MIXER"},
	{"I2S TX1 CH3 MUX", "ADC2 Sample switch", "ADC2 DIG MIXER"},
	{"I2S TX1 CH3 MUX", "ADC3 Sample switch", "ADC3 DIG MIXER"},
	{"I2S TX1 CH3 MUX", "ADC4 Sample switch", "ADC4 DIG MIXER"},

	/* //I2S TX1 CH4 MUX */
	{"I2S TX1 CH4 MUX", "ADC1 Sample switch", "ADC1 DIG MIXER"},
	{"I2S TX1 CH4 MUX", "ADC2 Sample switch", "ADC2 DIG MIXER"},
	{"I2S TX1 CH4 MUX", "ADC3 Sample switch", "ADC3 DIG MIXER"},
	{"I2S TX1 CH4 MUX", "ADC4 Sample switch", "ADC4 DIG MIXER"},

	/* //AIF OUT */
	{"AIF ADC OUT", NULL, "I2S TX1 CH1 MUX"},
	{"AIF ADC OUT", NULL, "I2S TX1 CH2 MUX"},
	{"AIF ADC OUT", NULL, "I2S TX1 CH3 MUX"},
	{"AIF ADC OUT", NULL, "I2S TX1 CH4 MUX"},
};

static int ac108_read(u8 reg, u8 *rt_value, struct i2c_client *client)
{
	int ret;
	u8 read_cmd[3] = { 0 };
	u8 cmd_len = 0;

	read_cmd[0] = reg;
	cmd_len = 1;

	if (client->adapter == NULL)
		pr_err("ac108_read client->adapter==NULL\n");

	ret = i2c_master_send(client, read_cmd, cmd_len);
	if (ret != cmd_len) {
		pr_err("ac108_read error1\n");
		return -1;
	}

	ret = i2c_master_recv(client, rt_value, 1);
	if (ret != 1) {
		pr_err("ac108_read error2, ret = %d.\n", ret);
		return -1;
	}

	return 0;
}

static int ac108_write(u8 reg, unsigned char value, struct i2c_client *client)
{
	int ret = 0;
	u8 write_cmd[2] = { 0 };

	write_cmd[0] = reg;
	write_cmd[1] = value;

	ret = i2c_master_send(client, write_cmd, 2);
	if (ret != 2) {
		pr_err("ac108_write error->[REG-0x%02x,val-0x%02x]\n", reg,
				value);
		return -1;
	}

	return 0;
}

static int ac108_update_bits(u8 reg, u8 mask, u8 value,
		struct i2c_client *client)
{
	u8 val_old, val_new;

	ac108_read(reg, &val_old, client);
	val_new = (val_old & ~mask) | (value & mask);
	if (val_new != val_old)
		ac108_write(reg, val_new, client);

	return 0;
}

static int ac108_multi_chips_read(u8 reg, unsigned char *rt_value)
{
	u8 i;

	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++)
		ac108_read(reg, rt_value++, i2c_clt[i]);

	return 0;
}

static int ac108_multi_chips_write(u8 reg, unsigned char value)
{
	u8 i;

	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++)
		ac108_write(reg, value, i2c_clt[i]);

	return 0;
}

static int ac108_multi_chips_update_bits(u8 reg, u8 mask, u8 value)
{
	u8 i;

	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++)
		ac108_update_bits(reg, mask, value, i2c_clt[i]);

	return 0;
}

static void ac108_set_vol(struct i2c_client *i2c)
{
	u32 i;
	for (i = 0; i < 4; i++) {
		if ((ac108_pub_cfg.ref_chip_addr == i2c->addr) && (ac108_pub_cfg.ref_channel_num == i)) {
			ac108_write(ANA_PGA1_CTRL + i, ac108_pub_cfg.ref_pga_gain, i2c);
			if (ac108_pub_cfg.pa_double_used == 1) {
				i++;
				ac108_write(ANA_PGA1_CTRL + i, ac108_pub_cfg.ref_pga_gain, i2c);
			}
		} else {
			ac108_write(ANA_PGA1_CTRL + i, ac108_pub_cfg.pga_gain, i2c);
		}
	}
}


static void ac108_hw_init(struct i2c_client *i2c)
{
	/*** Chip reset ***/
	/* //ac108_write(CHIP_AUDIO_RST, 0x12, i2c);
	 * 0x00=0x12: resets all register to their default state*/
	struct ac108_priv *ac108 = dev_get_drvdata(&i2c->dev);
#if !AC108_DMIC_EN
	/*** Analog voltage enable ***/
	ac108_write(PWR_CTRL6, 0x01, i2c);/*0x06=0x01: Enable Analog LDO */
	ac108_write(PWR_CTRL7, 0x9b, i2c);
	/*0x07=0x9b: VREF faststart Enable, Enable VREF @ 3.4V (5V) or
	 * 3.1V (3.3V) (needed for Analog LDO and MICBIAS) */
	ac108_write(PWR_CTRL9, 0x81, i2c);
	/*0x09=0x81: VREFP faststart Enable, Enable VREFP
	 * (needed by all audio input channels) */

	ac108_write(ANA_ADC3_CTRL7, 0x03, i2c);
#endif

	/*** SYSCLK Config ***/
	ac108_write(MOD_CLK_EN, 0x93, i2c);
	/*0x21=0x93: Module clock enable<I2S, ADC digital,
	 * MIC offset Calibration, ADC analog> */
	ac108_write(MOD_RST_CTRL, 0x93, i2c);
	/*0x22=0x93: Module reset de-asserted<I2S, ADC digital,
	 * MIC offset Calibration, ADC analog> */

	/*** I2S Common Config ***/
	ac108_update_bits(I2S_CTRL, 0x1 << SDO1_EN | 0x1 << GEN,\
			0x1 << SDO1_EN | 0x1 << GEN, i2c);
	/*SDO1 enable, Globe Enable */

	ac108_update_bits(I2S_BCLK_CTRL, 0x1 << EDGE_TRANSFER, \
			0x0 << EDGE_TRANSFER, i2c);
	/*SDO drive data and SDI sample data at the different BCLK edge */

	ac108_update_bits(I2S_LRCK_CTRL1, 0x3 << LRCK_PERIODH,
			(((ac108_pub_cfg.ac108_nums * ac108_pub_cfg.slot_width) * 2 - 1) >> 8) << LRCK_PERIODH, i2c);
	ac108_write(I2S_LRCK_CTRL2, (u8) ((ac108_pub_cfg.ac108_nums * ac108_pub_cfg.slot_width) * 2 - 1), i2c);
	/*config LRCK period: 16bit * 8ch = 128,
	 * 32bit * 8ch = 256, 32bit *16ch =512 */

	ac108_update_bits(I2S_FMT_CTRL1,
			0x1 << ENCD_SEL | 0x1 << TX_SLOT_HIZ | 0x1 << \
			TX_STATE, !!AC108_ENCODING_EN << ENCD_SEL | \
			0x0 << TX_SLOT_HIZ \
			| 0x1 << TX_STATE, i2c);

	ac108_update_bits(I2S_FMT_CTRL2, 0x7 << SLOT_WIDTH_SEL,\
			(AC108_SLOT_WIDTH / 4 - 1) << SLOT_WIDTH_SEL, i2c);
	/*16bit or 32bit Slot Width */

	ac108_write(I2S_FMT_CTRL3, 0x70, i2c);
	/*0x36=0x70: TX MSB first, TX2 Mute,
	 * LRCK = 1 BCLK width (short frame), Linear PCM Data Mode */
	/* //Transfer 0 after each sample in each
	 * slot(sample resolution < slot width), */

	ac108_write(I2S_TX1_CHMP_CTRL1, 0xe4, i2c);
	/*0x3C=0xe4: TX1 CHn Map to CHn adc sample, n=[1,4] */
	ac108_write(I2S_TX1_CHMP_CTRL2, 0xe4, i2c);
	/*0x3D=0xe4: TX1 CHn Map to CH(n-4) adc sample, n=[5,8] */
	ac108_write(I2S_TX1_CHMP_CTRL3, 0xe4, i2c);
	/*0x3E=0xe4: TX1 CHn Map to CH(n-8) adc sample, n=[9,12] */
	ac108_write(I2S_TX1_CHMP_CTRL4, 0xe4, i2c);
	/*0x3F=0xe4: TX1 CHn Map to CH(n-12) adc sample, n=[13,16] */

	/*** ADC DIG part Config***/
	ac108_write(ADC_DIG_EN, 0x1f, i2c);
	/*0x61=0x1f: Digital part globe enable, ADCs digital part enable */
	if (ac108_pub_cfg.debug_mode) {
		ac108_write(HPF_EN, 0x00, i2c);
		ac108_write(ADC_DIG_DEBUG, 0x01, i2c);
	}
	ac108_write(ANA_ADC4_CTRL7, 0x0f, i2c);
	/*0xBB=0x0f: Gating ADCs CLK de-asserted (ADCs CLK Enable) */

#if 0
	/*** ADCs analog PGA gain Config***/
	if (i2c->addr == 0x3b) {
		ac108_write(ANA_PGA1_CTRL, ac108->pga_gain, i2c);
		/*0x90=0x3d: ADC1 PGA gain 30.5dB */
		ac108_write(ANA_PGA2_CTRL, ac108->pga_gain, i2c);
		/*0x91=0x3d: ADC2 PGA gain 30.5dB */
#ifdef CONFIG_SND_MOZART_SINGLE_AC108
		ac108_write(ANA_PGA3_CTRL, ac108->pga_gain, i2c);
#else
		ac108_write(ANA_PGA3_CTRL, ac108->ref_pga_gain, i2c);
#endif
		/*0x92=0x3d: ADC3 PGA gain 30.5dB */
		ac108_write(ANA_PGA4_CTRL, ac108->ref_pga_gain, i2c);
		/*0x93=0x3d: ADC4 PGA gain 30.5dB */
	} else if (i2c->addr == 0x35) {
		ac108_write(ANA_PGA1_CTRL, ac108->pga_gain, i2c);
		/*0x90=0x3d: ADC1 PGA gain 30.5dB */
		ac108_write(ANA_PGA2_CTRL, ac108->pga_gain, i2c);
		/*0x91=0x3d: ADC2 PGA gain 30.5dB */
		ac108_write(ANA_PGA3_CTRL, ac108->pga_gain, i2c);
		/*0x92=0x3d: ADC3 PGA gain 30.5dB */
		ac108_write(ANA_PGA4_CTRL, ac108->pga_gain, i2c);
		/*0x93=0x3d: ADC4 PGA gain 30.5dB */
	}
#endif
#if !AC108_DMIC_EN
	ac108_set_vol(i2c);
	/*** enable AAF/ADC/PGA  and UnMute Config ***/
	ac108_write(ANA_ADC1_CTRL1, 0x07, i2c);
	/*0xA0=0x07: ADC1 AAF & ADC enable, ADC1 PGA enable,
	 * ADC1 MICBIAS enable and UnMute */
	ac108_write(ANA_ADC2_CTRL1, 0x07, i2c);
	/*0xA7=0x07: ADC2 AAF & ADC enable, ADC2 PGA enable,
	 * ADC2 MICBIAS enable and UnMute */
	ac108_write(ANA_ADC3_CTRL1, 0x07, i2c);
	/*0xAE=0x07: ADC3 AAF & ADC enable, ADC3 PGA enable,
	 * ADC3 MICBIAS enable and UnMute */
	ac108_write(ANA_ADC4_CTRL1, 0x07, i2c);
	/*0xB5=0x07: ADC4 AAF & ADC enable, ADC4 PGA enable,
	 * ADC4 MICBIAS enable and UnMute */

	/* //ac108_update_bits(PWR_CTRL7, 0x1<<VREF_FASTSTART_ENABLE,
	 * 0x0<<VREF_FASTSTART_ENABLE, i2c);            VREF faststart disable*/
	/*ac108_update_bits(PWR_CTRL9, 0x1<<VREFP_FASTSTART_ENABLE,
	 * 0x0<<VREFP_FASTSTART_ENABLE, i2c);  VREFP faststart disable*/
#else
	/*DMIC module Enable */
	ac108_write(DMIC_EN, 0x03, i2c);
	/*DMIC1/2 Enable, while ADC DIG source select DMIC1/2 */
	ac108_write(GPIO_CFG1, 0xee, i2c);
	/*GPIO1 as DMIC1_DAT, GPIO2 as DMIC_CLK */
	ac108_write(GPIO_CFG2, 0x7e, i2c);
	/*GPIO3 as DMIC2_DAT */
#endif
}

static int ac108_set_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	AC108_DEBUG("\n--->%s\n", __func__);

	switch (clk_id) {
	case SYSCLK_SRC_MCLK:
		AC108_DEBUG("AC108 SYSCLK source select MCLK\n\n");
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x1 << SYSCLK_SRC,\
				0x0 << SYSCLK_SRC);
		/* //System Clock Source Select MCLK */
		break;
	case SYSCLK_SRC_PLL:
		AC108_DEBUG("AC108 SYSCLK source select PLL\n\n");
		ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x1 << SYSCLK_SRC,\
				0x1 << SYSCLK_SRC);
		/* //System Clock Source Select PLL */
		break;
	default:
		pr_err("AC108 SYSCLK source config error:%d\n\n", clk_id);
		return -EINVAL;
	}

	/* //SYSCLK Enable */
	ac108_multi_chips_update_bits(SYSCLK_CTRL, 0x1 << SYSCLK_EN,
			0x1 << SYSCLK_EN);
	return 0;
}

static int ac108_set_pll(struct snd_soc_dai *dai, int pll_id, int source,
		unsigned int freq_in, unsigned int freq_out)
{
	AC108_DEBUG("\n--->%s\n", __func__);
	u32 i, m1, m2, n, k1, k2;

	if (!freq_out)
		return 0;

	if (freq_in < 128000 || freq_in > 24576000) {
		pr_err("AC109 PLLCLK source input freq only "
				"support [128K,24M],while now %u\n\n", \
				freq_in);
		return -EINVAL;
	} else if ((freq_in == 24576000 || freq_in == 22579200)
			&& pll_id == SYSCLK_SRC_MCLK) {
		/* //System Clock Source Select MCLK, SYSCLK Enable */
		AC108_DEBUG("AC108 don't need to use PLL\n\n");
		ac108_multi_chips_update_bits(SYSCLK_CTRL,
				0x1 << SYSCLK_SRC | 0x1 <<
				SYSCLK_EN,
				0x0 << SYSCLK_SRC | 0x1 <<
				SYSCLK_EN);
		return 0;
		/* //Don't need to use PLL */
	}
	/* //PLL Clock Source Select */
	switch (pll_id) {
	case PLLCLK_SRC_MCLK:
			AC108_DEBUG("AC108 PLLCLK input source select MCLK\n");
			ac108_multi_chips_update_bits(
					SYSCLK_CTRL, 0x3 << PLLCLK_SRC,
					0x0 << PLLCLK_SRC);
			break;
	case PLLCLK_SRC_BCLK:
			AC108_DEBUG("AC108 PLLCLK input source select BCLK\n");
			ac108_multi_chips_update_bits(
					SYSCLK_CTRL, 0x3 << PLLCLK_SRC,
					0x1 << PLLCLK_SRC);
			break;
	case PLLCLK_SRC_GPIO2:
			AC108_DEBUG("AC108 PLLCLK input source select GPIO2\n");
			ac108_multi_chips_update_bits(
					SYSCLK_CTRL, 0x3 << PLLCLK_SRC,
					0x2 << PLLCLK_SRC);
			break;
	case PLLCLK_SRC_GPIO3:
			AC108_DEBUG("AC108 PLLCLK input source select GPIO3\n");
			ac108_multi_chips_update_bits(
					SYSCLK_CTRL, 0x3 << PLLCLK_SRC,
					0x3 << PLLCLK_SRC);
			break;
	default:
		pr_err("AC108 PLLCLK source config error:%d\n\n", pll_id);
			return -EINVAL;
	}

	/* //FOUT =(FIN * N) / [(M1+1) * (M2+1)*(K1+1)*(K2+1)] ; */
	for (i = 0; i < ARRAY_SIZE(ac108_pll_div); i++) {
		if (ac108_pll_div[i].freq_in == freq_in
				&& ac108_pll_div[i].freq_out == freq_out) {
			m1 = ac108_pll_div[i].m1;
			m2 = ac108_pll_div[i].m2;
			n = ac108_pll_div[i].n;
			k1 = ac108_pll_div[i].k1;
			k2 = ac108_pll_div[i].k2;
			AC108_DEBUG
				("AC108 PLL freq_in match:%u, freq_out:%u\n\n",
				 freq_in, freq_out);
			break;
		}
	}

	if (i == ARRAY_SIZE(ac108_pll_div)) {
		pr_err("AC108 don't match PLLCLK "
				"freq_in and freq_out table\n\n");
		return -EINVAL;
	}
	/* //Config PLL DIV param M1/M2/N/K1/K2 */
	ac108_multi_chips_update_bits(PLL_CTRL2,
			0x1f << PLL_PREDIV1 | 0x1 << PLL_PREDIV2,
			m1 << PLL_PREDIV1 | m2 << PLL_PREDIV2);
	ac108_multi_chips_update_bits(PLL_CTRL3, 0x3 << PLL_LOOPDIV_MSB,
			(n >> 8) << PLL_LOOPDIV_MSB);
	ac108_multi_chips_update_bits(PLL_CTRL4, 0xff << PLL_LOOPDIV_LSB,
			(u8) n << PLL_LOOPDIV_LSB);
	ac108_multi_chips_update_bits(PLL_CTRL5,
			0x1f << PLL_POSTDIV1 | 0x1 <<
			PLL_POSTDIV2,
			k1 << PLL_POSTDIV1 | k2 << PLL_POSTDIV2);

	/* //PLL module enable */
	ac108_multi_chips_update_bits(PLL_LOCK_CTRL, 0x1 << PLL_LOCK_EN,\
			0x1 << PLL_LOCK_EN);
	/* //PLL CLK lock enable */
	ac108_multi_chips_update_bits(PLL_CTRL1, 0x1 << PLL_EN |\
			0x1 << PLL_COM_EN, 0x1 << PLL_EN | 0x1 << PLL_COM_EN);
	/* //PLL Common voltage Enable, PLL Enable */

	/* //PLLCLK Enable, System Clock Source Select PLL, SYSCLK Enable */
	ac108_multi_chips_update_bits(SYSCLK_CTRL,
			0x1 << PLLCLK_EN | 0x1 << SYSCLK_SRC | 0x1
			<< SYSCLK_EN,
			0x1 << PLLCLK_EN | 0x1 << SYSCLK_SRC | 0x1
			<< SYSCLK_EN);

	return 0;
}

static int ac108_set_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	/* AC108_DEBUG("\n--->%s\n", __func__); */
	u32 i, bclk_div, bclk_div_reg_val;

	if (!div_id) {
		/* //use div_id to judge Master/Slave mode,
		 * 0: Slave mode, 1: Master mode */
		AC108_DEBUG("AC108 work as Slave mode, don't need to config BCLK_DIV\n\n");
		return 0;
	}

	bclk_div = div / (ac108_pub_cfg.ac108_nums * ac108_pub_cfg.slot_width * (AC108_ENCODING_EN ?\
				(ac108_pub_cfg.ac108_nums * 4 + 1) / 2 : 1));

	for (i = 0; i < ARRAY_SIZE(ac108_bclk_div); i++) {
		if (ac108_bclk_div[i].real_val == bclk_div) {
			bclk_div_reg_val = ac108_bclk_div[i].reg_val;
			AC108_DEBUG("AC108 set BCLK_DIV_[%u]\n\n", bclk_div);
			break;
		}
	}

	if (i == ARRAY_SIZE(ac108_bclk_div)) {
		pr_err("AC108 don't support BCLK_DIV_[%u]\n\n", bclk_div);
		return -EINVAL;
	}
	/* //AC108 set BCLK DIV */
	ac108_bclk_div_val = bclk_div_reg_val;
#if !AC108_ENCODING_EN
	/* //TX Encoding mode wil config BCLK_DIV at trigger function */
	ac108_multi_chips_update_bits(I2S_BCLK_CTRL, 0xf << BCLKDIV,
			bclk_div_reg_val << BCLKDIV);
#endif
	return 0;
}

static int ac108_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	AC108_DEBUG("\n--->%s\n", __func__);
	u8 i, tx_offset, i2s_mode, lrck_polarity, brck_polarity;
	struct ac108_priv *ac108 = dev_get_drvdata(dai->dev);

	/* //AC108 config Master/Slave mode */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		/* //AC108 Master */
		AC108_DEBUG("AC108 set to work as Master\n");
		ac108_update_bits(I2S_CTRL, 0x3 << LRCK_IOEN,\
				0x3 << LRCK_IOEN, ac108->i2c);
		/* //BCLK & LRCK output */
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		/* //AC108 Slave */
		AC108_DEBUG("AC108 set to work as Slave\n");
		ac108_update_bits(I2S_CTRL, 0x3 << LRCK_IOEN,\
				0x0 << LRCK_IOEN, ac108->i2c);
		/* //BCLK & LRCK input */
		break;
	default:
		pr_err("AC108 Master/Slave mode config error:%u\n\n",
				(fmt & SND_SOC_DAIFMT_MASTER_MASK) >> 12);
		return -EINVAL;
	}
	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++) {
		/* //multi_chips: only one chip set as Master,
		 * and the others also need to set as Slave */
		if (i2c_clt[i] == ac108->i2c)
			continue;
		ac108_update_bits(I2S_CTRL, 0x3 << LRCK_IOEN, 0x0 << LRCK_IOEN,
				i2c_clt[i]);
	}

	/* //AC108 config I2S/LJ/RJ/PCM format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		AC108_DEBUG("AC108 config I2S format\n");
		i2s_mode = LEFT_JUSTIFIED_FORMAT;
		tx_offset = 1;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		AC108_DEBUG("AC108 config RIGHT-JUSTIFIED format\n");
		i2s_mode = RIGHT_JUSTIFIED_FORMAT;
		tx_offset = 0;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		AC108_DEBUG("AC108 config LEFT-JUSTIFIED format\n");
		i2s_mode = LEFT_JUSTIFIED_FORMAT;
		tx_offset = 0;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		AC108_DEBUG("AC108 config PCM-A format\n");
		i2s_mode = PCM_FORMAT;
		tx_offset = 1;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		AC108_DEBUG("AC108 config PCM-B format\n");
		i2s_mode = PCM_FORMAT;
		tx_offset = 0;
		break;
	default:
		pr_err("AC108 I2S format config error:%u\n\n",
				fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}
	ac108_multi_chips_update_bits(I2S_FMT_CTRL1,
			0x3 << MODE_SEL | 0x1 << TX2_OFFSET | 0x1
			<< TX1_OFFSET,
			i2s_mode << MODE_SEL | tx_offset <<
			TX2_OFFSET | tx_offset << TX1_OFFSET);

	/* //AC108 config BCLK&LRCK polarity */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
AC108_DEBUG("AC108 config BCLK&LRCK polarity:BCLK_normal,LRCK_normal\n");
			brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
			lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
			break;
	case SND_SOC_DAIFMT_NB_IF:
AC108_DEBUG("AC108 config BCLK&LRCK polarity:BCLK_normal,LRCK_invert\n");
			brck_polarity = BCLK_NORMAL_DRIVE_N_SAMPLE_P;
			lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
			break;
	case SND_SOC_DAIFMT_IB_NF:
AC108_DEBUG("AC108 config BCLK&LRCK polarity:BCLK_invert,LRCK_normal\n");
			brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
			lrck_polarity = LRCK_LEFT_LOW_RIGHT_HIGH;
			break;
	case SND_SOC_DAIFMT_IB_IF:
AC108_DEBUG("AC108 config BCLK&LRCK polarity:BCLK_invert,LRCK_invert\n");
			brck_polarity = BCLK_INVERT_DRIVE_P_SAMPLE_N;
			lrck_polarity = LRCK_LEFT_HIGH_RIGHT_LOW;
			break;
	default:
			pr_err("AC108 config BCLK/LRCLK polarity error:%u\n\n",
					(fmt & SND_SOC_DAIFMT_INV_MASK) >> 8);
			return -EINVAL;
	}
	ac108_multi_chips_update_bits(I2S_BCLK_CTRL, 0x1 << BCLK_POLARITY,
			brck_polarity << BCLK_POLARITY);
	ac108_multi_chips_update_bits(I2S_LRCK_CTRL1, 0x1 << LRCK_POLARITY,
			lrck_polarity << LRCK_POLARITY);

	return 0;
}

static int ac108_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	AC108_DEBUG("\n--->%s\n", __func__);
	u16 i, channels, channels_en, sample_resolution;
	int ret = 0;

	/* //AC108 hw init */
	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++) {
		/* //(params_channels(params)+3)/4 */
		ac108_hw_init(i2c_clt[i]);
	}

	/* //AC108 set sample rate */
	for (i = 0; i < ARRAY_SIZE(ac108_sample_rate); i++) {
		if (ac108_sample_rate[i].real_val == params_rate(params)) {
			ac108_multi_chips_update_bits(ADC_SPRC,
					0xf << ADC_FS_I2S1,
					ac108_sample_rate
					[i].reg_val <<
					ADC_FS_I2S1);
			break;
		}
	}

	/* //AC108 set channels */
	channels = params_channels(params);
#ifdef BPI
	if (channels != ac108_pub_cfg.ac108_nums * 4)
		return -EINVAL;
#else
	printk("BPI: channels = %d\n", channels);
#endif
	for (i = 0; i < (channels + 3) / 4; i++) {
		channels_en =
			(channels >=
			 4 * (i + 1)) ? 0x000f << (4 * i) :
			((1 << (channels % 4)) -
			 1) << (4 * i);
		ac108_write(I2S_TX1_CTRL1, channels - 1, i2c_clt[i]);
		ac108_write(I2S_TX1_CTRL2, (u8) channels_en, i2c_clt[i]);
		ac108_write(I2S_TX1_CTRL3, channels_en >> 8, i2c_clt[i]);
	}
	for (; i < ac108_pub_cfg.ac108_nums; i++) {
		ac108_write(I2S_TX1_CTRL1, 0, i2c_clt[i]);
		ac108_write(I2S_TX1_CTRL2, 0, i2c_clt[i]);
		ac108_write(I2S_TX1_CTRL3, 0, i2c_clt[i]);
	}

	/* //AC108 set sample resorution */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		sample_resolution = 8;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		sample_resolution = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		sample_resolution = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		sample_resolution = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		sample_resolution = 32;
		break;
	default:
		dev_err(dai->dev, \
				"AC108 don't supported the "
			"sample resolution: %u\n", params_format(params));
		return -EINVAL;
	}

#if AC108_ENCODING_EN
	/* if(sample_resolution <= 24 && sample_resolution != 16)
	 * sample_resolution += 4;
	 * //TX Encoding mode, SR add 4bits to mark channel number */
#endif
	for (i = 0; i < ARRAY_SIZE(ac108_sample_resolution); i++) {
		if (ac108_sample_resolution[i].real_val == sample_resolution) {
			ac108_multi_chips_update_bits(I2S_FMT_CTRL2,
					0x7 << SAMPLE_RESOLUTION,
					ac108_sample_resolution
					[i].reg_val <<
					SAMPLE_RESOLUTION);
			break;
		}
	}

	ac108_multi_chips_update_bits(I2S_CTRL, 0x1 << TXEN, 0x1 << TXEN);
	return 0;
}

static int ac108_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *dai)
{
	AC108_DEBUG("\n--->%s\n", __func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			AC108_DEBUG("AC108 I2S TX block Enable\n\n");
			/* ac108_multi_chips_update_bits
			 * (I2S_CTRL, 0x1<<TXEN, 0x1<<TXEN); */
		} else {
			AC108_DEBUG("AC108 I2S RX block Enable\n\n");
			ac108_multi_chips_update_bits(I2S_CTRL, \
					0x1 << RXEN,
					0x1 << RXEN);
		}
			break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* //Can't disable I2S TX/RX block here, do it in hw free */
		break;

	default:
		pr_err("AC108 trigger cmd error:%u\n\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int ac108_hw_free(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	AC108_DEBUG("\n--->%s\n", __func__);

	AC108_DEBUG("AC108 reset all register to their default value\n\n");
	ac108_multi_chips_write(CHIP_AUDIO_RST, 0x12);
	/* //if config TX Encoding mode, also disable BCLK */

	return 0;
}

/*** define  ac108  dai_ops  struct ***/
static const struct snd_soc_dai_ops ac108_dai_ops = {
	/*DAI clocking configuration */
	.set_sysclk = ac108_set_sysclk,
	.set_pll = ac108_set_pll,
	.set_clkdiv = ac108_set_clkdiv,

	/*ALSA PCM audio operations */
	.hw_params = ac108_hw_params,
	.trigger = ac108_trigger,
	.hw_free = ac108_hw_free,

	/*DAI format configuration */
	.set_fmt = ac108_set_fmt,
};

/*** define  ac108  dai_driver struct ***/
static const struct snd_soc_dai_driver ac108_dai0 = {
	.name = "ac108-pcm0",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_NUM_MAX * 4,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static const struct snd_soc_dai_driver ac108_dai1 = {
	.name = "ac108-pcm1",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_NUM_MAX * 4,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static const struct snd_soc_dai_driver ac108_dai2 = {
	.name = "ac108-pcm2",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_NUM_MAX * 4,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static const struct snd_soc_dai_driver ac108_dai3 = {
	.name = "ac108-pcm3",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = AC108_NUM_MAX * 4,
		.rates = AC108_RATES,
		.formats = AC108_FORMATS,
	},
	.ops = &ac108_dai_ops,
};

static const struct snd_soc_dai_driver *ac108_dai[] = {
	&ac108_dai0,

	&ac108_dai1,

	&ac108_dai2,

	&ac108_dai3,
};

static int ac108_probe(struct snd_soc_codec *codec)
{
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);
	int ret = 0;

	codec->control_data =
		devm_regmap_init_i2c(ac108->i2c, &ac108_regmap_config);
	ret = PTR_RET(codec->control_data);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	ac108->codec = codec;

	return 0;
}

static int ac108_remove(struct snd_soc_codec *codec)
{
	return 0;
}

#ifdef CONFIG_PM
static int ac108_suspend(struct snd_soc_codec *codec)
{
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);

	if (regulator_en && !IS_ERR(ac108_pub_cfg.vol_supply.vcc3v3)) {
		regulator_disable(ac108_pub_cfg.vol_supply.vcc3v3);
	}
	if (regulator_en && gpio_is_valid(ac108_pub_cfg.power_gpio)) {
		gpio_set_value(ac108_pub_cfg.power_gpio, 0);
		gpio_free(ac108_pub_cfg.power_gpio);
	}

	regulator_en = 0;
	return 0;
}

static int ac108_resume(struct snd_soc_codec *codec)
{
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);
	int ret;
	u8 i;
	if (!regulator_en && !IS_ERR(ac108_pub_cfg.vol_supply.vcc3v3)) {
		ret = regulator_enable(ac108_pub_cfg.vol_supply.vcc3v3);
		if (ret != 0)
			pr_err("[AC108] %s: some error happen, "
				" fail to enable regulator!\n",  __func__);
	}
	if (!regulator_en && gpio_is_valid(ac108_pub_cfg.power_gpio)) {
		ret = gpio_request(ac108_pub_cfg.power_gpio, "gpio-power");
		if (ret) {
			pr_debug("failed to request gpio-power gpio\n");
		} else {
				gpio_direction_output(ac108_pub_cfg.power_gpio, 1);
				gpio_set_value(ac108_pub_cfg.power_gpio, 1);
			}
	}
	regulator_en = 1;
	for (i = 0; i < ac108_pub_cfg.ac108_nums; i++)
		ac108_hw_init(i2c_clt[i]);

	return 0;
}

#else

#define ac108_suspend	NULL
#define ac108_resume	NULL

#endif

static unsigned int ac108_codec_read(struct snd_soc_codec *codec,
		unsigned int reg)
{
	/* //AC108_DEBUG("\n--->%s\n",__func__); */
	u8 val_r;
	struct ac108_priv *ac108 = dev_get_drvdata(codec->dev);

	ac108_read(reg, &val_r, ac108->i2c);
	return val_r;
}

static int ac108_codec_write(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	/* //AC108_DEBUG("\n--->%s\n",__func__); */
	ac108_multi_chips_write(reg, value);
	return 0;
}

/*** define  ac108  codec_driver struct ***/
static const struct snd_soc_codec_driver ac108_soc_codec_driver = {
	.probe = ac108_probe,
	.remove = ac108_remove,
	.suspend = ac108_suspend,
	.resume = ac108_resume,

#if AC108_CODEC_RW_TEST_EN
	.read = ac108_codec_read,
	.write = ac108_codec_write,
#endif

#if AC108_DAPM_TEST_EN
	.controls = ac108_controls,
	.num_controls = ARRAY_SIZE(ac108_controls),
	.dapm_widgets = ac108_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ac108_dapm_widgets),
	.dapm_routes = ac108_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(ac108_dapm_routes),
#endif
};

static ssize_t ac108_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int val = 0, flag = 0;
	int ret;
	u8 i = 0, reg, num, value_w, value_r;

	struct ac108_priv *ac108 = dev_get_drvdata(dev);

	ret = kstrtol(buf, 16, &val);
	flag = (val >> 16) & 0xF;

	if (flag) {
		reg = (val >> 8) & 0xFF;
		value_w = val & 0xFF;
		ac108_write(reg, value_w, ac108->i2c);
		printk(KERN_DEBUG "Write 0x%02x to REG:0x%02x\n", value_w, reg);
	} else {
		reg = (val >> 8) & 0xFF;
		num = val & 0xff;
		printk(KERN_DEBUG "\nRead: start "
				"REG:0x%02x,count:0x%02x\n", reg, num);

		do {
			value_r = 0;
			ac108_read(reg, &value_r, ac108->i2c);
			printk(KERN_DEBUG "REG[0x%02x]: 0x%02x;  ",
					reg, value_r);
			reg++;
			i++;
			if ((i == num) || (i % 4 == 0))
				printk(KERN_DEBUG "\n");
		} while (i < num);
	}

	return count;
}

static ssize_t ac108_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
#if 1
	printk(KERN_DEBUG "echo flag|reg|val > ac108\n");
	printk(KERN_DEBUG "eg read star addres=0x06,"
			"count 0x10:echo 0610 >ac108\n");
	printk(KERN_DEBUG "eg write value:0xfe to address: "
		"0x06 :echo 106fe > ac108\n");
	return 0;
#else
	return snprintf(buf, PAGE_SIZE,
			"echo flag|reg|val > ac108\n"
			"eg read star addres=0x06,count 0x10:echo 0610 >ac108\n"
			"eg write value:0xfe to address:0x06 :echo 106fe > ac108\n");
#endif
}

static DEVICE_ATTR(ac108, 0644, ac108_show, ac108_store);

static struct attribute *ac108_debug_attrs[] = {
	&dev_attr_ac108.attr,
	NULL,
};

static struct attribute_group ac108_debug_attr_group = {
	.name = "ac108_debug",
	.attrs = ac108_debug_attrs,
};

static int ac108_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *i2c_id)
{
	struct ac108_priv *ac108;
	struct device_node *np = i2c->dev.of_node;
	char *regulator_name = NULL;
	u32 temp_val;
	u8 reg;
	int ret = 0;
	struct gpio_config config;
	pr_err("%s ,line:%d\n", __func__, __LINE__);

	ac108 = devm_kzalloc(&i2c->dev, sizeof(struct ac108_priv), GFP_KERNEL);
	if (ac108 == NULL) {
		dev_err(&i2c->dev, "Unable to allocate ac108 private data\n");
		return -ENOMEM;
	}

	ac108->i2c = i2c;
	dev_set_drvdata(&i2c->dev, ac108);

	ret = of_property_read_u32(np, "pga_gain", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]pga_gain  configurations missing or invalid.\n");
		ac108_pub_cfg.pga_gain = 0x32;
	} else {
		ac108_pub_cfg.pga_gain = temp_val;
	}

	ret = of_property_read_u32(np, "ref_pga_gain", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]ref_pga_gain configurations missing or invalid.\n");
		ac108_pub_cfg.ref_pga_gain = 0x08;
	} else {
		ac108_pub_cfg.ref_pga_gain = temp_val;
	}

	ret = of_property_read_u32(np, "twi_bus", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]twi_bus configurations missing or invalid.\n");
		ac108_pub_cfg.i2c_bus_num = 0;
	} else {
		ac108_pub_cfg.i2c_bus_num = temp_val;
	}

	ret = of_property_read_u32(np, "debug_mode", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]debug_mode configurations missing or invalid.\n");
		ac108_pub_cfg.debug_mode = 0;
	} else {
		ac108_pub_cfg.debug_mode = temp_val;
	}

	ret = of_property_read_u32(np, "codec_mic_used", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]codec_mic_used configurations missing or invalid.\n");
		ac108_pub_cfg.codec_mic_used = 0;
	} else {
		ac108_pub_cfg.codec_mic_used = temp_val;
	}

	ret = of_property_read_u32(np, "ref_chip_addr", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]ref_chip_addr configurations missing or invalid.\n");
		ac108_pub_cfg.ref_chip_addr = 0;
	} else {
		ac108_pub_cfg.ref_chip_addr = temp_val;
	}

	ret = of_property_read_u32(np, "slot_width", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]slot_width configurations missing or invalid.\n");
		ac108_pub_cfg.slot_width = 0;
	} else {
		ac108_pub_cfg.slot_width = temp_val;
	}

	ret = of_property_read_u32(np, "pa_double_used", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]pa_double_used configurations missing or invalid.\n");
		ac108_pub_cfg.pa_double_used = 0;
	} else {
		ac108_pub_cfg.pa_double_used = temp_val;
	}

	ret = of_property_read_u32(np, "ref_channel_num", &temp_val);
	if (ret < 0) {
		pr_err("[ac108]ref_channel_num configurations missing or invalid.\n");
		ac108_pub_cfg.ref_channel_num = 0;
	} else {
		ac108_pub_cfg.ref_channel_num = temp_val;
	}

	if (!regulator_en) {
		ret = of_property_read_string(np, AC108_REGULATOR_NAME,\
				&ac108_pub_cfg.power_name);
		/* //(const char**) */
		if (ret) {
			pr_err("get ac108 regulator name failed\n");
		} else {
			ac108_pub_cfg.vol_supply.vcc3v3 =
				regulator_get(NULL, ac108_pub_cfg.power_name);
			if (IS_ERR(ac108_pub_cfg.vol_supply.vcc3v3)) {
				pr_err("get ac108 audio-3v3 failed\n");
			} else {
				ret = of_property_read_u32(np, "power_vol", &ac108_pub_cfg.power_vol);
				regulator_set_voltage(ac108_pub_cfg.vol_supply.vcc3v3, ac108_pub_cfg.power_vol,
						ac108_pub_cfg.power_vol);
				ret = regulator_enable(ac108_pub_cfg.vol_supply.vcc3v3);
				if (ret != 0)
					pr_err("[AC108] %s: some error happen, "
					"fail to enable regulator!\n", __func__);
			}
			ac108_pub_cfg.power_gpio = of_get_named_gpio_flags(np, "gpio-power",
							0, (enum of_gpio_flags *)&config);
			if (!gpio_is_valid(ac108_pub_cfg.power_gpio)) {
				pr_err("failed to get power_gpio gpio from dts,power_gpio:%d\n", ac108_pub_cfg.power_gpio);
			} else {
				ret = gpio_request(ac108_pub_cfg.power_gpio, "power_gpio");
				if (ret) {
					pr_err("failed to request power_gpio  gpio\n");
				} else {
					gpio_direction_output(ac108_pub_cfg.power_gpio, 1);
					gpio_set_value(ac108_pub_cfg.power_gpio, 1);
					msleep(3);
				}
			}
			regulator_en = 1;
		}
	}
	/* ac108 detect */
	ac108_read(CHIP_AUDIO_RST, &reg, i2c);
	if (0x4a != reg)
		return -EFAULT;
	ac108_pub_cfg.ac108_nums++;

	if (i2c_id->driver_data < AC108_NUM_MAX) {
		i2c_clt[ac108_pub_cfg.ac108_nums - 1] = i2c;
		pr_err("%s,line:%d\n", __func__, __LINE__);
		ret = snd_soc_register_codec(&i2c->dev,\
				&ac108_soc_codec_driver,\
				ac108_dai[i2c_id->driver_data], 1);
		/* //(struct snd_soc_dai_driver *) */
		if (ret < 0)
			dev_err(&i2c->dev, \
					"Failed to register "
					"ac108 codec: %d\n", ret);
	} else {
		pr_err("The wrong i2c_id"
			"number :%d\n", (int)(i2c_id->driver_data));
	}

	ret = sysfs_create_group(&i2c->dev.kobj, &ac108_debug_attr_group);
	if (ret)
		pr_err("failed to create attr group\n");

	return ret;
}

static int ac108_i2c_remove(struct i2c_client *i2c)
{
	/* //sysfs_remove_group(&i2c->dev.kobj, &ac108_debug_attr_group); */
	snd_soc_unregister_codec(&i2c->dev);
	return 0;
}

static const unsigned short ac108_i2c_addr[] = {0x3b, 0x35, 0x3c, 0x36, I2C_CLIENT_END,};

static const struct i2c_device_id ac108_i2c_id[] = {
	{"MicArray_0", 0},
	/* //ac108_0 */

	{"MicArray_1", 1},
	/* //ac108_1 */

	{"MicArray_2", 2},
	/* //ac108_2 */

	{"MicArray_3", 3},
	/* //ac108_3 */
	{}
};

MODULE_DEVICE_TABLE(i2c, ac108_i2c_id);

static const struct of_device_id ac108_dt_ids[] = {
	{.compatible = "MicArray_0",},
	/* //ac108_0 */

	{.compatible = "MicArray_1",},
	/* //ac108_1 */

	{.compatible = "MicArray_2",},
	/* //ac108_2 */

	{.compatible = "MicArray_3",},
	/* //ac108_3 */
	{}
};

MODULE_DEVICE_TABLE(of, ac108_dt_ids);

static struct i2c_driver ac108_i2c_driver = {
	.class = I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.driver = {
		.name = "ac108",
		.owner = THIS_MODULE,
		.of_match_table = ac108_dt_ids,
	},
	.probe = ac108_i2c_probe,
	.remove = ac108_i2c_remove,
	.id_table = ac108_i2c_id,
};

static int __init ac108_init(void)
{
	int ret;
	pr_err("Enter ac108_init\n");

	ret = i2c_add_driver(&ac108_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register ac108 i2c driver : %d\n", ret);
	return ret;

}

module_init(ac108_init);

static void __exit ac108_exit(void)
{
	i2c_del_driver(&ac108_i2c_driver);
}

module_exit(ac108_exit);

MODULE_DESCRIPTION("ASoC ac108 codec driver");
MODULE_AUTHOR("panjunwen");
MODULE_LICENSE("GPL");
