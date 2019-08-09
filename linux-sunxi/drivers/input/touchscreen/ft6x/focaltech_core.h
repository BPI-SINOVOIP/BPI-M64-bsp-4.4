/*
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
 */

#ifndef __LINUX_FTS_TS_H__
#define __LINUX_FTS_TS_H__

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
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
//#include <linux/sensors.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>
#include <linux/ioctl.h>

#include <linux/sunxi-gpio.h>
#include <linux/irq.h>

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FTS_DRIVER_INFO  "Spreadtrum_Ver 1.1 2017-06-20"
#define SCREEN_MAX_X   	(screen_max_x)
#define SCREEN_MAX_Y    (screen_max_y)

#define FTS_REG_FW_VER		0xA6
#define PRESS_MAX       255
#define FTS_REG_POINT_RATE	0x88


#define FOCALTECH_TS_NAME	   	       "ft6x_ts"   //	"focaltech_ts"
#define FOCALTECH_TS_ADDR			    0x38

/*register address*/
#define FT_REG_CHIP_ID				0xA3    //chip ID
#define FT_REG_FW_VER				0xA6   //FW  version
#define FT_REG_VENDOR_ID			0xA8   // TP vendor ID
#define TPD_MAX_POINTS_2                        2
#define TPD_MAX_POINTS_5                        5
#define TPD_MAXPOINTS_10                        10
#define AUTO_CLB_NEED                              1
#define AUTO_CLB_NONEED                          0

//FTS_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03

#define FTS_TS_DBG
#ifdef FTS_TS_DBG
#define DBG(fmt, args...) 				printk("[FTS]" fmt, ## args)
#else
#define DBG(fmt, args...) 				do{}while(0)
#endif

//#ifndef ABS_MT_TOUCH_MAJOR
//#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
//#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
//#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
//#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
//#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
//#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
//#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
//#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
//#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
//#define ABS_MT_PRESSURE		0x3a	/* Pressure on contact area */
//#define ABS_MT_TRACKING_ID      0x39	/* Unique ID of initiated contact */
//#endif /* ABS_MT_TOUCH_MAJOR */


#define	USE_WAIT_QUEUE	0
#define	USE_THREADED_IRQ	0
#define	USE_WORK_QUEUE 	1

//#define	TOUCH_VIRTUAL_KEYS
#define	MULTI_PROTOCOL_TYPE_B	 0
#define	TS_MAX_FINGER		1

/*configure this macro by meuconfig*/
#define	CONFIG_FT6X_MULTITOUCH IS_ENABLED(FT6X_MULTITOUCH_TS)

#define	FTS_PACKET_LENGTH		128

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

#define FTS_DBG_EN 1
#if FTS_DBG_EN
#define FTS_DBG(fmt, args...) 				printk("[FTS]" fmt, ## args)
#else
#define FTS_DBG(fmt, args...) 				do{}while(0)
#endif


/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x38,I2C_CLIENT_END};
static const int chip_id_value[] = {0x55,0x06,0x08,0x02,0xa3};
static int chip_id = 0;
static __u32 twi_id = 0;
u32	 x_old;
u32 	 y_old;
static int screen_max_x = 0;
static int screen_max_y = 0;
static int revert_x_flag = 0;
static int revert_y_flag = 0;
static int exchange_x_y_flag = 0;

struct ctp_config_info_ft6x {
	int ctp_used;
	int twi_id;
	int screen_max_x;
	int screen_max_y;
	int revert_x_flag;
	int revert_y_flag;
	int exchange_x_y_flag;
	struct gpio_config wakeup_gpio;
	struct gpio_config irq_gpio;
};

static struct ctp_config_info_ft6x config_info;

struct fts_Upgrade_Info {
       u8 CHIP_ID;
       u8 TPD_MAX_POINTS;
       u8 AUTO_CLB;
	u16 delay_aa;		/*delay of write FT_UPGRADE_AA */
	u16 delay_55;		/*delay of write FT_UPGRADE_55 */
	u8 upgrade_id_1;	/*upgrade id 1 */
	u8 upgrade_id_2;	/*upgrade id 2 */
	u16 delay_readid;	/*delay of read id */
	u16 delay_earse_flash; /*delay of earse flash*/
};

struct ts_event {
	u16	x1;
	u16	y1;
	u16	x2;
	u16	y2;
	u16	x3;
	u16	y3;
	u16	x4;
	u16	y4;
	u16	x5;
	u16	y5;
	u16	pressure;
   u8  touch_point;
};


struct fts_ts_data {
	struct input_dev	*input_dev;
	struct i2c_client	*client;
	struct ts_event    event;
#if USE_WORK_QUEUE
	struct work_struct	pen_event_work;
	struct workqueue_struct	*fts_workqueue;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct work_struct       resume_work;
	struct workqueue_struct *fts_resume_workqueue;
	struct early_suspend	early_suspend;
#endif
	struct fts_platform_data	*platform_data;
};


struct fts_platform_data{
	int irq_gpio_number;
	int reset_gpio_number;
	const char *vdd_name;
	int virtualkeys[12];
	int TP_MAX_X;
	int TP_MAX_Y;
};

enum fts_regs {
	FTS_REG_THGROUP					= 0x80,
	FTS_REG_THPEAK						= 0x81,
	FTS_REG_THCAL						= 0x82,
	FTS_REG_THWATER					= 0x83,
	FTS_REG_THTEMP					= 0x84,
	FTS_REG_THDIFF						= 0x85,
	FTS_REG_CTRL						= 0x86,
	FTS_REG_TIMEENTERMONITOR			= 0x87,
	FTS_REG_PERIODACTIVE				= 0x88,
	FTS_REG_PERIODMONITOR			= 0x89,
	FTS_REG_HEIGHT_B					= 0x8a,
	FTS_REG_MAX_FRAME					= 0x8b,
	FTS_REG_DIST_MOVE					= 0x8c,
	FTS_REG_DIST_POINT				= 0x8d,
	FTS_REG_FEG_FRAME					= 0x8e,
	FTS_REG_SINGLE_CLICK_OFFSET		= 0x8f,
	FTS_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FTS_REG_SINGLE_CLICK_TIME			= 0x91,
	FTS_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FTS_REG_UP_DOWN_OFFSET			= 0x93,
	FTS_REG_DISTANCE_LEFT_RIGHT		= 0x94,
	FTS_REG_DISTANCE_UP_DOWN		= 0x95,
	FTS_REG_ZOOM_DIS_SQR				= 0x96,
	FTS_REG_RADIAN_VALUE				=0x97,
	FTS_REG_MAX_X_HIGH                       	= 0x98,
	FTS_REG_MAX_X_LOW             			= 0x99,
	FTS_REG_MAX_Y_HIGH            			= 0x9a,
	FTS_REG_MAX_Y_LOW             			= 0x9b,
	FTS_REG_K_X_HIGH            			= 0x9c,
	FTS_REG_K_X_LOW             			= 0x9d,
	FTS_REG_K_Y_HIGH            			= 0x9e,
	FTS_REG_K_Y_LOW             			= 0x9f,
	FTS_REG_AUTO_CLB_MODE			= 0xa0,
	FTS_REG_LIB_VERSION_H 				= 0xa1,
	FTS_REG_LIB_VERSION_L 				= 0xa2,
	FTS_REG_CIPHER						= 0xa3,
	FTS_REG_MODE						= 0xa4,
	FTS_REG_PMODE						= 0xa5,	/* Power Consume Mode		*/
	FTS_REG_FIRMID						= 0xa6,
	FTS_REG_STATE						= 0xa7,
	FTS_REG_FT5201ID					= 0xa8,
	FTS_REG_ERR						= 0xa9,
	FTS_REG_CLB						= 0xaa,
};

/*******************************************************************************
* Static variables
*******************************************************************************/

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
//Function Switchs: define to open,  comment to close
#define FTS_GESTRUE_EN 0
#define GTP_ESD_PROTECT 0
//#define FTS_APK_DEBUG
//#define FTS_SYSFS_DEBUG
//#define FTS_CTL_IIC
//#define FTS_AUTO_UPGRADE
//#define CONFIG_TOUCHPANEL_PROXIMITY_SENSOR

extern struct fts_Upgrade_Info fts_updateinfo_curr;
extern  struct i2c_client *fts_i2c_client;
extern  struct fts_ts_data *fts_wq_data;
extern  struct input_dev *fts_input_dev;

static DEFINE_MUTEX(i2c_rw_access);

//Getstre functions
extern int fts_Gesture_init(struct input_dev *input_dev);
extern int fts_read_Gestruedata(void);
extern int fetch_object_sample(unsigned char *buf,short pointnum);
extern void init_para(int x_pixel,int y_pixel,int time_slot,int cut_x_pixel,int cut_y_pixel);

//upgrade functions
extern void fts_update_fw_vendor_id(struct fts_ts_data *data);
extern void fts_update_fw_ver(struct fts_ts_data *data);
extern void fts_get_upgrade_array(void);
extern int fts_ctpm_auto_upgrade(struct i2c_client *client);
extern int fts_fw_upgrade(struct device *dev, bool force);
extern int fts_ctpm_auto_clb(struct i2c_client *client);
extern int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client, char *firmware_name);
extern int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client);
extern int fts_ctpm_get_i_file_ver(void);

//Apk and functions
extern int fts_create_apk_debug_channel(struct i2c_client * client);
extern void fts_release_apk_debug_channel(void);

//ADB functions
extern int fts_create_sysfs(struct i2c_client *client);
extern void fts_release_sysfs(struct i2c_client *client);

//char device for old apk
extern int fts_rw_iic_drv_init(struct i2c_client *client);
extern void  fts_rw_iic_drv_exit(void);

//Base functions
extern int fts_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
extern int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen);
extern int fts_read_reg(struct i2c_client *client, u8 addr, u8 *val);
extern int fts_write_reg(struct i2c_client *client, u8 addr, const u8 val);


/*******************************************************************************
* Static function prototypes
*******************************************************************************/

#endif
