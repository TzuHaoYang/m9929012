# Toploevel makefile to build HND router applications
#
#
#
#

default : all

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export BUILD_DIR
export ALLSRCBASE= $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/bcm9$(BRCM_CHIP)
export HNDAPPS_DIR = $(ALLSRCBASE)/$(HNDAPPS_BASE)
export WLCFGDIR = $(ALLSRCBASE)/main/src/wl/config
export WLMAIN = main/src
export BUILD_MCAST_PROXY
export BUILD_BSTREAM_IQOS
export BUILD_WLAN_TESTBED_AP

ifneq ($(strip $(BCA_CPEROUTER)),)
-include $(HNDAPPS_DIR)/.config
-include $(HNDAPPS_DIR)/.config.plt
ifeq ($(CONFIG_UCLIBC),y)
PLATFORM_INSTALL := $(HNDAPPS_DIR)/$(PLT)-uclibc/install
else ifeq ($(CONFIG_GLIBC),y)
PLATFORM_INSTALL := $(HNDAPPS_DIR)/$(PLT)-glibc/install
else
PLATFORM_INSTALL := $(HNDAPPS_DIR)/$(PLT)/install
endif
else
export PLT = arm
export LINUX_VERSION = 4_1_0
endif
export LINUXDIR = $(KERNEL_DIR)
SUBMAKE_SETTINGS = SRCBASE=$(SRCBASE) BASEDIR=$(BASEDIR)

USR_CFLAGS = -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
USR_LDFLAGS = -L$(INSTALL_DIR)/lib -lnvram -lbcm_flashutil -lbcm_boardctl -lbcm_util -lsys_util -lgen_util -L$(TOOLCHAIN)/../lib

# DSLCPE
ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
ALLOWED_LIB_DIRS := /lib:/lib/public:/lib/private
LIB_RPATH = $(INSTALL_DIR)$(subst :,:$(INSTALL_DIR),$(ALLOWED_LIB_DIRS))
USR_LDFLAGS += -Wl,-rpath-link=$(LIB_RPATH)
export BUILD_BRCM_UNFWLCFG
endif
# end DSLCPE


ifneq ($(strip $(BUILD_MCAST_PROXY)),)
ifneq ($(strip $(BUILD_IPV6)),)
USR_CFLAGS += -DBCM_NBUFF_WLMCAST_IPV6
endif
endif

USR_CFLAGS += -DCHIP_$(BRCM_CHIP)

ifeq ($(strip $(BRCM_USER_SSP)),y)
USR_CFLAGS  += $(SSP_MIN_COMPILER_OPTS)
USR_LDFLAGS += $(SSP_LIBS)
endif
ifneq ($(strip $(BCA_CPEROUTER)),)
export USE_EXTERNAL_HTTPD=y
USR_CFLAGS += -DBCA_CPEROUTER  -DNO_NVRAM_GUI -DUSE_EXTERNAL_HTTPD -I$(BUILD_DIR)/userspace/private/libs/wlcsm/include
ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
USR_CFLAGS += -DBCA_SUPPORT_UNFWLCFG
endif
ifeq ($(strip $(BRCM_IKOS)),y)
# for IKOS APP, only need to have wl and/or dhd command,enable dhd if needed. 
IKOS_WLAPPS := wlexe
#IKOS_WLAPPS += dhd
IKOS_WLAPPS_INSTALL:= $(foreach app, $(IKOS_WLAPPS),$(app)-install)
IKOS_WLAPPS_CLEAN:= $(foreach app, $(IKOS_WLAPPS),$(app)-clean)
endif
endif

export USR_CFLAGS
export USR_LDFLAGS
ifneq ($(strip $(BCA_HNDROUTER)),)
export BRCM_KERNEL_BONDING
endif

.PHONY: all oldconfig clean
all: version
ifneq ($(strip $(BRCM_IKOS)),y)
	$(MAKE) -C $(HNDAPPS_DIR) all
	$(MAKE) -C $(HNDAPPS_DIR) install
else
	$(MAKE) -C $(HNDAPPS_DIR) $(IKOS_WLAPPS) $(IKOS_WLAPPS_INSTALL)
	for dir in $(wildcard $(patsubst %,$(PLATFORM_INSTALL)/%,$(IKOS_WLAPPS))) ; do \
		    (cd $${dir} && tar cpf - .) | (cd $(INSTALL_DIR) && tar xpf -) \
	done
endif

version: oldconfig
	$(MAKE) -C $(HNDAPPS_DIR) version

oldconfig:
	$(MAKE) -C $(HNDAPPS_DIR) oldconfig

ifneq ($(strip $(BCA_HNDROUTER)),)
clean:
ifneq ($(strip $(BRCM_IKOS)),y)
	if [ -d $(HNDAPPS_DIR) ]; then \
	$(MAKE) -C $(HNDAPPS_DIR) clean; \
	fi
else
	$(MAKE) -C $(HNDAPPS_DIR) $(IKOS_WLAPPS_CLEAN)
endif
else
clean:
	@true
endif

