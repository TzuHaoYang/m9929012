#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <jansson.h>
#include <ev.h>
#include <syslog.h>
#include <getopt.h>


#include "log.h"
#include "os.h"
#include "ovsdb.h"
#include "evext.h"
#include "os_backtrace.h"
#include "json_util.h"
#include "target.h"


#include "blem.h"

/******************************************************************************/

#define MODULE_ID LOG_MODULE_ID_MAIN

/******************************************************************************/

blem_state_t g_state;

static log_severity_t blem_log_severity = LOG_SEVERITY_INFO;

/******************************************************************************
 *  PROTECTED definitions
 *****************************************************************************/

static void
blem_init()
{
    MEMZERO(g_state);
    g_state.mgr_start_handled = false;
}

/******************************************************************************
 *  PUBLIC API definitions
 *****************************************************************************/

int main(int argc, char ** argv)
{
    struct ev_loop *loop = EV_DEFAULT;

    // Parse command-line arguments
    if (os_get_opt(argc, argv, &blem_log_severity)){
        return -1;
    }
    target_log_open("BLEM", 0);
    LOGN("Starting BLEM (BLE manager)");
    log_severity_set(blem_log_severity);

    // From this point on log severity can change in runtime.
    log_register_dynamic_severity(loop);

    blem_init();

    backtrace_init();

    json_memdbg_init(loop);

    // Target dependent BLE pre init hook
    if (!target_ble_preinit(loop)) {
        return -1;
    }

    if (!target_init(TARGET_INIT_MGR_BLEM, loop)) {
        return -1;
    }

    if (!ovsdb_init_loop(loop, "BLEM")) {
        LOGE("Initializing BLEM "
             "(Failed to initialize OVSDB)");
        return -1;
    }

    if (blem_ovsdb_init()) {
        LOGE("Initializing BLEM "
             "(Failed to initialize BLEM tables)");
        return -1;
    }

    // Target dependent BLE pre run hook
    if (!target_ble_prerun(loop)) {
        return -1;
    }

    ev_run(loop, 0);

    // Exit

    target_close(TARGET_INIT_MGR_BLEM, loop);

    if (!ovsdb_stop_loop(loop)) {
        LOGE("Stopping BLEM "
             "(Failed to stop OVSDB");
    }

    ev_default_destroy();

    LOGN("Exiting BLEM");

    return 0;
}
