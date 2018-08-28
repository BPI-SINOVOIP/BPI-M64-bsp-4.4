#ifdef CUSTOMER_HW_ALLWINNER

/*#include <osl.h>
*/
#include <linux/gpio.h>

#ifdef CUSTOMER_HW_PLATFORM
#include <plat/sdhci.h>
#define	sdmmc_channel	sdmmc_device_mmc0
#endif /* CUSTOMER_HW_PLATFORM */

#if defined(BUS_POWER_RESTORE)
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#endif /* defined(BUS_POWER_RESTORE) */

#define POWERUP_MAX_RETRY 3

#if defined(HIF_SDIO)
#include "if_ath_sdio.h"
#endif

#include "hdd_wlan_aw_platform.h"

extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(int on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);

int hdd_wlan_set_carddetect(bool present)
{
	int wlan_bus_index = 0;

	wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0)
		return wlan_bus_index;

	if (present) {
		printk("===========  card detection to detect SDIO card! ===========\n");
		sunxi_mmc_rescan_card(wlan_bus_index);
	} else {
		printk("===========  card detection to remove SDIO card! ===========\n");
		sunxi_mmc_rescan_card(wlan_bus_index);
	}

	return 0;
}

int hdd_wlan_set_power(bool on_off)
{
	if (on_off) {
		printk("===========  To power on WIFI module! ===========\n");
		sunxi_wlan_set_power(1);
	} else {
		printk("===========  To power off WIFI module! ===========\n");
		sunxi_wlan_set_power(0);
	}

	return 0;
}

int hdd_wlan_aw_flatform_on(void)
{
	bool chip_up = FALSE;
	int retry = POWERUP_MAX_RETRY;
	int err = 0;

	do {
		err = wlan_sdio_func_reg_notify();
		if (err) {
			printk("%s wlan_sdio_reg__notify fail(%d)\n\n",
				__FUNCTION__, err);
			return err;
		}
		err = hdd_wlan_set_power(1);
		if (err) {
			/* WL_REG_ON state unknown, Power off forcely */
			hdd_wlan_set_power(0);
			continue;
		} else {
			hdd_wlan_set_carddetect(1);
			err = 0;
		}

		if ((wlan_sdio_wait_card()) == 0) {
			printk("%s Carddetect finished!\n\n", __FUNCTION__);
			wlan_sdio_func_unreg_notify();
			chip_up = TRUE;
			break;
		}

		printk("failed to power up, %d retry left\n", retry);
		wlan_sdio_func_unreg_notify();
		hdd_wlan_set_power(0);
		hdd_wlan_set_carddetect(0);
	} while (retry--);

	if (!chip_up) {
		printk(("failed to power up, max retry reached**\n"));
		return -ENODEV;
	}

	return err;
}
#endif
