# (C) Copyright 2000-2008
#  Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2013
#  Bo Shen <voice.shen@atmel.com>
#

include $(SPLDIR)/config.mk
LIB	=  $(obj)libsource_spl.o

COBJS	+= rtc_region.o
COBJS	+= timer_spl.o
COBJS	+= gpio_spl.o
COBJS	+= serial_spl.o
COBJS	+= clock_spl.o
COBJS	+= efuse_spl.o
COBJS	+= fip_common.o
COBJS 	+= jmp.o

#COBJS-$(CONFIG_XXXX)	+= xxxx.o

SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))



all:	$(obj).depend $(LIB)

$(LIB):	$(OBJS)
	$(call cmd_link_o_target, $(OBJS))

#########################################################################

depend:	$(obj).depend
.PHONY: depend

# defines $(obj).depend target
include $(SRCTREE)/rules.mk
