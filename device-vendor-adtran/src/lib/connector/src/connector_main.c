#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_main.h"
#include "cms_lan.h"

#include "connector.h"
#include "log.h"
#include "target.h"
#include "kconfig.h"

#include "connector_main.h"
#include "connector_wan.h"
#include "connector_lan.h"
#include "connector_wifi.h"
#include "connector_awlan.h"
#include "connector_ntp.h"

/******************************************************************************/

/******************************************************************************/
struct connector_ovsdb_api* connector_api_local;
struct connector_ovsdb_api* connector_api(void) { return connector_api_local; }

bool connector_init(struct ev_loop *loop, const struct connector_ovsdb_api *api)
{
    /* read settings + send callbacks */

    connector_api_local = (struct connector_ovsdb_api*)api;

    connector_ntp_config_push_cms(TARGET_DEFAULT_NTP_SERVER, true);
    connector_lan_br_config_push_ovsdb();
    connector_wan_interface_push_ovsdb();
    connector_awlan_config_push_ovsdb();
    connector_wifi_ssid_and_psk_push_ovsdb(TARGET_WIFI_2G_HOME);
    connector_wifi_ssid_and_psk_push_ovsdb(TARGET_WIFI_5G_HOME);

    /* force reset of the upnp */
    cms_upnp_set(false);
    sleep(1);

    return true;
}

bool connector_close(struct ev_loop *loop)
{
    /* Close access to your DB */
    return true;
}

bool connector_sync_mode(const connector_device_mode_e mode)
{
    /* Handle device mode */
    return true;
}

bool connector_sync_radio(const struct schema_Wifi_Radio_Config *rconf)
{
    /*
     * You can go over all radio settings or process just _changed flags to populate your DB
     *  Example: if (rconf.channel_changed) set_new_channel(rconf.channel);
     */
    return true;
}

bool connector_sync_vif(const struct schema_Wifi_VIF_Config *vconf)
{
    /*
     * You can go over all radio settings or process just _changed flags to populate your DB
     *  Example: if (vconf.ssid_changed) set_new_ssid(vconf.ssid);
     */

    if(!kconfig_enabled(CONFIG_ADTRAN_SYNC_SSID_AND_PSK)) { return true; }

    if(!strcmp(vconf->if_name, TARGET_WIFI_2G_HOME) || !strcmp(vconf->if_name, TARGET_WIFI_5G_HOME))
    {
        if(vconf->ssid_changed){ connector_wifi_ssid_push_cms(vconf); }
        if(vconf->security_changed){ connector_wifi_psk_push_cms(vconf); }
    }
    return true;
}

bool connector_sync_inet(const struct schema_Wifi_Inet_Config *iconf)
{
    /*
     * You can go over all inet settings or process just _changed flags to populate your DB
     *  Example: if (inet->inet_addr_changed) set_new_ipl(inet->inet_addr);
     */
    if(!kconfig_enabled(CONFIG_ADTRAN_SYNC_HOME_BR_CONFIG)) { return true; }

    if(!strcmp(iconf->if_name, CONFIG_TARGET_LAN_BRIDGE_NAME))
    {
        if(iconf->inet_addr_changed || iconf->netmask_changed || iconf->dhcpd_changed )
        {
            connector_lan_br_config_push_cms(iconf);
        }
    }

    return true;
}
