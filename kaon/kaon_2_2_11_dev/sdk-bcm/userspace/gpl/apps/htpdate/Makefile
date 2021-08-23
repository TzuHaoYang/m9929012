
all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
export CFLAGS
LDFLAGS = $(BCM_LD_FLAGS)
export LDFLAGS
endif

APP = htpdate-1.0.4

check_untar_patch_configure: sanity_check
	if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring htpdate source ..." ; \
		(tar xjf $(APP).tar.bz2 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		(for PATCH in patches/* ; \
		do \
			patch -p1 -b -N -s -d $(APP) < $$PATCH ; \
		done) \
	fi

conditional_build: check_untar_patch_configure
	@echo "Making $(APP)"
	@cd $(APP) ; \
	$(MAKE) CC=$(CC) ; \
	mkdir -p $(INSTALL_DIR)/usr/bin ; \
	mkdir -p $(INSTALL_DIR)/etc/init.d ; \
	install -m 755 htpdate $(INSTALL_DIR)/usr/bin/htpdate ; \
	install -m 755 ../init.d $(INSTALL_DIR)/etc/init.d/htpdate ; \
	$(STRIP) $(INSTALL_DIR)/usr/bin/htpdate ;

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	@if [ -e $(APP)/Makefile ]; then \
		cd $(APP); $(MAKE) clean ; \
	fi
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

