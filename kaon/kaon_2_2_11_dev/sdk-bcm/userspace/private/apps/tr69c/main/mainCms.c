/*
 * <:copyright-BRCM:2016:proprietary:standard
 *
 *    Copyright (c) 2016 Broadcom
 *    All Rights Reserved
 *
 *  This program is the proprietary software of Broadcom and/or its
 *  licensors, and may only be used, duplicated, modified or distributed pursuant
 *  to the terms and conditions of a separate, written license agreement executed
 *  between you and Broadcom (an "Authorized License").  Except as set forth in
 *  an Authorized License, Broadcom grants no license (express or implied), right
 *  to use, or waiver of any kind with respect to the Software, and Broadcom
 *  expressly reserves all rights in and to the Software and all intellectual
 *  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1. This program, including its structure, sequence and organization,
 *     constitutes the valuable trade secrets of Broadcom, and you shall use
 *     all reasonable efforts to protect the confidentiality thereof, and to
 *     use this information only in connection with your use of Broadcom
 *     integrated circuit products.
 *
 *  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *     PERFORMANCE OF THE SOFTWARE.
 *
 *  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *     LIMITED REMEDY.
 * :>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <string.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netdb.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_msg_modsw.h"
#include "cms_qdm.h"
#include "cms_params_modsw.h"
#include "inc/tr69cdefs.h"

#include "inc/appdefs.h"
#include "inc/utils.h"
#include "bcmLibIF/bcmWrapper.h"
#include "bcmLibIF/bcmConfig.h"
#include "event.h"
#include "informer_public.h"
#include "httpProto.h"

#include "cms_linklist.h"
#include "osgid.h"


extern void proto_Init(void);  /* in protocol.h */
extern void freeAllListeners(void);  /* in event.c */
extern void changeNameSpaceCwmpVersionURL(int version);
extern void handleNotificationLimit(char *notificationLimitName,
                                    int notificationLimitValue,
                                    CmsEventHandler limitInformFunc);
extern void manageableDeviceNotificationLimitFunc(void *handle);
#ifdef SUPPORT_TR69C_VENDOR_RPC
extern void registerVendorSpecificRPCs(void);
extern void unregisterVendorSpecificRPCs(void);
#endif /* SUPPORT_TR69C_VENDOR_RPC */
extern void freeOpReqList(ChangeDuStateOpReqInfo *pOpReqList);
extern UBOOL8 areAllDuStateChangeResponsesReceived
   (ChangeDuStateOpReqInfo *pOpReqList);
#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
extern AutonomousFileTransferStats autonomousCompleteStats;
extern UBOOL8 doSendAutonTransferComplete;
extern CmsRet autonomousFileTypeToFileTypeStr(UINT32 fileType, char **fileStr);
extern void freeInitAutonomousCompleteStats(AutonomousFileTransferStats *pAutonomousCompleteStats);
#endif

#ifdef SUPPORT_STUN
extern void stun_configChanged(void);
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/** external data **/
extern int sendGETRPC;              /* send a GetRPCMetods */
extern UBOOL8 rebootingFlag;        /*set if we are doing system reboot or factoryreset*/
extern UBOOL8 needDisconnect;
extern UBOOL8 loggingSOAP;
extern ACSState acsState;
extern int tr69cTerm;
extern void *tmrHandle;
extern const char *RootDevice;
extern LimitNotificationQInfo limitNotificationList;
extern ChangeDuStateOpReqInfo *pAutonResult;
extern eSessionState  sessionState;
extern eXMPPStatus g_XMPPStatus;
extern HttpTask      httpTask;       /* http io desc */

extern void acsState_cleanup(void);
extern void usage(char *progName);
extern CmsEntityId g_eIdTR69C;

/** public data **/
void *msgHandle = NULL;


/* forward declaration */
void getAndRespondSessionStatus(const CmsMsgHeader *msg);
void deregisterEvents(void);
void allResponseReceived(void);
void main_cleanup(SINT32 code);
UBOOL8 checkDuStateChangePolicy(char *operationTypeList, char *failureCodeList);
extern void goSendAutonomousDuStateComplete(void);
extern void delayedTermFunc(void *handle);
extern void tr69c_sigTermHandler(int sig);
static void registerSmdMessageListener(void);
static void registerInterestInEvent(CmsMsgType msgType, UBOOL8 positive, void *msgData, UINT32 msgDataLen);
static void readMessageFromSmd(void *handle);

static void handleDuStateChangeResponse(const CmsMsgHeader *msg);
static void handleAutonDuStateChangeResponse(const CmsMsgHeader *msg);

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
static void handleAutonomousTransferComplete(const CmsMsgHeader *msg);
extern CmsRet autonomousFileTypeToFileTypeStr(UINT32 fileType, char **fileStr);
extern void freeInitAutonomousCompleteStats(AutonomousFileTransferStats *pAutonomousCompleteStats);
extern void goSendAutonomousTransferComplete(void);
extern UBOOL8 checkAutonomousTransferCompletePolicy(UBOOL8 isDownload, UINT32 failureCode, char *fileTypeStr);
#endif

#ifdef DMP_DSLDIAGNOSTICS_1
int waitingForDiagToBringLinkUp = 0;
#endif
int g_TR069WANIPChanged = -1;
int g_initFinished = FALSE;
static int g_req_id = 0;
/*
 * Normally, the connection request server socket is opened by the smd, and inherited
 * by tr69c when smd forks/exec's tr69c.  But for unittests, we may want to start tr69c
 * by itself, so we need to be able to tell tr69c to open its own server socket.
 */
extern UBOOL8 openConnReqServerSocket;



#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

/* the extended 2nd entity map */
#if defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID)
const static sOidMap g_OidMapE2E [] =
{
   /* TODO:: open this if needs to enable multiple tr69c supporting in TR98 */

   //{.stdOid = MDMOID_MANAGEMENT_SERVER,                                                .eeOid = MDMOID_E2_MANAGEMENT_SERVER},

   {.stdOid = INVALIDE_MDM_MAX_OID, /* ensure this is the last one!! */                      .eeOid = INVALIDE_MDM_MAX_OID},
};

#else /* Pure TR181 */
const static sOidMap g_OidMapE2E [] =
{
   {.stdOid = MDMOID_TR69C_CFG,                                                        .eeOid = MDMOID_E2_TR69C_CFG},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER,                                           .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,                         .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE},
   {.stdOid = MDMOID_DEV2_AUTON_XFER_COMPLETE_POLICY,                                  .eeOid = MDMOID_E2_DEV2_AUTON_XFER_COMPLETE_POLICY},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY,                     .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_ANNOUNCEMENT,        .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_ANNOUNCEMENT},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_ANNOUNCEMENT_GROUP,  .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_ANNOUNCEMENT_GROUP},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_QUERY,               .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_DOWNLOAD_AVAILABILITY_QUERY},
   {.stdOid = MDMOID_DEV2_DU_STATE_CHANGE_COMPL_POLICY,                                .eeOid = MDMOID_E2_DEV2_DU_STATE_CHANGE_COMPL_POLICY},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_EMBEDDED_DEVICE,                           .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_EMBEDDED_DEVICE},
   {.stdOid = MDMOID_DEV2_MANAGEMENT_SERVER_VIRTUAL_DEVICE,                            .eeOid = MDMOID_E2_DEV2_MANAGEMENT_SERVER_VIRTUAL_DEVICE},

   {.stdOid = INVALIDE_MDM_MAX_OID, /* ensure this is the last one!! */                      .eeOid = INVALIDE_MDM_MAX_OID},
};
#endif // defined(SUPPORT_DM_LEGACY98) || defined(SUPPORT_DM_HYBRID)


#if defined(BRCM_PKTCBL_SUPPORT)
/* the Blacklist of RpcMethods which aren't supported by tr69c_2 */
const static eRPCMethods g_e2eRpcMethodsBL [] =
{
   rpcUnknown,
   rpcAddObject,
   rpcDeleteObject,
   rpcReboot,
   rpcDownload,
   rpcUpload,
   rpcGetQueuedTransfers,
   rpcFactoryReset,
   rpcChangeDuState,

   INVALID_MAX_RPC_METHOD /* ensure this is the last one!! */
};
#else // defined(BRCM_PKTCBL_SUPPORT)
/* stub */
const static eRPCMethods g_e2eRpcMethodsBL [] =
{
   //rpcUnknown,

   INVALID_MAX_RPC_METHOD /* ensure this is the last one!! */
};
#endif //defined(BRCM_PKTCBL_SUPPORT)

#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)


/*
 *  Function: Re-map stdOid(standard Oid) to eeOid(extended entity Oid)
 *  Return: eeOid if it is existed; otherwise, still return stdOid.
 */
MdmObjectId stdOid2EeOid (MdmObjectId stdOid)
{
   /* no need to remap for EID_TR69C */
   if (EID_TR69C == g_eIdTR69C)
   {
      return stdOid;
   }

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   if (EID_TR69C_2 == g_eIdTR69C)
   {
      const sOidMap *OidMapEe;
      int i = 0;
      OidMapEe = g_OidMapE2E;
      while(INVALIDE_MDM_MAX_OID != OidMapEe[i].stdOid)
      {
         if (stdOid == OidMapEe[i].stdOid)
         {
            cmsLog_debug("found eeOid(%d) for stdOid(%d)", OidMapEe[i].eeOid, stdOid);
            return OidMapEe[i].eeOid;
         }
         i++;
      }
   }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

   /* no eeOid for given stdOid, so eeOid == stdOid */
   return stdOid;
}


/*
 *  Function: Re-map eeOid(extended entity Oid) to stdOid(standard Oid)
 *  Return: stdOid if it is existed; otherwise, still return eeOid.
 */
MdmObjectId eeOid2StdOid (MdmObjectId eeOid)
{
   /* no need to remap for EID_TR69C */
   if (EID_TR69C == g_eIdTR69C)
   {
      return eeOid;
   }

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   if (EID_TR69C_2 == g_eIdTR69C)
   {
      const sOidMap *OidMapEe;
      int i = 0;
      OidMapEe = g_OidMapE2E;
      while(INVALIDE_MDM_MAX_OID != OidMapEe[i].eeOid)
      {
         if (eeOid == OidMapEe[i].eeOid)
         {
            cmsLog_debug("found stdOid(%d) for eeOid(%d)", OidMapEe[i].stdOid, eeOid);
            return OidMapEe[i].stdOid;
         }
         i++;
      }
   }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

   /* no stdOid for given eeOid, so stdOid == eeOid */
   return eeOid;
}


/*
 *  Function: get standard pathDesc or extended entity pathDesc by fullpath
 *  Return: extended entity pathDesc if it is existed; otherwise, return standard pathDesc.
 */
CmsRet tr69c_fullPathToPathDescriptor(const char *fullpath, MdmPathDescriptor *pathDesc)
{
   CmsRet ret;
   ret = cmsMdm_fullPathToPathDescriptor(fullpath, pathDesc);
   if (CMSRET_SUCCESS != ret)
   {
      return ret;
   }

   if (EID_TR69C_2 == g_eIdTR69C)
   {
      pathDesc->oid = stdOid2EeOid(pathDesc->oid);
   }

   return ret;
}


/*
 *  Function: get standard fullpath by pathDesc
 *  Input: standard pathDesc or extended entity pathDesc
 *  Return: standard fullpath.
 */
CmsRet tr69c_pathDescriptorToStdFullPath(const MdmPathDescriptor *pathDesc, char **fullpath)
{
   if (EID_TR69C_2 == g_eIdTR69C)
   {
      MdmPathDescriptor tmp_pathDesc;

      memcpy(&tmp_pathDesc, pathDesc, sizeof(MdmPathDescriptor));
      tmp_pathDesc.oid = eeOid2StdOid(tmp_pathDesc.oid);
      return cmsMdm_pathDescriptorToFullPath(&tmp_pathDesc, fullpath);
   }

   return cmsMdm_pathDescriptorToFullPath(pathDesc, fullpath);
}


/*
 *  Function: test whether the RpcMethod is supported
 *  Input: RpcMethod
 *  Return: TRUE/FALSE.
 */
UBOOL8 isRpcMethodSupport(eRPCMethods m)
{
   cmsLog_debug("In: RPCMethod(%d)", m);

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
   const eRPCMethods *RpcMethodsBL = NULL;
   int i = 0;

   if (EID_TR69C_2 == g_eIdTR69C)
   {
      RpcMethodsBL = g_e2eRpcMethodsBL;
      while(INVALID_MAX_RPC_METHOD != RpcMethodsBL[i])
      {
         if (m == RpcMethodsBL[i])
         {
            cmsLog_debug("RPCMethod(%d) not support", m);
            return FALSE;
         }
         i++;
      }
   }
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

   return TRUE; /* support all by default */
}


void deregisterEvents(void)
{
   registerInterestInEvent(CMS_MSG_ACS_CONFIG_CHANGED, FALSE, NULL, 0);
   registerInterestInEvent(CMS_MSG_TR69C_CONFIG_CHANGED, FALSE, NULL, 0);
   registerInterestInEvent(CMS_MSG_TR69_ACTIVE_NOTIFICATION, FALSE, NULL, 0);
   registerInterestInEvent(CMS_MSG_WAN_CONNECTION_UP, FALSE, NULL, 0);
   registerInterestInEvent(CMS_MSG_RESPONSE_DU_STATE_CHANGE, FALSE, NULL, 0);
   registerInterestInEvent(CMS_MSG_DIAG, FALSE, NULL, 0);
#ifdef SUPPORT_STUN
   registerInterestInEvent(CMS_MSG_STUN_CONFIG_CHANGED, FALSE, NULL, 0);
#endif
}

/*
 * Initialize all the various tasks
 */
static void initTasks(void)
{
   /* INIT Protocol http, ssl */
   proto_Init();

   /* initialize tr69c listener for any future messages from smd */
   registerSmdMessageListener();

#ifdef SUPPORT_TR69C_VENDOR_RPC
   /* do vendor specific registration */
   registerVendorSpecificRPCs();
#endif /* SUPPORT_TR69C_VENDOR_RPC */

   /* Just booted so send initial Inform */
   initInformer();

   /* Sync tmrHandle from smd DelayMsg timer list*/
   syncPeriodicInformFromDelayMsg();

}  /* End of initTasks() */


static void initLoggingFromConfig(UBOOL8 useConfiguredLogLevel)
{
   Tr69cCfgObject *obj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(TR69C_BOOTUP_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("failed to get lock, ret=%d", ret);
      cmsLck_dumpInfo();
      return;
   }

   if ((ret = cmsObj_get(stdOid2EeOid(MDMOID_TR69C_CFG), &iidStack, 0, (void **) &obj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of TR69C_CFG object failed, ret=%d", ret);
   }
   else
   {
      if (useConfiguredLogLevel)
      {
         cmsLog_setLevel(cmsUtl_logLevelStringToEnum(obj->loggingLevel));
      }

      cmsLog_setDestination(cmsUtl_logDestinationStringToEnum(obj->loggingDestination));

      loggingSOAP = obj->loggingSOAP;
      acsState.noneConnReqAuth = (obj->connectionRequestAuthentication == TRUE) ? 0 : 1;

      cmsObj_free((void **) &obj);
   }

   cmsLck_releaseLock();
}


static void registerSmdMessageListener(void)
{
   SINT32 fd;

   cmsMsg_getEventHandle(msgHandle, &fd);

#ifdef DESKTOP_LINUX
   if (fd == CMS_INVALID_FD)
   {
      /* when running on desktop, we might be in standalone mode for
       * unittests.  In this scenario, we don't have a fd to smd. */
      return;
   }
#endif

   if (fd != CMS_INVALID_FD)
   {
      cmsLog_debug("registering fd=%d with listener for messages from smd", fd);
      setListener(fd, readMessageFromSmd, NULL);
   }
   else
   {
      cmsLog_error("Invalid msg comm fd %d", fd);
   }

   return;
}


static void unregisterSmdMessageListener(void)
{
   SINT32 fd;

   cmsMsg_getEventHandle(msgHandle, &fd);

#ifdef DESKTOP_LINUX
   if (fd == CMS_INVALID_FD)
   {
      /* when running on desktop, we might be in standalone mode for
       * unittests.  In this scenario, we don't have a fd to smd. */
      return;
   }
#endif

   if (fd != CMS_INVALID_FD)
   {
      cmsLog_debug("unregistering fd=%d with listener for messages from smd", fd);
      stopListener(fd);
   }
   else
   {
      cmsLog_error("Invalid msg comm fd %d", fd);
   }

   return;
}


/** Register or unregister our interest for some event events with smd.
 *
 * If a management entity changes the ACS config, it will send out
 * this notification and we will get it, possibly waking us up.
 *
 * @param msgType (IN) The notification message/event that we are
 *                     interested in or no longer interested in.
 * @param positive (IN) If true, then register, else unregister.
 * @param data     (IN) Any optional data to send with the message.
 * @param dataLength (IN) Length of the data
 */
void registerInterestInEvent(CmsMsgType msgType, UBOOL8 positive, void *msgData, UINT32 msgDataLen)
{
   CmsMsgHeader *msg;
   char *data;
   void *msgBuf;
   char *action __attribute__ ((unused)) = (positive) ? "REGISTER" : "UNREGISTER";
   CmsRet ret;


   if (msgData != NULL && msgDataLen != 0)
   {
      /* for msg with user data */
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader) + msgDataLen, ALLOC_ZEROIZE);
   }
   else
   {
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   }

   msg = (CmsMsgHeader *)msgBuf;

   /* fill in the msg header */
   msg->type = (positive) ? CMS_MSG_REGISTER_EVENT_INTEREST : CMS_MSG_UNREGISTER_EVENT_INTEREST;
   msg->src = g_eIdTR69C;
   msg->dst = EID_SMD;
   msg->flags_request = 1;
   msg->wordData = msgType;

   if (msgData != NULL && msgDataLen != 0)
   {
      data = (char *) (msg + 1);
      msg->dataLength = msgDataLen;
      memcpy(data, (char *)msgData, msgDataLen);
   }


   ret = cmsMsg_sendAndGetReply(msgHandle, msg);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("%s_EVENT_INTEREST for 0x%x failed, ret=%d", action, msgType, ret);
   }
   else
   {
      cmsLog_debug("%s_EVENT_INTEREST for 0x%x succeeded", action, msgType);
   }

   cmsMem_free(msgBuf);

   return;
}


/** Make sure we have enough information to start, specifically:
 *  - The acsURL needs to be set
 *  - The interface we are bound to must be up/connected.
 *
 * Side effect: if we need the WAN connection to come up before starting
 * and the WAN connection currently is not up, send a message to smd
 * requesting WAN connection up notification.
 *
 * @return TRUE if we can start, FALSE otherwise.
 */
static UBOOL8 checkStartupPreReqs(void)
{
   UBOOL8 sts = FALSE;

   if (acsState.acsURL == NULL)
   {
      cmsLog_notice("ACS URL is not defined yet.  Return false");
      return sts;
   }


   if (cmsUtl_strcmp(acsState.boundIfName, MDMVS_LAN) == 0)
   {
      CmsRet r2;
      char *ipAddr=NULL;

      if ((r2 = cmsLck_acquireLockWithTimeout(TR69C_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get lock, ret=%d", r2);
         return FALSE;
      }

      r2 = getLanIPAddressInfo(&ipAddr, NULL);
      if (r2 == CMSRET_SUCCESS &&
          !cmsUtl_isZeroIpvxAddress(CMS_AF_SELECT_IPVX, ipAddr))
      {
         /* LAN side is really UP */
         sts = TRUE;
      }
      else
      {
         /* LAN side is in DHCP mode and has not acquired IP address yet.
          * XXX Temp: Register interest in WAN interface coming up.
          * Obviously, LAN != WAN, but for now, ssk will also send
          * WAN_CONNECTION_UP event when LAN side (br0) acquires an IP addr.
          */
         cmsLog_debug("boundIfName is LAN, but LAN is not up yet, register event interest");
         registerInterestInEvent(CMS_MSG_WAN_CONNECTION_UP, TRUE, NULL, 0);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(ipAddr);

      cmsLck_releaseLock();
   }
   else if (cmsUtl_strcmp(acsState.boundIfName, MDMVS_LOOPBACK) == 0)
   {
      cmsLog_debug("boundIfName is %s, Loopback is always up", MDMVS_LOOPBACK);
      sts = TRUE;
   }
   else if (cmsUtl_strcmp(acsState.boundIfName, MDMVS_ANY_WAN) == 0)
   {

      if (getRealWanState(NULL) == eWAN_ACTIVE)
      {
         cmsLog_debug("BoundIfName is %s, and one or more WAN connection is up", MDMVS_ANY_WAN);
         sts = TRUE;
      }
      else
      {
         cmsLog_debug("register interest for any WAN connection up");
         registerInterestInEvent(CMS_MSG_WAN_CONNECTION_UP, TRUE, NULL, 0);
#ifdef DMP_DSLDIAGNOSTICS_1
         if (waitingForDiagToBringLinkUp)
         {
            sts = TRUE;
         }
#endif
      }
   }
   else
   {
      /* boundifname must be a specific wan connection */
      cmsLog_debug("boundIfName=%s", acsState.boundIfName);
      if (getRealWanState(acsState.boundIfName) == eWAN_ACTIVE)
      {
         cmsLog_debug("WAN connection %s is up", acsState.boundIfName);
         sts = TRUE;
      }
      else
      {
         cmsLog_debug("register for WAN_CONNECTION_UP event on %s", acsState.boundIfName);
         registerInterestInEvent(CMS_MSG_WAN_CONNECTION_UP, TRUE, acsState.boundIfName, strlen(acsState.boundIfName)+1);
#ifdef DMP_DSLDIAGNOSTICS_1
         if (waitingForDiagToBringLinkUp)
         {
            sts = TRUE;
         }
#endif
      }
   }

   return sts;
}


/* Copy settings from the MDM into acsState.
 * As a side effect, global variable.
 */
void updateTr69cCfgInfo_igd(void)
{
   UrlProto urlProto;
   char *urlAddr, *urlPath;
   UINT16 urlPort=0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   ManagementServerObject *acsCfg = NULL;
   UBOOL8 connReqURLchanged = FALSE;
   CmsRet ret;

   cmsLog_debug("Entered ");

   if ((ret = cmsLck_acquireLockWithTimeout(TR69C_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get lock, ret=%d", ret);
      return;
   }


   /*
    * Fill in our deviceInfo params only if has not been done before.
    * Once we've filled it in, no need to do it again.  It will not
    * change while the system is still up.
    */
   if (acsState.manufacturer == NULL)
   {
      IGDDeviceInfoObject *deviceInfoObj=NULL;

      if ((ret = cmsObj_get(MDMOID_IGD_DEVICE_INFO, &iidStack, 0, (void **) &deviceInfoObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not get device info object!, ret=%d", ret);
      }
      else
      {
         cmsLog_debug("got deviceInfo object");
         cmsLog_debug("%s/%s/%s/%s", deviceInfoObj->manufacturer, deviceInfoObj->manufacturerOUI, deviceInfoObj->productClass, deviceInfoObj->serialNumber);

         CMSMEM_REPLACE_STRING(acsState.manufacturer, deviceInfoObj->manufacturer);
         CMSMEM_REPLACE_STRING(acsState.manufacturerOUI, deviceInfoObj->manufacturerOUI);
         CMSMEM_REPLACE_STRING(acsState.productClass, deviceInfoObj->productClass);
         CMSMEM_REPLACE_STRING(acsState.serialNumber, deviceInfoObj->serialNumber);

         cmsObj_free((void **) &deviceInfoObj);
      }
   }


   /*
    * Get managment server object.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   if ((ret = cmsObj_get(MDMOID_MANAGEMENT_SERVER, &iidStack, 0, (void *) &acsCfg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of MANAGEMENT_SERVER failed, ret=%d", ret);
      cmsLck_releaseLock();
      return;
   }

   /*
    * Check that boundIfName and acsURL are consistent.  But do not change
    * boundIfName for the user.  The user must do that himself.
    */
   if ((acsCfg->URL != NULL) &&
       (cmsUtl_parseUrl(acsCfg->URL, &urlProto, &urlAddr, &urlPort, &urlPath) == CMSRET_SUCCESS))
   {
      if (matchAddrOnLanSide(urlAddr) && cmsUtl_strcmp(acsCfg->X_BROADCOM_COM_BoundIfName, MDMVS_LAN))
      {
         cmsLog_error("ACS URL is on LAN side (%s), but boundIfName is not set to LAN (%s)",
                      urlAddr, acsCfg->X_BROADCOM_COM_BoundIfName);
      }
      else if (((cmsUtl_strcmp(urlAddr, "127.0.0.1") == 0) || (cmsUtl_strcmp(urlAddr, "::1") == 0))
               && cmsUtl_strcmp(acsCfg->X_BROADCOM_COM_BoundIfName, MDMVS_LOOPBACK))
      {
         cmsLog_error("ACS URL is on Loopback (%s), but boundIfName is not set to LOOPBACK (%s)",
                      urlAddr, acsCfg->X_BROADCOM_COM_BoundIfName);
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(urlAddr);
      CMSMEM_FREE_BUF_AND_NULL_PTR(urlPath);
   }


   cmsLck_releaseLock();


   // ACS URL
   /* case 1) : tr69c is alive, if URL is changed, we need to update acsState.acsURL, and reset retryCount
    *              to 0.
    *              Because lastConnectedURL can't be changed before cpe received InformResponse from ACS
    *              successfully, we can't reset retryCount at case 3.
    *
    * case 2) : tr69c is exit normally(such as we only have periodic inform, no retry, and out of select time
    *              out) or cpe just boot, when tr69c is launch again for some reason(value change, periodicinform),
    *		     we need to initialize acsState.acsURL.
    *
    * case 3) : we will add  INFORM_EVENT_BOOTSTRAP in this case, because acsState.acsURL will lose
    *               its value when tr69c exit.(we will set the value of acsCfg->lastConnectedURL when we
    *               receive Informresponse)
    *
    */
   if(acsCfg->URL != NULL)
   {
        /*case 1: tr69c is alive, */
	if(acsState.acsURL != NULL)
	{
	   if (cmsUtl_strcmp(acsState.acsURL, acsCfg->URL) != 0)
      {
         cmsMem_free(acsState.acsURL);
         acsState.acsURL = cmsMem_strdup(acsCfg->URL);
         acsState.retryCount = 0; //reset retryCount when ACS URL is changed.
         acsState.cwmpVersion = CWMP_VERSION_1_2;
         changeNameSpaceCwmpVersionURL(acsState.cwmpVersion);
         needDisconnect = TRUE;
         cmsLog_debug("acsURL changed, add eIEBootStrap inform event");
      }
	}
	else /*case 2 : cpe just boot or tr69c exit normally*/
   {
	   acsState.acsURL = cmsMem_strdup(acsCfg->URL);
   }
	cmsLog_debug("acsState.acsURL=%s", acsState.acsURL);

      /*case 3 */
      if ((acsCfg->lastConnectedURL == NULL) ||
	     (cmsUtl_strcmp(acsCfg->lastConnectedURL, acsCfg->URL) != 0))
      {
         addInformEventToList(INFORM_EVENT_BOOTSTRAP);
         cmsLog_debug("setting acsURL for the first time, add eIEBootStrap event to inform list. lastConnectedURL=%s, acsCfg->URL=%s",
		 	acsCfg->lastConnectedURL, acsCfg->URL);

         /* when bootstrap is sent, parameters on table 5 of TR98 specification
          * needs to be reset back to default Active Notification.
          */
         setDefaultActiveNotification();
      }
   }

   // ACS username
   if (acsState.acsUser!= NULL && acsCfg->username != NULL)
   {
      if (cmsUtl_strcmp(acsState.acsUser, acsCfg->username) != 0)
      {
         cmsMem_free(acsState.acsUser);
         acsState.acsUser = cmsMem_strdup(acsCfg->username);
      }
   }
   else if (acsCfg->username != NULL)
   {
      acsState.acsUser = cmsMem_strdup(acsCfg->username);
   }

   // ACS password
   if (acsState.acsPwd != NULL && acsCfg->password != NULL)
   {
      if (cmsUtl_strcmp(acsState.acsPwd, acsCfg->password) != 0)
      {
         cmsMem_free(acsState.acsPwd);
         acsState.acsPwd = cmsMem_strdup(acsCfg->password);
      }
   }
   else if (acsCfg->password != NULL)
   {
      acsState.acsPwd = cmsMem_strdup(acsCfg->password);
   }

   // connectionRequestURL
   if (acsState.connReqURL != NULL && acsCfg->connectionRequestURL != NULL)
   {
      if (cmsUtl_strcmp(acsState.connReqURL, acsCfg->connectionRequestURL) != 0)
      {
         CMSMEM_REPLACE_STRING(acsState.connReqURL, acsCfg->connectionRequestURL);
         connReqURLchanged = TRUE;
      }
      if ((acsState.connReqIpAddr != NULL) && (acsState.connReqIpAddrFullPath == NULL))
      {
         /* if a path had not been built because External IP address was not up
            due to layer 2 link down.  Try to build it.
         */
         connReqURLchanged = TRUE;
      }
   }
   else if (acsCfg->connectionRequestURL != NULL)
   {
      acsState.connReqURL = cmsMem_strdup(acsCfg->connectionRequestURL);
      connReqURLchanged = TRUE;
   }

   // connReqIpAddr, connReqIfNameFullPath, connReqPath
   if (connReqURLchanged)
   {
      /*
       * ConnectionRequestURL has changed or has been set to the acsState
       * for the first time.  Update the 3 other variables associated with
       * connReqURL.
       */
      cmsMem_free(acsState.connReqIpAddr);
      cmsMem_free(acsState.connReqIpAddrFullPath);
      cmsMem_free(acsState.connReqPath);

      g_TR069WANIPChanged = 1;
      /*
       * parseUrl should always succeed since our own STL handler function
       * built this URL.  Note this algorithm assumes the IP address portion
       * is always in dotted decimal format, not a DNS name.  I think this is
       * a safe assumption.
       */
      cmsUtl_parseUrl(acsCfg->connectionRequestURL, &urlProto, &acsState.connReqIpAddr, &urlPort, &acsState.connReqPath);
      cmsLog_debug("connReqURL=%s ==> connReqIPAddr=%s connReqPath=%s",
                   acsCfg->connectionRequestURL,
                   acsState.connReqIpAddr, acsState.connReqPath);

      if ((ret = cmsLck_acquireLockWithTimeout(TR69C_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get lock, ret=%d", ret);
      }
      else
      {
         acsState.connReqIpAddrFullPath = getFullPathToIpvxAddrLocked(
                                                     CMS_AF_SELECT_IPVX,
                                                     acsState.connReqIpAddr);
         if (acsState.connReqIpAddrFullPath == NULL)
         {
            cmsLog_error("could not build full path to %s", acsState.connReqIpAddr);
         }
         else
         {
            cmsLog_debug("connReqIpAddrFullPath=%s", acsState.connReqIpAddrFullPath);
         }

#ifdef DMP_DEVICE2_XMPPCONNREQ_1
         // connReqXMPPConnection is set, and its status is not up
         // tr69c need to wait until XMPP connection up.
         if (acsCfg->connReqXMPPConnection)
            g_XMPPStatus = qdmXmpp_getXmppConnectionStatusLocked_dev2(acsCfg->connReqXMPPConnection);
#endif

         cmsLck_releaseLock();
      }
   }

   // connectionRequestUsername
   if (acsState.connReqUser != NULL && acsCfg->connectionRequestUsername != NULL)
   {
      if (cmsUtl_strcmp(acsState.connReqUser, acsCfg->connectionRequestUsername) != 0)
      {
         cmsMem_free(acsState.connReqUser);
         acsState.connReqUser = cmsMem_strdup(acsCfg->connectionRequestUsername);
      }
   }
   else if (acsCfg->connectionRequestUsername != NULL)
   {
      acsState.connReqUser = cmsMem_strdup(acsCfg->connectionRequestUsername);
   }

   // connectionRequestPassword
   if (acsState.connReqPwd != NULL && acsCfg->connectionRequestPassword != NULL)
   {
      if (cmsUtl_strcmp(acsState.connReqPwd, acsCfg->connectionRequestPassword) != 0)
      {
         cmsMem_free(acsState.connReqPwd);
         acsState.connReqPwd = cmsMem_strdup(acsCfg->connectionRequestPassword);
      }
   }
   else if (acsCfg->connectionRequestPassword != NULL)
   {
      acsState.connReqPwd = cmsMem_strdup(acsCfg->connectionRequestPassword);
   }

   // boundIfName, it should never be NULL
   if (cmsUtl_strcmp(acsState.boundIfName, acsCfg->X_BROADCOM_COM_BoundIfName) != 0)
   {
      CMSMEM_REPLACE_STRING(acsState.boundIfName, acsCfg->X_BROADCOM_COM_BoundIfName);
   }

   // Periodic Inform Interval
   if (acsState.informInterval != (SINT32)acsCfg->periodicInformInterval)
   {
      acsState.informInterval = acsCfg->periodicInformInterval;
      if (acsState.informEnable == TRUE)
      {
         cancelPeriodicInform();
         resetPeriodicInform(acsCfg->periodicInformInterval);
      }
   }

   // Periodic Inform Enable
   if (acsState.informEnable != acsCfg->periodicInformEnable)
   {
      acsState.informEnable = acsCfg->periodicInformEnable;
      if (acsState.informEnable == TRUE)
      {
         resetPeriodicInform(acsCfg->periodicInformInterval);
         cmsLog_debug("periodic inform is now enabled, add event INFORM_EVENT_PERIODIC");
      }
      else
      {
         cancelPeriodicInform();
      }
   }

   /* ManageableDeviceNotificationLimit.  Update limitNotificationList. */
   if (acsCfg->manageableDeviceNotificationLimit != 0)
   {
      handleNotificationLimit("InternetGatewayDevice.ManagementServer.ManageableDeviceNumberOfEntries",
                              acsCfg->manageableDeviceNotificationLimit,manageableDeviceNotificationLimitFunc);
   }

   cmsObj_free((void **)&acsCfg);

   return;
}

void updateTr69cInfo(void)
{
   CmsRet ret;

   if ((ret = cmsLck_acquireLockWithTimeout(TR69C_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get lock, ret=%d", ret);
   }
   else
   {
      InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
      Tr69cCfgObject *tr69cCfg = NULL;

      /*
       *        * get tr69c config object
       *               */
      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_get(stdOid2EeOid(MDMOID_TR69C_CFG), &iidStack, 0, (void *) &tr69cCfg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("get of TR69C_CFG failed, ret=%d", ret);
      }
      else
      {
         loggingSOAP = tr69cCfg->loggingSOAP;
         acsState.noneConnReqAuth = (tr69cCfg->connectionRequestAuthentication == TRUE) ? 0 : 1;
         cmsObj_free((void **) &tr69cCfg);
      }

      cmsLck_releaseLock();
   }
}

void readMessageFromSmd(void *handle)
{
   CmsMsgHeader *msg;
   CmsRet ret;
   int count __attribute__ ((unused)) = 0;
   int timeout;

   if (handle == NULL)
   {
      timeout = 0;
   }
   else
   {
      timeout = *(int*)handle;
   }

   /*
    * At startup, call receiveWithTimeout with a timeout of BOOTUP_MSG_RECEIVE_TIMEOUT.
    * After the first message is used, timeout is 0.
    * There should already be a message waiting for me.
    */
   while ((ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, timeout)) == CMSRET_SUCCESS)
   {
      timeout = 0;
      switch((UINT32) msg->type)
      {
      case CMS_MSG_SYSTEM_BOOT:
         cmsLog_debug("got SYSTEM_BOOT, adding eIEBoot to informEvList");

         /* according to TR69c spec, after reboot event, the session retry count
            must be set to 0. */
         acsState.retryCount = 0;

         /* system just booted up */
         addInformEventToList(INFORM_EVENT_BOOT);

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
         /* It is possible system boots after an image download */
         {
            CmsImageTransferStats stats;
            char *fileTypeStr=NULL;
            UBOOL8 sendNotification;

            ret = cmsImg_getImageTransferStats(&stats);
            if (ret == CMSRET_SUCCESS)
            {
               if (autonomousFileTypeToFileTypeStr(stats.fileType,&fileTypeStr) != CMSRET_SUCCESS)
               {
                  cmsLog_error("Autonomous file tranfer complete, fileType not supported %d",stats.fileType);
                  return;
               }
               sendNotification = checkAutonomousTransferCompletePolicy(stats.isDownload,stats.faultCode,fileTypeStr);
               if (sendNotification)
               {
                  addInformEventToList(INFORM_EVENT_AUTON_TRANSFER_COMPLETE);
                  doSendAutonTransferComplete = TRUE;
               }
            }
         }
#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */
         break;

      case CMS_MSG_ACS_CONFIG_CHANGED:
         cmsLog_debug("got ACS config changed");
         /* updateTR69cCfgInfo adds inform event to list if necessary */
         clearModemConnectionURL();
         updateTr69cCfgInfo();

         CMSMEM_FREE_BUF_AND_NULL_PTR(httpTask.authHdr);

         if (g_initFinished)
            sendInform(NULL);
         break;

      case CMS_MSG_TR69C_CONFIG_CHANGED:
         updateTr69cInfo();
         break;

#ifdef SUPPORT_STUN
      case CMS_MSG_STUN_CONFIG_CHANGED:
         cmsLog_debug("got STUN config changed");
         stun_configChanged();
      break;
#endif

      case CMS_MSG_TR69_ACTIVE_NOTIFICATION:
         cmsLog_debug("got tr69 active notification");
         /* wan connection IP address may be changed.  This means the ConnectionURL is changed */
         clearModemConnectionURL();
         updateTr69cCfgInfo();
         addInformEventToList(INFORM_EVENT_VALUE_CHANGE);
         if (g_initFinished)
            sendInform(NULL);
         break;

      case CMS_MSG_DELAYED_MSG:
         if (msg->wordData == PERIODIC_INFORM_TIMEOUT_ID)
         {
            cmsLog_debug("got delayed msg, periodic inform while running");
            periodicInformTimeout(NULL);
         }
         else
         {
            cmsLog_error("unrecognized wordData 0x%x in DELAYED_MSG", msg->wordData);
         }
         break;

      case CMS_MSG_WAN_CONNECTION_UP:
         cmsLog_debug("got WAN_CONNECTION_UP msg");

         /* wan connection IP address may be changed.  This means the ConnectionURL is changed */
         g_TR069WANIPChanged = 0;
         clearModemConnectionURL();
         updateTr69cCfgInfo();
#ifdef DMP_DSLDIAGNOSTICS_1
         if (waitingForDiagToBringLinkUp)
         {
            /* it's up, clear the flag */
            waitingForDiagToBringLinkUp = 0;
         }
#endif
         if(g_TR069WANIPChanged == 1)
         {
            addInformEventToList(INFORM_EVENT_VALUE_CHANGE);
            if (g_initFinished)
               sendInform(NULL);
         }

         break;

      case CMS_MSG_SET_LOG_LEVEL:
         cmsLog_debug("got set log level to %d", msg->wordData);
         cmsLog_setLevel(msg->wordData);
         if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
         {
            cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
         }
         break;

      case CMS_MSG_SET_LOG_DESTINATION:
         cmsLog_debug("got set log destination to %d", msg->wordData);
         cmsLog_setDestination(msg->wordData);
         if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
         {
            cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
         }
         break;

      case CMS_MSG_PING_STATE_CHANGED:
         {
            PingDataMsgBody *pingInfo = (PingDataMsgBody *) (msg + 1);
            if (cmsUtl_strcmp(pingInfo->diagnosticsState,MDMVS_COMPLETE) == 0)
            {
               count = addInformEventToList(INFORM_EVENT_DIAGNOSTICS_COMPLETE);
               if (g_initFinished)
                  sendInform(NULL);

               cmsLog_debug("got CMS_MSG_PING_STATE_CHANGED, count=%d", count);
            }
         }
         break;

      case CMS_MSG_TRACERT_STATE_CHANGED:
         cmsLog_debug("got CMS_MSG_TRACERT_STATE_CHANGED");
         addInformEventToList(INFORM_EVENT_DIAGNOSTICS_COMPLETE);
         if (g_initFinished)
             sendInform(NULL);
         break;

      case CMS_MSG_DIAG:
         cmsLog_debug("got CMS_MSG_DIAG");
         addInformEventToList(INFORM_EVENT_DIAGNOSTICS_COMPLETE);
         if (g_initFinished)
            sendInform(NULL);
         break;

#ifdef DMP_DSLDIAGNOSTICS_1
      case CMS_MSG_DSL_LOOP_DIAG_COMPLETE:
         cmsLog_debug("got CMS_MSG_DSL_LOOP_DIAG_COMPLETE");

         /*
         * getDslLoopDiagResultsAndLinkUp requires   lineId (line id) which is
         * passed back in the msg->wordData field in ssk 
         * in 2 places, ssk2_xdsl.c and lnkstatus_wan.c
         */
         getDslLoopDiagResultsAndLinkUp(msg->wordData);
         waitingForDiagToBringLinkUp = 1;
         addInformEventToList(INFORM_EVENT_DIAGNOSTICS_COMPLETE);
         if (g_initFinished)
            sendInform(NULL);
         break;
#endif /* DMP_DSLDIAGNOSTICS_1  */
#ifdef DMP_X_BROADCOM_COM_SELT_1
      case CMS_MSG_DSL_SELT_DIAG_COMPLETE:
         cmsLog_debug("got CMS_MSG_DSL_SELT_DIAG_COMPLETE");
         addInformEventToList(INFORM_EVENT_DIAGNOSTICS_COMPLETE);
         if (g_initFinished)
            sendInform(NULL);
         break;
#endif /* DMP_X_BROADCOM_COM_SELT_1   */
      case CMS_MSG_TR69_GETRPCMETHODS_DIAG:
         addInformEventToList(INFORM_EVENT_PERIODIC);
         if (g_initFinished)
            sendInform(NULL);
         sendGETRPC = 1;
         cmsLog_debug("got CMS_MSG_TR69_GETRPCMETHODS_DIAG");
         break;

      case CMS_MSG_MANAGEABLE_DEVICE_NOTIFICATION_LIMIT_CHANGED:
         if (acsState.dataModel == DATA_MODEL_TR98)
         {
            handleNotificationLimit("InternetGatewayDevice.ManagementServer.ManageableDeviceNumberOfEntries",
                                    msg->wordData,manageableDeviceNotificationLimitFunc);
         }
         else
         {
            handleNotificationLimit("Device.ManagementServer.ManageableDeviceNumberOfEntries",
                                    msg->wordData,manageableDeviceNotificationLimitFunc);
         }
         break;

#ifdef SUPPORT_DEBUG_TOOLS
      case CMS_MSG_MEM_DUMP_STATS:
         cmsMem_dumpMemStats();
         break;
#endif

#ifdef CMS_MEM_LEAK_TRACING
      case CMS_MSG_MEM_DUMP_TRACEALL:
         cmsMem_dumpTraceAll();
         break;

      case CMS_MSG_MEM_DUMP_TRACE50:
         cmsMem_dumpTrace50();
         break;

      case CMS_MSG_MEM_DUMP_TRACECLONES:
         cmsMem_dumpTraceClones();
         break;
#endif

      case CMS_MSG_INTERNAL_NOOP:
         /* just ignore this message.  It will get freed below. */
         break;

      case CMS_MSG_RESPONSE_DU_STATE_CHANGE:
         cmsLog_debug("got CMS_MSG_RESPONSE_DU_STATE_CHANGE");
         if (acsState.duStateOpList == NULL)
         {
            /* the DU state change is trigger by other management application and not TR69 ACS */
            handleAutonDuStateChangeResponse(msg);
         }
         else
         {
            handleDuStateChangeResponse(msg);
         }
         break;

      case CMS_MSG_GET_TR69C_SESSION_STATUS:
         cmsLog_debug("got CMS_MSG_IS_TR69C_SESSION_ENDED");
         getAndRespondSessionStatus(msg);
         break;

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
      case CMS_MSG_AUTONOMOUS_TRANSFER_COMPLETE:
         cmsLog_debug("got CMS_MSG_AUTONOMOUS_TRANSFER_COMPLETE");
         handleAutonomousTransferComplete(msg);
         break;
#endif

      case CMS_MSG_XMPP_CONNECTION_UP:
         cmsLog_debug("got CMS_MSG_XMPP_CONNECTION_UP");
         g_XMPPStatus = XMPP_STATUS_UP;
         break;

      case CMS_MSG_XMPP_REQUEST_SEND_CONNREQUEST_EVENT:
         cmsLog_debug("got XMPP_REQUEST_SEND_CONNREQUEST_EVENT");

         /* send Inform doesn not return anything, so I will do some checking here first.
          * There are several reason request cannot be sent out: session is ongoing,
          * ACS configuration is not set.
          */
         if ((isAcsConnected() || (acsState.acsURL == NULL) || acsState.acsURL[0] == '\0'))
         {
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_REQUEST_DENIED)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response (request denied) for msg 0x%x failed, ret=%d", msg->type, ret);
            }
         }
         else
         {
            addInformEventToList(INFORM_EVENT_CONNECTION_REQUEST);
            if (g_initFinished)
               sendInform(NULL);
            if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
            {
               cmsLog_error("send response (request fulfilled) for msg 0x%x failed, ret=%d", msg->type, ret);
            }
         }
         break;

      default:
         cmsLog_error("unrecognized msg 0x%x from %d (flags=0x%x)",
                      msg->type, msg->src, msg->flags.all);
         break;
      }

      CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
   }

   if (ret == CMSRET_DISCONNECTED)
   {
      if (tr69cTerm == 0)
      {
         CmsRet r2;
         if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS))
         {
            cmsLog_error("lost connection to smd, exiting now.");
         }
         unregisterSmdMessageListener();
#ifdef SUPPORT_TR69C_VENDOR_RPC
         unregisterVendorSpecificRPCs();
#endif /* SUPPORT_TR69C_VENDOR_RPC */
         tr69cTerm = 1;
         r2 = cmsTmr_set(tmrHandle, delayedTermFunc, 0, DELAYED_TERMINAL_ACTION_DELAY, "sig_delayed_proc");
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("setting delayed signal processing timer failed, ret=%d", r2);
         }
      }
   }
}

int main(int argc, char** argv)
{
   SINT32      c, logLevelNum;
   SINT32      shmId=UNINITIALIZED_SHM_ID;
   CmsLogLevel logLevel=DEFAULT_LOG_LEVEL;
   UBOOL8      useConfiguredLogLevel=TRUE;
#ifdef DESKTOP_LINUX
   char        *forcedAcsUrl=NULL;
   char        *forcedConnReqUrl=NULL;
   UINT32      informInterval=0;  /* 0 is invalid value */
   SINT32      informEnable=-1;   /* -1 is not set, 0 is false, 1 is true */
   char        *forcedBoundIfName=NULL;
#endif
   CmsRet      ret;
   int timeout=0;
   int dataModel=DATA_MODEL_TR98;
   char *interestedConfigId=NULL;

   /* init log util by default EID because still don't know who am I. Will re-init later */
   cmsLog_initWithName(EID_TR69C, argv[0]);

   /* parse command line args */
   while ((c = getopt(argc, argv, "v:m:e:u:i:b:r:of:I:")) != -1)
   {
      switch(c)
      {
         case 'v':
            logLevelNum = atoi(optarg);
            if (logLevelNum == 0)
            {
               logLevel = LOG_LEVEL_ERR;
            }
            else if (logLevelNum == 1)
            {
               logLevel = LOG_LEVEL_NOTICE;
            }
            else
            {
               logLevel = LOG_LEVEL_DEBUG;
            }
            cmsLog_setLevel(logLevel);
            useConfiguredLogLevel = FALSE;
            break;

         case 'm':
            shmId = atoi(optarg);
            break;

         case 'o':
            openConnReqServerSocket = TRUE;
            break;

         case 'I':
            {
               int instanceID;
               instanceID = atoi(optarg);
               cmsLog_notice("start tr69c(%d)", instanceID);
               if (1 == instanceID)
                  g_eIdTR69C = EID_TR69C;
               else if (2 == instanceID)
                  g_eIdTR69C = EID_TR69C_2;
            }
            break;

#ifdef DESKTOP_LINUX
         case 'u':
            forcedAcsUrl = optarg;
            break;

         case 'r':
            forcedConnReqUrl = optarg;
            cmsLog_debug("forcedConnReqUrl %s", forcedConnReqUrl);
            break;

         case 'b':
            informEnable = atoi(optarg);
            break;

         case 'i':
            informInterval = atoi(optarg);
            break;

         case 'f':
            forcedBoundIfName = optarg;
            break;
#endif
         default:
            usage(argv[0]);
            break;
      }
   }

   /* re-init log util */
   cmsLog_cleanup();
   cmsLog_init(g_eIdTR69C); /* auto get AppName from EntityInfo */
   cmsLog_setLevel(logLevel);

   cmsLog_notice("g_eIdTR69C=%d", g_eIdTR69C);


   /*
    * Detach myself from the terminal so I don't get any control-c/sigint.
    * On the desktop, it is smd's job to catch control-c and exit.
    * When tr69c detects that smd has exited, tr69c will also exit.
    */
   if (setsid() == -1)
   {
      cmsLog_error("Could not detach from terminal");
   }
   else
   {
      cmsLog_debug("detached from terminal");
   }

   /* set signal masks */
   signal(SIGPIPE, SIG_IGN); /* Ignore SIGPIPE signals */
   signal(SIGTERM, tr69c_sigTermHandler);
   signal(SIGINT, tr69c_sigTermHandler);

#ifdef USE_DMALLOC
/*
# basic debugging for non-freed memory
0x403       --   log-stats, log-non-free, check-fence

# same as above plus heap check
0xc03       -- log-stats, log-non-free, check-fence, check-heap

# same as above plus error abort
0x400c0b    -- log-stats, log-non-free, log-trans, check-fence, check-heap, error-abort

# debug with extensive checking (very extensive)
0x700c2b    -- log-stats, log-non-free, log-trans, log-admin, check-fence,
               check-heap, realloc-copy, free-blank, error-abort

Just replace the hex number below to change the dmalloc option,
eg. change "debug=0x403,log=gotToSyslog" to "debug=0x400c0b,log=gotToSyslog for more
checking.
*/

   // setup dmalloc options:   log_unfree is called in period inform for now.
   dmalloc_debug_setup("debug=0x700c2b,log=goToSyslog");
   printf("\n*** DMALLOC Initialized***\n");
#endif // USE_DMALLO

   cmsLog_notice("initializing timers");
   if ((ret = cmsTmr_init(&tmrHandle)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsTmr_init failed, ret=%d", ret);
      return -1;
   }

   cmsLog_notice("calling cmsMsg_init");
   if ((ret = cmsMsg_initWithFlags(g_eIdTR69C, 0, &msgHandle)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMsg_init failed, ret=%d", ret);
      return 0;
   }

   cmsLog_notice("calling cmsMdm_init with shmId=%d", shmId);
   if ((ret = cmsMdm_initWithAcc(g_eIdTR69C, NDA_ACCESS_TR69C, msgHandle, &shmId)) != CMSRET_SUCCESS)
   {
      cmsMsg_cleanup(&msgHandle);
      cmsLog_error("cmsMdm_init error ret=%d", ret);
      return 0;
   }

   /* After we attach to MDM, figure out which data model we are using */
   if (cmsMdm_isDataModelDevice2())
   {
      dataModel = DATA_MODEL_TR181;
      RootDevice = (Device);
   }
   else
   {
      dataModel = DATA_MODEL_TR98;
      RootDevice = (InternetGatewayDevice);
   }

   initLoggingFromConfig(useConfiguredLogLevel);

   /* initialize acs state from mdm */
   memset(&acsState, 0, sizeof(ACSState));

   acsState.dataModel = dataModel;

#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
   freeInitAutonomousCompleteStats(&autonomousCompleteStats);
#endif

   /* read saved state from persistent scratch pad, including informState */
   retrieveTR69StatusItems();

   /* read any vendor config info from flash if any */
   retrieveClearTR69VendorConfigInfo();

#ifdef DESKTOP_LINUX
   /*
    * process command line arguments.
    * These are used during unittests, which need to set some MDM parameters
    * for tr69c.  We need to get the write lock before writing to MDM.
    */
   if ((forcedAcsUrl != NULL) || (forcedBoundIfName != NULL) ||
       (informEnable != -1) || (informInterval != 0))
   {
      if ((ret = cmsLck_acquireLockWithTimeout(TR69C_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not get lock, ret=%d", ret);
         /* just not set the variables I guess */
      }
      else
      {
         if (forcedAcsUrl != NULL)
         {
            cmsLog_notice("forcing acsUrl to %s", forcedAcsUrl);
            setMSrvrURL(forcedAcsUrl);
         }

         if (forcedBoundIfName != NULL)
         {
            cmsLog_notice("forcing boundIfName to %s", forcedBoundIfName);
            setMSrvrBoundIfName(forcedBoundIfName);
         }

         if (informEnable != -1)
         {
            UBOOL8 b;
            cmsLog_notice("forcing informEnable to %d", informEnable);
            b = (informEnable == 0) ? FALSE : TRUE;
            setMSrvrInformEnable(b);
         }

         if (informInterval != 0)
         {
            cmsLog_notice("forcing informInterval to %u", informInterval);
            setMSrvrInformInterval(informInterval);
         }

         cmsLck_releaseLock();
      }
   }
#endif /* DESKTOP_LINUX */

   /* clear limit notification list */
   memset((char*)&limitNotificationList,0,sizeof(limitNotificationList));

   /* retrieve tr69c configuration including acsUrl, ManageableDeviceLimitNotification if any */
   updateTr69cCfgInfo();

   /* check for any message from smd telling me why I was launched. */
   timeout = BOOTUP_MSG_RECEIVE_TIMEOUT;
   readMessageFromSmd(&timeout);

   cmsLog_debug("acsState.manufacturer   = %s", acsState.manufacturer);
   cmsLog_debug("acsState.manufacturerOUI= %s", acsState.manufacturerOUI);
   cmsLog_debug("acsState.productClass   = %s", acsState.productClass);
   cmsLog_debug("acsState.serialNumber   = %s", acsState.serialNumber);
   cmsLog_debug("acsState.acsURL         = %s", acsState.acsURL);
   cmsLog_debug("acsState.boundIfName    = %s", acsState.boundIfName);
   cmsLog_debug("acsState.acsUser        = %s", acsState.acsUser);
   cmsLog_debug("acsState.acsPwd         = %s", acsState.acsPwd);
   cmsLog_debug("acsState.connReqURL     = %s", acsState.connReqURL);
   cmsLog_debug("acsState.connReqIpAddr  = %s", acsState.connReqIpAddr);
   if (acsState.connReqIpAddrFullPath != NULL)
   {
      cmsLog_debug("acsState.connReqIpAddrFullPath= %s", acsState.connReqIpAddrFullPath);
   }
   cmsLog_debug("acsState.connReqUser    = %s", acsState.connReqUser);
   cmsLog_debug("acsState.connReqPwd     = %s", acsState.connReqPwd);
   cmsLog_debug("acsState.informEnable   = %d", acsState.informEnable);
   cmsLog_debug("acsState.informInterval = %ld", acsState.informInterval);
   cmsLog_debug("informState             = %d", informState);

   /*
    * Register our interest for ACS_CONFIG_CHANGED event with smd.
    */
   if (EID_TR69C == g_eIdTR69C)
   {
      interestedConfigId = MULTI_TR69C_CONFIG_INDEX_1;
   }
   else if (EID_TR69C_2 == g_eIdTR69C)
   {
      interestedConfigId = MULTI_TR69C_CONFIG_INDEX_2;
   }

   registerInterestInEvent(CMS_MSG_ACS_CONFIG_CHANGED, TRUE, interestedConfigId, strlen(interestedConfigId)+1);
   registerInterestInEvent(CMS_MSG_TR69C_CONFIG_CHANGED, TRUE, interestedConfigId, strlen(interestedConfigId)+1);
   registerInterestInEvent(CMS_MSG_TR69_ACTIVE_NOTIFICATION, TRUE, NULL, 0);
   registerInterestInEvent(CMS_MSG_TR69_GETRPCMETHODS_DIAG, TRUE, NULL, 0);
   registerInterestInEvent(CMS_MSG_RESPONSE_DU_STATE_CHANGE, TRUE, NULL, 0);
   registerInterestInEvent(CMS_MSG_DIAG, TRUE, NULL, 0);
   registerInterestInEvent(CMS_MSG_XMPP_CONNECTION_UP, TRUE, NULL, 0);
#ifdef SUPPORT_STUN
   registerInterestInEvent(CMS_MSG_STUN_CONFIG_CHANGED, TRUE, NULL, 0);
#endif
   if (checkStartupPreReqs())
   {
      /* OK, we have all the information we need to start. */

      /*
       * Initialize SSL and also send first inform, if necessary.
       */
      initTasks();

      /*
       * This is where everything happens.
       */
      eventLoop();
   }
   else
   {
      /* we don't have enough info to run or or WAN connection is not up yet.
       * Fall through to exit path and let smd wake us up when there is
       * enough info or when the WAN connection is up.
       */
      saveTR69StatusItems();
   }


   /*
    * Cleanup before exiting.
    * There is one other exit point in informer.c: acsDisconnect.
    * Eventually that should be consolidated into here.
    */
   main_cleanup(0);

   return(0); /* not reached, since main_cleanup calls exit */
}  /* End of main() */


/** Cleanup all resources before exiting.  This makes resource checkers happy.
 *
 * This function does not return.  It will cause the program to exit.
 *
 * @param code (IN) exit code.
 */
void main_cleanup(SINT32 code)
{
   cmsLog_notice("exiting with code %d", code);

   freeAllListeners();

   acsState_cleanup();
   cmsMdm_cleanup();
   cmsMsg_cleanup(&msgHandle);
   cmsTmr_cleanup(&tmrHandle);
   cmsLog_cleanup();
#ifdef DMP_DSLDIAGNOSTICS_1
   freeDslLoopDiagResults();
#endif /* DMP_DSLDIAGNOSTICS_1 */
   exit(code);
}


static CmsRet du_install(const char *url,
                         const char *uuid,
                         const char *username,
                         const char *password,
                         const char *EERef,
                         LIST_TYPE  *du_resp_list)
{
   CmsMsgHeader *reqMsg = NULL;
   DUrequestStateChangedMsgBody *msgPayload = NULL;
   CmsEntityId destEid = EID_INVALID;
   ENTRY_TYPE *list_entry = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(DUrequestStateChangedMsgBody),
         ALLOC_ZEROIZE);
   if(reqMsg == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }

   list_entry = cmsMem_alloc(sizeof(ENTRY_TYPE), ALLOC_ZEROIZE);
   if (list_entry == NULL)
   {
      CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* initialize header fields */
   reqMsg->type = CMS_MSG_REQUEST_DU_STATE_CHANGE;
   reqMsg->src = cmsMsg_getHandleEid(msgHandle);
   reqMsg->flags_event = 1;
   reqMsg->flags_bounceIfNotRunning = 1;
   reqMsg->dataLength = sizeof(DUrequestStateChangedMsgBody);

   /* copy file into the payload and send message */
   msgPayload = (DUrequestStateChangedMsgBody*)(reqMsg + 1);

   msgPayload->reqId = ++g_req_id;
   cmsUtl_strncpy(msgPayload->operation,SW_MODULES_OPERATION_INSTALL,sizeof(msgPayload->operation));
   cmsUtl_strncpy(msgPayload->URL,url,sizeof(msgPayload->URL));
   cmsUtl_strncpy(msgPayload->UUID,uuid,sizeof(msgPayload->UUID));
   cmsUtl_strncpy(msgPayload->username,username,sizeof(msgPayload->username));
   cmsUtl_strncpy(msgPayload->password,password,sizeof(msgPayload->password));

   //EERef should be a fullpath to ExecEnv instance, end with '.'
   if (EERef == NULL || strlen(EERef) == 0)
   {
      qdmModsw_getExecEnvFullPathByNameLocked(NULL,
                                              msgPayload->execEnvFullPath,
                                              sizeof(msgPayload->execEnvFullPath));
   }
   else
   {
      char tmpFullPath[CMS_MAX_FULLPATH_LENGTH];
      UINT32 len;
      /* tr69 specification has an '.' at then end of EE fullpath, and we store it  without '.'.
      * Just take out '.' from EE fullpath 
      */
      cmsUtl_strncpy(tmpFullPath, EERef, sizeof(tmpFullPath)-1);
      len = cmsUtl_strlen(tmpFullPath);
      
      if (len > 0 && tmpFullPath[len-1]  == '.')
      {
         tmpFullPath[len-1] = '\0';
      }
      cmsUtl_strncpy(msgPayload->execEnvFullPath,tmpFullPath,sizeof(msgPayload->execEnvFullPath));
   }

   ret = qdmModsw_getMngrEidByExecEnvFullPathLocked(msgPayload->execEnvFullPath,
                                                    &destEid);
   if (ret != CMSRET_SUCCESS)
   {
      /* this also should not fail for the same reason as above. */
      cmsLog_error("Could not get Mngr Eid for %s (ret=%d)",
            msgPayload->execEnvFullPath, ret);
      ret = CMSRET_INTERNAL_ERROR;
      goto grace_exit;
   }
   else
   {
      reqMsg->dst = destEid;
   }

   if((ret = cmsMsg_send(msgHandle, reqMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to send message (ret=%d)", ret);
   }
   else
   {
      cmsLog_debug("Sent req message op=%s dstEid=%d",
            msgPayload->operation, reqMsg->dst);

      list_entry->keyType = KEY_STRING;
      list_entry->key = cmsMem_alloc(8, ALLOC_ZEROIZE);
      sprintf(list_entry->key, "%d", g_req_id);
      list_entry->data = NULL;
      addEnd(list_entry, du_resp_list);
   }

grace_exit:   
   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
   return ret;
}


static int
du_init_update(const char *uuid,
               const char *version,
               const char *url,
               const char *username,
               const char *password,
               const DUObject *duObj,
               CmsMsgHeader **reqMsg)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsEntityId destEid = EID_INVALID;
   DUrequestStateChangedMsgBody *msgPayload = NULL;
   UBOOL8 emptyUuid = FALSE, emptyUrl = FALSE, emptyVersion = FALSE;

   emptyUuid = IS_EMPTY_STRING(uuid);
   emptyVersion = IS_EMPTY_STRING(version);
   emptyUrl = IS_EMPTY_STRING(url);

   ret = qdmModsw_getMngrEidByExecEnvFullPathLocked(duObj->executionEnvRef,
                                                    &destEid);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get mngrEid for execEnvRef=%s duid=%s",
                   duObj->executionEnvRef, duObj->DUID);
      return ret;
   }

   *reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(DUrequestStateChangedMsgBody),
                          ALLOC_ZEROIZE);
   if (*reqMsg == NULL)
   {
      cmsLog_error("Alloc msg memory error.");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* initialize header fields */
   (*reqMsg)->type = CMS_MSG_REQUEST_DU_STATE_CHANGE;
   (*reqMsg)->src = cmsMsg_getHandleEid(msgHandle);
   (*reqMsg)->flags_event = 1;
   (*reqMsg)->flags_bounceIfNotRunning = 1;
   (*reqMsg)->dataLength = sizeof(DUrequestStateChangedMsgBody);
   (*reqMsg)->dst = destEid;

   /* copy info into the payload and send message */
   msgPayload = (DUrequestStateChangedMsgBody *) ((*reqMsg) + 1);

   strncpy(msgPayload->operation,
           SW_MODULES_OPERATION_UPDATE,
           strlen(SW_MODULES_OPERATION_UPDATE));

   if (emptyUuid == FALSE)
   {
      strncpy(msgPayload->UUID, uuid, strlen(uuid));
   }
   else
   {
      strncpy(msgPayload->UUID, duObj->UUID, strlen(duObj->UUID));
   }

   if (emptyVersion == FALSE)
   {
      strncpy(msgPayload->version, version, strlen(version));
   }
   else
   {
      strncpy(msgPayload->version, duObj->version, strlen(duObj->version));
   }

   if (emptyUrl == FALSE)
   {
      strncpy(msgPayload->URL, url, strlen(url));
      strncpy(msgPayload->username, username, strlen(username));
      strncpy(msgPayload->password, password, strlen(password));
   }
   else
   {
      strncpy(msgPayload->URL, duObj->URL, strlen(duObj->URL));
      strncpy(msgPayload->username,
              duObj->X_BROADCOM_COM_Username,
              strlen(duObj->X_BROADCOM_COM_Username));
      strncpy(msgPayload->password,
              duObj->X_BROADCOM_COM_Password,
              strlen(duObj->X_BROADCOM_COM_Password));
   }

   strncpy(msgPayload->execEnvFullPath,
           duObj->executionEnvRef,
           strlen(duObj->executionEnvRef));

   msgPayload->reqId = ++g_req_id;

   return ret;
}


static int
du_add_update(LIST_TYPE *du_resp_list)
{
   ENTRY_TYPE *list_entry = cmsMem_alloc(sizeof(ENTRY_TYPE), ALLOC_ZEROIZE);

   if (list_entry == NULL)
   {
      cmsLog_error("Fail to allocate LIST_ENTRY!");
      return CMSRET_RESOURCE_EXCEEDED;
   }

   list_entry->keyType = KEY_STRING;

   list_entry->key = cmsMem_alloc(8, ALLOC_ZEROIZE);
   if (list_entry->key == NULL)
   {
      cmsLog_error("Fail to allocate LIST_ENTRY KEY!");
      CMSMEM_FREE_BUF_AND_NULL_PTR(list_entry);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   sprintf(list_entry->key, "%d", g_req_id);

   list_entry->data = NULL;

   addEnd(list_entry, du_resp_list);
   
   return CMSRET_SUCCESS;
}


static int
du_update(const char *uuid,
          const char *version,
          const char *url,
          const char *username,
          const char *password,
          LIST_TYPE  *du_resp_list)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   DUObject *duObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 emptyUuid = FALSE, emptyUrl = FALSE;

   emptyUuid = IS_EMPTY_STRING(uuid);
   emptyUrl = IS_EMPTY_STRING(url);

   if (emptyUuid == FALSE && emptyUrl == FALSE)
   {
      /* UUID populated, URL populated: The CPE MUST Update the DU
       * with the matching UUID and update its internal URL */
      while (cmsObj_getNextFlags(MDMOID_DU,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&duObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(duObj->UUID, uuid) == 0)
         {
            ret = du_init_update(uuid, version,
                                 url, username, password,
                                 duObj, &reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = cmsMsg_send(msgHandle, reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to send message (ret=%d)", ret);
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = du_add_update(du_resp_list);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            cmsLog_debug("Sent req message op=%s dstEid=%d",
                         SW_MODULES_OPERATION_UPDATE, reqMsg->dst);

            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
         }

         cmsObj_free((void **) &duObj);
      }
   }
   else if (emptyUuid == FALSE && emptyUrl == TRUE)
   {
      /* UUID populated, URL empty: The CPE MUST Update the DU
       * with the matching UUID based on its internal URL
       * (the CPE SHOULD use the credentials that were last used
       * to Install or Update this DU) */
      while (cmsObj_getNextFlags(MDMOID_DU,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&duObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(duObj->UUID, uuid) == 0)
         {
            ret = du_init_update(uuid, version,
                                 url, username, password,
                                 duObj, &reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = cmsMsg_send(msgHandle, reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to send message (ret=%d)", ret);
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = du_add_update(du_resp_list);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            cmsLog_debug("Sent req message op=%s dstEid=%d",
                         SW_MODULES_OPERATION_UPDATE, reqMsg->dst);

            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
         }

         cmsObj_free((void **) &duObj);
      }
   }
   else if (emptyUuid == TRUE && emptyUrl == FALSE)
   {
      /* UUID empty, URL populated: The CPE MUST Update the DU
       * that last used the URL at either Install or Update
       * (i.e. matches the URL Parameter in the DeploymentUnit.{i}. table) */
      while (cmsObj_getNextFlags(MDMOID_DU,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&duObj) == CMSRET_SUCCESS)
      {
         if (cmsUtl_strcmp(duObj->URL, url) == 0)
         {
            ret = du_init_update(uuid, version,
                                 url, username, password,
                                 duObj, &reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = cmsMsg_send(msgHandle, reqMsg);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Failed to send message (ret=%d)", ret);
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            ret = du_add_update(du_resp_list);
            if (ret != CMSRET_SUCCESS)
            {
               CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
               cmsObj_free((void **) &duObj);
               return ret;
            }

            cmsLog_debug("Sent req message op=%s dstEid=%d",
                         SW_MODULES_OPERATION_UPDATE, reqMsg->dst);

            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
         }

         cmsObj_free((void **) &duObj);
      }
   }
   else
   {
      /* UUID empty, URL empty: The CPE MUST Update all DUs based on
       * their internal URL (the CPE SHOULD use the credentials that
       * were last used to Install or Update the DU) */
      while (cmsObj_getNextFlags(MDMOID_DU,
                                 &iidStack,
                                 OGF_NO_VALUE_UPDATE,
                                 (void **)&duObj) == CMSRET_SUCCESS)
      {
         ret = du_init_update(uuid, version,
                              url, username, password,
                              duObj, &reqMsg);
         if (ret != CMSRET_SUCCESS)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
            cmsObj_free((void **) &duObj);
            return ret;
         }

         ret = cmsMsg_send(msgHandle, reqMsg);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to send message (ret=%d)", ret);
            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
            cmsObj_free((void **) &duObj);
            return ret;
         }

         ret = du_add_update(du_resp_list);
         if (ret != CMSRET_SUCCESS)
         {
            CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);
            cmsObj_free((void **) &duObj);
            return ret;
         }

         cmsLog_debug("Sent req message op=%s dstEid=%d",
                      SW_MODULES_OPERATION_UPDATE, reqMsg->dst);

         CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

         cmsObj_free((void **) &duObj);
      }
   }

   return ret;
}


static CmsRet du_uninstall(const char *uuid,
                           const char *version,
                           const char *EERef,
                           LIST_TYPE  *du_resp_list)
{
   CmsRet ret = CMSRET_SUCCESS;
   CmsMsgHeader *reqMsg = NULL;
   DUrequestStateChangedMsgBody *msgPayload = NULL;
   DUObject *duObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found = FALSE;
   CmsEntityId destEid = EID_INVALID;

   reqMsg = cmsMem_alloc(sizeof(CmsMsgHeader) + sizeof(DUrequestStateChangedMsgBody),
                         ALLOC_ZEROIZE);
   if (reqMsg == NULL)
   {
      return CMSRET_RESOURCE_EXCEEDED;
   }

   /* initialize header fields */
   reqMsg->type = CMS_MSG_REQUEST_DU_STATE_CHANGE;
   reqMsg->src = cmsMsg_getHandleEid(msgHandle);
   reqMsg->flags_event = 1;
   reqMsg->flags_bounceIfNotRunning = 1;
   reqMsg->dataLength = sizeof(DUrequestStateChangedMsgBody);

   /* fill in the message body, some info must come from DuObj */
   msgPayload = (DUrequestStateChangedMsgBody *) (reqMsg + 1);
   cmsUtl_strncpy(msgPayload->operation,
                  SW_MODULES_OPERATION_UNINSTALL,
                  sizeof(msgPayload->operation));
   
   while ((cmsObj_getNextFlags(MDMOID_DU, &iidStack, 
                               OGF_NO_VALUE_UPDATE,
                               (void **)&duObj) == CMSRET_SUCCESS))
   {
      if(cmsUtl_strcmp(duObj->UUID, uuid) == 0)
      {
         // Passing version parameter, need to check with duObj's version
         if (!IS_EMPTY_STRING(version) && cmsUtl_strcmp(duObj->version, version) != 0)
         {
            cmsObj_free((void **) &duObj);
            continue;
         }

         // Passing EERef parameter, need to check with duObj's executionEnvRef
         if (!IS_EMPTY_STRING(EERef))
         {
            char tmpFullPath[CMS_MAX_FULLPATH_LENGTH];
            UINT32 len;
            /* tr69 specification has an '.' at then end of EE fullpath, and we store it  without '.'.
            * Just take out '.' from EE fullpath 
            */
            cmsUtl_strncpy(tmpFullPath, EERef, sizeof(tmpFullPath)-1);
            len = cmsUtl_strlen(tmpFullPath);
            
            if (len > 0 && tmpFullPath[len-1] == '.')
            {
               tmpFullPath[len-1] = '\0';
            }
            if (cmsUtl_strcmp(tmpFullPath, duObj->executionEnvRef) != 0)
            {
               cmsObj_free((void **) &duObj);
               continue;
            }
         }

         ret = qdmModsw_getMngrEidByExecEnvFullPathLocked
                  (duObj->executionEnvRef, &destEid);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("could not get mngrEid for execEnvRef=%s duid=%s",
                         duObj->executionEnvRef, duObj->DUID);
            cmsObj_free((void **) &duObj);
            continue;
         }   
         else
         {
            msgPayload->reqId = ++g_req_id;
            cmsUtl_strncpy(msgPayload->version,
                           duObj->version,
                           sizeof(msgPayload->version));
            cmsUtl_strncpy(msgPayload->UUID,
                           duObj->UUID,
                           sizeof(msgPayload->UUID));
            cmsUtl_strncpy(msgPayload->execEnvFullPath,
                           duObj->executionEnvRef,
                           sizeof(msgPayload->execEnvFullPath));
            reqMsg->dst = destEid;
            found = TRUE;
            if((ret = cmsMsg_send(msgHandle, reqMsg)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Failed to send message (ret=%d)", ret);
            }
            else
            {
                ENTRY_TYPE *list_entry = cmsMem_alloc
                   (sizeof(ENTRY_TYPE), ALLOC_ZEROIZE);

                if (list_entry == NULL)
                {
                   ret = CMSRET_RESOURCE_EXCEEDED;
                   cmsObj_free((void **) &duObj);
                   break;
                }

                list_entry->keyType = KEY_STRING;
                list_entry->key = cmsMem_alloc(8, ALLOC_ZEROIZE);
                sprintf(list_entry->key, "%d", g_req_id);
                list_entry->data = NULL;
                addEnd(list_entry, du_resp_list);
            }
         }
      }

      cmsObj_free((void **) &duObj);     
   }  

   if (!found)
   {
      cmsLog_error("Could not find %s (%s) for %s(ret=%d)",
                   uuid, version, EERef, ret);
      ret = CMSRET_DU_UNKNOWN;
   }
   else
   {
      cmsLog_debug("Found DU(s) to uninstall");
   }

   CMSMEM_FREE_BUF_AND_NULL_PTR(reqMsg);

   return ret;
}


CmsRet sendReqToExecEnvMngrLocked(ChangeDuStateOpReqInfo *opReq)
{
   CmsRet ret = CMSRET_SUCCESS;

   if (opReq->operationType == eDuInstall)
   {
      ret = du_install(opReq->url, opReq->uuid, opReq->user, opReq->pwd,
                       opReq->execEnvFullPath, &(opReq->du_resp_list));
   }
   else if (opReq->operationType == eDuUpdate)
   {
      ret = du_update(opReq->uuid, opReq->version,
                      opReq->url, opReq->user, opReq->pwd,
                      &(opReq->du_resp_list));
   }
   else if (opReq->operationType == eDuUninstall)
   {
      ret = du_uninstall(opReq->uuid, opReq->version,
                         opReq->execEnvFullPath, &(opReq->du_resp_list));
   }
   
   return (ret);
}


static CmsRet addDuStateChangeRespone(const CmsMsgHeader *msg,
                                      ChangeDuStateOpReqInfo *opReq)
{
   char key[8];
   DuResponseStateChangedEntry *du_resp_entry = NULL;
   DUresponseStateChangedMsgBody *du_resp = NULL;
   ENTRY_TYPE *list_entry = NULL;
   ENTRY_TYPE *prev_entry = NULL;
   CmsRet ret = CMSRET_OBJECT_NOT_FOUND;

   du_resp = (DUresponseStateChangedMsgBody *) (msg+1);

   sprintf(key, "%d", du_resp->reqId);

   if (findEntry(&(opReq->du_resp_list),
                 (void *)key,
                 KEY_STRING,
                 &prev_entry,
                 &list_entry))
   {
      du_resp_entry = cmsMem_alloc
         (sizeof(DuResponseStateChangedEntry), ALLOC_ZEROIZE);

      if (du_resp_entry == NULL)
      {
         cmsLog_error("Cannot allocate memory for DU response entry");
         return CMSRET_RESOURCE_EXCEEDED;
      }

      du_resp_entry->num_of_du =
         msg->dataLength / sizeof(DUresponseStateChangedMsgBody);

      du_resp_entry->du_list = cmsMem_alloc(msg->dataLength, ALLOC_ZEROIZE);

      if (du_resp_entry->du_list == NULL)
      {
         cmsLog_error("Cannot allocate memory for list of responsd DUs");
         return CMSRET_RESOURCE_EXCEEDED;
      }

      memcpy(du_resp_entry->du_list, du_resp, msg->dataLength);

      list_entry->data = du_resp_entry;

      ret = CMSRET_SUCCESS;
   }
   
   return ret;
}

/*
 * We received a response about a DU State Change request we sent out.
 * Update acsState structure
 */
void handleDuStateChangeResponse(const CmsMsgHeader *msg)
{
   DUresponseStateChangedMsgBody *msgResBody = (DUresponseStateChangedMsgBody *)(msg+1);
   ChangeDuStateOpReqInfo *pAcsStateOpResult = acsState.duStateOpList;
   int found = 0;

   if (pAcsStateOpResult == NULL)
   {
      /* something is really wrong, this is not supposed to be NULL */
      cmsLog_error("Something is wrong, acsState's record of OP request is NULL");
      return;
   }

   cmsLog_debug("operation=%s URL=%s UUID=%s version=%s",
                 msgResBody->operation, msgResBody->URL, msgResBody->UUID,
                 msgResBody->version);

   if (strcmp(msgResBody->operation, SW_MODULES_OPERATION_INSTALL) == 0)
   {
      /* so now, look for the DU request in the list */
      while (pAcsStateOpResult != NULL)
      {
         if (pAcsStateOpResult->operationType == eDuInstall)
         {
            if ((cmsUtl_strcmp(pAcsStateOpResult->url, msgResBody->URL)) == 0)
            {
               found = 1;
               break;
            }
            
         }
         pAcsStateOpResult = pAcsStateOpResult->next;
      } /* while */
   }
   else if (cmsUtl_strcmp(msgResBody->operation,SW_MODULES_OPERATION_UNINSTALL) == 0)
   {
      /* so now, look for the DU request in the list */
      while (pAcsStateOpResult != NULL)
      {
         if (pAcsStateOpResult->operationType == eDuUninstall)
         {
            if (IS_EMPTY_STRING(pAcsStateOpResult->version) == TRUE)
            {
               found = (cmsUtl_strcmp(pAcsStateOpResult->uuid, msgResBody->UUID) == 0);
            }
            else
            {
               found = ((cmsUtl_strcmp(pAcsStateOpResult->version, msgResBody->version) == 0) &&
                        (cmsUtl_strcmp(pAcsStateOpResult->uuid, msgResBody->UUID) == 0));
            }
            if (found)
            {
               break;
            }
         }
         pAcsStateOpResult = pAcsStateOpResult->next;
      } /* while */
   }
   else if (cmsUtl_strcmp(msgResBody->operation,SW_MODULES_OPERATION_UPDATE) == 0)
   {
      /* so now, look for the DU request in the list */
      while (pAcsStateOpResult != NULL)
      {
         if (pAcsStateOpResult->operationType == eDuUpdate)
         {
            if (IS_EMPTY_STRING(pAcsStateOpResult->uuid) == FALSE &&
                IS_EMPTY_STRING(pAcsStateOpResult->url) == FALSE)
            {
               if (cmsUtl_strcmp(pAcsStateOpResult->uuid, msgResBody->UUID) == 0 &&
                   cmsUtl_strcmp(pAcsStateOpResult->url, msgResBody->URL) == 0 )
               {
                  found = 1;
                  break;
               }
            }
            else if (IS_EMPTY_STRING(pAcsStateOpResult->uuid) == FALSE &&
                     IS_EMPTY_STRING(pAcsStateOpResult->url) == TRUE)
            {
               if (cmsUtl_strcmp(pAcsStateOpResult->uuid, msgResBody->UUID) == 0)
               {
                  found = 1;
                  break;
               }
            }
            else if (IS_EMPTY_STRING(pAcsStateOpResult->uuid) == TRUE &&
                     IS_EMPTY_STRING(pAcsStateOpResult->url) == FALSE)
            {
               if (cmsUtl_strcmp(pAcsStateOpResult->url, msgResBody->URL) == 0)
               {
                  found = 1;
                  break;
               }
            }
            else
            {
               found = 1;
               break;
            }
         }
         pAcsStateOpResult = pAcsStateOpResult->next;
      } /* while */
   }

   if (found)
   {
      addDuStateChangeRespone(msg, pAcsStateOpResult);
   }

   if (areAllDuStateChangeResponsesReceived(acsState.duStateOpList) == TRUE)
   {
      allResponseReceived();
   }
} /* handleDuStateChangeResponse */


void handleAutonDuStateChangeResponse(const CmsMsgHeader *msg)
{
   int i = 0;
   int numDuResp = 0;
   DUresponseStateChangedMsgBody *pDuResp = NULL;
   UBOOL8 sendNotification;
   char opList[BUFLEN_32]={0};
   /* allow for 120 DUs; max request is 16 */
   char faultCodeList[BUFLEN_256]={0};
   char faultCodeStr[BUFLEN_8]={0};
   UINT32 faultCodeCount=0;
   UINT32 resultBits = 0;

   /* pAutonResult stores the info for AutonDuStateComplete */
   if (pAutonResult != NULL)
   {
      freeOpReqList(pAutonResult);
   }

   pAutonResult = cmsMem_alloc(sizeof(ChangeDuStateOpReqInfo),ALLOC_ZEROIZE);
   if (pAutonResult == NULL)
   {
      cmsLog_error("cmsMem_alloc of %d for pAutonResult failed",
                   sizeof(ChangeDuStateOpReqInfo));
      return;
   }

   /* first find out how many DUs are in the message */
   numDuResp = (msg->dataLength)/(sizeof(DUresponseStateChangedMsgBody));

   pAutonResult->numDuResp = numDuResp;
   pAutonResult->duResp = cmsMem_alloc
      ((sizeof(DUresponseStateChangedMsgBody)*numDuResp), ALLOC_ZEROIZE);

   if (pAutonResult->duResp == NULL)
   {
      cmsLog_error("Cannot allocate memory for list of responsd DUs");
      freeOpReqList(pAutonResult);
      return;
   }

   pDuResp = (DUresponseStateChangedMsgBody *)(msg+1);

   memcpy(pAutonResult->duResp, pDuResp, msg->dataLength);
   
   for (i = 0, pDuResp = pAutonResult->duResp;
        i < numDuResp && pDuResp != NULL;
        i++, pDuResp++)
   {
      if (strcmp(pDuResp->operation, SW_MODULES_OPERATION_INSTALL) == 0)
      {
         resultBits |= RESULT_BIT_DU_OP_INSTALL;
      }
      else if (cmsUtl_strcmp(pDuResp->operation,SW_MODULES_OPERATION_UPDATE) == 0)
      {
         resultBits |= RESULT_BIT_DU_OP_UPDATE;
      }
      else
      {
         resultBits |= RESULT_BIT_DU_OP_UNINSTALL;
      }

      if (pDuResp->faultCode != CMSRET_SUCCESS)
      {
         if (faultCodeCount == 0)
         {
            sprintf(faultCodeStr, "%d", pDuResp->faultCode);

         }
         else if (faultCodeCount < RESULT_FAULT_CODE_COUNT_MAX)
         {
            sprintf(faultCodeStr, ",%d", pDuResp->faultCode);
         }

         if (faultCodeCount < RESULT_FAULT_CODE_COUNT_MAX)
         {
            strcat(faultCodeList, faultCodeStr);
            faultCodeCount++;
         }
      }
   }

   /* build the opList based on the bits detection above */
   if ((resultBits & RESULT_BIT_DU_OP_INSTALL) == RESULT_BIT_DU_OP_INSTALL)
   {
      strcat(opList,"Install");
   }
   if ((resultBits & RESULT_BIT_DU_OP_UNINSTALL) == RESULT_BIT_DU_OP_UNINSTALL)
   {
      if (opList[0] == '\0')
      {
         strcat(opList,"Uninstall");
      }
      else
      {
         strcat(opList,",Uninstall");
      }
   }
   if ((resultBits & RESULT_BIT_DU_OP_UPDATE) == RESULT_BIT_DU_OP_UPDATE)
   {
      if (opList[0] == '\0')
      {
         strcat(opList,"Update");
      }
      else
      {
         strcat(opList,",Update");
      }
   }

   sendNotification = checkDuStateChangePolicy(opList,faultCodeList);

   if (sendNotification)
   {
      goSendAutonomousDuStateComplete();
   }
   else
   {
      cmsLog_debug("Autonomous DuState Change not sent due to DuStateStateChange Policy");
   }
} /* handleAutonDuStateChangeResponse */


void getAndRespondSessionStatus(const CmsMsgHeader *msg)
{
   UINT32 status = 1; /* session is still active */
   CmsMsgHeader replyMsg = EMPTY_MSG_HEADER;
   CmsRet ret;

   if (sessionState == eSessionEnd)
   {
      /* session ended */
      status = 0;
   }

   cmsLog_debug("WAN Link status=%d", status);

   replyMsg.type = msg->type;
   replyMsg.src = g_eIdTR69C;
   replyMsg.dst = msg->src;
   replyMsg.flags_request = 0;
   replyMsg.flags_response = 1;
   replyMsg.wordData = status;

   if ((ret = cmsMsg_send(msgHandle, &replyMsg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("msg send failed, ret=%d",ret);
   }
}


#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
void handleAutonomousTransferComplete(const CmsMsgHeader *msg)
{
   UBOOL8 sendNotification;
   char *fileTypeStr=NULL;
   char dateTimeBuf[BUFLEN_64];

   AutonomousTransferCompleteMsgBody *msgBody = (AutonomousTransferCompleteMsgBody *)(msg + 1);
   if (autonomousFileTypeToFileTypeStr(msgBody->fileType,&fileTypeStr) != CMSRET_SUCCESS)
   {
      cmsLog_error("Autonomous file tranfer complete, fileType not supported %d",msgBody->fileType);
      return;
   }

   /* Autonomous Policy filter notification under 4 conditions:
    * 1. Transfer Complete Notification enable?
    * 2. Type filter: Upload, Download or both
    * 3. Result filter: success, failure or both
    * 4. FileType filter: comma seperated config, firmware
    */
   sendNotification = checkAutonomousTransferCompletePolicy(msgBody->isDownload,msgBody->faultCode,fileTypeStr);
   if (sendNotification)
   {
      autonomousCompleteStats.isDownload = msgBody->isDownload;
      autonomousCompleteStats.fileType = fileTypeStr;
      autonomousCompleteStats.fileSize = msgBody->fileSize;
      autonomousCompleteStats.fault.faultCode = msgBody->faultCode;
      if (msgBody->faultStr != NULL)
      {
         autonomousCompleteStats.fault.faultString = cmsMem_strdup(msgBody->faultStr);
      }
      cmsTms_getXSIDateTime(msgBody->startTime, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING(autonomousCompleteStats.startTime,dateTimeBuf);
      cmsTms_getXSIDateTime(msgBody->completeTime, dateTimeBuf, sizeof(dateTimeBuf));
      CMSMEM_REPLACE_STRING(autonomousCompleteStats.completeTime,dateTimeBuf);
      goSendAutonomousTransferComplete();
   }
   else
   {
      cmsMem_free(fileTypeStr);
   }
} /* handleAutonomousTransferComplete */

#endif /* SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE */
