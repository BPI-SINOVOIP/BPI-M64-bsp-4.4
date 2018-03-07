/*
 * sound\soc\codec\tas5731.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Young <guoyingyang@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include "tas5731.h"
#define TAS5731_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define TAS5731_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
				SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE)
#define SUNXI_CODEC_NAME	"tas5731-codec"
static u32 power_item;
static u32 reset_item;
static u32 amp_poweren;
static int tas5731_power_val;
struct tas5731_priv {
	struct snd_soc_codec *codec;
	struct regmap *regmap;
	struct regulator *vcc_amp;
	struct regulator *vcc_pg;
	struct regulator *vcc_pa;
};

static const struct regmap_config tas5731_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.cache_type = REGCACHE_NONE,
};

static const struct snd_kcontrol_new tas5731_audio_controls[] = {

};

static int tas5731_audio_mute(struct snd_soc_dai *codec_dai, int mute)
{
	return 0;
}

static int tas5731_audio_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	return 0;
}

static void tas5731_audio_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{

}

static int tas5731_audio_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

	} else {

	}
	return 0;
}

static int tas5731_audio_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	return 0;
}

static int tas5731_audio_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int tas5731_audio_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int sample_rate)
{
	return 0;
}

static int tas5731_audio_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static struct snd_soc_dai_ops tas5731_audio_dai_ops = {
	.startup = tas5731_audio_startup,
	.shutdown = tas5731_audio_shutdown,
	.prepare  =	tas5731_audio_prepare,
	.hw_params = tas5731_audio_hw_params,
	.digital_mute = tas5731_audio_mute,
	.set_sysclk = tas5731_audio_set_dai_sysclk,
	.set_clkdiv = tas5731_audio_set_dai_clkdiv,
	.set_fmt = tas5731_audio_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5731_audio_dai = {
	.name = "tas5731_audio",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = TAS5731_RATES,
		.formats = TAS5731_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = TAS5731_RATES,
		.formats = TAS5731_FORMATS,
	},
	/* pcm operations */
	.ops = &tas5731_audio_dai_ops,
};
EXPORT_SYMBOL(tas5731_audio_dai);

struct i2c_client *tas5731_client;

/*en will be 0 or 1*/
void tas5731_power_en(int en)
{
	gpio_set_value(power_item, en);
}

/*en will be 0 or 1*/
void tas5731_set_reset(int en)
{
	gpio_set_value(reset_item, en);
}

static int tas5731_set_power(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	tas5731_power_val = ucontrol->value.integer.value[0];
	gpio_set_value(reset_item, tas5731_power_val);
	pr_info("%s,L:%d, tas5731_power_val:%d\n", __func__, __LINE__, tas5731_power_val);
	return 0;
}

static int tas5731_get_power(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = tas5731_power_val;
	return 0;
}

static const struct snd_kcontrol_new tas5731_controls[] = {
	SOC_SINGLE_BOOL_EXT("tas5731 reset enable", 		0, tas5731_get_power, 	tas5731_set_power),
};
static struct snd_soc_codec *debug_codec;
/***************************************************************************/
static ssize_t tas5731_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val = 0, flag = 0;
	u8 reg, num, i = 0;
	u8 value_w, value_r[128];
	val = simple_strtol(buf, NULL, 16);
	flag = (val >> 24) & 0xF;
	if (flag) {
		reg = (val >> 16) & 0xFF;
		value_w =  val & 0xFF;
		snd_soc_write(debug_codec, reg, value_w);
		printk("write 0x%x to reg:0x%x\n", value_w, reg);
	} else {
		reg = (val>>8) & 0xFF;
		num = val & 0xff;
		printk("\n");
		printk("read:start add:0x%x,count:0x%x\n", reg, num);
		do {
			value_r[i] = snd_soc_read(debug_codec, reg);
			printk("0x%x: 0x%04x ", reg, value_r[i]);
			reg += 1;
			i++;
			if (i == num)
				printk("\n");
			if (i%4 == 0)
				printk("\n");
		} while (i < num);
	}
	return count;
}
static ssize_t tas5731_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo flag|reg|val > tas5731\n");
	printk("eg read star addres=0x06,count 0x10:echo 0610 >tas5731\n");
	printk("eg write value:0x13fe to address:0x06 :echo 10613fe > tas5731\n");
    return 0;
}
static DEVICE_ATTR(tas5731, 0644, tas5731_debug_show, tas5731_debug_store);

static struct attribute *tas5731_debug_attrs[] = {
	&dev_attr_tas5731.attr,
	NULL,
};

static struct attribute_group tas5731_debug_attr_group = {
	.name   = "tas5731_debug",
	.attrs  = tas5731_debug_attrs,
};
static int tas5731_audio_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	ret = snd_soc_add_codec_controls(codec, tas5731_controls,
					ARRAY_SIZE(tas5731_controls));
	if (ret) {
		pr_err("[tas5731] Failed to register audio mode control, "
				"will continue without it.\n");
	}
	debug_codec = codec;

	gpio_direction_output(power_item, 1);
	gpio_set_value(power_item, 0);

	gpio_direction_output(reset_item, 1);
	gpio_set_value(reset_item, 0);

	tas5731_power_en(1);
	msleep(2);
	tas5731_set_reset(1);
	msleep(5);
	tas5731_set_reset(0);
	msleep(2);
	tas5731_set_reset(1);
	msleep(20);

	snd_soc_write(codec, 0x00, 0x60);
	snd_soc_write(codec, 0x1b, 0x00);
	snd_soc_write(codec, 0x06, 0x00);
	snd_soc_write(codec, 0x1a, 0x0a);
	snd_soc_write(codec, 0x0a, 0x30);
	snd_soc_write(codec, 0x09, 0x30);
	snd_soc_write(codec, 0x08, 0x30);
	snd_soc_write(codec, 0x14, 0x54);
	snd_soc_write(codec, 0x13, 0xac);
	snd_soc_write(codec, 0x12, 0x54);
	snd_soc_write(codec, 0x11, 0xac);
	snd_soc_write(codec, 0x0e, 0xd1);
	snd_soc_write(codec, 0x10, 0x07);
	snd_soc_write(codec, 0x1c, 0x02);
	snd_soc_write(codec, 0x19, 0x30);
	snd_soc_write(codec, 0x05, 0x84);
	snd_soc_write(codec, 0x07, 0x40);

	return ret;
}

static int tas5731_audio_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static int tas5731_audio_suspend(struct snd_soc_codec *codec)
{
	struct tas5731_priv *tas5731 = dev_get_drvdata(codec->dev);

	if (gpio_is_valid(power_item)) {
		gpio_set_value(power_item, 0);
		gpio_free(power_item);
	}
	if (gpio_is_valid(reset_item)) {
		gpio_set_value(reset_item, 0);
		gpio_free(reset_item);
	}
	if (gpio_is_valid(amp_poweren)) {
		gpio_set_value(amp_poweren, 0);
		gpio_free(amp_poweren);
	}
	if (!IS_ERR(tas5731->vcc_pg))
		regulator_disable(tas5731->vcc_pg);
	if (!IS_ERR(tas5731->vcc_amp))
		regulator_disable(tas5731->vcc_amp);
	if (!IS_ERR(tas5731->vcc_pa))
		regulator_disable(tas5731->vcc_pa);
	return 0;
}

static int tas5731_audio_resume(struct snd_soc_codec *codec)
{
	struct tas5731_priv *tas5731 = dev_get_drvdata(codec->dev);
	int ret = 0;
	if (!IS_ERR(tas5731->vcc_amp))
		ret = regulator_enable(tas5731->vcc_amp);

	if (!IS_ERR(tas5731->vcc_pg))
		ret = regulator_enable(tas5731->vcc_pg);

	if (!IS_ERR(tas5731->vcc_pa))
		ret = regulator_enable(tas5731->vcc_pa);
	if (gpio_is_valid(amp_poweren)) {
		ret = gpio_request(amp_poweren, "tas5731-poweren");
		if (ret) {
			pr_debug("failed to request tas5731-poweren gpio\n");
		} else {
			gpio_direction_output(amp_poweren, 1);
			gpio_set_value(amp_poweren, 1);
		}
	}

	if (gpio_is_valid(power_item)) {
		ret = gpio_request(power_item, "tas5731_power_en");
		if (ret) {
			pr_debug("failed to request tas5731-power-gpio gpio\n");
		} else {
			gpio_direction_output(power_item, 1);
			gpio_set_value(power_item, 0);
		}
	}

	if (gpio_is_valid(reset_item)) {
		ret = gpio_request(reset_item, "tas5731-gpio-reset");
		if (ret) {
			pr_debug("failed to request tas5731-gpio-reset gpio\n");
		} else {
			gpio_direction_output(reset_item, 1);
			gpio_set_value(reset_item, 0);
		}
	}

	gpio_direction_output(power_item, 1);
	gpio_set_value(power_item, 0);

	gpio_direction_output(reset_item, 1);
	gpio_set_value(reset_item, 0);

	tas5731_power_en(1);
	msleep(2);
	tas5731_set_reset(1);
	msleep(5);
	tas5731_set_reset(0);
	msleep(2);
	tas5731_set_reset(1);
	msleep(20);

	snd_soc_write(codec, 0x00, 0x60);
	snd_soc_write(codec, 0x1b, 0x00);
	snd_soc_write(codec, 0x06, 0x00);
	snd_soc_write(codec, 0x1a, 0x0a);
	snd_soc_write(codec, 0x0a, 0x30);
	snd_soc_write(codec, 0x09, 0x30);
	snd_soc_write(codec, 0x08, 0x30);
	snd_soc_write(codec, 0x14, 0x54);
	snd_soc_write(codec, 0x13, 0xac);
	snd_soc_write(codec, 0x12, 0x54);
	snd_soc_write(codec, 0x11, 0xac);
	snd_soc_write(codec, 0x0e, 0xd1);
	snd_soc_write(codec, 0x10, 0x07);
	snd_soc_write(codec, 0x1c, 0x02);
	snd_soc_write(codec, 0x19, 0x30);
	snd_soc_write(codec, 0x05, 0x84);
	snd_soc_write(codec, 0x07, 0x40);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_tas5731_audio = {
	.probe 		=	tas5731_audio_soc_probe,
	.remove 	=   tas5731_audio_soc_remove,
	.suspend 	= 	tas5731_audio_suspend,
	.resume 	=	tas5731_audio_resume,
	.controls 	= 	tas5731_audio_controls,
	.num_controls = ARRAY_SIZE(tas5731_audio_controls),
};

static int tas5731_probe(struct i2c_client *client,
				      const struct i2c_device_id *id)
{
	int ret = 0;
	tas5731_client = client;
	struct gpio_config config;
	struct device_node *np = tas5731_client->dev.of_node;
	struct tas5731_priv *tas5731;
	char *regulator_name = NULL;

	tas5731 = devm_kzalloc(&tas5731_client->dev, sizeof(struct tas5731_priv), GFP_KERNEL);
	if (tas5731 == NULL)
		return -ENOMEM;

	tas5731->regmap = devm_regmap_init_i2c(tas5731_client, &tas5731_regmap);
	if (IS_ERR(tas5731->regmap)) {
		ret = PTR_ERR(tas5731->regmap);
		dev_err(&tas5731_client->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	power_item = of_get_named_gpio_flags(np, "tas5731_power", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(power_item)) {
		pr_err("tas5731_power return type err\n");
		return -EFAULT;
	}

	/*request power gpio*/
	ret = gpio_request(power_item, "tas5731-gpio-power");
	if (0 != ret) {
		pr_err("request gpio failed!\n");
	}

	/*get the default pa val(close)*/
	reset_item = of_get_named_gpio_flags(np, "tas5731_reset", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(reset_item)) {
		pr_err("tas5731_reset return type err\n");
		return -EFAULT;
	}

	/*request reset gpio*/
	ret = gpio_request(reset_item, "tas5731-gpio-reset");
	if (0 != ret) {
		pr_err("request gpio failed!\n");
	}

	/*get the default pa val(close)*/
	amp_poweren = of_get_named_gpio_flags(np, "amp_poweren", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(amp_poweren)) {
		pr_err("tas5731_poweren return type err\n");
		return -EFAULT;
	}

	/*request reset gpio*/
	ret = gpio_request(amp_poweren, "tas5731-poweren");
	if (0 != ret) {
		pr_err("request gpio failed!\n");
	}

	gpio_direction_output(amp_poweren, 1);
	gpio_set_value(amp_poweren, 1);

	ret = of_property_read_string(np, "regulator_name", &regulator_name);
	if (ret) {
		pr_err("get tas5731 regulator name failed\n");
	} else {
		tas5731->vcc_amp = regulator_get(NULL, regulator_name);
		if (IS_ERR(tas5731->vcc_amp)) {
			pr_err("get tas5731 regulator failed\n");
			return -EFAULT;
		}
		regulator_set_voltage(tas5731->vcc_amp, 3000000, 3000000);
		ret = regulator_enable(tas5731->vcc_amp);
		if (ret != 0)
		pr_err("[tas5731] %s: some error happen, "
				"fail to enable regulator!\n", __func__);
	}
	tas5731->vcc_pg = regulator_get(NULL, "vcc-pg");
	if (IS_ERR(tas5731->vcc_pg)) {
		pr_err("get tas5731 regulator failed\n");
		return -EFAULT;
	}
	regulator_set_voltage(tas5731->vcc_pg, 3000000, 3000000);
	ret = regulator_enable(tas5731->vcc_pg);
	if (ret != 0)
		pr_err("[tas5731] %s: some error happen, "
			"fail to enable regulator!\n", __func__);

	tas5731->vcc_pa = regulator_get(NULL, "vcc-pa");
	if (IS_ERR(tas5731->vcc_pa)) {
		pr_err("get tas5731 regulator failed\n");
		return -EFAULT;
	}
	regulator_set_voltage(tas5731->vcc_pa, 3300000, 3300000);
	ret = regulator_enable(tas5731->vcc_pa);
	if (ret != 0)
		pr_err("[tas5731] %s: some error happen, "
			"fail to enable regulator!\n", __func__);

	i2c_set_clientdata(tas5731_client, tas5731);
	snd_soc_register_codec(&client->dev, &soc_codec_dev_tas5731_audio, &tas5731_audio_dai, 1);
	ret = sysfs_create_group(&client->dev.kobj, &tas5731_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}

	return ret;
}

static int tas5731_remove(struct i2c_client *i2c)
{
	snd_soc_unregister_codec(&i2c->dev);
	return 0;
}

static const struct i2c_device_id sunxi_codec_id[] = {
	{"tas5731_PA", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, sunxi_codec_id);

static const struct of_device_id tas5731_dt_ids[] = {
	{.compatible = "tas5731_PA"},
	{}
};
MODULE_DEVICE_TABLE(of, tas5731_dt_ids);

static struct i2c_driver sunxi_codec_driver = {
	.class 		= I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.id_table 	= sunxi_codec_id,
	.probe 		= tas5731_probe,
	.remove 	= tas5731_remove,
	.driver 	= {
		.owner 	= THIS_MODULE,
		.name 	= "tas5731-codec",
		.of_match_table = tas5731_dt_ids,
	},
};

static int __init tas5731_audio_codec_init(void)
{
	int ret = 0;

	pr_info("%s,l:%d\n", __func__, __LINE__);
	ret = i2c_add_driver(&sunxi_codec_driver);
	return ret;
}
module_init(tas5731_audio_codec_init);

static void __exit tas5731_audio_codec_exit(void)
{
	i2c_del_driver(&sunxi_codec_driver);
}
module_exit(tas5731_audio_codec_exit);

MODULE_DESCRIPTION("SNDVIR_AUDIO ALSA soc codec driver");
MODULE_AUTHOR("Young");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-vir_audio-codec");
