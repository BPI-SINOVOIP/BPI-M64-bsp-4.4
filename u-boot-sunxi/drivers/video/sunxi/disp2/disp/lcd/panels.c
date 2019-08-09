/**
 * drivers/video/sunxi/disp2/disp/lcd/panels.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Include file for SUNXI HCI Host Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "panels.h"

struct sunxi_lcd_drv g_lcd_drv;

extern __lcd_panel_t default_panel;
extern __lcd_panel_t lt070me05000_panel;
extern __lcd_panel_t wtq05027d01_panel;
extern __lcd_panel_t t27p06_panel;
extern __lcd_panel_t dx0960be40a1_panel;
extern __lcd_panel_t tft720x1280_panel;
extern __lcd_panel_t S6D7AA0X01_panel;
extern __lcd_panel_t inet_dsi_panel;
extern __lcd_panel_t tm_dsi_panel;
extern __lcd_panel_t ili9881c_dsi_panel;
extern __lcd_panel_t default_eink;

__lcd_panel_t* panel_array[] = {
	&default_panel,
	&he0801a068_panel,
#if defined(CONFIG_ARCH_SUN50IW3P1)
	&lq101r1sx03_panel,
	&ls029b3sx02_panel,
	&vr_ls055t1sx01_panel,
	&sl008pn21d_panel,
	&ili9881c_dsi_panel,
#else
#if defined(CONFIG_ARCH_SUN8IW12P1) || defined(CONFIG_ARCH_SUN8IW16P1)
	&st7701s_panel,
	&ili9341_panel,
	&fd055hd003s_panel,
	&to20t20000_panel,
	&t30p106_panel,
	&st7796s_panel,
	&st7789v_panel,
	&lh219wq1_panel,
	&frd450h40014_panel,
	&h245qbn02_panel,
	&s2003t46g_panel,
#else
	&lt070me05000_panel,
	&wtq05027d01_panel,
	&t27p06_panel,
	&dx0960be40a1_panel,
	&tft720x1280_panel,
	&S6D7AA0X01_panel,
	&inet_dsi_panel,
	&tm_dsi_panel,
	&gg1p4062utsw_panel,
	&vr_sharp_panel,
	&WilliamLcd_panel,
	&S070WV20_MIPI_RGB_panel,
#endif
#endif /*endif CONFIG_ARCH_SUN50IW3P1 */

#if defined(CONFIG_EINK_PANEL_USED)
	&default_eink,
#endif
	/* add new panel below */

	NULL,
};

static void lcd_set_panel_funs(void)
{
	int i;

	for (i=0; panel_array[i] != NULL; i++) {
		sunxi_lcd_set_panel_funs(panel_array[i]->name, &panel_array[i]->func);
	}

	return ;
}

int lcd_init(void)
{
	sunxi_disp_get_source_ops(&g_lcd_drv.src_ops);
	lcd_set_panel_funs();

	return 0;
}
