#include "ufdt_common.h"
#include "ufdt_overlay.h"
#include "ufdt_node_pool.h"
#include "dtbo_img.h"

#define DTBOIMG_BUF_SIZE	(1024 * 256)
#define DTO_PARTION			("dtbo")

extern int do_sunxi_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);
extern void sunxi_dump(void *addr, unsigned int size);
#if defined(CONFIG_SUNXI_MULITCORE_BOOT)
extern uint *p_spin_lock_heap;
#endif

int check_dtbo(void *dtboimg_buf)
{
    struct dt_table_header *dt_table_head = NULL;
    struct dt_table_entry  *dt_table_entry = NULL;
    u32 i = 0;
    u32 ret = 0;
    u32 offset = 0;

	dt_table_head = (struct dt_table_header *) dtboimg_buf;
#if 0
    dto_error("dtboimg magic is:0x%x\n", dt_table_header_magic(dtboimg_buf));
    dto_error("dtboimg total size is:0x%x\n", dt_table_header_total_size(dtboimg_buf));
    dto_error("dtboimg header size:0x%x\n", dt_table_header_header_size(dtboimg_buf));
    dto_error("dtboimg entry size:0x%x\n", dt_table_header_dt_entry_size(dtboimg_buf));
    dto_error("dtboimg entry count:0x%x\n", dt_table_header_dt_entry_count(dtboimg_buf));
    dto_error("dtboimg entry offset:0x%x\n", dt_table_header_dt_entries_offset(dtboimg_buf));
#endif
	if (dt_table_header_magic(dtboimg_buf) != DT_TABLE_MAGIC) {
		dto_error("dtboimg magic is bad:0x%x\n", dt_table_header_magic(dtboimg_buf));
		return -1;
	}
	offset = dt_table_header_dt_entries_offset(dtboimg_buf);
    for (i = 1; i <= dt_table_header_dt_entry_count(dtboimg_buf); i++) {
		dt_table_entry = (struct dt_table_entry *) ((char *)dt_table_head + offset);
#if 0
    dto_error("dt_table_entry_dt_size:0x%x\n", dt_table_entry_dt_size(dt_table_entry));
    dto_error("dt_table_entry_dt_offset:0x%x\n", dt_table_entry_dt_offset(dt_table_entry));
    dto_error("dt_table_entry_id:0x%x\n", dt_table_entry_id(dt_table_entry));
    dto_error("dt_table_entry_rev:0x%x\n", dt_table_entry_rev(dt_table_entry));
    dto_error("dt_table_entry_custom0:0x%x\n", dt_table_entry_custom(dt_table_entry, 0));
    dto_error("dt_table_entry_custom1:0x%x\n", dt_table_entry_custom(dt_table_entry, 1));
    dto_error("dt_table_entry_custom2:0x%x\n", dt_table_entry_custom(dt_table_entry, 2));
    dto_error("dt_table_entry_custom3:0x%x\n", dt_table_entry_custom(dt_table_entry, 3));
#endif
    ret =  dt_table_entry_dt_offset(dt_table_entry);
    return ret;

    }

    return -1;

}

int load_dtboimg(void *dtboimg_buf)
{
	char *argv[6];
	char  dtboimg_head[32];
	u32 dto_addr = (u32)dtboimg_buf;

	sprintf(dtboimg_head, "%x", (u32)dto_addr);
	argv[0] = "sunxi_flash";
	argv[1] = "read";
    argv[2] = dtboimg_head;
	argv[3] = DTO_PARTION;
	argv[4] = NULL;

	if (do_sunxi_flash(0, 0, 4, argv)) {
		dto_error("sunxi_flash read dto partion error:%s\n", argv[3]);
		return -1;
	}
    printf("sunxi_flash read dto partion success\n");

    return 0;
}

int sunxi_support_ufdt(void *dtb_base, u32 dtb_len)
{
	struct fdt_header *main_fdt_header = NULL;
    struct fdt_header *merge_fdt = NULL;
	void *dtboimg_buf;
	u32 main_fdt_size;
	u32 overlay_size;
    int ret;

	p_spin_lock_heap = NULL;
	main_fdt_size = dtb_len;
	main_fdt_header = ufdt_install_blob(dtb_base, main_fdt_size);
	if (main_fdt_header == NULL) {
		dto_error("get main_fdt_header fail\n");
		return -1;
	}
	dtboimg_buf = (void *) malloc(DTBOIMG_BUF_SIZE);
    if (dtboimg_buf == NULL) {
		dto_error("malloc dtboimg_buf fail\n");
		return -1;
    }
	ret = load_dtboimg(dtboimg_buf);
    if (ret < 0) {
		dto_error("load_dto fail\n");
		free(dtboimg_buf);
		return -1;
    }

	ret = check_dtbo(dtboimg_buf);
	if (ret < 0) {
		dto_error("don't have match dtbo\n");
		free(dtboimg_buf);
		return -1;
	}

	overlay_size = fdt_totalsize(dtboimg_buf + ret);
    dto_debug("overlay_size=0x%x\n", overlay_size);

	merge_fdt = ufdt_apply_overlay(main_fdt_header, main_fdt_size, (dtboimg_buf + ret), overlay_size);
	if (merge_fdt == NULL) {
		dto_error(" merge fdt fail\n");
		free(dtboimg_buf);
		return -2;
	}
    dto_debug("merge fdt success\n");
    dtb_len = fdt_totalsize(merge_fdt);
    memcpy((void *)dtb_base, (void *)merge_fdt, dtb_len);
	return 0;
}
