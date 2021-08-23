
APP = ncurses-6.1
LIBNAME = libncurses

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
	    --prefix= \
	    --exec-prefix= \
	    --with-install-prefix=$(BCM_FSBUILD_DIR)/gpl \
	    --without-ada \
	    --without-cxx \
	    --without-cxx-binding \
	    --without-manpages \
	    --without-progs \
	    --without-tack \
	    --without-tests \
	    --without-debug \
	    --without-profile \
	    --without-develop \
	    --disable-home-terminfo \
	    --enable-echo \
	    --enable-const \
	    --enable-overwrite \
	    --disable-rpath \
	    --disable-big-core \
	    --with-shared \
	    CFLAGS="-s -Os -Wno-strict-aliasing" \
	    CPPFLAGS="-P" \
	    || exit 1);
	@echo "$(APP) is configured"
	touch $(APP)/$(CONFIG_COMPLETE_FILE)


conditional_build: $(APP)/$(CONFIG_COMPLETE_FILE) sanity_check
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install.libs install.data

	mkdir -p $(INSTALL_DIR)/lib
	mkdir -p $(INSTALL_DIR)/share

	cp -a $(BCM_FSBUILD_DIR)/gpl/lib/$(LIBNAME).so* $(INSTALL_DIR)/lib/
	cp -a $(BCM_FSBUILD_DIR)/gpl/share/terminfo $(INSTALL_DIR)/share/

	echo "Done building $(APP)"

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(INSTALL_DIR)/lib/$(LIBNAME).so*
	rm -rf $(INSTALL_DIR)/share/terminfo
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

shell:
	bash

