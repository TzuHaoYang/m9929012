#include <stdio.h>
#include <unistd.h>

#include "ovsdb_update.h"
#include "ovsdb_sync.h"
#include "ovsdb_table.h"
#include "schema.h"

#include "csc_man.h"

/* Log entries from this file will contain "OVSDB" */
#define MODULE_ID LOG_MODULE_ID_OVSDB

static ovsdb_table_t table_Node_Config;
static ovsdb_table_t table_Node_State;
static ovsdb_table_t table_SSL;

#ifdef CSC_MAN_AUTOSTART_MOTION
static ovsdb_table_t table_AWLAN_Node;
#endif

void read_ovsdb_config(void) {
    int count;
    void *node_config_p;

    node_config_p = ovsdb_table_select(&table_Node_Config, "module", MOTION_OVSDB_MODULE, &count);
    if (node_config_p == NULL)
        count = 0;
    LOGI("%s: Node_Config "MOTION_OVSDB_MODULE" module count: %d", __func__, count);

#ifdef CONFIG_CSC_LIBUCI
    int i;
    int driver_en = -1;
    unsigned val;
    struct schema_Node_Config *node_config;
    for (i = 0; i < count; i++) {
        node_config = (struct schema_Node_Config *)(node_config_p + table_Node_Config.schema_size*i);
        if (node_config->key_exists && !strcmp(node_config->key, MOTION_OVSDB_DRIVER_EN_KEY)) {
            if (!node_config->value_exists || sscanf(node_config->value, "%u", &val) != 1) {
                LOGE("Node_Config "MOTION_OVSDB_DRIVER_EN_KEY": not a number");
                driver_en = -1;
                break;
            }
            val = !!val;
            if (driver_en >= 0) {
                if ((int)val != driver_en) {
                    LOGE("Node_Config "MOTION_OVSDB_DRIVER_EN_KEY": 2 conflicting values");
                    driver_en = -1;
                    break;
                }
            } else {
                driver_en = (int)val;
            }
        }
    }
#endif

#ifdef CSC_MAN_AUTOSTART_MOTION
    LOGI("CSC_MAN_AUTOSTART_MOTION");
    count = 1;
#endif

#ifdef CONFIG_CSC_LIBUCI
    // if driver_en not set, don't change uci config
    if (driver_en >= 0) {
        LOGI("%s "MOTION_OVSDB_DRIVER_EN_KEY": %d", __func__, driver_en);

        if (driver_enabled_platform != driver_en)
            write_uci(driver_en);

        if (!driver_en)
            count = 0;
    }
#endif

    if (count)
        start_motion();
    else
        stop_motion();

    if (node_config_p)
        free(node_config_p);
}

static void callback_Node_Config(
        ovsdb_update_monitor_t *mon,
        struct schema_Node_Config *old_rec,
        struct schema_Node_Config *conf)
{
    // check if any MOTION_OVSDB_MODULE rows changed and read full table to count MOTION_OVSDB_MODULE entries
    if ((conf->module_exists && !strcmp(MOTION_OVSDB_MODULE, conf->module)) ||
        (mon->mon_type == OVSDB_UPDATE_MODIFY && old_rec->module_exists && !strcmp(MOTION_OVSDB_MODULE, old_rec->module)))
        read_ovsdb_config();
}

static void callback_SSL(
        ovsdb_update_monitor_t *mon,
        struct schema_SSL *old_rec,
        struct schema_SSL *ssl)
{
    if (cert_ready || !ssl->ca_cert_exists || !ssl->certificate_exists || !ssl->private_key_exists)
        return;

    strncpy(ca_file, ssl->ca_cert, sizeof(ca_file));
    strncpy(cert_file, ssl->certificate, sizeof(cert_file));
    strncpy(key_file, ssl->private_key, sizeof(key_file));
    csc_man_cert_init();

    if (cert_ready && motion_stack_ready && !motion_stack_started)
        start_motion();
}

void write_ovsdb_state(void) {
    json_t *where;
    json_t *cond;
    struct schema_Node_State node_state;
    char value[] = "0 (need-reboot)";
    value[0] += driver_enabled_platform;
    if (driver_enabled_platform == driver_enabled_boot)
        value[1] = '\0';

    LOGI("%s "MOTION_OVSDB_DRIVER_EN_KEY": %s", __func__, value);

    where = json_array();

    cond = ovsdb_tran_cond_single("module", OFUNC_EQ, MOTION_OVSDB_MODULE);
    json_array_append_new(where, cond);

    cond = ovsdb_tran_cond_single("key", OFUNC_EQ, MOTION_OVSDB_DRIVER_EN_KEY);
    json_array_append_new(where, cond);

    MEMZERO(node_state);
    SCHEMA_SET_STR(node_state.module, MOTION_OVSDB_MODULE);
    SCHEMA_SET_STR(node_state.key, MOTION_OVSDB_DRIVER_EN_KEY);
    SCHEMA_SET_STR(node_state.value, value);
    SCHEMA_SET_INT(node_state.persist, 1);
    ovsdb_table_upsert_where(&table_Node_State, where, &node_state, 0);

    return;
}

#ifdef CSC_MAN_AUTOSTART_MOTION
void wait_mqtt_headers(void) {
    struct schema_AWLAN_Node awlan_node;
    LOGI(__func__);
    while(!ovsdb_table_select_one_where(&table_AWLAN_Node, json_array(), &awlan_node) || awlan_node.mqtt_headers_len < 2)
        sleep(3);
}
#endif

int csc_man_ovsdb_init(void) {
    LOGI("Initializing Cognitive Manager tables");

    // Initialize OVSDB tables
    OVSDB_TABLE_INIT_NO_KEY(Node_Config);
    OVSDB_TABLE_INIT_NO_KEY(Node_State);
#ifdef CSC_MAN_AUTOSTART_MOTION
    OVSDB_TABLE_INIT_NO_KEY(AWLAN_Node);
#endif

    // Initialize OVSDB monitor callbacks
    OVSDB_TABLE_MONITOR_F(Node_Config, ((char*[]){"module", "key", "value", NULL}));

    if (!cert_ready) {
        OVSDB_TABLE_INIT_NO_KEY(SSL);
        OVSDB_TABLE_MONITOR_F(SSL, ((char*[]){"ca_cert", "certificate", "private_key", NULL}));
    }

    return 0;
}
