#include <stdio.h>
#include <stdbool.h>
#include <wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "os_types.h"
#include "os_util.h"
#include "util.h"
#include "os.h"
#include "log.h"
#include "target.h"
#include "const.h"
#if defined(CONFIG_TARGET_WATCHDOG) && defined(CONFIG_WPD_ENABLED)
bool target_device_wdt_ping(void)
{
    char *wdt_cmd = "[ -x "CONFIG_INSTALL_PREFIX"/bin/wpd ] && "CONFIG_INSTALL_PREFIX"/bin/wpd --ping";

    return target_device_execute(wdt_cmd);
}
#endif /* defined(CONFIG_TARGET_WATCHDOG) && defined(CONFIG_WPD_ENABLED) */