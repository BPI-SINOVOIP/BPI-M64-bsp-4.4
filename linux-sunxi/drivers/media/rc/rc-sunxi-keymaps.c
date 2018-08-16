/* Sunxi Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2014 by allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include "sunxi-ir-rx.h"

static struct rc_map_table sunxi_nec_scan[] = {
	{ 0x17c612, KEY_POWER },
	{ 0x17c601, KEY_UP },
	{ 0x17c619, KEY_LEFT },
	{ 0x17c611, KEY_RIGHT },
	{ 0x17c609, KEY_DOWN },
	{ 0x17c640, KEY_ENTER },
	{ 0x17c60f, KEY_HOME },
	{ 0x17c60d, KEY_MENU },
	{ 0x17c61c, KEY_VOLUMEUP },
	{ 0x17c67f, KEY_VOLUMEDOWN },
};

static struct rc_map_list sunxi_map = {
	.map = {
		.scan    = sunxi_nec_scan,
		.size    = ARRAY_SIZE(sunxi_nec_scan),
		.rc_type = RC_TYPE_NEC,	/* Legacy IR type */
		.name    = RC_MAP_SUNXI,
	}
};

int init_sunxi_ir_map(void)
{
	return rc_map_register(&sunxi_map);
}

void exit_sunxi_ir_map(void)
{
	rc_map_unregister(&sunxi_map);
}
