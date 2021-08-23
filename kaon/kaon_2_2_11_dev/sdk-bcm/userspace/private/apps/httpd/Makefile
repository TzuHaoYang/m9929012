#
# In most cases, you only need to modify this first section.
#
EXE = httpd

OBJS = main.o httpd.o basic.o ej.o cgi_cmd.o cgi_dhcpd.o cgi_main.o \
       cgi_ntwk.o cgi_route.o cgi_sec.o cgi_sts.o cgi_upload.o cgi_util.o cgi_wan.o \
       cgi_selt.o \
       cgi2_util.o cgi2_wan.o cgi2_sec.o cgi2_sts.o \
       cgi2_ethintf.o cgi2_bridging.o cgi2_route.o cgi2_usb_hosts.o cgi.o \
       cgi_diag.o cgi2_diag.o cgi2_stsdsl.o cgi2_stsxtm.o

WAN_OBJS = cgi_qos.o cgi2_qos.o \
           cgi_qos_class.o cgi2_qos_class.o \
           cgi_qos_queue.o cgi2_qos_queue.o \
           cgi_qos_policer.o cgi2_qos_policer.o \
           cgi_qos_port_shaping.o cgi2_qos_port_shaping.o \
           cgi_ethintf.o \
           cgi_pptpacintf.o cgi_seclog.o cgi_autodetection.o 


all dynamic install: $(EXE) generic_exe_install cpall_html extra_install

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)

.PHONY: all dynamic install extra_install


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


CUSTOM_CFLAGS += $(BRCM_APP_CFLAGS)

LIBS = -lcms_dal -lcms_msg $(CMS_COMMON_LIBS) $(CMS_CORE_LIBS)

ifneq ($(strip $(BUILD_CUSTOMER)),)
LIBS += -lcms_dal_customer
include customers/$(BUILD_CUSTOMER_NAME)/make.$(BUILD_CUSTOMER_NAME)
endif

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
LOCAL_IMPL=$(strip $(shell readlink $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/bcm9$(BRCM_CHIP) | sed 's/[^0-9]//g'))
ifeq ($(LOCAL_IMPL),)
# the symbolic link of bcm9$(BRCM_CHIP) has been removed/cleaned, set IMPL to an integer to avoid warning message
LOCAL_IMPL=0
endif
WL_IMPL_PLUS= $(strip $(shell if [ "$(LOCAL_IMPL)" -ge 24 ]; then echo Y; fi ;))
WL_BASE=$(strip $(shell echo /main;))
WL_SRC_BASE=$(shell echo /main/src;)
DHD_WL_EXE=$(shell echo dhd/src/wl/exe;)
DHD_EXE=$(shell echo dhd/src;)
ifeq ($(strip $(BUILD_BRCM_CPEROUTER)),)
-include $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/impl$(LOCAL_IMPL)$(WL_SRC_BASE)/wl/config/wlconfig_lx_wl_dslcpe
-include $(BUILD_DIR)/bcmdrivers/broadcom/net/wl/impl$(LOCAL_IMPL)$(WL_SRC_BASE)/router/config/defconfig-2.6-vista-dhdap-router-atlas
else
ifneq ($(strip $(BUILD_BCM_WLAN_DPDCTL)),)
CUSTOM_CFLAGS += -DBUILD_BCM_WLAN_DPDCTL
endif
endif
WIRELESS_DRIVER_PATH=$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)$(WL_SRC_BASE)
WLCSM_EXT_BASE:=$(BRCMDRIVERS_DIR)/broadcom/char/wlcsm_ext/impl1
CUSTOM_CFLAGS += -DBUILD_WLAN
endif

BASE_HTMLS = backupsettings.html defaultsettings.html dhcpinfo.html diag.html                    \
             diagbr.html diaglan.html dnscfg.html footer.html hlpethconn.html hlpusbconn.html hlpwlconn.html \
             index.html info.html lancfg2.html lockerror.html logconfig.html logintro.html       \
             logo.html main.html menu.html password.html rebootinfo.html resetrouter.html        \
             restoreinfo.html routeadd.html StaticIpAdd.html StaticIpErr.html statsifc.html      \
             statsifcreset.html updatesettings.html upload.html uploadinfo.html diagethoam.html  \
             statsqueue.html

WAN_HTMLS =  algcfg.html diagipow.html diagmer.html diagpppoa.html diagpppoe.html                   \
             enblbridge.html enblservice.html hlpatmetoe.html hlpatmseg.html hlppngdns.html         \
             hlppnggw.html hlppppoasess.html hlppppoeauth.html hlppppoeconn.html hlppppoeip.html    \
             hlptstdns.html ifcdns.html ifcgateway.html ipoacfg.html lanvlancfg.html natcfg2.html   \
             ntwksum2.html pppoe.html pradd.html qoscls.html qospoliceradd.html qosportshaping.html \
             qosqmgmt.html qosqueueadd.html rtdefaultcfgerr.html rtdefaultcfg.html                  \
             scdmz.html scinflt.html scmacflt.html scmacpolicy.html scoutflt.html scprttrg.html     \
             scvrtsrv.html seclogintro.html statswanreset.html wanadderr.html wancfg.html

UNUSED_HTMLS = enblbridge.html enblservice.html statsatmerr.html statsatm.html statsatmreset.html

HTMLS = html/*.js html/*.png html/*.css $(addprefix html/,$(BASE_HTMLS))

ifneq ($(strip $(BUILD_WAN_HTML)),)
OBJS += $(WAN_OBJS)
HTMLS += $(addprefix html/,$(WAN_HTMLS))
CUSTOM_CFLAGS += -DSUPPORT_WAN_HTML
endif


ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
LIBS += -lwlsysutil
ifeq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
LIBS += -lwlcsm
endif
ifneq ($(strip $(BUILD_BRCM_CPEROUTER)),)
LIBS += -lwifihttp -lnvram -lbcmcrypto -lshared -L$(INSTALL_DIR)/usr/lib
else
LIBS += -lwifihttp -lnvram -lwlbcmcrypto -lwlbcmshared
endif
ifneq ($(strip $(WL_IMPL_PLUS)), )
CUSTOM_CFLAGS += -DWL_IMPL_PLUS
endif
ifneq ($(strip $(HNDAPPS_IN_COMP)),)
CFLAGS += -I$(HNDAPPS_DIR)/shared  -I$(HNDAPPS_DIR)/../wlioctl/include -I$(HNDAPPS_DIR)/../proto/include
endif

ifneq ($(strip $(BUILD_WLVISUALIZATION)),)
CUSTOM_CFLAGS += -DSUPPORT_WLAN_VISUALIZATION
CUSTOM_CFLAGS += -I$(WIRELESS_DRIVER_PATH)/router/shared
endif

ifneq ($(strip $(BUILD_BSTREAM_IQOS)),)
ifneq ($(wildcard $(WIRELESS_DRIVER_PATH)/../components/vendor/trend/iqos),)
CUSTOM_CFLAGS += -DSUPPORT_WLAN_IQOS
endif 
LIBS += -ljson-c
endif
endif #BRCM_DRIVER_WIRELESS

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
include $(BUILD_DIR)/make.voice
LIBS += -lpthread
ifeq ($(BUILD_TR104_PROFILES),1)
OBJS += cgi_voice.o
endif
ifeq ($(BUILD_TR104_PROFILES),2)
OBJS += cgi2_voice.o
endif
HTMLS += html/voice*.html
endif

ifneq ($(strip $(BUILD_OMCI)),)
LIBS += -lomciutil
OBJS += cgi_omci_cfg.o cgi_omci_create.o cgi_omci_getnext.o \
	cgi_omci_macro.o cgi_omci_download.o cgi_omci_raw.o
HTMLS += html/omci*.html
CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/private/include
ifneq ($(strip $(BUILD_BCMIPC)),)
CUSTOM_CFLAGS += -DBUILD_BCMIPC
LIBS += -lbcmipc
endif
endif

ifneq ($(strip $(BUILD_GPON)),)
OBJS += cgi_gponintf.o cgi_sts_dev2.o
HTMLS += html/*gpon*.html html/statsopticifc*.html
endif

ifneq ($(strip $(BUILD_QUICKSETUP)),)
OBJS += cgi_quicksetup.o cgi2_quicksetup.o
HTMLS += html/quick*.html html/qsmain.html
endif

ifneq ($(strip $(BUILD_STORAGESERVICE)),)
OBJS += cgi_storageservice.o
HTMLS += html/storage*.html
endif

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
OBJS += cgi_wl.o cgi_sts_wifi.o 
HTMLS += html/cfgwlwan.html
ifneq ($(strip $(BUILD_TR98_PROFILES)),)
# TR98 only files go here (currently none)
endif

ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
LIBS += -lwlmdm -lstaged -ljson-c -lpthread
CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/private/include
else
CUSTOM_CFLAGS += -I$(BUILD_DIR)/userspace/private/apps/wlan/wlcsm/dm/include
CUSTOM_CFLAGS += -I$(WLCSM_EXT_BASE)/include
endif # BUILD_BRCM_UNFWLCFG

endif # BRCM_DRIVER_WIRELESS

ifneq ($(strip $(BUILD_WEB_SOCKETS_TEST)),)
HTMLS += html/wstest.html
endif

ifneq ($(strip $(BUILD_LIBMODSW)),)
OBJS += cgi_modsw_ee.o
LIBS += -lmodsw -lbeeputil -lssl -lcrypto -lcurl -ljson-c
ifeq ($(strip $(BUILD_LIBCURL_WITH_HTTP2)),y)
LIBS   += -lnghttp2
endif
endif

ifneq ($(strip $(BUILD_LXC)),)
OBJS += cgi_container.o
HTMLS += html/containertable.html html/containerdetail.html
endif

ifneq ($(strip $(BUILD_CERT)),)
OBJS += cgi_cert.o 
HTMLS += html/cert*.html
endif

ifneq ($(strip $(BUILD_VCONFIG)),)
OBJS += cgi_pmap.o
HTMLS += html/port*.html
endif

ifneq ($(strip $(BUILD_BMU)),)
OBJS += cgi_bmu.o
HTMLS += html/bmu.html
endif

ifneq ($(strip $(BUILD_DDNSD)),)
OBJS += cgi_ddns.o
HTMLS += html/ddns*.html
endif

ifneq ($(strip $(BUILD_DLNA)),)
OBJS += cgi_dlna.o
HTMLS += html/dlna*.html
endif

ifneq ($(strip $(BUILD_DPROXY)),)
OBJS += cgi_dnsproxy.o
HTMLS += html/dnsp*.html
endif

ifneq ($(strip $(BUILD_DPROXYWITHPROBE)),)
OBJS += cgi_dnsproxy.o
HTMLS += html/dnsp*.html
endif

ifneq ($(strip $(BUILD_DSL)),)
OBJS += cgi_dslintf.o cgi2_dslintf.o
HTMLS += html/*dsl*.html html/cfgatm.html html/cfgptm.html html/atmdelerr.html html/ber*.html html/statsxtmreset.html html/ptm*.html
endif

ifneq ($(strip $(BUILD_DEBUG_TOOLS)),)
OBJS += cgi_dump.o cgi_eng.o
HTMLS += html/eng*.html
endif

ifneq ($(strip $(BUILD_EPONWAN)),)
OBJS += cgi_eponintf.o
HTMLS += html/*epon*.html
endif

ifneq ($(strip $(BUILD_EPON_SDK)),)
OBJS += cgi_epon.o
HTMLS += html/eponloid.html
endif

ifneq ($(strip $(BUILD_ETHWAN)),)
HTMLS += html/eth*.html html/cfgeth.html
endif

ifneq ($(strip $(BUILD_IPPD)),)
OBJS += cgi_ipp.o
HTMLS += html/ippcfg.html
endif

ifneq ($(strip $(BUILD_IPSEC_TOOLS)),)
OBJS += cgi_ipsec.o cgi2_ipsec.o
HTMLS += html/ips*.html
endif

ifeq ($(RDPA_IFILTER_SUPPORT),y)
OBJS += cgi_ingress_filters.o
HTMLS += html/ingressFilters.html
HTMLS += html/IF/*.js html/IF/*.css
endif

ifneq ($join $(strip $(DMP_X_BROADCOM_COM_IGMP_1)), $(strip $(DMP_X_BROADCOM_COM_MLD_1)), $(strip $(DMP_X_BROADCOM_COM_MCAST_1)),)
OBJS += cgi_multicast.o
endif

ifneq ($(strip $(BUILD_L2TPAC)),)
OBJS += cgi_l2tpacintf.o
HTMLS += html/*l2tp*.html
endif

ifneq ($(strip $(BUILD_IPV6)),)
OBJS += cgi_iptunnel.o cgi_lan6.o
HTMLS += html/ipv6lancfg.html html/tunnel*.html
ifneq ($(strip $(BUILD_MAP)),)
HTMLS += html/map.html
endif
endif

ifneq ($(strip $(BUILD_MoCACTL)),)
OBJS += cgi_mocaintf.o cgi2_sts_moca.o
HTMLS += html/*moca*.html
endif

ifneq ($(strip $(BUILD_MCAST_PROXY)),)
HTMLS += html/multicast.html
endif

ifneq ($(strip $(BUILD_PWRCTL)),)
OBJS += cgi_pwrmngt.o
HTMLS += html/pwrmngt.html
endif

ifneq ($(strip $(BUILD_SNMP)),)
HTMLS += html/snmpconfig.html
endif

ifneq ($(strip $(BUILD_DBUSREMOTE_TCP)),)
HTMLS += html/dbusremotecfg.html
endif

ifneq ($(strip $(BUILD_SNTP)),)
HTMLS += html/sntpcfg.html
endif

ifneq ($(strip $(BUILD_BUSYBOX_NTPD)),)
HTMLS += html/ntpdcfg.html
endif

ifneq ($(strip $(BRCM_STANDBY)),)
OBJS += cgi_standby.o 
HTMLS += html/standby.html
endif

ifneq ($(strip $(BUILD_TOD)),)
OBJS += cgi_tod.o 
HTMLS += html/todadd.html
endif

ifneq ($(strip $(BUILD_TR69C)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_TR69C_BEEP)),)
HTMLS += html/tr69*.html
endif

ifneq ($(strip $(BUILD_UPNP)),)
HTMLS += html/upnpcfg.html
endif

ifneq ($(strip $(BUILD_URLFILTER)),)
HTMLS += html/url_add.html
OBJS += cgi_urlfilter.o
endif

ifneq ($(strip $(BUILD_HOMEPLUG)),)
OBJS += cgi_homeplug.o
HTMLS += html/homeplug.html html/homeplugpassword.html
endif

ifeq ($(strip $(BUILD_DSL_SELT_TEST)),y)
HTMLS += html/seltcfg.html html/seltcfgc.html
endif

ifeq ($(strip $(BUILD_SPDSVC)), y)
HTMLS += html/speedsvc.html
OBJS += cgi_spdsvc.o
endif

ifneq ($(strip $(BUILD_XMPP)),)
HTMLS += html/xmppconn.html
OBJS += cgi_xmpp_conn.o
endif

ifeq ($(strip $(BUILD_IEEE1905)),y)
HTMLS += html/ieee1905cfg.html 
OBJS += cgi2_ieee1905.o
endif

#ifdef DMP_X_BROADCOM_COM_NFC_1
HTMLS += html/nfc.html
OBJS += cgi_nfc.o
#endif

ifneq ($(strip $(BUILD_JQPLOT)),)
ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
HTMLS += html/cpumemcharts.html
HTMLS += html/flowstats.html
endif
endif

ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
ifneq ($(strip $(BUILD_DPI)),)
HTMLS += html/dpicharts.html html/dpi.css
HTMLS += html/dpi.js
endif
endif

ifneq ($(strip $(BUILD_QRCODE)),)
OBJS += cgi_qrcode.o
CUSTOM_CFLAGS += -DSUPPORT_QRCODE
endif

ifneq ($(strip $(BUILD_OPENVSWITCH)),)
OBJS += cgi_openVS.o
HTMLS += html/openvswitchcfg.html
endif

ifeq ($(strip $(BUILD_STUN)),y)
HTMLS += html/stuncfg.html 
endif

ifneq ($(strip $(BRCM_KERNEL_BONDING)),)
ifneq ($(strip $(BUILD_DM_PURE181)),)
OBJS += cgi2_ethlag.o
HTMLS += html/cfgethlag.html
endif
endif

ifneq ($(strip $(BCM_PON)),)
OBJS += cgi_wan_select.o
HTMLS += html/wan_select.html
endif

.NOTPARALLEL:

$(eval $(call CREATE_WEB_TRG, cpall_html, html, $(INSTALL_DIR)/webs, staging, minimized_webs.tar, $(HTMLS:html/%=%)))

#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(KERNEL_LINKS_DIR)

ifeq ($(strip $(BUILD_IEEE1905)),y)
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/private/apps/ieee1905
endif

ifneq ($(strip $(BUILD_OMCI)),)
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/private/libs/omcisdk/cmninc 
endif

ifeq ($(RDPA_IFILTER_SUPPORT),y)
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/private/include
ALLOWED_INCLUDE_PATHS += $(INC_RDP_FLAGS)
endif
#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

ifneq ($(strip $(BRCM_DRIVER_WIRELESS)), )
ALLOWED_INCLUDE_PATHS +=   -I$(BUILD_DIR)/userspace/private/apps/wlan/wlmngr2 \
                           -I$(WIRELESS_DRIVER_PATH)/include \
                           -I$(BCM_FSBUILD_DIR)/private/include \
                           -I$(INC_BCMDRIVER_PATH)/include


endif


# unforunately, this custom stuff has to be put here, after the include make.common
ifneq ($(strip $(BUILD_HTTPD_SSL)),)
ifneq ($(strip $(BUILD_TR69C_BCM_SSL)),)
	LIBS += -lbcmssl -lbcmcrypto
else
	LIBS += -lssl -lcrypto -lexpat -lbcm_sslconf
	CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include
	CFLAGS += -DSUPPORT_OPENSSL
endif

else

ifneq ($(strip $(BUILD_HTTPD_BASIC_SSL)),)
ifneq ($(strip $(BUILD_TR69C_BCM_SSL)),)
	LIBS += -lbcmssl -lbcmcrypto
else
	LIBS += -lssl -lcrypto -lexpat -lbcm_sslconf
	CUSTOM_CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include
	CFLAGS += -DSUPPORT_OPENSSL
endif
endif
endif

ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS += $(SSP_MIN_COMPILER_OPTS)
LIBS += -L$(INSTALL_DIR)/lib $(SSP_LIBS) 
endif

ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
LIBS += -lhttpdshared
ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/userspace/public/libs/httpdshared
endif


LIBS += -lc -lgcc_s


# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors

extra_install:
ifneq ($(strip $(BUILD_BRCM_CPEROUTER)),)
	mkdir -p $(INSTALL_DIR)/webs/wlrouter
	(cd  $(INSTALL_DIR)/webs/wlrouter;ln -sf ../jquery-2.0.3.min.js  jquery-2.0.3.min.js)
else
	$(info "no need to install jquery-2.0.3")
endif
#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): $(OBJS)
	@echo building $@
	$(CC) $(BCM_LD_FLAGS) -o $@ $^ -Wl,-rpath,$(CMS_LIB_RPATH) $(CMS_LIB_PATH) $(LIBS)



#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)
