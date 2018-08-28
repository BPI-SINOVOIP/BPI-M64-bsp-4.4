#include "ufdt_common.h"
#include "ufdt_overlay.h"
#include "ufdt_node_pool.h"

#define OVERLAY_BUF		(1024 * 256)
#define DTO_PARTION     ("odm")

extern int do_sunxi_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);

int load_dto(void *dto_buf)
{
	char *argv[6];
	char  dto_head[32];
	u32 dto_addr = (u32)dto_buf;

	sprintf(dto_head, "%x", (u32)dto_addr);
	argv[0] = "sunxi_flash";
	argv[1] = "read";
    argv[2] = dto_head;
	argv[3] = DTO_PARTION;
	argv[4] = NULL;

	if (do_sunxi_flash(0, 0, 4, argv)) {
		dto_error("sunxi_flash read dto partion error:%s\n", argv[3]);
		return -1;
	}
    printf("sunxi_flash read dto partion success\n");

    return 0;
}

int load_main_dt(void *dto_buf)
{
    char *argv[6];
    char  dto_head[32];
    u32 dto_addr = (u32)dto_buf;

    sprintf(dto_head, "%x", (u32)dto_addr);
    argv[0] = "sunxi_flash";
    argv[1] = "read";
    argv[2] = dto_head;
    argv[3] = "main_dt";
    argv[4] = NULL;

    if (do_sunxi_flash(0, 0, 4, argv)) {
		dto_error("sunxi_flash read dto partion error:%s\n", argv[3]);
		return -1;
	}
	printf("sunxi_flash read dto partion success\n");
	return 0;


}

int sunxi_dto_merge_test(void)
{
    struct fdt_header *main_fdt_header = NULL;
    struct fdt_header *merge_fdt = NULL;
    void *overlay_buf = NULL;
    void *main_buf = NULL;
    int ret = 0;
    u32 main_fdt_size = 0;
    u32 overlay_size = 0;

    main_buf = (void *) malloc(OVERLAY_BUF);
    if (main_buf == NULL) {
		dto_error("malloc main_buf fairl\n");
		return -1;
    }

    ret = load_main_dt(main_buf);
    if (ret < 0) {
		free(overlay_buf);
		return -1;
    }
    main_fdt_size = fdt_totalsize(main_buf);
    printf("main_fdt_size=%d\n", main_fdt_size);
    main_fdt_header = ufdt_install_blob(main_buf, main_fdt_size);
	if (main_fdt_header == NULL) {
		dto_error("get main_fdt_header fail\n");
		return -1;
	}

	overlay_buf = (void *) malloc(OVERLAY_BUF);
    if (overlay_buf == NULL) {
		dto_error("malloc overlay_buf fairl\n");
		return -1;
    }
    ret = load_dto(overlay_buf);
    if (ret < 0) {
		free(overlay_buf);
		return -1;
    }

    overlay_size = fdt_totalsize(overlay_buf);
    printf("overlay_size=%d\n", overlay_size);

    merge_fdt = ufdt_apply_overlay(main_fdt_header, main_fdt_size, overlay_buf, overlay_size);
	if (merge_fdt == NULL) {
		dto_error(" merge fdt fail\n");
		free(overlay_buf);
		return -2;
	}
    printf("merge fdt success\n");

}

int sunxi_support_ufdt(void *dtb_base, u32 *dtb_len)
{
	struct fdt_header *main_fdt_header = NULL;
    struct fdt_header *merge_fdt = NULL;
	void *overlay_buf;
	u32 main_fdt_size;
	u32 overlay_size;
    int ret;

	main_fdt_size = *dtb_len;
	main_fdt_header = ufdt_install_blob(dtb_base, main_fdt_size);
	if (main_fdt_header == NULL) {
		dto_error("get main_fdt_header fail\n");
		return -1;
	}
    dto_debug("main_fdt_size=%d\n", main_fdt_size);
	overlay_buf = (void *) malloc(OVERLAY_BUF);
    if (overlay_buf == NULL) {
		dto_error("malloc overlay_buf fairl\n");
		return -1;
    }
	ret = load_dto(overlay_buf);
    if (ret < 0) {
		free(overlay_buf);
		return -1;
    }
	overlay_size = fdt_totalsize(overlay_buf);
    dto_debug("overlay_size=%d\n", overlay_size);

	merge_fdt = ufdt_apply_overlay(main_fdt_header, main_fdt_size, overlay_buf, overlay_size);
	if (merge_fdt == NULL) {
		dto_error(" merge fdt fail\n");
		free(overlay_buf);
		return -2;
	}
    dto_debug("merge fdt success\n");
    *dtb_len = fdt_totalsize(merge_fdt);
    memcpy((void *)dtb_base, (void *)merge_fdt, *dtb_len);
	return 0;
}
