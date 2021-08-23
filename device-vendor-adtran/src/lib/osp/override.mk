##############################################################################
#
# OSP layer library
#
##############################################################################

UNIT_CFLAGS := $(filter-out -DTARGET_H=%,$(UNIT_CFLAGS))
ifeq ($(TARGET),814-v6)
UNIT_SRC_TOP += $(if $(CONFIG_PM_ENABLE_LED),$(OVERRIDE_DIR)/src/osp_led.c,)
UNIT_SRC_TOP += $(if $(CONFIG_PM_ENABLE_TM),$(OVERRIDE_DIR)/src/osp_tm.c,)

UNIT_LDFLAGS := -li2cledctl
endif

ifneq ($(TARGET),814-v6)
UNIT_CFLAGS += -I$(OVERRIDE_DIR)/inc
UNIT_SRC_TOP += $(if $(CONFIG_PM_ENABLE_LED),$(OVERRIDE_DIR)/src/osp_ledlib.c,)
UNIT_LDFLAGS := -lcurl
endif

UNIT_EXPORT_CFLAGS  := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)
