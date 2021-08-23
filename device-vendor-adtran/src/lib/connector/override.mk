##############################################################################
#
# Adtran unit override for connector
#
##############################################################################
UNIT_CFLAGS := $(filter-out -DTARGET_H=%,$(UNIT_CFLAGS))
ifeq ($(TARGET),814-v6)
UNIT_CFLAGS += -DTARGET_H=\"target_$(TARGET).h\"
UNIT_CFLAGS += -I$(OVERRIDE_DIR)/inc

#ifneq ($(filter 814-v6,$(TARGET)),)
UNIT_SRC_TOP := $(OVERRIDE_DIR)/src/connector_main.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/connector_lan.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/connector_wan.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/connector_wifi.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/connector_awlan.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/connector_ntp.c

UNIT_DEPS += platform/bcm/src/lib/bcmcms
UNIT_DEPS += src/lib/kconfig
UNIT_DEPS += src/lib/log
UNIT_DEPS += src/lib/schema
UNIT_DEPS += src/lib/common

UNIT_DEPS_CFLAGS += src/lib/target
endif
#endif //814-v6

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
