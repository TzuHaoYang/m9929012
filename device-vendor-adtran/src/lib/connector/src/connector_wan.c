#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_wan.h"

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
#include "connector_wan.h"

static ovsdb_table_t table_Connection_Manager_Uplink;

static int connector_wan_interface_push_vif_interface_ovsdb(char *wan_if)
{
    struct schema_Wifi_Inet_Config tmp;

    LOGI("Setting WAN interface to VIF_Config table");

    memset(&tmp, 0, sizeof(tmp));
    SCHEMA_SET_INT(tmp.NAT, 1);
    SCHEMA_SET_INT(tmp.enabled, 1);
    SCHEMA_SET_INT(tmp.network, 0);
    SCHEMA_SET_STR(tmp.if_name, wan_if);
    SCHEMA_SET_STR(tmp.if_type, "bridge");

    LOGI("Upserting WAN: %s", tmp.if_name);

    if(connector_api()->connector_inet_update_cb(&tmp) == false){ goto wan_interface_set_err; }

    LOGI("Setting WAN interface to VIF_Config table: OK");
    return 0;

wan_interface_set_err:
    LOGI("Setting WAN interface to VIF_Config table: FAILED");
    return -1;
}

static int connector_wan_interface_push_manager_uplink_ovsdb(char *wan_if)
{
    struct schema_Connection_Manager_Uplink tmp;

    LOGI("Setting WAN uplink");
    OVSDB_TABLE_INIT(Connection_Manager_Uplink, if_name);

    memset(&tmp, 0, sizeof(tmp));
    SCHEMA_SET_INT(tmp.has_L2, 1);
    SCHEMA_SET_INT(tmp.has_L3, 1);
    SCHEMA_SET_INT(tmp.is_used, 1);
    strscpy(tmp.if_name, wan_if, sizeof(tmp.if_name));
    strscpy(tmp.if_type, "eth", sizeof(tmp.if_type));

    LOGI("Upserting uplink WAN: %s(%s)", tmp.if_name, tmp.if_type);
    if(ovsdb_table_upsert(&table_Connection_Manager_Uplink, &tmp, false) == false){ goto wan_uplink_set_err; }

    LOGI("Setting WAN uplink: OK");
    return 0;

wan_uplink_set_err:
    LOGW("Setting WAN uplink: FAILED");
    return -1;
}

int connector_wan_interface_push_ovsdb(void)
{
    int err = 0;
    char wan_if[16] = {0};
    LOGI("Setting WAN");

    if(cms_active_wan_interface_get(wan_if, sizeof(wan_if)) == -1){ return -1; }

    err |= connector_wan_interface_push_manager_uplink_ovsdb(wan_if);
    err |= connector_wan_interface_push_vif_interface_ovsdb(wan_if);

    return err;
}
