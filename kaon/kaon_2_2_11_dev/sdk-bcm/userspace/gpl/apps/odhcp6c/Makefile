# Makefile to build odhcp6c

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

all dynamic install:

include $(BUILD_DIR)/make.common

export CFLAGS
export LDFLAGS

APP = odhcp6c-2015-07-29
APP_ARCHIVE = $(APP).tar.bz2
APP_BIN = odhcp6c

check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/.unpacked ]; then \
		echo "Untarring $(APP) source and overrides..." ; \
		tar xf $(APP_ARCHIVE); \
		echo "Applying patches to $(APP) ..." ; \
		cd $(APP); \
		ln -sf ../patches .; \
		if [ ! -e patches/series ]; then (cd patches; ls *.patch > series;); fi; \
		quilt push -a || exit $$?; \
		touch .unpacked; \
	fi

conditional_build: check_untar_patch_configure
	@echo "Making $(APP)"
	@cd $(APP); \
	cmake . && $(MAKE) || exit $?; \
	mkdir -p $(INSTALL_DIR)/usr/sbin && \
	install -m 755 $(APP_BIN) $(INSTALL_DIR)/usr/sbin/$(APP_BIN) && \
	$(STRIP) $(INSTALL_DIR)/usr/sbin/$(APP_BIN)
	mkdir -p $(INSTALL_DIR)/lib/netifd/proto && \
	install -m 755 files/dhcpv6.sh $(INSTALL_DIR)/lib/netifd/proto/dhcpv6.sh && \
	install -m 755 files/dhcpv6.script $(INSTALL_DIR)/lib/netifd/

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
bcm_dorel_distclean: distclean

