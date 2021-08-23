# Makefile to build dnsmasq

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

all dynamic install:

include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
export CFLAGS
LDFLAGS = $(BCM_LD_FLAGS)
export LDFLAGS
endif

APP = dnsmasq-2.78

$(APP)/Makefile:
	tar xzf "$(APP).tar.gz"
	ln -sf ../patches "$(APP)/patches"
	cd "$(APP)"; quilt push -a

prepare: $(APP)/Makefile

conditional_build: prepare
	@echo "Making $(APP)"
	@cd $(APP); \
	$(MAKE); \
	mkdir -p $(INSTALL_DIR)/usr/sbin; \
	install -m 755 src/dnsmasq $(INSTALL_DIR)/usr/sbin/dnsmasq
	$(STRIP) $(INSTALL_DIR)/usr/sbin/dnsmasq

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

