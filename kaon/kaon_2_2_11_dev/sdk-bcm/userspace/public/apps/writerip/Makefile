
ifneq ($(wildcard Makefile.fullsrc),)

include Makefile.fullsrc

else
LIBS += -lpthread -lbcm_boardctl

EXE = writerip
dynamic: generic_exe_install

clean:
	rm -f $(EXE)

ethswctl:
	cp $(EXE)_$(PROFILE).save $(EXE)

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

endif
