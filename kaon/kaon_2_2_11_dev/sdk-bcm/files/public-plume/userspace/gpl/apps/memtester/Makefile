memtester: conditional_build 


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

CFLAGS:=-Wno-maybe-uninitialized $(CFLAGS)
#export LINUX_VER_STR TOOLCHAIN_PREFIX
#export CFLAGS LDFLAGS

APP = memtester-4.1.3

conditional_build: all

memtester/configure: $(APP).tar.gz
	mkdir -p $(APP)
	(cd $(APP) && tar --strip-components=1 -xzf ../$(APP).tar.gz)
	touch -c $(APP)/configure

	sed -i 's,cc -O2,$(CC) $(CFLAGS),' $(APP)/conf-cc 
	sed -i "s,cc,$(CC)," $(APP)/conf-ld 

	@echo "$(APP) is untarred"

all: memtester/configure
	cd $(APP); $(MAKE)
	install -m 755 $(APP)/memtester $(INSTALL_DIR)/bin

clean:
	rm -f $(INSTALL_DIR)/bin/memtester
	rm -rf $(APP)

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i

