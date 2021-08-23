###############################################################################
#
# BLE manager
#
###############################################################################
UNIT_DISABLE := $(if $(CONFIG_MANAGER_BLEM),n,y)

UNIT_NAME := blem

# Template type:
UNIT_TYPE := BIN

UNIT_SRC    := src/blem_main.c
UNIT_SRC    += src/blem_ovsdb.c

# UNIT_CFLAGS += -I$(TOP_DIR)/src/lib/common/inc/

# UNIT_LDFLAGS := -lpthread
# UNIT_LDFLAGS += -ljansson
# UNIT_LDFLAGS += -ldl
# UNIT_LDFLAGS += -lev
# UNIT_LDFLAGS += -lrt

# UNIT_EXPORT_CFLAGS := $(UNIT_CFLAGS)
# UNIT_EXPORT_LDFLAGS := $(UNIT_LDFLAGS)

# UNIT_DEPS := src/lib/log
UNIT_DEPS := src/lib/ovsdb
# UNIT_DEPS += src/lib/schema