
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)


EXE  = btrmcfg

OBJS = $(EXE).o

SAVED = btrmcfg.o_saved


all install : $(EXE)
	mkdir -p $(PREFIX)/bin;  \
	install -m 755 $(EXE) $(PREFIX)/bin; 


ifneq ($(wildcard $(mdir)/*.c),)
$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) -Wl,-rpath,$(LIBS)

else
$(EXE): $(SAVED)
	cat $< > $@

endif

clean: 
	-rm -f *.o *.d $(EXE) $(PREFIX)/bin/$(EXE)

binaryonly_prepare:
	-cp $(OBJS) $(SAVED)
	-$(MAKE) -f $(name) clean
	rm -rf $(mdir)/*.c $(mdir)/*.h 


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
