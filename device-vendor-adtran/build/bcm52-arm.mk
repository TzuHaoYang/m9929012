
ifneq (,$(wildcard $(PROFILE_DIR)/../../make.wlquilt))
DRIVER_VERSION                  := impl999
DRIVER_VERSION_REAL             := impl$(shell grep BCM_WLIMPL= $(PROFILE_DIR)/$(shell basename $(PROFILE_DIR)) | cut -d= -f2)
else ifneq ($(PROFILE_DIR),)
DRIVER_VERSION                  := impl$(shell grep BCM_WLIMPL= $(PROFILE_DIR)/$(shell basename $(PROFILE_DIR)) | cut -d= -f2)
else
DRIVER_VERSION                  := impl0
endif

SDK                             = bcm52
RCD_TARGET_DIR                  = rc3.d
DEFAULT_SHELL                   = /bin/sh
export RCD_TARGET_DIR
export DEFAULT_SHELL


include platform/bcm/build/$(SDK).mk
