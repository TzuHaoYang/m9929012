
#GLOBAL_RELEASE_SCRIPT_NOTOUCH


ifneq ($(wildcard Makefile.fullsrc),)

include Makefile.fullsrc

else

EXE = tr69c

all dynamic install: conditional_build


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_TR69C)),)

conditional_build: tr69c generic_exe_install

clean:
	rm -f $(EXE)

else 

conditional_build:
	@echo "skipping  $(EXE) (not configured)"

clean:

endif # BUILD_TR69C

tr69c:
	cp $(EXE)_$(PROFILE).save $(EXE)

endif # NO_Makefile.fullsrc
