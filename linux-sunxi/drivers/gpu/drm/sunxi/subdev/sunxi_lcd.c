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


#include <drm/drmP.h>

#include "../sunxi_drm_drv.h"
#include "../sunxi_drm_core.h"
#include "../sunxi_drm_crtc.h"
#include "../sunxi_drm_encoder.h"
#include "../sunxi_drm_connector.h"
#include "../sunxi_drm_plane.h"
#include "../drm_de/drm_al.h"
#include "../subdev/sunxi_common.h"
#include "../sunxi_drm_panel.h"
#include "sunxi_lcd.h"
#include "de/disp_lcd.h"
#include "de/include.h"

#define DISP_PIN_STATE_ACTIVE "active"
#define DISP_PIN_STATE_SLEEP "sleep"

static struct lcd_clk_info clk_tbl[] = {
	{LCD_IF_HV,     6, 1, 1, 0},
	{LCD_IF_CPU,   12, 1, 1, 0},
	{LCD_IF_LVDS,   7, 1, 1, 0},
	{LCD_IF_DSI,    4, 1, 4, 148500000},
};

static struct sunxi_lcd_private *sunxi_lcd_priv;

int sunxi_lcd_bl_gpio_init(struct disp_lcd_cfg *lcd_cfg);
static int sunxi_lcd_pin_enalbe(struct sunxi_lcd_private *sunxi_lcd);
static int sunxi_lcd_pin_soft_enalbe(struct sunxi_lcd_private *sunxi_lcd);
/*static int sunxi_lcd_pin_disable(
struct sunxi_lcd_private *sunxi_lcd);*/

static void sunxi_lcd_set_lcd_priv(struct sunxi_lcd_private *priv)
{
	sunxi_lcd_priv = priv;
}

static struct sunxi_lcd_private *sunxi_lcd_get_lcd_priv(void)
{
	return sunxi_lcd_priv;
}

/*set gpio lcd_bl_en output 1*/
static int sunxi_lcd_extern_backlight_init(struct disp_lcd_cfg *lcd_cfg)
{
	struct disp_gpio_set_t  gpio_info[1];

	/* for use the gpio ctl the lcd panel,
	* don't contain the lcd IO pin.
	*/
	if (lcd_cfg->lcd_bl_en_used) {
		memcpy(gpio_info, &(lcd_cfg->lcd_bl_en),
			sizeof(struct disp_gpio_set_t));
		if (lcd_cfg->lcd_bl_gpio_hdl)
			sunxi_drm_sys_gpio_release(lcd_cfg->lcd_bl_gpio_hdl);
		lcd_cfg->lcd_bl_gpio_hdl =
			sunxi_drm_sys_gpio_request(gpio_info);
	}

	return 0;
}

/*set gpio lcd_bl_en output 0*/
static int sunxi_lcd_extern_backlight_release(struct disp_lcd_cfg *lcd_cfg)
{
	struct disp_gpio_set_t  gpio_info[1];

	if (lcd_cfg->lcd_bl_gpio_hdl) {
		sunxi_drm_sys_gpio_release(lcd_cfg->lcd_bl_gpio_hdl);
		memcpy(gpio_info, &(lcd_cfg->lcd_bl_en),
			sizeof(struct disp_gpio_set_t));
		gpio_info->data = (gpio_info->data == 0) ? 1 : 0;
		lcd_cfg->lcd_bl_gpio_hdl =
			sunxi_drm_sys_gpio_request(gpio_info);
	}

	return 0;
}


/*turn on lcd_bl_en_power(VCC-PD) and gpio*/
static int sunxi_lcd_extern_backlight_enable(
	struct sunxi_lcd_private *sunxi_lcd)
{
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	DRM_DEBUG_DRIVER("%s lcd_bl_en_used:%d  bl_enalbe:%d\n",
		__func__, lcd_cfg->lcd_bl_en_used, sunxi_lcd->bl_enalbe);

	if (lcd_cfg->lcd_bl_en_used && sunxi_lcd->bl_enalbe == 0) {
		if (!((!strcmp(lcd_cfg->lcd_bl_en_power, "")) ||
			(!strcmp(lcd_cfg->lcd_bl_en_power, "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_bl_en_power);
		}
		sunxi_lcd_extern_backlight_init(lcd_cfg);

		sunxi_lcd->bl_enalbe = 1;
	}

	return 0;
}

/*turn off lcd_bl_en_power(VCC-PD) and gpio*/
static int sunxi_lcd_extern_backlight_disable(
		struct sunxi_lcd_private *sunxi_lcd)
{
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	DRM_DEBUG_DRIVER("%s:", __func__);
	if (lcd_cfg->lcd_bl_en_used && sunxi_lcd->bl_enalbe == 1) {
		sunxi_lcd_extern_backlight_release(lcd_cfg);

		if (!((!strcmp(lcd_cfg->lcd_bl_en_power, "")) ||
		(!strcmp(lcd_cfg->lcd_bl_en_power, "none")))) {
			sunxi_drm_sys_power_disable(
				lcd_cfg->lcd_bl_en_power);
		}
		sunxi_lcd->bl_enalbe = 0;
	}

	return 0;
}

/*turn on lcd_power/lcd_power1/lcd_power2*/
/*i.e. vcc-mipi/lcd/pe*/
void sunxi_lcd_extrn_power_on(struct sunxi_lcd_private *sunxi_lcd, int i)
{
	struct disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;

	if (lcd_cfg->lcd_power_used[i] == 1) {
		/* regulator type */
		sunxi_drm_sys_power_enable(lcd_cfg->lcd_power[i]);
	}
}

/*turn off lcd_power/lcd_power1/lcd_power2*/
/*i.e. vcc-mipi/lcd/pe*/
void sunxi_lcd_extrn_power_off(struct sunxi_lcd_private *sunxi_lcd, int i)
{
	struct disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;

	if (lcd_cfg->lcd_power_used[i] == 1) {
		/* regulator type */
		sunxi_drm_sys_power_disable(lcd_cfg->lcd_power[i]);
	}
}

/*lcd_gpio_1: LCD-RST*/
void sunxi_lcd_sys_gpio_set_value(struct sunxi_lcd_private *sunxi_lcd,
	u32 io_index, u32 value_to_gpio)
{
	struct disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;
	char gpio_name[20];

	if (!lcd_cfg->gpio_hdl[io_index])
		return;

	sprintf(gpio_name, "lcd_gpio_%d", io_index);
	sunxi_drm_sys_gpio_set_value(
		lcd_cfg->gpio_hdl[io_index],
		value_to_gpio, gpio_name);
}

/*get tcon_div/lcd_div/dsi_div/dsi_rate info*/
struct lcd_clk_info *sunxi_lcd_clk_info_init(struct disp_panel_para *panel)
{
	/* tcon inner div */
	int tcon_div = 6;
	/* lcd clk div */
	int lcd_div = 1;
	/* dsi clk div */
	int dsi_div = 4;
	int dsi_rate = 0;
	int i;
	int find = 0;
	struct lcd_clk_info *info;

	if (!panel) {
		DRM_ERROR("give a err sunxi_panel.\n");
		return NULL;
	}

	info = kzalloc(sizeof(struct lcd_clk_info), GFP_KERNEL);
	if (!info) {
		DRM_ERROR("failed to allocate disp_panel_para.\n");
		return NULL;
	}

	for (i = 0; i < sizeof(clk_tbl) / sizeof(struct lcd_clk_info); i++) {
		if (clk_tbl[i].lcd_if == panel->lcd_if) {
			tcon_div = clk_tbl[i].tcon_div;
			lcd_div = clk_tbl[i].lcd_div;
			dsi_div = clk_tbl[i].dsi_div;
			dsi_rate = clk_tbl[i].dsi_rate;
			find = 1;
			break;
		}
	}

	if (panel->lcd_if == LCD_IF_DSI) {
		u32 lane = panel->lcd_dsi_lane;
		u32 bitwidth = 0;

		switch (panel->lcd_dsi_format) {
		case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
		case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		dsi_div = bitwidth / lane;
	}

	if (!find)
		DRM_ERROR("can not find the lcd_clk_info, use default.\n");

	info->tcon_div = tcon_div;
	info->lcd_div = lcd_div;
	info->dsi_div = dsi_div;
	info->dsi_rate = dsi_rate;

	return info;
}

static bool default_panel_init(struct sunxi_panel *sunxi_panel)
{
	u32 i = 0, j = 0;
	u32 items = 0, curve_tbl_num = 0;

	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;
	struct disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;
	struct panel_extend_para *info = sunxi_lcd->extend_panel;
	struct disp_panel_para *panel = sunxi_lcd->panel;
	u32 lcd_bright_curve_tbl[101][2];
	/*init gamma*/

	u8 lcd_gamma_tbl[][2] = {
		/* {input value, corrected value} */
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};
	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
			{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
			{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
			{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
			{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
			{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	DRM_DEBUG_DRIVER("%s\n", __func__);
	lcd_cfg->backlight_dimming = 0;
	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] +
				((lcd_gamma_tbl[i+1][1] -
				lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
				(value<<16) + (value<<8) + value;
		}
	}

	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
		(lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];
	/* init cmap */

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

	/*init the bright */
	for (i = 0, curve_tbl_num = 0; i < 101; i++) {
		if (lcd_cfg->backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[curve_tbl_num][0] = 0;
				lcd_bright_curve_tbl[curve_tbl_num][1] = 0;
				curve_tbl_num++;
			}
		} else {
			lcd_bright_curve_tbl[curve_tbl_num][0] = 255 * i / 100;
			lcd_bright_curve_tbl[curve_tbl_num][1] =
				lcd_cfg->backlight_curve_adjust[i];
			curve_tbl_num++;
		}
	}

	for (i = 0; i < curve_tbl_num-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] -
			lcd_bright_curve_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] +
				((lcd_bright_curve_tbl[i+1][1] -
				lcd_bright_curve_tbl[i][1]) * j)/num;
		info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j]
			= value;
		}
	}

	if (curve_tbl_num > 0)
		info->lcd_bright_curve_tbl[255] =
			lcd_bright_curve_tbl[curve_tbl_num-1][1];

	sunxi_lcd->clk_info = sunxi_lcd_clk_info_init(panel);
	if (!sunxi_lcd->clk_info) {
		DRM_ERROR("failed to init clk_info.\n");
		return false;
	}
	sunxi_panel->clk_rate = panel->lcd_dclk_freq * 1000000;
	return true;
}

bool default_panel_open(struct sunxi_panel *sunxi_panel)
{
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	sunxi_drm_delayed_ms(80);
	sunxi_lcd_pin_enalbe(sunxi_panel->private);
	sunxi_drm_delayed_ms(50);
	sunxi_chain_enable(sunxi_panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(100);
	return true;
}

bool default_panel_close(struct sunxi_panel *panel)
{
	sunxi_chain_disable(panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(200);
	sunxi_lcd_extrn_power_off(panel->private, 0);
	sunxi_drm_delayed_ms(500);
	return true;
}

/*set pwm and backlight*/
bool sunxi_lcd_default_set_bright(struct sunxi_panel *sunxi_panel,
	unsigned int bright)
{
	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;

	sunxi_common_pwm_set_bl(sunxi_lcd, bright);

	return true;
}

/*enable pwm and set pwm value*/
/*request lcd_bl_en gpio and set it output 1*/
bool sunxi_lcd_inet_set_bright(struct sunxi_panel *sunxi_panel,
	unsigned int bright)
{
	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;

	sunxi_common_pwm_set_bl(sunxi_lcd, bright);

	return true;
}

bool cmp_panel_with_display_mode(struct drm_display_mode *mode,
	struct disp_panel_para       *panel)
{
	if ((mode->clock == panel->lcd_dclk_freq * 1000) &&
		(mode->hdisplay == panel->lcd_x) &&
		(mode->hsync_start == (panel->lcd_ht - panel->lcd_hbp)) &&
		(mode->hsync_end == (panel->lcd_ht -
		panel->lcd_hbp + panel->lcd_hspw)) &&
		(mode->htotal == panel->lcd_ht) &&
		(mode->vdisplay == panel->lcd_y) &&
		(mode->vsync_start == (panel->lcd_vt - panel->lcd_vbp)) &&
		(mode->vsync_end == (panel->lcd_vt -
		panel->lcd_vbp + panel->lcd_vspw)) &&
		(mode->vtotal == panel->lcd_vt) &&
		(mode->vscan == 0) &&
		(mode->flags == 0) &&
		(mode->width_mm == panel->lcd_width) &&
		(mode->height_mm == panel->lcd_height) &&
		(mode->vrefresh == (mode->clock * 1000 /
		mode->vtotal / mode->htotal)))
		return true;
	return false;
}

enum drm_mode_status
	sunxi_lcd_valid_mode(struct sunxi_panel *panel,
	struct drm_display_mode *mode)
{
	enum drm_mode_status status = MODE_BAD;
	struct sunxi_lcd_private *sunxi_lcd_p =
		(struct sunxi_lcd_private *)panel->private;

	if (cmp_panel_with_display_mode(mode, sunxi_lcd_p->panel))
		status = MODE_OK;

	DRM_DEBUG_DRIVER("%s status:%d\n", __func__, status);
	return status;
}

static inline void convert_panel_to_display_mode(
	struct drm_display_mode *mode,
	struct disp_panel_para  *panel)
{
	mode->clock       = panel->lcd_dclk_freq * 1000;

	mode->hdisplay    = panel->lcd_x;
	mode->hsync_start = panel->lcd_ht - panel->lcd_hbp;
	mode->hsync_end   = panel->lcd_ht - panel->lcd_hbp + panel->lcd_hspw;
	mode->htotal      = panel->lcd_ht;

	mode->vdisplay    = panel->lcd_y;
	mode->vsync_start = panel->lcd_vt - panel->lcd_vbp;
	mode->vsync_end   = panel->lcd_vt - panel->lcd_vbp + panel->lcd_vspw;
	mode->vtotal      = panel->lcd_vt;
	mode->vscan       = 0;
	mode->flags       = 0;
	mode->width_mm    = panel->lcd_width;
	mode->height_mm   = panel->lcd_height;
	mode->vrefresh    = mode->clock * 1000 / mode->vtotal / mode->htotal;
	DRM_DEBUG_KMS("Modeline %d:%d %d %d %d %d %d %d %d %d %d 0x%x 0x%x\n",
		mode->base.id, mode->vrefresh, mode->clock,
		mode->hdisplay, mode->hsync_start,
		mode->hsync_end, mode->htotal,
		mode->vdisplay, mode->vsync_start,
		mode->vsync_end, mode->vtotal, mode->type, mode->flags);
	DRM_DEBUG_KMS("panel: clk[%d] [x %d, ht %d, hbp %d, hspw %d]\n",
		panel->lcd_dclk_freq * 1000,
		panel->lcd_x, panel->lcd_ht,
		panel->lcd_hbp, panel->lcd_hspw);
	DRM_DEBUG_KMS("[y%d, vt%d, bp %d, pw %d] %dx%d\n",
		panel->lcd_y, panel->lcd_vt,
		panel->lcd_vbp, panel->lcd_vspw, panel->lcd_width,
		panel->lcd_height);
}

unsigned int sunxi_lcd_get_modes(struct sunxi_panel *sunxi_panel)
{
	struct disp_panel_para   *panel;
	struct drm_display_mode *mode;
	struct sunxi_lcd_private *sunxi_lcd_p =
		(struct sunxi_lcd_private *)sunxi_panel->private;
	struct drm_connector *connector = sunxi_panel->drm_connector;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	panel = sunxi_lcd_p->panel;;
	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode.\n");
		return 0;
	}

	convert_panel_to_display_mode(mode, panel);
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	return 1;
}

enum drm_connector_status
	sunxi_lcd_common_detect(struct sunxi_panel *panel)
{
	DRM_DEBUG_DRIVER("%s\n", __func__);
	return connector_status_connected;
}

static struct  panel_ops  default_panel = {
	.init = default_panel_init,
	.open = default_panel_open,
	.close = default_panel_close,
	.reset = NULL,
	.bright_light = sunxi_lcd_default_set_bright,
	.detect  = sunxi_lcd_common_detect,
	.change_mode_to_timming = NULL,
	.check_valid_mode = sunxi_lcd_valid_mode,
	.get_modes = sunxi_lcd_get_modes,
};

static bool inet_dsi_panel_init(struct sunxi_panel *sunxi_panel)
{
	u32 i = 0, j = 0;
	u32 items = 0, curve_tbl_num = 0;

	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;
	struct disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;
	struct panel_extend_para *info = sunxi_lcd->extend_panel;
	struct disp_panel_para *panel = sunxi_lcd->panel;
	u32 lcd_bright_curve_tbl[101][2];
	/*init gamma*/

	u8 lcd_gamma_tbl[][2] = {
		/*{input value, corrected value} */
		{0, 0}, {15, 15}, {30, 30}, {45, 45}, {60, 60},
		{75, 75}, {90, 90}, {105, 105}, {120, 120}, {135, 135},
		{150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
		{225, 225}, {240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
			{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
			{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
		},
		{
			{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
			{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
			{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
		},
	};

	DRM_DEBUG_DRIVER("%s\n", __func__);
	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
			u32 num = lcd_gamma_tbl[i + 1][0] -
				lcd_gamma_tbl[i][0];

			for (j = 0; j < num; j++) {
					u32 value = 0;

					value =
						lcd_gamma_tbl[i][1] +
						((lcd_gamma_tbl[i + 1][1] -
						lcd_gamma_tbl[i][1]) *
						 j) / num;
				info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
					(value << 16) + (value << 8) + value;
			}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
		(lcd_gamma_tbl[items - 1][1] << 8) +
		lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));


	/*init the bright */
	for (i = 0, curve_tbl_num = 0; i < 101; i++) {
		if (lcd_cfg->backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[curve_tbl_num][0] = 0;
				lcd_bright_curve_tbl[curve_tbl_num][1] = 0;
				curve_tbl_num++;
			}
		} else {
			lcd_bright_curve_tbl[curve_tbl_num][0] = 255 * i / 100;
			lcd_bright_curve_tbl[curve_tbl_num][1] =
				lcd_cfg->backlight_curve_adjust[i];
			curve_tbl_num++;
		}
	}

	for (i = 0; i < curve_tbl_num-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] -
			lcd_bright_curve_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] +
				((lcd_bright_curve_tbl[i+1][1] -
				lcd_bright_curve_tbl[i][1]) * j)/num;
		info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j]
			= value;
		}
	}

	if (curve_tbl_num > 0)
		info->lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[curve_tbl_num-1][1];

	sunxi_lcd->clk_info = sunxi_lcd_clk_info_init(panel);
	if (!sunxi_lcd->clk_info) {
		DRM_ERROR("failed to init clk_info.\n");
		return false;
	}
	sunxi_panel->clk_rate = panel->lcd_dclk_freq * 1000000;
	return true;
}

#define REGFLAG_DELAY 0XFE
#define REGFLAG_END_OF_TABLE 0xFF /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

/*add panel initialization below*/
static struct LCM_setting_table LCM_LT080B21BA94_setting[] = {
	{0xE0, 1, { 0x00 } },
	{0xE1, 1, { 0x93 } },
	{0xE2, 1, { 0x65 } },
	{0xE3, 1, { 0xF8 } },
	{0xE0, 1, { 0x00 } },
	{0x70, 1, { 0x02 } },
	{0x71, 1, { 0x23 } },
	{0x72, 1, { 0x06 } },
	{0xE0, 1, { 0x01 } },
	{0x00, 1, { 0x00 } },
	{0x01, 1, { 0xA0 } },
	{0x03, 1, { 0x00 } },
	{0x04, 1, { 0xA0 } },
	{0x17, 1, { 0x00 } },
	{0x18, 1, { 0xB1 } },
	{0x19, 1, { 0x00 } },
	{0x1A, 1, { 0x00 } },
	{0x1B, 1, { 0xB1 } },
	{0x1C, 1, { 0x00 } },
	{0x1F, 1, { 0x48 } },
	{0x20, 1, { 0x23 } },
	{0x21, 1, { 0x23 } },
	{0x22, 1, { 0x0E } },
	{0x23, 1, { 0x00 } },
	{0x24, 1, { 0x38 } },
	{0x26, 1, { 0xD3 } },
	{0x37, 1, { 0x59 } },
	{0x38, 1, { 0x05 } },
	{0x39, 1, { 0x08 } },
	{0x3A, 1, { 0x12 } },
	{0x3C, 1, { 0x78 } },
	{0x3E, 1, { 0x80 } },
	{0x3F, 1, { 0x80 } },
	{0x40, 1, { 0x06 } },
	{0x41, 1, { 0xA0 } },
	{0x55, 1, { 0x0F } },
	{0x56, 1, { 0x01 } },
	{0x57, 1, { 0x85 } },
	{0x58, 1, { 0x0A } },
	{0x59, 1, { 0x0A } },
	{0x5A, 1, { 0x32 } },
	{0x5B, 1, { 0x0F } },
	{0x5D, 1, { 0x7C } },
	{0x5E, 1, { 0x62 } },
	{0x5F, 1, { 0x50 } },
	{0x60, 1, { 0x42 } },
	{0x61, 1, { 0x3D } },
	{0x62, 1, { 0x2D } },
	{0x63, 1, { 0x30 } },
	{0x64, 1, { 0x19 } },
	{0x65, 1, { 0x30 } },
	{0x66, 1, { 0x2E } },
	{0x67, 1, { 0x2D } },
	{0x68, 1, { 0x4A } },
	{0x69, 1, { 0x3A } },
	{0x6A, 1, { 0x43 } },
	{0x6B, 1, { 0x37 } },
	{0x6C, 1, { 0x37 } },
	{0x6D, 1, { 0x2D } },
	{0x6E, 1, { 0x1F } },
	{0x6F, 1, { 0x00 } },
	{0x70, 1, { 0x7C } },
	{0x71, 1, { 0x62 } },
	{0x72, 1, { 0x50 } },
	{0x73, 1, { 0x42 } },
	{0x74, 1, { 0x3D } },
	{0x75, 1, { 0x2D } },
	{0x76, 1, { 0x30 } },
	{0x77, 1, { 0x19 } },
	{0x78, 1, { 0x30 } },
	{0x79, 1, { 0x2E } },
	{0x7A, 1, { 0x2D } },
	{0x7B, 1, { 0x4A } },
	{0x7C, 1, { 0x3A } },
	{0x7D, 1, { 0x43 } },
	{0x7E, 1, { 0x37 } },
	{0x7F, 1, { 0x37 } },
	{0x80, 1, { 0x2D } },
	{0x81, 1, { 0x1F } },
	{0x82, 1, { 0x00 } },
	{0xE0, 1, { 0x02 } },
	{0x00, 1, { 0x1F } },
	{0x01, 1, { 0x1F } },
	{0x02, 1, { 0x13 } },
	{0x03, 1, { 0x11 } },
	{0x04, 1, { 0x0B } },
	{0x05, 1, { 0x0B } },
	{0x06, 1, { 0x09 } },
	{0x07, 1, { 0x09 } },
	{0x08, 1, { 0x07 } },
	{0x09, 1, { 0x1F } },
	{0x0A, 1, { 0x1F } },
	{0x0B, 1, { 0x1F } },
	{0x0C, 1, { 0x1F } },
	{0x0D, 1, { 0x1F } },
	{0x0E, 1, { 0x1F } },
	{0x0F, 1, { 0x07 } },
	{0x10, 1, { 0x05 } },
	{0x11, 1, { 0x05 } },
	{0x12, 1, { 0x01 } },
	{0x13, 1, { 0x03 } },
	{0x14, 1, { 0x1F } },
	{0x15, 1, { 0x1F } },
	{0x16, 1, { 0x1F } },
	{0x17, 1, { 0x1F } },
	{0x18, 1, { 0x12 } },
	{0x19, 1, { 0x10 } },
	{0x1A, 1, { 0x0A } },
	{0x1B, 1, { 0x0A } },
	{0x1C, 1, { 0x08 } },
	{0x1D, 1, { 0x08 } },
	{0x1E, 1, { 0x06 } },
	{0x1F, 1, { 0x1F } },
	{0x20, 1, { 0x1F } },
	{0x21, 1, { 0x1F } },
	{0x22, 1, { 0x1F } },
	{0x23, 1, { 0x1F } },
	{0x24, 1, { 0x1F } },
	{0x25, 1, { 0x06 } },
	{0x26, 1, { 0x04 } },
	{0x27, 1, { 0x04 } },
	{0x28, 1, { 0x00 } },
	{0x29, 1, { 0x02 } },
	{0x2A, 1, { 0x1F } },
	{0x2B, 1, { 0x1F } },
	{0x2C, 1, { 0x1F } },
	{0x2D, 1, { 0x1F } },
	{0x2E, 1, { 0x00 } },
	{0x2F, 1, { 0x02 } },
	{0x30, 1, { 0x08 } },
	{0x31, 1, { 0x08 } },
	{0x32, 1, { 0x0A } },
	{0x33, 1, { 0x0A } },
	{0x34, 1, { 0x04 } },
	{0x35, 1, { 0x1F } },
	{0x36, 1, { 0x1F } },
	{0x37, 1, { 0x1F } },
	{0x38, 1, { 0x1F } },
	{0x39, 1, { 0x1F } },
	{0x3A, 1, { 0x1F } },
	{0x3B, 1, { 0x04 } },
	{0x3C, 1, { 0x06 } },
	{0x3D, 1, { 0x06 } },
	{0x3E, 1, { 0x12 } },
	{0x3F, 1, { 0x10 } },
	{0x40, 1, { 0x1F } },
	{0x41, 1, { 0x1F } },
	{0x42, 1, { 0x1F } },
	{0x43, 1, { 0x1F } },
	{0x44, 1, { 0x01 } },
	{0x45, 1, { 0x03 } },
	{0x46, 1, { 0x09 } },
	{0x47, 1, { 0x09 } },
	{0x48, 1, { 0x0B } },
	{0x49, 1, { 0x0B } },
	{0x4A, 1, { 0x05 } },
	{0x4B, 1, { 0x1F } },
	{0x4C, 1, { 0x1F } },
	{0x4D, 1, { 0x1F } },
	{0x4E, 1, { 0x1F } },
	{0x4F, 1, { 0x1F } },
	{0x50, 1, { 0x1F } },
	{0x51, 1, { 0x05 } },
	{0x52, 1, { 0x07 } },
	{0x53, 1, { 0x07 } },
	{0x54, 1, { 0x13 } },
	{0x55, 1, { 0x11 } },
	{0x56, 1, { 0x1F } },
	{0x57, 1, { 0x1F } },
	{0x58, 1, { 0x40 } },
	{0x59, 1, { 0x00 } },
	{0x5A, 1, { 0x00 } },
	{0x5B, 1, { 0x30 } },
	{0x5C, 1, { 0x02 } },
	{0x5D, 1, { 0x30 } },
	{0x5E, 1, { 0x01 } },
	{0x5F, 1, { 0x02 } },
	{0x60, 1, { 0x30 } },
	{0x61, 1, { 0x01 } },
	{0x62, 1, { 0x02 } },
	{0x63, 1, { 0x03 } },
	{0x64, 1, { 0x6B } },
	{0x65, 1, { 0x75 } },
	{0x66, 1, { 0x08 } },
	{0x67, 1, { 0x73 } },
	{0x68, 1, { 0x02 } },
	{0x69, 1, { 0x03 } },
	{0x6A, 1, { 0x6B } },
	{0x6B, 1, { 0x07 } },
	{0x6C, 1, { 0x00 } },
	{0x6D, 1, { 0x04 } },
	{0x6E, 1, { 0x04 } },
	{0x6F, 1, { 0x88 } },
	{0x70, 1, { 0x00 } },
	{0x71, 1, { 0x00 } },
	{0x72, 1, { 0x06 } },
	{0x73, 1, { 0x7B } },
	{0x74, 1, { 0x00 } },
	{0x75, 1, { 0x3C } },
	{0x76, 1, { 0x00 } },
	{0x77, 1, { 0x0D } },
	{0x78, 1, { 0x2C } },
	{0x79, 1, { 0x00 } },
	{0x7A, 1, { 0x00 } },
	{0x7B, 1, { 0x00 } },
	{0x7C, 1, { 0x00 } },
	{0x7D, 1, { 0x03 } },
	{0x7E, 1, { 0x7B } },
	{0xE0, 1, { 0x01 } },
	{0x0E, 1, { 0x01 } },
	{0xE0, 1, { 0x03 } },
	{0x98, 1, { 0x2F } },
	{0xE0, 1, { 0x00 } },
	{0x11, 0, { 0x00 } },
	{REGFLAG_DELAY, REGFLAG_DELAY, { 200 } },
	{0x29, 0, { 0x00 } },
	{REGFLAG_DELAY, REGFLAG_DELAY, { 5 } },
	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {} }
};

/*add panel initialization code*/
static struct LCM_setting_table LCM_TM040YVZG31_setting[] = {
	{0xB9, 3, {0xFF, 0x83, 0x69} },
	/*Set Power*/
	{0xB1, 19, {0x01, 0x00, 0x34, 0x0A,
				 0x00, 0x11, 0x11, 0x2F, 0x37,
				 0x3F, 0x3F, 0x07, 0x3A, 0x01,
				 0xE6, 0xE6, 0xE6, 0xE6, 0xE6} },
	/* set display,0x2B DSI video mode
	 * 0x20 DSI command mod
	 */
	{0xB2, 15, {0x00, 0x2B, 0x03, 0x03,
				 0x70, 0x00, 0xFF, 0x00, 0x00,
			     0x00, 0x00, 0x03, 0x03, 0x00,
				 0x01} },
	/*set display 480*800
	 *0x00 column inversion, 0x01 1-dot inversion
	 *0x02 2-dot inversion, 0x3 zig-zag inversion
	 */
	{0xB4, 5, {0x00, 0x18, 0x70, 0x00, 0x00} },
	/*Set VCOM*/
	{0xB6, 2, {0x4a, 0x4a} },
	/*set enable chopper function*/
	{0x36, 1, {0x00} },
	/*set GIP*/
	{0xD5, 26, {0x00, 0x00, 0x03, 0x37,
				0x01, 0x02, 0x00, 0x70, 0x11,
				0x13, 0x00, 0x00, 0x60, 0x24,
				0x71, 0x35, 0x00, 0x00, 0x60,
				0x24, 0x71, 0x35, 0x07, 0x0F,
				0x00, 0x04 } },
	/*R Gamma*/
	{0xE0, 34, {0x04, 0x1F, 0x25, 0x2B,
				0X3E, 0x35, 0x34, 0x4A, 0x08,
				0x0E, 0x0F, 0x14, 0x16, 0x14,
				0x15, 0x14, 0x1F, 0x04, 0x1F,
				0x25, 0x2B, 0x3E, 0x35, 0x34,
				0x4A, 0x08, 0x0E, 0x0F, 0x14,
				0x16, 0x14, 0x15, 0x14, 0x1F} },
	{0xCC, 1, {0x02 } },
	{0x3A, 1, {0x77 } },
	 /*set mipi, 0x11 two lanes, 0x10 one lane*/
	{0xBA, 13, {0x00, 0xA0, 0xC6, 0x00,
				0x0A, 0x00, 0x10, 0x30, 0x6F,
			    0x02, 0x11, 0x18, 0x40} },
	/*sleep exit*/
	{0x11, 0, {0x00 } },
	{REGFLAG_DELAY, REGFLAG_DELAY, {120} },
	/*display on*/
	{0x29, 0, {0x00 } },
	{REGFLAG_DELAY, REGFLAG_DELAY, {10} },
	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {} }
};

/*add panel initialization below*/
static struct LCM_setting_table LCM_S070WV20_setting[] = {
	{0x7A, 1, { 0xC1 } },
	{0x20, 1, { 0x20 } },
	{0x21, 1, { 0xE0 } },
	{0x22, 1, { 0x13 } },
	{0x23, 1, { 0x28 } },
	{0x24, 1, { 0x30 } },
	{0x25, 1, { 0x28 } },
	{0x26, 1, { 0x00 } },
	{0x27, 1, { 0x0D } },
	{0x28, 1, { 0x03 } },
	{0x29, 1, { 0x1D } },
	{0x34, 1, { 0x80 } },
	{0x36, 1, { 0x28 } },
	{0xB5, 1, { 0xA0 } },
	{0x5C, 1, { 0xFF } },
	{0x2A, 1, { 0x01 } },
	{0x56, 1, { 0x92 } },
	{0x6B, 1, { 0x71 } },
	{0x69, 1, { 0x2B } },
	{0x10, 1, { 0x40 } },
	{0x11, 1, { 0x98 } },
	{0xB6, 1, { 0x20 } },
	{0x51, 1, { 0x20 } },
#if Bist_mode
	{0x14, 1, { 0x43 } },
	{0x2A, 1, { 0x49 } },
#endif
	{0x09, 1, { 0x10 } },

	{REGFLAG_END_OF_TABLE, REGFLAG_END_OF_TABLE, {} }
};

/*enable dsi clk and send a set of cmds of dsi to init dsi LCD panel*/
static void inet_dsi_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	u8 tmp[5];

	DRM_DEBUG_DRIVER("%s\n", __func__);
	disp_sys_script_get_item("lcd0", "lcd_model_name",
		(int *)model_name, 25);
	dsi_clk_enable(sel, 1);
	sunxi_drm_delayed_ms(20);
	dsi_dcs_wr(sel, DSI_DCS_SOFT_RESET, tmp, 0);
	sunxi_drm_delayed_ms(10);

	for (i = 0; ; i++) {
		if (LCM_LT080B21BA94_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (LCM_LT080B21BA94_setting[i].count == REGFLAG_DELAY)
			sunxi_drm_delayed_ms(
				LCM_LT080B21BA94_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
		else
			dsi_dcs_wr(sel, LCM_LT080B21BA94_setting[i].cmd,
				LCM_LT080B21BA94_setting[i].para_list,
				LCM_LT080B21BA94_setting[i].count);
#endif
		/* break; */
	}
	return;
}

/*send a set of dsi cmd to  dsi LCD panel to make it close*/
static void inet_dsi_exit(u32 sel)
{
	u8 tmp[5];

	DRM_DEBUG_DRIVER("%s\n", __func__);
	dsi_dcs_wr(sel, DSI_DCS_SET_DISPLAY_OFF, tmp, 0);
	sunxi_drm_delayed_ms(20);
	dsi_dcs_wr(sel, DSI_DCS_ENTER_SLEEP_MODE, tmp, 0);
	sunxi_drm_delayed_ms(80);
}


bool inet_dsi_panel_open(struct sunxi_panel *sunxi_panel)
{
	DRM_DEBUG_DRIVER("%s\n", __func__);

	/*lcd_power(vcc-mipi)*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	sunxi_drm_delayed_ms(5);

	/*lcd_power_1(vcc-lcd)*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 1);
	sunxi_drm_delayed_ms(5);

	/*lcd_power_2(vcc-pe)*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 2);
	sunxi_drm_delayed_ms(5);

	/*lcd_gpio_0( ???):set output 1*/
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 1);
	sunxi_drm_delayed_ms(20);

	/*lcd_gpio_0(LCD-RST):set output 1*/
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_drm_delayed_ms(5);

	/*enable lcd_pin_power*/
	/*make the pintrl of lcd0/1 node to active state*/
	/*it usually do NOT have lcd pin power*/
	sunxi_lcd_pin_enalbe(sunxi_panel->private);
	sunxi_drm_delayed_ms(100);

	/*sunxi_lcd = container_of(&sunxi_panel, struct sunxi_lcd_private, sunxi_panel);
	dsi_cfg(sunxi_lcd->lcd_id, sunxi_lcd->panel);*/

	/*enable dsi clk and send a set of cmds of dsi*/
	/*to init dsi LCD panel*/
	inet_dsi_init(0);
	sunxi_drm_delayed_ms(200);

	/*enable encoder(tcon)*/
	sunxi_chain_enable(sunxi_panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(50);
	return true;
}

bool inet_dsi_panel_open_soft(struct sunxi_panel *sunxi_panel)
{
	DRM_DEBUG_DRIVER("%s\n", __func__);

	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 1);
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 2);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 1);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_lcd_pin_soft_enalbe(sunxi_panel->private);
	return true;
}



bool inet_dsi_panel_close(struct sunxi_panel *panel)
{
	DRM_DEBUG_DRIVER("%s\n", __func__);

	sunxi_chain_disable(panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(200);
	inet_dsi_exit(0);
	sunxi_drm_delayed_ms(10);
	sunxi_lcd_extrn_power_off(panel->private, 2);
	sunxi_drm_delayed_ms(5);
	sunxi_lcd_extrn_power_off(panel->private, 1);
	sunxi_drm_delayed_ms(5);
	sunxi_lcd_extrn_power_off(panel->private, 0);
	sunxi_drm_delayed_ms(5);
	return true;
}

static struct  panel_ops  inet_dsi_panel = {
	.init = inet_dsi_panel_init,
	.open = inet_dsi_panel_open,
	.soft_open = inet_dsi_panel_open_soft,
	.close = inet_dsi_panel_close,
	.reset = NULL,
	.bright_light = sunxi_lcd_inet_set_bright,
	.detect  = sunxi_lcd_common_detect,
	.change_mode_to_timming = NULL,
	.check_valid_mode = sunxi_lcd_valid_mode,
	.get_modes = sunxi_lcd_get_modes,
};

static void tm_dsi_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	disp_sys_script_get_item("lcd0", "lcd_model_name",
		(int *)model_name, 25);
	dsi_clk_enable(sel, 1);
	sunxi_drm_delayed_ms(20);

	for (i = 0; ; i++) {
		if (LCM_TM040YVZG31_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (LCM_TM040YVZG31_setting[i].count == REGFLAG_DELAY)
			sunxi_drm_delayed_ms(
				LCM_TM040YVZG31_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
		else
			dsi_dcs_wr(sel, LCM_TM040YVZG31_setting[i].cmd,
				LCM_TM040YVZG31_setting[i].para_list,
				LCM_TM040YVZG31_setting[i].count);
#endif
		/* break; */
	}
	return;
}

bool tm_dsi_panel_open(struct sunxi_panel *sunxi_panel)
{
	/*lcd_power = vcc-mipi*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	sunxi_drm_delayed_ms(5);
	/*lcd_power1 = vcc-lcd*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 1);
	sunxi_drm_delayed_ms(5);

	sunxi_lcd_pin_enalbe(sunxi_panel->private);
	sunxi_drm_delayed_ms(100);
	/*Mipi mode*/
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 2, 1);
	sunxi_drm_delayed_ms(10);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_drm_delayed_ms(1000);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 0);
	sunxi_drm_delayed_ms(10);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_drm_delayed_ms(6);
	tm_dsi_init(0);
	sunxi_drm_delayed_ms(200);
	sunxi_chain_enable(sunxi_panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(50);
	return true;
}


static struct  panel_ops  tm_dsi_panel = {
	.init = inet_dsi_panel_init,
	.open = tm_dsi_panel_open,
	.close = inet_dsi_panel_close,
	.reset = NULL,
	.bright_light = sunxi_lcd_inet_set_bright,
	.detect  = sunxi_common_detect,
	.change_mode_to_timming = NULL,
	.check_valid_mode = sunxi_lcd_valid_mode,
	.get_modes = sunxi_lcd_get_modes,
};

static void bpi_dsi_init(u32 sel)
{
	__u32 i;
	char model_name[25];
	u8 tmp[5];
	disp_sys_script_get_item("lcd0", "lcd_model_name",
		(int *)model_name, 25);
	dsi_clk_enable(sel, 1);
	sunxi_drm_delayed_ms(20);

	for (i = 0; ; i++) {
		if (LCM_S070WV20_setting[i].count == REGFLAG_END_OF_TABLE)
			break;
		else if (LCM_S070WV20_setting[i].count == REGFLAG_DELAY)
			sunxi_drm_delayed_ms(
				LCM_S070WV20_setting[i].para_list[0]);
#ifdef SUPPORT_DSI
		else
			dsi_gen_wr(sel, LCM_S070WV20_setting[i].cmd,
				LCM_S070WV20_setting[i].para_list,
				LCM_S070WV20_setting[i].count);
#endif
		/* break; */
	}
	return;
}

bool bpi_dsi_panel_open(struct sunxi_panel *sunxi_panel)
{
	DRM_INFO("[BPI]LCD_panel_init\n");
	
	/*lcd_power = vcc-mipi*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	sunxi_drm_delayed_ms(5);
	/*lcd_power1 = vcc-lcd*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 1);
	sunxi_drm_delayed_ms(5);

	sunxi_lcd_pin_enalbe(sunxi_panel->private);
	sunxi_drm_delayed_ms(100);
	/*Mipi mode*/
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_drm_delayed_ms(10);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 1);
	sunxi_drm_delayed_ms(1000);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 0);
	sunxi_drm_delayed_ms(10);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 1);
	sunxi_drm_delayed_ms(6);
	bpi_dsi_init(0);
	sunxi_drm_delayed_ms(200);
	sunxi_chain_enable(sunxi_panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(50);
	return true;
}

bool bpi_dsi_panel_soft_open(struct sunxi_panel *sunxi_panel)
{
	/*lcd_power = vcc-mipi*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 0);
	/*lcd_power1 = vcc-lcd*/
	sunxi_lcd_extrn_power_on(sunxi_panel->private, 1);

	/*lcd_pin_power*/
	sunxi_lcd_pin_soft_enalbe(sunxi_panel->private);

	/*Mipi mode*/
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 1, 1);
	sunxi_lcd_sys_gpio_set_value(sunxi_panel->private, 0, 1);

	return true;
}

bool bpi_dsi_panel_close(struct sunxi_panel *panel)
{
	sunxi_chain_disable(panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(200);

	sunxi_lcd_extrn_power_off(panel->private, 2);
	sunxi_drm_delayed_ms(5);
	sunxi_lcd_extrn_power_off(panel->private, 1);
	sunxi_drm_delayed_ms(5);
	sunxi_lcd_extrn_power_off(panel->private, 0);
	sunxi_drm_delayed_ms(5);
	return true;
}

static struct  panel_ops  bpi_lcd7_panel = {
	.init = inet_dsi_panel_init,
	.open = bpi_dsi_panel_open,
	.soft_open = bpi_dsi_panel_soft_open,
	.close = bpi_dsi_panel_close,
	.reset = NULL,
	.bright_light = sunxi_lcd_inet_set_bright,
	.detect  = sunxi_lcd_common_detect,
	.change_mode_to_timming = NULL,
	.check_valid_mode = sunxi_lcd_valid_mode,
	.get_modes = sunxi_lcd_get_modes,
};

int sunxi_get_lcd_sys_info(struct sunxi_lcd_private *sunxi_lcd)
{
	struct disp_gpio_set_t  *gpio_info;
	char primary_key[20], sub_name[25];
	int i = 0, ret = -EINVAL, value = 1;
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;
	struct disp_panel_para  *info = sunxi_lcd->panel;
	char compat[32];
	struct device_node *node;

	sprintf(primary_key, "sunxi-lcd%d", sunxi_lcd->lcd_id);
	node = sunxi_drm_get_name_node(primary_key);
	if (!node) {
		DRM_ERROR("of_find_compatible_node %s fail\n", compat);
		return -EINVAL;
	}
	/* lcd_used */
	ret = sunxi_drm_get_sys_item_int(node, "lcd_used", &value);
	if (ret == 0) {
		lcd_cfg->lcd_used = value;
	}

	if (lcd_cfg->lcd_used == 0) {
		return -EINVAL;
	}

	/* lcd_bl_en */
	lcd_cfg->lcd_bl_en_used = 0;
	gpio_info = &(lcd_cfg->lcd_bl_en);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_bl_en", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_bl_en_used = 1;
		DRM_INFO("get gpio:lcd_bl_en: ");
		DRM_INFO("name:%s port:%d  port_num:%d mul_sel:%d pull:%d \
							drv_level:%d data:%d gpio:%d\n",
							gpio_info->gpio_name, gpio_info->port,
							gpio_info->port_num, gpio_info->mul_sel,
							gpio_info->pull, gpio_info->drv_level,
							gpio_info->data, gpio_info->gpio);
	}

	sprintf(sub_name, "lcd_bl_en_power");
	ret = sunxi_drm_get_sys_item_char(node, sub_name,
		lcd_cfg->lcd_bl_en_power);
	if (ret == 0)
		DRM_INFO("get %s(%s)\n", sub_name,
				lcd_cfg->lcd_bl_en_power);

	/* lcd fix power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0) {
			sprintf(sub_name, "lcd_fix_power");
		} else {
			sprintf(sub_name, "lcd_fix_power%d", i);
		}

		lcd_cfg->lcd_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_fix_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_fix_power_used[i] = 1;
			DRM_INFO("get %s(%s)\n", sub_name,
					lcd_cfg->lcd_fix_power[i]);
		}
	}

	/* lcd_power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0) {
			sprintf(sub_name, "lcd_power");
		} else {
			sprintf(sub_name, "lcd_power%d", i);
		}
		lcd_cfg->lcd_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_power_used[i] = 1;
			DRM_INFO("get %s(%s)\n", sub_name,
					lcd_cfg->lcd_power[i]);
		}
	}

	/* lcd_gpio */
	for (i = 0; i < LCD_GPIO_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_%d", i);
		gpio_info = &(lcd_cfg->lcd_gpio[i]);
		ret = sunxi_drm_get_sys_item_gpio(node, sub_name, gpio_info);
		if (ret == 0) {
			lcd_cfg->lcd_gpio_used[i] = 1;
			DRM_INFO("get gpio:%s: ", sub_name);
			DRM_INFO("name:%s port:%d  port_num:%d mul_sel:%d pull:%d \
								drv_level:%d data:%d gpio:%d\n",
							gpio_info->gpio_name, gpio_info->port,
							gpio_info->port_num, gpio_info->mul_sel,
							gpio_info->pull, gpio_info->drv_level,
							gpio_info->data, gpio_info->gpio);
		}
	}

	/*lcd_gpio_scl,lcd_gpio_sda*/
	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_scl", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL] = 1;
		DRM_INFO("get lcd_gpio_scl: ");
		DRM_INFO("name:%s port:%d  port_num:%d mul_sel:%d pull:%d \
								drv_level:%d data:%d gpio:%d\n",
							gpio_info->gpio_name, gpio_info->port,
							gpio_info->port_num, gpio_info->mul_sel,
							gpio_info->pull, gpio_info->drv_level,
							gpio_info->data, gpio_info->gpio);
	}

	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_sda", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA] = 1;
		DRM_INFO("get lcd_gpio_sda: ");
		DRM_INFO("name:%s port:%d  port_num:%d mul_sel:%d pull:%d \
								drv_level:%d data:%d gpio:%d\n",
							gpio_info->gpio_name, gpio_info->port,
							gpio_info->port_num, gpio_info->mul_sel,
							gpio_info->pull, gpio_info->drv_level,
							gpio_info->data, gpio_info->gpio);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_power%d", i);
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_gpio_power[i]);
		if (ret == 0)
			DRM_INFO("get %s(%s)\n", sub_name,
					lcd_cfg->lcd_gpio_power[i]);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (0 == i) {
			sprintf(sub_name, "lcd_pin_power");
		} else {
			sprintf(sub_name, "lcd_pin_power%d", i);
		}
		ret = sunxi_drm_get_sys_item_char(node, sub_name,
			lcd_cfg->lcd_pin_power[i]);
		if (ret == 0)
			DRM_INFO("get %s(%s)\n", sub_name,
					lcd_cfg->lcd_pin_power[i]);
	}

	/* backlight adjust */
	for (i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if (i == 100) {
			lcd_cfg->backlight_curve_adjust[i] = 255;
		}

		ret = sunxi_drm_get_sys_item_int(node, sub_name, &value);
		if (ret == 0) {
			value = (value > 100) ? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}

	sprintf(sub_name, "lcd_backlight");
	ret = sunxi_drm_get_sys_item_int(node, sub_name, &value);
	if (ret == 0) {
		value = (value > 256) ? 256:value;
		lcd_cfg->backlight_bright = value;
	} else {
		lcd_cfg->backlight_bright = 197;
	}
	lcd_cfg->lcd_bright = lcd_cfg->backlight_bright;
	/* display mode */
	ret = sunxi_drm_get_sys_item_int(node, "lcd_x", &value);
	if (ret == 0) {
		info->lcd_x = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_y", &value);
	if (ret == 0) {
		info->lcd_y = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_width", &value);
	if (ret == 0) {
		info->lcd_width = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_height", &value);
	if (ret == 0) {
		info->lcd_height = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dclk_freq", &value);
	if (ret == 0) {
		info->lcd_dclk_freq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_used", &value);
	if (ret == 0) {
		info->lcd_pwm_used = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_ch", &value);
	if (ret == 0) {
		info->lcd_pwm_ch = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_freq", &value);
	if (ret == 0) {
		info->lcd_pwm_freq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_pol", &value);
	if (ret == 0) {
		info->lcd_pwm_pol = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_if", &value);
	if (ret == 0) {
	info->lcd_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hbp", &value);
	if (ret == 0) {
		info->lcd_hbp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_ht", &value);
	if (ret == 0) {
		info->lcd_ht = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vbp", &value);
	if (ret == 0) {
		info->lcd_vbp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vt", &value);
	if (ret == 0) {
		info->lcd_vt = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_if", &value);
	if (ret == 0) {
		info->lcd_hv_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vspw", &value);
	if (ret == 0) {
		info->lcd_vspw = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hspw", &value);
	if (ret == 0) {
		info->lcd_hspw = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_if", &value);
	if (ret == 0) {
		info->lcd_lvds_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_mode", &value);
	if (ret == 0) {
		info->lcd_lvds_mode = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_colordepth",
		&value);
	if (ret == 0) {
		info->lcd_lvds_colordepth = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_io_polarity",
		&value);
	if (ret == 0) {
		info->lcd_lvds_io_polarity = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cpu_if", &value);
	if (ret == 0) {
		info->lcd_cpu_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cpu_te", &value);
	if (ret == 0) {
		info->lcd_cpu_te = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_frm", &value);
	if (ret == 0) {
		info->lcd_frm = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_if", &value);
	if (ret == 0) {
		info->lcd_dsi_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_lane", &value);
	if (ret == 0) {
		info->lcd_dsi_lane = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_format",
		&value);
	if (ret == 0) {
		info->lcd_dsi_format = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_eotp", &value);
	if (ret == 0) {
		info->lcd_dsi_eotp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_te", &value);
	if (ret == 0) {
		info->lcd_dsi_te = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_rate", &value);
	if (ret == 0) {
		info->lcd_edp_rate = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_lane", &value);
	if (ret == 0) {
		info->lcd_edp_lane = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_colordepth", &value);
	if (ret == 0) {
		info->lcd_edp_colordepth = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_fps", &value);
	if (ret == 0) {
		info->lcd_edp_fps = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_clk_phase", &value);
	if (ret == 0) {
		info->lcd_hv_clk_phase = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_sync_polarity",
		&value);
	if (ret == 0) {
		info->lcd_hv_sync_polarity = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_srgb_seq", &value);
	if (ret == 0) {
		info->lcd_hv_srgb_seq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_rb_swap", &value);
	if (ret == 0) {
		info->lcd_rb_swap = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_syuv_seq", &value);
	if (ret == 0) {
		info->lcd_hv_syuv_seq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_syuv_fdly", &value);
	if (ret == 0) {
		info->lcd_hv_syuv_fdly = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_gamma_en", &value);
	if (ret == 0) {
		info->lcd_gamma_en = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cmap_en", &value);
	if (ret == 0) {
		info->lcd_cmap_en = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_xtal_freq", &value);
	if (ret == 0) {
		info->lcd_xtal_freq = value;
	}

	ret = sunxi_drm_get_sys_item_char(node, "lcd_size",
		(void *)info->lcd_size);
	ret = sunxi_drm_get_sys_item_char(node, "lcd_model_name",
		(void *)info->lcd_model_name);

	return 0;
}

/*enable lcd_pin_power*/
/*make the pintrl of lcd0/1 node to active state*/
static int sunxi_lcd_pin_enalbe(struct sunxi_lcd_private *sunxi_lcd)
{
	int  i;
	/*char dev_name[25];*/
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_pin_power[i], "")) ||
		(!strcmp(lcd_cfg->lcd_pin_power[i], "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_pin_power[i]);
		}
	}

	/*sprintf(dev_name, "lcd%d", sunxi_lcd->lcd_id);
	sunxi_drm_sys_pin_set_state(dev_name, DISP_PIN_STATE_ACTIVE);*/

	if (LCD_IF_DSI ==  sunxi_lcd->panel->lcd_if) {
		dsi_io_open(0, sunxi_lcd->panel);
	}

	return 0;
}

static int sunxi_lcd_pin_soft_enalbe(struct sunxi_lcd_private *sunxi_lcd)
{
	int  i;
	/*char dev_name[25];*/
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_pin_power[i], "")) ||
		(!strcmp(lcd_cfg->lcd_pin_power[i], "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_pin_power[i]);
		}
	}

	/*sprintf(dev_name, "lcd%d", sunxi_lcd->lcd_id);
	sunxi_drm_sys_pin_set_state(dev_name, DISP_PIN_STATE_ACTIVE);*/

	return 0;
}


/*static int sunxi_lcd_pin_disable(struct sunxi_lcd_private *sunxi_lcd)
{
	int  i;
	char dev_name[25];
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	sprintf(dev_name, "lcd%d", sunxi_lcd->lcd_id);
	sunxi_drm_sys_pin_set_state(dev_name, DISP_PIN_STATE_SLEEP);

	if (LCD_IF_DSI ==  sunxi_lcd->panel->lcd_if) {
		dsi_io_close(sunxi_lcd->lcd_id);
	}

	for (i = LCD_GPIO_REGU_NUM-1; i >= 0; i--) {
		if (!((!strcmp(lcd_cfg->lcd_pin_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_pin_power[i], "none")))) {

			sunxi_drm_sys_power_disable(
				lcd_cfg->lcd_pin_power[i]);
		}
	}

	return 0;
}*/

static ssize_t type_show(struct device *dev,
			struct device_attribute *attr,
					char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "%s\n", "raw");
	return n;
}

static ssize_t type_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(type, S_IRUGO|S_IWUSR|S_IWGRP, type_show, type_store);

static ssize_t brightness_show(struct device *dev,
			struct device_attribute *attr,
					char *buf)
{
	ssize_t n = 0;
	struct sunxi_lcd_private *lcd_priv = sunxi_lcd_get_lcd_priv();

	n += sprintf(buf + n, "%d", lcd_priv->lcd_cfg->backlight_bright);

	return n;
}

static ssize_t brightness_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct sunxi_lcd_private *lcd_priv = sunxi_lcd_get_lcd_priv();
	unsigned long data;
	int err;

	err = kstrtoul(buf, 0, &data);
	if (err) {
		pr_err("Invalid size\n");
		return err;
	}

	if (count < 1)
		return -EINVAL;

	sunxi_common_pwm_set_bl(lcd_priv, (unsigned int)data);

	return count;
}

static DEVICE_ATTR(brightness, S_IRUGO|S_IWUSR|S_IWGRP,
			brightness_show, brightness_store);

static ssize_t max_brightness_show(struct device *dev,
			struct device_attribute *attr,
					char *buf)
{
	ssize_t n = 0;
	int max = 255;

	n += sprintf(buf + n, "%d\n", max);
	return n;
}

static ssize_t max_brightness_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(max_brightness, S_IRUGO|S_IWUSR|S_IWGRP,
			max_brightness_show, max_brightness_store);

static ssize_t actual_brightness_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t n = 0;
	struct sunxi_lcd_private *lcd_priv = sunxi_lcd_get_lcd_priv();

	n += sprintf(buf + n, "%d", lcd_priv->lcd_cfg->backlight_bright);

	return n;
}

static ssize_t actual_brightness_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	if (count < 1)
		return -EINVAL;

	return count;
}

static DEVICE_ATTR(actual_brightness, S_IRUGO|S_IWUSR|S_IWGRP,
			actual_brightness_show, actual_brightness_store);
static struct attribute *pwm_dev_attrs[] = {
	&dev_attr_type.attr,
	&dev_attr_brightness.attr,
	&dev_attr_max_brightness.attr,
	&dev_attr_actual_brightness.attr,
	NULL
};

static const struct attribute_group pwm_dev_group = {
	.attrs = pwm_dev_attrs,
};

static const struct attribute_group *pwm_attr_groups[] = {
	&pwm_dev_group,
	NULL
};

int sunxi_pwm_dev_init(struct sunxi_lcd_private *sunxi_lcd)
{
	__u64 backlight_bright;
	__u64 period_ns, duty_ns;
	struct disp_panel_para  *panel_info = sunxi_lcd->panel;
	struct pwm_info_t  *pwm_info;

	sunxi_lcd_set_lcd_priv(sunxi_lcd);
	if (panel_info->lcd_pwm_used) {
		alloc_chrdev_region(&sunxi_lcd->bright_devno,
					0, 1, "backlight");

		/*Create a path: sys/class/backlight*/
		sunxi_lcd->bright_class = class_create(THIS_MODULE, "backlight");
		if (IS_ERR(sunxi_lcd->bright_class)) {
			DRM_ERROR("Error:brightness class_create fail\n");
			return -1;
		}

		/*Create a path "sys/class/brightness/pwm"*/
		sunxi_lcd->bright_dev =
			device_create_with_groups(sunxi_lcd->bright_class,
					sunxi_drm_get_dev(), sunxi_lcd->bright_devno, NULL,
						pwm_attr_groups, "pwm");

		pwm_info = kzalloc(sizeof(struct pwm_info_t), GFP_KERNEL);
		if (!pwm_info) {
			DRM_ERROR("failed to alloc pwm_info.\n");
			return -ENOMEM;
		}

		pwm_info->channel = panel_info->lcd_pwm_ch;
		pwm_info->polarity = panel_info->lcd_pwm_pol;
		pwm_info->pwm_dev = pwm_request(panel_info->lcd_pwm_ch,
			"lcd");

		if (!pwm_info->pwm_dev) {
			DRM_ERROR("failed to pwm_request pwm_dev.\n");
			kfree(pwm_info);
			return -ENOMEM;
		}

		if (panel_info->lcd_pwm_freq != 0) {
			period_ns = 1000*1000*1000 / panel_info->lcd_pwm_freq;
		} else {
			DRM_INFO("lcd pwm use default Hz.\n");
			/* default 1khz */
			period_ns = 1000*1000*1000 / 1000;
		}

		backlight_bright = sunxi_lcd->lcd_cfg->backlight_bright;

		duty_ns = (backlight_bright * period_ns) / 256;
		pwm_set_polarity(pwm_info->pwm_dev, pwm_info->polarity);
		pwm_info->duty_ns = duty_ns;
		pwm_info->period_ns = period_ns;
		sunxi_lcd->pwm_info = pwm_info;
		DRM_DEBUG_KMS("[%d] %s %d", __LINE__, __func__,
			pwm_info->polarity);
		DRM_DEBUG_KMS("duty_ns:%lld period_ns:%lld\n",
			duty_ns, period_ns);
	}

	return 0;
}

void sunxi_pwm_dev_destroy(struct pwm_info_t  *pwm_info)
{
	if (pwm_info) {
		if (pwm_info->pwm_dev)
			pwm_free(pwm_info->pwm_dev);
		kfree(pwm_info);
	}
}

int sunxi_common_pwm_set_bl(struct sunxi_lcd_private *sunxi_lcd,
	unsigned int bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct panel_extend_para *extend_panel;
	struct disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	extend_panel = sunxi_lcd->extend_panel;

	if (sunxi_lcd->pwm_info->pwm_dev && extend_panel != NULL) {
		if (backlight_bright != 0) {
			backlight_bright += 1;
		}
		DRM_DEBUG_DRIVER("[%d] %s light:%d", __LINE__, __func__,
			bright);
		bright = (bright > 255) ? 255:bright;
		backlight_bright =
			extend_panel->lcd_bright_curve_tbl[bright];

		sunxi_lcd->lcd_cfg->backlight_dimming =
			(0 == sunxi_lcd->lcd_cfg->backlight_dimming) ?
			256:sunxi_lcd->lcd_cfg->backlight_dimming;
		backlight_dimming = sunxi_lcd->lcd_cfg->backlight_dimming;
		period_ns = sunxi_lcd->pwm_info->period_ns;
		duty_ns = (backlight_bright * backlight_dimming *
			period_ns/256 + 128) / 256;
		sunxi_lcd->pwm_info->duty_ns = duty_ns;
		lcd_cfg->backlight_bright = bright;
		DRM_DEBUG_KMS("bright:%d duty_ns:%d period_ns:%llu\n",
			bright, duty_ns, period_ns);

		sunxi_drm_sys_pwm_config(sunxi_lcd->pwm_info->pwm_dev,
			duty_ns, period_ns);
	}
	return 0;
}

int sunxi_lcd_panel_ops_init(struct sunxi_panel *sunxi_panel)
{
	char primary_key[20];
	int  ret;
	char drv_name[20];
	struct device_node *node;
	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;

	sprintf(primary_key, "sunxi-lcd%d", sunxi_lcd->lcd_id);
	node = sunxi_drm_get_name_node(primary_key);

	ret = sunxi_drm_get_sys_item_char(node, "lcd_driver_name",
		drv_name);
	if (!strcmp(drv_name, "default_lcd")) {
		sunxi_panel->panel_ops = &default_panel;
		sunxi_panel->panel_ops->init(sunxi_panel);
		return 0;
	} else if (!strcmp(drv_name, "inet_dsi_panel")) {
		sunxi_panel->panel_ops = &inet_dsi_panel;
		sunxi_panel->panel_ops->init(sunxi_panel);
		return 0;
	} else if (!strcmp(drv_name, "tm_dsi_panel")) {
		sunxi_panel->panel_ops = &tm_dsi_panel;
		sunxi_panel->panel_ops->init(sunxi_panel);
		return 0;
	} else if (!strcmp(drv_name, "bpi_lcd7_panel")) {
		sunxi_panel->panel_ops = &bpi_lcd7_panel;
		sunxi_panel->panel_ops->init(sunxi_panel);
		return 0;
	}

	DRM_ERROR("failed to init sunxi_panel.\n");
	return -EINVAL;
}

void sunxi_lcd_panel_ops_destroy(struct panel_ops  *panel_ops)
{
	return;
}

/*enable fix_power(vcc-dsi-33)*/
static bool sunxi_fix_power_enable(struct disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_fix_power_used[i]) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_fix_power[i]);
		}
	}
	return true;
}

/*disable fix_power(vcc-dsi-33)*/
static bool sunxi_fix_power_disable(struct disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_fix_power_used[i]) {
			sunxi_drm_sys_power_disable(
				lcd_cfg->lcd_fix_power[i]);
		}
	}
	return true;
}

/*enable lcd gpio power*/
/*request lcd_gpio_0/1*/
static int sunxi_gpio_request(struct disp_lcd_cfg  *lcd_cfg)
{
	int i;
	struct disp_gpio_set_t  gpio_info[1];
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_gpio_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_gpio_power[i], "none")))) {
			sunxi_drm_sys_power_enable(
				lcd_cfg->lcd_gpio_power[i]);
		}
	}

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		lcd_cfg->gpio_hdl[i] = 0;

		if (lcd_cfg->lcd_gpio_used[i]) {
			memcpy(gpio_info, &(lcd_cfg->lcd_gpio[i]),
				sizeof(struct disp_gpio_set_t));
			if (lcd_cfg->gpio_hdl[i])
				sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);
			lcd_cfg->gpio_hdl[i] =
				sunxi_drm_sys_gpio_request(gpio_info);
			if (lcd_cfg->gpio_hdl[i])
				DRM_INFO("request gpio:%s success, hdl:%d\n",
						gpio_info->gpio_name,
						lcd_cfg->gpio_hdl[i]);
		}
	}

	return 0;
}

/*release lcd_gpio_0/1*/
/*disable lcd gpio power*/
static int sunxi_gpio_release(struct disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_GPIO_NUM; i++) {
		if (lcd_cfg->gpio_hdl[i]) {
			struct disp_gpio_set_t  gpio_info[1];

			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);

			memcpy(gpio_info, &(lcd_cfg->lcd_gpio[i]),
				sizeof(struct disp_gpio_set_t));
			gpio_info->mul_sel = 7;
			lcd_cfg->gpio_hdl[i] =
				sunxi_drm_sys_gpio_request(gpio_info);
			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);
			lcd_cfg->gpio_hdl[i] = 0;
		}
	}

	for (i = LCD_GPIO_REGU_NUM-1; i >= 0; i--) {
		if (!((!strcmp(lcd_cfg->lcd_gpio_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_gpio_power[i], "none")))) {
			sunxi_drm_sys_power_disable(
				lcd_cfg->lcd_gpio_power[i]);
		}
	}

	return 0;
}

bool sunxi_common_enable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_panel *sunxi_panel;
	struct panel_ops *panel_ops;
	struct disp_lcd_cfg   *lcd_cfg;
	struct sunxi_lcd_private *sunxi_lcd;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	sunxi_connector = to_sunxi_connector(data);
	hw_res = sunxi_connector->hw_res;
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = sunxi_panel->private;
	panel_ops = sunxi_panel->panel_ops;
	lcd_cfg = sunxi_lcd->lcd_cfg;

	/*enable fix_power(vcc-dsi-33)*/
	sunxi_fix_power_enable(lcd_cfg);
	/*enable lcd gpio power*/
	/*request lcd_gpio_0/1*/
	sunxi_gpio_request(lcd_cfg);

	sunxi_clk_set(sunxi_connector->hw_res);
	/* after enable the tcon ,THK ok?*/
	sunxi_clk_enable(sunxi_connector->hw_res);
	dsi_cfg(sunxi_lcd->lcd_id, sunxi_lcd->panel);


#if 0
	sunxi_lcd_pin_enalbe(sunxi_lcd);
#endif
	panel_ops->open(sunxi_panel);

	panel_ops->bright_light(sunxi_panel, lcd_cfg->lcd_bright);

	sunxi_lcd_extern_backlight_enable(sunxi_lcd);
	sunxi_drm_sys_pwm_enable(sunxi_lcd->pwm_info);

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	sunxi_irq_request(sunxi_connector->hw_res);
#endif

	return true;
}

static bool sunxi_common_enable_soft(void *data)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_panel *sunxi_panel;
	struct panel_ops *panel_ops;
	struct disp_lcd_cfg   *lcd_cfg;
	struct sunxi_lcd_private *sunxi_lcd;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	sunxi_connector = to_sunxi_connector(data);
	hw_res = sunxi_connector->hw_res;
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = sunxi_panel->private;
	panel_ops = sunxi_panel->panel_ops;
	lcd_cfg = sunxi_lcd->lcd_cfg;

	sunxi_fix_power_enable(lcd_cfg);
	sunxi_gpio_request(lcd_cfg);

	sunxi_clk_set(sunxi_connector->hw_res);

	/* after enable the tcon ,THK ok?*/
	sunxi_clk_enable(sunxi_connector->hw_res);

	sunxi_lcd_extern_backlight_enable(sunxi_lcd);
	sunxi_drm_sys_pwm_enable(sunxi_lcd->pwm_info);

	if (panel_ops->soft_open)
		panel_ops->soft_open(sunxi_panel);

	return true;
}

static bool sunxi_common_disable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_panel *sunxi_panel;
	struct panel_ops *panel_ops;
	struct disp_lcd_cfg      *lcd_cfg;
	struct sunxi_lcd_private *sunxi_lcd;

	DRM_DEBUG_DRIVER("%s\n", __func__);
	sunxi_connector = to_sunxi_connector(data);
	hw_res = sunxi_connector->hw_res;
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = sunxi_panel->private;
	panel_ops = sunxi_panel->panel_ops;
	lcd_cfg = sunxi_lcd->lcd_cfg;

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	sunxi_irq_free(hw_res);
#endif

	panel_ops->bright_light(sunxi_panel, 0);
	sunxi_lcd_extern_backlight_disable(sunxi_lcd);
	sunxi_drm_sys_pwm_disable(sunxi_lcd->pwm_info);
	panel_ops->close(sunxi_panel);
#if 0
	sunxi_lcd_pin_disable(sunxi_lcd);
#endif
	sunxi_clk_disable(hw_res);
	sunxi_gpio_release(lcd_cfg);
	sunxi_fix_power_disable(lcd_cfg);
	return true;
}

static bool sunxi_dsi_irq_query(void *data, int need_irq)
{
#ifdef CONFIG_ARCH_SUN8IW11
	enum __dsi_irq_id_t id;
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	enum __dsi_irq_id_t id;
#endif

	DRM_DEBUG_DRIVER("%s:", __func__);
	switch (need_irq) {
	case QUERY_VSYNC_IRQ:
		id = DSI_IRQ_VIDEO_VBLK;
		break;
	default:
		drm_err("ERROR irq:%d\n", need_irq);
		return false;
	}
	/* bug for sunxi_connector->connector_id */
	if (dsi_irq_query(0, id))
		return true;

	return false;
}

static bool sunxi_dsi_init(void *data)
{
	struct sunxi_panel *sunxi_panel;
	struct sunxi_lcd_private *sunxi_lcd;
	struct sunxi_drm_connector *sunxi_connector =
	to_sunxi_connector(data);

	DRM_DEBUG_DRIVER("%s\n", __func__);
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = (struct sunxi_lcd_private *)sunxi_panel->private;
	sunxi_connector->hw_res->clk_rate = sunxi_lcd->clk_info->dsi_rate;

	return true;
}

static bool sunxi_lvds_init(void *data)
{
	return true;
}

static bool sunxi_lvds_reset(void *data)
{
	struct drm_connector *connector = (struct drm_connector *)data;
	struct sunxi_drm_connector *sunxi_connector =
			to_sunxi_connector(data);
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	if (!connector || !connector->encoder ||
		!connector->encoder->crtc) {
		return false;
	}
	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	sunxi_crtc = to_sunxi_crtc(connector->encoder->crtc);
	sunxi_clk_enable(sunxi_connector->hw_res);

	return true;
}

static bool sunxi_dsi_reset(void *data)
{
	struct drm_connector *connector = (struct drm_connector *)data;
	struct sunxi_drm_connector *sunxi_connector =
	to_sunxi_connector(data);
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_lcd_private *sunxi_lcd;

	DRM_DEBUG_DRIVER("%s:", __func__);
	if (!connector || !connector->encoder ||
		!connector->encoder->crtc) {
		drm_err("NULL handle and exit!");
		return false;
	}

	DRM_DEBUG_DRIVER("do reset!\n");
	sunxi_lcd = (struct sunxi_lcd_private *)
		sunxi_connector->panel->private;
	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	sunxi_crtc = to_sunxi_crtc(connector->encoder->crtc);

	return true;
}

static struct sunxi_hardware_ops dsi_con_ops = {
	.init = sunxi_dsi_init,
	.reset = sunxi_dsi_reset,
	.enable = sunxi_common_enable,
	.soft_enable = sunxi_common_enable_soft,
	.disable = sunxi_common_disable,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.irq_query = sunxi_dsi_irq_query,
	.vsync_delayed_do = NULL,
	.set_timming = NULL,
};

static struct sunxi_hardware_ops edp_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops lvds_con_ops = {
	.init = sunxi_lvds_init,
	.reset = sunxi_lvds_reset,
	.enable = sunxi_common_enable,
	.disable = sunxi_common_disable,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.irq_query = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops hv_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops cpu_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops edsi_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static int sunxi_lcd_hwres_ops_init(struct sunxi_hardware_res *hw_res,
	int type)
{
	switch (type) {
	case LCD_IF_HV:
		hw_res->ops = &hv_con_ops;
		break;
	case LCD_IF_CPU:
		hw_res->ops = &cpu_con_ops;
		break;
	case LCD_IF_LVDS:
		hw_res->ops = &lvds_con_ops;
		break;
	case LCD_IF_DSI:
		hw_res->ops = &dsi_con_ops;
		break;
	case LCD_IF_EDP:
		hw_res->ops = &edp_con_ops;
		break;
	case LCD_IF_EXT_DSI:
		hw_res->ops = &edsi_con_ops;
		break;
	default:
		DRM_ERROR("give us an err hw_res.\n");
		return -EINVAL;
	}
	return 0;
}

static void sunxi_lcd_hwres_ops_destroy(struct sunxi_hardware_res *hw_res)
{
	hw_res->ops = NULL;
}

static bool sunxi_cmp_panel_type(enum disp_mod_id res_id, int type)
{
	/* TODO for  DISP_MOD_EINK HDMI */
	/* 0:hv(sync+de); 1:8080; 2:ttl; 3:lvds; 4:dsi; 5:edp */
	switch ((int)(res_id)) {
	case DISP_MOD_DSI0:
		if (type == 4)
			return true;
		break;
	case DISP_MOD_DSI1:
		if (type == 4)
			return true;
		break;
		/* for :0:hv(sync+de); 1:8080; 2:ttl; */
	case DISP_MOD_DSI2:
		if (type < 3)
			return true;
		break;
	case DISP_MOD_HDMI:
		if (type == 7)
			return true;
		break;
	case DISP_MOD_LVDS:
		if (type == 3)
			return true;
		break;

	default:
		DRM_ERROR("[%s][%s]get a err lcd type.\n",
			__FILE__, __func__);
	}

	return false;
}

static int sunxi_lcd_private_init(struct sunxi_panel *sunxi_panel, int lcd_id)
{
	struct sunxi_lcd_private *sunxi_lcd_p;
	sunxi_lcd_p = kzalloc(sizeof(struct sunxi_lcd_private), GFP_KERNEL);
	if (!sunxi_lcd_p) {
		DRM_ERROR("failed to allocate sunxi_lcd_p.\n");
		return -EINVAL;
	}
	sunxi_panel->private = sunxi_lcd_p;

	sunxi_lcd_p->lcd_cfg = kzalloc(sizeof(struct disp_lcd_cfg), GFP_KERNEL);
	if (!sunxi_lcd_p->lcd_cfg) {
		DRM_ERROR("failed to allocate lcd_cfg.\n");
		goto lcd_err;
	}

	sunxi_lcd_p->extend_panel = kzalloc(sizeof(struct panel_extend_para),
		GFP_KERNEL);
	if (!sunxi_lcd_p->extend_panel) {
		DRM_ERROR("failed to allocate extend_panel.\n");
		goto lcd_err;
	}

	sunxi_lcd_p->panel = kzalloc(sizeof(struct disp_panel_para),
		GFP_KERNEL);
	if (!sunxi_lcd_p->panel) {
		DRM_ERROR("failed to allocate panel.\n");
		goto lcd_err;
	}

	sunxi_lcd_p->lcd_id = lcd_id;
	sunxi_get_lcd_sys_info(sunxi_lcd_p);
	if (sunxi_pwm_dev_init(sunxi_lcd_p))
		goto lcd_err;

	return 0;

lcd_err:
	if (sunxi_lcd_p->panel)
		kfree(sunxi_lcd_p->panel);
	if (sunxi_lcd_p->extend_panel)
		kfree(sunxi_lcd_p->extend_panel);
	if (sunxi_lcd_p->lcd_cfg)
		kfree(sunxi_lcd_p->lcd_cfg);
	return -EINVAL;
}

static void sunxi_lcd_private_destroy(struct sunxi_lcd_private *sunxi_lcd_p)
{
	if (sunxi_lcd_p->panel)
		kfree(sunxi_lcd_p->panel);
	if (sunxi_lcd_p->extend_panel)
		kfree(sunxi_lcd_p->extend_panel);
	if (sunxi_lcd_p->lcd_cfg)
		kfree(sunxi_lcd_p->lcd_cfg);
	sunxi_pwm_dev_destroy(sunxi_lcd_p->pwm_info);
	kfree(sunxi_lcd_p);
}

struct sunxi_panel *sunxi_lcd_init(struct sunxi_hardware_res *hw_res,
	int panel_id, int lcd_id)
{
	char primary_key[20];
	int value, ret;
	struct sunxi_panel *sunxi_panel = NULL;
	struct device_node *node;

	sprintf(primary_key, "sunxi-lcd%d", lcd_id);
	node = sunxi_drm_get_name_node(primary_key);
	if (!node) {
		DRM_ERROR("get device [%s] node fail.\n", primary_key);
		return NULL;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_used", &value);
	if (ret == 0) {
		if (value == 1) {
			ret = sunxi_drm_get_sys_item_int(node,
				"lcd_if", &value);
			if (ret == 0) {
				if (!sunxi_cmp_panel_type(hw_res->res_id,
					value)) {
					goto err_false;
				}

				sunxi_panel = sunxi_panel_creat(
					DISP_OUTPUT_TYPE_LCD, panel_id);
				if (!sunxi_panel) {
					DRM_ERROR("creat sunxi panel fail.\n");
					goto err_false;
				}

				ret = sunxi_lcd_private_init(sunxi_panel,
					lcd_id);
				if (ret) {
					DRM_ERROR("creat lcd_private \
						fail.\n");
					goto err_panel;
				}

				ret = sunxi_lcd_hwres_ops_init(hw_res,
					value);
				if (ret) {
					DRM_ERROR("creat hwres_ops fail.\n");
					goto err_pravate;
				}

				ret = sunxi_lcd_panel_ops_init(sunxi_panel);
				if (ret) {
					DRM_ERROR("creat panel_ops fail.\n");
					goto err_ops;
				}
			}
		}
	}

	return sunxi_panel;

err_ops:
	sunxi_lcd_hwres_ops_destroy(hw_res);
err_pravate:
	sunxi_lcd_private_destroy(sunxi_panel->private);
err_panel:
	if (sunxi_panel)
		sunxi_panel_destroy(sunxi_panel);
err_false:
	return NULL;
}

void sunxi_lcd_destroy(struct sunxi_panel *sunxi_panel,
	struct sunxi_hardware_res *hw_res)
{
	if (sunxi_panel) {
		sunxi_lcd_hwres_ops_destroy(hw_res);
		sunxi_lcd_private_destroy(sunxi_panel->private);
	}
	return ;
}
