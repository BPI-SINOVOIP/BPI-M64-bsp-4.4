/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */

#ifndef __SUNXI_CODEC_H__
#define __SUNXI_CODEC_H__

struct sunxi_codec_t {
	volatile u32 dac_dpc;
	volatile u32 dac_fifoc;
	volatile u32 dac_fifos;
	volatile u32 dac_txdata;
	volatile u32 dac_actl;
	volatile u32 dac_tune;
	volatile u32 dac_dg;
	volatile u32 adc_fifoc;
	volatile u32 adc_fifos;
	volatile u32 adc_rxdata;
	volatile u32 adc_actl;
	volatile u32 adc_dg;
	volatile u32 dac_cnt;
	volatile u32 adc_cnt;
	volatile u32 ac_dg;
};


extern  int sunxi_codec_init(void);
extern  int sunxi_codec_config(int samplerate, int samplebit, int channel);
extern  int sunxi_codec_start(void *buffer, uint length, uint loop_mode);
extern  int sunxi_codec_stop(void);
extern  int sunxi_codec_wink(void *buffer, uint length);
extern  int sunxi_codec_exit(void);


#endif


