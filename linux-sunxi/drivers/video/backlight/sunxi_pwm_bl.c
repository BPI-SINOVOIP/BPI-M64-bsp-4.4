/*
 * linux/drivers/video/backlight/pwm_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data being correctly configured
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/pwm_backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/sunxi-gpio.h>

struct pwm_bl_data {
	struct pwm_device	*pwm;
	struct device		*dev;
	unsigned int		period;
	unsigned int		lth_brightness;
	unsigned int		*levels;
	bool			enabled;
	struct regulator	*power_supply;
	struct gpio_desc	*enable_gpio;
	unsigned int		scale;
	bool			legacy;
	int			(*notify)(struct device *,
					  int brightness);
	void			(*notify_after)(struct device *,
					int brightness);
	int			(*check_fb)(struct device *, struct fb_info *);
	void			(*exit)(struct device *);
};

static void pwm_backlight_power_on(struct pwm_bl_data *pb, int brightness)
{
	int err;

	if (pb->enabled)
		return;

	if (pb->power_supply) {
		err = regulator_enable(pb->power_supply);
		if (err < 0)
			dev_err(pb->dev, "failed to enable power supply\n");
	}
	
	if (pb->enable_gpio)
		gpiod_set_value(pb->enable_gpio, 1);

	pwm_enable(pb->pwm);
	pb->enabled = true;
}

static void pwm_backlight_power_off(struct pwm_bl_data *pb)
{
	if (!pb->enabled)
		return;

	pwm_config(pb->pwm, 0, pb->period);
	pwm_disable(pb->pwm);

	if (pb->enable_gpio)
		gpiod_set_value(pb->enable_gpio, 0);

	if (pb->power_supply)
		regulator_disable(pb->power_supply);
	
	pb->enabled = false;
}

static int compute_duty_cycle(struct pwm_bl_data *pb, int brightness)
{
	unsigned int lth = pb->lth_brightness;
	int duty_cycle;

	if (pb->levels)
		duty_cycle = pb->levels[brightness];
	else
		duty_cycle = brightness;

	pr_info("%s, brightness = %d, duty_cycle = %d", __func__, brightness, duty_cycle);

	return (duty_cycle * (pb->period - lth) / pb->scale) + lth;
}

static int pwm_backlight_update_status(struct backlight_device *bl)
{
	struct pwm_bl_data *pb = bl_get_data(bl);
	int brightness = bl->props.brightness;
	int duty_cycle;

	pr_info("%s brightness = %d\n", __func__, brightness);

	if (bl->props.power != FB_BLANK_UNBLANK ||
	    bl->props.fb_blank != FB_BLANK_UNBLANK ||
	    bl->props.state & BL_CORE_FBBLANK)
		brightness = 0;

	if (pb->notify)
		brightness = pb->notify(pb->dev, brightness);

	if (brightness > 0) {
		duty_cycle = compute_duty_cycle(pb, brightness);
		pr_info("%s, duty_cycle = %d\n", __func__, duty_cycle);
		pwm_config(pb->pwm, duty_cycle, pb->period);
		pwm_backlight_power_on(pb, brightness);
	} else
		pwm_backlight_power_off(pb);

	if (pb->notify_after)
		pb->notify_after(pb->dev, brightness);

	return 0;
}

static int pwm_backlight_check_fb(struct backlight_device *bl,
				  struct fb_info *info)
{
	struct pwm_bl_data *pb = bl_get_data(bl);

	return !pb->check_fb || pb->check_fb(pb->dev, info);
}

static const struct backlight_ops pwm_backlight_ops = {
	.update_status	= pwm_backlight_update_status,
	.check_fb	= pwm_backlight_check_fb,
};

#ifdef CONFIG_OF
static int pwm_backlight_parse_dt(struct device *dev,
				  struct platform_pwm_backlight_data *data)
{
	struct device_node *node = dev->of_node;
	struct property *prop;
	struct gpio_config config;
	const char *power;
	int length;
	u32 value;
	int ret;

	if (!node)
		return -ENODEV;

	memset(data, 0, sizeof(*data));

	if (of_property_read_string(node, "power_name", &power)) {
		dev_warn(dev, "Missing bl_power regulator\n");
	} else {
		data->power_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if (!data->power_name) {
			ret = -ENOMEM;
		} else
			strcpy(data->power_name, power);
	}
	dev_info(dev, "bl_regulator_name (%s)\n", data->power_name);

	ret = of_property_read_u32(node, "pwm_ch", &value);
	if (ret < 0) {
		dev_err(dev, "failed to get pwm_id\n");
		return ret;
	}
	data->pwm_id = value;

	ret = of_property_read_u32(node, "pwm_pol", &value);
	if (ret < 0) {
		dev_err(dev, "failed to get pwm_pol\n");
		return ret;
	}
	data->polarity = value;

	ret = of_property_read_u32(node, "pwm_freq", &value);
	if (ret < 0) {
		dev_err(dev, "failed to get pwm_freq\n");
		return ret;
	}
	data->pwm_period_ns = 1000 * 1000 * 1000 / value;

	ret = of_property_read_u32(node, "lth_brightness", &value);
	if (ret < 0) {
		dev_err(dev, "failed to get lth_brightness\n");
		return ret;
	}
	data->lth_brightness = value;

	ret = of_property_read_u32(node, "dft_brightness", &value);
	if (ret < 0) {
		dev_err(dev, "failed to get lth_brightness\n");
		return ret;
	}
	data->dft_brightness = value;

	/* backlight power enable gpio */
	data->enable_gpio = of_get_named_gpio_flags(node, "bl_enable", 0,
						(enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->enable_gpio)) {
		dev_err(dev, "get bl_enable gpio failed\n");
	} else {
		dev_info(dev, "bl_enable gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
			 config.gpio, config.mul_sel, config.pull, config.drv_level, config.data);
	}

	/* determine the number of brightness levels */
	prop = of_find_property(node, "brightness-levels", &length);
	if (!prop){
		dev_err(dev, "failed to brightness-levels node\n");
		//return -EINVAL;
	} 
	else {
		data->max_brightness = length / sizeof(u32);

		/* read brightness levels from DT property */
		if (data->max_brightness > 0) {
			size_t size = sizeof(*data->levels) * data->max_brightness;

			data->levels = devm_kzalloc(dev, size, GFP_KERNEL);
			if (!data->levels)
				return -ENOMEM;

			ret = of_property_read_u32_array(node, "brightness-levels",
							 data->levels,
							 data->max_brightness);
			if (ret < 0) {
				dev_err(dev, "failed to brightness-levels value\n");
				return ret;
			}

			ret = of_property_read_u32(node, "default_brightness_level",
						   &value);
			if (ret < 0){
				dev_err(dev, "failed to default_brightness_level value\n");
				return ret;
			}
			data->dft_brightness = value;
			
			data->max_brightness--;
		}
	}

	data->max_brightness = 255;

	pr_info("pwm[%d] polarity[%d] max_brightness[%d] dft_brightness[%d] lth_brightness[%d] pwm_period_ns[%d]\n", 
			data->pwm_id, data->polarity, data->max_brightness, data->dft_brightness, data->lth_brightness, data->pwm_period_ns);
	
	return 0;
}

static struct of_device_id pwm_backlight_of_match[] = {
	{ .compatible = "pwm-backlight" },
	{ }
};

MODULE_DEVICE_TABLE(of, pwm_backlight_of_match);
#else
static int pwm_backlight_parse_dt(struct device *dev,
				  struct platform_pwm_backlight_data *data)
{
	return -ENODEV;
}
#endif

static int pwm_backlight_probe(struct platform_device *pdev)
{
	struct platform_pwm_backlight_data *data = dev_get_platdata(&pdev->dev);
	struct platform_pwm_backlight_data defdata;
	struct backlight_properties props;
	struct backlight_device *bl;
	struct pwm_bl_data *pb;
	int ret;

	dev_info(&pdev->dev, "pwm-backlight probe\n");

	if (!data) {
		ret = pwm_backlight_parse_dt(&pdev->dev, &defdata);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to find platform data\n");
			return ret;
		}

		data = &defdata;
	}

	if (data->init) {
		ret = data->init(&pdev->dev);
		if (ret < 0)
			return ret;
	}

	pb = devm_kzalloc(&pdev->dev, sizeof(*pb), GFP_KERNEL);
	if (!pb) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	if (data->levels) {
		unsigned int i;

		for (i = 0; i <= data->max_brightness; i++)
			if (data->levels[i] > pb->scale)
				pb->scale = data->levels[i];

		pb->levels = data->levels;
	} else
		pb->scale = data->max_brightness;

	pb->notify = data->notify;
	pb->notify_after = data->notify_after;
	pb->check_fb = data->check_fb;
	pb->exit = data->exit;
	pb->dev = &pdev->dev;
	pb->enabled = false;

	pb->enable_gpio = devm_gpiod_get_optional(&pdev->dev, "enable",
						  GPIOD_OUT_LOW);
	if (IS_ERR(pb->enable_gpio)) {
		dev_err(&pdev->dev, "failed to get enable gpio\n");
		ret = PTR_ERR(pb->enable_gpio);
		goto err_alloc;
	}

	/*
	 * Compatibility fallback for drivers still using the integer GPIO
	 * platform data. Must go away soon.
	 */
	if (!pb->enable_gpio && gpio_is_valid(data->enable_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, data->enable_gpio,
					    GPIOF_OUT_INIT_LOW, "bl_enable");
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request GPIO#%d: %d\n",
				data->enable_gpio, ret);
			goto err_alloc;
		}

		pb->enable_gpio = gpio_to_desc(data->enable_gpio);
	}

	/* power */
	if (data->power_name) {
		pb->power_supply = regulator_get(&pdev->dev, data->power_name);
		if (IS_ERR(pb->power_supply)) {
			dev_err(&pdev->dev, "failed to get power supply\n");
			ret = PTR_ERR(pb->power_supply);
			goto err_alloc;
		}
	}

	/* pwm */
	pb->legacy = true;
	pb->pwm = pwm_request(data->pwm_id, "pwm-backlight");
	if (IS_ERR(pb->pwm)) {
		ret = PTR_ERR(pb->pwm);
		if (ret != -EPROBE_DEFER)
			dev_err(&pdev->dev, "unable to request PWM\n");
		goto err_alloc;
	}

	dev_info(&pdev->dev, "got pwm for backlight\n");

	/*
	 * The DT case will set the pwm_period_ns field to 0 and store the
	 * period, parsed from the DT, in the PWM device. For the non-DT case,
	 * set the period from platform data if it has not already been set
	 * via the PWM lookup table.
	 */
	pb->period = pwm_get_period(pb->pwm);
	if (!pb->period && (data->pwm_period_ns > 0)) {
		pb->period = data->pwm_period_ns;
		pwm_set_period(pb->pwm, data->pwm_period_ns);
	}

	pb->lth_brightness = data->lth_brightness * (pb->period / pb->scale);

	pwm_set_polarity(pb->pwm, data->polarity);

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = data->max_brightness;
	pdev->dev.init_name = "pwm-backlight";
	bl = backlight_device_register(dev_name(&pdev->dev), &pdev->dev, pb,
				       &pwm_backlight_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		ret = PTR_ERR(bl);
		goto err_alloc;
	}

	if (data->dft_brightness > data->max_brightness) {
		dev_warn(&pdev->dev,
			 "invalid default brightness level: %u, using %u\n",
			 data->dft_brightness, data->max_brightness);
		data->dft_brightness = data->max_brightness;
	}

	bl->props.brightness = data->dft_brightness;
	backlight_update_status(bl);

	platform_set_drvdata(pdev, bl);

	dev_info(&pdev->dev, "pwm-backlight probe end\n");
	
	return 0;

err_alloc:
	if (data->exit)
		data->exit(&pdev->dev);
	return ret;
}

static int pwm_backlight_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	backlight_device_unregister(bl);
	pwm_backlight_power_off(pb);

	if (pb->exit)
		pb->exit(&pdev->dev);
	if (pb->legacy)
		pwm_free(pb->pwm);

	return 0;
}

static void pwm_backlight_shutdown(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	pwm_backlight_power_off(pb);
}

#ifdef CONFIG_PM_SLEEP
static int pwm_backlight_suspend(struct device *dev)
{
	struct backlight_device *bl = dev_get_drvdata(dev);
	struct pwm_bl_data *pb = bl_get_data(bl);

	if (pb->notify)
		pb->notify(pb->dev, 0);

	pwm_backlight_power_off(pb);

	if (pb->notify_after)
		pb->notify_after(pb->dev, 0);

	return 0;
}

static int pwm_backlight_resume(struct device *dev)
{
	struct backlight_device *bl = dev_get_drvdata(dev);

	backlight_update_status(bl);

	return 0;
}
#endif

static const struct dev_pm_ops pwm_backlight_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend = pwm_backlight_suspend,
	.resume = pwm_backlight_resume,
	.poweroff = pwm_backlight_suspend,
	.restore = pwm_backlight_resume,
#endif
};

static struct platform_driver pwm_backlight_driver = {
	.driver		= {
		.name		= "pwm-backlight",
		.pm		= &pwm_backlight_pm_ops,
		.of_match_table	= of_match_ptr(pwm_backlight_of_match),
	},
	.probe		= pwm_backlight_probe,
	.remove		= pwm_backlight_remove,
	.shutdown	= pwm_backlight_shutdown,
};

module_platform_driver(pwm_backlight_driver);

MODULE_DESCRIPTION("PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pwm-backlight");
