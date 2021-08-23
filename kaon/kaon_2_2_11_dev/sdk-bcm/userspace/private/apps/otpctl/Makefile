
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)


EXE  = otpctl

OBJS = $(EXE).o


all install : $(EXE)
	mkdir -p $(PREFIX)/bin;  \
	install -m 755 $(EXE) $(PREFIX)/bin; 

$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) -Wl,-rpath,$(LIBS)

clean: 
	-rm -f *.o *.d $(EXE) $(PREFIX)/bin/$(EXE)

binaryonly_prepare:
	-cp $(OBJS) $(SAVED)
	-$(MAKE) -f $(name) clean
	rm -rf $(mdir)/*.c $(mdir)/*.h 


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
