#include <linux/types.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/export.h>

#define SECTOR_SIZE 512
#define MMC_BASE_ADDR (20 * 1024 * 1024 / SECTOR_SIZE)

int sunxi_mmc_panic_init(void);
void sunxi_mmc_panic_exit(void);
int sunxi_mmc_panic_write(u32 sec_addr, u32 sec_cnt, const char *inbuf);
int sunxi_mmc_panic_read(u32 sec_addr, u32 sec_cnt, char *outbuf);

static u32 kcrash_addr;
static u32 kcrash_size;
static char extra[SECTOR_SIZE] = {0};

/**
 * kcrash_emmc_panic_write - write by flash panic apis
 * @buf: buffer that saving the data
 * @numbytes: maximum size of the buffer
 *
 * This api will not check the boundary.
 */
ssize_t kcrash_emmc_panic_write(char *buf, size_t numbytes)
{
	ssize_t wr_len = 0;
	u32 sec_cnt = numbytes / SECTOR_SIZE;

	if (sunxi_mmc_panic_write(kcrash_addr, sec_cnt, buf)) {
		pr_err("[kcrash] write to emmc failed\n");
		return -EIO;
	}
	wr_len += sec_cnt * SECTOR_SIZE;

	/* read and fill extra */
	if (numbytes % SECTOR_SIZE) {
		memset(extra, 0, SECTOR_SIZE);
		if (sunxi_mmc_panic_read(kcrash_addr + sec_cnt, 1, extra)) {
			pr_err("[kcrash] read from emmc failed\n");
			goto out;
		}
		memcpy(extra, buf + wr_len, numbytes % SECTOR_SIZE);
		if (sunxi_mmc_panic_write(kcrash_addr + sec_cnt, 1, extra)) {
			pr_err("[kcrash] write to emmc failed\n");
			goto out;
		}
		wr_len += SECTOR_SIZE;
	}

out:
	return wr_len;
}
EXPORT_SYMBOL(kcrash_emmc_panic_write);

/**
 * kcrash_emmc_panic_read - read by flash panic apis
 * @buf: buffer to copy the data
 * @numbytes: maximum size of the buffer
 *
 * This api will not check the boundary.
 */
ssize_t kcrash_emmc_panic_read(char *buf, size_t numbytes)
{
	ssize_t rd_len = 0;
	u32 sec_cnt = numbytes / SECTOR_SIZE;

	if (sunxi_mmc_panic_read(kcrash_addr, sec_cnt, buf)) {
		pr_err("[kcrash] read from emmc failed\n");
		return -EIO;
	}
	rd_len += sec_cnt * SECTOR_SIZE;

	/* read and fill extra */
	if (numbytes % SECTOR_SIZE) {
		memset(extra, 0, SECTOR_SIZE);
		if (sunxi_mmc_panic_read(kcrash_addr + sec_cnt, 1, extra)) {
			pr_err("[kcrash] read from emmc failed\n");
			goto out;
		}
		memcpy(buf + rd_len, extra, numbytes % SECTOR_SIZE);
		rd_len += numbytes % SECTOR_SIZE;
	}

out:
	return rd_len;
}
EXPORT_SYMBOL(kcrash_emmc_panic_read);

static int __init kcrash_emmc_init(void)
{
	struct device_node *kcrash_node;
	u32 offset, size;
	int ret = -1;

	kcrash_node = of_find_node_by_name(NULL, "kcrash");
	if (!kcrash_node) {
		pr_info("No kcrash node in dts, kcrash not work\n");
		goto out;
	}

	/* offset unit is sector */
	ret = of_property_read_u32(kcrash_node, "offset", &offset);
	if (ret) {
		pr_info("No offset in kcrash node, kcrash not work\n");
		goto out;
	}

	/* size unit is byte */
	ret = of_property_read_u32(kcrash_node, "size", &size);
	if (ret) {
		pr_info("No size in kcrash node, kcrash not work\n");
		goto out;
	}

	ret = sunxi_mmc_panic_init();
	if (ret) {
		pr_info("Emmc panic apis init failed, kcrash not work\n");
		goto out;
	}

	kcrash_addr = offset + MMC_BASE_ADDR;
	kcrash_size = size;
out:
	return ret;
}

static void __exit kcrash_emmc_exit(void)
{
	sunxi_mmc_panic_exit();
}

module_init(kcrash_emmc_init);
module_exit(kcrash_emmc_exit);
