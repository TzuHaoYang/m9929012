# Makefile to build tinyproxy

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

all dynamic install:

include $(BUILD_DIR)/make.common

APP = tinyproxy-1.10.0

export CFLAGS
export LDFLAGS

APPCFLAGS+= -I$(BCM_FSBUILD_DIR)/gpl/include -I$(BCM_FSBUILD_DIR)/public/include

$(APP)/Makefile:
	tar xzf "$(APP).tar.gz"
	ln -sf ../patches "$(APP)/patches"
	if [ ! -e patches/series ]; then (cd patches; ls *.patch > series;); fi; \
	cd $(APP); quilt push -a || exit $$?;
	touch .unpacked; \
    cd $(APP);./autogen.sh; \
    echo "Checking configure on $(APP)/configure"; \
	    if [ -e configure ]; then \
	        echo "Running configure on $(APP)"; \
	        ./configure --host=$(TOOLCHAIN_PREFIX) \
	        --target=$(TOOLCHAIN_PREFIX) CFLAGS="$(APPCFLAGS)" LIBS="-lssl -lcrypto" LDFLAGS="-L$(BCM_FSBUILD_DIR)/gpl/lib -L$(BCM_FSBUILD_DIR)/public/lib"; \
	    fi; \

prepare: $(APP)/Makefile

conditional_build: prepare
	@echo "Making $(APP)"
	@cd $(APP); \
	$(MAKE); \
	mkdir -p $(INSTALL_DIR)/usr/sbin; \
	install -m 755 src/tinyproxy $(INSTALL_DIR)/usr/sbin/tinyproxy
	$(STRIP) $(INSTALL_DIR)/usr/sbin/tinyproxy

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

