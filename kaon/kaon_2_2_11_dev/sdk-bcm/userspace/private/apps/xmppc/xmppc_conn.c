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

#ifdef SUPPORT_XMPP

#include <time.h>
#include <strophe/strophe.h>

#include "cms_core.h"
#include "cms_msg.h"
#include "cms_qdm.h"
#include "cms_util.h"

#include "xmppc_conn.h"

#define DEFAULT_XMPP_KEEP_ALIVE_TIME (30*1000)     // ms
#define DEFAULT_XMPP_DELAY_SEND_CONN_UP_TIME 250  // ms
static int64_t keepAliveInterval = -1; 
static void xmppc_conn_update_keepAlive(xmpp_conn_t * const conn, int64_t keepAlive);
/*
 * extern variables
 */

extern void *xmppc_msg_hndl;

/*========================= PRIVATE FUNCTIONS ==========================*/


/* XMPP sends a message to TR69c asking it to establish a connection with
 * its predetermined ACS connection with "6 Connection Request"
 */
static CmsRet xmpp_send_request_msg_to_tr69c(void)
{
    CmsMsgHeader msg;
    CmsRet ret;

    memset(&msg, 0, sizeof(CmsMsgHeader));

    msg.type = CMS_MSG_XMPP_REQUEST_SEND_CONNREQUEST_EVENT;
    msg.src = EID_XMPPC;
    msg.dst = EID_TR69C;
    msg.flags_request = 1;

    ret = cmsMsg_sendAndGetReply(xmppc_msg_hndl, &msg);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("Request to send connection request event was denied, ret=%d", ret);
    }
    else
    {
       cmsLog_debug("Request to send connection request event was successful, ret=%d", ret);
    }

    return ret;
}

static int xmpp_send_connection_up(xmpp_conn_t * const conn  __attribute__((unused)),
                                   void *userdata  __attribute__((unused)))
{
    CmsMsgHeader msg;
    CmsRet ret;

    memset(&msg, 0, sizeof(CmsMsgHeader));

    msg.type = CMS_MSG_XMPP_CONNECTION_UP;
    msg.src = EID_XMPPC;
    msg.dst = EID_SMD;
    msg.flags_event = 1;

    ret = cmsMsg_send(xmppc_msg_hndl, &msg);
    if (ret != CMSRET_SUCCESS)
    {
       cmsLog_error("Request to send connection up event was denied, ret=%d", ret);
    }
    else
    {
       cmsLog_debug("Request to send connection up event was successful, ret=%d", ret);
    }

    return 0;
}

static CmsRet xmppc_update_conn_info
    (const char *jabberID,
     UBOOL8 connected)
{
    char   dateTimeBuf[BUFLEN_64];
    UBOOL8 found = FALSE;
    CmsRet ret = CMSRET_OBJECT_NOT_FOUND;
    Dev2XmppConnObject *xmppConn = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    while ((ret = cmsLck_acquireLockWithTimeout(XMPPC_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
       cmsLog_notice("could not get lock, ret=%d! try again", ret);
       usleep(100);
    }

    while ((!found) &&
           (cmsObj_getNext(MDMOID_DEV2_XMPP_CONN,
                           &iidStack,
                           (void **) &xmppConn) == CMSRET_SUCCESS))
    {
        if (cmsUtl_strcmp(jabberID, xmppConn->jabberID) == 0)
        {
            if (connected == TRUE)
            {
                CMSMEM_REPLACE_STRING(xmppConn->status, "Up");
                if (xmppConn->useTLS == TRUE)
                    xmppConn->TLSEstablished = TRUE;
                else
                    xmppConn->TLSEstablished = FALSE;
            }
            else
            {
                CMSMEM_REPLACE_STRING(xmppConn->status, "Down");
                xmppConn->TLSEstablished = FALSE;
            }

            cmsTms_getXSIDateTime(0, dateTimeBuf, sizeof(dateTimeBuf));
            CMSMEM_REPLACE_STRING(xmppConn->lastChangeDate, dateTimeBuf);

            if ((ret = cmsObj_setFlags(xmppConn,
                                       &iidStack,
                                       OSF_NO_RCL_CALLBACK |
                                       OSF_NO_ACCESSPERM_CHECK)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Update xmppc connection with jabberID %s failed, ret=%d",
                             jabberID, ret);
            }

            found = TRUE;
        }

        cmsObj_free((void **) &xmppConn);
    }

    cmsLck_releaseLock();

    return ret;
}


static xmpp_stanza_t* xmppc_create_error_iq
    (xmpp_conn_t * const conn,
     xmpp_stanza_t * const stanza,
     xmpp_error_type_t error_type,
     void * const userdata)
{
    xmpp_stanza_t *stz_iq, *stz_error, *error_condition, *child = NULL;
    PXMPPC_DATA xmppcData = (PXMPPC_DATA) userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)xmppcData->context;

    stz_error = xmpp_stanza_new(ctx);
    error_condition = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(stz_error, "error"); 
    xmpp_stanza_set_ns(error_condition, XMPP_IETF_NS);

    switch(error_type)
    {
        case XMPP_SE_RESOURCE_CONSTRAINT:
            xmpp_stanza_set_name(error_condition, "resource-constraint");
            xmpp_stanza_set_type(stz_error, XMPP_CONN_REQUEST_RESPONSE_WAIT);
            break;
        case XMPP_SE_NOT_AUTHORIZED:
            xmpp_stanza_set_name(error_condition, "not-authorized");
            xmpp_stanza_set_type(stz_error, XMPP_CONN_REQUEST_RESPONSE_CANCEL);
            break;
        case XMPP_SE_BAD_FORMAT:
            xmpp_stanza_set_name(error_condition, "bad-request");
            xmpp_stanza_set_type(stz_error, XMPP_CONN_REQUEST_RESPONSE_MODIFY);
            break;
        case XMPP_SE_INVALID_FROM:
            xmpp_stanza_set_name(error_condition, "not-allowed");
            xmpp_stanza_set_type(stz_error, XMPP_CONN_REQUEST_RESPONSE_CANCEL);
            break;
        default:
            break;
    }

    xmpp_stanza_add_child(stz_error, error_condition);
    xmpp_stanza_release(error_condition);
    

    // create a iq stanza which type=error
    stz_iq = xmpp_iq_new(ctx, XMPP_TYPE_ERROR, xmpp_stanza_get_id(stanza)); 
    xmpp_stanza_set_from(stz_iq, xmpp_conn_get_jid(conn));
    xmpp_stanza_set_to(stz_iq, xmpp_stanza_get_attribute(stanza, "from"));

    //xmpp_stanza_add_child(stz_iq, xmpp_stanza_get_children(stanza));
    if (xmpp_stanza_get_child_by_name(stz_iq, "connectionRequest") != NULL)
    {
        child = xmpp_stanza_copy(xmpp_stanza_get_children(stanza));
        xmpp_stanza_add_child(stz_iq, child);
        xmpp_stanza_release(child);
    }

    xmpp_stanza_add_child(stz_iq, stz_error);
    xmpp_stanza_release(stz_error);

    return stz_iq;
}


static int xmppc_version_handler
    (xmpp_conn_t * const conn,
     xmpp_stanza_t * const stanza,
     void * const userdata)
{
    xmpp_stanza_t *reply, *query, *name, *version, *text;
    const char *ns = NULL;
    PXMPPC_DATA xmppcData = (PXMPPC_DATA) userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)xmppcData->context;
    cmsLog_notice("Received version request from %s", xmpp_stanza_get_attribute(stanza, "from"));
    
    // refresh keepAlive timeout
    xmppc_conn_update_keepAlive(conn, keepAliveInterval);

    reply = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reply, "iq");
    xmpp_stanza_set_type(reply, "result");
    xmpp_stanza_set_id(reply, xmpp_stanza_get_id(stanza));
    xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

    query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, "query");
    ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
    if (ns)
    {
        xmpp_stanza_set_ns(query, ns);
    }

    name = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(name, "name");
    xmpp_stanza_add_child(query, name);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "xmppc");
    xmpp_stanza_add_child(name, text);

    version = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(version, "version");
    xmpp_stanza_add_child(query, version);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "1.0");
    xmpp_stanza_add_child(version, text);

    xmpp_stanza_add_child(reply, query);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);
    
    return 1;
}


static void xmppc_do_conn_req
    (xmpp_conn_t * const conn,
     xmpp_stanza_t * const stanza,
     xmpp_stanza_t * const stz_conn_req,
     void * const userdata)
{
    char *pInText = NULL;
    char username[BUFLEN_256], pwd[BUFLEN_256];
    char mngrServerConnReqUsername[BUFLEN_256] = {0};
    char mngrServerConnReqPassword[BUFLEN_256] = {0};
    char mngrServerUsername[BUFLEN_256] = {0};
    char mngrServerPassword[BUFLEN_256] = {0};
    char url[BUFLEN_256] = {0};
    CmsRet ret;
    UBOOL8 hasError = FALSE;
    xmpp_error_type_t error_type = XMPP_SE_XML_NOT_WELL_FORMED;
    xmpp_stanza_t *stz_resp = NULL;
    PXMPPC_DATA xmppcData = (PXMPPC_DATA) userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)xmppcData->context;

    if (xmpp_stanza_get_child_by_name(stz_conn_req, "username") &&
        xmpp_stanza_get_child_by_name(stz_conn_req, "password"))
    {
        pInText = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stz_conn_req, "username"));
        cmsUtl_strcpy(username, pInText);
        pInText = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stz_conn_req, "password"));
        cmsUtl_strcpy(pwd, pInText);

        /* TR69 requirement: username/pwd in this stanza must match ManagementServer.ConnectionRequestUsername/Password */
        if ((ret = cmsLck_acquireLockWithTimeout(XMPPC_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
        {
           cmsLog_error("could not get lock, ret=%d", ret);
           hasError = TRUE;
           error_type = XMPP_SE_RESOURCE_CONSTRAINT;
        }
        else
        {
           if ((ret = qdmTr69c_getManagementServerCfgLocked(url,mngrServerConnReqUsername,mngrServerConnReqPassword,mngrServerUsername,mngrServerPassword)) != CMSRET_SUCCESS)
           {
              cmsLog_error("could not get management server configuration, ret=%d", ret);
              hasError = TRUE;
              error_type = XMPP_SE_RESOURCE_CONSTRAINT;
           }
           else
           {
              if (strcmp(username, mngrServerConnReqUsername) != 0)
              {
                 cmsLog_error("username comparison fails %s", username);
                 hasError = TRUE;
                 error_type = XMPP_SE_NOT_AUTHORIZED;
              }
              if (strcmp(pwd, mngrServerConnReqPassword) != 0)
              {
                 cmsLog_error("password comparison fails %s", pwd);
                 hasError = TRUE;
                 error_type = XMPP_SE_NOT_AUTHORIZED;
              }
           }
           cmsLck_releaseLock();
        } /* lock acquired */
    }
    else
    {
        hasError = TRUE;
        error_type = XMPP_SE_BAD_FORMAT;
    }

    if (hasError == FALSE)
    {
       /* send message to tr69c client to send connReq to ACS */
       if (xmpp_send_request_msg_to_tr69c() != CMSRET_SUCCESS)
       {
          hasError = TRUE;
          error_type = XMPP_SE_RESOURCE_CONSTRAINT;
       }
    }

    if (hasError == FALSE)
    {
        // create response iq with empty body
        stz_resp = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(stz_resp, "iq");
        xmpp_stanza_set_type(stz_resp, "result");
        xmpp_stanza_set_id(stz_resp, xmpp_stanza_get_id(stanza));
        xmpp_stanza_set_attribute(stz_resp, "to", xmpp_stanza_get_attribute(stanza, "from"));
    }
    else
    {
        stz_resp = xmppc_create_error_iq(conn, stanza, error_type, userdata);
    }

    xmpp_send(conn, stz_resp);
    xmpp_stanza_release(stz_resp);
}


static int checkAllowedJabberIDs(char* allowList, const char* fromID)
{
    char *allowedID;
    char *str, *saveptr;
    int ret = 0;

    for (str = allowList; ;str=NULL)
    {
        allowedID = strtok_r(str, ",", &saveptr);
        if (allowedID == NULL)
            break;

        if (strcmp(allowedID, fromID) == 0)
            ret = 1;
    }

    return ret;
}


static int xmppc_conn_req_handler
    (xmpp_conn_t * const conn,
     xmpp_stanza_t * const stanza,
     void * const userdata)
{
    char connPath[BUFLEN_64];
    char allowedJabberIds[BUFLEN_1024]; 
    char connReqJabberId[BUFLEN_64];
    CmsRet ret = CMSRET_SUCCESS;
    xmpp_stanza_t *stz_conn_req = NULL;

    // refresh keepAlive timeout
    xmppc_conn_update_keepAlive(conn, keepAliveInterval);

    if (!xmpp_stanza_get_attribute(stanza, "from"))
    {
        xmpp_stanza_t *error_iq = xmppc_create_error_iq(conn, stanza, XMPP_SE_INVALID_FROM, userdata); 
        xmpp_send(conn, error_iq);
        xmpp_stanza_release(error_iq);

        cmsLog_notice("Received IQ without from attribute");
        return 1;
    }

    cmsLog_notice("Received connection request from %s", xmpp_stanza_get_attribute(stanza, "from"));

    if((ret = cmsLck_acquireLockWithTimeout(XMPPC_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
        xmpp_stanza_t *error_iq = xmppc_create_error_iq(conn, stanza, XMPP_SE_RESOURCE_CONSTRAINT, userdata); 
        xmpp_send(conn, error_iq);
        xmpp_stanza_release(error_iq);
        cmsLog_notice("could not get lock, ret=%d! try again", ret);
        return 1;
    }

    if (qdmTr69c_getManagementServerXmppCfgLocked(connPath, allowedJabberIds, connReqJabberId) == CMSRET_SUCCESS)
    {
        // Check from:jabberID is in allowedJabberIDs
        if (strlen(allowedJabberIds) == 0 ||
            checkAllowedJabberIDs(allowedJabberIds, xmpp_stanza_get_attribute(stanza, "from")) == 1)
        {
            stz_conn_req = xmpp_stanza_get_child_by_name(stanza, "connectionRequest");
        }
        else // from not allowed jabber ID
        {
            xmpp_stanza_t *error_iq = xmppc_create_error_iq(conn, stanza, XMPP_SE_INVALID_FROM, userdata); 
            xmpp_send(conn, error_iq);
            xmpp_stanza_release(error_iq);

            cmsLog_error("JabberID:%s is not an allowed jabber ID(AllowedIDs:%s)", xmpp_stanza_get_attribute(stanza, "from"), allowedJabberIds);
            cmsLck_releaseLock();
            return 1;
        }
    }
    else
    {
        xmpp_stanza_t *error_iq = xmppc_create_error_iq(conn, stanza, XMPP_SE_INVALID_FROM, userdata); 
        xmpp_send(conn, error_iq);
        xmpp_stanza_release(error_iq);

        cmsLog_error("Can not access Management Server");
        cmsLck_releaseLock();
        return 1;
    }

    cmsLck_releaseLock();

    if (stz_conn_req != NULL)
    {
        xmppc_do_conn_req(conn, stanza, stz_conn_req, userdata);
    }
    else
    {
        xmpp_stanza_t *error_iq = xmppc_create_error_iq(conn, stanza, XMPP_SE_BAD_FORMAT, userdata); 
        xmpp_send(conn, error_iq);
        xmpp_stanza_release(error_iq);

        cmsLog_error("Received iq from %s without connectionRequest", xmpp_stanza_get_attribute(stanza, "from"));
    }

    return 1;
}

static int xmppc_conn_keepAlive(xmpp_conn_t * const conn, void *userdata  __attribute__((unused)))
{
    xmpp_send_raw_string(conn, " ");

    return 1;
}


static void xmppc_conn_update_keepAlive(xmpp_conn_t * const conn, int64_t keepAlive)
{
    xmpp_timed_handler_delete(conn, xmppc_conn_keepAlive);

    if (keepAlive > 0)
    {
        xmpp_timed_handler_add(conn,
                               xmppc_conn_keepAlive,
                               keepAlive*1000,
                               NULL);
    }
    else if (keepAlive == -1)
    {
        xmpp_timed_handler_add(conn,
                               xmppc_conn_keepAlive,
                               DEFAULT_XMPP_KEEP_ALIVE_TIME,
                               NULL);
    }

    keepAliveInterval = keepAlive;
}


/* define a handler for cpe connection request response */
static void xmppc_cpe_conn_handler
    (xmpp_conn_t * const conn,
     const xmpp_conn_event_t status,
     const int error  __attribute__((unused)),
     xmpp_stream_error_t * const stream_error  __attribute__((unused)),
     void * const userdata)
{
    PXMPPC_DATA xmppcData = (PXMPPC_DATA) userdata;
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)xmppcData->context;

    if (status == XMPP_CONN_CONNECT)
    {
        xmpp_stanza_t* stz_pres;

        cmsLog_notice("DEBUG: connected\n");

        xmpp_handler_add(conn,
                         xmppc_version_handler,
                         "jabber:iq:version",
                         "iq",
                         NULL,
                         userdata);
        xmpp_handler_add(conn,
                         xmppc_conn_req_handler,
                         "jabber:client",
                         "iq",
                         "get",
                         userdata);

        stz_pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(stz_pres, "presence");
        /* Send initial <presence/> to appear online to contacts */
        xmpp_send(conn, stz_pres);
        xmpp_stanza_release(stz_pres);

        /* Update XMPPC connection status */
        xmppc_update_conn_info(xmpp_conn_get_jid(conn), TRUE);
        xmppcData->isDisconnected = -1;

        // delay a while to send connection up event
        xmpp_timed_handler_add(conn, xmpp_send_connection_up, DEFAULT_XMPP_DELAY_SEND_CONN_UP_TIME, NULL);
    }
    else
    {
        time_t now = time(NULL);
        xmppcData->isDisconnected = 1;
        cmsLog_notice("DEBUG: disconnected\n");

        srand(now);
        xmpp_handler_delete(conn, xmppc_version_handler);
        xmpp_handler_delete(conn, xmppc_conn_req_handler);
        xmpp_timed_handler_delete(conn, xmppc_conn_keepAlive);

        // random backoff time for re-establish connection
        xmppcData->backoffTime = now + (random()%60);
        /* Update XMPPC connection status */
        xmppc_update_conn_info(xmpp_conn_get_jid(conn), FALSE);
    }
}


static void xmppc_conn_setup
    (xmpp_ctx_t * const ctx  __attribute__((unused)),
     xmpp_conn_t * const conn,
     PXMPPC_DATA xmpp_data)
{
    long flags = xmpp_conn_get_flags(conn);
    /* setup authentication information */
    xmpp_conn_set_jid(conn, xmpp_data->jabberID);
    xmpp_conn_set_pass(conn, xmpp_data->password);

    if (xmpp_data->enableTLS)
       flags |= XMPP_CONN_FLAG_TRUST_TLS;
    else
       flags |= XMPP_CONN_FLAG_DISABLE_TLS;

    xmpp_conn_set_flags(conn, flags);

    /* initiate connection */
    if (xmpp_connect_client(conn, NULL, 0, xmppc_cpe_conn_handler, xmpp_data) == 0)
    {
        xmpp_data->isDisconnected = 0;
        xmpp_data->backoffTime = 0;
        xmppc_conn_update_keepAlive(conn, xmpp_data->keepAlive);
    }
}


/*========================= PUBLIC FUNCTIONS ==========================*/

void* xmppc_conn_preinit(int loglevel)
{
    xmpp_log_t *log = NULL;
    xmpp_ctx_t *ctx = NULL;

    srand(time(NULL));
    /* init XMPP library */
    xmpp_initialize();

    /* create a XMPP context */
    log = xmpp_get_default_logger(loglevel); /* pass NULL instead to silence output */

    ctx = xmpp_ctx_new(NULL, log);

    return (void*)ctx;
}

void xmppc_conn_cleanup(void *handle)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)handle;

    /* release context */
    xmpp_ctx_free(ctx);

    /* final shutdown of the library */
    xmpp_shutdown();
}

CmsRet xmppc_conn_create(void *handle, PXMPPC_DATA xmpp_data)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)handle;
    xmpp_conn_t *conn = xmpp_conn_new(ctx);

    if (conn != NULL)
    {
        xmpp_data->conn = (void*)conn;
        xmpp_data->context = handle;

        xmpp_data->isDisconnected = 1;
        /* setup connection -- support truncated binary exponential backoff */
        xmppc_conn_setup(ctx, conn, xmpp_data);

        return CMSRET_SUCCESS;
    }
    return CMSRET_INTERNAL_ERROR;
}

CmsRet xmppc_conn_delete(PXMPPC_DATA xmpp_data)
{
    xmpp_conn_t *conn = (xmpp_conn_t*) xmpp_data->conn;

    /* release our connection*/
    xmpp_conn_release(conn);

    memset(xmpp_data, 0, sizeof(XMPPC_DATA));

    return CMSRET_SUCCESS;
}

void xmppc_conn_updateall(PXMPPC_DATA xmpp_data)
{
    xmpp_conn_t *conn = (xmpp_conn_t*) xmpp_data->conn;

    xmppc_conn_update_keepAlive(conn, xmpp_data->keepAlive);
}

void xmppc_conn_stack(void *handle, XMPPC_DATA xmpp_data[], int size)
{
    int i = 0;
    time_t now = time(NULL);
    xmpp_ctx_t *ctx = (xmpp_ctx_t*) handle;

    for (i = 0 ; i < size ; i ++)
    {
        PXMPPC_DATA xmppcData = &xmpp_data[i];
        xmpp_conn_t *conn = (xmpp_conn_t *)xmppcData->conn;

        if (xmppcData->isDisconnected > 0 && now > xmppcData->backoffTime)
        {
            if (xmppcData->isDisconnected < 10)
               xmppcData->isDisconnected += 1;

            xmppc_conn_setup(ctx, conn, xmppcData);

            // setup fail
            if ( xmppcData->isDisconnected > 0 && xmppcData->backoffTime != 0)
                xmppcData->backoffTime = time(NULL) + (xmppcData->isDisconnected *(random()%60));
        }
    }

    /* send any data that has been queued by xmpp_send
     * and related functions and run through the Strophe
     * even loop a single time, and will not wait more
     * than timeout milliseconds for events.
     */
    xmpp_run_once(ctx,  100);
} 
#endif    // SUPPORT_XMPP


