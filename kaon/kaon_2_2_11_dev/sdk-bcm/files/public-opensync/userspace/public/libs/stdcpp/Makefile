all installl: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

APP:=stdcpp

# Copy the C++ runtime, the library name is in the libstdc++.XX.YY.ZZ format (and all variantes without XX, YY, ZZ).
# EXTRALIBDIR actually points to the toolchain lib folder
conditional_build:
	find $(EXTRALIBDIR) -regex '.*/libstdc\+\+\.so\(\.[0-9]+\)*' | while read x ;\
	do \
		cp -a "$$x" "$(INSTALL_DIR)/lib/" ;\
		[ ! -f "$$X" ] || $(STRIP) "$(INSTALL_DIR)/lib/$$(basename $$x)" ;\
	done
