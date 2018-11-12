#include <linux/types.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kmsg_dump.h>

#define PANIC_LOG_SIZE (128 * 1024)

ssize_t kcrash_emmc_panic_write(char *buf, size_t numbytes);
static char panic_log[PANIC_LOG_SIZE];

static void kcrash_panic_handler(struct kmsg_dumper *dumper,
		enum kmsg_dump_reason reason)
{
	if (reason != KMSG_DUMP_PANIC)
		return;

	pr_err("[kcrash] catch panic event\n");

	if (false == kmsg_dump_get_buffer(dumper, true, panic_log,
				PANIC_LOG_SIZE, NULL)) {
		pr_err("[kcrash] get panic log failed\n");
		return;
	}

	if (kcrash_emmc_panic_write(panic_log, PANIC_LOG_SIZE) < 0)
		pr_err("[kcrash] record panic log failed\n");

	pr_info("[kcrash] record panic log success\n");
}

static struct kmsg_dumper kcrash_panic = {
	.dump = kcrash_panic_handler,
	.max_reason = KMSG_DUMP_PANIC,
};

static int __init kcrash_panic_init(void)
{
	pr_err("[kcrash] stare at panic event\n");
	return kmsg_dump_register(&kcrash_panic);
}

static void __exit kcrash_panic_exit(void)
{
	kmsg_dump_unregister(&kcrash_panic);
}

module_init(kcrash_panic_init);
module_exit(kcrash_panic_exit);
