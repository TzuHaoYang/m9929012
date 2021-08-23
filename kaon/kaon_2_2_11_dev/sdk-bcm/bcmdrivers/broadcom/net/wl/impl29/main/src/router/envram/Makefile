#
# Linux ENVRAM Makefile
#
# $Copyright Broadcom Corporation$
#
# $Id: Makefile 679153 2017-01-12 19:36:34Z arishoni $
#


include $(TOP)/.config

CFLAGS	+= -I. -I$(SRCBASE)/include ${WLAN_StdIncPathA} -I$(SRCBASE)/../components/shared -I$(SRCBASE)/../components/wlioctl/include -I$(SRCBASE)/../components/proto/include -I$(SRCBASE)/../components/math/include -I$(SRCBASE)/common/include -I$(SRCBASE)/tools/misc//lzma_src/C/ -Wall

#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
ifneq (2_4,$(LINUX_VERSION))
CFLAGS += -DLINUX26
endif
LDFLAGS += -L.

vpath %.c $(SRCBASE)/shared/ $(SRCBASE)/tools/misc/lzma_src/C/ $(SRCBASE)/../components/math/src

all: $(if $(CONFIG_ENVRAM_UTILITY),envrams) $(if $(CONFIG_ENVRAM_UTILITY),envram)

install: all
ifneq ($(CONFIG_ENVRAM_UTILITY),)
	install -d $(INSTALLDIR)/usr/sbin/
	install envrams $(INSTALLDIR)/usr/sbin/
	install envram $(INSTALLDIR)/usr/sbin/
	$(STRIP) $(INSTALLDIR)/usr/sbin/envrams
	$(STRIP) $(INSTALLDIR)/usr/sbin/envram
else
	# Utility not wanted: get rid of any leftover one
	rm -f $(INSTALLDIR)/usr/sbin/envrams
	rm -f $(INSTALLDIR)/usr/sbin/envram
endif


clean:
	rm -f *.o *.a *.so envram

envrams: bcmutils.o bcm_math.o envrams.o LzmaLib.o Alloc.o LzmaEnc.o LzmaDec.o LzFind.o
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS) 

envram: envramc.o
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS) 
