##############################################################################
#
# Watchdog Proxy Daemon
#
##############################################################################
UNIT_NAME := wpd
UNIT_TYPE := BIN

UNIT_SRC  := src/wpd.c
UNIT_SRC  += src/wpd_util.c

UNIT_LDFLAGS += -lev
UNIT_DEPS := src/lib/log
UNIT_DEPS += src/lib/osp

UNIT_DISABLE := $(if $(CONFIG_WPD_ENABLED),n,y)
