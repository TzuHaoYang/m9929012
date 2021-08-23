###############################################################################
#
# DM overrides for vendor/plume
#
###############################################################################

UNIT_SRC := $(filter-out src/dm_hook.c,$(UNIT_SRC))

ifeq ($(TARGET),814-v6)
UNIT_CFLAGS += -D814V6_COMPILE_FLAG
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_test.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_config.c
endif

ifneq ($(TARGET),814v6)
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_reboot_trigger.c
endif

ifeq ($(CONFIG_SPEEDTEST_OOKLA),y)
UNIT_SRC := $(filter-out src/dm_st.c,$(UNIT_SRC))
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_st.c
UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_st_iperf.c
UNIT_DEPS += src/lib/pasync
endif

UNIT_SRC_TOP += $(OVERRIDE_DIR)/src/dm_hook.c
