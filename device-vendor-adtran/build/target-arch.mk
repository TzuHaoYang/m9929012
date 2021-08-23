##
# TARGET definitions
#
# NOTE: A single vendor repository may contain multiple targets,
# which share some portions of code.
#
ADTRAN_TARGETS := 814-v6 SR400ac 834-5
ADTRAN_DEPLOYMENT_PROFILES := opensync-dev opensync-dev-debug adtran-prod

OS_TARGETS     += $(ADTRAN_TARGETS)

##
# ADTRAN targets
#
ifneq ($(filter $(TARGET),$(ADTRAN_TARGETS)),)

ifeq ($(TARGET),814-v6)
# Vendor specific build defaults
# specifies whether curl utilities are built or not
BUILD_COMMON_CURL              ?= y
# specifies whether upgrade is built or not
BUILD_UPGRADE                  ?= n
# specifies whether Speed test is built or not (st_ookla)
BUILD_SPEED_TEST                = y
# default to version 2 of main managers
BUILD_USE_NM2                   = y
# obfuscate the default OVSDB file
BUILD_OBFUSCATE_OVSDB           = y
endif 

VENDOR              := adtran
VENDOR_DIR          := vendor/$(VENDOR)

ARCH_MK             := $(VENDOR_DIR)/build/$(TARGET).mk
KCONFIG_TARGET      ?= $(VENDOR_DIR)/kconfig/targets/$(TARGET)

ifeq ($(TARGET),814-v6)

PLATFORM            := bcm
PLATFORM_DIR        := platform/$(PLATFORM)

ARCH                = bcm52
CPU_TYPE            = arm
ARCH_MK             = $(VENDOR_DIR)/build/$(ARCH)-$(CPU_TYPE).mk

include $(VENDOR_DIR)/build/$(TARGET).mk

$(eval $(if $(CONFIG_WEBROOT), INCLUDE_LAYERS += 3rdparty/webroot))
$(eval $(if $(CONFIG_3RDPARTY_WALLEYE), INCLUDE_LAYERS += 3rdparty/walleye))

# service providers for AdTran
export SERVICE_PROVIDERS ?= opensync-dev adtran
export IMAGE_DEPLOYMENT_PROFILE ?= adtran-prod
endif 

ifeq ($(TARGET),SR400ac)
PLATFORM                 := cfg80211
PLATFORM_DIR             := platform/$(PLATFORM)
SERVICE_PROVIDERS        ?= opensync-dev
IMAGE_DEPLOYMENT_PROFILE ?= opensync-dev
KCONFIG_TARGET           ?= $(VENDOR_DIR)/kconfig/targets/$(TARGET)
ARCH_MK                   = $(PLATFORM_DIR)/build/$(PLATFORM).mk

$(eval $(if $(CONFIG_WEBROOT), INCLUDE_LAYERS += 3rdparty/webroot))
endif

ifeq ($(TARGET),834-5)
PLATFORM                 := cfg80211
PLATFORM_DIR             := platform/$(PLATFORM)
SERVICE_PROVIDERS        ?= opensync-dev
IMAGE_DEPLOYMENT_PROFILE ?= opensync-dev
KCONFIG_TARGET           ?= $(VENDOR_DIR)/kconfig/targets/$(TARGET)
ARCH_MK                   = $(PLATFORM_DIR)/build/$(PLATFORM).mk

$(eval $(if $(CONFIG_WEBROOT), INCLUDE_LAYERS += 3rdparty/webroot))
endif


ifeq ($(V),1)
$(info --- ADTRAN ENV ---)
$(info PLATFORM=$(PLATFORM))
$(info VENDOR=$(VENDOR))
$(info TARGET=$(TARGET))
$(info BUILD_DIR=$(BUILD_DIR))
$(info ARCH_MK=$(ARCH_MK))
$(info CMS_COMMON_LIBS=$(CMS_COMMON_LIBS))
$(info CMS_CORE_LIBS=$(CMS_CORE_LIBS))
$(info CMS_COMPILE_FLAGS=$(CMS_COMPILE_FLAGS))
$(info SERVICE_PROVIDERS=$(SERVICE_PROVIDERS))
$(info IMAGE_DEPLOYMENT_PROFILE=$(IMAGE_DEPLOYMENT_PROFILE))
$(info ------------------)
endif

endif # ADTRAN_TARGETS
