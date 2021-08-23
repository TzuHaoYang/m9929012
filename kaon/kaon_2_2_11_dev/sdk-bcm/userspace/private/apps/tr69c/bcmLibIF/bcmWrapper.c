/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#define _XOPEN_SOURCE
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h>

#include "inc/tr69cdefs.h" /* defines for ACS state */
#include "inc/appdefs.h"

#include "bcmWrapper.h"
#include "cms_image.h"
#include "cms_qdm.h"
#include "cms_msg.h"
#include "bcmConfig.h"

/*#define DEBUG*/

extern int sysGetSdramSize( void );   /* in cms_utils/xboard_api.c */
extern void clearInformEventList(void);
extern ACSState   acsState;
extern void *msgHandle;
extern InformEvList informEvList;
extern UINT32 addInformEventToList(UINT8 event);


#ifdef SUPPORT_TR69C_AUTONOMOUS_TRANSFER_COMPLETE
extern UBOOL8 doSendAutonTransferComplete;
#endif

struct informEvtTableEntry informEvtStringTable[] = {
   {INFORM_EVENT_BOOTSTRAP,               "0 BOOTSTRAP"},
   {INFORM_EVENT_BOOT,                    "1 BOOT" },
   {INFORM_EVENT_PERIODIC,                "2 PERIODIC"},
   {INFORM_EVENT_SCHEDULED,               "3 SCHEDULED"},
   {INFORM_EVENT_VALUE_CHANGE,            "4 VALUE CHANGE"},
   {INFORM_EVENT_KICKED,                  "5 KICKED"},
   {INFORM_EVENT_CONNECTION_REQUEST,      "6 CONNECTION REQUEST"},
   {INFORM_EVENT_TRANSER_COMPLETE,        "7 TRANSFER COMPLETE"},
   {INFORM_EVENT_DIAGNOSTICS_COMPLETE,    "8 DIAGNOSTICS COMPLETE"},
   {INFORM_EVENT_REBOOT_METHOD,           "M Reboot"},
   {INFORM_EVENT_SCHEDULE_METHOD,         "M ScheduleInform"},
   {INFORM_EVENT_DOWNLOAD_METHOD,         "M Download"},
   {INFORM_EVENT_UPLOAD_METHOD,           "M Upload"},
   {INFORM_EVENT_REQUEST_DOWNLOAD,        "9 REQUEST DOWNLOAD"},
   {INFORM_EVENT_AUTON_TRANSFER_COMPLETE, "10 AUTONOMOUS TRANSFER COMPLETE"},
   {INFORM_EVENT_DU_CHANGE_COMPLETE, "11 DU STATE CHANGE COMPLETE"},
   {INFORM_EVENT_AUTON_DU_CHANGE_COMPLETE, "12 AUTONOMOUS DU STATE CHANGE COMPLETE"},
   {INFORM_EVENT_SCHEDULE_DOWNLOAD_METHOD, "M ScheduleDownload"},
   {INFORM_EVENT_CHANGE_DU_CHANGE_METHOD,  "M ChangeDUState"}
};

#define NUM_INFORM_EVT_STRING_TABLE_ENTRIES (sizeof(informEvtStringTable)/sizeof(struct informEvtTableEntry))

const char *getInformEvtString(UINT32 evt)
{
   UINT32 i;

   for (i=0; i < NUM_INFORM_EVT_STRING_TABLE_ENTRIES; i++)
   {
      if (informEvtStringTable[i].informEvt == evt)
      {
         return informEvtStringTable[i].str;
      }
   }

   cmsLog_error("Unsupported Inform Event value %d", evt);
   return "Internal Error (getInformEvtString)";
}

void setInformState(eInformState state)
{
   cmsLog_debug("set informState=%d", state);

   if (informState != state)
   {
      informState = state;
      saveTR69StatusItems();
   }
}  /* End of setInformState() */

void wrapperReboot(eInformState rebootContactValue)
{
   addInformEventToList(INFORM_EVENT_REBOOT_METHOD);

   setInformState(rebootContactValue);    /* reset on BcmCfm_download error */
   saveTR69StatusItems();

   cmsLog_notice("CPE is REBOOTING with rebootContactValue =%d", rebootContactValue);

   tr69SaveTransferList();
   wrapperReset();
}



/* Returns state of WAN interface to be used by tr69 client */
eWanState getWanState(void)
{
   /* add BCM shared library call here to determine status of outbound I/F*/
   /* mwang: even in the released 3.0.8 code, this function just returns
    * eWAN_ACTIVE. So in most places in the code, we will continue to use this
    * until we understand the implication of fixing it or removing it.
    * In the mean time, there is also a getRealWanState function below. */
   return eWAN_ACTIVE;
}

UBOOL8 matchAddrOnLanSide(const char *urlAddrString)
{
   UBOOL8 match=FALSE;
   char *ipAddrStr=NULL, *ipMaskStr=NULL;
   CmsRet ret;

   ret = getLanIPAddressInfo(&ipAddrStr,&ipMaskStr);
   
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_debug("one or more LAN_IP_CFG parameters are null");
   }
   else
   {
      struct in_addr urlAddr, ipAddr, mask;

      inet_aton(urlAddrString, &urlAddr);
      inet_aton(ipAddrStr, &ipAddr);
      inet_aton(ipMaskStr, &mask);

      if ((urlAddr.s_addr & mask.s_addr) == (ipAddr.s_addr & mask.s_addr))
      {
         cmsLog_debug("urlAddr %s is on LAN side", urlAddrString);
         match = TRUE;
      }
      else
      {
         cmsLog_debug("urlAddr %s is not on LAN side (%s/%s)",
                      urlAddrString,
                      ipAddrStr,
                      ipMaskStr);
      }
      cmsMem_free(ipAddrStr);
      cmsMem_free(ipMaskStr);
   }
   return match;
}


