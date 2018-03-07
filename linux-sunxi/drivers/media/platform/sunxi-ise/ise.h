#ifndef ISE_H
#define ISE_H

/* This head file include all files need for basical
** ise hardware operation, such as read and write.
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/mm.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <linux/clk/sunxi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include "ise_api.h"
#include <linux/regulator/consumer.h>

#define DEVICE_NAME "sunxi_ise"

/* system address */
#define PLL_ISE_CTRL_REG                (0x00D0)
#define ISE_CLK_REG                     (0x06A0)
#define MBUS_CLK_GATING_REG             (0x0804)
#define ISE_BGR_REG                     (0x06AC)

/* ise register */
#define ISE_CTRL_REG                    (0x00)
#define ISE_IN_SIZE                     (0x28)
#define ISE_OUT_SIZE                    (0x38)
#define ISE_ICFG_REG                    (0x04)
#define ISE_OCFG_REG                    (0x08)
#define ISE_INTERRUPT_EN                (0x0c)
#define ISE_TIME_OUT_NUM                (0x3c)

#define ISE_INTERRUPT_STATUS            (0x10)
#define ISE_ERROR_FLAG                  (0x14)
#define ISE_RESET_REG                   (0x88)

/* scale register */
#define ISE_SC_OUT_SIZE_0               (0x4c)
#define ISE_SC_OUT_SIZE_1               (0x50)
#define ISE_SC_OUT_SIZE_2               (0x54)

#define ISE_SC_OCFG_REG_0               (0x7c)
#define ISE_SC_OCFG_REG_1               (0x80)
#define ISE_SC_OCFG_REG_2               (0x84)

#define ise_err(x, arg...) printk(KERN_ERR"[ISE_ERR]"x, ##arg)
#define ise_warn(x, arg...) printk(KERN_WARNING"[ISE_WARN]"x, ##arg)
#define ise_print(x, arg...) printk(KERN_INFO"[ISE]"x, ##arg)
#define ise_debug(x, arg...) printk(KERN_DEBUG"[ISE_DEBUG]"x, ##arg)

#endif