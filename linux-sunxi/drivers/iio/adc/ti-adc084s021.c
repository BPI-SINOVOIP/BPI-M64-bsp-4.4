/**
 * Copyright (C) 2017 Axis Communications AB
 *
 * Driver for Texas Instruments' ADC084S021 ADC chip.
 * Datasheets can be found here:
 * http://www.ti.com/lit/ds/symlink/adc084s021.pdf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/events.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/regulator/consumer.h>

#define MODULE_NAME     "adc084s021"
#define DRIVER_VERSION  "1.0"
#define ADC_RESOLUTION  8
#define ADC_N_CHANNELS  4

struct adc084s021_configuration {
	const struct iio_chan_spec *channels;
	u8 num_channels;
};

struct adc084s021 {
	struct spi_device *spi;
	struct spi_message message;
	struct spi_transfer spi_trans[2];
	struct regulator *reg;
	struct mutex lock;
	/*
	 * DMA (thus cache coherency maintenance) requires the
	 * transfer buffers to live in their own cache lines.
	 */
	union {
		u8 tx_buf[2];
		u8 rx_buf[2];
	} ____cacheline_aligned;
	u8 cur_adc_values[ADC_N_CHANNELS];
};

/**
 * Event triggered when value changes on a channel
 */
static const struct iio_event_spec adc084s021_event = {
	.type = IIO_EV_TYPE_CHANGE,
	.dir = IIO_EV_DIR_NONE,
};

/**
 * Channel specification
 */
#define ADC084S021_VOLTAGE_CHANNEL(num)                  \
	{                                                      \
		.type = IIO_VOLTAGE,                                 \
		.channel = (num),                                    \
		.address = (num << 3),                               \
		.indexed = 1,                                        \
		.scan_index = num,                                   \
		.scan_type = {                                       \
			.sign = 'u',                                       \
			.realbits = 8,                                     \
			.storagebits = 32,                                 \
			.shift = 24 - ((num << 3)),                        \
		},                                                   \
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),        \
		.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),\
		.event_spec = &adc084s021_event,                     \
		.num_event_specs = 1,                                \
	}

static const struct iio_chan_spec adc084s021_channels[] = {
	ADC084S021_VOLTAGE_CHANNEL(0),
	ADC084S021_VOLTAGE_CHANNEL(1),
	ADC084S021_VOLTAGE_CHANNEL(2),
	ADC084S021_VOLTAGE_CHANNEL(3),
	IIO_CHAN_SOFT_TIMESTAMP(4),
};

static const struct adc084s021_configuration adc084s021_config[] = {
	{ adc084s021_channels, ARRAY_SIZE(adc084s021_channels) },
};

/**
 * Read an ADC channel and return its value.
 *
 * @adc: The ADC SPI data.
 * @channel: The IIO channel data structure.
 */
static int adc084s021_adc_conversion(struct adc084s021 *adc,
			   struct iio_chan_spec const *channel)
{
	u16 value;
	int ret;

	mutex_lock(&adc->lock);
	adc->tx_buf[0] = channel->address;

	/* Do the transfer */
	ret = spi_sync(adc->spi, &adc->message);

	if (ret < 0) {
		mutex_unlock(&adc->lock);
		return ret;
	}

	value = (adc->rx_buf[0] << 4) | (adc->rx_buf[1] >> 4);
	mutex_unlock(&adc->lock);

	dev_dbg(&adc->spi->dev, "value 0x%02X on channel %d\n",
			     value, channel->channel);
	return value;
}

/**
 * Make a readout of requested IIO channel info.
 *
 * @indio_dev: The industrial I/O device.
 * @channel: The IIO channel data structure.
 * @val: First element of value (integer).
 * @val2: Second element of value (fractional).
 * @mask: The info_mask to read.
 */
static int adc084s021_read_raw(struct iio_dev *indio_dev,
			   struct iio_chan_spec const *channel, int *val,
			   int *val2, long mask)
{
	struct adc084s021 *adc = iio_priv(indio_dev);
	int retval;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		retval = adc084s021_adc_conversion(adc, channel);
		if (retval < 0)
			return retval;

		*val = retval;
		return IIO_VAL_INT;
		
	case IIO_CHAN_INFO_SCALE:
		retval = regulator_get_voltage(adc->reg);
		*val = retval / 1000;
		return IIO_VAL_INT;
		
	default:
		return -EINVAL;
	}
}

/**
 * Read enabled ADC channels and push data to the buffer.
 *
 * @irq: The interrupt number (not used).
 * @pollfunc: Pointer to the poll func.
 */
#ifdef BUFFER_USED	
static irqreturn_t adc084s021_trigger_handler(int irq, void *pollfunc)
{
	struct iio_poll_func *pf = pollfunc;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct adc084s021 *adc = iio_priv(indio_dev);
	u8 *data;
	s64 timestamp;
	int value, scan_index;

	data = kzalloc(indio_dev->scan_bytes, GFP_KERNEL);
	if (!data) {
		iio_trigger_notify_done(indio_dev->trig);
		return IRQ_NONE;
	}

	timestamp = iio_get_time_ns();

	for_each_set_bit(scan_index, indio_dev->active_scan_mask,
			 indio_dev->masklength) {
		const struct iio_chan_spec *channel =
			&indio_dev->channels[scan_index];
		value = adc084s021_adc_conversion(adc, channel);
		data[scan_index] = value;

		/*
		 * Compare read data to previous read. If it differs send
		 * event notification for affected channel.
		 */
		if (adc->cur_adc_values[scan_index] != (u8)value) {
			adc->cur_adc_values[scan_index] = (u8)value;
			iio_push_event(indio_dev,
						  IIO_EVENT_CODE(IIO_VOLTAGE, 0,
						    IIO_NO_MOD, IIO_EV_DIR_NONE,
						    IIO_EV_TYPE_CHANGE,
						    channel->channel, 0, 0),
						  timestamp);
			dev_dbg(&indio_dev->dev,
					    "new value on ch%d: 0x%02X (ts %llu)\n",
					    channel->channel, value, timestamp);
		}
	}

	iio_push_to_buffers_with_timestamp(indio_dev, data, timestamp);
	iio_trigger_notify_done(indio_dev->trig);
	kfree(data);
	return IRQ_HANDLED;
}
#endif

static const struct iio_info adc084s021_info = {
	.read_raw = adc084s021_read_raw,
	.driver_module = THIS_MODULE,
};

/**
 * Create and register ADC IIO device for SPI.
 */
static int adc084s021_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct adc084s021 *adc;
	int config = spi_get_device_id(spi)->driver_data;
	int retval;

	/* Allocate an Industrial I/O device */
	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*adc));
	if (!indio_dev) {
		dev_err(&spi->dev, "Failed to allocate IIO device\n");
		return -ENOMEM;
	}

	adc = iio_priv(indio_dev);
	adc->spi = spi;
	spi->bits_per_word = ADC_RESOLUTION;

	/* Update the SPI device with config and connect the iio dev */
	retval = spi_setup(spi);
	if (retval) {
		dev_err(&spi->dev, "Failed to update SPI device\n");
		return retval;
	}
	spi_set_drvdata(spi, indio_dev);

	/* Initiate the Industrial I/O device */
	indio_dev->dev.parent = &spi->dev;
	indio_dev->dev.of_node = spi->dev.of_node;
	indio_dev->name = spi_get_device_id(spi)->name;
	indio_dev->modes = INDIO_BUFFER_TRIGGERED; //INDIO_DIRECT_MODE;
	indio_dev->info = &adc084s021_info;
	indio_dev->channels = adc084s021_config[config].channels;
	indio_dev->num_channels = adc084s021_config[config].num_channels;

	/* Create SPI transfer for channel reads */
	adc->spi_trans[0].tx_buf = &adc->tx_buf[0];
	adc->spi_trans[0].len = 2;
	adc->spi_trans[0].speed_hz = spi->max_speed_hz;
	adc->spi_trans[0].bits_per_word = spi->bits_per_word;
	adc->spi_trans[1].rx_buf = &adc->rx_buf[0];
	adc->spi_trans[1].len = 2;
	adc->spi_trans[1].speed_hz = spi->max_speed_hz;
	adc->spi_trans[1].bits_per_word = spi->bits_per_word;

	/* Setup SPI message for channel reads */
	spi_message_init(&adc->message);
	spi_message_add_tail(&adc->spi_trans[0], &adc->message);
	spi_message_add_tail(&adc->spi_trans[1], &adc->message);

	adc->reg = devm_regulator_get(&spi->dev, "vcc-adc");
	if (IS_ERR(adc->reg))
		return PTR_ERR(adc->reg);

	retval = regulator_enable(adc->reg);
	if (retval < 0)
		return retval;

	mutex_init(&adc->lock);

	/* Setup triggered buffer with pollfunction */
#ifdef BUFFER_USED	
	retval = iio_triggered_buffer_setup(indio_dev, NULL,
					    adc084s021_trigger_handler, NULL);
	if (retval) {
		dev_err(&spi->dev, "Failed to setup triggered buffer\n");
		goto buffer_setup_failed;
	}
#endif	
	retval = iio_device_register(indio_dev);
	if (retval) {
		dev_err(&spi->dev, "Failed to register IIO device\n");
		goto device_register_failed;
	}

	dev_info(&spi->dev, "adc084s021 probed succeed!\n");
	return 0;

device_register_failed:
	iio_triggered_buffer_cleanup(indio_dev);

#ifdef BUFFER_USED	
buffer_setup_failed:
#endif
	regulator_disable(adc->reg);
	return retval;
}

/**
 * Unregister ADC IIO device for SPI.
 */
static int adc084s021_remove(struct spi_device *spi)
{
	struct iio_dev *indio_dev = spi_get_drvdata(spi);
	struct adc084s021 *adc = iio_priv(indio_dev);

	iio_device_unregister(indio_dev);
#ifdef BUFFER_USED	
	iio_triggered_buffer_cleanup(indio_dev);
#endif	
	regulator_disable(adc->reg);
	return 0;
}

static const struct of_device_id adc084s021_of_match[] = {
	{ .compatible = "ti,adc084s021", },
	{},
};

MODULE_DEVICE_TABLE(of, adc084s021_of_match);

static const struct spi_device_id adc084s021_id[] = {
	{ MODULE_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(spi, adc084s021_id);

static struct spi_driver adc084s021_driver = {
	.driver = {
		.name = MODULE_NAME,
		.of_match_table = of_match_ptr(adc084s021_of_match),
	},
	.probe = adc084s021_probe,
	.remove = adc084s021_remove,
	.id_table = adc084s021_id,
};

module_spi_driver(adc084s021_driver);

MODULE_AUTHOR("Marten Lindahl <martenli@xxxxxxxx>");
MODULE_DESCRIPTION("Texas Instruments ADC084S021");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);
