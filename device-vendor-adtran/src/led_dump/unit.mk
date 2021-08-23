###############################################################################
#
#  LED STACK DUMP
#
###############################################################################

UNIT_NAME := led_stack

# Template type:
UNIT_TYPE := BIN

UNIT_SRC    += src/led_stack_main.c

UNIT_CFLAGS := -I$(UNIT_PATH)/inc
UNIT_CFLAGS += -Isrc/lib/common/inc/
UNIT_CFLAGS += -Isrc/lib/version/inc/

ifneq ($(BUILD_SHARED_LIB),y)
UNIT_LDFLAGS += -lpcap
endif

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)

UNIT_DEPS += src/lib/osp
UNIT_DEPS_CFLAGS += src/lib/version
