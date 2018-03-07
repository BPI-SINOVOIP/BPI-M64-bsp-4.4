################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################

ifneq ($(LICHEE_KDIR),)
SOC=$(shell \
	sed -n 's/^CONFIG_ARCH_\(SUN.*P1\)=y/\1/p' $(LICHEE_KDIR)/.config | sed 's/[A-Z]/\l&/g')
obj-m += $(SOC)/
else
SOC=$(shell \
		sed -n 's/^CONFIG_ARCH_\(SUN.*P1\)=y/\1/p' .config | sed 's/[A-Z]/\l&/g')
obj-$(CONFIG_SUNXI_NAND) += $(SOC)/
endif

.PHONY: install build clean

install: build
	@cp $(SOC)/nand.ko $(LICHEE_MOD_DIR)/

build:
	$(MAKE) -C $(LICHEE_KDIR) M=$(shell pwd)

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~

build:
	@make -C ${SOC}/lib clean 2>/dev/null
	@make -C ${SOC}/lib install
