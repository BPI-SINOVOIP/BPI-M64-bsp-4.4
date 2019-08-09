/*
 * drivers/input/touchscreen/focaltech.c
 *
 * FocalTech focaltech TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * VERSION      	DATE			AUTHOR
 *    1.0		       2014-09			mshl
 *
 */

 /*******************************************************************************
*
* File Name: focaltech.c
*
* Author: mshl
*
* Created: 2014-09
*
* Modify by mshl on 2015-04-30
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/******************************************************************************
* Included header files
*******************************************************************************/

#include "focaltech_core.h"

#if(defined(CONFIG_I2C_SPRD) ||defined(CONFIG_I2C_SPRD_V1))
#include <mach/i2c-sprd.h>
#endif

#if defined(CONFIG_PM)
#include <linux/power/scenelock.h>
#include <linux/pm_wakeup.h>
#endif

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
//#define FTS_DBG
#ifdef FTS_DBG
#define ENTER printk(KERN_INFO "[FTS_DBG] func: %s  line: %04d\n", __func__, __LINE__);
#define PRINT_DBG(x...)  pr_debug("[FTS_DBG] " x)
#define PRINT_INFO(x...)  pr_info("[FTS_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FTS_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FTS_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#else
#define ENTER
#define PRINT_DBG(x...)
#define PRINT_INFO(x...)  printk(KERN_INFO "[FTS_INFO] " x)
#define PRINT_WARN(x...)  printk(KERN_INFO "[FTS_WARN] " x)
#define PRINT_ERR(format,x...)  printk(KERN_ERR "[FTS_ERR] func: %s  line: %04d  info: " format, __func__, __LINE__, ## x)
#endif


static struct device_node *dev_np;

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

struct fts_Upgrade_Info fts_updateinfo_curr;


/*******************************************************************************
* Static variables
*******************************************************************************/
#if USE_WAIT_QUEUE
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static int tpd_flag = 0;
#endif

/*static int bsuspend = 0;*/

 struct fts_ts_data *fts_wq_data;
 struct i2c_client *fts_i2c_client;
 struct input_dev *fts_input_dev;

/*static unsigned char suspend_flag = 0;*/

/*static unsigned char fts_read_fw_ver(void);*/

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fts_ts_suspend(struct early_suspend *handler);
static void fts_ts_resume(struct early_suspend *handler);
#endif

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/

#ifdef TOUCH_VIRTUAL_KEYS

/*******************************************************************************
* Name: virtual_keys_show
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/

static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct fts_ts_data *data = i2c_get_clientdata(fts_i2c_client);
	struct fts_platform_data *pdata = data->platform_data;
	return sprintf(buf,"%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d:%s:%s:%d:%d:%d:%d\n"
		,__stringify(EV_KEY), __stringify(KEY_MENU),pdata ->virtualkeys[0],pdata ->virtualkeys[1],pdata ->virtualkeys[2],pdata ->virtualkeys[3]
		,__stringify(EV_KEY), __stringify(KEY_HOMEPAGE),pdata ->virtualkeys[4],pdata ->virtualkeys[5],pdata ->virtualkeys[6],pdata ->virtualkeys[7]
		,__stringify(EV_KEY), __stringify(KEY_BACK),pdata ->virtualkeys[8],pdata ->virtualkeys[9],pdata ->virtualkeys[10],pdata ->virtualkeys[11]);
}

static struct kobj_attribute virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.focaltech_ts",
        .mode = S_IRUGO,
    },
    .show = &virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
    &virtual_keys_attr.attr,
    NULL
};

static struct attribute_group properties_attr_group = {
    .attrs = properties_attrs,
};

/*******************************************************************************
* Name: fts_virtual_keys_init
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/

static void fts_virtual_keys_init(void)
{
    int ret = 0;
    struct kobject *properties_kobj;

    pr_info("[FST] %s\n",__func__);

    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,
                     &properties_attr_group);
    if (!properties_kobj || ret)
        pr_err("failed to create board_properties\n");
}

#endif

/**
 * ctp_print_info - sysconfig print function
 * return value:
 *
 */
static void ctp_print_info(struct ctp_config_info_ft6x *info)
{
	PRINT_DBG("info->ctp_used:%d\n", info->ctp_used);
	PRINT_DBG("info->twi_id:%d\n", info->twi_id);
	PRINT_DBG("info->screen_max_x:%d\n", info->screen_max_x);
	PRINT_DBG("info->screen_max_y:%d\n", info->screen_max_y);
	PRINT_DBG("info->revert_x_flag:%d\n", info->revert_x_flag);
	PRINT_DBG("info->revert_y_flag:%d\n", info->revert_y_flag);
	PRINT_DBG("info->exchange_x_y_flag:%d\n", info->exchange_x_y_flag);
	PRINT_DBG("info->irq_gpio_number:%d\n", info->irq_gpio.gpio);
	PRINT_DBG("info->wakeup_gpio_number:%d\n", info->wakeup_gpio.gpio);
}


/*******************************************************************************
* Name: fts_i2c_read
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;

	mutex_lock(&i2c_rw_access);

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = 0,
				 .len = writelen,
				 .buf = writebuf,
			 },
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c read error.\n", __func__);
	} else {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}

/*******************************************************************************
* Name: fts_i2c_write
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	mutex_lock(&i2c_rw_access);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s: i2c write error.\n", __func__);

	mutex_unlock(&i2c_rw_access);

	return ret;
}

/*******************************************************************************
* Name: fts_write_reg
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_write_reg(struct i2c_client *client, u8 addr, const u8 val)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = val;

	return fts_i2c_write(client, buf, sizeof(buf));
}

/*******************************************************************************
* Name: fts_read_reg
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_read_reg(struct i2c_client *client, u8 addr, u8 *val)
{
	return fts_i2c_read(client, &addr, 1, val, 1);
}

/*******************************************************************************
* Name: fts_read_fw_ver
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
/*static unsigned char fts_read_fw_ver(void)
{
	unsigned char ver;
	fts_read_reg(fts_i2c_client, FTS_REG_FIRMID, &ver);
	return(ver);
}*/


/*******************************************************************************
* Name: fts_clear_report_data
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
/*static void fts_clear_report_data(struct fts_ts_data *fts)
{
	int i;

	for(i = 0; i < TS_MAX_FINGER; i++) {
	#if MULTI_PROTOCOL_TYPE_B
		input_mt_slot(fts->input_dev, i);
		input_mt_report_slot_state(fts->input_dev, MT_TOOL_FINGER, false);
	#endif
	}
	input_report_key(fts->input_dev, BTN_TOUCH, 0);
	#if !MULTI_PROTOCOL_TYPE_B
		input_mt_sync(fts->input_dev);
	#endif
	input_sync(fts->input_dev);
}*/

/*******************************************************************************
* Name: fts_update_data
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int fts_update_data(void)
{
	struct fts_ts_data *data = i2c_get_clientdata(fts_i2c_client);
	struct ts_event *event = &data->event;
	u8 buf[33] = {0};
	int ret = -1;
	int i;
	u16 x , y;
	u8 ft_pressure , ft_size;


	ret = fts_i2c_read(fts_i2c_client, buf, 1, buf, 33);

	if (ret < 0) {
		pr_err("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}

	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07;

	if(event->touch_point ==0){

		input_report_abs(data->input_dev, ABS_X, x_old);
		input_report_abs(data->input_dev, ABS_Y, y_old);
		input_report_abs(data->input_dev, ABS_PRESSURE, 0);
	    input_report_key(data->input_dev, BTN_TOUCH, 0);
	    input_sync(data->input_dev);
		return 0;
	}

	for(i = 0; i < TS_MAX_FINGER; i++) {
		if((buf[6*i+3] & 0xc0) == 0xc0)
			continue;
		x = (s16)(buf[6*i+3] & 0x0F)<<8 | (s16)buf[6*i+4];
		y = (s16)(buf[6*i+5] & 0x0F)<<8 | (s16)buf[6*i+6];

		ft_pressure = buf[6*i+7];
		if(ft_pressure > 127 || ft_pressure == 0)
			ft_pressure = 127;
		ft_size = (buf[6*i+8]>>4) & 0x0F;
		if(ft_size == 0)
		{
			ft_size = 0x09;
		}
		if((buf[6*i+3] & 0x40) == 0x0) {
			if (exchange_x_y_flag == 1)
				swap(x, y);
			if (revert_x_flag == 1)
				x = screen_max_x - x;
			if (revert_y_flag == 1)
				y = screen_max_y - y;
			if (x > screen_max_x || x < 0 ||
				y > screen_max_y || y < 0)
				return -1;

#ifdef CONFIG_FT6X_MULTITOUCH   /* 单、多点触摸选择 */
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, true);
		#else
			input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, buf[6*i+5]>>4);
		#endif
			input_report_abs(data->input_dev, ABS_MT_POSITION_X, x);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y, y);
			input_report_abs(data->input_dev, ABS_MT_PRESSURE, ft_pressure);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, ft_size);
			input_report_key(data->input_dev, BTN_TOUCH, 1);
		#if !MULTI_PROTOCOL_TYPE_B
			input_mt_sync(data->input_dev);
		#endif

#else //单点

		input_report_abs(data->input_dev, ABS_X, x);
		input_report_abs(data->input_dev, ABS_Y, y);

		input_report_abs(data->input_dev, ABS_PRESSURE, 127);
	    input_report_key(data->input_dev, BTN_TOUCH, 1);
	  	input_sync(data->input_dev);

#endif

		}
		else {
		#if MULTI_PROTOCOL_TYPE_B
			input_mt_slot(data->input_dev, buf[6*i+5]>>4);
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, false);
		#endif
		}
	}

	x_old = x;
	y_old = y;

	return 0;
}

#if USE_WAIT_QUEUE

/*******************************************************************************
* Name: touch_event_handler
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int touch_event_handler(void *unused)
{
	struct sched_param param = { .sched_priority = 5 };
	sched_setscheduler(current, SCHED_RR, &param);
	u8 state;
	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter, (0 != tpd_flag));
		tpd_flag = 0;
		set_current_state(TASK_RUNNING);
#if FTS_GESTRUE_EN
		if(bsuspend == 1)
		{
			fts_read_reg(fts_i2c_client, 0xd0, &state);
			if( state ==1)
			{
		                fts_read_Gestruedata();
		                continue;
			}
		}
#endif
		fts_update_data();

	} while (!kthread_should_stop());

	return 0;
}
#endif

#if USE_WORK_QUEUE

/*******************************************************************************
* Name: fts_pen_irq_work
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_pen_irq_work(struct work_struct *work)
{
	fts_update_data();
	enable_irq(fts_i2c_client->irq);
}
#endif

/*******************************************************************************
* Name: fts_interrupt
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static irqreturn_t fts_interrupt(int irq, void *dev_id)
{
#if USE_WORK_QUEUE
	struct fts_ts_data *fts = (struct fts_ts_data *)dev_id;
	disable_irq_nosync(fts_i2c_client->irq);

	if (!work_pending(&fts->pen_event_work)) {
		queue_work(fts->fts_workqueue, &fts->pen_event_work);
	}
	return IRQ_HANDLED;
#endif


#if USE_WAIT_QUEUE
	disable_irq_nosync(fts_i2c_client->irq);
	tpd_flag = 1;
	wake_up_interruptible(&waiter);
	return IRQ_HANDLED;
#endif

#if USE_THREADED_IRQ
	fts_update_data();
	return IRQ_HANDLED;
#endif

}

/*******************************************************************************
* Name: fts_reset
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_reset(void)
{
	struct fts_platform_data *pdata = fts_wq_data->platform_data;

	gpio_direction_output(pdata->reset_gpio_number, 1);
	msleep(1);
	gpio_set_value(pdata->reset_gpio_number, 0);
	msleep(10);
	gpio_set_value(pdata->reset_gpio_number, 1);
	msleep(200);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

/*******************************************************************************
* Name: fts_ts_suspend
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_ts_suspend(struct early_suspend *handler)
{
	int ret = -1;
	int count = 5;
	pr_info("==%s==\n", __FUNCTION__);
#if FTS_GESTRUE_EN
	fts_write_reg(fts_i2c_client, 0xd0, 0x01);
	if (fts_updateinfo_curr.CHIP_ID==0x54)
	{
		fts_write_reg(fts_i2c_client, 0xd1, 0xff);
		fts_write_reg(fts_i2c_client, 0xd2, 0xff);
		fts_write_reg(fts_i2c_client, 0xd5, 0xff);
		fts_write_reg(fts_i2c_client, 0xd6, 0xff);
		fts_write_reg(fts_i2c_client, 0xd7, 0xff);
		fts_write_reg(fts_i2c_client, 0xd8, 0xff);
	}
	bsuspend = 1;
	return 0;
#endif
	ret = fts_write_reg(fts_i2c_client, FTS_REG_PMODE, PMODE_HIBERNATE);
	if(ret){
		PRINT_ERR("==fts_suspend==  fts_write_reg fail\n");
	}
	disable_irq(fts_i2c_client->irq);
	fts_clear_report_data(fts_wq_data);
	bsuspend = 1;
}

/*******************************************************************************
* Name: fts_ts_resume
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_ts_resume(struct early_suspend *handler)
{
	struct fts_ts_data  *fts = (struct fts_ts_data *)i2c_get_clientdata(fts_i2c_client);
	queue_work(fts->fts_resume_workqueue, &fts->resume_work);
	bsuspend = 0;
}

/*******************************************************************************
* Name: fts_resume_work
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_resume_work(struct work_struct *work)
{
	pr_info("==%s==\n", __FUNCTION__);
#if FTS_GESTRUE_EN
	fts_write_reg(fts_i2c_client,0xD0,0x00);
#endif
	fts_reset();
	enable_irq(fts_i2c_client->irq);
	msleep(2);
	fts_clear_report_data(fts_wq_data);
}
#endif

/*******************************************************************************
* Name: fts_hw_init
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void fts_hw_init(struct fts_ts_data *fts)
{
	struct regulator *reg_vdd;
	struct i2c_client *client = fts->client;
	struct fts_platform_data *pdata = fts->platform_data;
	int ret = 0;


	printk("[FST] %s [irq=%d];[rst=%d]\n",__func__,pdata->irq_gpio_number,pdata->reset_gpio_number);
	gpio_request(pdata->irq_gpio_number, "ts_irq_pin");
	gpio_request(pdata->reset_gpio_number, "ts_rst_pin");
	gpio_direction_output(pdata->reset_gpio_number, 1);
	gpio_direction_input(pdata->irq_gpio_number);

	reg_vdd = regulator_get(&client->dev, "vcc-ctp");
	if (!WARN(IS_ERR(reg_vdd), "[FST] fts_hw_init regulator: failed to get %s.\n", pdata->vdd_name)) {
		/*regulator_set_voltage(reg_vdd, 2800000, 2800000);*/
		ret = regulator_enable(reg_vdd);
	}
	msleep(100);
	fts_reset();
}
/**
 * ctp_wakeup - function
 *
 */
int ctp_wakeup(int status,int ms)
{


	if (status == 0) {

		if(ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}
	}
	if (status == 1) {
		if(ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}
	}
	msleep(5);

	return 0;
}


/*******************************************************************************
* Name: fts_parse_dt__
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static struct fts_platform_data *fts_parse_dt__(void)
{
	struct fts_platform_data *pdata;


	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		printk("Could not allocate struct fts_platform_data");
		return NULL;
	}

	pdata->reset_gpio_number = config_info.wakeup_gpio.gpio;
	if(pdata->reset_gpio_number < 0){
		printk("fail to get reset_gpio_number\n");
		goto fail;
	}

	pdata->irq_gpio_number = config_info.irq_gpio.gpio;
	if(pdata->reset_gpio_number < 0){
		printk("fail to get reset_gpio_number\n");
		goto fail;
	}

	SCREEN_MAX_X = config_info.screen_max_x;
	if(SCREEN_MAX_X < 0){
		printk("fail to get TP_MAX_X\n");
		goto fail;
	}

	SCREEN_MAX_Y= config_info.screen_max_y;
	if(SCREEN_MAX_Y < 0){
		printk("fail to get TP_MAX_Y\n");
		goto fail;
	}

	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
static int fts_parse_config(void)
{
	int ret =-1;

	twi_id = config_info.twi_id;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	if((screen_max_x == 0) || ( screen_max_y == 0)){
		printk("%s:read config error!\n",__func__);
		return 0;
	}
   return ret;
}
static int ctp_get_sysconfig(struct device_node *np, struct ctp_config_info_ft6x *info)
{
	int ret = 0;
	int gt1x_int_gpio, gt1x_rst_gpio;
	gt1x_int_gpio =
	    of_get_named_gpio_flags(np, "ctp-int-gpio", 0,
				    (enum of_gpio_flags *)&info->irq_gpio);
	gt1x_rst_gpio =
	    of_get_named_gpio_flags(np, "ctp-rst-gpio", 0,
				    (enum of_gpio_flags *)&info->wakeup_gpio);

	if (!gpio_is_valid(gt1x_int_gpio) || !gpio_is_valid(gt1x_rst_gpio)) {
		pr_err("Invalid GPIO, irq-gpio:%d, rst-gpio:%d",
			  gt1x_int_gpio, gt1x_rst_gpio);
		return -EINVAL;
	}
	if (of_property_read_u32(np, "i2c_id", &info->twi_id)) {
		pr_err("failed to get i2c_id\n");
		return -EINVAL;
	}

	if (of_property_read_u32(np, "ctp_exchange_x_y_flag", &info->exchange_x_y_flag)) {
		 info->exchange_x_y_flag = 0;
		pr_err("failed to get ctp_exchange_x_y_flag\n");
	}
	if (of_property_read_u32(np, "ctp_revert_x_flag", &info->revert_x_flag)) {
		info->revert_x_flag = 0;
		pr_err("failed to get revert_x_flag\n");
	}
	if (of_property_read_u32(np, "ctp_revert_y_flag", &info->revert_y_flag)) {
		info->revert_y_flag = 0;
		pr_err("failed to get revert_y_flag\n");
	}
	if (of_property_read_u32(np, "ctp_screen_max_x", &info->screen_max_x)) {
		info->screen_max_x = 320;
		pr_err("failed to get ctp_screen_max_x\n");
	}
	if (of_property_read_u32(np, "ctp_screen_max_y", &info->screen_max_y)) {
		info->screen_max_y = 240;
		pr_err("failed to get ctp_screen_max_y\n");
	}
	ctp_print_info(info);

	return ret;
}

/*******************************************************************************
* Name: fts_probe
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int fts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fts_ts_data *fts;
	struct input_dev *input_dev;
	struct fts_platform_data *pdata = client->dev.platform_data;
	int err = 0;
	unsigned char uc_reg_value;


	printk("[FST] %s: probe\n",__func__);
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		printk("Could not allocate struct fts_platform_data");
		return 0;
	}
#ifdef CONFIG_OF
	err = ctp_get_sysconfig(dev_np, &config_info);
		if (err)
			pr_err("ctp_get_sysconfig failed\n");
#endif
	ctp_wakeup(0, 10);
       //*******************//
	pdata = fts_parse_dt__();
	fts_parse_config();
	if(pdata){
		client->dev.platform_data = pdata;
	}
	else{
		err = -ENOMEM;
		goto exit_alloc_platform_data_failed;
	}



	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	fts = kzalloc(sizeof(*fts), GFP_KERNEL);
	if (!fts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}


	fts_wq_data = fts;
	fts->platform_data = pdata;

	fts_i2c_client = client;
	fts->client = client;

	fts_hw_init(fts);
	i2c_set_clientdata(client, fts);
	client->irq = gpio_to_irq(pdata->irq_gpio_number);


	#if(defined(CONFIG_I2C_SPRD) ||defined(CONFIG_I2C_SPRD_V1))
	sprd_i2c_ctl_chg_clk(client->adapter->nr, 400000);
	#endif

	err = fts_read_reg(fts_i2c_client, FTS_REG_CIPHER, &uc_reg_value);

	if (err < 0)
	{
		pr_err("[FST] read chip id error %x\n", uc_reg_value);
		err = -ENODEV;
		goto exit_chip_check_failed;
	}

#if USE_WORK_QUEUE
	INIT_WORK(&fts->pen_event_work, fts_pen_irq_work);

	fts->fts_workqueue = create_singlethread_workqueue("focal-work-queue");
	if (!fts->fts_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	INIT_WORK(&fts->resume_work, fts_resume_work);
	fts->fts_resume_workqueue = create_singlethread_workqueue("fts_resume_work");
	if (!fts->fts_resume_workqueue) {
		err = -ESRCH;
		goto exit_create_singlethread;
	}
#endif
	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "[FST] failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	fts_input_dev = input_dev;

#ifdef TOUCH_VIRTUAL_KEYS
	fts_virtual_keys_init();
#endif
	fts->input_dev = input_dev;

#ifdef CONFIG_FT6X_MULTITOUCH   /* 单、多点触摸选择 */
	__set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	__set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	__set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	__set_bit(KEY_MENU,  input_dev->keybit);
	__set_bit(KEY_BACK,  input_dev->keybit);
	__set_bit(KEY_HOMEPAGE,  input_dev->keybit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

#if MULTI_PROTOCOL_TYPE_B
	input_mt_init_slots(input_dev, TS_MAX_FINGER);
#endif
	input_set_abs_params(input_dev,ABS_MT_POSITION_X, 0, screen_max_x, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_POSITION_Y, 0, screen_max_y, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_TOUCH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_PRESSURE, 0, 127, 0, 0);
#if !MULTI_PROTOCOL_TYPE_B
	input_set_abs_params(input_dev,ABS_MT_TRACKING_ID, 0, 255, 0, 0);
#endif
#else //单点触摸

 	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_X, 0, screen_max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, screen_max_y, 0, 0);
	input_set_abs_params(input_dev,ABS_PRESSURE, 0, 127, 0 , 0);
#endif
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);

	input_dev->name = FOCALTECH_TS_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"[FST] fts_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#if USE_THREADED_IRQ
	err = request_threaded_irq(client->irq, NULL, fts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, fts);
#else
	err = request_irq(client->irq, fts_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND, client->name, fts);
#endif
	if (err < 0) {
		dev_err(&client->dev, "[FST] ft5x0x_probe: request irq failed %d\n",err);
		goto exit_irq_request_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	fts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	fts->early_suspend.suspend = fts_ts_suspend;
	fts->early_suspend.resume	= fts_ts_resume;
	register_early_suspend(&fts->early_suspend);
#endif

//fts_get_upgrade_array();

#ifdef FTS_SYSFS_DEBUG
fts_create_sysfs(client);
#endif

#ifdef FTS_CTL_IIC
if (fts_rw_iic_drv_init(client) < 0)
{
	dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",	__func__);
}
#endif

#if FTS_GESTRUE_EN
	fts_Gesture_init(input_dev);
	init_para(720,1280,0,0,0);
#endif

#ifdef FTS_APK_DEBUG
	fts_create_apk_debug_channel(client);
#endif

/*
#ifdef FTS_AUTO_UPGRADE
	printk("********************Enter CTP Auto Upgrade********************\n");
	fts_ctpm_auto_upgrade(client);
#endif
*/

#if USE_WAIT_QUEUE
	thread = kthread_run(touch_event_handler, 0, "focal-wait-queue");
	if (IS_ERR(thread))
	{
		err = PTR_ERR(thread);
		PRINT_ERR("failed to create kernel thread: %d\n", err);
	}
#endif

	return 0;

exit_irq_request_failed:
	input_unregister_device(input_dev);
exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
exit_create_singlethread:
exit_chip_check_failed:
	gpio_free(pdata->irq_gpio_number);
	gpio_free(pdata->reset_gpio_number);
	kfree(fts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	fts = NULL;
	i2c_set_clientdata(client, fts);
exit_alloc_platform_data_failed:
	return err;
}

/*******************************************************************************
* Name: fts_remove
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int fts_remove(struct i2c_client *client)
{
	struct fts_ts_data *fts = i2c_get_clientdata(client);

	pr_info("==fts_remove=\n");

	#ifdef FTS_APK_DEBUG
		fts_release_apk_debug_channel();
	#endif

	#ifdef FTS_SYSFS_DEBUG
		fts_release_sysfs(fts_i2c_client);
	#endif

	#ifdef FTS_CTL_IIC
		fts_rw_iic_drv_exit();
	#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fts->early_suspend);
#endif
	free_irq(client->irq, fts);
	input_unregister_device(fts->input_dev);
	input_free_device(fts->input_dev);
#if USE_WORK_QUEUE
	cancel_work_sync(&fts->pen_event_work);
	destroy_workqueue(fts->fts_workqueue);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	cancel_work_sync(&fts->resume_work);
	destroy_workqueue(fts->fts_resume_workqueue);
#endif
	kfree(fts);
	fts = NULL;
	i2c_set_clientdata(client, fts);

	return 0;
}

static const struct i2c_device_id fts_id[] = {
	{ FOCALTECH_TS_NAME, 0 },{ }
};

/*******************************************************************************
* Name: fts_suspend
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
/*static int fts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	PRINT_INFO("fts_suspend\n");
	return 0;
}*/

/*******************************************************************************
* Name: fts_resume
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
/*static int fts_resume(struct i2c_client *client)
{
	PRINT_INFO("fts_resume\n");
	return 0;
}*/

MODULE_DEVICE_TABLE(i2c, fts_id);

static const struct of_device_id focaltech_of_match[] = {
       { .compatible = "focaltech,focaltech_ts", },
       { }
};
MODULE_DEVICE_TABLE(of, focaltech_of_match);

#if defined(CONFIG_PM)
static int gt1x_pm_suspend(struct device *dev)
{
	return 0;
}
static int gt1x_pm_resume(struct device *dev)
{
	return 0;
}
static const struct dev_pm_ops gt1x_ts_pm_ops = {
	.suspend = gt1x_pm_suspend,
	.resume = gt1x_pm_resume,
};
#endif

static struct i2c_driver fts_driver = {
    .class = I2C_CLASS_HWMON,
	.probe		= fts_probe,
	.remove		= fts_remove,
	.id_table	= fts_id,
	.driver	= {
		.name	= FOCALTECH_TS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = focaltech_of_match,
#if defined(CONFIG_PM)
		.pm = &gt1x_ts_pm_ops,
#endif
	},
	.address_list	= normal_i2c,
};

static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
        int ret = 0;


        if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
                return -ENODEV;

	if(twi_id == adapter->nr){

	        ret = i2c_smbus_read_byte_data(client,0xA3);
		if(ret == -70) {
			msleep(10);
			ret = i2c_smbus_read_byte_data(client,0xA3);
		}


        if(ret < 0){

        	return -ENODEV;
		} else {

            strlcpy(info->type, FOCALTECH_TS_NAME, I2C_NAME_SIZE);
		chip_id = ret;
    		return 0;
		}

	}else{
		return -ENODEV;
	}
}

/*******************************************************************************
* Name: fts_init
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static int __init fts_init(void)
{


	dev_np = of_find_compatible_node(NULL, NULL, "focaltech,focaltech_ts");
	if (!dev_np)
		return -1;
	if (of_property_read_u32(dev_np, "i2c_id", &twi_id)) {
		pr_err("failed to get i2c_id\n");
		return -EINVAL;
	}
	fts_driver.detect = ctp_detect;
	return i2c_add_driver(&fts_driver);
}

/*******************************************************************************
* Name: fts_exit
* Brief:
* Input:
* Output: None
* Return: None
*******************************************************************************/
static void __exit fts_exit(void)
{
	i2c_del_driver(&fts_driver);
}

module_init(fts_init);
module_exit(fts_exit);

MODULE_AUTHOR("<fangjianjun>");
MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPL");

