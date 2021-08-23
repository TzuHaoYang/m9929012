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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>         
#include <netdb.h>

   /* 
    * Normally, TCP_NOCOPY value will be available through the libc toolchain headers
    * when recompiled against the kernel header file. However, we want to avoid
    * changing the toolchain, so this value is redefined here.
    * Note that TCP_NOCOPY value is kernel version dependent.
    */
#define TCP_NOCOPY 27 

#include "tr143_defs.h"
#include "tr143_private.h"
#include "mdm_validstrings.h"
#include "cms_msg.h"

#ifdef BRCM_SPDTEST
#include <spdt_api.h>
#include <signal.h>

int8_t g_stream_idx = -1;

static void terminal_signal_handler_func(int sig_num)
{
   if (g_stream_idx == -1) return;

   spdt_disconnect(g_stream_idx);
   spdt_uninit(g_stream_idx);
}

int tcpspeedtest_setup(spdt_proto_t proto)
{
   struct sigaction new_action = {};
   int rc;

   new_action.sa_handler = terminal_signal_handler_func;
   new_action.sa_flags = SA_RESETHAND;
   if ((rc = sigaction(SIGTERM, &new_action, NULL))) return rc;

   /* reuse SIGTERM handler for SIGINT */
   if ((rc = sigaction(SIGINT, &new_action, NULL))) return rc;

   return spdt_init(proto, (uint8_t *)&g_stream_idx);
}
#endif /* BRCM_SPDTEST */

char connIfName[CMS_IFNAME_LENGTH] = {};
UBOOL8 loggingSOAP = FALSE;

char *tr143_result_fname = NULL;
char server_name[1024] = "192.168.1.100";
struct sockaddr_storage server_addr = {};

int server_port;
char uri[256], proto[256];
char *url, if_name[32];
unsigned char dscp;
unsigned int testFileLength = 0;
void *msgHandle = NULL;
struct tr143diagstats_t diagstats;

const char *DiagnosticsState[] = {
   MDMVS_NONE,
   MDMVS_REQUESTED,
   MDMVS_COMPLETE,
   MDMVS_ERROR_INITCONNECTIONFAILED,
   MDMVS_ERROR_NORESPONSE,
   MDMVS_ERROR_TRANSFERFAILED,
   MDMVS_ERROR_PASSWORDREQUESTEDFAILED,
   MDMVS_ERROR_LOGINFAILED,
   MDMVS_ERROR_NOTRANSFERMODE,
   MDMVS_ERROR_NOPASV,
   MDMVS_ERROR_INCORRECTSIZE,
   MDMVS_ERROR_TIMEOUT,
   MDMVS_ERROR_NOCWD,
   MDMVS_ERROR_NOSTOR,
};


void cleanup_and_notify(Tr143DiagState state, CmsEntityId eid)
{
   FILE * fp;
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   cmsLog_notice("exit state = %d", state);
   cmsLog_debug("tcpOpen:%s, tcpResp:%s\nROM:%s, BOM:%s, EOM:%s\n"
         "TestBytesReceived:%d, TotalBytesReceived:%d, TotalBytesSent:%d", 
         diagstats.TCPOpenRequestTime, diagstats.TCPOpenResponseTime, 
         diagstats.ROMTime, diagstats.BOMTime, diagstats.EOMTime,
         diagstats.TestBytes, diagstats.TotalBytesReceived, diagstats.TotalBytesSent);

   strncpy(diagstats.DiagnosticsState, DiagnosticsState[state], sizeof(diagstats.DiagnosticsState));
   fp=fopen(tr143_result_fname,"w");
   if (fp) 
   {
      fwrite(&diagstats,1,sizeof(diagstats),fp);
      fclose(fp);
   }
   else
      cmsLog_error("open %s error", tr143_result_fname);

   if (msgHandle)
   {
      msg.type = CMS_MSG_DIAG;
      msg.src =  eid;
      msg.dst = EID_SMD;
      msg.flags_event = 1;
      if (cmsMsg_send(msgHandle, &msg) != CMSRET_SUCCESS)
         cmsLog_error("could not send out CMS_MSG_DIAG event msg");
      else
         cmsLog_debug("Send out CMS_MSG_DIAG event msg.");

      cmsMsg_cleanup(&msgHandle);
   }
   cmsLog_cleanup();
}

int safe_strtoul(char *arg, unsigned long* value)
{
   char *endptr;
   int errno_save = errno;

   if (arg ==NULL) return -1;

   errno = 0;
   *value = strtoul(arg, &endptr, 0);
   if (errno != 0 || endptr==arg) {
      return -1;
   }
   errno = errno_save;
   return 0;
}

//flag=1 is read, flag=2 is write
int select_with_timeout(int fd, int flag)
{
   fd_set fdset;
   struct timeval tm;
   int ret;

   FD_ZERO(&fdset);
   FD_SET(fd, &fdset);
   tm.tv_sec = TR143_SESSION_TIMEOUT;
   tm.tv_usec = 0;

   if (flag)
      ret = select(fd + 1, &fdset, NULL, NULL, &tm);
   else
      ret = select(fd + 1, NULL, &fdset, NULL, &tm);

   if ( ret <= 0 || !FD_ISSET(fd, &fdset)) 
   {
      cmsLog_error("select timeout");
      return -1;
   }

   return 0;
}

size_t read_with_timeout(int fd, char *buf, int len)
{
   if (select_with_timeout(fd, 1))
      return -1;

   return read(fd, buf, len);
}

static int host_to_addr(char *name, struct sockaddr_storage *addr)
{
   struct addrinfo hints = {};
   struct addrinfo *res;

   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(name, NULL, &hints, &res) || !res)
      return -1;

   addr->ss_family = res->ai_family;            /* IP/IPv6 */
   memcpy(addr, res->ai_addr, res->ai_addrlen); /* Copy address */
   freeaddrinfo(res);

   return 0;
}

int open_conn_socket(struct sockaddr_storage *addr)
{
   int fd;
   int flags, bflags;
   struct ifreq ifr;
   unsigned char tos;
   struct sockaddr_in bindsun;
   int if_index;
   socklen_t sunlen;

   fd = socket(addr->ss_family, SOCK_STREAM, 0);
   if (fd < 0)
   {
      cmsLog_error("open socket error");
      perror("open socket error");
      return -1;
   }

   if (if_name[0])
   {
      strcpy(ifr.ifr_name, if_name);
      ifr.ifr_addr.sa_family = addr->ss_family;
      if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) 
      {
         cmsLog_error("SIOCGIFADDR error");
         return -1;
      }

      if (bind(fd, (struct sockaddr *)&ifr.ifr_addr, sizeof(ifr.ifr_addr)) < 0)
      {
         close(fd);
         cmsLog_error("bind error");
         return -1;
      }
   }

   flags = (long) fcntl(fd, F_GETFL);
   bflags = flags | O_NONBLOCK; /* clear non-block flag, i.e. block */
   fcntl(fd, F_SETFL, bflags);
   if ((connect(fd, (struct sockaddr *)addr, sizeof(*addr)) < 0 && errno != EINPROGRESS) ||
         select_with_timeout(fd, 0) < 0)
   {
      close(fd);
      cmsLog_error("connect error");
      return -1;
   }       
   fcntl(fd, F_SETFL, flags);

   if (if_name[0] == '\0')
   {
      sunlen = sizeof(bindsun);
      getsockname(fd, (struct sockaddr *)&bindsun, &sunlen);

      for (if_index = 1; 1; if_index++)
      {
         ifr.ifr_ifindex = if_index;
         if (ioctl(fd, SIOCGIFNAME, &ifr)) break;
         if (ioctl(fd, SIOCGIFADDR, &ifr)) continue;
         if (bindsun.sin_addr.s_addr == ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr)
         {
            strncpy(if_name, ifr.ifr_name, sizeof(if_name)-1);
            cmsLog_debug("bind interface:%s\n", ifr.ifr_name);
            break;
         }
      }
   }

   tos = dscp <<  2;
   if (setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < 0)
      cmsLog_error("set tos error");

   return fd;
}


int setupconfig()
{
   cmsLog_notice("interface %s, dscp %d, url %s", if_name, dscp, url);

   if (url == NULL)
   {
      cmsLog_error("url is empty");
      return -1;
   }

   if (parseUrl(url, proto, server_name, &server_port, uri) < 0)
   {
      cmsLog_error("url parse error");
      return -1;
   }

   if (uri[0] == '\0' || (uri[0] == '/' && uri[1] == '\0'))
   {
      cmsLog_error("uri is null");
      return -1;
   }

   if (host_to_addr(server_name, &server_addr))
   {
      cmsLog_error("can't resolve host name");
      return -1;
   }

   memset(&diagstats, 0, sizeof(diagstats));
   return 0;
}

int get_if_stats(unsigned int *rx, unsigned int *tx)
{
   FILE *fh;
   char buf[512], *p;
   unsigned int discard;
   int ret = -1;
   unsigned int lrx, ltx;

   if (if_name[0] == '\0'){
      cmsLog_error("if_name is null");
      return -1;
   }

   fh = fopen("/proc/net/dev", "r");
   if (!fh) {
      cmsLog_error("/proc/net/dev couldn't be opened");
      return -1;
   }

   fgets(buf, sizeof(buf), fh);
   fgets(buf, sizeof(buf), fh);

   while (fgets(buf, sizeof(buf), fh)) 
   {
      if (!strstr(buf, if_name) || !(p = strchr(buf, ':'))) continue;
      sscanf(p+1, "%u%u%u%u%u%u%u%u%u",
            &lrx, &discard, &discard, &discard, &discard, &discard, &discard, &discard, &ltx);
      ret = 0;
      break;
   }

   if (rx) *rx = lrx;
   if (tx) *tx = ltx;

   fclose(fh);
   return ret;
}

void set_sockopt_nocopy(int fd)
{
#ifdef CONFIG_BCM_SPEEDYGET
   int optval=1; 
   if(setsockopt(fd,SOL_TCP,TCP_NOCOPY,&optval,sizeof(optval)))
      perror("setting setsockopt\n");
   else
      cmsLog_notice("set sock opt");
#endif
   return;
}


int compare_timestamp(struct timeval t1, struct timeval t2)
{
   if (t1.tv_sec > t2.tv_sec)
      return 1;
   else if (t2.tv_sec > t1.tv_sec)
      return -1;
   else if (t1.tv_usec > t2.tv_usec)
      return 1;
   else if (t2.tv_usec > t2.tv_usec)
      return -1;
   else
      return 0;
}


void add_connection_result(connectionResult_t *result, int index)
{
   FILE * fp;
   char connectionFileName[CMS_IFNAME_LENGTH];

   if (result == NULL)
      return;

   sprintf(connectionFileName, "%s%s%d", tr143_result_fname, TR143_CONNFILE_MIDLE_NAME, index);
   fp = fopen(connectionFileName, "w");
   if (fp)
   {
      fwrite(result, 1, sizeof(connectionResult_t), fp);
      fclose(fp);
   }
   else
      cmsLog_error("open %s error", connectionFileName);

}
