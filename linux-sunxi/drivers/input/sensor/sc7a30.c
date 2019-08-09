
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include <linux/uaccess.h>
#include<linux/init-input.h>

#define ALLWINNER_PLATFORM
//#define ACTIONS_PLATFORM
#ifdef ALLWINNER_PLATFORM
#include <linux/sys_config.h>
#endif

// device info
#define SENSOR_NAME                   "sc7a30"
#define SENSOR_I2C_ADDR               0x1D
#define ABSMIN                              -512
#define ABSMAX                             512
#define FUZZ                                 2
#define LSG                                  64
#define MAX_DELAY                       200

// constant define
#define SC7A30_CHIP_ID                0x2A
#define SC7A30_DATA_READY        0x07

#define SC7A30_RANGE_2G             0x00
#define SC7A30_RANGE_8G             0x01

#define SC7A30_MODE_STANDBY       0x07
#define SC7A30_MODE_ACTIVE        0x47

#define SC7A30_RATE_400HZ           0x00
#define SC7A30_RATE_100HZ           0x01

// register define
#define SC7A30_XOUT			0x29
#define SC7A30_YOUT			0x2B
#define SC7A30_ZOUT			0x2D
#define SC7A30_MODE_REG			0x20
#define SC7A30_MODE1			0x21
#define SC7A30_MODE2			0x22
#define SC7A30_STATUS			0x27
#define SC7A30_WU_CFG                0X30
#define SC7A30_WU_SRC                0X31
#define SC7A30_WU_THS                0X32
#define SC7A30_WU_DUTATION               0X33

#define POLL_INTERVAL_MAX	500
#define POLL_INTERVAL		50
#define INPUT_FUZZ	2
#define INPUT_FLAT	2
#define SENSORS_INONE	0


#define MODE_CHANGE_DELAY_MS 100

// register bits define
#define SC7A30_RANGE_BIT__POS            7
#define SC7A30_RANGE_BIT__LEN            1
#define SC7A30_RANGE_BIT__MSK            0x20
#define SC7A30_RANGE_BIT__REG            SC7A30_MODE_REG

#define SC7A30_MODE_BIT__POS            0
#define SC7A30_MODE_BIT__LEN            7
#define SC7A30_MODE_BIT__MSK            0x47
#define SC7A30_MODE_BIT__REG            SC7A30_MODE_REG

#define SC7A30_RATE_BIT__POS            5
#define SC7A30_RATE_BIT__LEN            1
#define SC7A30_RATE_BIT__MSK            0x80
#define SC7A30_RATE_BIT__REG            SC7A30_MODE_REG


#define SC7A30_GET_BITSLICE(regvar, bitname)\
    ((regvar & bitname##__MSK) >> bitname##__POS)

#define SC7A30_SET_BITSLICE(regvar, bitname, val)\
    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
};

struct SC7A30_acc{
    s16    x;
    s16    y;
    s16    z;
} ;

struct SC7A30_data {
    struct i2c_client *SC7A30_client;
    struct input_dev *input;
    atomic_t delay;
    atomic_t enable;
    struct mutex enable_mutex;
    struct delayed_work work;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    atomic_t position;
    atomic_t calibrated;
    atomic_t fuzz;
     int offset[3];
};

// cfg data : 1-- used
#define CFG_GSENSOR_USE_CONFIG  1

// calibration file path
#define CFG_GSENSOR_CALIBFILE   "/data/data/com.actions.sensor.calib/files/gsensor_calib.txt"

/*******************************************
* for xml cfg
*******************************************/
#ifdef ACTIONS_PLATFORM
#define CFG_GSENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_GSENSOR_POSITION         "gsensor.position"
#define CFG_GSENSOR_CALIBRATION      "gsensor.calibration"
#define CFG_GSENSOR_MOD_POSITION   "gsensor_"SENSOR_NAME".position"
extern int get_config(const char *key, char *buff, int len);
#endif

#ifdef ALLWINNER_PLATFORM
//static union{
//	unsigned short dirty_addr_buf[2];
//	const unsigned short normal_i2c[2];
//}u_i2c_addr = {{0x00},{SENSOR_I2C_ADDR, I2C_CLIENT_END}};
static __u32 twi_id = 0;
static int sc7a30_pin_hd;
#endif
/*******************************************
* end for xml cfg
*******************************************/

//#ifdef CONFIG_HAS_EARLYSUSPEND
//static void SC7A30_early_suspend(struct early_suspend *h);
//static void SC7A30_early_resume(struct early_suspend *h);
//#endif

#ifdef ALLWINNER_PLATFORM
/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
/*static int gsensor_fetch_sysconfig_para(void)
{
											
	int ret = -1;
	int device_used = -1;
	__u32 twi_addr = 0;
	char name[I2C_NAME_SIZE];
	script_parser_value_type_t type = SCIRPT_PARSER_VALUE_TYPE_STRING;

	printk("========%s===================\n", __func__);

#if(SENSORS_INONE)
	if(SCRIPT_PARSER_OK != (ret = script_parser_fetch("gsensor_para1", "gsensor_used", &device_used, 1))){
		pr_err("%s: script_parser_fetch err.ret = %d. \n", __func__, ret);
		goto script_parser_fetch_err;
	}
#else
	if(SCRIPT_PARSER_OK != (ret = script_parser_fetch("gsensor_para", "gsensor_used", &device_used, 1))){
		pr_err("%s: script_parser_fetch err.ret = %d. \n", __func__, ret);
		goto script_parser_fetch_err;
	}
#endif
	if(1 == device_used){
#if(SENSORS_INONE)
		if(SCRIPT_PARSER_OK != script_parser_fetch_ex("gsensor_para1", "gsensor_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
			pr_err("%s: line: %d script_parser_fetch err. \n", __func__, __LINE__);
			goto script_parser_fetch_err;
		}
#else
		if(SCRIPT_PARSER_OK != script_parser_fetch_ex("gsensor_para", "gsensor_name", (int *)(&name), &type, sizeof(name)/sizeof(int))){
			pr_err("%s: line: %d script_parser_fetch err. \n", __func__, __LINE__);
			goto script_parser_fetch_err;
		}
#endif
		if(strcmp(SENSOR_NAME, name)){
			pr_err("%s: name %s does not match SENSOR_NAME. \n", __func__, name);
			pr_err(SENSOR_NAME);
			//ret = 1;
			return ret;
		}
#if(SENSORS_INONE)		
		if(SCRIPT_PARSER_OK != script_parser_fetch("gsensor_para1", "gsensor_twi_addr", &twi_addr, sizeof(twi_addr)/sizeof(__u32))){
			pr_err("%s: line: %d: script_parser_fetch err. \n", name, __LINE__);
			goto script_parser_fetch_err;
		}
#else
		if(SCRIPT_PARSER_OK != script_parser_fetch("gsensor_para", "gsensor_twi_addr", &twi_addr, sizeof(twi_addr)/sizeof(__u32))){
			pr_err("%s: line: %d: script_parser_fetch err. \n", name, __LINE__);
			goto script_parser_fetch_err;
		}
#endif
		u_i2c_addr.dirty_addr_buf[0] = twi_addr;
		u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
		printk("%s: after: gsensor_twi_addr is 0x%x, dirty_addr_buf: 0x%hx. dirty_addr_buf[1]: 0x%hx \n", \
				__func__, twi_addr, u_i2c_addr.dirty_addr_buf[0], u_i2c_addr.dirty_addr_buf[1]);
#if(SENSORS_INONE)
		if(SCRIPT_PARSER_OK != script_parser_fetch("gsensor_para1", "gsensor_twi_id", &twi_id, 1)){
			pr_err("%s: script_parser_fetch err. \n", name);
			goto script_parser_fetch_err;
		}
#else
		if(SCRIPT_PARSER_OK != script_parser_fetch("gsensor_para", "gsensor_twi_id", &twi_id, 1)){
			pr_err("%s: script_parser_fetch err. \n", name);
			goto script_parser_fetch_err;
		}
#endif
		printk("%s: twi_id is %d. \n", __func__, twi_id);
#if(SENSORS_INONE)
		sc7a30_pin_hd = gpio_request_ex("gsensor_para1",NULL);
#else
		sc7a30_pin_hd = gpio_request_ex("gsensor_para",NULL);
#endif
		if (sc7a30_pin_hd==-1) {
			printk("sc7a30 pin request error!\n");
		}
		ret = 0;

	}else{
		pr_err("%s: gsensor_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_parser_fetch_err:
	pr_notice("=========script_parser_fetch_err============\n");
	return ret;
}*/
/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if(twi_id == adapter->nr){
		pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
				__func__, SENSOR_NAME, i2c_adapter_id(adapter), client->addr);

		strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
		return 0;
	}else{
		return -ENODEV;
	}
}
#endif

static int SC7A30_axis_remap(struct i2c_client *client, struct SC7A30_acc *acc);

static int SC7A30_smbus_read_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_read_byte_data(client, reg_addr);
    if (dummy < 0)
        return -1;
    *data = dummy & 0x000000ff;

    return 0;
}

static int SC7A30_smbus_write_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int SC7A30_smbus_read_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int SC7A30_smbus_write_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;    
    dummy = i2c_smbus_write_i2c_block_data(client, reg_addr, len, data);
        if (dummy < 0)
            return -1;    
    return 0;
}

static int SC7A30_set_mode(struct i2c_client *client, unsigned char mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);    
    data  = SC7A30_SET_BITSLICE(data, SC7A30_MODE_BIT, mode);
    comres += SC7A30_smbus_write_byte(client, SC7A30_MODE_BIT__REG, &data);

    return comres;
}

static int SC7A30_get_mode(struct i2c_client *client, unsigned char *mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);
    *mode  = SC7A30_GET_BITSLICE(data, SC7A30_MODE_BIT);

    return comres;
}

static int SC7A30_set_range(struct i2c_client *client, unsigned char range)
{
    int comres = 0;
    unsigned char data = 0;
    unsigned char mode;

    // save mode
    comres += SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);
    mode = SC7A30_GET_BITSLICE(data, SC7A30_MODE_BIT);    
    if (mode == SC7A30_MODE_ACTIVE) {
        data = SC7A30_SET_BITSLICE(data, SC7A30_MODE_BIT, SC7A30_MODE_STANDBY);
        comres += SC7A30_smbus_write_byte(client, SC7A30_MODE_BIT__REG, &data);
    }
    
    comres += SC7A30_smbus_read_byte(client, SC7A30_RANGE_BIT__REG, &data);    
    data  = SC7A30_SET_BITSLICE(data, SC7A30_RANGE_BIT, range);
    comres += SC7A30_smbus_write_byte(client, SC7A30_RANGE_BIT__REG, &data);

    // restore mode
    if (mode == SC7A30_MODE_ACTIVE) {
        comres += SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);
        data = SC7A30_SET_BITSLICE(data, SC7A30_MODE_BIT, SC7A30_MODE_ACTIVE);
        comres += SC7A30_smbus_write_byte(client, SC7A30_MODE_BIT__REG, &data);
    }
    
    return comres;
}

static int SC7A30_get_range(struct i2c_client *client, unsigned char *range)
{
    int comres = 0;
    unsigned char data = 0;

    comres = SC7A30_smbus_read_byte(client, SC7A30_RANGE_BIT__REG, &data);
    *range  = SC7A30_GET_BITSLICE(data, SC7A30_RANGE_BIT);

    return comres;
}

static int SC7A30_set_rate(struct i2c_client *client, unsigned char rate)
{
    int comres = 0;
    unsigned char data = 0;
    unsigned char mode;

    // save mode
    comres += SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);
    mode = SC7A30_GET_BITSLICE(data, SC7A30_MODE_BIT);    
    if (mode == SC7A30_MODE_ACTIVE) {
        data = SC7A30_SET_BITSLICE(data, SC7A30_MODE_BIT, SC7A30_MODE_STANDBY);
        comres += SC7A30_smbus_write_byte(client, SC7A30_MODE_BIT__REG, &data);
    }
    
    comres += SC7A30_smbus_read_byte(client, SC7A30_RATE_BIT__REG, &data);    
    data = SC7A30_SET_BITSLICE(data, SC7A30_RATE_BIT, rate);
    comres += SC7A30_smbus_write_byte(client, SC7A30_RATE_BIT__REG, &data);

    // restore mode
    if (mode == SC7A30_MODE_ACTIVE) {
        comres += SC7A30_smbus_read_byte(client, SC7A30_MODE_BIT__REG, &data);
        data = SC7A30_SET_BITSLICE(data, SC7A30_MODE_BIT, SC7A30_MODE_ACTIVE);
        comres += SC7A30_smbus_write_byte(client, SC7A30_MODE_BIT__REG, &data);
    }
    
    return comres;
}

static int SC7A30_get_rate(struct i2c_client *client, unsigned char *rate)
{
    int comres = 0;
    unsigned char data = 0;

    comres = SC7A30_smbus_read_byte(client, SC7A30_RATE_BIT__REG, &data);
    *rate  = SC7A30_GET_BITSLICE(data, SC7A30_RATE_BIT);

    return comres;
}


static int SC7A30_hw_init(struct i2c_client *client)
{
    int comres = 0;    
    // sample rate: 100Hz
    comres += SC7A30_set_rate(client, SC7A30_RATE_100HZ);
    
    // range: -2G~+2G
    comres += SC7A30_set_range(client, SC7A30_RANGE_2G);
    
    return comres;
}

static int SC7A30_read_data(struct i2c_client *client, struct SC7A30_acc *acc)
{
   s8 px,py,pz; 
   s32 result;
   result=i2c_smbus_read_byte_data(client, SC7A30_XOUT);
   px=result&0xff;
   result=i2c_smbus_read_byte_data(client, SC7A30_YOUT);
   py=result&0xff;
   result=i2c_smbus_read_byte_data(client, SC7A30_ZOUT);
   pz=result&0xff;	
   acc->x = (((short)px) << 8) >> 8;
   acc->y = (((short)py) << 8) >> 8;
   acc->z = (((short)pz) << 8) >> 8;
   //printk("pga_debug\n");
   return 0;
}

static ssize_t SC7A30_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int address, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    sscanf(buf, "[0x%x]=0x%x", &address, &value);    
    if (SC7A30_smbus_write_byte(SC7A30->SC7A30_client, (unsigned char)address,
                (unsigned char *)&value) < 0)
        return -EINVAL;

    return count;
}

static ssize_t SC7A30_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);    
    size_t count = 0;
    u8 reg[0x32];
    int i;
    
    for (i = 0 ; i < 0x32; i++) {
        SC7A30_smbus_read_byte(SC7A30->SC7A30_client, i, reg+i);    
        count += sprintf(&buf[count], "0x%x: 0x%x\n", i, reg[i]);
    }
    
    return count;
}

static ssize_t SC7A30_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    if (SC7A30_get_mode(SC7A30->SC7A30_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t SC7A30_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (SC7A30_set_mode(SC7A30->SC7A30_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t SC7A30_rate_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    if (SC7A30_get_rate(SC7A30->SC7A30_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t SC7A30_rate_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (SC7A30_set_rate(SC7A30->SC7A30_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t SC7A30_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct SC7A30_data *SC7A30 = input_get_drvdata(input);
    struct SC7A30_acc acc;
    int result;
    int retry = 10;

    while (retry > 0) {
        result = SC7A30_read_data(SC7A30->SC7A30_client, &acc);
        if (result == 0) {
            break;
        }
        retry = retry - 1;
        msleep(20);
    }
    
    if (result == 0) {
        SC7A30_axis_remap(SC7A30->SC7A30_client, &acc);    
        return sprintf(buf, "%d %d %d\n", acc.x, acc.y, acc.z);
    } else {
        return sprintf(buf, "SC7A30_read_data failed!\n");
    }
}

static ssize_t SC7A30_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&SC7A30->delay));

}

static ssize_t SC7A30_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    unsigned char rate;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (data > MAX_DELAY) {
        data = MAX_DELAY;
    }
    
    atomic_set(&SC7A30->delay, (unsigned int) data);

    // change sample rate
    data = 1000 / data;
    if (data > 200) {
        rate = SC7A30_RATE_400HZ;
    } else if (data > 50) {
        rate = SC7A30_RATE_100HZ;
    }else {
        rate = SC7A30_RATE_100HZ;
    }
    SC7A30_set_rate(SC7A30->SC7A30_client, rate);  
    return count;
}


static ssize_t SC7A30_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&SC7A30->enable));

}

static void SC7A30_do_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    if (enable) {
        SC7A30_set_mode(SC7A30->SC7A30_client, SC7A30_MODE_ACTIVE);
        schedule_delayed_work(&SC7A30->work,
            msecs_to_jiffies(atomic_read(&SC7A30->delay)));
    } else {
        SC7A30_set_mode(SC7A30->SC7A30_client, SC7A30_MODE_STANDBY);
        cancel_delayed_work_sync(&SC7A30->work);
    }
}

static void SC7A30_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);
	int pre_enable = atomic_read(&SC7A30->enable);

    mutex_lock(&SC7A30->enable_mutex);
    if (enable != pre_enable) {
        SC7A30_do_enable(dev, enable);
        atomic_set(&SC7A30->enable, enable);
    }
    mutex_unlock(&SC7A30->enable_mutex);
}

static ssize_t SC7A30_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1))
        SC7A30_set_enable(dev, data);

    return count;
}

static ssize_t SC7A30_fuzz_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&SC7A30->fuzz));

}

static ssize_t SC7A30_fuzz_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    
    atomic_set(&(SC7A30->fuzz), (int) data);
    
    if(SC7A30->input != NULL) {
        SC7A30->input->absinfo[ABS_X].fuzz = data;
        SC7A30->input->absinfo[ABS_Y].fuzz = data;
        SC7A30->input->absinfo[ABS_Z].fuzz = data;
    }

    return count;
}

static ssize_t SC7A30_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    data = atomic_read(&(SC7A30->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t SC7A30_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(SC7A30->position), (int) data);

    return count;
}

static ssize_t SC7A30_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);
    struct SC7A30_acc acc;
  	long xyz[3]={0};
    int result,i;
    int retry = 16;
     for(i=0;i<retry;i++)
	 {
      result = SC7A30_read_data(SC7A30->SC7A30_client, &acc);
        xyz[0]+=acc.x;
        xyz[1]+=acc.y;
        xyz[2]+=acc.z;
	 }
	  acc.x=xyz[0]/retry;
	  acc.y=xyz[1]/retry;
	  acc.z=xyz[2]/retry;
	if (acc.z > 0) {
      acc.z -= LSG;
    } 
	else {
        acc.z += LSG;
    }
	/* 
    while (retry > 0) {
        result = SC7A30_read_data(SC7A30->SC7A30_client, &acc);
        if (result == 0) {
            break;
        }
        retry = retry - 1;
        msleep(20);
    }

    if (result == 0) {   
        printk(KERN_INFO "data: %d %d %d\n", acc.x, acc.y, acc.z);
    } else {
        printk(KERN_INFO "run calibration failed!\n");     
        return count;
    }
    
    acc.x = (0 - acc.x) / 2;
    acc.y = (0 - acc.y) / 2;
    if (atomic_read(&SC7A30->position) > 0) {
        acc.z = (LSG - acc.z) / 2;
    } else {
        acc.z = ((-LSG) - acc.z) / 2;
    }
    */
    printk(KERN_INFO "calibration: %d %d %d\n", acc.x, acc.y, acc.z);

    SC7A30->offset[0] = acc.x;
    SC7A30->offset[1] = acc.y;
    SC7A30->offset[2] = acc.z;
    printk(KERN_INFO "run calibration finished\n");
    return count;
}

static ssize_t SC7A30_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);
    
    memset(&(SC7A30->offset), 0, sizeof(SC7A30->offset));    
    printk(KERN_INFO "reset calibration finished\n");
    return count;
}

static ssize_t SC7A30_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    return sprintf(buf, "%d %d %d\n", SC7A30->offset[0], 
                                SC7A30->offset[1], SC7A30->offset[2]);
}

static ssize_t SC7A30_calibration_value_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int data[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);

    sscanf(buf, "%d %d %d", &data[0], &data[1], &data[2]);
    SC7A30->offset[0] = data[0];
    SC7A30->offset[1] = data[1];
    SC7A30->offset[2] = data[2];    
    printk(KERN_INFO "set calibration finished\n");
    return count;
}

static DEVICE_ATTR(reg, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_register_show, SC7A30_register_store);
static DEVICE_ATTR(mode, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_mode_show, SC7A30_mode_store);
static DEVICE_ATTR(rate, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_rate_show, SC7A30_rate_store);
static DEVICE_ATTR(value, 0644,//S_IRUGO,
        SC7A30_value_show, NULL);
static DEVICE_ATTR(delay, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_delay_show, SC7A30_delay_store);
static DEVICE_ATTR(enable, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_enable_show, SC7A30_enable_store);
static DEVICE_ATTR(fuzz, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_fuzz_show, SC7A30_fuzz_store);
static DEVICE_ATTR(board_position, 0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_board_position_show, SC7A30_board_position_store);
static DEVICE_ATTR(calibration_run, 0644,//S_IWUSR|S_IWGRP, //calibration_run
        NULL, SC7A30_calibration_run_store);
static DEVICE_ATTR(calibration_reset, 0644,//S_IWUSR|S_IWGRP,
        NULL, SC7A30_calibration_reset_store);
static DEVICE_ATTR(calibration_value,0644,//S_IRUGO|S_IWUSR|S_IWGRP,
        SC7A30_calibration_value_show,
        SC7A30_calibration_value_store);

static struct attribute *SC7A30_attributes[] = {
    &dev_attr_reg.attr,
    &dev_attr_mode.attr,    
    &dev_attr_rate.attr,
    &dev_attr_value.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_fuzz.attr,
    &dev_attr_board_position.attr,
    &dev_attr_calibration_run.attr,
    &dev_attr_calibration_reset.attr,
    &dev_attr_calibration_value.attr,
    NULL
};

static struct attribute_group SC7A30_attribute_group = {
    .attrs = SC7A30_attributes
};

static int SC7A30_read_file(char *path, char *buf, int size)
{
    struct file *filp;
    loff_t len, offset;
    int ret=0;
    mm_segment_t fs;

    filp = filp_open(path, O_RDWR, 0777);
    if (IS_ERR(filp)) {
        ret = PTR_ERR(filp);
        goto out;
    }

    len = vfs_llseek(filp, 0, SEEK_END);
    if (len > size) {
        len = size;
    }
    
    offset = vfs_llseek(filp, 0, SEEK_SET);

    fs=get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_read(filp, (char __user *)buf, (size_t)len, &(filp->f_pos));

    set_fs(fs);

    filp_close(filp, NULL);    
out:
    return ret;
}

static int SC7A30_load_user_calibration(struct i2c_client *client)
{
    char buffer[16];
    int ret = 0;
    int data[3];
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);    
    int calibrated = atomic_read(&SC7A30->calibrated);
    
    // only calibrate once
    if (calibrated) {
        goto usr_calib_end;
    } else {
        atomic_set(&SC7A30->calibrated, 1);
    }

    ret = SC7A30_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        printk(KERN_ERR "gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d", &data[0], &data[1], &data[2]);
    SC7A30->offset[0] = data[0];
    SC7A30->offset[1] = data[1];
    SC7A30->offset[2] = data[2];    
    printk(KERN_INFO "user cfg_calibration: %d %d %d\n", data[0], data[1], data[2]);
    
usr_calib_end:
    return ret;
}

static int SC7A30_axis_remap(struct i2c_client *client, struct SC7A30_acc *acc)
{
    s16 swap;
    struct SC7A30_data *SC7A30 = i2c_get_clientdata(client);
    int position = atomic_read(&SC7A30->position);

    switch (abs(position)) {
        case 1:
            swap = acc->x;
            acc->x = acc->y;
            acc->y = -swap; 
            break;
        case 2:
            acc->x = -(acc->x);
            acc->y = -(acc->y);
            break;
        case 3:
            swap = acc->x;
            acc->x = -acc->y;
            acc->y = swap;
            break;
        case 4:
            break;
    }
    
    if (position < 0) {
        acc->z = -(acc->z);
        acc->x = -(acc->x);
    }
    
    return 0;
}

static void SC7A30_work_func(struct work_struct *work)
{
    struct SC7A30_data *SC7A30 = container_of((struct delayed_work *)work,
            struct SC7A30_data, work);
    static struct SC7A30_acc acc;    
    int result;
    unsigned long delay = msecs_to_jiffies(atomic_read(&SC7A30->delay));
    
    SC7A30_load_user_calibration(SC7A30->SC7A30_client);
	
    result = SC7A30_read_data(SC7A30->SC7A30_client, &acc);
	acc.x-=SC7A30->offset[0];
	acc.y-=SC7A30->offset[1];
	acc.z-=SC7A30->offset[2];
    if (result == 0) {		
        SC7A30_axis_remap(SC7A30->SC7A30_client, &acc);
        
        input_report_abs(SC7A30->input, ABS_X, acc.x);
        input_report_abs(SC7A30->input, ABS_Y, -acc.y);
        input_report_abs(SC7A30->input, ABS_Z, acc.z);		
        input_sync(SC7A30->input);
    }

    schedule_delayed_work(&SC7A30->work, delay);
}

static int SC7A30_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    struct SC7A30_data *data;
    struct input_dev *dev;
    int cfg_position;
    int cfg_calibration[3];	
	
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    data = kzalloc(sizeof(struct SC7A30_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }
    i2c_set_clientdata(client, data);
    data->SC7A30_client = client;
    mutex_init(&data->enable_mutex);

    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;
    dev->name = SENSOR_NAME;
    dev->id.bustype = BUS_I2C;

    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_X, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_abs_params(dev, ABS_Y, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_abs_params(dev, ABS_Z, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        goto kfree_exit;
    }

    data->input = dev;

    INIT_DELAYED_WORK(&data->work, SC7A30_work_func);
    atomic_set(&data->delay, MAX_DELAY);
#ifdef ALLWINNER_PLATFORM
	SC7A30_set_enable(&client->dev, 1);
#endif
#ifdef ACTIONS_PLATFORM
    atomic_set(&data->enable, 0);
#endif

#ifdef ACTIONS_PLATFORM
#if CFG_GSENSOR_USE_CONFIG > 0
        /*get xml cfg*/
        err = get_config(CFG_GSENSOR_MOD_POSITION, (char *)(&cfg_position), sizeof(int));
        if (err != 0) {
            err = get_config(CFG_GSENSOR_POSITION, (char *)(&cfg_position), sizeof(int));
            if (err != 0) {
                printk(KERN_ERR"get position %d fail\n", cfg_position);
                goto kfree_exit;
            }
        }
#else
        cfg_position = -3;
#endif
#else
            cfg_position = -3;
#endif
    atomic_set(&data->position, cfg_position);
    atomic_set(&data->calibrated, 0);
    atomic_set(&data->fuzz, FUZZ);
        
    //power on init regs    
    err = SC7A30_hw_init(client); 
    if (err < 0) {
        printk(KERN_ERR"SC7A30 probe fail! err:%d\n", err);
        goto kfree_exit;
    }

    err = sysfs_create_group(&data->input->dev.kobj,
            &SC7A30_attribute_group);
    if (err < 0)
        goto error_sysfs;

//#ifdef CONFIG_HAS_EARLYSUSPEND	
//    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
//    data->early_suspend.suspend = SC7A30_early_suspend;
//    data->early_suspend.resume = SC7A30_early_resume;
//    register_early_suspend(&data->early_suspend);
//#endif

#ifdef ACTIONS_PLATFORM
#if CFG_GSENSOR_USE_CONFIG > 0
    /*get xml cfg*/
    err = get_config(CFG_GSENSOR_CALIBRATION, (char *)cfg_calibration, sizeof(cfg_calibration));
    if (err != 0) {
        printk(KERN_ERR"get calibration fail\n");
        goto error_sysfs;
    }
#else
    memset(cfg_calibration, 0, sizeof(cfg_calibration));
#endif  
#else
    memset(cfg_calibration, 0, sizeof(cfg_calibration));
#endif
    
    // calibrate offset
    data->offset[0] = (int)cfg_calibration[0];
    data->offset[1] = (int)cfg_calibration[1];
    data->offset[2] = (int)cfg_calibration[2];     
    return 0;

error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return err;
}

/*#ifdef CONFIG_HAS_EARLYSUSPEND
static void SC7A30_early_suspend(struct early_suspend *h)
{
    // sensor hal will disable when early suspend   
}


static void SC7A30_early_resume(struct early_suspend *h)
{
    // sensor hal will enable when early resume   
}
#endif
*/
static int SC7A30_remove(struct i2c_client *client)
{
    struct SC7A30_data *data = i2c_get_clientdata(client);
	
    SC7A30_set_enable(&client->dev, 0);
//#ifdef CONFIG_HAS_EARLYSUSPEND
//    unregister_early_suspend(&data->early_suspend);
//#endif
    sysfs_remove_group(&data->input->dev.kobj, &SC7A30_attribute_group);
    input_unregister_device(data->input);
    kfree(data);

    return 0;
}

#ifdef CONFIG_PM

static int SC7A30_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *data = i2c_get_clientdata(client);
    
    SC7A30_do_enable(dev, 0);  
	input_set_power_enable(&(gsensor_info.input_type),0);
    return 0;
}

static int SC7A30_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct SC7A30_data *data = i2c_get_clientdata(client);
    
    //power on init regs    
    SC7A30_hw_init(data->SC7A30_client);
    SC7A30_do_enable(dev, atomic_read(&data->enable));
	input_set_power_enable(&(gsensor_info.input_type),1);
    return 0;
}


#else

#define SC7A30_suspend        NULL
#define SC7A30_resume        NULL

#endif*/ /* CONFIG_PM */


static SIMPLE_DEV_PM_OPS(SC7A30_pm_ops, SC7A30_suspend, SC7A30_resume);

static const unsigned short  SC7A30_addresses[] = {
    SENSOR_I2C_ADDR,
    I2C_CLIENT_END,
};

static const struct i2c_device_id SC7A30_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, SC7A30_id);

static const struct of_device_id sc7a30_of_id[] = {
	{ .compatible = "sc7a30",},
	{},
};


static struct i2c_driver SC7A30_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &SC7A30_pm_ops,
        .of_match_table = "allwinner,sun50i-gsensor-para",
    },
    .class        = I2C_CLASS_HWMON,
    .address_list    = SC7A30_addresses,
    .id_table    = SC7A30_id,
    .probe        = SC7A30_probe, 
    .remove        = SC7A30_remove,   
    
//  #ifdef ALLWINNER_PLATFORM
//  .address_list	= u_i2c_addr.normal_i2c,
//  #endif
};

static struct i2c_board_info SC7A30_board_info={
    .type = SENSOR_NAME, 
    .addr = SENSOR_I2C_ADDR,
};

static struct i2c_client *SC7A30_client;

static int __init SC7A30_init(void)
{
    struct i2c_adapter *i2c_adap;
    unsigned int cfg_i2c_adap_id;
#ifdef ACTIONS_PLATFORM
#if CFG_GSENSOR_USE_CONFIG > 0
    int ret;
    
    /*get xml cfg*/
    ret = get_config(CFG_GSENSOR_ADAP_ID, (char *)(&cfg_i2c_adap_id), sizeof(unsigned int));
    if (ret != 0) {
        printk(KERN_ERR"get i2c_adap_id %d fail\n", cfg_i2c_adap_id);
        return ret;
    }
#else
    cfg_i2c_adap_id = 1;
#endif
#else
    cfg_i2c_adap_id = 1;
	
#endif
#ifdef ACTIONS_PLATFORM
    i2c_adap = i2c_get_adapter(cfg_i2c_adap_id);  
    SC7A30_client = i2c_new_device(i2c_adap, &SC7A30_board_info);  
    i2c_put_adapter(i2c_adap);
#endif
#ifdef ALLWINNER_PLATFORM
   int ret ;
  if (input_fetch_sysconfig_para(&(gsensor_info.input_type))){
	printk("%s: err.\n", __func__);
	return -1;
	}else{
		ret = input_init_platform_resource(&(gsensor_info.input_type));
		if (0 != ret){
			printk("%s:ctp_ops.init_platform_resource err. \n", __func__);
		}
	}
  	input_set_power_enable(&(gsensor_info.input_type),1);
	twi_id = gsensor_info.twi_id;
	printk("%s i2c twi is %d \n", __func__, twi_id);
	SC7A30_driver.detect = gsensor_detect;
	
#endif
    
    return i2c_add_driver(&SC7A30_driver);
}

static void __exit SC7A30_exit(void)
{
  #ifdef ACTIONS_PLATFORM
    i2c_unregister_device(SC7A30_client);
  #endif
    i2c_del_driver(&SC7A30_driver);
    input_free_platform_resource(&(gsensor_info.input_type));
}

MODULE_AUTHOR("zhiyi quan <quanzhiyi@silan.com.cn>");
MODULE_DESCRIPTION("SC7A30 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(SC7A30_init);
module_exit(SC7A30_exit);

