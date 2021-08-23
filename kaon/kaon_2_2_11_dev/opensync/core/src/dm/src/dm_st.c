/*
Copyright (c) 2015, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ev.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>

#include "log.h"
#include "ovsdb.h"
#include "schema.h"

#include "monitor.h"
#include "json_util.h"
#include "ovsdb_update.h"
#include "ovsdb_sync.h"
#include "os_util.h"
#include "util.h"

#include "target.h"
#include "pasync.h"
#include "dm.h"


/* speed test json output decoding  */
#include "dm_st_pjs.h"
#include "pjs_gen_h.h"
#include "dm_st_pjs.h"
#include "pjs_gen_c.h"

//#include "dm_st_iperf.h"

#define ST_STATUS_OK    (0)     /* everything all right         */
#define ST_STATUS_JSON  (-1)    /* received non-json ST output  */
#define ST_STATUS_READ  (-2)    /* can't convert json to struct */

#define SISP        "isp: "
#define LSISP       strlen(SISP)
#define SSERVERID   "serverid: "
#define LSSERVERID  strlen(SSERVERID)

/* can't be on the stack */
static ovsdb_update_monitor_t st_monitor;
static bool st_in_progress = false;  /* prevent multiple speedtests simultaneous run */
static ev_timer st_timeout;
static ev_timer st_timestart;

#define SPEEDTEST_EXE "ookla"
#define SPEEDTEST_PATH CONFIG_INSTALL_PREFIX "/bin/" SPEEDTEST_EXE
#define SPEEDTEST_TIMEOUT   90       /* seconds */
#define SPEEDTEST_START     1       /* seconds */

#ifndef CONFIG_SPEEDTEST_CMD
#define CONFIG_SPEEDTEST_CMD \
    CONFIG_SPEEDTEST_NICE " " SPEEDTEST_PATH \
    " -c " CONFIG_SPEEDTEST_URL CONFIG_SPEEDTEST_PARAM" -fjson"
#endif


#define OOKLA_SELECTION_DETAILS_CMD \
        SPEEDTEST_PATH " -c " CONFIG_SPEEDTEST_URL" -B --selection-details"

#define MAX_PREFERRED_SERVERS  32
#define MAX_ELIGIBLE_SERVERS   64

struct st_config
{
    unsigned select_server_id;
    unsigned preferred_list[MAX_PREFERRED_SERVERS];
    unsigned num_preferred;
};

struct eligible_server
{
    unsigned  server_id;
    double    latency;
    char      sponsor[64];
    char      location[64];
};

struct st_context
{
    bool pref_list_mode;
    bool pref_selected;
    bool higher_ranked_offered;
};

/* helper function for writing an integer to a proc/sysfs file */
bool sysfs_write(const char *path, int value)
{
    char buf[16];
    bool retval = false;
    int fd = -1;

    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        goto error;
    }

    snprintf(buf, sizeof(buf), "%d\n", value);

    if (write(fd, buf, strlen(buf)) <= 0)
    {
        goto error;
    }

    retval = true;
error:
    if (fd >= 0)
    {
        close(fd);
    }

    return retval;
}

#if defined(CONFIG_OOKLA_WAR_MODULE)

#define PATH_SYS_MODULES        "/sys/module"
#define PATH_PARAM_IGNORE_CSUM  "parameters/ignore_csum"

static bool pir_war_set(bool enable)
{
    char pname[PATH_MAX];

    struct dirent *de;

    DIR *dir = NULL;
    bool retval = false;
    int val = (enable == true) ? 1 : 0;

    dir = opendir(PATH_SYS_MODULES);
    if (dir == NULL)
    {
        LOG(WARN, "Error opening: %s. WAR for PIR-10476 will not be enabled.\n", PATH_SYS_MODULES);
        goto error;
    }

    /* Scan the /sys/modules folder, check if there's a module that has the "parameters/ignore_csum" file entry */
    while (errno = 0, (de = readdir(dir)) != NULL)
    {
        snprintf(pname, sizeof(pname), "%s/%s/%s",
                PATH_SYS_MODULES,
                de->d_name,
                PATH_PARAM_IGNORE_CSUM);

        if (access(pname, W_OK) != 0) continue;

        LOG(INFO, "%s WAR for PIR-10476 on interface: %s", enable ? "Enabling" : "Disabling", de->d_name);

        if (!sysfs_write(pname, val))
        {
            LOG(INFO, "Error writing to %s. WAR for PIR-10476 won't be enabled/disabled.", pname);
        }
    }

    if (errno != 0)
    {
        LOG(WARN, "Error scanning %s.\n", PATH_SYS_MODULES);
        goto error;
    }

    retval = true;

error:
    if (dir != NULL)
    {
        closedir(dir);
    }

    return retval;
}


static void enable_perf_war(void)
{
    pir_war_set(true);
}

static void disable_perf_war(void)
{
    pir_war_set(false);
}

#elif defined(CONFIG_OOKLA_WAR_GRO)

static void enable_perf_war(void)
{
    int rc;

    rc = system("ethtool -K eth0 gro on; ethtool -K eth1 gro on");
    if (rc < 0 || WEXITSTATUS(rc) != 0)
    {
        LOGE("Failed to turn on GRO");
    }
}

static void disable_perf_war(void)
{
    int rc;

    rc = system("ethtool -K eth0 gro off; ethtool -K eth1 gro off");
    if (rc < 0 || WEXITSTATUS(rc) != 0)
    {
        LOGE("Failed to turn off GRO");
    }
}

#else /* CONFIG_OOKLA_WAR_TCP_IGNORE_CSUM */

static void enable_perf_war(void)
{
    if (access("/proc/sys/net/ipv4/tcp_ignore_csum", W_OK) < 0)
        return;

    if (sysfs_write("/proc/sys/net/ipv4/tcp_ignore_csum", 1) != true)
    {
        LOGE("Failed to set sysctl ignore TCP checksums");
    }
}

static void disable_perf_war(void)
{
    if (access("/proc/sys/net/ipv4/tcp_ignore_csum", W_OK) < 0)
        return;

    if (sysfs_write("/proc/sys/net/ipv4/tcp_ignore_csum", 0) != true)
    {
        LOGE("Failed to set sysctl ignore TCP checksums");
    }
}
#endif

static int parse_ookla_json(
        json_t *js_root,
        struct schema_Wifi_Speedtest_Status *st_status)
{
    const char *isp_str = NULL;
    int bandwith, elapsed;
    json_error_t error;
    json_t *js;
    int id;


    js = json_object_get(js_root, "ping");
    if (js == NULL || json_unpack_ex(js, &error, 0, "{s:f, s:f}",
            "latency", &st_status->RTT,
            "jitter", &st_status->jitter) != 0)
    {
        if (js) LOG(ERR, "JSON parse error: %d:%d: %s", error.line, error.column, error.text);
        return -1;
    }
    st_status->RTT_exists = true;
    st_status->jitter_exists = true;

    js = json_object_get(js_root, "download");
    if (js == NULL || json_unpack_ex(js, &error, 0, "{s:i, s:i, s:i}",
            "bandwidth", &bandwith,
            "bytes", &st_status->DL_bytes,
            "elapsed", &elapsed) != 0)
    {
        if (js) LOG(ERR, "JSON parse error: %d:%d: %s", error.line, error.column, error.text);
        return -1;
    }
    st_status->DL = ((double)bandwith * 8) / 1e6;  // [bytes/s] --> [Mbit/s]
    st_status->DL_exists = true;
    st_status->DL_bytes_exists = true;
    st_status->DL_duration = (double)elapsed / 1000.0;  // [ms] --> [s]
    st_status->DL_duration_exists = true;
    st_status->duration += st_status->DL_duration;
    st_status->duration_exists = true;

    js = json_object_get(js_root, "upload");
    if (js == NULL || json_unpack_ex(js, &error, 0, "{s:i, s:i, s:i}",
            "bandwidth", &bandwith,
            "bytes", &st_status->UL_bytes,
            "elapsed", &elapsed) != 0)
    {
        if (js) LOG(ERR, "JSON parse error: %d:%d: %s", error.line, error.column, error.text);
        return -1;
    }
    st_status->UL = ((double)bandwith * 8) / 1e6; // [bytes/s] --> [Mbit/s]
    st_status->UL_exists = true;
    st_status->UL_bytes_exists = true;
    st_status->UL_duration = (double)elapsed / 1000.0;  // [ms] --> [s]
    st_status->UL_duration_exists = true;
    st_status->duration += st_status->UL_duration;
    st_status->duration_exists = true;

    if (json_unpack_ex(js_root, &error, 0, "{s:s}", "isp", &isp_str) != 0)
    {
        LOG(ERR, "JSON parse error: %d:%d: %s", error.line, error.column, error.text);
        return -1;
    }
    STRSCPY(st_status->ISP, isp_str);
    st_status->ISP_exists = true;

    if (json_unpack_ex(js_root, &error, 0, "{s:{s:i}}", "server", "id", &id) != 0)
    {
        LOG(ERR, "JSON parse error: %d:%d: %s", error.line, error.column, error.text);
        return -1;
    }
    /* Ookla APP default output does not return sponsor, hence we've
     * been using serverid as server name string.
     * TODO: After switching to Ookla json output, we can get sponsor
     *       string. Shall we keep reporting server_id for backward
     *       compatibility? */
    sprintf(st_status->server_name, "%d", id);
    st_status->server_name_exists = true;

    return 0;
}

static int parse_ookla_json_output(
        const char *buff,
        int buff_sz,
        struct schema_Wifi_Speedtest_Status *st_status)
{
    json_t *js;
    json_error_t je;


    js = json_loads(buff, 0, &je);
    if (js == NULL)
    {
        LOG(ERR, "OOKLA ST report:\n%s", (char*)buff);
        LOG(ERR, "OOKLA ST report JSON validation failed=%s line=%d pos=%d",
           je.text,
           je.line,
           je.position);
        return -1;
    }
    if (parse_ookla_json(js, st_status) != 0)
    {
        LOG(ERR, "OOKLA ST: Failed to parse json: '%s'", buff);
        json_decref(js);
        return -1;
    }
    json_decref(js);
    return 0;
}

/* pasync API callback, invoked upon speedtest completes its job     */
void pa_cb(struct st_context *st_ctx, void *buff, int buff_sz)
{
    //struct st_context *st_ctx = (struct st_context *)ctx->data;
		
    struct schema_Wifi_Speedtest_Status  st_status;
    int status = ST_STATUS_READ;
    pjs_errmsg_t err;
    ovs_uuid_t uuid = { '\0' };
    int rc;

    LOG(DEBUG, "Received speed test result. bytes=%d, str='%s'", buff_sz, (char *)buff);

    ev_timer_stop(EV_DEFAULT, &st_timeout);
    /* signal that ST has been completed */
    st_in_progress = false;

    /* disable temporary performance workarounds */
    disable_perf_war();

    /* zero whole st_status structure */
    memset(&st_status, 0, sizeof(struct schema_Wifi_Speedtest_Status));

    if (buff_sz > 0)
    {
        /* parse ookla results */
        rc = parse_ookla_json_output(buff, buff_sz, &st_status);
        if (rc == 0 && (st_status.UL_exists) && (st_status.DL_exists))
            status = ST_STATUS_OK;
        else
            status = ST_STATUS_READ;
    }
    else
    {
		/*
        if (ctx->rc != 0)
            LOG(ERR, "Speedtest command failed with rc=%d", ctx->rc);
		*/
        if (buff_sz == 0)
            LOG(ERR, "No output from speedtest app received.");
    }

    st_status.status = status;
    st_status.status_exists = true;
    st_status.testid = 0 ; //ctx->id;
    st_status.testid_exists = true;
    STRSCPY(st_status.test_type, "OOKLA");
    st_status.test_type_exists = true;

    st_status.timestamp = time(NULL);
    st_status.timestamp_exists = true;

    if (st_ctx != NULL && st_ctx->pref_list_mode)
    {
        st_status.pref_selected = st_ctx->pref_selected;
        st_status.pref_selected_exists = true;

        st_status.hranked_offered = st_ctx->higher_ranked_offered;
        st_status.hranked_offered_exists = true;
    }

    LOG(NOTICE, "OOKLA ST: Speedtest results:"
                " DL: %f Mbit/s (bytes=%d, duration=%f s),"
                " UL: %f Mbit/s (bytes=%d, duration=%f s),"
                " duration: %f s, ISP: %s, server: %s, latency %f,"
                " testid: %d, status=%d, timestamp=%d",
                 st_status.DL,
                 st_status.DL_bytes,
                 st_status.DL_duration,
                 st_status.UL,
                 st_status.UL_bytes,
                 st_status.UL_duration,
                 st_status.duration,
                 st_status.ISP,
                 st_status.server_name,
                 st_status.RTT,
                 st_status.testid,
                 st_status.status,
                 st_status.timestamp);

    /* fill the row with NODE data */
    if (false == ovsdb_sync_insert(SCHEMA_TABLE(Wifi_Speedtest_Status),
                                   schema_Wifi_Speedtest_Status_to_json(&st_status, err),
                                   &uuid)
       )
    {
        LOG(ERR, "ovsdb table: Wifi_Speedtest_Status: insert error; ST results not written.");
    }
    else
    {
        LOG(NOTICE, "ovsdb table: Wifi_Speedtest_Status: ST results written. uuid=%s", uuid.uuid);
    }
}

static int parse_st_config(
        struct st_config *st_config,
        const struct schema_Wifi_Speedtest_Config *speedtest_config)
{
    unsigned i;

    memset(st_config, 0, sizeof(*st_config));

    if (speedtest_config->select_server_id_exists)
    {
        st_config->select_server_id = speedtest_config->select_server_id;
        LOG(DEBUG, "ST: select_server_id=%d", st_config->select_server_id);
    }

    st_config->num_preferred = speedtest_config->preferred_list_len;
    for (i = 0; i < st_config->num_preferred && i < MAX_PREFERRED_SERVERS; i++)
    {
        st_config->preferred_list[i] = speedtest_config->preferred_list[i];
        LOG(DEBUG, "ST: preferred_list[%d]=%u", i, st_config->preferred_list[i]);
    }

    return 0;
}

static void on_speedtest_timeout(struct ev_loop *loop, ev_timer *watcher, int revent)
{
    (void)loop;
    (void)watcher;
    (void)revent;

    LOG(ERR, "Speedtest maximum duration exceeded. Killing speedtest app.");

    strexa("killall", "-KILL", SPEEDTEST_EXE);

    st_in_progress = false;  /* Not strictly necessary here, since after
                              * speedtest app is killed the pasync callback
                              * should be called with 0 bytes output
                              * and this flag will be cleared there. */
}

static bool is_server_id_in_preferred_list(
        unsigned server_id,
        const struct st_config *st_config)
{
    unsigned i;

    for (i = 0; i < st_config->num_preferred; i++)
    {
        if (st_config->preferred_list[i] == server_id)
            return true;
    }
    return false;
}

static int ookla_get_eligible_servers(
        struct eligible_server *esrvs,
        unsigned *num_esrvs)
{
    const char *cmd = OOKLA_SELECTION_DETAILS_CMD;
    const char *SEL_PREFIX = "selection result: ";
    unsigned num;
    char buf[4096];
    char *tok;
    char *strp;
    FILE *f;

    LOG(DEBUG, "Running cmd: %s", cmd);
    f = popen(cmd, "r");
    if (f == NULL)
    {
        LOG(ERR, "Error running: %s", cmd);
        return -1;
    }

    num = 0;
    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strncmp(buf, SEL_PREFIX, strlen(SEL_PREFIX)) == 0)
        {
            /* Parsing lines such as:
             * selection result: 3560; Telekom Slovenije, d.d.; Ljubljana: 1.05 ms */
            strp = buf + strlen(SEL_PREFIX);

            if ((tok = strsep(&strp, ";")) == NULL) break;
            esrvs[num].server_id = (unsigned) atoi(tok);

            if ((tok = strsep(&strp, ";")) == NULL) break;
            tok++;
            STRSCPY(esrvs[num].sponsor, tok);

            if ((tok = strsep(&strp, ":")) == NULL) break;
            tok++;
            STRSCPY(esrvs[num].location, tok);

            if ((tok = strsep(&strp, "\n")) == NULL) break;
            esrvs[num].latency = atof(tok);

            LOG(INFO, "ST: eligibleserver[%d]: id=%u, latency=%f, sponsor='%s', "
                      "location='%s'",
                      num,
                      esrvs[num].server_id, esrvs[num].latency, esrvs[num].sponsor,
                      esrvs[num].location);
            num++;
            if (num == MAX_ELIGIBLE_SERVERS)
            {
                LOG(WARN, "ST: Max=%u eligible servers reached.", num);
                break;
            }
        }
        else if (num > 0)
        {
            /* End of "selection results" detected. */
            break;
        }
    }
    *num_esrvs = num;

    /* Kill the speedtest process at this point, otherwise pclose() will
     * block until the process terminates. We want to get only server selection
     * results with latency numbers, but not run (and wait for) speedtest, yet.  */
    strexa("killall", "-KILL", SPEEDTEST_EXE);

    pclose(f);
    return 0;
}

/* Comparator for qsort() to sort by latency, lowest latency first. */
int compare_eligible_servers(const void *a, const void *b)
{
    struct eligible_server *es_a = (struct eligible_server *)a;
    struct eligible_server *es_b = (struct eligible_server *)b;

    if (es_a->latency < es_b->latency)
        return -1;
    else if (es_a->latency > es_b->latency)
        return 1;
    else
        return 0;
}

static int ookla_preferred_list_select_server(
        unsigned *selected_server_id,
        bool *is_pref_selected,
        bool *is_higher_ranked_offered,
        const struct st_config *st_config)
{
    struct eligible_server esrvs[MAX_ELIGIBLE_SERVERS] = { 0 };
    bool higher_ranked_offered = false;
    bool pref_selected = false;
    unsigned selected_id = 0;
    unsigned num;
    unsigned i;

    /* Get ookla-offered server list: */
    if (ookla_get_eligible_servers(esrvs, &num) != 0)
        return -1;

    /* Sort the list by latency: */
    qsort(esrvs, num, sizeof(esrvs[0]), compare_eligible_servers);

    /* Traverse through list and try to select most highly ranked preferred:  */
    for (i=0; i < num; i++)
    {
        if (i == 0)
            selected_id = esrvs[i].server_id; /* Default to highest ranked eligible. */

        if (is_server_id_in_preferred_list(esrvs[i].server_id, st_config))
        {
            /* Highest ranked eligible server that is also in preferred_list: */
            selected_id = esrvs[i].server_id;

            pref_selected = true;
            if (i > 0)
                higher_ranked_offered = true; /* preferred selected event though higher ranked offered */

            LOG(INFO, "ST: Selected eligible server in preferred list:"
                      " id=%u, latency=%f, sponsor='%s', "
                      "location='%s'",
                      esrvs[i].server_id, esrvs[i].latency, esrvs[i].sponsor, esrvs[i].location);
            break; // found
        }
    }

    *selected_server_id = selected_id;
    LOG(INFO, "ST: Selected server_id=%u, pref_selected=%d, "
              "higher_ranked_offered=%d",
               selected_id, pref_selected, higher_ranked_offered);
    *is_pref_selected = pref_selected;
    *is_higher_ranked_offered = higher_ranked_offered;

    return 0;
}

char st_cmd[TARGET_BUFF_SZ];


static void on_speedtest_start(struct ev_loop *loop, ev_timer *watcher, int revent)
{
    (void)loop;
    (void)watcher;
    (void)revent;
    FILE *f;
    char buff[1024];
    int buff_sz = 0;
    //struct st_context st_ctx = { 0 };
    struct st_context *st_ctx = NULL;
	
	
	st_ctx = (struct st_context *)watcher->data;
    LOG(DEBUG, "%s", __FUNCTION__);

	if (st_in_progress)
    {
        LOG(ERR, "Speed test already running");
        goto ret;
    }
	st_in_progress = true; 
    LOG(DEBUG, "Running cmd: %s", st_cmd);
    f = popen(st_cmd, "r");
    if (f == NULL)
    {
        LOG(ERR, "Error running: %s", st_cmd);
        goto ret;
    }
    else
    {
        LOG(NOTICE, "OOKLA speedtest started!");
        st_in_progress = true;
    }

    while (fgets(buff, sizeof(buff), f) != NULL)
    {
		LOG(DEBUG, "Result : %s", buff);
    }
    buff_sz = strlen(buff);
    pa_cb(st_ctx, buff, buff_sz);

    pclose(f);
ret:	
    st_in_progress = false;
	free(st_ctx);
}


void dm_stupdate_cb(ovsdb_update_monitor_t *self)
{
    struct schema_Wifi_Speedtest_Config speedtest_config;
    struct st_config st_config;
    pjs_errmsg_t perr;
	struct st_context *st_ctx_data = NULL;
    struct st_context st_ctx = { 0 };

    LOG(DEBUG, "%s", __FUNCTION__);

    switch (self->mon_type)
    {
        case OVSDB_UPDATE_NEW:
        case OVSDB_UPDATE_MODIFY:

            if (st_in_progress)
            {
                LOG(ERR, "Speedtest already in progress");
                return;
            }

            if (!schema_Wifi_Speedtest_Config_from_json(&speedtest_config, self->mon_json_new, false, perr))
            {
                LOG(ERR, "Parsing Wifi_Speedtest_Config NEW or MODIFY request: %s", perr);
                return;
            }

            /* run the speed test according to the cloud instructions */
            if (!strcmp(speedtest_config.test_type, "OOKLA"))
            {
                const char* tools_dir = target_tools_dir();
                if (tools_dir == NULL)
                {
                    LOG(ERR, "Error, tools dir not defined");
                }
                else
                {
                    struct stat sb;
                    if ( !(0 == stat(tools_dir, &sb) && S_ISDIR(sb.st_mode)) )
                    {
                        LOG(ERR, "Error, tools dir does not exist");
                    }
                }

                /* enable temporary performance workarounds */
                enable_perf_war();

                parse_st_config(&st_config, &speedtest_config);

                if (st_config.select_server_id > 0)   /* select server id mode */
                {
                    LOG(INFO, "ST: select_server_id mode: selected server_id=%u",
                            st_config.select_server_id);
                }
                else if (st_config.num_preferred > 0)  /* preferred list mode: */
                {
                    unsigned selected_server_id = 0;

                    if (ookla_preferred_list_select_server(
                            &selected_server_id,
                            &st_ctx.pref_selected,
                            &st_ctx.higher_ranked_offered,
                            &st_config) != 0)
                    {
                        LOG(ERR, "Error selecting from preferred list");
                        return;
                    }
                    st_config.select_server_id = selected_server_id;
                    st_ctx.pref_list_mode = true;

                    LOG(INFO, "ST: preferred_list mode: selected server_id=%u",
                            st_config.select_server_id);
                }

                ev_timer_init(&st_timeout, on_speedtest_timeout, SPEEDTEST_TIMEOUT, 0);
                ev_timer_start(EV_DEFAULT, &st_timeout);

                /* Build speedtest command: */

                if (st_config.select_server_id > 0)
                {
                    snprintf(st_cmd, sizeof(st_cmd), "%s -s %u",
                            CONFIG_SPEEDTEST_CMD,
                            st_config.select_server_id);
                }
                else
                {
                    snprintf(st_cmd, sizeof(st_cmd), "%s",
                            CONFIG_SPEEDTEST_CMD);
                }
                LOG(DEBUG, "%s  ### ST cmd %s", __FUNCTION__ , st_cmd);

				st_timestart.data = st_ctx_data;
                ev_timer_init(&st_timestart, on_speedtest_start, SPEEDTEST_START, 0);
                ev_timer_start(EV_DEFAULT, &st_timestart);
            }
            else if (!strcmp(speedtest_config.test_type, "IPERF3_S"))
            {
				LOG(ERR, "Iperf speedtest not yet implemented");
                //iperf_run_speedtest(&speedtest_config);
            }
            else if (!strcmp(speedtest_config.test_type, "IPERF3_C"))
            {
				LOG(ERR, "Iperf speedtest not yet implemented");
                //iperf_run_speedtest(&speedtest_config);
            }
            else if (!strcmp(speedtest_config.test_type, "MPLAB"))
            {
                LOG(ERR, "MPLAB speedtest not yet implemented");
            }
            else
            {
                LOG(ERR, "%s speedtest doesn't exist", speedtest_config.test_type);
            }

            break;

        case OVSDB_UPDATE_DEL:
            /* Reset configuration */
            LOG(INFO, "Cloud cleared Wifi_Speedtest_Config table");
            break;

        default:
            LOG(ERR, "Update Monitor for Wifi_Speedtest_Config reported an error.");
            break;
    }
}


/*
 * Monitor Wifi_Speedtest_Config table
 */
bool dm_st_monitor()
{
    bool        ret = false;

    /* Set monitoring */
    if (false == ovsdb_update_monitor(&st_monitor,
                                      dm_stupdate_cb,
                                      SCHEMA_TABLE(Wifi_Speedtest_Config),
                                      OMT_ALL)
       )
    {
        LOG(ERR, "Error initializing Wifi_Speedtest_Config monitor");
        goto exit;
    }
    else
    {
        LOG(NOTICE, "Wifi_Speedtest_Config monitor started");
        st_in_progress = false;
    }
    ret = true;

exit:
    return ret;
}
