#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ev.h>

#include "log.h"
#include "os_socket.h"
#include "os_backtrace.h"
#include "ovsdb.h"
#include "schema.h"
#include "jansson.h"
#include "monitor.h"
#include "json_util.h"
#include "target.h"
#include "const.h"

#include "dm_test.h"

#define MODULE_ID LOG_MODULE_ID_MAIN

#define DM_CFG_TBL_MAX 1
#define OVSDB_DEF_TABLE                 "Open_vSwitch"
#define LEGACY_INSTALL_PREFIX           "/usr/plume"

/*
 * All the context information for DM
 */
typedef struct dm_ctx_s {
    int monitor_id;
} dm_ctx_t;

static dm_ctx_t g_dm_ctx;

/*
 *The ovsdb tables that dm cares about
 */

static char *dm_cfg_tbl_names [] =
{
    "Wifi_Test_Config",
    "Wifi_Test_State",
};

static dm_cfg_table_parser_t * dm_cfg_table_handlers [DM_CFG_TBL_MAX] = {
    dm_execute_command_config
};

static int dm_cfg_extract_table(json_t *value)
{
    int tbl_idx;
    dm_cfg_table_parser_t *handler = NULL;

    for (tbl_idx = 0; tbl_idx < DM_CFG_TBL_MAX; tbl_idx++) {
        json_t *tbl = NULL;
        if ((tbl = json_object_get(value, dm_cfg_tbl_names[tbl_idx]))) {
            handler = dm_cfg_table_handlers[tbl_idx];
            handler(tbl);
        }
    }
    if (!handler) {
        LOG(NOTICE, "can't find table\n");
        return -1;
    }

    return 0;
}

static void dm_cfg_update_cb(int id, json_t *js, void * data)
{
    json_t *jparams = NULL;
    size_t index;
    json_t *value = NULL;

    if (id != g_dm_ctx.monitor_id) {
        LOG(NOTICE,"mon id mismatch!\n");
        return;
    }

    jparams = json_object_get(js, "params");
    json_array_foreach(jparams, index, value) {
        if (index == 0) {
            if (!json_is_integer(value) || id != json_integer_value(value)) {
                goto _error_exit;
            }
        } else if (json_is_object(value)) {
            dm_cfg_extract_table(value);
        } else {
            LOG(NOTICE,"can't find obj\n");
        }
    }

_error_exit:
    if (jparams)
        json_decref(jparams);
    if (value)
        json_decref(value);

    return;
}

static void dm_monitor_req_cb(int id, bool is_error, json_t *js, void * data)
{
    const char *key;
    json_t *value;
    int tbl_idx;
    dm_cfg_table_parser_t *handler;

    LOG(NOTICE,"monitor reply id %d \n", id);

    if (!json_is_object(js))
        return;

    /* need to parse the existing table info */
    json_object_foreach(js, key, value) {
        for (tbl_idx = 0; tbl_idx < DM_CFG_TBL_MAX; tbl_idx++) {
            if (!strcmp(key, dm_cfg_tbl_names[tbl_idx])) {
                handler = dm_cfg_table_handlers[tbl_idx];
                handler(value);
            }
        }
    }
}

int dm_config_monitor()
{
    int i, mon_id;
    int ret = -1;
    json_t *jparams = NULL;
    json_t *tbls = NULL;

    mon_id = ovsdb_register_update_cb(dm_cfg_update_cb, NULL);
    if (mon_id < 1) {
        LOG(NOTICE,"dm register update cb failed\n");
        return -1;
    }
    g_dm_ctx.monitor_id = mon_id;

    jparams = json_array();
    if (!jparams || json_array_append_new(jparams, json_string(OVSDB_DEF_TABLE))) {
        goto _error_exit;
    }
    if (json_array_append_new(jparams, json_integer(mon_id))) {
        goto _error_exit;
    }

     /* monitor all selects
     *  "tableName" : { "select" : {
     *                   "initial" : true,
     *                   "insert" : true,
     *                   "delete" : true,
     *                   "modify" : true
     *                }
     *              }
     */
    tbls = json_object();
    for (i = 0; i < DM_CFG_TBL_MAX; i++) {
        json_t *tbl;

        tbl = json_pack("{s:{s:b, s:b, s:b, s:b}}", "select",
                "initial", true,
                "insert" , true,
                "delete" , true,
                "modify" , true);
        if (!tbl || json_object_set_new(tbls, dm_cfg_tbl_names[i], tbl)) {
            goto _error_exit;
        }
    }

    if (json_array_append_new(jparams, tbls)) {
        goto _error_exit;
    }
    /* this routine steals ref of jparams and decref afterward, not need to decref
     * unless set_new fails
     */
    ret = ovsdb_method_send(dm_monitor_req_cb, NULL, MT_MONITOR, jparams);

    return ret?0:-1;

_error_exit:
    if (tbls)
        json_decref(tbls);
    if (jparams)
        json_decref(jparams);

    return -1;

}

json_t* dm_get_test_cfg_command_config(json_t *jtbl)
{
    json_t *jcfg = NULL;
    json_t *ival = NULL;
    void   *iter = NULL;
    const char *key;

    json_object_foreach(jtbl, key, ival) {
        if ((iter = json_object_iter_at(ival,"new")) != NULL){
            break;
        } else if (json_object_iter_at(ival,"old")) {
            continue;
        }
    }

    jcfg = json_object_get(ival, "new");
    if (!jcfg) {
        LOG(NOTICE,"can't obtain cfg \n");
    }
    return jcfg;
}
/*
 * Wifi_Test_State table insert callback, called upon failed or successful
 * insert operation
 */
void insert_wifi_test_state_cb(int id, bool is_error, json_t *msg, void * data)
{

    (void)id;
    (void)is_error;
    (void)data;
    (void)msg;
    char * str;
    json_t * uuids = NULL;
    size_t index;
    json_t *value;
    json_t *oerr;

    str = json_dumps(msg, 0);
    LOG(NOTICE, "insert json response: %s\n", str);
    json_free(str);


    /* response itself is an array, extract it from response array  */
    if (1 == json_array_size(msg))
    {
        uuids = json_array_get(msg, 0);

        uuids = json_object_get(uuids, "uuid");

        if (NULL != uuids)
        {
            str = json_dumps (uuids, 0);
            LOG(NOTICE, "Wifi_Test_State::uuid=%s", str);
            json_free(str);
        }

    }
    else
    {
        /* array longer then 1 means an error occurred          */
        /* process response & try to extract response error     */
        json_array_foreach(msg, index, value)
        {
            if (json_is_object(value))
            {
                oerr = json_object_get(value, "error");
                if (NULL != oerr)
                {
                    LOG(ERR, "OVSDB error::msg=%s", json_string_value(oerr));
                }

                oerr = json_object_get(value, "details");
                if (NULL != oerr)
                {
                    LOG(ERR, "OVSDB details::msg=%s", json_string_value(oerr));
                }
            }
        }

    }

}
/*
 * Try to fill in Wifi_Test_State table
 */
bool wifi_test_state_fill_entity (const char *p_test_id, const char *p_state)
{
    bool retval = false;
    struct schema_Wifi_Test_State s_wifi_test_state;

    memset(&s_wifi_test_state, 0, sizeof(struct schema_Wifi_Test_State));

    STRSCPY(s_wifi_test_state.test_id, p_test_id);
    STRSCPY(s_wifi_test_state.state, p_state);

    LOG(NOTICE, "Test State::test_id=%s|State=%s",
                                         s_wifi_test_state.test_id,
                                         s_wifi_test_state.state);

    retval = ovsdb_tran_call(insert_wifi_test_state_cb,
                             NULL,
                             "Wifi_Test_State",
                             OTR_INSERT,
                             NULL,
                             schema_Wifi_Test_State_to_json(&s_wifi_test_state, NULL));

    return retval;
}

int dm_execute_command_config (json_t *jtbl)
{
    json_t *jcfg;
    json_t *jmap;
    json_t *jtest_id;

    const char*  path = NULL;
    const char*  arg = NULL;
    const char*  log = NULL;
    const char*  cmd = NULL;
    const char*  test_id = NULL;
    int ret = -1;
    int fd;

    char *command = NULL;

    if (!(jcfg = dm_get_test_cfg_command_config(jtbl))) {
        return -1;
    }

    // TEST ID
    jtest_id = json_object_get(jcfg, "test_id");
    if (jtest_id == NULL)
    {
        LOG(ERR, "DM TEST: Missing 'test_id' object.");
        goto _error_exit;
    }

    test_id = json_string_value(jtest_id);
    if (test_id == NULL)
    {
        LOG(ERR, "DM TEST: 'test_id' is not a string.");
        goto _error_exit;
    }
    LOG(DEBUG,"test_id: %s\n", test_id);

    // PARAM
    jmap = json_object_get(jcfg, "params");
    LOG(NOTICE,"params %s\n", json_dumps_static(jmap,JSON_ENSURE_ASCII));

    if (json_is_array(jmap)) {
       json_t *tmpjmap = NULL;
       size_t i=0,j=0,k=0;
       json_t *jst1,*jst2;
       json_array_foreach(jmap, i, tmpjmap) {
         if (json_is_array(tmpjmap)) {
            json_array_foreach(tmpjmap, j, jst1) {
               json_array_foreach(jst1, k, jst2) {
                  const char *value = json_string_value(jst2);
                  if (!strcmp(value,"arg")) {
                     arg = json_string_value(json_array_get(jst1,(k+1)));
                     LOG(NOTICE,"key:  %s\n",json_string_value(jst2));
                     LOG(NOTICE,"value: %s\n",arg);
                  }
                  else if (!strcmp(value,"path")) {
                     path = json_string_value(json_array_get(jst1,(k+1)));
                     LOG(NOTICE,"key:  %s\n",json_string_value(jst2));
                     LOG(NOTICE,"value: %s\n",path);
                  }
                  else if (!strcmp(value,"log")) {
                     log = json_string_value(json_array_get(jst1,(k+1)));
                     LOG(NOTICE,"key:  %s\n",json_string_value(jst2));
                     LOG(NOTICE,"value: %s\n",log);
                  }
                  else if (!strcmp(value,"cmd")) {
                     cmd = json_string_value(json_array_get(jst1,(k+1)));
                     LOG(NOTICE,"key:  %s\n",json_string_value(jst2));
                     LOG(NOTICE,"value: %s\n",cmd);
                  }
                  else {
                     LOG(NOTICE,"Undefined key:  %s\n",json_string_value(jst2));
                  }
               }
            }
         }
       }
    }

    // Execute defined target action or system call
    if (   cmd != NULL
        && (strcmp(cmd, SCHEMA_CONSTS_DM_CMD_EXECUTE))
       )
    {
        if (target_device_execute(cmd)) {
            wifi_test_state_fill_entity(test_id,"RUNNING");
            return true;
        }
        return false;
    }

    if (path == NULL)
    {
        LOG(ERR, "DM TEST: No path specified.");
        goto _error_exit;
    }

    // Replace legacy install prefix with actual install prefix
    char new_path[256];
    int len = strlen(LEGACY_INSTALL_PREFIX);
    if (strcmp(LEGACY_INSTALL_PREFIX, CONFIG_INSTALL_PREFIX) != 0) {
        if (strncmp(path, LEGACY_INSTALL_PREFIX, len) == 0) {
            snprintf(new_path, sizeof(new_path), "%s%s", CONFIG_INSTALL_PREFIX, path + len);
            path = new_path;
        }
    }

    char *argv[128];
    char **pargv;

    pargv = argv;
    /* The first argument is always the executable path */
    *pargv++ = (char *)path;

    if (arg != NULL)
    {
        command = strdup(arg);
        if (command == NULL)
        {
            LOG(ERR, "Unable to allocate command.");
            goto _error_exit;
        }

        char *pcmd = command;
        char *p;

        /* Split command into argument list separated by spaces */
        while ((p = strsep(&pcmd, " ")) != NULL)
        {
            *pargv++ = p;

            if (pargv == (argv + ARRAY_LEN(argv)))
            {
                LOG(ERR, "DM TEST: Argument list too long: %s\n", arg);
                goto _error_exit;
            }
        }

    }

    *pargv++ = NULL;

    /* logfile for test results */
    char logfile[256];
    snprintf(logfile, sizeof(logfile), "/tmp/test_id_%s.log", test_id);

    LOG(NOTICE, "DM TEST: Executing test_id:%s path:%s arg:\"%s\" logfile:%s",
            test_id,
            path,
            arg == NULL ? "NULL" : arg,
            logfile);

    /* Verify that path is an executable */
    if (access(path, X_OK) != 0)
    {
        LOG(ERR, "DM TEST: Path '%s' is not an executable.", path);
        goto _error_exit;
    }

    if (fork() == 0)
    {
        /* Do a double fork() so we disown the process */
        if (fork() != 0) exit(1);

        fd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        dup2(fd,1);
        dup2(fd,2);
        close(fd);
        /* child process */
        execv(path, argv);
        /* Just in case execv() fails */
        exit(1);
    }

    wifi_test_state_fill_entity(test_id,"RUNNING");

_error_exit:
    if (command != NULL) free(command);

    return ret;

}
