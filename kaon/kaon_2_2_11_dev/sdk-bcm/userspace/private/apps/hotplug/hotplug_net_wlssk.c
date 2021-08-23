/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom
 *  All Rights Reserved
 *
<:label-BRCM:2018:proprietary:standard

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

#include "hotplug.h"
#include "prctl.h"

extern void *g_msgHandle;
void notify_wlssk(char *ifname, char *action)
{
   char buf[sizeof(CmsMsgHeader) + 32]={0}, *cmd;
   /* send a changed notification to wlmngr */
   CmsMsgHeader *msg=(CmsMsgHeader *) buf;
   void *msgHandle = g_msgHandle ;
   int maxRetries = 5, ret;
   int pid = 0;

   if(msgHandle == NULL)
   {
      do
      {
         if ((ret = cmsMsg_initWithFlags(EID_HOTPLUG, EIF_MULTIPLE_INSTANCES, &g_msgHandle)) == CMSRET_SUCCESS)
         {
            msgHandle = g_msgHandle;
            break; 
         }
         else
         {
            sleep(30);
            g_msgHandle = NULL;
         }

      } while(maxRetries--);

      if(ret != CMSRET_SUCCESS)
      {
          cprintf("cmsMsg_init error ret=%d", ret);
          return;
      }
   }
    
   msg->type = CMS_MSG_WLAN_CHANGED;
   msg->src = MAKE_SPECIFIC_EID(getpid(), EID_HOTPLUG);
   msg->dst = EID_WLSSK;
   msg->flags_event = 1;


   cmd = (char *) (msg + 1);
   msg->dataLength = 32;
   snprintf(cmd, 32, "Hotplug %s %s", ifname, action);

   /*
    * We hope the message will be send out after wlssk is launched by SMD in stage2.
    * That is to prevent wlssk from launching by SMD due to message triggering
    * while MDM is initializing in system booting.
    */
   if ((pid = prctl_getPidByName("wlssk")) == 0)
   {
       cmsLog_debug("Process wlssk is not in the system.");
   }else{
       ret = cmsMsg_send(msgHandle, msg);
   }

   cmsMsg_cleanup(&msgHandle);
   g_msgHandle = NULL;
}

