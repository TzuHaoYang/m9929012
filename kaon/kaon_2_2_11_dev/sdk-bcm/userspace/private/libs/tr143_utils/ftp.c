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
#include "spdt_api.h"
#endif

static int ftpcmd(char *s1, char *s2, int sfd, FILE *fp, char *buf, int bufLen)
{
   fd_set fdset;
   struct timeval tm;
   int ret;

   cmsLog_debug("ftpcmd:buflen=%d", bufLen);

   if (s1) 
   {
      if (!s2) s2="";
      fprintf(fp, "%s%s\r\n", s1, s2);
      fflush(fp);
   }

   FD_ZERO(&fdset);
   FD_SET(sfd, &fdset);
   tm.tv_sec = TR143_SESSION_TIMEOUT;
   tm.tv_usec = 0;

   ret = select(sfd + 1, &fdset, NULL, NULL, &tm);
   if ( ret <= 0 || !FD_ISSET(sfd, &fdset)) 
   {
       cmsLog_error("select time out");
       return -1;
   }

   do 	
   {
      if (fgets(buf, bufLen - 2, fp) == NULL) 
      {
         cmsLog_error("fgets error");
         return -1;
      }

      cmsLog_debug("fgets:%s", buf);
   } while (!isdigit(buf[0]) || buf[3] != ' ');

   return atoi(buf);
}

#ifdef BRCM_SPDTEST
int _ftpUploadDiag(connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   spdt_conn_params_t params = {};
   FILE *sfp;
   int sfd;
   char *s, buf[16384];
   char *filename;
   char connUri[BUFLEN_128] = {};
   char connFilename[BUFLEN_128] = {};
   int errCode = Complete;
   int8_t stream_idx = -1;
   uint16_t port;
   unsigned int fileLength = testFileLength;
   spdt_tx_params_t tx_params = {};

   if (tcpspeedtest_setup(SPDT_FTP))
   {
      cmsLog_error("TCPSPEEDTEST init error");
      return Error_InitConnectionFailed;
   }

   stream_idx = g_stream_idx;

   // make a copy for re-entries
   strncpy(connUri, uri, 128); 

   if (server_port == 0) server_port = 21;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0 || ((sfp = fdopen(sfd, "r+")) == NULL)) 
   {
      cmsLog_error("open control socket error");
      return Error_InitConnectionFailed;
   }

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 220)
   {
      cmsLog_error("server ready error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   switch(ftpcmd("USER ", "anonymous", sfd, sfp, buf, sizeof(buf))) {
   case 230:
      break;
   case 331:
      if (ftpcmd("PASS ", "anonymous", sfd, sfp, buf, sizeof(buf)) == 230) break;
      cmsLog_error("auth-pass error");
      errCode = Error_LoginFailed;
      goto errOut2;
   default:
      cmsLog_error("auth-user error");
      errCode = Error_PasswordRequestFailed;
      goto errOut2;
   }

   if (ftpcmd("TYPE I", NULL, sfd, sfp, buf, sizeof(buf)) != 200)
   {
      cmsLog_error("transfer mode error");
      errCode = Error_NoTransferMode;
      goto errOut2;
   }


   if (ftpcmd("PASV", NULL, sfd, sfp, buf, sizeof(buf)) !=  227)
   {
      cmsLog_error("PASV error");
      errCode = Error_NoPASV;
      goto errOut2;
   }
   s = strrchr(buf, ',');
   *s = 0;
   port = atoi(s+1);
   s = strrchr(buf, ',');
   port += atoi(s+1) * 256;
   memcpy(&params.server_addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&params.server_addr)->sin_port = htons(port);
   params.tos = dscp << 2;
 
   if (spdt_connect(stream_idx, SPDT_DIR_TX, &params))
   {
      cmsLog_error("open data socket error stream_idx=%d", stream_idx);
      errCode = Error_NoResponse;
      goto errOut2;
   }

   if ((filename = strrchr(connUri, '/')) != NULL)
   {
      *filename = '\0';
      filename++;
      if (connUri[0] != '\0' && ftpcmd("CWD ", connUri, sfd, sfp, buf, sizeof(buf)) != 250) 
      {
         cmsLog_error("CWD to %s error", connUri);
         errCode = Error_NoCWD;
         goto errOut1;
      }
   }
   else
      filename = connUri;

   // Note: when test with multiple thread, the same filename would cause upload 
   // rejected or blocked by server. In such cases, we add postfix with filename
   if (connResult->index != -1)
      snprintf(connFilename, 128, "%s_%d", filename, connResult->index);
   else
      strncpy(connFilename, filename ,128);

   connResult->TotalBytesReceivedBegin = connResult->TotalBytesSentBegin = 0;
   gettimeofday(&connResult->ROMTime, NULL);
   if (ftpcmd("STOR ", connFilename, sfd, sfp, buf, sizeof(buf)) > 150)
   {
      cmsLog_error("STOR error");
      errCode = Error_NoSTOR;
      goto errOut1;
   }

   cmsLog_notice("start transmision");

   tx_params.proto.tcp.size = fileLength;
   if (spdt_send_start(stream_idx, &tx_params))
   {
      cmsLog_error("upload error");
      errCode = Error_TransferFailed;
      goto errOut1;
   }   
   
/*   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin); */
 
   gettimeofday(&connResult->BOMTime, NULL);
   
   while (1) 
   {
      spdt_stat_t spd_stat = {};
      tcp_spdt_rep_t *spd_report;

      if (spdt_stats_get(stream_idx, &spd_stat))
      {
         cmsLog_error("FTP Speed report error");
         errCode = Error_TransferFailed;
         goto errOut1;
      }
      spd_report = &(spd_stat.proto_ext.tcp_speed_rep);

      if (TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status || TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
      {
         connResult->TestBytes = spd_report->num_bytes;
         connResult->TotalBytesSentEnd = spd_report->num_bytes;
         connResult->TotalBytesReceivedEnd = 0;
         connResult->BOMTime.tv_sec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_sec;
         connResult->BOMTime.tv_usec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_BOM_TIME].tv_usec;
         connResult->EOMTime.tv_sec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_sec;
         connResult->EOMTime.tv_usec = spd_report->tr143_ts[SPDT_TR143_TS_REPORT_EOM_TIME].tv_usec;

         if (TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
             errCode = Error_TransferFailed;

         cmsLog_debug("Upload completed %s with %llu bytes at %u ms GoodPut=%d Mbps",
            TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status ? "OK" : "BAD", spd_report->num_bytes,
            spd_report->time_ms, spd_report->rate);

         break;
      }
   }

/*   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd); */

   cmsLog_notice("end transmision\n");

   spdt_disconnect(stream_idx);
   sleep(5);
   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 226)
   {
      cmsLog_error("read file send ok error");
      errCode = Error_TransferFailed;
   }

   ftpcmd("QUIT", NULL, sfd, sfp, buf, sizeof(buf));

   goto errOut2;

errOut1:
   spdt_disconnect(stream_idx);
errOut2:
   if (-1 != stream_idx)
       spdt_uninit(stream_idx);
   close(sfd);
   fclose(sfp);
   cmsLog_debug("return errCode:%d", errCode);
   return errCode;	
}

int _ftpDownloadDiag(connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   spdt_conn_params_t params = {};
   FILE *sfp;
   int sfd;
   char *s, *filename, buf[16384];
   unsigned long filesize;
   int errCode = Complete;
   int8_t stream_idx = -1;
   uint16_t port;
   spdt_rx_params_t rx_params = {};

   if (tcpspeedtest_setup(SPDT_FTP))
   {
      cmsLog_error("TCPSPEEDTEST init error");
      return Error_InitConnectionFailed;
   }

   stream_idx = g_stream_idx;

   if (server_port == 0) server_port = 21;
   filename = (uri[0] == '/') ? &uri[1] : uri;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0 || ((sfp = fdopen(sfd, "r+")) == NULL)) 
   {
      cmsLog_error("open control socket error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 220)
   {
      cmsLog_error("server ready error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   switch(ftpcmd("USER ", "anonymous", sfd, sfp, buf, sizeof(buf))) {
   case 230:
      break;
   case 331:
      if (ftpcmd("PASS ", "anonymous", sfd, sfp, buf, sizeof(buf)) == 230) break;
      cmsLog_error("auth-pass error");
      errCode = Error_LoginFailed;
      goto errOut2;
   default:
      cmsLog_error("auth-user error");
      errCode = Error_PasswordRequestFailed;
      goto errOut2;
   }

   if (ftpcmd("TYPE I", NULL, sfd, sfp, buf, sizeof(buf)) != 200)
   {
      cmsLog_error("transfer mode error");
      errCode = Error_NoTransferMode;
      goto errOut2;
   }

   if ((ftpcmd("SIZE ", filename, sfd, sfp, buf, sizeof(buf)) != 213) || (safe_strtoul(buf + 4, &filesize)))
   {
      cmsLog_error("SIZE error");
      errCode = Error_IncorrectSize;
      goto errOut2;
   }

   if (ftpcmd("PASV", NULL, sfd, sfp, buf, sizeof(buf)) !=  227)
   {
      cmsLog_error("PASV error");
      errCode = Error_NoPASV;
      goto errOut2;
   }
   s = strrchr(buf, ',');
   *s = 0;
   port = (uint16_t)atoi(s + 1);
   s = strrchr(buf, ',');
   port += atoi(s + 1) * 256;
   memcpy(&params.server_addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&params.server_addr)->sin_port = htons(port);
   params.tos = dscp << 2;
 
   if (spdt_connect(stream_idx, SPDT_DIR_RX, &params))
   {
      cmsLog_error("open data socket error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   connResult->TotalBytesReceivedBegin = connResult->TotalBytesSentBegin = 0;
   gettimeofday(&connResult->ROMTime, NULL);

   rx_params.proto.tcp.size = filesize;
   rx_params.proto.tcp.file_name = "";
   if (spdt_recv_start(stream_idx, &rx_params))
   {
      cmsLog_error("download error");
      errCode = Error_TransferFailed;
      goto errOut1;
   }

   if (ftpcmd("RETR ", filename, sfd, sfp, buf, sizeof(buf)) > 150)
   {
      cmsLog_error("RETR error");
      errCode = Error_Timeout;
      goto errOut1;
   }

   cmsLog_notice("RETR sent");

   cmsLog_notice("start data transfer");

   while (1)
   {
      spdt_stat_t spd_stat = {};
      tcp_spdt_rep_t *spd_report;

      if (spdt_stats_get(stream_idx, &spd_stat))
      {
         cmsLog_error("FTP Speed report error");
         errCode = Error_TransferFailed;
         goto errOut1;

      }
      spd_report = &(spd_stat.proto_ext.tcp_speed_rep);

      if (TCPSPDTEST_GENL_CMD_STATUS_OK == spd_report->status || TCPSPDTEST_GENL_CMD_STATUS_ERR == spd_report->status)
      {
         connResult->TestBytes = spd_report->num_bytes;
         connResult->TotalBytesReceivedEnd = spd_report->num_bytes;
         connResult->TotalBytesSentEnd = 0;
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

   cmsLog_notice("end data transfer");

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 226)
   {
      cmsLog_error("read file send ok error");
   }

   ftpcmd("QUIT", NULL, sfd, sfp, buf, sizeof(buf));

errOut1:
   spdt_disconnect(stream_idx);
errOut2:
   if (-1 != stream_idx)
       spdt_uninit(stream_idx);
   close(sfd);
   fclose(sfp);
   cmsLog_debug("return errCode:%d", errCode);
   return errCode;	
}

#else

int _ftpUploadDiag(connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   FILE *sfp;
   int sfd, dfd;
   char *s, buf[16384];
   char *filename;
   char connUri[BUFLEN_128] = {0};
   char connFilename[BUFLEN_128] = {0};
   int n, errCode = Complete;
   int port;
   unsigned int fileLength = testFileLength;

   // make a copy for re-entries
   strncpy(connUri, uri, 128); 

   if (server_port == 0) server_port = 21;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&addr);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0 || ((sfp = fdopen(sfd, "r+")) == NULL)) 
   {
      cmsLog_error("open control socket error");
      return Error_InitConnectionFailed;
   }

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 220)
   {
      cmsLog_error("server ready error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   switch(ftpcmd("USER ", "anonymous", sfd, sfp, buf, sizeof(buf))) {
   case 230:
      break;
   case 331:
      if (ftpcmd("PASS ", "anonymous", sfd, sfp, buf, sizeof(buf)) == 230) break;
      cmsLog_error("auth-pass error");
      errCode = Error_LoginFailed;
      goto errOut2;
   default:
      cmsLog_error("auth-user error");
      errCode = Error_PasswordRequestFailed;
      goto errOut2;
   }

   if (ftpcmd("TYPE I", NULL, sfd, sfp, buf, sizeof(buf)) != 200)
   {
      cmsLog_error("transfer mode error");
      errCode = Error_NoTransferMode;
      goto errOut2;
   }


   if (ftpcmd("PASV", NULL, sfd, sfp, buf, sizeof(buf)) !=  227)
   {
      cmsLog_error("PASV error");
      errCode = Error_NoPASV;
      goto errOut2;
   }
   s = strrchr(buf, ',');
   *s = 0;
   port = atoi(s+1);
   s = strrchr(buf, ',');
   port += atoi(s+1) * 256;
   ((struct sockaddr_in *)&addr)->sin_port = htons(port);

   dfd = open_conn_socket(&addr);
   if (dfd <= 0) 
   {
      cmsLog_error("open data socket error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   if ((filename = strrchr(connUri, '/')) != NULL)
   {
      *filename = '\0';
      filename++;
      if (connUri[0] != '\0' && ftpcmd("CWD ", connUri, sfd, sfp, buf, sizeof(buf)) != 250) 
      {
         cmsLog_error("CWD to %s error", connUri);
         errCode = Error_NoCWD;
         goto errOut1;
      }
   }
   else
      filename = connUri;


   // Note: when test with multiple thread, the same filename would cause upload 
   // rejected or blocked by server. In such cases, we add postfix with filename
   if (connResult->index != -1)
      snprintf(connFilename, 128, "%s_%d", filename, connResult->index);
   else
      strncpy(connFilename, filename ,128);

   gettimeofday(&connResult->ROMTime, NULL);
   if (ftpcmd("STOR ", connFilename, sfd, sfp, buf, sizeof(buf)) > 150)
   {
      cmsLog_error("STOR error");
      errCode = Error_NoSTOR;
      goto errOut1;
   }

   memset(buf, 'a', sizeof(buf));

   cmsLog_notice("start transmision");
   
   set_sockopt_nocopy(dfd);
   
   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin);
 
   gettimeofday(&connResult->BOMTime, NULL);
   
   while (fileLength > 0) 
   {
      //cmsLog_debug("store file size:%d\n", fileLength);
      n =write(dfd, buf, (fileLength > sizeof(buf))?sizeof(buf):fileLength);
      if (n<=0) 
      {
         cmsLog_error("file write error, %d", n);
         errCode = Error_TransferFailed;
         goto errOut1;
      }
      fileLength -= n;
      connResult->TestBytes += n;
   }		

   gettimeofday(&connResult->EOMTime, NULL);
   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd);

   cmsLog_notice("end transmision\n");
   close(dfd);

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 226)
   {
      cmsLog_error("read file send ok error");
      errCode = Error_TransferFailed;
   }

   ftpcmd("QUIT", NULL, sfd, sfp, buf, sizeof(buf));
   goto errOut2;

errOut1:
   close(dfd);
errOut2:
   close(sfd);
   fclose(sfp);
   cmsLog_debug("return errCode:%d", errCode);
   return errCode;	
}

int _ftpDownloadDiag(connectionResult_t *connResult)
{
   struct sockaddr_storage addr = {};
   FILE *sfp;
   int sfd, dfd;
   char *s, *filename, buf[16384];
   unsigned long filesize;
   int n, errCode = Complete;
   int port;
   int firstRead = 1;

   if (server_port == 0) server_port = 21;
   filename =  (uri[0] == '/') ? &uri[1] : uri;

   memcpy(&addr, &server_addr, sizeof(server_addr));
   ((struct sockaddr_in *)&addr)->sin_port = htons(server_port);
   gettimeofday(&connResult->TCPOpenRequestTime, NULL);
   sfd = open_conn_socket(&s);
   gettimeofday(&connResult->TCPOpenResponseTime, NULL);
   if (sfd <= 0 || ((sfp = fdopen(sfd, "r+")) == NULL)) 
   {
      cmsLog_error("open control socket error");
      return Error_InitConnectionFailed;
   }

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 220)
   {
      cmsLog_error("server ready error");
      errCode = Error_InitConnectionFailed;
      goto errOut2;
   }

   switch(ftpcmd("USER ", "anonymous", sfd, sfp, buf, sizeof(buf))) {
   case 230:
      break;
   case 331:
      if (ftpcmd("PASS ", "anonymous", sfd, sfp, buf, sizeof(buf)) == 230) break;
      cmsLog_error("auth-pass error");
      errCode = Error_LoginFailed;
      goto errOut2;
   default:
      cmsLog_error("auth-user error");
      errCode = Error_PasswordRequestFailed;
      goto errOut2;
   }

   if (ftpcmd("TYPE I", NULL, sfd, sfp, buf, sizeof(buf)) != 200)
   {
      cmsLog_error("transfer mode error");
      errCode = Error_NoTransferMode;
      goto errOut2;
   }


   if (ftpcmd("PASV", NULL, sfd, sfp, buf, sizeof(buf)) !=  227)
   {
      cmsLog_error("PASV error");
      errCode = Error_NoPASV;
      goto errOut2;
   }
   s = strrchr(buf, ',');
   *s = 0;
   port = atoi(s+1);
   s = strrchr(buf, ',');
   port += atoi(s+1) * 256;
   ((struct sockaddr_in *)&addr)->sin_port = htons(port);
   dfd = open_conn_socket(&addr);
   if (dfd <= 0) 
   {
      cmsLog_error("open data socket error");
      errCode = Error_NoResponse;
      goto errOut2;
   }

   if ((ftpcmd("SIZE ", filename, sfd, sfp, buf, sizeof(buf)) != 213) || (safe_strtoul(buf+4, &filesize)))
   {
      cmsLog_error("SIZE error");
      errCode = Error_IncorrectSize;
      goto errOut1;
   }

   get_if_stats(&connResult->TotalBytesReceivedBegin, &connResult->TotalBytesSentBegin);
   gettimeofday(&connResult->ROMTime, NULL);

   if (ftpcmd("RETR ", filename, sfd, sfp, buf, sizeof(buf)) > 150)
   {
      cmsLog_error("RETR error");
      errCode = Error_Timeout;
      goto errOut1;
   }

   cmsLog_notice("RETR sent");

   set_sockopt_nocopy(dfd);

   cmsLog_notice("start data transfer");

   while (filesize > 0)  
   {
      //cmsLog_debug("retrieve file size:%ld\n", filesize);
      n =read(dfd, buf, (filesize > sizeof(buf))?sizeof(buf):filesize);
      if (n<=0) 
      {
         cmsLog_error("file read error, %d", n);
         errCode = n<0 ? Error_Timeout : Error_TransferFailed;
         goto errOut1;
      }

      if (firstRead) 
      {
         gettimeofday(&connResult->BOMTime, NULL);
         firstRead = 0;
      }

      connResult->TestBytes += n;
      filesize -= n;
   }		

   gettimeofday(&connResult->EOMTime, NULL);

   cmsLog_notice("end data transfer");

   get_if_stats(&connResult->TotalBytesReceivedEnd, &connResult->TotalBytesSentEnd);
   close(dfd);

   if (ftpcmd(NULL, NULL, sfd, sfp, buf, sizeof(buf)) != 226)
   {
      cmsLog_error("read file send ok error");
   }

   ftpcmd("QUIT", NULL, sfd, sfp, buf, sizeof(buf));
   goto errOut2;

errOut1:
   close(dfd);
errOut2:
   close(sfd);
   fclose(sfp);
   cmsLog_debug("return errCode:%d", errCode);
   return errCode;	
}
#endif

int ftpUploadDiag(void)
{
   int ret;
   connectionResult_t diagResult;
   ret = _ftpUploadDiag(&diagResult);

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

int ftpDownloadDiag(void)
{
   int ret;
   connectionResult_t diagResult = {};
   diagResult.index = -1;
   ret = _ftpDownloadDiag(&diagResult);

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

int ftpDownloadDiag_r(connectionResult_t *connResult)
{
   return _ftpDownloadDiag(connResult);
}

int ftpUploadDiag_r(connectionResult_t *connResult)
{
   return _ftpUploadDiag(connResult);
}
