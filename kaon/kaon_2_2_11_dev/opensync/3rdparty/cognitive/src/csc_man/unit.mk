##############################################################################
#
# CSC Manager service
#
##############################################################################
UNIT_DISABLE := $(if $(CONFIG_MANAGER_CMM),n,y)
UNIT_NAME := csc_man

# Template type:
UNIT_TYPE := BIN

UNIT_SRC := src/csc_man_main.c
UNIT_SRC += src/csc_man_ovsdb.c
UNIT_SRC += src/csc_man_motion.c
UNIT_SRC += src/csc_man_cert.c

UNIT_CFLAGS := -I$(UNIT_PATH)/inc
UNIT_CFLAGS += -I$(TOP_DIR)/src/lib/common/inc/
#UNIT_CFLAGS += -DCSC_MAN_AUTOSTART_MOTION

ifdef CONFIG_CSC_LIBUCI
UNIT_LDFLAGS := -lev -luci
else
UNIT_LDFLAGS := -lev
endif

UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)

UNIT_DEPS += src/lib/common
UNIT_DEPS += src/lib/ovsdb
UNIT_DEPS += src/lib/pjs
UNIT_DEPS += src/lib/schema
UNIT_DEPS += src/lib/version
UNIT_DEPS += src/lib/datapipeline
UNIT_DEPS += src/lib/json_util
UNIT_DEPS += src/lib/schema
UNIT_DEPS += src/lib/kconfig
