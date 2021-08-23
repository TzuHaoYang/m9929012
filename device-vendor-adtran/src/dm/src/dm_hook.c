#include <stdbool.h>
#include <ev.h>
#include <mosquitto.h>

#include "dm.h"
#include "dm_test.h"
#include "dm_config.h"

#include "dm_reboot_trigger.h"

// Optional DM hook - placeholder for vendor override

bool dm_hook_init(struct ev_loop *loop)
{
#if defined (TARGET_SR400AC) || defined (TARGET_834_5)
        dm_reboot_trigger_init(loop);
#else
    (void)loop;

    mosquitto_lib_init();

    /* configuration system initialization */
    init_config();

    dm_config_monitor();
#endif
    return true;
}

bool dm_hook_close()
{
    return true;
}
