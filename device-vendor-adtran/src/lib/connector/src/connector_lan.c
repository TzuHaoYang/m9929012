#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_main.h"
#include "cms_lan.h"

#include "connector.h"
#include "os_types.h"
#include "os_util.h"
#include "os_nif.h"
#include "util.h"
#include "os.h"
#include "target.h"
#include "log.h"
#include "const.h"
#include "ovsdb.h"
#include "ovsdb_update.h"
#include "ovsdb_sync.h"
#include "ovsdb_table.h"
#include "schema.h"

#include "connector_main.h"
#include "connector_lan.h"

int connector_lan_br_config_push_ovsdb(void)
{
    int ip[4];
    bool upnp;
    cms_lan_t lan;
    cms_ipv4_t ipv4;
    struct schema_Wifi_Inet_Config tmp;

    LOGI("Setting LAN interface/dhcp to ovsdb");

    if(cms_dhcp_server_pool_get(&lan) == -1){ goto lan_set_err_ovsdb; }
    if(cms_ipv4_address_get(CONFIG_TARGET_LAN_BRIDGE_NAME, &ipv4) == -1){ goto lan_set_err_ovsdb; }
    if(cms_upnp_get(&upnp) == -1){ goto lan_set_err_ovsdb; }

    if(strcmp(lan.IPRouters, ipv4.IPAddress)){ LOGW("IP mismatch detected (dhcp pool/br ip): %s/%s", lan.IPRouters, ipv4.IPAddress); }

    if(sscanf(lan.IPRouters, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) { goto lan_set_err_ovsdb; }

    memset(&tmp, 0, sizeof(tmp));
    SCHEMA_SET_INT(tmp.enabled, 1);
    SCHEMA_SET_INT(tmp.network, 1);
    SCHEMA_SET_STR(tmp.if_name, CONFIG_TARGET_LAN_BRIDGE_NAME);
    SCHEMA_SET_STR(tmp.if_type, "bridge");
    SCHEMA_SET_STR(tmp.upnp_mode, (upnp) ? "internal" : "disabled");
    SCHEMA_SET_STR(tmp.ip_assign_scheme, "static");
    SCHEMA_SET_STR(tmp.inet_addr, lan.IPRouters);
    SCHEMA_SET_STR(tmp.netmask, lan.subnetMask);
    SCHEMA_SET_STR(tmp.broadcast, strfmta("%d.%d.%d.255", ip[0], ip[1], ip[2]));
    SCHEMA_KEY_VAL_APPEND(tmp.dhcpd, "start", lan.minAddress);
    SCHEMA_KEY_VAL_APPEND(tmp.dhcpd, "stop", lan.maxAddress);
    SCHEMA_KEY_VAL_APPEND(tmp.dhcpd, "lease_time", strfmta("%dh", lan.leaseTime/3600));
    SCHEMA_KEY_VAL_APPEND(tmp.dhcpd, "dhcp_option", strfmta("3,%s;6,%s", lan.IPRouters, lan.IPRouters));
    SCHEMA_KEY_VAL_APPEND(tmp.dhcpd, "force", "false");

    LOGI("Upserting LAN: %s(%s)", tmp.if_name, lan.interface);

    if(connector_api()->connector_inet_update_cb(&tmp) == false){ goto lan_set_err_ovsdb; }

    LOGI("Setting LAN interface to ovsdb: OK");
    return 0;

lan_set_err_ovsdb:
    LOGW("Setting LAN interface to ovsdb: FAILED");
    return -1;
}

int connector_lan_br_config_push_cms(const struct schema_Wifi_Inet_Config *inet)
{
    cms_lan_t lan;
    cms_ipv4_t ip;
    struct schema_Wifi_Inet_Config tmp;

    LOGI("Setting LAN interface/dhcp to CMS");

    if(cms_dhcp_server_pool_get(&lan) == -1){ goto lan_set_err_cms; }
    if(cms_ipv4_address_get(CONFIG_TARGET_LAN_BRIDGE_NAME, &ip) == -1){ goto lan_set_err_cms; }

    if(strcmp(lan.IPRouters, inet->inet_addr) || strcmp(lan.subnetMask, inet->netmask) || \
       strcmp(lan.minAddress, (char *)SCHEMA_KEY_VAL(inet->dhcpd, "start")) || \
       strcmp(lan.maxAddress, (char *)SCHEMA_KEY_VAL(inet->dhcpd, "stop"))) {}
    else
    {
        LOGI("Setting LAN interface/dhcp to CMS: MOK");
        return 0;
    }

    strscpy(lan.IPRouters, inet->inet_addr, sizeof(lan.IPRouters));
    strscpy(lan.subnetMask, inet->netmask, sizeof(lan.subnetMask));
    strscpy(lan.minAddress, (char *)SCHEMA_KEY_VAL(inet->dhcpd, "start"), sizeof(lan.minAddress));
    strscpy(lan.maxAddress, (char *)SCHEMA_KEY_VAL(inet->dhcpd, "stop"), sizeof(lan.maxAddress));
    //TODO: lan.leaseTime

    LOGI("Upserting LAN: %s(%s)", tmp.if_name, lan.interface);
    if(cms_dhcp_server_pool_set(&lan) == -1){ goto lan_set_err_cms; }

    strscpy(ip.IPAddress, inet->inet_addr, sizeof(ip.IPAddress));
    strscpy(ip.subnetMask, inet->netmask, sizeof(ip.subnetMask));

    LOGI("Upserting IP: %s(%s)", ip.IPAddress, ip.subnetMask);
    if(cms_ipv4_address_set(CONFIG_TARGET_LAN_BRIDGE_NAME, &ip) == -1){ goto lan_set_err_cms; }

    if(cms_commit() == -1) { goto lan_set_err_cms; }

    LOGI("Setting LAN interface to CMS: OK");
    return 0;

lan_set_err_cms:
    LOGW("Setting LAN interface to CMS: FAILED");
    return -1;
}

