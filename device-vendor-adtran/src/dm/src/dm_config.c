#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ev.h>

#include "log.h"

#include "config_pjs.h"
#include "pjs_gen_h.h"
#include "config_pjs.h"
#include "pjs_gen_c.h"

#include "dm.h"
#include "dm_config.h"

#define SKIP_ONFAIL(result, function)   result = result && function

/* declarations */
static bool config_open();
static bool config_apply();

/* hard-coded file path */
static const char *conf_path = CONFIG_INSTALL_PREFIX"/bin/conf.json";

/* dm configuration structure - perfect match to json config file   */
static struct config conf;
static ev_stat wconf;


/* file change callback function */
static void cb_config(EV_P_ ev_stat * w, int revents)
{
    bool success = true;

    /* trigger re-read configuration */
    if (w->attr.st_nlink)
    {
        LOG(NOTICE, "Config file changed?");

        success = config_open();
        /* on success re-apply configuration */

        if (success)
        {
            config_apply();
        }
    }
    else
    {
        LOG(ERR, "Config file removed, But why?");
    }
}


static bool config_open()
{
    bool success = true;
    json_t * js = NULL;
    json_error_t    je;

    /* Convert json to config structure */
    js = json_load_file(conf_path, 0, &je);

    if (NULL == js)
    {
        LOG(INFO, "Verify config json error::text=%s|line=%d|pos=%d",
           je.text,
           je.line,
           je.position);
        success = false;
    }

    if (success && !config_from_json(&conf, js, false, NULL))
    {
        LOG(ERR, "Parse config error");
        success = false;
    }

    /* release memory */
    if(NULL != js) json_decref(js);

    return success;
}


static bool config_watch()
{
    ev_stat_init(&wconf, cb_config, conf_path, 0);
    ev_stat_start(EV_DEFAULT_ &wconf);

    LOG(NOTICE, "Watching config started!");

    return true;
}


static bool config_apply()
{
    LOG(NOTICE, "Applying config ...");

    /* for a moment apply only new settings */
    return log_severity_parse(conf.loglevel);

}


bool init_config()
{
    bool success = true;

    /* open configuration file and verify json */
    SKIP_ONFAIL(success, config_open());

    /* start watcher on configuration file     */
    SKIP_ONFAIL(success, config_watch());

    /* apply configuration */
    SKIP_ONFAIL(success, config_apply(&conf));

    return true;
}
