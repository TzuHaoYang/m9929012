EXE := teec_sec_key
OBJS := teec_sec_key.o sec_key_lib.o
LIBS := -lteec
Q ?= @
all install: conditional_build

clean: conditional_clean

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifeq ($(strip $(BCM_OPTEE)),y)
conditional_build: $(EXE) generic_exe_install
	install -m 755 demo_helper $(INSTALL_DIR)/bin$(BCM_INSTALL_SUFFIX_DIR)
	install -m 755 demo.sh $(INSTALL_DIR)/bin$(BCM_INSTALL_SUFFIX_DIR)

conditional_clean:
	$(Q)rm -f $(EXE) *.o *.d
	$(Q)rm -f $(INSTALL_DIR)/bin/$(EXE)
else
conditional_build conditional_clean: ;
endif


#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I$(BCM_FSBUILD_DIR)/public/include

#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public


$(EXE): $(OBJS)
	@echo "LD   $@"
	$(Q)$(CC) -o $@ $^ -Wl,-rpath, -L$(BCM_FSBUILD_DIR)/public/lib  $(LIBS)


-include $(OBJS:.o=.d)
