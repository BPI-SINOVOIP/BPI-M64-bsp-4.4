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
export LICHEE_KDIR := $(shell pwd)
endif

.PHONY: install build clean

install:
	@if cat $(LICHEE_KDIR)/.config|grep "CONFIG_SUNXI_NAND=m" >/dev/null 2>&1;then \
		cp $(SOC)/nand.ko $(LICHEE_MOD_DIR)/; \
	else \
		echo "[NAND] NOT buid as ko"; \
	fi

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~

build:
	@make -C ${SOC}/lib clean 2>/dev/null
	@make -C ${SOC}/lib install
