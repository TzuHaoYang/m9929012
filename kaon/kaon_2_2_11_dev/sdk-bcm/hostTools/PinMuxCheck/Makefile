
vpath %.c ../../shared/opensource/boardparms/bcm963xx
vpath %.h ../../shared/opensource/boardparms/bcm963xx

default:run_bptest

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /hostTools, /hostTools,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))


include $(BUILD_DIR)/make.common
unexport AR AS LD CC CPP CXX NM STRIP SSTRIP OBJCOPY OBJDUMP RANLIB
CFLAGS = $(DEFS) $(WARNINGS) -O2 -DGNU $(INCLUDEDIR)
CC=gcc



CFLAGS +=  -I ../../shared/opensource/include/bcm963xx/  -I ../../bcmdrivers/opensource/include/bcm963xx/ -DCONFIG_BCM9$(BRCM_CHIP) -D_BCM9$(BRCM_CHIP)_ -DPINMUXCHECK -DCONFIG_BOARDPARMS_VOICE -D_CFE_ -DPRINT_ERRORS


ifneq ($(findstring _$(strip $(BP_PHYS_INTF))_,_y_),)
CFLAGS += -DCONFIG_BP_PHYS_INTF
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_6848_63148_63138_63381_63268_47189_6838_),)
CFLAGS += -DCONFIG_CHECK_EXT_INT_GPIO
endif

run_bptest: bptest
	./bptest

bptest: bptest.c boardparms_chip.c boardparms_voice.c bp_funcs.c pinmux_table_$(BRCM_CHIP).c boardparms_$(BRCM_CHIP).c

clean:
	rm -f *.o bptest

