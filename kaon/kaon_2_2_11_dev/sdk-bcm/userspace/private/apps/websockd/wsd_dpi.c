/***********************************************************************
 *
 *  Copyright (c) 2014  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2014:proprietary:standard 

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
 *
 ************************************************************************/

#ifdef SUPPORT_WEB_SOCKETS
#ifdef SUPPORT_DPI

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "websockets/libwebsockets.h"

#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"

#include "dpictl_api.h"
#include "wsd_main.h"

#define TMP_BUFFER_SIZE   4096

static int sprintf_realloc(char **buf, int *pos, int *len, const char *fmt, ...)
{
   int ret;
   static char tmp[TMP_BUFFER_SIZE];
   va_list args;

   va_start(args, fmt);

   ret = vsnprintf(tmp, sizeof(tmp), fmt, args);
   if (ret < 0) {
      printf("%s: error during vsnprintf: %d\n", __func__, ret);
      goto out;
   }

   /* realloc data if required */
   if (*pos + ret >= *len) {
      char *t = *buf;

      *len = *len * 2;
      *buf = realloc(*buf, *len);
      if (!*buf) {
         printf("%s: unable to realloc data to size %d\n", __func__, *len);
         free(t);
         ret = -1;
         goto out;
      }
   }

   memcpy(&(*buf)[*pos], tmp, ret + 1);
   *pos = *pos + ret;

out:
   va_end(args);
   return ret;
}

static void dpi_handle_sessionkey(struct wsd_cmn_state *state, char *args)
{
   if (!args) {
      cmsLog_error("key command with no session key provided");
      return;
   }

   state->session.sessionKey = atoi(args);
}

static void dpi_get_app_names(struct wsd_cmn_state *state, char *args)
{
   struct dc_table *table = dpictl_get_db(db_app);
   struct dc_id_name_map *entry;
   char *data;
   int len = TMP_BUFFER_SIZE;
   int pos = 0;
   int ret;
   int i;

   if (!table)
      return;
   entry = (struct dc_id_name_map *) table->data;

   data = malloc(len);
   if (!data) {
      printf("%s: unable to allocate response data\n", __func__);
      goto out_free_table;
   }

   pos += sprintf(data, "[\"appnames\",{");

   for (i = 0; i < table->entries; i++) {
      ret = sprintf_realloc(&data, &pos, &len, "%s\"%u\":\"%s\"",
                            (i ? "," : ""),
                            entry[i].id, entry[i].name);
      if (ret < 0)
         goto out_free_data;
   }

   ret = sprintf_realloc(&data, &pos, &len, "}]");
   if (ret < 0)
      goto out_free_data;

   /* send response to client */
   wsd_append_response(state, data);

out_free_data:
   free(data);
out_free_table:
   for (i = 0; i < table->entries; i++)
      free(entry[i].name);
   free(table);
}

static void dpi_get_device_names(struct wsd_cmn_state *state, char *args)
{
   struct dc_table *table = dpictl_get_db(db_dev);
   struct dc_id_name_map *entry;
   char *data;
   int len = TMP_BUFFER_SIZE;
   int pos = 0;
   int ret;
   int i;

   if (!table)
      return;
   entry = (struct dc_id_name_map *) table->data;

   data = malloc(len);
   if (!data) {
      printf("%s: unable to allocate response data\n", __func__);
      goto out_free_table;
   }

   pos += sprintf(data, "[\"devicenames\",{");

   for (i = 0; i < table->entries; i++) {
      ret = sprintf_realloc(&data, &pos, &len, "%s\"%u\":\"%s\"",
                            (i ? "," : ""),
                            entry[i].id, entry[i].name);
      if (ret < 0)
         goto out_free_data;
   }

   ret = sprintf_realloc(&data, &pos, &len, "}]");
   if (ret < 0)
      goto out_free_data;

   /* send response to client */
   wsd_append_response(state, data);

out_free_data:
   free(data);
out_free_table:
   for (i = 0; i < table->entries; i++)
      free(entry[i].name);
   free(table);
}

static void dpi_get_appinsts(struct wsd_cmn_state *state, char *args)
{
   struct dc_table *appinsts;
   struct dc_appinst *appinst;
   struct dc_qos_appinst_stat *qos;
   struct dc_qos_info *info;
   char *data;
   int ds_default = -1;
   int us_default = -1;
   int len = TMP_BUFFER_SIZE;
   int pos = 0;
   int ret;
   int i;

   dpictl_update(DPICTL_UPDATE_APPINSTS | DPICTL_UPDATE_QOS_APPINSTS |
                 DPICTL_UPDATE_QOS_QUEUES);

   info = dpictl_get_qos_info();
   if (info) {
      ds_default = info->ds.default_queue;
      us_default = info->us.default_queue;
   }
   appinsts = dpictl_get_appinsts(0);
   if (!appinsts)
      goto out_free_info;
   appinst = (struct dc_appinst *)appinsts->data;

   data = malloc(len);
   if (!data) {
      printf("%s: unable to allocate response data\n", __func__);
      goto out_free_appinsts;
   }

   pos += sprintf(data, "[\"appinsts\",[");

   for (i = 0; i < appinsts->entries; i++) {
      qos = dpictl_get_qos_appinst(appinst[i].app.app_id, appinst[i].dev.mac);
      ret = sprintf_realloc(&data, &pos, &len,
                            "%s{\"a\":%d,\"m\":\"%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\","
                               "\"d\":%u,"
                               "\"up\":%llu,\"ub\":%llu,"
                               "\"dp\":%llu,\"db\":%llu,"
                               "\"dsq\":%d,"
                               "\"usq\":%d,"
                               "\"f\":%d}",
                            (i ? "," : ""),
                            appinst[i].app.app_id,
                            appinst[i].dev.mac[0], appinst[i].dev.mac[1],
                            appinst[i].dev.mac[2], appinst[i].dev.mac[3],
                            appinst[i].dev.mac[4], appinst[i].dev.mac[5],
                            appinst[i].dev.dev_id,
                            appinst[i].us.pkts,
                            appinst[i].us.bytes,
                            appinst[i].ds.pkts,
                            appinst[i].ds.bytes,
                            (qos ? qos->appinst.ds_queue : ds_default),
                            (qos ? qos->appinst.us_queue : us_default),
                            appinst[i].conn_count);
      if (qos)
         free(qos);
      if (ret < 0)
         goto out_free_appinsts;
   }
   ret = sprintf_realloc(&data, &pos, &len, "]]");
   if (ret < 0)
      goto out_free_data;

   /* send response to client */
   wsd_append_response(state, data);

out_free_data:
   free(data);
out_free_appinsts:
   free(appinsts);
out_free_info:
   if (info)
      free(info);
}

static void dpi_get_flows(struct wsd_cmn_state *state, char *args)
{
   char us_src[INET6_ADDRSTRLEN];
   char us_dst[INET6_ADDRSTRLEN];
   char ds_src[INET6_ADDRSTRLEN];
   char ds_dst[INET6_ADDRSTRLEN];
   struct dc_table *connections;
   struct dc_conn *c;
   uint32_t app_id;
   uint8_t mac[6];
   char *data;
   int len = TMP_BUFFER_SIZE;
   int pos = 0;
   int ret;
   int i, j = 0;

   /* parse app_id & mac */
   if (sscanf(args, "%u,%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            &app_id, &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])
         != 7) {
      printf("%s: Received invalid appinst: %s\n", __func__, args);
      return;
   }

   /* update appinsts to update connection info */
   dpictl_update(DPICTL_UPDATE_APPINSTS);

   connections = dpictl_get_connections();
   if (!connections)
      return;
   c = (struct dc_conn *)connections->data;

   data = malloc(len);
   if (!data) {
      printf("%s: unable to allocate response data\n", __func__);
      goto out_free_connections;
   }

   pos += sprintf(data, "[\"flows\",\"%d,%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\",[",
                  app_id, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

   for (i = 0; i < connections->entries; i++) {
      if (app_id != c[i].app.app_id ||
            memcmp(mac, c[i].dev.mac, sizeof(mac)))
         continue;

      inet_ntop(c[i].us.l3protonum, &c[i].us.l3src, us_src, sizeof(us_src));
      inet_ntop(c[i].us.l3protonum, &c[i].us.l3dst, us_dst, sizeof(us_dst));
      inet_ntop(c[i].ds.l3protonum, &c[i].ds.l3src, ds_src, sizeof(ds_src));
      inet_ntop(c[i].ds.l3protonum, &c[i].ds.l3dst, ds_dst, sizeof(ds_dst));

      ret = sprintf_realloc(&data, &pos, &len,
                            "%s{"
                               "\"up\":%llu,\"ub\":%llu,\"ut\":\"%s:%d %s:%d\","
                               "\"dp\":%llu,\"db\":%llu,\"dt\":\"%s:%d %s:%d\""
                              "}",
                            (j ? "," : ""),
                            c[i].us.pkts,
                            c[i].us.bytes,
                            us_src, c[i].us.l4src,
                            us_dst, c[i].us.l4dst,
                            c[i].ds.pkts,
                            c[i].ds.bytes,
                            ds_src, c[i].ds.l4src,
                            ds_dst, c[i].ds.l4dst);
      j++;

      if (ret < 0)
         goto out_free_data;
   }

   ret = sprintf_realloc(&data, &pos, &len, "]]");
   if (ret < 0)
      goto out_free_data;

   /* send response to client */
   wsd_append_response(state, data);

out_free_data:
   free(data);
out_free_connections:
   free(connections);
}

static void dpi_get_queues(struct wsd_cmn_state *state, char *args)
{
   struct dc_table *table;
   struct dc_qos_info *info;
   struct dc_qos_queue *entry;
   char *data;
   int len = TMP_BUFFER_SIZE;
   int pos = 0;
   int ret, i;

   /* update qos info and queues */
   dpictl_update(DPICTL_UPDATE_QOS_QUEUES);

   info = dpictl_get_qos_info();
   if (!info)
      return;
   table = dpictl_get_qos_queues();
   if (!table)
      goto out_free_info;
   entry = (struct dc_qos_queue *) table->data;

   data = malloc(len);
   if (!data) {
      printf("%s: unable to allocate response data\n", __func__);
      goto out_free_table;
   }

   pos += sprintf(&data[pos],
                  "[\"queues\",{"
                  "\"ds_num\":%d,\"ds_default\":%d,"
                  "\"ds_bypass\":%d,\"overall_bw\":%u,"
                  "\"us_num\":%d,\"us_default\":%d,"
                  "\"us_bypass\":%d,"
                  "\"queues\":{",
                  info->ds.num_queues,
                  info->ds.default_queue,
                  info->ds.bypass_queue,
                  info->ds.overall_bw * 1000,
                  info->us.num_queues,
                  info->us.default_queue,
                  info->us.bypass_queue);

   for (i = 0; i < table->entries; i++) {
      ret = sprintf_realloc(&data, &pos, &len,
                            "%s\"%d\":{\"valid\":%d,\"min\":%u,\"max\":%u}",
                            (i ? "," : ""),
                            entry[i].id,
                            entry[i].valid,
                            entry[i].min_kbps * 1000,
                            entry[i].max_kbps * 1000);
      if (ret < 0)
         goto err_free_data;
   }

   ret = sprintf_realloc(&data, &pos, &len, "}}]");
   if (ret < 0)
      goto err_free_data;

   /* send response to client */
   wsd_append_response(state, data);

err_free_data:
   free(data);
out_free_table:
   free(table);
out_free_info:
   free(info);
}

int wsd_callback_dpi(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len)
{
   struct wsd_cmn_state *state = (struct wsd_cmn_state*) user;
   int ret = 0;

   switch(reason)
   {
      case LWS_CALLBACK_ESTABLISHED:
         lwsl_info("%s: LWS_CALLBACK_ESTABLISHED\n", __func__);
         state->session.sessionKey = 0;
         /* save the wsi pointer in our state */
         state->wsi = wsi;
         break;

      case LWS_CALLBACK_CLOSED:
      {
         struct wsd_cmn_response *resp = state->responses;
         while(resp) {
            struct wsd_cmn_response *cur = resp;
            resp = resp->next;
            free(cur);
         }
      }
      break;

      case LWS_CALLBACK_SERVER_WRITEABLE:
          ret = wsd_callback_serverwritable(state, wsi);
          break;

      case LWS_CALLBACK_RECEIVE:
      {
         char *cmd = NULL;
         char *args = NULL;

         cmd = strtok((char*)in, " ");
         if (cmd)
            args = strtok(NULL, " ");
         if (!state->session.sessionKey && strcmp(cmd, "key") != 0) {
            cmsLog_error("command '%s' received without session key", cmd);
            break;
         }

         /* parse the command */
         if (strcmp(cmd, "key") == 0)
            dpi_handle_sessionkey(state, args);
         else if (strcmp(cmd, "appnames") == 0)
            dpi_get_app_names(state, args);
         else if (strcmp(cmd, "devicenames") == 0)
            dpi_get_device_names(state, args);
         else if (strcmp(cmd, "appinsts") == 0)
            dpi_get_appinsts(state, args);
         else if (strcmp(cmd, "flows") == 0)
            dpi_get_flows(state, args);
         else if (strcmp(cmd, "queues") == 0)
            dpi_get_queues(state, args);
         else
            cmsLog_error("received invalid command '%s'", cmd);
      }
      break;

      default:
         break;
   }

   return ret < 0 ? ret : 0;
}


#endif   // SUPPORT_DPI
#endif   // SUPPORT_WEB_SOCKETS

