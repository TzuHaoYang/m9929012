
all dynamic install: conditional_build


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_BRCM_CMS)),)
ifneq ($(strip $(BUILD_PMD)),)
LIBS += -Wl,-rpath,$(BCM_FSBUILD_DIR)/private/lib:$(BCM_FSBUILD_DIR)/public/lib -L$(BCM_FSBUILD_DIR)/private/lib -lpmd -L$(BCM_FSBUILD_DIR)/public/lib -ljson-c
endif
LIBS += -lcms_cli -lcms_dal -lcms_msg $(CMS_CORE_LIBS) $(CMS_COMMON_LIBS)
CUSTOM_CFLAGS += -DBRCM_CMS_BUILD
CFLAGS +=$(ALLOWED_INCLUDE_PATHS)
endif

ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS := $(BCM_LD_FLAGS)
export BCM_BLT32_FLAGS
TOOLCHAIN_PREFIX=$(PROFILE_ARCH)-none-linux
endif

ifneq ($(strip $(BUILD_OPENSYNC)),)
# ./configure is expected to use --prefix=/ instead of usual
# --prefix=/usr. This requires adjusting path to dbclient
# which is not automatically derived by dropbear based on
# the prefix.
CFLAGS += -DDROPBEAR_PATH_SSH_PROGRAM=\\\"/bin/dbclient\\\"
endif

#
# Public apps are only allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)

APP = dropbear-2019.78

clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	rm -rf $(APP)

#.PHONY: check_untar_patch_configure conditional_build

# Public apps are only allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

ifneq ($(strip $(BUILD_SSHD)),)

SHADOW_FLAG = $(if $(strip $(BUILD_OPENSYNC)),--enable-shadow,--disable-shadow)
check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/Makefile.in ]; then \
    	echo "Untarring original $(APP) source"; \
	    (tar xkf $(APP).tar.bz2 2> /dev/null || true); \
	    if [ "x$(BUILD_BTEST_TOOLS)" == "x" ]; then \
	        patch -p1 -b -s -d$(APP) < $(APP).patch; \
	    fi; \
	    if [ "x$(BUILD_BRCM_CMS)" != "x" ]; then \
		patch -p1 -b -s -d$(APP) < $(APP)_cms.patch; \
	    fi; \
	    if [ "x$(BUILD_OPENSYNC)" != "x" ]; then \
		mkdir -p $(APP)/patches && cd $(APP)/patches && for i in ../../???-*.patch; do ln -vsf $$i .; done && ls > .series && mv -v .series series && cd .. && quilt push -a || exit $$?; \
	    fi; \
	    (cd $(APP); ./configure CFLAGS="$(CFLAGS)" LDFLAGS="$(BCM_BLT32_FLAGS) -L$(INSTALL_DIR)/lib" LIBS="$(LIBS)" --host=$(TOOLCHAIN_PREFIX) \
        --target=$(TOOLCHAIN_PREFIX) \
        --disable-largefile --disable-zlib  --disable-openpty --disable-syslog $(SHADOW_FLAG) --disable-lastlog  --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx  \
        --prefix=/ \
        --sbindir=/bin); \
	fi

conditional_build: check_untar_patch_configure
ifneq ($(strip $(BUILD_OPENSYNC)),)
	$(MAKE) -C $(APP) \
		MULTI=1 \
		PROGRAMS="dropbear dbclient scp dropbearkey ssh" \
		SCPPROGRESS=1 \
		DESTDIR=$(INSTALL_DIR) \
		inst_dropbearmulti
else
	cd $(APP); $(MAKE)
	install -m 755 $(APP)/dropbear  $(INSTALL_DIR)/bin/sshd
	install -m 755 $(APP)/dropbearconvert $(INSTALL_DIR)/bin/dropbearconvert
endif
ifneq ($(strip $(BUILD_BTEST_TOOLS)),)
	mkdir -p $(INSTALL_DIR)/etc/dropbear
	ln -fs /var/tmp/dropbear_dss_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_dss_host_key
	ln -fs /var/tmp/dropbear_rsa_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_rsa_host_key
	ln -fs /var/tmp/dropbear_ecdsa_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_ecdsa_host_key
endif
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)



