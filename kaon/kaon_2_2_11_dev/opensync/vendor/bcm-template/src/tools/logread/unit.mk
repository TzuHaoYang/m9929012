##############################################################################
#
# Plume's version of the logread utility
#
##############################################################################

UNIT_NAME := logread
UNIT_DIR := tools

# Template type:
UNIT_TYPE := BIN

UNIT_SRC := src/logread.c

UNIT_DEPS := src/lib/tailf
# UNIT_DEPS += src/lib/osa
# UNIT_DEPS += src/lib/common
UNIT_DEPS += src/lib/target
UNIT_DEPS += src/lib/log

# UNIT_LDFLAGS := -ljansson
