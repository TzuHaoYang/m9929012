/***********************************************************************
 *
 *  Copyright (c) 2008-2010  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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

#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include "cms.h"
#include "cms_msg.h"
#include "cms_util.h"
#include "cms_phl.h"
#include "httpd.h"
#include "cgi_main.h"
#include "cgi_omci.h"
#include "omciutl_cmn.h"
#include "omci_ipc.h"

//#define OMCI_CFG_DEBUG 1

#define MDMOID_ONT_DATA_MEID 0


UINT16 tcIdCur = 0;
UINT16 numMsgRx = 0;
UINT16 numMsgTx = 0;
UINT16 numRetry = 0;
omciPacket packetList[NUM_PACKETS_MAX];
UBOOL8 extMsgSet = TRUE;

/*
 * local variables
 */
static char object[BUFLEN_264] = {0};
static char action[BUFLEN_264] = {0};
static UINT16 mibUploadCounter = 0, mibUploadNumber = 0;
static UINT16 allAlarmsCounter = 0, allAlarmsNumber = 0;

// software image class Id is 7, instance is 0 or 1
#define ME_CLASS_ID_SW_IMAGE 7
#define SWDL_MEID_0          0x0000
#define SWDL_MEID_1          0x0001

static UINT16 swdlMeId = SWDL_MEID_1;

static char *OmciMsgTable[] = {
   "Command is processed successfully.",
   "Command is processed with error.",
   "Command is not supported.",
   "Command has wrong parameter.",
   "Command has unknown managed entity.",
   "Command has unknown managed entity instance.",
   "Device is busy.",
   "Managed entity instance already exists."
   };

#define NUM_OMCI_MSGS (sizeof(OmciMsgTable) / sizeof(char *))

/* local functions */

static UBOOL8 cgiOmci_isExtMsgSetOn(void);
static CmsRet cgiOmci_sendGetRequests(UBOOL8 currentFlag);
static CmsRet cgiOmci_sendSetRequests(char *query);
static CmsRet cgiOmci_sendDeleteRequests(void);
static CmsRet cgiOmci_sendMibUploadRequest(void);
static CmsRet cgiOmci_sendMibUploadNextRequest(UINT16 meId);
static CmsRet cgiOmci_sendGetAllAlarmsRequest(void);
static CmsRet cgiOmci_sendGetAllAlarmsNextRequest(UINT16 meId);
static void cgiOmciCfgView(FILE *fs);
static void writeOmciCfgHeader(FILE *fs);
static void writeOmciCfgScript(FILE *fs);
static void writeOmciCfgObject(FILE *fst);
static void writeOmciCfgParameters(FILE *fs);
static void writeOmciCfgAction(FILE *fs);
static void writeOmciCfgButton(FILE *fs);
static CmsRet cgiOmci_getNthParam(OmciParamInfo_t *schemaInfoListP,
  UINT32 paramNum, UINT32 paramIdx, OmciParamInfo_t *paramInfoP);

/* public functions */

void cgiOmci_getResultMessage(char *msg, UINT16 msgSize)
{
   UINT16 i = 0;
   char buf[BUFLEN_256];

   memset(msg, 0, msgSize);

   if (OMCI_PACKET_MSG(&packetList[0])[0] < NUM_OMCI_MSGS)
   {
      sprintf(msg, "Packet 1: %s", OmciMsgTable[OMCI_PACKET_MSG(&packetList[0])[0]]);
   }

   for (i = 1; i < numMsgRx; i++)
   {
      if (OMCI_PACKET_MSG(&packetList[i])[0] < NUM_OMCI_MSGS)
      {
         sprintf(buf, "\nPacket %d: %s", i+1, OmciMsgTable[OMCI_PACKET_MSG(&packetList[i])[0]]);
         if (strlen(msg) + strlen(buf) < msgSize)
         {
            strcat(msg, buf);
         }
         else
         {
            break;
         }
      }
   }
}

CmsRet cgiOmci_setParameterValue(omciPacket *packet, SINT32 *msgIndex,
   const OmciObjectInfo_t pathDesc, const SINT32 paramIndex, const char *paramValue)
{
   UINT32 attrSize = 0;
   OmciParamInfo_t *paramInfoList = NULL;
   SINT32 paramNum = 0;
   OmciParamInfo_t paramInfo;
   CmsRet ret = CMSRET_SUCCESS;

   if (paramValue == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }
   if (paramIndex >= NUM_PACKETS_MAX)
   {
      cmsLog_error("Invalid index %d", paramIndex);
      return CMSRET_INVALID_ARGUMENTS;
   }

   // get paramInfo in this object
   if (omciIpc_getParamInfo(pathDesc.oid, &paramInfoList, &paramNum) != 0)
   {
      cmsLog_error("get param info for object oid %d failed", pathDesc.oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   //attrNbr = paramIndex + 1
   if ((ret = cgiOmci_getNthParam(paramInfoList, paramNum,
     paramIndex + 1, &paramInfo)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of parameter %d from object id %d failed, ret=%d",
                   paramIndex + 1, pathDesc.oid, ret);
      omciIpc_free(paramInfoList);
      return CMSRET_INVALID_ARGUMENTS;
   }

   omciIpc_free(paramInfoList);

   /* get attribute size */
   attrSize = omciUtl_getParamSize(paramInfo.type, paramInfo.maxVal);

   switch(paramInfo.type)
   {
      case MPT_STRING:
      case MPT_BASE64:
         if(attrSize == 0)
         {
            cmsLog_error("invalid attribute size, object id %d, size %d",
                                         pathDesc.oid, attrSize);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            memcpy(&OMCI_PACKET_MSG(packet)[*msgIndex],
                                    (UINT8 *)paramValue, attrSize);
         }
         break;
      case MPT_HEX_BINARY:
         if(attrSize == 0)
         {
            cmsLog_error("invalid attribute size, object id %d, size %d",
                                         pathDesc.oid, attrSize);
            ret = CMSRET_INTERNAL_ERROR;
         }
         else
         {
            UINT8 *buf = NULL;
            UINT32 bufSize = 0;
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            ret = cmsUtl_hexStringToBinaryBuf(paramValue, &buf, &bufSize);
            memcpy(&OMCI_PACKET_MSG(packet)[*msgIndex], buf, bufSize);
            cmsMem_free(buf);
            bufSize = sizeof(paramValue);
         }
         break;
      case MPT_INTEGER:
      {
         UINT32 i = 0;
         SINT32 val = atoi(paramValue);
         for (i = 0; i < attrSize; i++)
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex+i] =
                                (UINT8)((val >> (8*(attrSize-1-i))) & 0xFF);
         }
         break;
      }
      case MPT_UNSIGNED_INTEGER:
      {
         UINT32 i = 0;
         UINT32 val = strtoul(paramValue, (char **)NULL, 10);
         for (i = 0; i < attrSize; i++)
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex+i] =
                                (UINT8)((val >> (8*(attrSize-1-i))) & 0xFF);
         }
         break;
      }
      case MPT_LONG64:
      {
         UINT32 i = 0;
         UINT64 val;
         cmsUtl_strtol64(paramValue, NULL, 0, (SINT64 *)&val);
         for (i = 0; i < attrSize; i++)
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex+i] =
                                (UINT8)((val >> (8*(attrSize-1-i))) & 0xFF);
         }
         break;
      }
      case MPT_UNSIGNED_LONG64:
      {
         UINT32 i = 0;
         UINT64 val;
         cmsUtl_strtoul64(paramValue, NULL, 0, (UINT64 *)&val);
         for (i = 0; i < attrSize; i++)
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex+i] =
                                (UINT8)((val >> (8*(attrSize-1-i))) & 0xFF);
         }
         break;
      }
      case MPT_BOOLEAN:
         if (strcasecmp(paramValue, "TRUE") == 0 || strcmp(paramValue, "1") == 0)
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex] = 1;
         }
         else
         {
            /* only set the first attribute that is started at OMCI_SET_OVERHEAD in msg field */
            OMCI_PACKET_MSG(packet)[*msgIndex] = 0;
         }
         break;
      default:
          cmsLog_error("invalid parameter type, %s %d",
                           paramInfo.paramName, paramInfo.type);
          ret = CMSRET_INTERNAL_ERROR;
          break;
   }

   *msgIndex += attrSize;

#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmci_setParameterValue: index=%d, type=%d, maxVal=%u, attrSize=%u, val=%s, msgIndex = %d\n",
   paramIndex, paramInfo.type, paramInfo.maxVal, attrSize, paramValue, *msgIndex);
#endif

   return ret;
}

// generate path for omci configuration file
void cgiOmci_makePathToOmci(char *buf, UINT32 bufLen, const char *fileName)
{
   CmsRet ret;
   int r;

   if ((ret = cmsUtl_getRunTimePath(OMCI_DIR, buf, bufLen)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not form root dir, ret=%d", ret);
      return;
   }

   cmsLog_debug("check access to %s", buf);

   // if /var/omci/ directory does not exist then create it
   r = mkdir(buf, 0777);
   if ((r < 0) && (errno != EEXIST))
   {
      cmsLog_error("access %s failed", buf);
      return;
   }

   if ((ret = cmsUtl_strncat(buf, bufLen, "/")) != CMSRET_SUCCESS)
   {
      cmsLog_error("buf (len=%d) is too small", bufLen);
      return;
   }

   if ((ret = cmsUtl_strncat(buf, bufLen, fileName)) != CMSRET_SUCCESS)
   {
      cmsLog_error("buf (len=%d) is too small for %s + %s",
                    bufLen, buf, fileName);
   }

   cmsLog_debug("buf=%s", buf);
   return;
}

void cgiOmci_writeOmciResult(FILE *fs, char *message)
{
   fprintf(fs, "            <br><br>\n");
   fprintf(fs, "            <font color='green'><b>4. After executing your command, the result message is shown below.</font></b><br><br>\n");
   fprintf(fs, "               <table>\n");
   fprintf(fs, "                  <tr>\n");
   fprintf(fs, "                     <td>\n");
   fprintf(fs, "                        <textarea name='result' cols='50' rows='4' wrap='virtual'>%s</textarea>\n", message);
   fprintf(fs, "                     </td>\n");
   fprintf(fs, "                  </tr>\n");
   fprintf(fs, "               </table>\n");
}

void cgiOmci_writeOmciEnd(FILE *fs)
{
   fprintf(fs, "         </form>\n");
   fprintf(fs, "      </blockquote>\n");
   fprintf(fs, "   </body>\n");
   fprintf(fs, "</html>\n");
   fflush(fs);
   fs = NULL;
}

CmsRet cgiOmci_handleResponse(const CmsMsgHeader *msgRes)
{
   CmsRet ret = CMSRET_SUCCESS;
   omciPacket *packetRes = (omciPacket *) (msgRes+1);

   /* keep value of reponse packet to use later */
   if (numMsgRx < NUM_PACKETS_MAX)
   {
      memcpy(&packetList[numMsgRx], packetRes, sizeof(omciPacket));
#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmci_handleResponse: type=%d, class=%d, inst=%d, numMsgRx = %d\n",
   OMCI_PACKET_MT(&packetList[numMsgRx]), OMCI_PACKET_ME_CLASS(&packetList[numMsgRx]),
   OMCI_PACKET_ME_INST(&packetList[numMsgRx]), numMsgRx);
#endif
      numMsgRx++;
   }

   cgiOmci_queueRawPacket(packetRes);
   if (cgiOmciRawMsgInProgress != 0) {
      cgiOmciRawMsgInProgress = 0;
      // Raw message will be polled later, do not process here
      //memcpy(&rawPacket, packetRes, sizeof(omciPacket));
      return ret;
   }

   if (OMCI_PACKET_MSG(packetRes)[0] == OMCI_MSG_RESULT_SUCCESS)
   {
      switch (OMCI_PACKET_MT(packetRes))
      {
         case OMCI_MSG_TYPE_GETNEXT:
            // if there is no error increase sequence number
            numSequence++;
            getNextInProgress = TRUE;
            break;
         case OMCI_MSG_TYPE_STARTSOFTWAREDOWNLOAD:
            dlWindowSize = OMCI_PACKET_MSG(packetRes)[1];
            dlCurrentImageLen = 0;
            dlSectionNumber = 0;
            cgiOmci_sendDownloadSection(packetRes);
            break;
         case OMCI_MSG_TYPE_DOWNLOADSECTION:
            if ((dlImageLen - dlCurrentImageLen) > 0)
            {
               cgiOmci_sendDownloadSection(packetRes);
            }
            else
            {
               cgiOmci_sendRequest(OMCI_MSG_TYPE_ENDSOFTWAREDOWNLOAD);
            }
            break;
         case OMCI_MSG_TYPE_ENDSOFTWAREDOWNLOAD:
            // free download image
            CMSMEM_FREE_BUF_AND_NULL_PTR(dlImage);
            //cgiOmci_sendRequest(OMCI_MSG_TYPE_ACTIVATESOFTWARE);
            break;
         case OMCI_MSG_TYPE_MIBUPLOAD:
            // get number of subsequent commands
            mibUploadNumber = OMCI_NTOHS(&OMCI_PACKET_MSG(packetRes)[0]);
            if (mibUploadCounter < mibUploadNumber)
               cgiOmci_sendMibUploadNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_MIBUPLOADNEXT:
            if (++mibUploadCounter < mibUploadNumber)
               cgiOmci_sendMibUploadNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_GETALLALARMS:
            // get number of subsequent commands
            allAlarmsNumber = OMCI_NTOHS(&OMCI_PACKET_MSG(packetRes)[0]);
            if (allAlarmsCounter < allAlarmsNumber)
               cgiOmci_sendGetAllAlarmsNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_GETALLALARMSNEXT:
            if (++allAlarmsCounter < allAlarmsNumber)
               cgiOmci_sendGetAllAlarmsNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         default:
            break;
      }
   }
   else
   {
      switch (OMCI_PACKET_MT(packetRes))
      {
         case OMCI_MSG_TYPE_GETNEXT:
            // if error then reset sequence number and
            // inProgress for stop refreshing web page
            numSequence = 0;
            getNextInProgress = FALSE;
            break;
         // re-send current image section if error
         case OMCI_MSG_TYPE_DOWNLOADSECTION:
            cgiOmci_sendDownloadSection(packetRes);
            break;
         case OMCI_MSG_TYPE_ENDSOFTWAREDOWNLOAD:
            if (OMCI_PACKET_MSG(packetRes)[0] == OMCI_MSG_RESULT_DEV_BUSY)
            {
               cgiOmci_sendRequest(OMCI_MSG_TYPE_ENDSOFTWAREDOWNLOAD);
            }
            else
            {
               // free download image if error
               CMSMEM_FREE_BUF_AND_NULL_PTR(dlImage);
            }
            break;
         case OMCI_MSG_TYPE_MIBUPLOAD:
            // get number of subsequent commands
            mibUploadNumber = OMCI_NTOHS(&OMCI_PACKET_MSG(packetRes)[0]);
            if (mibUploadCounter < mibUploadNumber)
               cgiOmci_sendMibUploadNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_MIBUPLOADNEXT:
            if (++mibUploadCounter < mibUploadNumber)
               cgiOmci_sendMibUploadNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_GETALLALARMS:
            // get number of subsequent commands
            allAlarmsNumber = OMCI_NTOHS(&OMCI_PACKET_MSG(packetRes)[0]);
            if (allAlarmsCounter < allAlarmsNumber)
               cgiOmci_sendGetAllAlarmsNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         case OMCI_MSG_TYPE_GETALLALARMSNEXT:
            if (++allAlarmsCounter < allAlarmsNumber)
               cgiOmci_sendGetAllAlarmsNextRequest(OMCI_PACKET_ME_INST(packetRes));
            break;
         default:
            break;
      }
   }

   return ret;
}

CmsRet cgiOmci_sendRequest(omciMsgType msgType)
{
   UINT32 imageCrc = 0;
   char buf[sizeof(CmsMsgHeader) + sizeof(omciPacket)];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   msg->sequenceNumber = 1;

   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;
   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(msgType);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 0 byte
   omciReqMsgSetMsgLen(packet, extMsgSet, 0);

   // setup message identifier for software download
   switch (msgType)
   {
      case OMCI_MSG_TYPE_STARTSOFTWAREDOWNLOAD:
         OMCI_HTONS(&packet->classNo[0], ME_CLASS_ID_SW_IMAGE);
         OMCI_HTONS(&packet->instId[0], swdlMeId);
         if (extMsgSet == TRUE)
         {
             // set to 5 bytes: window size (1) + image size (4)
             OMCI_HTONS(&packet->B.msgLen, 5);
         }
         OMCI_PACKET_MSG(packet)[0] = dlWindowSize;
         // dlImageLen should have its value in cgiOmciDownloadPre() function
         OMCI_HTONL(&OMCI_PACKET_MSG(packet)[1], dlImageLen);
         break;
      case OMCI_MSG_TYPE_ENDSOFTWAREDOWNLOAD:
         OMCI_HTONS(&packet->classNo[0], ME_CLASS_ID_SW_IMAGE);
         OMCI_HTONS(&packet->instId[0], swdlMeId);
         if (extMsgSet == TRUE)
         {
             // set to 8 bytes: crc (4) + image size (4)
             OMCI_HTONS(&packet->B.msgLen, 8);
         }
         // calculate image CRC
         imageCrc = omciUtl_getCrc32(-1, dlImage, dlImageLen);
         OMCI_HTONL(&OMCI_PACKET_MSG(packet)[0], imageCrc);
         // dlImageLen should have its value in cgiOmciDownloadPre() function
         OMCI_HTONL(&OMCI_PACKET_MSG(packet)[4], dlImageLen);
         break;
      case OMCI_MSG_TYPE_ACTIVATESOFTWARE:
         OMCI_HTONS(&packet->classNo[0], ME_CLASS_ID_SW_IMAGE);
         OMCI_HTONS(&packet->instId[0], swdlMeId);
         break;
      case OMCI_MSG_TYPE_COMMITSOFTWARE:
         OMCI_HTONS(&packet->classNo[0], ME_CLASS_ID_SW_IMAGE);
         OMCI_HTONS(&packet->instId[0], swdlMeId);
         break;
      default:
         break;
   }

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      char fileName[BUFLEN_264];
      FILE *fs = NULL;
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs != NULL)
      {
         UINT16 i = 0, msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
         for (i = 0; i < msgSize; i++)
         {
            fputc(buf[i], fs);
         }
         fclose(fs);
      }
      else
      {
         cmsLog_error("fopen(%s) failed.", fileName);
      }
   }
   else
   {
      if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST "
           "with msgType=%d, ret=%d", msgType, ret);
      }
      else
      {
         cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = %d",
           msgType);
         numMsgTx++;
       }
   }

#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmci_sendRequest, tcId = %d, msgType = %d, numMsgTx = %d\n",
   tcIdCur, msgType, numMsgTx);
#endif
   return ret;
}

// Main entry for OMCI configuration web pages
void cgiOmciCfg(char *query, FILE *fs)
{
   object[0] = action[0] = '\0';

   extMsgSet = cgiOmci_isExtMsgSetOn();

   /* OMCI operations do not need CMS lock. */
   cmsLck_releaseLock();

   cgiGetValueByName(query, "selAction", action);
   cgiGetValueByName(query, "selObject", object);

   if (cmsUtl_strcmp(action, "meget") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         memset(packetList, 0x0, sizeof(omciPacket) * NUM_PACKETS_MAX);
         cgiOmci_sendGetRequests(FALSE);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "mecurrent") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         memset(packetList, 0x0, sizeof(omciPacket) * NUM_PACKETS_MAX);
         cgiOmci_sendGetRequests(TRUE);
      }
      cgiOmciCfgView(fs);
   }

   else if (cmsUtl_strcmp(action, "meset") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendSetRequests(query);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "delete") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendDeleteRequests();
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "mibupload") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         mibUploadCounter = mibUploadNumber = 0;
         cgiOmci_sendMibUploadRequest();
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "getallalarms") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         allAlarmsCounter = allAlarmsNumber = 0;
         cgiOmci_sendGetAllAlarmsRequest();
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "synctime") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendRequest(OMCI_MSG_TYPE_SYNCHRONIZETIME);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "reboot") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendRequest(OMCI_MSG_TYPE_REBOOT);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "mibreset") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendRequest(OMCI_MSG_TYPE_MIBRESET);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "activate") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendRequest(OMCI_MSG_TYPE_ACTIVATESOFTWARE);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "commit") == 0)
   {
      if (numMsgTx == 0)
      {
         numMsgRx = 0;
         cgiOmci_sendRequest(OMCI_MSG_TYPE_COMMITSOFTWARE);
      }
      cgiOmciCfgView(fs);
   }
   else if (cmsUtl_strcmp(action, "reset") == 0)
   {
      // action is reset so clear all selected value
      object[0] = action[0] = '\0';
      numMsgTx = numMsgRx = 0;
      cgiOmciCfgView(fs);
   }
   else
   {
      numMsgTx = numMsgRx = 0;
      cgiOmciCfgView(fs);
   }

   (void)cmsLck_acquireLockWithTimeout(HTTPD_LOCK_TIMEOUT);
}

CmsRet cgiOmci_getMeId(const MdmObjectId oid,
  const InstanceIdStack iidStack, UINT16 *meIdP)
{
   OmciGenObject *genObj = NULL;

   if (omciIpc_getObject(oid, &iidStack, (void**)&genObj) != 0)
   {
      cmsLog_error("Get object for oid %d failed", oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   *meIdP = genObj->managedEntityId;
   omciIpc_free(genObj);
   return CMSRET_SUCCESS;
}

/* local functions */

UBOOL8 cgiOmci_getMeClass(UINT16 oid, UINT16 *meClass)
{
   OmciMdmOidClassId_t info;

   // convert object OID in pathDesc to OMCI ME class ID
   memset(&info, 0, sizeof(OmciMdmOidClassId_t));
   info.mdmOid = oid;
   omciUtl_oidToClassId(&info);
   if (info.classId == 0)
   {
      // not an OMCI ME object
      return FALSE;
   }

   *meClass = info.classId;
   return TRUE;
}

static UBOOL8 cgiOmci_isExtMsgSetOn(void)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *omciSys = NULL;
    UBOOL8 extMsgOn = FALSE;

    if (cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&omciSys) == CMSRET_SUCCESS)
    {
        if (omciSys->extMsgSetEnable == TRUE)
        {
            extMsgOn = TRUE;
        }
        cmsObj_free((void**)&omciSys);
    }
    else
    {
        cmsLog_error("get of OMCI system object failed");
    }

    return extMsgOn;
}

static CmsRet cgiOmci_getNthParam(OmciParamInfo_t *schemaInfoListP,
  UINT32 paramNum, UINT32 paramIdx, OmciParamInfo_t *paramInfoP)
{
    OmciParamInfo_t *paramNodeP = NULL;

    if ((schemaInfoListP == NULL) || (paramInfoP == NULL) ||
      (paramIdx >= paramNum))
    {
        cmsLog_error("Invalid arguments");
        return CMSRET_INVALID_ARGUMENTS;
    }

    paramNodeP = schemaInfoListP + paramIdx;

    memset(paramInfoP, 0x0, sizeof(OmciParamInfo_t));
    strcpy(paramInfoP->paramName, paramNodeP->paramName);
    paramInfoP->type = paramNodeP->type;
    paramInfoP->minVal = (UINT32)paramNodeP->minVal;
    paramInfoP->maxVal = (UINT32)paramNodeP->maxVal;
    paramInfoP->offsetInObject = paramNodeP->offsetInObject;

    return CMSRET_SUCCESS;
}

CmsRet cgiOmci_getParameterValue(const MdmObjectId oid,
  const InstanceIdStack iidStack  __attribute__((unused)), const SINT32 paramIndex,
  char *paramValue, const UINT16 paramSize)
{
   UINT32 attrSize;
   OmciParamInfo_t paramInfo;
   CmsRet ret = CMSRET_SUCCESS;
   OmciParamInfo_t *paramInfoList = NULL;
   SINT32 paramNum = 0;
   UINT8 attrOverhead;
   omciPacket *pPacket;

   if (paramValue == NULL)
   {
      return CMSRET_INVALID_ARGUMENTS;
   }

   memset(paramValue, 0, paramSize);

   if (paramIndex >= NUM_PACKETS_MAX)
   {
      cmsLog_error("Invalid index %d", paramIndex);
      return CMSRET_INVALID_ARGUMENTS;
   }

   pPacket = &packetList[paramIndex];
   if (pPacket == NULL)
   {
      cmsLog_error("Invalid packet, index %d", paramIndex);
      return CMSRET_INVALID_ARGUMENTS;
   }

   // get paramInfo in this object
   if (omciIpc_getParamInfo(oid, &paramInfoList, &paramNum) != 0)
   {
      cmsLog_error("get param info for object oid %d failed", oid);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* paramIndex is the index of packetList
    * paramIndex is also equal to attrNbr - 1
    */
   if ((ret = cgiOmci_getNthParam(paramInfoList, paramNum,
     paramIndex + 1, &paramInfo)) != CMSRET_SUCCESS)
   {
      cmsLog_error("get of parameter %d from object id %d failed, ret=%d",
                   paramIndex + 1, oid, ret);
      omciIpc_free(paramInfoList);
      return CMSRET_INVALID_ARGUMENTS;
   }

   // free object
   omciIpc_free(paramInfoList);

   /* get attribute size */
   attrSize = omciUtl_getParamSize(paramInfo.type, paramInfo.maxVal);

   /* Only get the first attribute, as Get request sets only one. */
   switch (paramInfo.type)
   {
      case MPT_STRING:
      case MPT_BASE64:
         if (attrSize == 0)
         {
            cmsLog_error("invalid attribute size, object id %d, size %d",
              oid, attrSize);
            ret = CMSRET_INTERNAL_ERROR;
            break;
         }

         if (paramSize < attrSize)
         {
            cmsLog_error("Parameter size is %d smaller than actual value size %d",
              paramSize, attrSize);
            ret = CMSRET_INVALID_ARGUMENTS;
            break;
         }
         attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
         memcpy(paramValue, &OMCI_PACKET_MSG(pPacket)[attrOverhead], attrSize);
         break;
      case MPT_HEX_BINARY:
         if (attrSize == 0)
         {
            cmsLog_error("invalid attribute size, object id %d, size %d",
              oid, attrSize);
            ret = CMSRET_INTERNAL_ERROR;
            break;
         }
         if (paramSize < (attrSize * 2))
         {
            cmsLog_error("Parameter size is %d smaller than actual value size %d",
              paramSize, (attrSize * 2));
            ret = CMSRET_INVALID_ARGUMENTS;
            break;
         }
         {
             char *hexString = NULL;

             attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
             ret = cmsUtl_binaryBufToHexString(&OMCI_PACKET_MSG(pPacket)[attrOverhead],
               attrSize, &hexString);
             strcpy(paramValue, hexString);
             cmsMem_free(hexString);
         }
         break;
      case MPT_INTEGER:
      {
         UINT32 i;
         SINT32 val = 0;

         for (i = 0; i < attrSize; i++)
         {
            attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
            val |= OMCI_PACKET_MSG(pPacket)[attrOverhead + i] << ATTR_BYTE_POS(attrSize, i);
         }

         sprintf(paramValue, "%d", ((attrSize == 1) ? (char)val :
           ((attrSize == 2) ? (short)val : (int)val)));
         break;
      }
      case MPT_UNSIGNED_INTEGER:
      {
         UINT32 i;
         UINT32 val = 0;

         for (i = 0; i < attrSize; i++)
         {
            attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
            val |= OMCI_PACKET_MSG(pPacket)[attrOverhead + i] << ATTR_BYTE_POS(attrSize, i);
         }
         sprintf(paramValue, "%u", val);
         break;
      }

      case MPT_LONG64:
      {
         UINT32 i;
         UINT64 val = 0;
         UINT8 *tempPtr;

         attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
         tempPtr = (UINT8*)&(OMCI_PACKET_MSG(pPacket)[attrOverhead]);
         for (i = 0;i < attrSize; i++)
         {
            val <<= 8;
            val |= *tempPtr++;
         }
         sprintf(paramValue, "%" PRId64, val);
         break;
      }
      case MPT_UNSIGNED_LONG64:
      {
         UINT32 i;
         UINT64 val = 0;
         UINT8 *tempPtr;

         attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
         tempPtr = (UINT8*)&(OMCI_PACKET_MSG(pPacket)[attrOverhead]);
         for (i = 0; i < attrSize;i++)
         {
            val <<= 8;
            val |= *tempPtr++;
         }
         sprintf(paramValue, "%" PRIu64, val);
         break;
      }
      case MPT_BOOLEAN:
         attrOverhead = GETRSP_ATTRVAL_OVERHEAD(pPacket);
         if (OMCI_PACKET_MSG(pPacket)[attrOverhead] == 1)
         {
            strcpy(paramValue, "TRUE");
         }
         else
         {
            strcpy(paramValue, "FALSE");
         }
         break;
      default:
         cmsLog_error("invalid parameter type, %s %d",
           paramInfo.paramName, paramInfo.type);
         ret = CMSRET_INTERNAL_ERROR;
         break;
   }

#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmci_getParameterValue: index=%d, type=%d, maxVal=%u, attrSize=%u, val=%s\n",
   paramIndex, paramInfo.type, paramInfo.maxVal, attrSize, paramValue);
#endif

   return ret;
}

static CmsRet cgiOmci_sendGetRequests(UBOOL8 currentFlag)
{
   SINT32 numInfoEntries = 0, i = 0;
   UINT16 j = 0, msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char fullPathName[BUFLEN_1024];
   OmciParamInfo_t *paramInfoList;
   OmciObjectInfo_t pathDesc;
   UINT16 meClass = 0, meInst = 0, meAttrMask = 0;
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cmsUtl_strcmp(object, "") == 0 ||
       omciIpc_fullPathToPathDescriptor(object, &pathDesc) != 0)
   {
      // invalid object
      return ret;
   }

   if (cgiOmci_getMeClass((UINT16)pathDesc.oid, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   if (omciIpc_getParamInfo(pathDesc.oid, &paramInfoList, &numInfoEntries) != 0)
   {
      // failed operation
      return ret;
   }

   strcpy(fullPathName, object);
   strcat(fullPathName, paramInfoList[0].paramName);
   // convert Management Object ID full path to path descriptor
   omciIpc_fullPathToPathDescriptor(fullPathName, &pathDesc);
   // get Management Object ID value using its full path
   ret = cgiOmci_getMeId(pathDesc.oid, pathDesc.iidStack, &meInst);
   if (ret != CMSRET_SUCCESS)
   {
       omciIpc_free(paramInfoList);
       return ret;
   }

   // update swdlMeId for software image class 7, so that
   // it can be either SWDL_MEID_0 or SWDL_MEID_1
   if (meClass == ME_CLASS_ID_SW_IMAGE)
   {
      swdlMeId = meInst;
   }

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         // free parameter info list
         omciIpc_free(paramInfoList);
         return CMSRET_OPEN_FILE_ERROR;
      }
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   for (i = 1;
        i < numInfoEntries &&
        (i-1) < NUM_PACKETS_MAX &&
        paramInfoList[i].paramName[0] != '\0';
        i++)
   {
      // message sequence number is position of parameter in the object
      msg->sequenceNumber = i - 1;
      // transaction ID
      OMCI_HTONS(&packet->tcId, tcIdCur);
      tcIdCur++;
      // message type
      if (currentFlag != TRUE)
      {
          packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_GET);
      }
      else
      {
          packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_GETCURRENTDATA);
      }
      // device ID
      omciReqMsgSetDevId(packet, extMsgSet);
      // ext msg len, fixed to 2 bytes
      omciReqMsgSetMsgLen(packet, extMsgSet, 2);

      OMCI_HTONS(&packet->classNo, meClass);
      OMCI_HTONS(&packet->instId, meInst);

      // attribute mask
      meAttrMask = 0x8000 >> (i - 1);
      OMCI_HTONS(&OMCI_PACKET_MSG(packet)[0], meAttrMask);

      cgiOmci_addCrc(packet, EID_HTTPD);

      if (macroState == OMCI_MARCO_ON)
      {
         for (j = 0; j < msgSize; j++)
         {
            fputc(buf[j], fs);
         }
      }

      if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST, "
           "ret=%d, id = %d", ret, i);
      }
      else
      {
         cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType "
           "= %d, id = %d", OMCI_MSG_TYPE_GET, i);
         numMsgTx++;
      }
   }

   if (macroState == OMCI_MARCO_ON)
   {
      fclose(fs);
   }
   // free parameter info list
   omciIpc_free(paramInfoList);

   return ret;
}

static CmsRet cgiOmci_sendSetRequests(char *query)
{
   SINT32 numInfoEntries = 0, i = 0, msgIndex = 0;
   UINT16 j = 0, msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   UINT16 contentSize = 0;
   char fullPathName[BUFLEN_1024], paramName[BUFLEN_16], paramValue[BUFLEN_256];
   OmciParamInfo_t *paramInfoList;
   OmciObjectInfo_t pathDesc;
   UINT16 meClass = 0, meInst = 0, meAttrMask = 0;
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cmsUtl_strcmp(object, "") == 0 ||
       omciIpc_fullPathToPathDescriptor(object, &pathDesc) != 0)
   {
      // invalid object
      return ret;
   }

   if (cgiOmci_getMeClass((UINT16)pathDesc.oid, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   if (omciIpc_getParamInfo(pathDesc.oid, &paramInfoList, &numInfoEntries) != 0)
   {
      // failed operation
      return ret;
   }

   strcpy(fullPathName, object);
   strcat(fullPathName, paramInfoList[0].paramName);
   // convert Management Object ID full path to path descriptor
   omciIpc_fullPathToPathDescriptor(fullPathName, &pathDesc);
   // get Management Object ID value using its full path
   ret = cgiOmci_getMeId(pathDesc.oid, pathDesc.iidStack, &meInst);
   if (ret != CMSRET_SUCCESS)
   {
       omciIpc_free(paramInfoList);
       return ret;
   }

   // initialize paramName and paramValue
   memset(paramName, 0, BUFLEN_16);
   memset(paramValue, 0, BUFLEN_256);
   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         // free parameter info list
         omciIpc_free(paramInfoList);
         return CMSRET_OPEN_FILE_ERROR;
      }
   }

   for (i = 1;
        i < numInfoEntries &&
        (i-1) < NUM_PACKETS_MAX &&
        paramInfoList[i].paramName[0] != '\0';
        i++)
   {
      // skip parameter that cannot be writable
      if (paramInfoList[i].writable == TRUE)
      {
         // Init request message
         memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
         omciReqMsgInit(msg);

         // message sequence number is position of parameter in the object
         msg->sequenceNumber = i - 1;
         // transaction ID
         OMCI_HTONS(&packet->tcId, tcIdCur);
         tcIdCur++;
         // message type
         packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_SET);
         // device ID
         omciReqMsgSetDevId(packet, extMsgSet);
         // ext msg len
         if (extMsgSet == TRUE)
         {
             // one attribute + 2 byte attribute mask at a time
             contentSize = omciUtl_getParamSize(paramInfoList[i].type,
               paramInfoList[i].maxVal) + 2;
             OMCI_HTONS(&packet->B.msgLen, contentSize);
         }

         OMCI_HTONS(&packet->classNo, meClass);
         OMCI_HTONS(&packet->instId, meInst);

         // attribute mask
         meAttrMask = 0x8000 >> (i - 1);
         OMCI_HTONS(&OMCI_PACKET_MSG(packet)[0], meAttrMask);
         // message offset
         msgIndex = OMCI_SET_OVERHEAD;
         // attribute value
         sprintf(paramName, "param%d", i);
         cgiGetValueByName(query, paramName, paramValue);
         cgiOmci_setParameterValue(packet, &msgIndex, pathDesc, i - 1, paramValue);
         cgiOmci_addCrc(packet, EID_HTTPD);

         if (macroState == OMCI_MARCO_ON)
         {
            for (j = 0; j < msgSize; j++)
            {
               fputc(buf[j], fs);
            }
         }

         if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
         {
            cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST, "
              "ret=%d, id = %d", ret, i);
         }
         else
         {
            cmsLog_notice("sent CMS_MSG_OMCI_COMMAND_REQUEST with msgType = "
              "%d, id = %d", OMCI_MSG_TYPE_SET, i);
            numMsgTx++;
         }
      }
   }

   if (macroState == OMCI_MARCO_ON)
   {
      fclose(fs);
   }

   // free parameter info list
   omciIpc_free(paramInfoList);

   return ret;
}

static CmsRet cgiOmci_sendDeleteRequests(void)
{
   SINT32 numInfoEntries = 0, i = 0;
   UINT16 j = 0, msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char fullPathName[BUFLEN_1024];
   OmciParamInfo_t *paramInfoList;
   OmciObjectInfo_t pathDesc;
   UINT16 meClass = 0, meInst = 0;
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cmsUtl_strcmp(object, "") == 0 ||
       omciIpc_fullPathToPathDescriptor(object, &pathDesc) != 0)
   {
      // invalid object
      return ret;
   }

   if (cgiOmci_getMeClass((UINT16)pathDesc.oid, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   if (omciIpc_getParamInfo(pathDesc.oid, &paramInfoList, &numInfoEntries) != 0)
   {
      // failed operation
      return ret;
   }

   // get Management Object ID of the second item [1] in the list
   strcpy(fullPathName, object);
   strcat(fullPathName, paramInfoList[1].paramName);
   // convert Management Object ID full path to path descriptor
   omciIpc_fullPathToPathDescriptor(fullPathName, &pathDesc);
   // get Management Object ID value using its full path
   ret = cgiOmci_getMeId(pathDesc.oid, pathDesc.iidStack, &meInst);
   if (ret != CMSRET_SUCCESS)
   {
       omciIpc_free(paramInfoList);
       return ret;
   }

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         // free parameter info list
         omciIpc_free(paramInfoList);
         return CMSRET_OPEN_FILE_ERROR;
      }
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   // message sequence number
   msg->sequenceNumber = 0;
   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;
   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_DELETE);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 0 byte
   omciReqMsgSetMsgLen(packet, extMsgSet, 0);

   OMCI_HTONS(&packet->classNo, meClass);
   OMCI_HTONS(&packet->instId, meInst);

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      for (j = 0; j < msgSize; j++)
      {
         fputc(buf[j], fs);
      }
   }

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST, "
        "ret=%d, id = %d", ret, i);
   }
   else
   {
      cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = "
        "%d, id = %d", OMCI_MSG_TYPE_DELETE, i);
      numMsgTx++;
   }

   if (macroState == OMCI_MARCO_ON)
   {
      fclose(fs);
   }

   // free parameter info list
   omciIpc_free(paramInfoList);

#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmci_sendDeleteRequests: object=%s, oid=%d, ret=%d\n",
     object, pathDesc.oid, ret);
#endif

   return ret;
}

static CmsRet cgiOmci_sendMibUploadRequest(void)
{
   UINT16 i = 0, meClass = 0, meInst = 0;
   UINT16 msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cgiOmci_getMeClass((UINT16)MDMOID_ONT_DATA, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   msg->sequenceNumber = 0;

   meInst = MDMOID_ONT_DATA_MEID;

   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;
   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_MIBUPLOAD);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 0 byte
   omciReqMsgSetMsgLen(packet, extMsgSet, 0);

   OMCI_HTONS(&packet->classNo, meClass);
   OMCI_HTONS(&packet->instId, meInst);

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         return CMSRET_OPEN_FILE_ERROR;
      }
      for (i = 0; i < msgSize; i++)
      {
         fputc(buf[i], fs);
      }
      fclose(fs);
   }

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST "
        "with msgType=%d, ret=%d", OMCI_MSG_TYPE_MIBUPLOAD, ret);
   }
   else
   {
      cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = %d",
        OMCI_MSG_TYPE_MIBUPLOAD);
      numMsgTx++;
   }

   return ret;
}

static CmsRet cgiOmci_sendMibUploadNextRequest(UINT16 meInst)
{
   UINT16 i = 0, meClass = 0;
   UINT16 msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cgiOmci_getMeClass((UINT16)MDMOID_ONT_DATA, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;
   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_MIBUPLOADNEXT);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 2 bytes
   omciReqMsgSetMsgLen(packet, extMsgSet, 2);

   OMCI_HTONS(&packet->classNo, meClass);
   OMCI_HTONS(&packet->instId, meInst);
   // command sequence number in MibUploadNext
   OMCI_HTONS(&OMCI_PACKET_MSG(packet)[0], mibUploadCounter);

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         return CMSRET_OPEN_FILE_ERROR;
      }
      for (i = 0; i < msgSize; i++)
      {
         fputc(buf[i], fs);
      }
      fclose(fs);
   }

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST "
        "with msgType=%d, ret=%d", OMCI_MSG_TYPE_MIBUPLOADNEXT, ret);
   }
   else
   {
      cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = %d",
        OMCI_MSG_TYPE_MIBUPLOADNEXT);
      numMsgTx++;
   }

   return ret;
}

static CmsRet cgiOmci_sendGetAllAlarmsRequest(void)
{
   UINT16 i = 0, meClass = 0, meInst = 0;
   UINT16 msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cgiOmci_getMeClass((UINT16)MDMOID_ONT_DATA, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   msg->sequenceNumber = 0;

   meInst = MDMOID_ONT_DATA_MEID;

   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;

   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_GETALLALARMS);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 1 byte
   omciReqMsgSetMsgLen(packet, extMsgSet, 1);

   OMCI_HTONS(&packet->classNo, meClass);
   OMCI_HTONS(&packet->instId, meInst);

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         return CMSRET_OPEN_FILE_ERROR;
      }
      for (i = 0; i < msgSize; i++)
      {
         fputc(buf[i], fs);
      }
      fclose(fs);
   }

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST "
        "with msgType=%d, ret=%d", OMCI_MSG_TYPE_GETALLALARMS, ret);
   }
   else
   {
      cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = %d",
        OMCI_MSG_TYPE_GETALLALARMS);
      numMsgTx++;
   }

   return ret;
}

static CmsRet cgiOmci_sendGetAllAlarmsNextRequest(UINT16 meInst)
{
   UINT16 i = 0, meClass = 0;
   UINT16 msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
   char buf[msgSize];
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   omciPacket *packet = (omciPacket *) (msg+1);
   CmsRet ret = CMSRET_SUCCESS;
   char fileName[BUFLEN_264];
   FILE *fs = NULL;

   if (cgiOmci_getMeClass((UINT16)MDMOID_ONT_DATA, &meClass) != TRUE)
   {
      // not an OMCI ME object
      return ret;
   }

   // Init request message
   memset(buf, 0, sizeof(CmsMsgHeader) + sizeof(omciPacket));
   omciReqMsgInit(msg);

   msg->sequenceNumber = 0;

   // transaction ID
   OMCI_HTONS(&packet->tcId, tcIdCur);
   tcIdCur++;

   // message type
   packet->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_GETALLALARMSNEXT);
   // device ID
   omciReqMsgSetDevId(packet, extMsgSet);
   // ext msg len, fixed to 2 bytes
   omciReqMsgSetMsgLen(packet, extMsgSet, 2);

   OMCI_HTONS(&packet->classNo, meClass);
   OMCI_HTONS(&packet->instId, meInst);
   // command sequence number in AllAlarmsNext
   OMCI_HTONS(&OMCI_PACKET_MSG(packet)[0], allAlarmsCounter);

   cgiOmci_addCrc(packet, EID_HTTPD);

   if (macroState == OMCI_MARCO_ON)
   {
      cgiOmci_makePathToOmci(fileName, BUFLEN_264, OMCI_CONF_NAME);
      fs = fopen(fileName, "ab");
      if (fs == NULL)
      {
         cmsLog_error("fopen(%s) failed.", fileName);
         return CMSRET_OPEN_FILE_ERROR;
      }
      for (i = 0; i < msgSize; i++)
      {
         fputc(buf[i], fs);
      }
      fclose(fs);
   }

   if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not send out CMS_MSG_OMCI_COMMAND_REQUEST "
        "with msgType=%d, ret=%d", OMCI_MSG_TYPE_GETALLALARMSNEXT, ret);
   }
   else
   {
      cmsLog_notice("Send CMS_MSG_OMCI_COMMAND_REQUEST with msgType = %d",
        OMCI_MSG_TYPE_GETALLALARMSNEXT);
      numMsgTx++;
   }

   return ret;
}

// OMCI configuration main page
static void cgiOmciCfgView(FILE *fs)
{
   char msg[BUFLEN_1024*2];

#ifdef OMCI_CFG_DEBUG
   printf("===> cgiOmciCfgView: object = %s, action = %s, numMsgTx = %d, numMsgRx = %d\n",
   object, action, numMsgTx, numMsgRx);
#endif

   writeOmciCfgHeader(fs);
   writeOmciCfgScript(fs);
   writeOmciCfgObject(fs);
   writeOmciCfgParameters(fs);
   writeOmciCfgAction(fs);
   writeOmciCfgButton(fs);

   if (cmsUtl_strcmp(action, "meget") == 0 ||
       cmsUtl_strcmp(action, "mecurrent") == 0 ||
       cmsUtl_strcmp(action, "meset") == 0 ||
       cmsUtl_strcmp(action, "delete") == 0 ||
       cmsUtl_strcmp(action, "mibupload") == 0 ||
       cmsUtl_strcmp(action, "getallalarms") == 0 ||
       cmsUtl_strcmp(action, "synctime") == 0 ||
       cmsUtl_strcmp(action, "reboot") == 0 ||
       cmsUtl_strcmp(action, "mibreset") == 0 ||
       cmsUtl_strcmp(action, "activate") == 0 ||
       cmsUtl_strcmp(action, "commit") == 0)
   {
      if (numMsgTx == numMsgRx)
      {
         cgiOmci_getResultMessage(msg, BUFLEN_1024*2);
         if (macroState == OMCI_MARCO_OFF)
         {
            cgiOmci_writeOmciResult(fs, msg);
         }
         else
         {
            if (cmsUtl_strcmp(action, "reboot") == 0)
               cgiOmci_writeOmciResult(fs, "REBOOT OMCI command is only saved to memory, and does not send to OMCID.");
            else if (cmsUtl_strcmp(action, "activate") == 0)
               cgiOmci_writeOmciResult(fs, "ACTIVATESOFTWARE OMCI command is only saved to memory, and does not send to OMCID.");
            else
               cgiOmci_writeOmciResult(fs, msg);
         }
         numRetry = numMsgTx = numMsgRx = 0;
      }
      else
      {
         if (numRetry < NUM_RETRIES_MAX)
         {
            if (cmsUtl_strcmp(action, "meget") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending multiple GET OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "mecurrent") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending multiple GET Current OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "meset") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending multiple SET OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "delete") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending DELETE OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "mibupload") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending MIBUPLOAD OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "getallalarms") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending GETALLALARMS OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "synctime") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending SYNCTIME OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "reboot") == 0)
            {
               if (macroState == OMCI_MARCO_ON)
               {
                  cgiOmci_writeOmciResult(fs, "REBOOT OMCI command is only saved to memory, and does not send to OMCID.");
               }
               else
               {
                  cgiOmci_writeOmciResult(fs, "HTTPD is sending REBOOT OMCI commands to OMCID. Please wait...");
               }
            }
            else if (cmsUtl_strcmp(action, "mibreset") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending MIB RESET OMCI commands to OMCID. Please wait...");
            }
            else if (cmsUtl_strcmp(action, "activate") == 0)
            {
               if (macroState == OMCI_MARCO_ON)
               {
                  cgiOmci_writeOmciResult(fs, "ACTIVATESOFTWARE OMCI command is only saved to memory, and does not send to OMCID.");
               }
               else
               {
                  cgiOmci_writeOmciResult(fs, "HTTPD is sending ACTIVATESOFTWARE OMCI commands to OMCID. Please wait...");
               }
            }
            else if (cmsUtl_strcmp(action, "commit") == 0)
            {
               cgiOmci_writeOmciResult(fs, "HTTPD is sending COMMITSOFTWARE OMCI commands to OMCID. Please wait...");
            }
            numRetry++;
         }
         else
         {
            sprintf(msg, "Failed to communicate with OMCID after %d retries.", numRetry);
            cgiOmci_writeOmciResult(fs, msg);
            numRetry = numMsgTx = numMsgRx = 0;
         }
      }
   }
   else
   {
      cgiOmci_writeOmciResult(fs, "");
   }

   cgiOmci_writeOmciEnd(fs);
}

// Header
static void writeOmciCfgHeader(FILE *fs)
{
   fprintf(fs, "<html>\n   <head>\n");
   fprintf(fs, "      <meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");
   fprintf(fs, "         <link rel=stylesheet href='stylemain.css' type='text/css'>\n");
   fprintf(fs, "         <link rel=stylesheet href='colors.css' type='text/css'>\n");
   fprintf(fs, "            <script language=\"javascript\" src=\"util.js\"></script>\n");
   fprintf(fs, "   </head>\n");
   if (numMsgTx == numMsgRx || numRetry == NUM_RETRIES_MAX)
   {
      fprintf(fs, "   <body>\n");
   }
   else
   {
      fprintf(fs, "   <body onLoad='frmLoad()'>\n");
   }
   fprintf(fs, "      <blockquote>\n         <form>\n");
}

// Scripts
static void writeOmciCfgScript(FILE *fs)
{
   UINT16 i = 0;

   fprintf(fs, "<script language='javascript'>\n");
   fprintf(fs, "<!-- hide\n\n");

   /*generate new session key in glbCurrSessionKey*/
   cgiGetCurrSessionKey(0,NULL,NULL);

   if (numMsgTx != numMsgRx && numRetry < NUM_RETRIES_MAX)
   {
      fprintf(fs, "function frmLoad() {\n");
      fprintf(fs, "   setTimeout('btnApply()', 1000);\n");
      fprintf(fs, "   with ( document.forms[0] ) {\n");
      fprintf(fs, "      applyBtn.disabled = true;\n");
      fprintf(fs, "      resetBtn.disabled = true;\n");
      fprintf(fs, "   }\n");
      fprintf(fs, "}\n\n");
   }

   fprintf(fs, "function btnApply() {\n");
   fprintf(fs, "   var loc = 'omcicfg.cmd?';\n\n");
   fprintf(fs, "   with ( document.forms[0] ) {\n");
   fprintf(fs, "      var object = getSelect(selObject);\n");
   fprintf(fs, "      var action = getSelect(selAction);\n");
   fprintf(fs, "      loc += 'selObject=' + object;\n");
   fprintf(fs, "      loc += submitSelect(selAction);\n");
   fprintf(fs, "      if (action.search('meset') > 0) {\n");
   // index 1 is management object id which cannot be setable
   // so the parameter that is available to set is started at 2
   for (i = 1; i < numMsgTx + 1; i++)
   {
      fprintf(fs, "            loc += '&param%d=' + param%d.value;\n", i, i);
   }
   fprintf(fs, "      }\n");
   fprintf(fs, "   }\n");
   fprintf(fs, "   loc += '&sessionKey=%d';\n",glbCurrSessionKey);
   fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");

   fprintf(fs, "function btnReset() {\n");
   fprintf(fs, "   var loc = 'omcicfg.cmd?action=reset';\n\n");
   fprintf(fs, "   loc += '&sessionKey=%d';\n",glbCurrSessionKey);
   fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");

   fprintf(fs, "// done hiding -->\n");
   fprintf(fs, "</script>\n");
}

static void writeOmciCfgObject(FILE *fs)
{
   SINT32 numEntries = 0;
   UINT32 j = 0;
   char *mdmPath = NULL;
   OmciObjectInfo_t *objectInfoListP;

   fprintf(fs, "            <b>OMCI -- Configuration</b><br><br>\n");
   fprintf(fs, "            This page allows you to configure CPE through OMCI protocol.<br><br>\n");
   if (macroState == OMCI_MARCO_ON)
   {
      fprintf(fs, "            <font color='red'><b>%s</b></font>\n", OMCI_MSG_MACRO_ON);
   }
   else
   {
      fprintf(fs, "            <font color='green'><b>%s</b></font>\n", OMCI_MSG_MACRO_OFF);
   }
   fprintf(fs, "            <br><br>\n");
   fprintf(fs, "            You can follow the steps below to customize your configuration.<br><br>\n");
   fprintf(fs, "            <font color='green'><b>1. Select the object in the Object list.</font></b><br><br>\n");
   fprintf(fs, "            <table border='0' cellpadding='0' cellspacing='0'>\n");
   fprintf(fs, "               <tr>\n");
   fprintf(fs, "                  <td width='90'>Object:</td>\n");
   fprintf(fs, "                  <td><select name='selObject' size='1'>\n");
   fprintf(fs, "                        <option value=''>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</option>\n");

   if (omciIpc_getObjectInfo(&objectInfoListP, &numEntries) == 0)
   {
      for (j = 0; j < (UINT32)numEntries; j++)
      {
         if (omciIpc_pathDescriptorToFullPath(&(objectInfoListP[j]), &mdmPath) == 0)
         {
            if (cmsUtl_strcmp(object, mdmPath) != 0)
            {
               fprintf(fs, "                        <option value='%s'>%s</option>\n", mdmPath, mdmPath);
            }
            else
            {
               fprintf(fs, "                        <option value='%s' selected>%s</option>\n", mdmPath, mdmPath);
            }
            omciIpc_free(mdmPath);
         }
      }
      omciIpc_free(objectInfoListP);
   }

   fprintf(fs, "                     </select>\n");
   fprintf(fs, "                  </td>\n");
   fprintf(fs, "               </tr>\n");
   fprintf(fs, "            </table>\n");
}

static void writeOmciCfgParameters(FILE *fs)
{
   SINT32 numInfoEntries = 0, i = 0;
   char paramVal[BUFLEN_1024 + 1];
   OmciParamInfo_t *paramInfoList;
   OmciObjectInfo_t pathDesc;

   if (cmsUtl_strcmp(object, "") != 0 &&
       omciIpc_fullPathToPathDescriptor(object, &pathDesc) == 0)
   {
      if (omciIpc_getParamInfo(pathDesc.oid, &paramInfoList, &numInfoEntries) == 0)
      {
         fprintf(fs, "            <br><br>\n");
         fprintf(fs, "            <table border='0' cellpadding='0' cellspacing='0'>\n");
         if (numMsgTx != numMsgRx || cmsUtl_strcmp(action, "meset") == 0)
         {
            for (i = 0; i < numInfoEntries; i++)
            {
               if (paramInfoList[i].paramName[0] != '\0')
               {
                  fprintf(fs, "               <tr>\n");
                  fprintf(fs, "                  <td width='90'>%s:</td>\n", paramInfoList[i].paramName);
                  fprintf(fs, "                  <td><input type='text' name='param%d' size='44' disabled='1'></td>\n", i);
                  fprintf(fs, "               </tr>\n");
               }
            }
         }
         else
         {
            // disable the first parameter (ManagedEntityId) in the object
            fprintf(fs, "               <tr>\n");
            fprintf(fs, "                  <td width='90'>%s:</td>\n", paramInfoList[0].paramName);
            fprintf(fs, "                  <td><input type='text' name='param0' size='44' value='%d' disabled='1'></td>\n",
                     OMCI_PACKET_ME_INST(&packetList[0]));
            fprintf(fs, "               </tr>\n");
 #ifdef OMCI_CFG_DEBUG
   printf("===> writeOmciCfgParameters, oid=%d, name=%s, class=%u, inst=%u\n",
   pathDesc.oid, paramInfoList[1].paramName,
   OMCI_PACKET_ME_CLASS(&packetList[0]), OMCI_PACKET_ME_INST(&packetList[0]));
#endif // OMCI_CFG_DEBUG
            for (i = 1;
                 i < numInfoEntries &&
                 (i-1) < NUM_PACKETS_MAX &&
                 paramInfoList[i].paramName[0] != '\0';
                 i++)
            {
               cgiOmci_getParameterValue(pathDesc.oid, pathDesc.iidStack, i - 1, paramVal, BUFLEN_1024);
               fprintf(fs, "               <tr>\n");
               fprintf(fs, "                  <td width='90'>%s:</td>\n", paramInfoList[i].paramName);
               if (paramInfoList[i].writable == FALSE)
               {
                  fprintf(fs, "                  <td><input type='text' name='param%d' size='44' value='%s' disabled='1'></td>\n", i, paramVal);
               }
               else
               {
                  // disable parameter if it cannot be writable
                  fprintf(fs, "                  <td><input type='text' name='param%d' size='44' value='%s'></td>\n", i, paramVal);
               }
               fprintf(fs, "               </tr>\n");
            }
         }
         omciIpc_free(paramInfoList);
         fprintf(fs, "            </table>\n");
      }
   }
}

static void writeOmciCfgAction(FILE *fs)
{
   fprintf(fs, "            <br><br>\n");
   fprintf(fs, "            <font color='green'><b>2. Select the action in the Action list to perform on the selected object.</font></b><br><br>\n");
   fprintf(fs, "            <table border='0' cellpadding='0' cellspacing='0'>\n");
   fprintf(fs, "               <tr>\n");
   fprintf(fs, "                  <td width='90'>Action:</td>\n");
   fprintf(fs, "                  <td><select name='selAction' size='1'>\n");
   fprintf(fs, "                        <option value=''>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</option>\n");

   if (cmsUtl_strcmp(action, "meget") == 0)
   {
      fprintf(fs, "                         <option value='meget' selected>Get</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='meget'>Get</option>\n");
   }
   if (cmsUtl_strcmp(action, "mecurrent") == 0)
   {
      fprintf(fs, "                         <option value='mecurrent' selected>GetCurrent</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='mecurrent'>GetCurrent</option>\n");
   }
   if (cmsUtl_strcmp(action, "meset") == 0)
   {
      fprintf(fs, "                        <option value='meset' selected>Set</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='meset'>Set</option>\n");
   }
   if (cmsUtl_strcmp(action, "delete") == 0)
   {
      fprintf(fs, "                        <option value='delete' selected>Delete</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='delete'>Delete</option>\n");
   }
   if (cmsUtl_strcmp(action, "mibupload") == 0)
   {
      fprintf(fs, "                        <option value='mibupload' selected>MibUpload</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='mibupload'>MibUpload</option>\n");
   }
   if (cmsUtl_strcmp(action, "getallalarms") == 0)
   {
      fprintf(fs, "                        <option value='getallalarms' selected>GetAllAlarms</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='getallalarms'>GetAllAlarms</option>\n");
   }
   if (cmsUtl_strcmp(action, "synctime") == 0)
   {
      fprintf(fs, "                        <option value='synctime' selected>SyncTime</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='synctime'>SyncTime</option>\n");
   }
   if (cmsUtl_strcmp(action, "reboot") == 0)
   {
      fprintf(fs, "                        <option value='reboot' selected>Reboot</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='reboot'>Reboot</option>\n");
   }
   if (cmsUtl_strcmp(action, "mibreset") == 0)
   {
      fprintf(fs, "                        <option value='mibreset' selected>MIB Reset</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='mibreset'>MIB Reset</option>\n");
   }
   if (cmsUtl_strcmp(action, "activate") == 0)
   {
      fprintf(fs, "                        <option value='activate' selected>Activate Software</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='activate'>Activate Software</option>\n");
   }
   if (cmsUtl_strcmp(action, "commit") == 0)
   {
      fprintf(fs, "                        <option value='commit' selected>Commit Software</option>\n");
   }
   else
   {
      fprintf(fs, "                        <option value='commit'>Commit Software</option>\n");
   }
   fprintf(fs, "                     </select>\n");
   fprintf(fs, "                  </td>\n");
   fprintf(fs, "               </tr>\n");
   fprintf(fs, "            </table>\n");
}

static void writeOmciCfgButton(FILE *fs)
{
   fprintf(fs, "            <br><br>\n");
   fprintf(fs, "            <font color='green'><b>3. Click on Apply to run your command or Reset to clear your settings.</font></b><br><br>\n");
   fprintf(fs, "            <table border='0' cellpadding='0' cellspacing='0'>\n");
   fprintf(fs, "               <tr>\n");
   fprintf(fs, "                     <td width='170'>&nbsp;</td>\n");
   fprintf(fs, "                     <td>\n");
   fprintf(fs, "                        <input type='button' onClick='btnApply()' value='Apply' name='applyBtn'>\n");
   fprintf(fs, "                     </td>\n");
   fprintf(fs, "                     <td>&nbsp;</td>\n");
   fprintf(fs, "                     <td>\n");
   fprintf(fs, "                        <input type='reset' onClick='btnReset()' value='Reset' name='resetBtn'>\n");
   fprintf(fs, "                     </td>\n");
   fprintf(fs, "                     <td>&nbsp;</td>\n");
   fprintf(fs, "               </tr>\n");
   fprintf(fs, "            </table>\n");
}

void cgiOmci_addCrc(omciPacket *packet, CmsEntityId eid)
{
   UINT32 crc32;

   packet->src_eid = eid;
   crc32 = omciUtl_getCrc32(-1, (char *) packet,
       OMCI_PACKET_SIZE(packet) - OMCI_PACKET_MIC_SIZE);
   OMCI_HTONL(OMCI_PACKET_CRC(packet), crc32);
}

#endif   //DMP_X_ITU_ORG_GPON_1
