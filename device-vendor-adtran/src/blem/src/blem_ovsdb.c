#include <stdlib.h>
#include <sys/wait.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <inttypes.h>
#include <jansson.h>

#include "log.h"
#include "schema.h"
#include "ovsdb_cache.h"
#include "ovsdb_table.h"
#include "ovsdb_sync.h"
#include "target.h"

#include "blem.h"


#define MODULE_ID LOG_MODULE_ID_MAIN


/*****************************************************************************/


extern blem_state_t g_state;

ovsdb_table_t table_AW_Bluetooth_Config;
ovsdb_table_t table_AW_Bluetooth_State;

/*****************************************************************************/

bool AW_Bluetooth_differs(struct schema_AW_Bluetooth_Config *config)
{
    // Note that we return true on every error since we cannot be
    // sure that tables are the same.
    json_t *where   = json_array();
    json_t *table   = ovsdb_sync_select_where(SCHEMA__AW_Bluetooth_State, where);
    json_t *row     = json_array_get(table, 0);
    json_decref(where);

    if (table == NULL) {
        LOGN("Unable to read AW_Bluetooth_State!");
        return true;
    }

    if (row == NULL) {
        json_decref(table);
        LOGN("Table AW_Bluetooth_State empty!");
        return true;
    }

    struct schema_AW_Bluetooth_State state;
    bool ret = schema_AW_Bluetooth_State_from_json(&state, row, false, NULL);
    json_decref(table);

    if (!ret) {
        LOGI("Unable to parse AW_Bluetooth_State row!");
        return true;
    }
    /*
    if (strcmp(config->command, state.command)    ||
        strcmp(config->mode,    state.mode)       ||
        strcmp(config->payload, state.payload)    ||
        config->txpower         != state.txpower  ||
        config->interval_millis != state.interval_millis ||
        config->connectable_exists != state.connectable_exists ||
        config->connectable     != state.connectable) {
        LOGD("Inconsistent state of tables detected.");
        return true;
    }
    */
    if (strcmp(config->command, state.command)    ||
        strcmp(config->mode,    state.mode)       ||
        strcmp(config->payload, state.payload)    ||
        config->txpower         != state.txpower  ||
        config->interval_millis != state.interval_millis) {
        LOGD("Inconsistent state of tables detected.");
        return true;
    }

    return false;
}

void callback_AW_Bluetooth_Config(ovsdb_update_monitor_t *mon,
        struct schema_AW_Bluetooth_Config *old_rec,
        struct schema_AW_Bluetooth_Config *config)
{
    // Initial state shall be applied even if the same in the OVSDB as BLE driver is currently not yet
    // configured, but for every following DB change update can be ignored if nothing changed.
    if (g_state.mgr_start_handled)
    {
        if (!AW_Bluetooth_differs(config))
        {
            LOGN("Ignoring OVSDB trigger due to consistent state of tables.");
            return;
        }
    }
    else
    {
        g_state.mgr_start_handled = true;
    }

    if (config->mode && !strcmp(config->mode, "on"))
    {
        target_ble_broadcast_start(config);
    }
    else
    {
        target_ble_broadcast_stop();
    }

    // Update state table
    if (!ovsdb_table_upsert(&table_AW_Bluetooth_State, config, false))
    {
        LOGE("Unable to update AW_Bluetooth_State table!");
    }

    return;
}

int blem_ovsdb_init()
{
    LOGI("Initializing BLEM tables");

    // Initialize OVSDB tables
    OVSDB_TABLE_INIT_NO_KEY(AW_Bluetooth_Config);
    OVSDB_TABLE_INIT_NO_KEY(AW_Bluetooth_State);

    // Initialize OVSDB monitor callbacks
    OVSDB_TABLE_MONITOR(AW_Bluetooth_Config, false);

    return 0;
}

