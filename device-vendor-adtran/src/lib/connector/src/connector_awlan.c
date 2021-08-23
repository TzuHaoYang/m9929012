#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_main.h"
#include "cms_entity.h"
#include "cms_opensync.h"

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
#include "connector_awlan.h"

static ovsdb_table_t table_AWLAN_Node;

int connector_awlan_config_push_ovsdb(void)
{
    cms_opensync_t opensync;

    struct schema_AWLAN_Node tmp;

    LOGI("Setting AWLAN data");

    OVSDB_TABLE_INIT_NO_KEY(AWLAN_Node);

    if(cms_opensync_data_get(&opensync) == -1) { return -1; }
    memset(&tmp, 0, sizeof(tmp));
    SCHEMA_SET_STR(tmp.device_mode, opensync.mode);
    SCHEMA_SET_STR(tmp.redirector_addr, opensync.URL);

    LOGI("Setting AWLAN data, mode: %s, URL: %s", tmp.device_mode, opensync.URL);

    char *filter[] = { "+", \
                        SCHEMA_COLUMN(AWLAN_Node, device_mode), \
                        SCHEMA_COLUMN(AWLAN_Node, redirector_addr), \
                        NULL };

    if(ovsdb_table_update_where_f(&table_AWLAN_Node, NULL, &tmp, filter)== false){ goto opensync_set_err; }

    LOGI("Setting AWLAN data: OK");
    return 0;

opensync_set_err:
    LOGW("Setting AWLAN data: FAILED");
    return -1;
}
