#**************************************************************************
#                Copyright C 1998-2009 Broadcom Corporation
#
#This material is the confidential trade secret and proprietary information
#of Broadcom Corporation.  It may not be reproduced, used, sold or
#transferred to any third party without the prior written consent of
#Broadcom Corporation.   All rights reserved.
#**************************************************************************
#  FileName        : Makefile
#  Purpose         : Top level makefile for DSL SDK integration
#  Limitations     : None
#  Creation Date   : 20-Jan-2009
#**************************************************************************

CURR_DIR := $(shell pwd)
DSL_DIR  := $(CURR_DIR)/build
BUILD_DIR:=$(word 1, $(subst /userspace, /userspace,$(CURR_DIR)))
default: conditional_build
include $(BUILD_DIR)/make.common

BUILD_BRCM_XDSL_DISTPOINT:=$(subst ",,$(BUILD_BRCM_XDSL_DISTPOINT)) # remove " to make it a list
ifneq ($(strip $(BUILD_BRCM_XDSL_DISTPOINT)),)
conditional_build: all
else
conditional_build:
	@echo ���xdsl_distpoint not selected��� skipping���
endif

export BCM_ENET_DRIVER_CB=1
export BCM_IOCTL_SPI_SIZE=4

all dynamic install: $(DSL_DIR)
	$(MAKE) -C $(DSL_DIR) all

ifeq (,$(wildcard $(DSL_DIR)))
# no src extracted yet => first thing to do
$(DSL_DIR): extract
else ifneq (,$(wildcard $(CURR_DIR)/dsl-release/*_delivery_cs*.tgz))
$(DSL_DIR): $(CURR_DIR)/dsl-release/*_delivery_cs*.tgz
	@echo 'A DSL DSK release, more recent compared to last build is available.'
	@echo 'In case you want to use it do a [make extract] in [$(CURR_DIR)].'
	@read -p "Press ENTER to continue and ignore the new release, or CTRL-C to abort. "
endif

extract:
	@./extract_dsl $(DSL_DIR)

clean:; $(MAKE) -C $(DSL_DIR) clean
distclean:; $(RM) -rf $(DSL_DIR)

.PHONY: extract clean distclean
