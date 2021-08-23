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

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#ifndef DESKTOP_LINUX
#include <wl_common_defs.h>
#endif

#include "wlsysutil.h"

#define DEV_TYPE_LEN	3
#define IF_NAME_SIZE    16
#define CMD_LENGTH      256
#define BUF_LENGTH      1024


#ifndef DESKTOP_LINUX
static int _wl_get_dev_type(char *name, void *buf, int len)
{
   int s;
   int ret = -1;
   struct ifreq ifr;
   struct ethtool_drvinfo info;

   /* open socket to kernel */
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror("_wl_get_dev_type: open socket");
      return ret;
   }

   /* get device type */
   memset(&info, 0, sizeof(info));
   info.cmd = ETHTOOL_GDRVINFO;
   ifr.ifr_data = (caddr_t)&info;
   strncpy(ifr.ifr_name, name, IF_NAME_SIZE-1);
   if ((ret = ioctl(s, SIOCETHTOOL, &ifr)) < 0) {

      /* print a good diagnostic if not superuser */
      if (errno == EPERM)
         perror("_wl_get_dev_type: SIOCETHTOOL");

      *(char *)buf = '\0';
   }
   else
   {
      strncpy(buf, info.driver, len);
   }

   close(s);
   return ret;
}
#endif

/*
  * Return the number of wlan adapters running on the system.
  * Dynamically detect it at run time.
  */
int wlgetintfNo(void)
{
#ifdef DESKTOP_LINUX
   return 2;
#else
   char proc_net_dev[] = "/proc/net/dev";
   FILE *fp;
   char buf[BUF_LENGTH], *c, *name;
   char dev_type[DEV_TYPE_LEN];
   int wlif_num=0;

   if (!(fp = fopen(proc_net_dev, "r")))
   {
      printf("Error reading %s!\n", proc_net_dev);
      goto out;
   }

   /* eat first two lines */
   if (!fgets(buf, sizeof(buf), fp) ||
         !fgets(buf, sizeof(buf), fp)) 
   {
      fclose(fp);
      goto out;
   }

   while (fgets(buf, sizeof(buf), fp))
   {
      c = buf;
      while (isspace(*c))
         c++;
      if (!(name = strsep(&c, ":")))
         continue;
      //printf("#### %s:%d  name:%s ####\r\n", __FUNCTION__, __LINE__, name);
      if (_wl_get_dev_type(name, dev_type, DEV_TYPE_LEN) >= 0 &&
            !strncmp(dev_type, "wl", 2))
      {
         //printf("#### %s:%d  it is wireless interface and devtype:%s ####\r\n", __FUNCTION__, __LINE__, dev_type);
         /* filter out virtual interface */
         if (!strstr(name,"."))
            wlif_num++;

         /* limit to the maximum */
         if (wlif_num >= WL_MAX_NUM_RADIO)
            break;
      }
   }
   fclose(fp);

out:
   return wlif_num;
#endif
}


/* Return the maximum number of SSID numbers is supported on the system for a given wlan adapter */
#ifdef DESKTOP_LINUX
int wlgetVirtIntfNo( int idx __attribute__((unused)))
{
   return 4;
}
#else
int wlgetVirtIntfNo(int idx)
{
   char buf[BUF_LENGTH], cmd[CMD_LENGTH];
   FILE *fp;
   int num = WL_DEFAULT_NUM_SSID; /* default to system maximum */

   if (wlgetintfNo() == 0)
       return 0;

   snprintf(cmd, CMD_LENGTH, "wl -i wl%d cap", idx);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      fprintf(stderr, "Error opening pipe!\n");
      goto out;
   }

   if (fgets(buf, BUF_LENGTH, fp) != NULL)
   {
      //cmsLog_error("#### [%s] ####", buf);
      if (strstr(buf, "1ssid"))
      {
         num= 1;
      } else if (strstr(buf, "mbss4"))
      {
         num= 4;
      } else if (strstr(buf, "mbss8"))
      {
         num= 8;

      } else if (strstr(buf, "mbss16"))
      {
         num= 16;
      }
      //printf("#### HW supported mbss=[%d] ####\n", num);
   }

   pclose(fp);

out:
   /* limit to the maximum */
   return num > WL_MAX_NUM_SSID ? WL_MAX_NUM_SSID : num;
}
#endif

#ifdef DESKTOP_LINUX
void wlgetVer(int idx __attribute__((unused)), char* version)
{
    if (version != NULL)
        sprintf(version, "17.10");
}
#else
void wlgetVer(int idx, char* version)
{
    char *tag = "version ", *p;
    char buf[BUF_LENGTH], cmd[CMD_LENGTH];
    FILE *fp;

    if (wlgetintfNo() == 0)
    {
        sprintf(version, "N/A");
        return;
    }

    snprintf(cmd, CMD_LENGTH, "wl -i wl%d  ver", idx);
    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        sprintf(version, "N/A");
        return;
    }

    if (fgets(buf, BUF_LENGTH, fp) == NULL) // skip the first line of output
    {
        sprintf(version, "N/A");
    } else if (fgets(buf, BUF_LENGTH, fp) != NULL)
    {
        p = strstr(buf, tag);
        if (p != NULL)
        {
            p += strlen(tag);
            sscanf(p, "%s", version);
        } else
            sprintf(version, "N/A");
    } else
    {
        sprintf(version, "N/A");
    }

    pclose(fp);
}
#endif

#ifdef DESKTOP_LINUX
int wlgetChannelList(const char *ifname __attribute__((unused)),
                     int band __attribute__((unused)),
                     int bandwidth __attribute__((unused)),
                     const char *country __attribute__((unused)),
                     int chanList[],
                     int size)
{
   int i;
   if (size < 14)
      return -1;

   for (i = 0 ; i < 14 ; i ++)
   {
      chanList[i] = i+1;
   }

   return 14;
}
#else
int wlgetChannelList(const char *ifname,
                     int band,
                     int bandwidth,
                     const char *country,
                     int chanList[],
                     int size)
{
   int ret = -1, i = 0;
   char buf[BUF_LENGTH], cmd[CMD_LENGTH];
   FILE *fp;

   if (wlgetintfNo() == 0)
   {
      return -1;
   }

   if (!country)
      snprintf(cmd, CMD_LENGTH, "wl -i %s chanspecs -b %d -w %d", ifname, band, bandwidth);
   else
      snprintf(cmd, CMD_LENGTH, "wl -i %s chanspecs -b %d -w %d -c %s", ifname, band, bandwidth, country);

   if ((fp = popen(cmd, "r")) == NULL)
   {
      fprintf(stderr, "Error opening pipe!\n");
      goto out;
   }

   while (fgets(buf, BUF_LENGTH, fp) != NULL)
   {
      if ( i >= size)
      { 
         ret = -2;   // chanlist array is too small
         pclose(fp);
         goto out;
      }
      if (sscanf(buf, "%d", &chanList[i]) == 1)
         i++;
   }

   ret = i;
   pclose(fp);

out:
   return ret;
}
#endif


#ifdef DESKTOP_LINUX
int wlgetStaionStats(const char* sta_addr __attribute__((unused)),
                     Station_Stats *stats)
{
    if (!stats)
        return -1;

    bzero(stats, sizeof(Station_Stats));
    return 0;
}
#else

int wlgetStaionStats(const char* sta_addr, Station_Stats *stats)
{
    char buf[BUF_LENGTH], cmd[CMD_LENGTH], *p;
    FILE *fp;

    if (!stats || wlgetintfNo() == 0)
        return -1;

    bzero(stats, sizeof(Station_Stats));
    snprintf(cmd, CMD_LENGTH, "wl sta_info %s", sta_addr);

    if ((fp = popen(cmd, "r")) == NULL)
    {
        fprintf(stderr, "Error opening pipe!\n");
        return -1;
    }

    while (fgets(buf, BUF_LENGTH, fp) != NULL)
    {
        // make sure each item would only be process once
        if (!stats->packetsSent && (p=strstr(buf, "tx total pkts: ")) != NULL)
            sscanf(p+15, "%llu", &stats->packetsSent);
        else if (!stats->bytesSent && (p=strstr(buf, "tx total bytes: ")) != NULL)
            sscanf(p+16, "%llu", &stats->bytesSent);
        else if (!stats->errorsSent &&  (p=strstr(buf, "tx failures: ")) != NULL)
            sscanf(p+13, "%u", &stats->errorsSent);
        else if (!stats->packetsReceived && (p=strstr(buf, "rx data pkts: ")) != NULL)
            sscanf(p+14, "%llu", &stats->packetsReceived);
        else if (!stats->bytesReceived && (p=strstr(buf, "rx data bytes: ")) != NULL)
            sscanf(p+15, "%llu", &stats->bytesReceived);
        else if (!stats->retransCount && (p=strstr(buf, "tx data pkts retried: ")) != NULL)
            sscanf(p+22, "%u", &stats->retransCount);
        else if (!stats->failedRetransCount && (p=strstr(buf, "tx pkts retry exhausted: ")) != NULL)
            sscanf(p+25, "%u", &stats->failedRetransCount);
        else if (!stats->retryCount &&  (p=strstr(buf, "tx pkts retries: ")) != NULL)
            sscanf(p+17, "%u", &stats->retryCount);
    }

    pclose(fp);
    return 0;
}
#endif
