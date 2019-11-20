/*
 *  * Copyright 2000-2009
 *   * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *    *
 *     * SPDX-License-Identifier:	GPL-2.0+
 *     */
#ifndef SUNXI_STRING_H_H_
#define SUNXI_STRING_H_H_

int sunxi_str_replace(char *dest_buf, char *goal, char *replace);
int sunxi_str_replace_all(char *dest_buf, char *goal, char *replace);

#endif

