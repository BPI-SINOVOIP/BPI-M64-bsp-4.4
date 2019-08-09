/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 */
#ifndef __SPI_H__
#define __SPI_H__

int spinor_init(void);

int spinor_exit(void);

int spinor_read(unsigned int start, unsigned int nblock, void *buffer);

#endif

