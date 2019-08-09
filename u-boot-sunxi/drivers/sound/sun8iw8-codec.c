/*
 * (C) Copyright 2014-2019
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * caiyongheng <caiyongheng@allwinnertech.com>
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

#include "sun8iw8-codec.h"
#include "sunxi_rw_func.h"

DECLARE_GLOBAL_DATA_PTR;

struct sunxi_codec {
	void __iomem *addr_clkbase;
	void __iomem *addr_dbase;
	void __iomem *addr_abase;
	user_gpio_set_t spk_cfg;
	u32 headphone_vol;
	u32 lineout_vol;
	u32 spk_handle;
	u32 pa_ctl_level;
};

static struct sunxi_codec g_codec;

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
	LABEL(SUNXI_DAC_DPC),
	LABEL(SUNXI_DAC_FIFOC),
	LABEL(SUNXI_DAC_FIFOS),
	LABEL(SUNXI_ADC_FIFOC),
	LABEL(SUNXI_ADC_FIFOS),
	LABEL(SUNXI_ADC_RXDATA),
	LABEL(SUNXI_DAC_TXDATA),
	LABEL(SUNXI_DAC_CNT),
	LABEL(SUNXI_ADC_CNT),
	LABEL(SUNXI_DAC_DEBUG),
	LABEL(SUNXI_ADC_DEBUG),

	LABEL(HP_VOLC),
	LABEL(LOMIXSC),
	LABEL(ROMIXSC),
	LABEL(DAC_PA_SRC),
	LABEL(LINEIN_GCTRL),
	LABEL(MIC_GCTR),
	LABEL(HP_CTRL),
	LABEL(LINEOUT_VOLC),
	LABEL(MIC2_CTRL),
	LABEL(BIAS_MIC_CTRL),
	LABEL(LADC_MIX_MUTE),
	LABEL(RADC_MIX_MUTE),
	LABEL(PA_ANTI_POP_CTRL),
	LABEL(AC_ADC_CTRL),
	LABEL(OPADC_CTRL),
	LABEL(OPMIC_CTRL),
	LABEL(ZERO_CROSS_CTRL),
	LABEL(ADC_FUN_CTRL),
	LABEL(CALIBRTAION_CTRL),
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

static const struct sample_rate sample_rate_conv[] = {
	{44100, 0},
	{48000, 0},
	{8000, 5},
	{32000, 1},
	{22050, 2},
	{24000, 2},
	{16000, 3},
	{11025, 4},
	{12000, 4},
	{192000, 6},
	{96000, 7},
};

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

	/*
	 * Audio codec
	 * SUNXI_DAC_FIFO_CTL		0xF0
	 * SUNXI_DAC_DPC		0x00
	 *
	 */
	/* set playback sample resolution, only little endian*/
	switch (sb) {
	case 16:
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(3 << TX_FIFO_MODE), (3 << TX_FIFO_MODE));
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(1 << TASR), (0 << TASR));
		break;
	case 24:
	case 32:
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(3 << TX_FIFO_MODE), (2 << TX_FIFO_MODE));
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(1 << TASR), (1 << TASR));
		break;
	default:
		return -1;
	}
	/* set playback sample rate */
	for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++) {
		if (sample_rate_conv[i].samplerate == sr) {
			sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
						(0x7 << DAC_FS),
						(sample_rate_conv[i].rate_bit << DAC_FS));
		}
	}
	/* set playback channels */
	if (ch == 1) {
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(1 << DAC_MONO_EN), (1 << DAC_MONO_EN));
	} else if (ch == 2) {
		sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
					(1 << DAC_MONO_EN), (0 << DAC_MONO_EN));
	} else {
		printf("channel:%u is not support!\n", ch);
	}
	return 0;
}

static int sunxi_codec_init(struct sunxi_codec *codec)
{
	void __iomem *cfg_reg;
	volatile u32 tmp_val = 0;

	/*
	 * CCMU register
	 * BASE				0x01c2 0000
	 * PLL_AUDIO_CTRL_REG		0x08
	 * PLL_AUDIO_BIAS_REG		0x224
	 * PLL_AUDIO_PAT_CTRL_REG	0x284
	 * BUS_SOFT_RST_REG3		0x2d0
	 * BUS_CLK_GATING_REG2		0x68
	 * AC_DIG_CLK_REG		0x140
	 *
	 */

	cfg_reg = codec->addr_clkbase + 0x08;
	tmp_val = 0x910d0d00;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x224;
	tmp_val = 0x10100000;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x284;
	tmp_val = 0xc000ac02;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x2d0;
	tmp_val = 0x00;
	writel(tmp_val, cfg_reg);
	tmp_val = 0x01;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x68;
	tmp_val = 0x21;
	writel(tmp_val, cfg_reg);

	cfg_reg = codec->addr_clkbase + 0x140;
	tmp_val = 0x80000000;
	writel(tmp_val, cfg_reg);

	/*
	 * Audio Codec
	 *
	 */

	sunxi_codec_update_bits(codec, HP_CTRL,
				(0x3 << HPCOM_FC), (0x0 << HPCOM_FC));
	sunxi_codec_update_bits(codec, HP_CTRL,
				(0x3 << COMPTEN), (0x0 << COMPTEN));
	sunxi_codec_update_bits(codec, HP_CTRL,
				(0x1 << LTRNMUTE), (0x1 << LTRNMUTE));
	sunxi_codec_update_bits(codec, HP_CTRL,
				(0x1 << HPPAEN), (0x1 << HPPAEN));

	sunxi_codec_update_bits(codec, DAC_PA_SRC,
				(0x1 << LHPPAMUTE), (0x1 << LHPPAMUTE));
	sunxi_codec_update_bits(codec, DAC_PA_SRC,
				(0x1 << RHPPAMUTE), (0x1 << RHPPAMUTE));

	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x3 << DRA_LEVEL), (0x3 << DRA_LEVEL));
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1 << DAC_FIFO_FLUSH), (0x1 << DAC_FIFO_FLUSH));
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
				(0x1 << FIR_VERSION), (0x0 << FIR_VERSION));

	sunxi_codec_update_bits(codec, PA_ANTI_POP_CTRL,
			(0x7 << PA_ANTI_POP_CTL), (0x2 << PA_ANTI_POP_CTL));


	/* set digital volume */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DPC,
			(0x3f << DIGITAL_VOL), (0x0 << PA_ANTI_POP_CTL));
	/* set headphone volume */
	sunxi_codec_update_bits(codec, HP_VOLC,
			(0x3f << HPVOL), (codec->headphone_vol << PA_ANTI_POP_CTL));
	/* set lineout volume */
	sunxi_codec_update_bits(codec, LINEOUT_VOLC,
			(0x1f << LINEOUTVOL), (codec->lineout_vol << LINEOUTVOL));

	/*
	 * codec route
	 * DAC_L -> Left Output Mixer  -> SPK_LR Adder -> SPK_L Mux -> SPKL -> External Speaker
	 * DAC_R -> Right Output Mixer -> SPK_LR Adder -> SPK_R Mux -> SPKR -> External Speaker
	 */

	/* enable DAC */
	sunxi_codec_update_bits(codec, SUNXI_DAC_DPC,
			(0x1 << DAC_EN), (1 << DAC_EN));
	/* enable Analog Right channel DAC */
	sunxi_codec_update_bits(codec, DAC_PA_SRC,
			(0x1 << DACAREN), (1 << DACAREN));
	/* enable Analog Right channel DAC */
	sunxi_codec_update_bits(codec, DAC_PA_SRC,
			(0x1 << DACALEN), (1 << DACALEN));
	/* enable Right Output Mixer */
	sunxi_codec_update_bits(codec, DAC_PA_SRC,
			(0x1 << RMIXEN), (1 << RMIXEN));
	/* set DRCR --> Right Output Mixer */
	sunxi_codec_update_bits(codec, ROMIXSC,
			(0x1 << RMIXMUTEDACR), (1 << RMIXMUTEDACR));
	/* enable Left Output Mixer */
	sunxi_codec_update_bits(codec, DAC_PA_SRC,
			(0x1 << LMIXEN), (1 << LMIXEN));
	/* set DRCL --> Left Output Mixer */
	sunxi_codec_update_bits(codec, LOMIXSC,
			(0x1 << LMIXMUTEDACL), (1 << RMIXMUTEDACL));
	/* set left lineout source: left output mixer + right output mixer */
	sunxi_codec_update_bits(codec, MIC2_CTRL,
			(0x1 << LEFTLINEOUTSRC), (1 << LEFTLINEOUTSRC));
	/* set right lineout source: left lineout, for different output */
	sunxi_codec_update_bits(codec, MIC2_CTRL,
			(0x1 << RIGHTLINEOUTSRC), (1 << RIGHTLINEOUTSRC));


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
	codec->addr_clkbase = (void *)0X01C20000;
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
	codec->addr_dbase = (void *)0x01c22c00;
	codec->addr_abase = (void *)(0x01c22c00+0x400);
#endif

	/*printf("dbase:0x%p, abase:0x%p\n", codec->addr_dbase, codec->addr_abase);*/

	ret = fdt_getprop_u32(blob, node, "lineout_vol", &codec->lineout_vol);
	if (ret < 0) {
		printf("lineout_vol not set. default 0x10\n");
		codec->lineout_vol = 0x10;
	}
	ret = fdt_getprop_u32(blob, node, "headphone_vol", &codec->headphone_vol);
	if (ret < 0) {
		printf("headphone_vol not set. default 0x3b\n");
		codec->headphone_vol = 0x3b;
	}
	ret = fdt_getprop_u32(blob, node, "pa_ctl_level", &codec->pa_ctl_level);
	if (ret < 0) {
		printf("pa_ctl_level not set. default 1\n");
		codec->pa_ctl_level = 1;
	}

	ret = fdt_get_one_gpio_by_offset(node, "audio_pa_ctrl", &codec->spk_cfg);
	if (ret < 0 || codec->spk_cfg.port == 0) {
		printf("parser gpio-spk failed, ret:%d, port:%u\n", ret, codec->spk_cfg.port);
		return -1;
	}

	codec->spk_handle = gpio_request(&codec->spk_cfg, 1);
	if (!codec->spk_handle) {
                printf("reuqest gpio for spk failed\n");
                return -1;
        }
	gpio_set_one_pin_io_status(codec->spk_handle, 1, "spk-gpio");

	return 0;
}

int sunxi_codec_playback_prepare(void)
{
	struct sunxi_codec *codec = &g_codec;

	/* FIFO flush, clear pending, clear sample counter */
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1 << DAC_FIFO_FLUSH), (0x1 << DAC_FIFO_FLUSH));
	/* enable DAC FIFO empty DRQ */
	sunxi_codec_update_bits(codec, SUNXI_DAC_FIFOC,
			(0x1 << DAC_DRQ), (0x1 << DAC_DRQ));

	/* enable lineout */
	sunxi_codec_update_bits(codec, MIC2_CTRL,
			(0x1 << LINEOUTRIGHTEN), (0x1 << LINEOUTRIGHTEN));
	sunxi_codec_update_bits(codec, MIC2_CTRL,
			(0x1 << LINEOUTLEFTEN), (0x1 << LINEOUTLEFTEN));

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
		debug("sys_config Codec values failed\n");
		return -1;
	}

	ret = sunxi_codec_init(codec);

	return ret;
}

void sunxi_codec_fill_txfifo(u32 *data)
{
	struct sunxi_codec *codec = &g_codec;

	sunxi_codec_write(codec, SUNXI_DAC_TXDATA, *data);
}

int sunxi_codec_playback_start(ulong handle, u32 *srcBuf, u32 cnt)
{
	int ret = 0;
	struct sunxi_codec *codec = &g_codec;

	flush_cache((unsigned long)srcBuf, cnt);

#if 0
	printf("start dma: form 0x%x to 0x%x  total 0x%x(%u)byte\n",
	       srcBuf, codec->addr_dbase + SUNXI_DAC_TXDATA, cnt, cnt);
#endif
	/*ret = sunxi_dma_start(handle, (uint)srcBuf, codec->addr_dbase + SUNXI_DAC_TXDATA);*/
	ret = sunxi_dma_start(handle, (uint)srcBuf,
			      (uint)(codec->addr_dbase + SUNXI_DAC_TXDATA), cnt);

	return ret;
}
