# Makefile to build miniupnpd


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

CFLAGS := $(CFLAGS) -I$(BCM_FSBUILD_DIR)/public/include -I$(BUILD_DIR)/kernel/linux-4.1/include/config/netfilter -I$(BUILD_DIR)/userspace/public/include
CFLAGS := $(CFLAGS) -Wno-error=maybe-uninitialized  # fixing miniupnpd build errors here


LDFLAGS := $(LDFLAGS) $(BCM_LD_FLAGS) -L$(BCM_FSBUILD_DIR)/public/lib
# CFLAGS := $(CFLAGS) $(LDFLAGS)

LDFLAFGS := $(LDFLAGS)  # miniupnpd's Makefile will use $LDFLAFGS if $IPTABLES_PATH is set (and we need to set it for cross-build)

export LDFLAFGS
export CFLAGS
export LDFLAGS

APP = miniupnpd-2.1

IPTABLES_PATH=$(BUILD_DIR)/userspace/gpl/apps/iptables/iptables-1.6.2

check_untar_patch_configure: sanity_check
	if [ ! -e $(APP)/Makefile.linux ]; then \
		echo "Untarring miniupnpd source and overrides..." ; \
		(tar xzf $(APP).tar.gz 2> /dev/null || true) ; \
		echo "Applying patches to $(APP) ... none" ; \
	fi

conditional_build: check_untar_patch_configure
	@echo "Making $(APP)"
	@cd $(APP); \
	cp ../config.h.bck ./config.h; \
	[ -e $(IPTABLES_PATH)/libiptc/.libs/libip4tc.o ] || ln -s $(IPTABLES_PATH)/libiptc/libip4tc.o  $(IPTABLES_PATH)/libiptc/.libs/libip4tc.o; \
	IPTABLESPATH=$(IPTABLES_PATH) $(MAKE) -f Makefile.linux; \
	mkdir -p $(INSTALL_DIR)/usr/sbin && install -m 755 miniupnpd $(INSTALL_DIR)/usr/sbin/miniupnpd; \
	$(STRIP) $(INSTALL_DIR)/usr/sbin/miniupnpd

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
bcm_dorel_distclean: distclean

