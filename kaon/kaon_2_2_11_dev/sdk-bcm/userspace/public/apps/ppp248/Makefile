all dynamic install: conditional_build

PPPD_VER := 2.4.8

BUILD_DIR:=$(word 1,$(subst /userspace, /userspace,$(CURDIR)))
include $(BUILD_DIR)/make.common
APPSOURCE = ppp-$(PPPD_VER).tar.gz
APPDIR = ppp-$(PPPD_VER)

INSTALL := install -m 644
INSTALL_BIN := install -s --strip-program=$(STRIP) -m 755

export CXX CFLAGS CXXFLAGS LDFLAGS CC USERSPACE_PUBLIC_LIBS_DIR

.PHONY: clean conditional_build pppd_extract pppd_build pppd_install

clean:
	rm -rf $(APPDIR)

ifneq ($(and $(BUILD_OPENSYNC),$(BUILD_PPPD),y),)
conditional_build: pppd_install
else
conditional_build: sanity_check
	@echo "skipping $(APPDIR) (not configured)"
endif

pppd_extract: sanity_check $(APPDIR)/.extracted

$(APPDIR)/.extracted:
	@echo "Extracting  $(APPDIR) source"
	tar zxf $(APPSOURCE)
	@echo "Applying quilt patches to $(APPDIR)"
	ln -sf ../patches $(APPDIR)/patches
	@[ ! -e patches/series ] || {\
		cd $(APPDIR) && quilt push -a ;\
	}
	cd $(APPDIR) && ./configure --prefix=/usr --host=$(TOOLCHAIN_PREFIX) --target=$(TOOLCHAIN_PREFIX)
	@touch $@

pppd_build: pppd_extract
	$(MAKE) -C $(CURDIR)/$(APPDIR)

pppd_install: pppd_build
	mkdir -p $(INSTALL_DIR)/etc/ppp
	mkdir -p $(INSTALL_DIR)/usr/lib/pppd/$(PPPD_VER)
	$(INSTALL_BIN) $(APPDIR)/pppd/pppd $(INSTALL_DIR)/usr/sbin
	$(INSTALL_BIN) $(APPDIR)/pppd/plugins/rp-pppoe/rp-pppoe.so $(INSTALL_DIR)/usr/lib/pppd/$(PPPD_VER)/rp-pppoe.so
	install -m 644 $(APPDIR)/etc.ppp/* $(INSTALL_DIR)/etc/ppp
	install -m 755 $(APPDIR)/scripts/ip-up.local.add $(INSTALL_DIR)/etc/ppp/ip-up
	install -m 755 $(APPDIR)/scripts/ip-down.local.add $(INSTALL_DIR)/etc/ppp/ip-down

