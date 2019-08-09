/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 */

#define   PHY_ONLY_TOG_AND_SDR              1
#define   PHY_WAIT_RB_BEFORE                0
#define   PHY_WAIT_RB_INTERRRUPT            0
#define   PHY_WAIT_DMA_INTERRRUPT           0

/*****************************************************************************
1.Single channel needs to be affixed to the same kind of flash
2.Dual channel needs to be affixed to  the same number and type of flash

Single channel
1.support two-plane
2.support vertical_interleave
3.if superpage>32k，two-plane not supported
4.vertical_interleave chip pairing with different rb in the channel

Dual channel
1.support two-plane
2.support dual_channel
3.support vertical_interleave
4.if superpage>32k，two-plane not supported
5.dual_channel chip pairing with same chip number between the channel
6.vertical_interleave chip pairing with different rb in the channel
*****************************************************************************/
#define   PHY_SUPPORT_TWO_PLANE                          1
#define   PHY_SUPPORT_VERTICAL_INTERLEAVE                1
#define   PHY_SUPPORT_DUAL_CHANNEL                       1


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_cfg_interface(void)
{
	return PHY_ONLY_TOG_AND_SDR ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_wait_rb_before(void)
{
	return PHY_WAIT_RB_BEFORE ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_wait_rb_mode(void)
{
	return PHY_WAIT_RB_INTERRRUPT ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_wait_dma_mode(void)
{
	return PHY_WAIT_DMA_INTERRRUPT ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_support_two_plane(void)
{
	return PHY_SUPPORT_TWO_PLANE ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_support_vertical_interleave(void)
{
	return PHY_SUPPORT_VERTICAL_INTERLEAVE ? 1 : 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_support_dual_channel(void)
{
	return PHY_SUPPORT_DUAL_CHANNEL ? 1 : 0;
}
