# Makefile to build libmxml

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

APP = mxml-2.11

all dynamic install:

include $(BUILD_DIR)/make.common

$(APP)/Makefile:
	mkdir "$(APP)"
	tar -C "$(APP)" -xzf "$(APP).tar.gz"
	ln -sf ../patches "$(APP)/patches"
	#cd "$(APP)"; quilt push -a
	cd "$(APP)" && ./configure \
		--host="$(TOOLCHAIN_PREFIX)" \
		--target="$(TOOLCHAIN_PREFIX)" \
		--prefix="$(BCM_FSBUILD_DIR)/public" \
		--disable-shared

prepare: $(APP)/Makefile

conditional_build: prepare
	@echo "Making $(APP)"
	@cd $(APP) && make && make install

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

