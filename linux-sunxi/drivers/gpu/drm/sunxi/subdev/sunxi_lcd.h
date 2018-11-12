 /*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*LCD panel ops*/
#ifndef _SUNXI_LCD_H_
#define _SUNXI_LCD_H_

#include "sunxi_drm_panel.h"
#include "sunxi_drm_core.h"

struct sunxi_lcd_private {
	int   lcd_id;
	struct sunxi_panel *sunxi_panel;
	struct disp_panel_para  *panel;
	struct panel_extend_para  *extend_panel;
	struct lcd_clk_info  *clk_info;
	struct pwm_info_t *pwm_info;
	struct disp_lcd_cfg  *lcd_cfg;
	bool      bl_enalbe;
	bool      ex_power_on;
	struct class *bright_class;
	struct device *bright_dev;
	dev_t  bright_devno;
};

int sunxi_get_lcd_sys_info(struct sunxi_lcd_private *sunxi_lcd);

int sunxi_pwm_dev_init(struct sunxi_lcd_private *sunxi_lcd);

struct sunxi_panel *sunxi_lcd_init(struct sunxi_hardware_res *hw_res,
						int panel_id, int lcd_id);
void sunxi_lcd_destroy(struct sunxi_panel *sunxi_panel,
	struct sunxi_hardware_res *hw_res);

int sunxi_common_pwm_set_bl(struct sunxi_lcd_private *sunxi_lcd,
	unsigned int bright);

#endif
