#include <ev.h>          // libev routines
#include <getopt.h>      // command line arguments

#include "log.h"         // logging routines
#include "json_util.h"   // json routines
#include "os.h"          // OS helpers
#include "ovsdb.h"       // OVSDB helpers
#include "target.h"      // target API

#include "csc_man.h"     // module header

/* Default log severity */
static log_severity_t  log_severity = LOG_SEVERITY_INFO;

/* Log entries from this file will contain "MAIN" */
#define MODULE_ID LOG_MODULE_ID_MAIN

/**
 * Main
 *
 * Note: Command line arguments allow overriding the log severity
 */
int main(int argc, char ** argv)
{
    struct ev_loop *loop = EV_DEFAULT;

    // Parse command-line arguments
    if (os_get_opt(argc, argv, &log_severity))
    {
        return -1;
    }

    // Initialize logging library
    target_log_open("CSC_MAN", 0);  // 0 = syslog and TTY (if present)
    LOGN("CSC_MAN");
    log_severity_set(log_severity);

    // Enable runtime severity updates
    log_register_dynamic_severity(loop);

    // Install crash handlers that dump the stack to the log file
    backtrace_init();

    // Allow recurrent json memory usage reports in the log file
    json_memdbg_init(loop);

    // Initialize target structure
    if (!target_init(TARGET_INIT_COMMON, loop))
    {
        LOGE("Initializing CSC_MAN "
             "(Failed to initialize target library)");
        return -1;
    }

    // Connect to OVSDB
    if (!ovsdb_init_loop(loop, "CSC_MAN"))
    {
        LOGE("Initializing CSC_MAN "
             "(Failed to initialize OVSDB)");
        return -1;
    }

    csc_man_cert_init();

    // Register to relevant OVSDB tables events
    if (csc_man_ovsdb_init())
    {
        LOGE("Initializing CSC_MAN "
             "(Failed to initialize CSC_MAN tables)");
        return -1;
    }

    // Initialize CSC Manager motion
    if (csc_man_motion_init(loop))
    {
        LOGE("Initializing CSC_MAN "
             "(Failed to initialize CSC_MAN motion)");
        return -1;
    }

    // Initialize data pipeline
    if (!dpp_init())
    {
        LOGE("Initializing CSC_MAN "
             "(Failed to initialize DPP library)");
        return -1;
    }

#ifdef CSC_MAN_AUTOSTART_MOTION
    wait_mqtt_headers();
#endif
    read_ovsdb_config();
    write_ovsdb_state();

    // Start the event loop
    ev_run(loop, 0);

    target_close(TARGET_INIT_COMMON, loop);

    if (!ovsdb_stop_loop(loop))
    {
        LOGE("Stopping CSC_MAN "
             "(Failed to stop OVSDB)");
    }

    ev_loop_destroy(loop);

    LOGN("Exiting CSC_MAN");

    return 0;
}
