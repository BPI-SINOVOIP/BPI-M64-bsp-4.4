/*
 * sunxi interrupt defer controller: main structures
 *
 * Copyright (C) 2014-2018 allwinnertech Corporation
 *
 * Written by fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Register of IDC device
 */
#define IRQ_NUMBERS				224
#define IRQ_DELAY_EN_REGS		7 /*7*/
#define IRQ_SELECT_REGS			28
#define CHANNEL_NUMBERS			16
#define INT_DU_CTRL_NUMBERS		4

#define IDC_EN_REG				0x0000
#define IDC_EN					BIT(0)
#define IDC_VER_REG				0x0004
#define DU_STA_REG				0x0008

#define IRQ_DLY_EN_BASE			0x0010
#define IRQ_DLY_EN(irq)		(IRQ_DLY_EN_BASE + (((irq) / 32) << 2))
#define IRQ_DLY_EN_MASK(irq)	(0x1 << (irq % 32))

#define IRQ_SEL_REG_BASE		0x0100
#define IRQ_SEL_REG(irq)	(IRQ_SEL_REG_BASE + (((irq) / 8) << 2))
#define IRQ_SEL_REG_MASK(irq)	(0xf << ((irq % 8) << 2))
#define IRQ_SEL_REG_DU(irq, du)	(du << ((irq % 8) << 2))

#define DU_CTRL_REG_BASE	0x0200
#define DU_CTRL_REG(num)	(DU_CTRL_REG_BASE + (0x10 * num))
#define DU_BYPASS			BIT(5)
#define DU_WFI_CHECK_EN		BIT(4)
#define DU_CLK_SRC_SEL_MASK	(0x3 << 2)
#define DU_CLK_32K			(0x0 << 2)
#define DU_CLK_24M			(0x1 << 2)
#define DU_RELOAD_MODE		BIT(1)
#define DU_EN				BIT(0)

#define DU_INTV_VALUE_BASE				0x0204
#define DU_INTV_VALUE(num)	(DU_INTV_VALUE_BASE + (0x10 * num))

#define DU_CUR_VALUE_BASE				0x0208
#define DU_CUR_VALUE(num)	(DU_CUR_VALUE_BASE + (0x10 * num))

#define INT_DU_CTRL_BASE		0x0300
#define INT_DU_CTRL(num)	(INT_DU_CTRL_BASE + (0x10 * num))
#define INT_SEL_MASK		(0xff << 16)
#define INT_SEL(irq)		(irq << 16)
#define OVF_STA				BIT(4)
#define INT_DU_CLK_SEL_MASK	(0x3 << 2)
#define INT_DU_CLK_SEL_32K	(0x00 << 2)
#define INT_DU_CLK_SEL_24M	(0x01 << 2)
#define INT_DU_RESTART		BIT(1)
#define INT_DU_EN			BIT(0)
#define INT_REG_TO_IRQ(reg)	((reg >> 16) & 0xff)

#define INT_DU_CUR_VALUE_BASE	0x0304
#define INT_DU_CUR_VALUE(num)	(INT_DU_CUR_VALUE_BASE + (0x10 * num))

#define sunxi_wait_when(COND, MS) ({ \
	unsigned long timeout__ = jiffies + msecs_to_jiffies(MS) + 1;	\
	int ret__ = 0;							\
	while ((COND)) {						\
		if (time_after(jiffies, timeout__)) {			\
			ret__ = (!COND) ? 0 : -ETIMEDOUT;		\
			break;						\
		}							\
		udelay(1);					\
	}								\
	ret__;								\
})

struct sunxi_idc_regs {
	unsigned int irq_delay_en[IRQ_DELAY_EN_REGS];
	unsigned int irq_select_reg[IRQ_SELECT_REGS];
	unsigned int delay_ctrl_reg[CHANNEL_NUMBERS];
	unsigned int delay_interval_reg[CHANNEL_NUMBERS];
	unsigned int irq_debug_ctrl_reg[INT_DU_CTRL_NUMBERS];
};

struct index_obj {
	struct kobject kobj;
	unsigned int index;
};

struct sunxi_idc {
	struct device *dev;
	void __iomem *base;
	struct sunxi_idc_regs idc_regs;
	struct kset *du_kset;
	struct kset *dbg_kset;
	struct index_obj *du_index[CHANNEL_NUMBERS];
	struct index_obj *dbg_index[INT_DU_CTRL_NUMBERS];
	spinlock_t lock;
};

#define to_index_obj(x) container_of(x, struct index_obj, kobj)

struct index_attribute {
	struct attribute attr;
	ssize_t (*show)(struct index_obj *foo, struct index_attribute *attr,
						char *buf);
	ssize_t (*store)(struct index_obj *foo, struct index_attribute *attr,
						const char *buf, size_t count);
};
#define to_index_attr(x) container_of(x, struct index_attribute, attr)
