
all: build

.PHONY: all distclean clean conditional_build

CURR_DIR  := $(shell pwd)
BUILD_DIR := $(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR := $(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
export TARGET=$(OPENSYNC_TARGET)
export BRCM_RELEASETAG

ifeq ($(OPENSYNC_SRC),)
ifneq ($(PLUME_SRC),)
# backward compatibility
OPENSYNC_SRC := $(PLUME_SRC)
endif
endif

ifneq ($(OPENSYNC_SRC),)
OSYNC_DIR       := $(OPENSYNC_SRC)
else
OSYNC_DIR       := $(CURR_DIR)/git
OSYNC_REPO      := git@github.com:plume-design/device-plume.git
OSYNC_BRANCH    := master
OSYNC_MODULES   := core vendor/plume platform/bcm 3rdparty/webroot 3rdparty/plume 3rdparty/symantec

$(OSYNC_DIR):
	rm -rf $(OSYNC_DIR).tmp
	git clone -b $(OSYNC_BRANCH) $(OSYNC_REPO) $(OSYNC_DIR).tmp
	git -C $(OSYNC_DIR).tmp submodule update --init $(OSYNC_MODULES)
	mv $(OSYNC_DIR).tmp $(OSYNC_DIR)
endif

OSYNC_CORE      := $(OSYNC_DIR)/core

export USERSPACE_DIR=$(BUILD_DIR)/userspace
export BCM_BUILD_ROOT=$(BUILD_DIR)

build: $(OSYNC_DIR)
	@echo ===== OPENSYNC BUILD START =====
	@#env
	make -C $(OSYNC_CORE) PLATFORM=bcm rootfs-install -j $(nproc)
	@echo ===== OPENSYNC BUILD END =====

clean:
	@echo ===== OPENSYNC CLEAN START =====
	make -C $(OSYNC_CORE) PLATFORM=bcm clean
	@echo ===== OPENSYNC CLEAN END =====

shell:
	bash

# redirect any custom targets
INT_GOALS := all build clean shell $(OSYNC_DIR)
ifneq ($(MAKECMDGOALS),)
ifeq ($(filter $(INT_GOALS),$(MAKECMDGOALS)),)
$(MAKECMDGOALS):
	make -C $(OSYNC_CORE) PLATFORM=bcm $(MAKECMDGOALS)
endif
endif

