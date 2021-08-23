
APP = jansson-2.7
LIBNAME = libjansson

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

UNTAR_COMPLETE_FILE = .untar_complete
CONFIG_COMPLETE_FILE = config_complete

$(APP)/$(UNTAR_COMPLETE_FILE): $(APP).tar.gz Makefile
	rm -rf $(APP)
	tar xf $(APP).tar.gz
	(cd $(APP); for PATCH in ../*.patch; do if [ -e "$$PATCH" ]; then patch -p1 < $$PATCH; fi; done)
	@echo "$(APP) is untarred"
	touch $(APP)/$(UNTAR_COMPLETE_FILE)

$(APP)/$(CONFIG_COMPLETE_FILE): $(APP)/$(UNTAR_COMPLETE_FILE) 
	(cd $(APP); ./configure \
	    --host=$(TOOLCHAIN_PREFIX) \
        --target=$(TOOLCHAIN_PREFIX) \
	    --prefix=$(BCM_FSBUILD_DIR)/public \
	    --sbindir=$(BCM_FSBUILD_DIR)/public/bin \
	    --libdir=$(BCM_FSBUILD_DIR)/public/lib \
	    CFLAGS=-s\ -Os\ -Wno-strict-aliasing || exit 1);
	@echo "$(APP) is configured"
	touch $(APP)/$(CONFIG_COMPLETE_FILE)

conditional_build: $(APP)/$(CONFIG_COMPLETE_FILE) sanity_check
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install
	cp -a $(BCM_FSBUILD_DIR)/public/lib/$(LIBNAME).so* $(INSTALL_DIR)/lib/
	echo "Done building $(APP)"

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(INSTALL_DIR)/lib/$(LIBNAME).so*
	@if [ -e $(APP)/Makefile ]; then \
		$(MAKE) -C $(APP) uninstall; \
		$(MAKE) -C $(APP) distclean; \
	fi
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

shell:
	bash

