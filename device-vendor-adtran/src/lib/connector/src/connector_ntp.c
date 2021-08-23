#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "cms_main.h"
#include "cms_ntp.h"

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
#include "connector_ntp.h"

int connector_ntp_config_push_cms(char *ntp_server, bool enable)
{
    cms_time_t time;
    LOGI("Pushing new NTP to CMS");

    if(cms_ntp_config_get(&time) == -1) { goto ntp_to_cms_sync_failed; }

    if(time.enable == enable)
    {
        if((enable == false) || \
            ((enable == true) && (strncmp(time.NTPServer1, ntp_server, strlen(ntp_server)))))
        {
            LOGI("Pushing NTP enable to CMS: MOK");
            return 0;
        }
    }

    time.enable = enable;
    strscpy(time.NTPServer1, ntp_server, sizeof(time.NTPServer1));

    if(cms_ntp_config_set(&time) == -1) { goto ntp_to_cms_sync_failed; }
    if(cms_commit() == -1) { goto ntp_to_cms_sync_failed; }

    LOGI("Pushing new NTP to CMS: OK");
    return 0;

ntp_to_cms_sync_failed:
    LOGW("Syncing NTP from OVSDB to CMS: FAILED");
    return -1;
}
