/*
 *
 * MAX44009 Ambient Light Sensor
 *
 * (C) Copyright 2010-2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Data sheet: https://datasheets.maximintegrated.com/en/ds/MAX44009.pdf
 *
 * 7-bit I2C slave address 0x4a
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/util_macros.h>
#include <linux/gpio/consumer.h>
#include <linux/iio/iio.h>
#include <linux/iio/events.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>
#include <linux/acpi.h>

#define MAX44009_DRV_NAME		"max44009"
#define MAX44009_REGMAP_NAME		"max44009_regmap"
#define MAX44009_EVENT			"max44009_event"
#define MAX44009_GPIO			"max44009_gpio"

/* Registers in datasheet order */
#define MAX44009_REG_INT_STATUS		0x00
#define MAX44009_REG_INT_ENABLE		0x01
#define MAX44009_REG_CFG		0x02
#define MAX44009_REG_LUX_HIGH		0x03
#define MAX44009_REG_LUX_LOW		0x04
#define MAX44009_REG_UP_THDH		0x05
#define MAX44009_REG_LO_THDH		0x06
#define MAX44009_REG_THDH_TIMER		0x07


/* REG_CFG bits */
#define MAX44009_CFG_CONT		0x80
#define MAX44009_CFG_MANUAL		0x40
#define MAX44009_CFG_CDR		0x08
#define MAX44009_CFG_TIM		0x07

#define MAX44009_INTS			0x01
#define MAX44009_INTE			0x01

#define MAX44009_CFG_TIM_SHIFT		0

struct max44009_data {
	struct mutex lock;
	struct regmap *regmap;
};

/* Available integration times with pretty manual alignment: */
static const int max44009_int_time_avail_ns_array[] = {
	   800000,
	   400000,
	   200000,
	   100000,
	    50000,
	    25000,
	    12500,
	     6250,
};
static const char max44009_int_time_avail_str[] =
	"0.800 "
	"0.400 "
	"0.200 "
	"0.100 "
	"0.050 "
	"0.025 "
	"0.0125 "
	"0.00625 ";

static const struct iio_event_spec max44009_events[] = {
	{
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_FALLING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE) |
				 BIT(IIO_EV_INFO_ENABLE),
	},
	{
		.type = IIO_EV_TYPE_THRESH,
		.dir = IIO_EV_DIR_RISING,
		.mask_separate = BIT(IIO_EV_INFO_VALUE) |
				 BIT(IIO_EV_INFO_ENABLE),
	},
};

static const struct iio_chan_spec max44009_channels[] = {
	{
		.type = IIO_LIGHT,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_INT_TIME),
		.event_spec = max44009_events,
		.num_event_specs = ARRAY_SIZE(max44009_events),
	},
};

static irqreturn_t max44009_irq_event_handler(int irq, void *private)
{
	int ret;
	u64 event;

	struct iio_dev *indio_dev = private;
	struct max44009_data *data = iio_priv(indio_dev);


	mutex_lock(&data->lock);
	event = IIO_UNMOD_EVENT_CODE(IIO_LIGHT, 0,
				     IIO_EV_TYPE_THRESH,
				     IIO_EV_DIR_FALLING);

	iio_push_event(indio_dev, event, 0xff);

	/* Reset the interrupt flag */
	ret = regmap_write_bits(data->regmap, MAX44009_REG_CFG,
				MAX44009_INTE, 1);
	if (ret < 0)
		pr_err("failed to reset interrupts\n");
	mutex_unlock(&data->lock);

	return IRQ_HANDLED;
}

static int max44009_gpio_probe(struct i2c_client *client)
{
	struct device *dev;
	struct gpio_desc *gpio;
	int ret;

	if (!client)
		return -EINVAL;

	dev = &client->dev;

	/* gpio interrupt pin */
	gpio = devm_gpiod_get_index(dev, MAX44009_GPIO, 0, GPIOD_IN);
	if (IS_ERR(gpio)) {
		dev_err(dev, "acpi gpio get index failed\n");
		return PTR_ERR(gpio);
	}

	ret = gpiod_to_irq(gpio);
	dev_info(dev, "GPIO resource, no:%d irq:%d\n", desc_to_gpio(gpio), ret);

	return ret;
}

static int max44009_read_luxtim(struct max44009_data *data)
{
	unsigned int val;
	int ret;

	ret = regmap_read(data->regmap, MAX44009_REG_CFG, &val);
	if (ret < 0)
		return ret;

	pr_debug("[%s] line :%d, val:0x%x\n", __func__, __LINE__, val);
	return (val & MAX44009_CFG_TIM) >> MAX44009_CFG_TIM_SHIFT;
}

static int max44009_write_luxtim(struct max44009_data *data, int val)
{
	pr_debug("[%s] line :%d, val:%d\n", __func__, __LINE__, val);

	return regmap_write_bits(data->regmap, MAX44009_REG_CFG,
				MAX44009_CFG_TIM,
				val<<MAX44009_CFG_TIM_SHIFT);
}

static int max44009_read_lux(struct max44009_data *data)
{
	u8 regval[2];
	u8 exponent, mantissa;
	int ret;

	ret = regmap_bulk_read(data->regmap, MAX44009_REG_LUX_HIGH,
			       &regval, sizeof(regval));
	if (ret < 0)
		return ret;

	exponent = (regval[0]&0xF0) >> 4;
	mantissa = (regval[0]&0x0F);
	ret = (1<<exponent) * mantissa * 72/100;

	pr_debug("exponent:%u, mantissa:%u, lux:%d\n", exponent, mantissa, ret);

	return ret;
}


static int max44009_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val, int *val2, long mask)
{
	struct max44009_data *data = iio_priv(indio_dev);
	int luxtim;
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		switch (chan->type) {
		case IIO_LIGHT:
			mutex_lock(&data->lock);
			ret = max44009_read_lux(data);
			mutex_unlock(&data->lock);
			if (ret < 0)
				return ret;
			*val = ret;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_INT_TIME:
		mutex_lock(&data->lock);
		luxtim = ret = max44009_read_luxtim(data);
		mutex_unlock(&data->lock);
		if (ret < 0)
			return ret;
		*val = 0;
		*val2 = max44009_int_time_avail_ns_array[luxtim];
		pr_debug("val:%u, val2:%u, luxtim:%d\n", *val, *val2, luxtim);
		return IIO_VAL_INT_PLUS_MICRO;
	default:
		return -EINVAL;
	}
}

static int max44009_write_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int val, int val2, long mask)
{
	struct max44009_data *data = iio_priv(indio_dev);
	int ret;

	if (mask == IIO_CHAN_INFO_INT_TIME && chan->type == IIO_LIGHT) {
		s64 valus = val * 1000000 + val2;
		int luxtim = find_closest_descending(valus,
				max44009_int_time_avail_ns_array,
				ARRAY_SIZE(max44009_int_time_avail_ns_array));
		pr_debug("val:%u, val2:%u, valus:%lu, luxtim:%d\n", val, val2, valus, luxtim);
		mutex_lock(&data->lock);
		ret = max44009_write_luxtim(data, luxtim);
		mutex_unlock(&data->lock);
		return ret;
	}
	return -EINVAL;
}

static int max44009_write_raw_get_fmt(struct iio_dev *indio_dev,
				      struct iio_chan_spec const *chan,
				      long mask)
{
	if (mask == IIO_CHAN_INFO_INT_TIME && chan->type == IIO_LIGHT)
		return IIO_VAL_INT_PLUS_MICRO;
	else
		return IIO_VAL_INT;
}

static IIO_CONST_ATTR(illuminance_integration_time_available, max44009_int_time_avail_str);

static struct attribute *max44009_attributes[] = {
	&iio_const_attr_illuminance_integration_time_available.dev_attr.attr,
	NULL
};

static const struct attribute_group max44009_attribute_group = {
	.attrs = max44009_attributes,
};

static int max44009_read_event(struct iio_dev *indio_dev,
			      const struct iio_chan_spec *chan,
			      enum iio_event_type type,
			      enum iio_event_direction dir,
			      enum iio_event_info info,
			      int *val, int *val2)
{
	u8 reg;
	u8 buf;
	int ret;
	struct max44009_data *data = iio_priv(indio_dev);
	int exponent, mantissa;

	if (info != IIO_EV_INFO_VALUE)
		return -EINVAL;

	if (dir == IIO_EV_DIR_RISING)
		reg = MAX44009_REG_UP_THDH;
	else if (dir == IIO_EV_DIR_FALLING)
		reg = MAX44009_REG_LO_THDH;
	else
		return -EINVAL;

	mutex_lock(&data->lock);
	ret = regmap_bulk_read(data->regmap, reg, &buf, 1);
	mutex_unlock(&data->lock);
	if (ret < 0) {
		pr_err("register read failed\n");
		return ret;
	}
	exponent = (buf&0xF0) >> 4;
	mantissa = ((buf&0x0F)<<4) + 15;
	*val = (1<<exponent) * mantissa * 45/1000;
	pr_info("exponent:%u, mantissa:%u, lux:%d\n", exponent, mantissa, *val);

	return IIO_VAL_INT;
}

static u8 calculate_reg(int val)
{
	int exponent, mantissa;
	u8 buf;

	exponent = 11;
	mantissa = 0;
	buf = (exponent << 4) | mantissa;

	return buf;
}

static int max44009_write_event(struct iio_dev *indio_dev,
			       const struct iio_chan_spec *chan,
			       enum iio_event_type type,
			       enum iio_event_direction dir,
			       enum iio_event_info info,
			       int val, int val2)
{
	u8 reg;
	u8 buf;
	int ret;
	unsigned int index;
	struct max44009_data *data = iio_priv(indio_dev);

	if (dir == IIO_EV_DIR_RISING)
		reg = MAX44009_REG_UP_THDH;
	else if (dir == IIO_EV_DIR_FALLING)
		reg = MAX44009_REG_LO_THDH;
	else
		return -EINVAL;

	buf = calculate_reg(val);
	pr_info("val:%u, val2:%u, buf:0x%x\n", val, val2, buf);
	ret = regmap_bulk_write(data->regmap, reg, &buf, 1);
	if (ret < 0)
		pr_err("failed to set PS threshold!\n");

	return ret;
}

static int max44009_read_event_config(struct iio_dev *indio_dev,
				     const struct iio_chan_spec *chan,
				     enum iio_event_type type,
				     enum iio_event_direction dir)
{
	unsigned int event_val;
	int ret;
	struct max44009_data *data = iio_priv(indio_dev);

	ret = regmap_read(data->regmap, MAX44009_REG_INT_ENABLE, &event_val);
	if (ret < 0)
		return ret;

	pr_info("[%s] line:%d event_val:0x%x\n", __func__, __LINE__, event_val);
	return (event_val & MAX44009_INTE);
}

static int max44009_write_event_config(struct iio_dev *indio_dev,
				      const struct iio_chan_spec *chan,
				      enum iio_event_type type,
				      enum iio_event_direction dir,
				      int state)
{
	int ret;
	struct max44009_data *data = iio_priv(indio_dev);

	if (state < 0 || state > 1)
		return -EINVAL;
	pr_info("[%s] line:%d state:%d\n", __func__, __LINE__, state);
	/* Set INT ENABLE value */
	mutex_lock(&data->lock);
	ret = regmap_write(data->regmap, MAX44009_REG_INT_ENABLE, state);
	if (ret < 0)
		pr_err("failed to set interrupt mode\n");
	ret = regmap_write(data->regmap, MAX44009_REG_THDH_TIMER, 0x05);
	if (ret < 0)
		pr_err("failed to set threshold time\n");
	mutex_unlock(&data->lock);

	return ret;
}

static const struct iio_info max44009_info = {
	.driver_module		= THIS_MODULE,
	.read_raw		= max44009_read_raw,
	.write_raw		= max44009_write_raw,
	.write_raw_get_fmt	= max44009_write_raw_get_fmt,
	.attrs			= &max44009_attribute_group,
	.read_event_value       = max44009_read_event,
	.write_event_value      = max44009_write_event,
	.read_event_config      = max44009_read_event_config,
	.write_event_config     = max44009_write_event_config,
};

static bool max44009_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX44009_REG_INT_STATUS:
	case MAX44009_REG_INT_ENABLE:
	case MAX44009_REG_CFG:
	case MAX44009_REG_LUX_HIGH:
	case MAX44009_REG_LUX_LOW:
	case MAX44009_REG_UP_THDH:
	case MAX44009_REG_LO_THDH:
	case MAX44009_REG_THDH_TIMER:
		return true;
	default:
		return false;
	}
}

static bool max44009_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX44009_REG_INT_ENABLE:
	case MAX44009_REG_CFG:
	case MAX44009_REG_UP_THDH:
	case MAX44009_REG_LO_THDH:
	case MAX44009_REG_THDH_TIMER:
		return true;
	default:
		return false;
	}
}

static bool max44009_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case MAX44009_REG_INT_STATUS:
	case MAX44009_REG_LUX_HIGH:
	case MAX44009_REG_LUX_LOW:
		return true;
	default:
		return false;
	}
}

static bool max44009_precious_reg(struct device *dev, unsigned int reg)
{
	return reg == MAX44009_REG_INT_STATUS;
}

static const struct regmap_config max44009_regmap_config = {
	.name           = MAX44009_REGMAP_NAME,
	.reg_bits	= 8,
	.val_bits	= 8,

	.max_register	= MAX44009_REG_THDH_TIMER,
	.readable_reg	= max44009_readable_reg,
	.writeable_reg	= max44009_writeable_reg,
	.volatile_reg	= max44009_volatile_reg,
	.precious_reg	= max44009_precious_reg,

	.use_single_rw	= 1,
	.cache_type	= REGCACHE_RBTREE,
};


static int max44009_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct max44009_data *data;
	struct iio_dev *indio_dev;
	int ret, reg;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;
	data = iio_priv(indio_dev);
	data->regmap = devm_regmap_init_i2c(client, &max44009_regmap_config);
	if (IS_ERR(data->regmap)) {
		dev_err(&client->dev, "regmap_init failed!\n");
		return PTR_ERR(data->regmap);
	}

	i2c_set_clientdata(client, indio_dev);
	mutex_init(&data->lock);
	indio_dev->dev.parent = &client->dev;
	indio_dev->info = &max44009_info;
	indio_dev->name = MAX44009_DRV_NAME;
	indio_dev->channels = max44009_channels;
	indio_dev->num_channels = ARRAY_SIZE(max44009_channels);



	/* configuration 800ms */
	ret = regmap_write(data->regmap, MAX44009_REG_CFG, 0x40);
	if (ret < 0) {
		dev_err(&client->dev, "failed to write default CFG_RX: %d\n",
			ret);
		return ret;
	}

	/* Read status at least once to clear any stale interrupt bits. */
	ret = regmap_read(data->regmap, MAX44009_REG_INT_STATUS, &reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed to read init status: %d\n", ret);
		return ret;
	}

	ret = regmap_write(data->regmap, MAX44009_REG_INT_ENABLE, 0x01);
	if (ret < 0) {
		dev_err(&client->dev, "failed to write int enable reg: %d\n",
			ret);
		return ret;
	}
	ret = regmap_write(data->regmap, MAX44009_REG_THDH_TIMER, 0x05);

	if (client->irq <= 0) {
		client->irq = max44009_gpio_probe(client);
		if (client->irq < 0) {
			ret = client->irq;
			return ret;
		}
	}

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(&client->dev, client->irq,
						NULL,
						max44009_irq_event_handler,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						MAX44009_EVENT, indio_dev);
		if (ret < 0) {
			dev_err(&client->dev, "request irq %d failed\n",
				client->irq);
			return ret;
		}
	}

	return iio_device_register(indio_dev);
}

static int max44009_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	iio_device_unregister(indio_dev);

	return 0;
}

static const struct i2c_device_id max44009_id[] = {
	{"max44009", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, max44009_id);

static const struct of_device_id max44009_of_match[] = {
	{ .compatible = "maxim,max44009", },
	{},
};
MODULE_DEVICE_TABLE(of, mxt_of_match);

static struct i2c_driver max44009_driver = {
	.driver = {
		.name	= MAX44009_DRV_NAME,
		.of_match_table = of_match_ptr(max44009_of_match),
	},
	.probe		= max44009_probe,
	.remove		= max44009_remove,
	.id_table	= max44009_id,
};

module_i2c_driver(max44009_driver);

MODULE_AUTHOR("Forever Cai <caiyongheng@allwinnertech.com>");
MODULE_DESCRIPTION("MAX44009 Ambient Light Sensor");
MODULE_LICENSE("GPL v2");
