#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_main.h"
#include "cms_vap.h"

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
#include "connector_wifi.h"

int connector_wifi_ssid_and_psk_push_ovsdb(const char *vap)
{
    cms_ssid_t ssid;
    cms_psk_t psk;
    char radio[8] = {0};
    char cms_vap[16] = {0};
    struct schema_Wifi_VIF_Config vconfig;

    LOGI("Syncing MDM ssid/psk from datamodel to OVSDB");

    memcpy(radio, vap, 3);
    if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1)     { strscpy(cms_vap, vap, sizeof(cms_vap)); }
    else if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == 0 ){ strscpy(cms_vap, radio, sizeof(cms_vap)); }
    else { snprintf(cms_vap, sizeof(cms_vap),"%s%s%d", radio, CONFIG_BCMWL_VAP_DELIMITER, CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX); }

    LOGI("Using %s mapping (OpenSync %s == %s datamodel)", \
        (CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1) ? "deafult" : "override", \
        vap, cms_vap);

    if(cms_vap_ssid_get(cms_vap, &ssid) == -1) { goto ssid_psk_sync_failed; }
    if(cms_vap_security_get(cms_vap, &psk) == -1) { goto ssid_psk_sync_failed; }

    memset(&vconfig, 0, sizeof(vconfig));
    vconfig._partial_update = true;

    SCHEMA_SET_STR(vconfig.if_name, vap);
    SCHEMA_SET_STR(vconfig.ssid, ssid.SSID);

    SCHEMA_KEY_VAL_APPEND(vconfig.security, "encryption", "WPA-PSK");
    SCHEMA_KEY_VAL_APPEND(vconfig.security, "key", psk.keyPassphrase);
    SCHEMA_KEY_VAL_APPEND(vconfig.security, "mode", "2");
    SCHEMA_KEY_VAL_APPEND(vconfig.security, "oftag", "home--1");

    /**
     * @note since we have all the data for home AP (excluding mpsk keys) it makes no sense
     *       to lease it disabled...
     */
    SCHEMA_SET_INT(vconfig.enabled, 1);

    //upsert
    if(connector_api()->connector_vif_update_cb(&vconfig, radio) == false){ goto ssid_psk_sync_failed; }

    /**
     * @note if we configure and start home AP we must also bridge it to home br.
     */
    strexa("ovs-vsctl", "add-port", CONFIG_TARGET_LAN_BRIDGE_NAME, vconfig.if_name);

    LOGI("Syncing MDM ssid/psk from datamodel to OVSDB: OK");
    return 0;

ssid_psk_sync_failed:
    LOGW("Syncing MDM ssid/psk from datamodel to OVSDB: FAILED");
    return -1;
}

int connector_wifi_ssid_push_cms(const struct schema_Wifi_VIF_Config *vconf)
{
    cms_ssid_t ssid;
    char radio[8] = {0};
    char cms_vap[16] = {0};
    LOGI("Pushing new SSID to CMS");

    memcpy(radio, vconf->if_name, 3);
    if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1)    { strscpy(cms_vap, vconf->if_name, sizeof(cms_vap)); }
    else if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX ==0 ){ strscpy(cms_vap, radio, sizeof(cms_vap)); }
    else { snprintf(cms_vap, sizeof(cms_vap),"%s%s%d", radio, CONFIG_BCMWL_VAP_DELIMITER, CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX); }

    LOGI("Using %s mapping (OpenSync %s == %s datamodel)", \
        (CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1) ? "deafult" : "override", \
        vconf->if_name, cms_vap);

    if(cms_vap_ssid_get(cms_vap, &ssid) == -1) { goto ssid_sync_failed; }

    if(!strncmp(vconf->ssid, ssid.SSID, strlen(vconf->ssid)))
    {
        LOGI("Pushing new SSID to CMS: MOK");
        return 0;
    }

    memset(ssid.SSID, 0, sizeof(ssid.SSID));
    strscpy(ssid.SSID, vconf->ssid, sizeof(ssid.SSID));

    if(cms_vap_ssid_set(cms_vap, &ssid) == -1) { goto ssid_sync_failed; }
    if(cms_commit() == -1) { goto ssid_sync_failed; }

    LOGI("Pushing new SSID to CMS: OK");
    return 0;

ssid_sync_failed:
    LOGW("Syncing SSID from OVSDB to CMS: FAILED");
    return -1;
}

int connector_wifi_psk_push_cms(const struct schema_Wifi_VIF_Config *vconf)
{
    cms_psk_t psk;
    char radio[8] = {0};
    char cms_vap[16] = {0};
    LOGI("Pushing new PSK to CMS");

    memcpy(radio, vconf->if_name, 3);
    if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1)    { strscpy(cms_vap, vconf->if_name, sizeof(cms_vap)); }
    else if(CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX ==0 ){ strscpy(cms_vap, radio, sizeof(cms_vap)); }
    else { snprintf(cms_vap, sizeof(cms_vap),"%s%s%d", radio, CONFIG_BCMWL_VAP_DELIMITER, CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX); }

    LOGI("Using %s mapping (OpenSync %s == %s datamodel)", \
        (CONFIG_ADTRAN_SYNC_SSID_AND_PSK_REMAP_HOME_IDX == -1) ? "deafult" : "override", \
        vconf->if_name, cms_vap);

    if(cms_vap_security_get(cms_vap, &psk) == -1) { goto psk_sync_failed; }

    char *PSK = (char *)SCHEMA_KEY_VAL(vconf->security, "key");
    if(!strncmp(PSK, psk.keyPassphrase, strlen(PSK)))
    {
        LOGI("Pushing new PSK to CMS: MOK");
        return 0;
    }

    memset(psk.keyPassphrase, 0, sizeof(psk.keyPassphrase));
    strscpy(psk.keyPassphrase, PSK, sizeof(psk.keyPassphrase));

    if(cms_vap_security_set(cms_vap, &psk) == -1) { goto psk_sync_failed; }
    if(cms_commit() == -1) { goto psk_sync_failed; }

    LOGI("Pushing new PSK to MDM: OK");
    return 0;

psk_sync_failed:
    LOGW("Syncing SSID from OVSDB to CMS: FAILED");
    return -1;
}
