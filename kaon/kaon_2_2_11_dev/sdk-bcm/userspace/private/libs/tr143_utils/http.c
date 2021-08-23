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
#include <sys/time.h>

#include "tr143_defs.h"
#include "tr143_private.h"

#ifdef BRCM_SPDTEST
#include <spdt_api.h>

static int _httpDownloadDiag (connectionResult_t *connResult)
{
   spdt_conn_params_t params = {};
   int8_t stream_idx = -1;
   int rc, errCode = Complete;
   spdt_rx_params_t rx_params = {};

   if (tcpspeedtest_setup(SPDT_HTTP))
   {
      cmsLog_error("TCPSPEEDTEST init error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }
   stream_idx = g_stream_idx;

   if (server_port == 0) server_port = 80;

   memcpy(&params.server_addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&params.server_addr)->sin_port = htons(server_port);
   params.tos = dscp << 2;
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   rc = spdt_connect(stream_idx, SPDT_DIR_RX, &params);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (rc) 
   {
      cmsLog_error("open socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }
   cmsLog_notice("====>connected to server, stream: %d", stream_idx);

   rx_params.proto.tcp.size = 0;
   rx_params.proto.tcp.file_name = uri[0] != '/' ? uri : uri + 1;
   if ((rc = spdt_recv_start(stream_idx, &rx_params)))
   {
      cmsLog_error("download error");
      errCode = Error_TransferFailed;
      goto errOut2;
   }

   connResult->TotalBytesReceivedBegin = connResult->TotalBytesSentBegin = 0;

   while (1)
   {
      spdt_stat_t spd_stat = {};
      tcp_spdt_rep_t *spd_report;

      if ((rc = spdt_stats_get(stream_idx, &spd_stat)))
      {
         cmsLog_error("HTTP speed report error");
         errCode = Error_TransferFailed;
         goto errOut2;
      }
      spd_report = &(spd_stat.proto_ext.tcp_speed_rep);

      if (TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status || TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
      {
         connResult->TestBytes = spd_report->num_bytes;
         connResult->TotalBytesReceivedEnd = spd_report->num_bytes;
         connResult->TotalBytesSentEnd = 0;
         connResult->ROMTime.tv_sec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_sec;
         connResult->ROMTime.tv_usec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_ROM_TIME].tv_usec;
         connResult->BOMTime.tv_sec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_sec;
         connResult->BOMTime.tv_usec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_usec;
         connResult->EOMTime.tv_sec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_sec;
         connResult->EOMTime.tv_usec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_usec;

         if (TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
             errCode = Error_TransferFailed;

         cmsLog_debug("Download completed %s with %llu bytes at %u ms GoodPut=%d Mbps",
            TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status ? "OK" : "BAD", spd_report->num_bytes,
            spd_report->time_ms, spd_report->rate);

         break;
      }
   }

errOut2:
   spdt_disconnect(stream_idx);
errOut1:
   if (-1 != stream_idx)
       spdt_uninit(stream_idx);
   cmsLog_debug("return:%d", errCode);
   return errCode;	
}

#else

/*
return:
1) actual size,  if content-length is not specified
2) content-length, if content-length is > 0
3) -1, if timeout or socket close
*/
static int http_readLengthMsg(tProtoCtx *pc, int readLth, int doFlushStream) 
{
   int bufCnt = 0, readCnt = 0;
   int bufLth = readLth;
   char buf[16384];

   set_sockopt_nocopy(pc->fd);

   cmsLog_notice("Payload read started");

   while (readLth <= 0 || bufCnt < readLth)
   {
      if ((readCnt = proto_Readn(pc, buf, (bufLth > sizeof(buf) || readLth <= 0 ) ? sizeof(buf) : bufLth)) > 0)
      {
          //cmsLog_debug("readCnt: %d\n",readCnt);
          bufCnt += readCnt;
          bufLth -= readCnt;
      }
      else
      {
         cmsLog_error("proto_Readn timeout");
         break;
      }
   }

   cmsLog_notice("Payload read ended");
   cmsLog_debug("buf readLth=%d, actual read=%d ", readLth, bufCnt);
   
   if(readCnt <= 0 && readLth > 0)
   {
      cmsLog_error("http_readLengthMsg error");
      return -1;
   }

   if (doFlushStream) proto_Skip(pc);         

   return bufCnt;
}

static int http_readChunkedMsg(tProtoCtx *pc) 
{
   char chunkedBuf[512];   
   int  chunkedSz = 0, readSz = 0, totSz = 0;

   while (1)
   {
      do
      {
         chunkedBuf[0] = '\0';
         readSz = proto_Readline(pc, chunkedBuf, sizeof(chunkedBuf));
         if (readSz <= 0) 
         {
            cmsLog_error("read chunked size error");
            return -1;
         }
      }
      while (readSz > 0 && isxdigit(chunkedBuf[0]) == 0);

      totSz += readSz;
      sscanf(chunkedBuf, "%x", &chunkedSz);

      if (chunkedSz <= 0) break;
      if ((readSz = http_readLengthMsg(pc, chunkedSz, FALSE)) < 0)
      {
         cmsLog_error("http_readLengthMsg error, chunked size = %d, readSz = %d", chunkedSz, readSz);
         return -1;
      }

      totSz += readSz;
   }      

   proto_Skip(pc);         
   return totSz;
}

static int send_http_get(tProtoCtx *pc)
{
   proto_SendRequest(pc, "GET", uri);
   proto_SendHeader(pc,  "Host", server_name);
   proto_SendHeader(pc,  "User-Agent", TR143_AGENT_NAME);
   proto_SendHeader(pc,  "Connection", "keep-alive");
   proto_SendRaw(pc, "\r\n", 2);

   return 0;
}  /* End of send_get_request() */


static int http_GetData(tProtoCtx *pc, connectionResult_t *connResult)
{
   tHttpHdrs *hdrs;
   int errCode = Complete;

   if ((hdrs = proto_NewHttpHdrs()) == NULL)
   {
      cmsLog_error("http hdr alloc error");
      return Error_TransferFailed;
   }

   if ((connResult->TestBytes = proto_ParseResponse(pc, hdrs)) < 0) 
   {
      cmsLog_error("error: illegal http response or read failure");
      errCode = Error_TransferFailed;
      goto errOut1;
   }

   connResult->TestBytes += proto_ParseHdrs(pc, hdrs);

   if (hdrs->status_code != 200 ) 
   {
      cmsLog_error("http reponse %d", hdrs->status_code);
      if (hdrs->status_code == 401)
         errCode = Error_LoginFailed;
      else
         errCode = Error_TransferFailed;
      goto errOut1;
   }

   if (hdrs->TransferEncoding && !strcasecmp(hdrs->TransferEncoding,"chunked"))
   {
      if ((connResult->TestBytes += http_readChunkedMsg(pc)) < 0)
      {
         cmsLog_error("calling http_readChunkedMsg error");
         errCode = Error_TransferFailed;
      }
   }
   else if (hdrs->content_length > 0)
   {
      if ((connResult->TestBytes += http_readLengthMsg(pc, hdrs->content_length, FALSE)) < 0)
      {
         cmsLog_error("calling readLengthMsg error, content_length=%d", hdrs->content_length);
         errCode = Error_TransferFailed;
      }
   }	

errOut1:
   proto_FreeHttpHdrs(hdrs);
   return errCode;
}

static int _httpDownloadDiag (connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   int errCode = Complete;
   int sfd = -1;
   tProtoCtx *pc = NULL;

   if (server_port == 0) server_port = 80;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);

   if (sfd <= 0) 
   {
      cmsLog_error("open socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }
   cmsLog_notice("====>connected to server");

   if ((pc = proto_NewCtx(sfd)) == NULL)
   {
      cmsLog_error("proto_NewCtx error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }

   gettimeofday(&connResult->ROMTime, NULL);

   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin);
   if (send_http_get(pc) < 0)
   {
      cmsLog_error("send get request error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }
   cmsLog_notice("====>http get sent");

   if (select_with_timeout(sfd, 1))
   { 
      cmsLog_error("http get reponse error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   gettimeofday(&connResult->BOMTime, NULL);

   errCode = http_GetData(pc, connResult);

   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd);
   gettimeofday(&connResult->EOMTime, NULL);

errOut2:
   proto_FreeCtx(pc);
errOut1:
   close(sfd);
   cmsLog_debug("return:%d", errCode);
   return errCode;	
}

#endif /* BRCM_SPDTEST */

static int send_http_put(tProtoCtx *pc, connectionResult_t *connResult)
{
   char buf[16384];
   char connUri[BUFLEN_128] = {0};
   int sndCnt;

   // Note: when test with multiple thread, the same uri would cause upload 
   // rejected by server. In such cases, we add postfix with uri
   if (connResult->index != -1)
      snprintf(connUri, 128, "%s_%d", uri, connResult->index);
   else
      strncpy(connUri, uri ,128);

   proto_SendRequest(pc, "PUT", connUri);
   proto_SendHeader(pc,  "Host", server_name);
   proto_SendHeader(pc,  "User-Agent", TR143_AGENT_NAME);
   proto_SendHeader(pc,  "Connection", "keep-alive");
   proto_SendHeader(pc,  "Content-Type", "text/xml");
   sprintf(buf, "%d", testFileLength);
   proto_SendHeader(pc,  "Content-Length", buf);
   proto_SendRaw(pc, "\r\n", 2);
   memset(buf, 'a', sizeof(buf));

   set_sockopt_nocopy(pc->fd);

   cmsLog_notice("start send");
   
   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin);
   gettimeofday(&connResult->BOMTime, NULL);

   for (sndCnt = testFileLength; sndCnt > 0; sndCnt -= sizeof(buf))
       proto_SendRaw(pc, buf, sndCnt > sizeof(buf) ? sizeof(buf) : sndCnt);

   cmsLog_notice("end send");
   return 0;
}  /* End of send_get_request() */


static int _httpUploadDiag (connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   int errCode = Complete;
   tProtoCtx *pc = NULL;
   tHttpHdrs *hdrs;
   int sfd;

   if (server_port == 0) server_port = 80;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0) 
   {
      cmsLog_error("open socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }
   cmsLog_notice("====>connected to server");

   if ((pc = proto_NewCtx(sfd)) == NULL)
   {
      cmsLog_error("proto_NewCtx error");
      errCode = Error_InitConnectionFailed;
      goto errOut1;
   }

   gettimeofday(&connResult->ROMTime, NULL);
   if (send_http_put(pc, connResult) < 0)
   {
      cmsLog_error("send get request error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }
   cmsLog_notice("====>http put sent");

   if (select_with_timeout(sfd, 1))
   {
      cmsLog_error("http get reponse error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   gettimeofday(&connResult->EOMTime, NULL);
   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd);

   if ((hdrs = proto_NewHttpHdrs()) == NULL)
   {
      cmsLog_error("http hdr alloc error");
      errCode = Error_TransferFailed;
      goto errOut2;
   }

   if (proto_ParseResponse(pc, hdrs) < 0) 
   {
      cmsLog_error("error: illegal http response or read failure");
      errCode = Error_TransferFailed;
      goto errOut3;
   }

   proto_ParseHdrs(pc, hdrs);

   if (hdrs->status_code != 100 &&  // Continue status might be returned by Microsoft-IIS/5.1
         hdrs->status_code != 201 &&   // Created status is returned by Microsoft-IIS/5.1
         hdrs->status_code != 204 &&   // No content status is returned by Apache/2.2.2
         hdrs->status_code != 200 ) 
   {
      cmsLog_error("http reponse %d", hdrs->status_code);
      if (hdrs->status_code == 401)
         errCode = Error_LoginFailed;
      else
         errCode = Error_TransferFailed;
   }	
   else 
   {
      errCode = Complete;
   }

errOut3:	
   proto_FreeHttpHdrs(hdrs);
errOut2:
   proto_FreeCtx(pc);
errOut1:
   close(sfd);
   cmsLog_debug("return:%d", errCode);
   return errCode;	
}


int httpDownloadDiag(void)
{
   int ret;
   connectionResult_t diagResult;
   diagResult.index = -1;
   ret = _httpDownloadDiag(&diagResult);

   // fill up DownloadDiagnostics
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

   if (diagResult.TotalBytesReceivedEnd >= diagResult.TotalBytesReceivedBegin) 
      diagstats.TotalBytesReceived = diagResult.TotalBytesReceivedEnd - diagResult.TotalBytesReceivedBegin;
   if (diagResult.TotalBytesSentEnd >= diagResult.TotalBytesSentBegin) 
      diagstats.TotalBytesSent = diagResult.TotalBytesSentEnd - diagResult.TotalBytesSentBegin;

   return ret;
}

int httpUploadDiag(void)
{
   int ret;
   connectionResult_t diagResult;
   diagResult.index = -1;
   ret = _httpUploadDiag(&diagResult);

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
   if (diagResult.TotalBytesReceivedEnd >= diagResult.TotalBytesReceivedBegin) 
      diagstats.TotalBytesReceived = diagResult.TotalBytesReceivedEnd - diagResult.TotalBytesReceivedBegin;
   if (diagResult.TotalBytesSentEnd >= diagResult.TotalBytesSentBegin) 
      diagstats.TotalBytesSent = diagResult.TotalBytesSentEnd - diagResult.TotalBytesSentBegin;

   return ret;
}

int httpDownloadDiag_r(connectionResult_t *connResult)
{
   return _httpDownloadDiag(connResult);
}

int httpUploadDiag_r(connectionResult_t *connResult)
{
   return _httpUploadDiag(connResult);
}
