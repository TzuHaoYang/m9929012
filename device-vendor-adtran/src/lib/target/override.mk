##############################################################################
#
# TARGET specific layer library
#
##############################################################################
UNIT_CFLAGS := $(filter-out -DTARGET_H=%,$(UNIT_CFLAGS))
UNIT_CFLAGS += -DTARGET_H=\"target_$(TARGET).h\"
UNIT_CFLAGS += -I$(OVERRIDE_DIR)/inc

#ifneq ($(filter 814-v6,$(TARGET)),)

ifeq ($(TARGET),814-v6)
##
# Platform BCM dependencies
#
UNIT_DEPS += platform/bcm/src/lib/bcmwl
UNIT_DEPS += platform/bcm/src/lib/bcm_bsal
UNIT_DEPS += platform/bcm/src/lib/bcmcms

##
endif

ifeq ($(TARGET),834-5)
ifdef CONFIG_MANAGER_BLEM
    UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/target_common_ble.c
    UNIT_LDFLAGS += -lbluetooth
endif
endif
# Target layer sources
#
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/target_$(TARGET).c

#endif //814-v6

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)
