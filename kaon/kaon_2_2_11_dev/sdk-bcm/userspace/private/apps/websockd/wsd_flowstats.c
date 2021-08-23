/*
 <:copyright-BRCM:2018:proprietary:standard

    Copyright (c) 2018 Broadcom
    All Rights Reserved

  This program is the proprietary software of Broadcom and/or its
  licensors, and may only be used, duplicated, modified or distributed pursuant
  to the terms and conditions of a separate, written license agreement executed
  between you and Broadcom (an "Authorized License").  Except as set forth in
  an Authorized License, Broadcom grants no license (express or implied), right
  to use, or waiver of any kind with respect to the Software, and Broadcom
  expressly reserves all rights in and to the Software and all intellectual
  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

  Except as expressly set forth in the Authorized License,

  1. This program, including its structure, sequence and organization,
     constitutes the valuable trade secrets of Broadcom, and you shall use
     all reasonable efforts to protect the confidentiality thereof, and to
     use this information only in connection with your use of Broadcom
     integrated circuit products.

  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
     PERFORMANCE OF THE SOFTWARE.

  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
     LIMITED REMEDY.
:>
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include "cms_dal.h"
#include "cms_qdm.h"
#include "websockets/libwebsockets.h"
#include "wsd_main.h"
#include "fcctl_api.h"

#define MAX_QUERIES_COUNT 16

static char response[BUFLEN_512];

static struct query_data {
    uint32_t id;
    __time_t last_ts;
    uint64_aligned last_bytes;
} queries[MAX_QUERIES_COUNT];

static uint32_t queries_count = 0;

static FlwStatsPollParams_t orig_poll_params;
static FlwStatsPollParams_t new_poll_params = {
    .intval = 100,     /* poll every 100ms */
    .granularity = 20  /* 1/20th of the flows */
};

#ifdef LIVESTATS_DEBUG
#define livestats_info(fmt...) printf("[LiveStats] " fmt)
#else
#define livestats_info(fmt...)
#endif
#define livestats_err(fmt...) printf("[LiveStats] Error! " fmt)

static int update_client(struct wsd_cmn_state *state)
{
    int i, n = 0;
    struct timeb ts;

    ftime(&ts);

    for (i = 0; i < MAX_QUERIES_COUNT; i++)
    {
        int ret;
        FlwStatsQueryInfo_t query = {};

        if (queries[i].id == (unsigned int )-1)
            continue;

        query.get.handle = queries[i].id;
        ret = fcCtlGetFlwStatsQuery(&query);

        if (queries[i].last_bytes == 0 && queries[i].last_ts == 0)
        {
            queries[i].last_ts = ts.time*1000 + ts.millitm;
            continue;
        }

        if (ret)
        {
            livestats_err("Failed to get query stats; id %d; err %d\n", query.get.handle, ret);
            continue;
        }

        if (n >= BUFLEN_512)
        {
            livestats_err("response buffer overflow, max %d, n %d\n", BUFLEN_512, n);
            break;
        }

        if (!n)
            n += sprintf(&response[0],"Update=");
        else
            n += sprintf(&response[n],"|");

        n += sprintf(&response[n],"%u|%llu", queries[i].id,
                ((query.get.flwSt.rx_bytes - queries[i].last_bytes)/((ts.time*1000 + ts.millitm) - queries[i].last_ts))*1000);

        queries[i].last_ts = ts.time*1000 + ts.millitm;
        queries[i].last_bytes = query.get.flwSt.rx_bytes;
    }

    if (n != 0)
    {
        wsd_append_response(state, response);
    }
    return 0;
}

static int create_command_parse(struct wsd_cmn_state *state, const char *in, FlwStatsQueryInfo_t *query)
{
    char buf[BUFLEN_512], *param;

    strncpy(buf, in, sizeof(buf));

    /* skip the command and query name */
    strtok(buf, " ");
    strtok(NULL, " ");

    while ((param = strtok(NULL, " ")))
    {
        char *value;

        if (!(value = strtok(NULL, " ")))
        {
            livestats_err("Error: no value for param %s\n", param);
            goto Error;
        }

        if (fcCtlSetTupleParam(param, value, query))
        {
            livestats_err("setTupleParam(%s, %s) failed\n", param, value);
            goto Error;
        }
    }

    return 0;

Error:
    sprintf(&response[0],"Error=Bad parameters");
    wsd_append_response(state, response);
    return -1;
}

static FlwStatsQueryInfo_t *single_param_parse(char *param, char *value)
{
    static FlwStatsQueryInfo_t query;

    memset(&query, 0, sizeof(query));
    if (fcCtlSetTupleParam(param, value, &query))
    {
        livestats_err("setTupleParam(%s, %s) failed\n", param, value);
        return NULL;
    }
    return &query;
}

static void query_delete(uint32_t idx)
{
    FlwStatsQueryInfo_t query = {};
    int ret;

    query.delete.handle = queries[idx].id;
    queries[idx].id = -1;
    queries[idx].last_ts = queries[idx].last_bytes = 0;

    queries_count--;

    if(!(ret = fcCtlDeleteFlwStatsQuery(&query)))
        livestats_info("Deleted query %d\n", query.delete.handle);
    else
        livestats_err("Failed to delete query %d; err %d\n", query.delete.handle, ret);

    if (!queries_count)
        fcCtlSetFlwPollParams(&orig_poll_params);
}

static void query_gauge_create(struct wsd_cmn_state *state, FlwStatsQueryInfo_t *query, char *name)
{
    int ret, i;
    char err_msg[64];

    if (!query)
        return;

    for (i = 0; i < MAX_QUERIES_COUNT && queries[i].id != (unsigned int) -1; i++);
    if (i == MAX_QUERIES_COUNT)
    {
        livestats_err("Error: too many queries\n");
        strncpy(err_msg, "Error=Too many queries", sizeof(err_msg));
        goto Error;
    }

    if ((ret = fcCtlCreateFlwStatsQuery(query)))
    {
        livestats_err("Failed to create new query; err %d\n", ret);
        snprintf(err_msg, sizeof(err_msg), "Error=Failed to create query; err %d", ret);
        goto Error;
    }
    livestats_info("Created query %d\n", query->create.handle);
    queries[i].id = query->create.handle;

    if (!queries_count)
    {
        fcCtlGetFlwPollParams(&orig_poll_params);
        if (fcCtlSetFlwPollParams(&new_poll_params))
            livestats_err("Failed to update FlwStats poll parameters\n");
    }
    queries_count++;

    sprintf(&response[0],"CreateAck=%d|%s|1G", queries[i].id, name);

    wsd_append_response(state, response);
    return;
Error:
    sprintf(&response[0], "%s", err_msg);
    wsd_append_response(state, response);
}

void default_gauges_create(struct wsd_cmn_state *state)
{
    UINT32 lanCount, wanCount;
    char query_name[32]={0};
    char lanIfNamesBuf[512]={0};
    char wanIfNamesBuf[256]={0};
    char *str, *savePtr, *ifName;

   cmsLck_acquireLock();
   lanCount = qdmIntf_getAllLanIntfNames(lanIfNamesBuf, sizeof(lanIfNamesBuf));
   cmsLog_debug("got %d lan intfs:%s", lanCount, lanIfNamesBuf);
   wanCount = qdmIntf_getAllWanL2IntfNames(wanIfNamesBuf, sizeof(wanIfNamesBuf));
   cmsLog_debug("got %d wan intfs:%s", wanCount, wanIfNamesBuf);
   cmsLck_releaseLock();

   if (lanCount > 0)
   {
      str = lanIfNamesBuf;
      savePtr = NULL;
      while ((ifName = strtok_r(str, ",", &savePtr)) != NULL)
      {
         cmsLog_debug("lan ifName=%s", ifName);
         if (!strncmp(ifName, "eth", 3) ||
             !strncmp(ifName, "wl", 2))
         {
            snprintf(query_name, sizeof(query_name), "TX %s",  ifName);
            query_gauge_create(state, single_param_parse("dstphy", ifName),
                        query_name);
         }
         str = NULL;
      }
   }
   
   if (wanCount > 0)
   {
      str = wanIfNamesBuf;
      savePtr = NULL;
      while ((ifName = strtok_r(str, ",", &savePtr)) != NULL)
      {
         cmsLog_debug("wan ifName=%s", ifName);
         if (!strncmp(ifName, "eth", 3))
         {
            snprintf(query_name, sizeof(query_name), "RX %s",  ifName);
            query_gauge_create(state, single_param_parse("srcphy", ifName),
                               query_name);
         }
         str = NULL;
      }
   }

   return;
}

int wsd_callback_flowstats(struct lws *wsi, enum lws_callback_reasons reason,
        void *user __attribute__((unused)), void *in, size_t len)
{
    struct wsd_cmn_state *state = (struct wsd_cmn_state*) user;
    int ret = 0;

    switch (reason)
    {
    case LWS_CALLBACK_ESTABLISHED:
        {
            int i;

            state->session.sessionKey = 0;
            /* save the wsi pointer in our state */
            state->wsi = wsi;

            for (i = 0; i < MAX_QUERIES_COUNT; i++)
                queries[i].id = -1;

            default_gauges_create(state);
        }
        break;

    case LWS_CALLBACK_CLOSED:
        {
            int i;

            for (i = 0; i < MAX_QUERIES_COUNT; i++)
            {
                if (queries[i].id == (unsigned int) -1)
                    continue;

                query_delete(i);
            }
        }
        break;

    case LWS_CALLBACK_SERVER_WRITEABLE:
        ret = wsd_callback_serverwritable(state, wsi);
        break;

    case LWS_CALLBACK_RECEIVE:
        if (!len)
            break;
        if (strncmp(in, "Update?", len) == 0)
        {
            if (update_client(state))
                livestats_err("Failed to update client\n");
        }
        else if (strstr(in, "Create"))
        {
            FlwStatsQueryInfo_t query = {};
            char name[64];

            sscanf(in, "Create %64s", name);
            livestats_info("Going to create query named %s\n", name);

            if (!create_command_parse(state, in, &query))
                query_gauge_create(state, &query, name);
        }
        else if (strstr(in, "Delete"))
        {
            unsigned int id, i;

            livestats_info("Got message \'%s\'\n", (char *)in);

            sscanf(in, "Delete %4d", &id);

            for (i = 0; i < MAX_QUERIES_COUNT && queries[i].id != id; i++);
            if (i == MAX_QUERIES_COUNT)
                break;

            query_delete(i);
        }
        break;

    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        break;

    default:
        break;
    }
    return ret < 0 ? ret : 0;
}
