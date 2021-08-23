
APP = mosquitto-1.6.9
LIBNAME = libmosquitto
APPNAME = mosquitto

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

UNTAR_COMPLETE_FILE = .untar_complete
CONFIG_COMPLETE_FILE = config_complete

MAKE_ARGS=CROSS_COMPILE= WITH_SRV=no WITH_UUID=no prefix=/
export CFLAGS  := -I$(BCM_FSBUILD_DIR)/public/include
export LDFLAGS := -L$(BCM_FSBUILD_DIR)/public/lib -Wl,-rpath-link=$(BCM_FSBUILD_DIR)/public/lib
export DESTDIR := $(BCM_FSBUILD_DIR)/public

$(APP)/$(UNTAR_COMPLETE_FILE): $(APP).tar.gz Makefile
	rm -rf $(APP)
	tar xf $(APP).tar.gz
	(cd $(APP); for PATCH in ../*.patch; do if [ -e "$$PATCH" ]; then patch -p1 < $$PATCH; fi; done)
	@echo "$(APP) is untarred"
	touch $(APP)/$(UNTAR_COMPLETE_FILE)

$(APP)/$(CONFIG_COMPLETE_FILE): $(APP)/$(UNTAR_COMPLETE_FILE) 
	@echo "$(APP) is configured"
	touch $(APP)/$(CONFIG_COMPLETE_FILE)

conditional_build: $(APP)/$(CONFIG_COMPLETE_FILE) sanity_check
	$(MAKE) -C $(APP) $(MAKE_ARGS)
	$(MAKE) -C $(APP) $(MAKE_ARGS) install
	cp -a $(BCM_FSBUILD_DIR)/public/lib/$(LIBNAME).so* $(INSTALL_DIR)/lib/
	cp -a $(BCM_FSBUILD_DIR)/public/sbin/$(APPNAME) $(INSTALL_DIR)/sbin/
	echo "Done building $(APP)"

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(INSTALL_DIR)/lib/$(LIBNAME).so*
	rm -f $(INSTALL_DIR)/sbin/$(APPNAME)
	@if [ -e $(APP)/Makefile ]; then \
		$(MAKE) -C $(APP) uninstall; \
		$(MAKE) -C $(APP) dist; \
	fi
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

shell:
	bash

