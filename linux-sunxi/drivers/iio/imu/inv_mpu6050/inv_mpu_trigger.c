/*
* Copyright (C) 2012 Invensense, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include "inv_mpu_iio.h"

#if defined(CONFIG_MPU_ALLWINNER)
struct inv_mpu6050_state *st_shadow;
extern int inv_irq;
#endif

static void inv_mpu6050_work(struct work_struct *work)
{
	struct inv_mpu6050_state *st = container_of(work,
		struct inv_mpu6050_state, work.work);

	iio_trigger_generic_data_rdy_poll(st->client->irq, st->trig);
#ifdef POLL_WORK
	schedule_work(&st->work);
#else
	if (st->irq_is_disable) {
		enable_irq(inv_irq);
		st->irq_is_disable = 0;
	}
#endif
}


#ifndef POLL_WORK
irqreturn_t mpu6050_irq_handler(int irq, void *private)
{
	/*struct inv_mpu6050_state *st = container_of((struct iio_trigger *)private,
		struct inv_mpu6050_state, trig);*/

	if (!st_shadow->irq_is_disable) {
		st_shadow->irq_is_disable = 1;
		disable_irq_nosync(inv_irq);
	}
	schedule_delayed_work(&st_shadow->work, st_shadow->poll_time_jiffies);
	return IRQ_HANDLED;
}
#endif


static void inv_scan_query(struct iio_dev *indio_dev)
{
	struct inv_mpu6050_state  *st = iio_priv(indio_dev);

	st->chip_config.gyro_fifo_enable =
		test_bit(INV_MPU6050_SCAN_GYRO_X,
			indio_dev->active_scan_mask) ||
			test_bit(INV_MPU6050_SCAN_GYRO_Y,
			indio_dev->active_scan_mask) ||
			test_bit(INV_MPU6050_SCAN_GYRO_Z,
			indio_dev->active_scan_mask);

	st->chip_config.accl_fifo_enable =
		test_bit(INV_MPU6050_SCAN_ACCL_X,
			indio_dev->active_scan_mask) ||
			test_bit(INV_MPU6050_SCAN_ACCL_Y,
			indio_dev->active_scan_mask) ||
			test_bit(INV_MPU6050_SCAN_ACCL_Z,
			indio_dev->active_scan_mask);
}

/**
 *  inv_mpu6050_set_enable() - enable chip functions.
 *  @indio_dev:	Device driver instance.
 *  @enable: enable/disable
 */
static int inv_mpu6050_set_enable(struct iio_dev *indio_dev, bool enable)
{
	struct inv_mpu6050_state *st = iio_priv(indio_dev);
	int result;

	if (enable) {
		result = inv_mpu6050_set_power_itg(st, true);
		if (result)
			return result;
		inv_scan_query(indio_dev);
		if (st->chip_config.gyro_fifo_enable) {
			result = inv_mpu6050_switch_engine(st, true,
					INV_MPU6050_BIT_PWR_GYRO_STBY);
			if (result)
				return result;
		}
		if (st->chip_config.accl_fifo_enable) {
			result = inv_mpu6050_switch_engine(st, true,
					INV_MPU6050_BIT_PWR_ACCL_STBY);
			if (result)
				return result;
		}
		result = inv_reset_fifo(indio_dev);
		if (result)
			return result;
	} else {
		result = inv_mpu6050_write_reg(st, st->reg->fifo_en, 0);
		if (result)
			return result;

		result = inv_mpu6050_write_reg(st, st->reg->int_enable, 0);
		if (result)
			return result;

		result = inv_mpu6050_write_reg(st, st->reg->user_ctrl, 0);
		if (result)
			return result;

		result = inv_mpu6050_switch_engine(st, false,
					INV_MPU6050_BIT_PWR_GYRO_STBY);
		if (result)
			return result;

		result = inv_mpu6050_switch_engine(st, false,
					INV_MPU6050_BIT_PWR_ACCL_STBY);
		if (result)
			return result;
		result = inv_mpu6050_set_power_itg(st, false);
		if (result)
			return result;
	}
	st->chip_config.enable = enable;

	return 0;
}

/**
 * inv_mpu_data_rdy_trigger_set_state() - set data ready interrupt state
 * @trig: Trigger instance
 * @state: Desired trigger state
 */
static int inv_mpu_data_rdy_trigger_set_state(struct iio_trigger *trig,
						bool state)
{
	struct iio_dev *indio_dev = iio_trigger_get_drvdata(trig);
#if defined(POLL_WORK)
	struct inv_mpu6050_state *st = iio_priv(indio_dev);

	if (state == true) {
		schedule_delayed_work(&st->work, st->poll_time_jiffies);
	} else {
		cancel_delayed_work_sync(&st->work);
	}
#endif
	return inv_mpu6050_set_enable(indio_dev, state);
}

static const struct iio_trigger_ops inv_mpu_trigger_ops = {
	.owner = THIS_MODULE,
	.set_trigger_state = &inv_mpu_data_rdy_trigger_set_state,
};


int inv_mpu6050_probe_trigger(struct iio_dev *indio_dev)
{
	int ret;
	struct inv_mpu6050_state *st = iio_priv(indio_dev);

#if defined(CONFIG_MPU_ALLWINNER)
	if (inv_irq == -1)
		return 0;
#endif
	st->trig = devm_iio_trigger_alloc(&indio_dev->dev,
					  "%s-dev%d",
					  indio_dev->name,
					  indio_dev->id);
	if (!st->trig)
		return -ENOMEM;

#if !defined(POLL_WORK)
	st->irq_is_disable = 0;
	st->poll_time_jiffies = msecs_to_jiffies(0);
	INIT_DELAYED_WORK(&st->work, inv_mpu6050_work);
	st_shadow = st;

	ret = devm_request_irq(&indio_dev->dev, st->client->irq,
			       mpu6050_irq_handler,
			       IRQF_TRIGGER_RISING,
			       "inv_mpu",
			       st->trig);
	if (ret)
		return ret;
#else
	st->poll_time_jiffies = msecs_to_jiffies(10);
	INIT_DELAYED_WORK(&st->work, inv_mpu6050_work);
#endif

	st->trig->dev.parent = &st->client->dev;
	st->trig->ops = &inv_mpu_trigger_ops;
	iio_trigger_set_drvdata(st->trig, indio_dev);

	ret = iio_trigger_register(st->trig);
	if (ret)
		return ret;

	indio_dev->trig = iio_trigger_get(st->trig);

	return 0;
}

void inv_mpu6050_remove_trigger(struct inv_mpu6050_state *st)
{
	iio_trigger_unregister(st->trig);
}
