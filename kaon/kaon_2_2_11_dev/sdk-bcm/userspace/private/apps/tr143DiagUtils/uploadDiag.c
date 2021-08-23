/***********************************************************************
 *
 *  Copyright (c) 2012-2013  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2013:proprietary:standard

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
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>

#include "tr143_defs.h"
#include "cms_msg.h"


static void *conn_Fcn(void *data)
{
   int ret;
   connectionResult_t *connResult = (connectionResult_t *)data;
   
   if (strstr(proto, "http"))
      ret = httpUploadDiag_r(connResult);
   else
      ret = ftpUploadDiag_r(connResult);

   connResult->state = ret;
   return NULL;
}

int main (int argc, char *argv[])
{
   int ret, ch;
   int connections = -1;
   const char cleanUpCmd[1024] = "rm -f " TR143_UPLOAD_RESULT_FILE"*";

   tr143_result_fname=TR143_UPLOAD_RESULT_FILE;

   cmsLog_init(EID_UPLOAD_DIAG);
   cmsLog_setLevel(LOG_LEVEL_ERR);

   if (system(cleanUpCmd) == -1) cmsLog_error("Failed to run cleanUpCmd");

   cmsLog_debug("upload diag start ......");

   if ((ret = cmsMsg_init(EID_UPLOAD_DIAG, &msgHandle)) != CMSRET_SUCCESS)
   {
      cmsLog_error("cmsMsg_init failed, ret=%d", ret);
      cleanup_and_notify(Error_InitConnectionFailed, EID_UPLOAD_DIAG);
      return 0;
   }

   memset(if_name, 0, sizeof(if_name));
   while ((ch = getopt(argc, argv, "i:c:d:u:l:D:")) != -1)
      switch (ch) {
      case 'i':
         strncpy(if_name, optarg, sizeof(if_name)-1);
         break;
      case 'c':
         connections = atoi(optarg);
         break;
      case 'd':
         dscp = atoi(optarg);
         break;
      case 'u':
         url = optarg;
         break;
      case 'l':
         testFileLength = atoi(optarg);
         break;
      case 'D':
         cmsLog_setLevel(atoi(optarg));
         break;
      }

   if (setupconfig() <0 )
   {
      cmsLog_error("setup config error");
      cleanup_and_notify(Error_InitConnectionFailed, EID_UPLOAD_DIAG);
      return 0;
   }

   if (connections > TR143_MAX_CONNECTION)
   {
      cleanup_and_notify(Error_InitConnectionFailed, EID_UPLOAD_DIAG);
   }
   else if (connections == -1)  // disable per-connection result
   {
      if (strstr(proto, "http"))
         ret = httpUploadDiag();
      else
         ret = ftpUploadDiag();

      cleanup_and_notify(ret, EID_UPLOAD_DIAG);
   }
   else
   {
      int i;
      pthread_t connThreads[TR143_MAX_CONNECTION];
      connectionResult_t perConnectionResults[TR143_MAX_CONNECTION]; 
      connectionResult_t diagResult;

      for (i = 0 ; i < connections ; i++)
      {
         // create thread for each connection test
         memset(&perConnectionResults[i], 0, sizeof(connectionResult_t));
         perConnectionResults[i].index = i;
         pthread_create(&connThreads[i], NULL, conn_Fcn, (void *)&perConnectionResults[i]);
      }

      sleep(1);

      // wait for every connection test exit
      for (i = 0 ; i < connections ; i++)
         pthread_join(connThreads[i], NULL);

      // Record per connection result 
      add_connection_result(&perConnectionResults[0], 0);
      memcpy(&diagResult, &perConnectionResults[0], sizeof(connectionResult_t)); 

      for (i = 1 ; i < connections; i++)
      {
         // create perConnectionResult instance
         add_connection_result(&perConnectionResults[i], i);

         // find the earliest ROMTime
         if (compare_timestamp(diagResult.ROMTime, perConnectionResults[i].ROMTime) > 0)
            diagResult.ROMTime = perConnectionResults[i].ROMTime;

         // find the earliest BOMTime and IP layer statistics
         if (compare_timestamp(diagResult.BOMTime, perConnectionResults[i].BOMTime) > 0)
         {
            diagResult.BOMTime = perConnectionResults[i].BOMTime;
            diagResult.TotalBytesReceivedBegin = perConnectionResults[i].TotalBytesReceivedBegin;
            diagResult.TotalBytesSentBegin = perConnectionResults[i].TotalBytesSentBegin;
         }

         // find the latest EOMTime and IP layer statistics
         if (compare_timestamp(diagResult.EOMTime, perConnectionResults[i].EOMTime) < 0)
         {
            diagResult.EOMTime = perConnectionResults[i].EOMTime;
            diagResult.TotalBytesReceivedEnd = perConnectionResults[i].TotalBytesReceivedEnd;
            diagResult.TotalBytesSentEnd = perConnectionResults[i].TotalBytesSentEnd;
         }

         // find the latest TCPOpenRequestTime
         if (compare_timestamp(diagResult.TCPOpenRequestTime, perConnectionResults[i].TCPOpenRequestTime) < 0)
            diagResult.TCPOpenRequestTime = perConnectionResults[i].TCPOpenRequestTime;

         // find the latest TCPOpenResponseTime
         if (compare_timestamp(diagResult.TCPOpenResponseTime, perConnectionResults[i].TCPOpenResponseTime) < 0)
            diagResult.TCPOpenResponseTime = perConnectionResults[i].TCPOpenResponseTime;

         diagResult.TestBytes += perConnectionResults[i].TestBytes;
      }

      // fill up UploadDiagnostics
      cmsTms_getXSIDateTimeMicroseconds(diagResult.TCPOpenRequestTime.tv_sec,
                                        diagResult.TCPOpenRequestTime.tv_usec,
                                        diagstats.TCPOpenRequestTime,
                                        sizeof(diagstats.TCPOpenRequestTime));
      cmsTms_getXSIDateTimeMicroseconds(diagResult.TCPOpenResponseTime.tv_sec,
                                        diagResult.TCPOpenResponseTime.tv_usec,
                                        diagstats.TCPOpenResponseTime,
                                        sizeof(diagstats.TCPOpenResponseTime));
      cmsTms_getXSIDateTimeMicroseconds(diagResult.ROMTime.tv_sec,
                                        diagResult.ROMTime.tv_usec,
                                        diagstats.ROMTime,
                                        sizeof(diagstats.ROMTime));
      cmsTms_getXSIDateTimeMicroseconds(diagResult.BOMTime.tv_sec,
                                        diagResult.BOMTime.tv_usec,
                                        diagstats.BOMTime,
                                        sizeof(diagstats.BOMTime));
      cmsTms_getXSIDateTimeMicroseconds(diagResult.EOMTime.tv_sec,
                                        diagResult.EOMTime.tv_usec,
                                        diagstats.EOMTime,
                                        sizeof(diagstats.EOMTime));
      diagstats.TestBytes = diagResult.TestBytes;
      diagstats.TotalBytesReceived = diagResult.TotalBytesReceivedEnd - diagResult.TotalBytesReceivedBegin;
      diagstats.TotalBytesSent = diagResult.TotalBytesSentEnd - diagResult.TotalBytesSentBegin;

      cleanup_and_notify(Complete, EID_UPLOAD_DIAG);
   }
   return 0;
}



