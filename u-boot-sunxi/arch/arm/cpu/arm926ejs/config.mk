#
# (C) Copyright 2002
# Gary Jennejohn, DENX Software Engineering, <garyj@denx.de>
#
# SPDX-License-Identifier:	GPL-2.0+
#

ifneq  ("$(TOPDIR)", "")
    #check gcc tools chain
    tooldir_armv5=$(TOPDIR)/../armv5_toolchain
    toolchain_archive_armv5=$(TOPDIR)/../toolchain/arm-none-linux-gnueabi-toolchain.tar.xz
    armv5_toolchain_check=$(strip $(shell if [  -d $(tooldir_armv5) ];  then  echo yes;  fi))

    ifneq ("$(armv5_toolchain_check)", "yes")
        $(info "gcc tools chain for armv5 not exist")
        $(info "Prepare toolchain...")
        $(shell mkdir -p $(tooldir_armv5))
        $(shell tar --strip-components=1 -xf $(toolchain_archive_armv5) -C $(tooldir_armv5))
    endif

    CROSS_COMPILE :=  $(TOPDIR)/../armv5_toolchain/arm-none-linux-gnueabi-toolchain/arm-2014.05/bin/arm-none-linux-gnueabi-
endif

PF_CPPFLAGS_ARM926EJS := $(call cc-option, -march=armv5te)
PLATFORM_CPPFLAGS += $(PF_CPPFLAGS_ARM926EJS) -Werror

# On supported platforms we set the bit which causes us to trap on unaligned
# memory access.  This is the opposite of what the compiler expects to be
# the default so we must pass in -mno-unaligned-access so that it is aware
# of our decision.
PF_NO_UNALIGNED := $(call cc-option, -mno-unaligned-access,)
PLATFORM_CPPFLAGS += $(PF_NO_UNALIGNED)
