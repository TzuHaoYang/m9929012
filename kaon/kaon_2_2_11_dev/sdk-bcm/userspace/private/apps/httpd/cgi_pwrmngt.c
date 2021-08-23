#ifdef DMP_X_BROADCOM_COM_PWRMNGT_1 /* aka SUPPORT_PWRMNGT */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "httpd.h"
#include "cgi_main.h"
#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_dal.h"
#include "cgi_pwrmngt.h"

#ifdef BUILD_BCM_WLAN_DPDCTL
extern char *nvram_get(const char *name);
#ifdef BUILD_BRCM_UNFWLCFG
#include "nvram_api.h"
extern char *nvram_kget(const char *name);
#else
#define nvram_kget    nvram_get
#endif
#endif

static int pmWlSetWlDpdCtl(char *query);
static void pmWlDpdCtlInit(void);

void cgiPowerManagement(char *query, FILE *fs) 
{
   CmsRet ret = CMSRET_SUCCESS;
   char path[BUFLEN_1024];
   char cgiVal[4];

   cmsLog_debug("cgiPowerManagement");
   if (cgiGetValueByName(query, "PmCPUSpeed", cgiVal) == CGI_STS_OK)
      glbWebVar.pmCPUSpeed = atoi(cgiVal);
   if (cgiGetValueByName(query, "PmCPUr4kWaitEn", cgiVal) == CGI_STS_OK)
      glbWebVar.pmCPUr4kWaitEn = atoi(cgiVal);
   if (cgiGetValueByName(query, "PmDRAMSelfRefreshEn", cgiVal) == CGI_STS_OK)
      glbWebVar.pmDRAMSelfRefreshEn = atoi(cgiVal);
   if (cgiGetValueByName(query, "PmEthAutoPwrDwnEn", cgiVal) == CGI_STS_OK)
      glbWebVar.pmEthAutoPwrDwnEn = atoi(cgiVal);
   if (cgiGetValueByName(query, "PmEthEEE", cgiVal) == CGI_STS_OK)
      glbWebVar.pmEthEEE = atoi(cgiVal);
   if (cgiGetValueByName(query, "PmAvsEn", cgiVal) == CGI_STS_OK)
      glbWebVar.pmAvsEn = atoi(cgiVal);

   if (pmWlSetWlDpdCtl(query) != 0)
   {
      cmsLog_error("pmWlSetWlDpdCtl failed");
      return;
   }

   if ((ret = dalPowerManagement(&glbWebVar)) != CMSRET_SUCCESS) 
   {
      cmsLog_error("dalPowerManagement failed, ret=%d", ret);
      return;
   }
   else 
   {
      glbSaveConfigNeeded = TRUE;
   }
   makePathToWebPage(path, sizeof(path), "pwrmngt.html");
   do_ej(path, fs);

   return;

} /* cgiPowerManagementCfg */

void cgiGetPowerManagement(int argc __attribute__((unused)),
                           char **argv __attribute__((unused)),
                           char *varValue __attribute__((unused)))
{
   cmsLog_debug("cgiGetPowerManagement");
   dalGetPowerManagement(&glbWebVar);
   pmWlDpdCtlInit();

   return;
}



/*
 * Initialize the WLAN power management variables
 * called when power management page is loaded/"Refresh"/"Apply"
 */
static void pmWlDpdCtlInit(void)
{
    int numIf = 0;
#ifdef BUILD_BCM_WLAN_DPDCTL
    char key[32];
    char *buf = nvram_kget("wl_dpdctl_enable");

    /* Initialize interface level */
    for (numIf=0; numIf < WLAN_MAX_NUM_WLIF; numIf++) {
        glbWebVar.pmWlDpd[numIf] = 0;
    }

    if (buf && (atoi(buf) == 1)) {
        /* Initialize if not done already */
        system("/etc/init.d/wifi.sh dpdsts > /dev/null");
        for (numIf=0; numIf < WLAN_MAX_NUM_WLIF; numIf++) {
            snprintf(key, sizeof(key), "wl%d_dpd", numIf);
            buf = nvram_kget(key);
            if (buf == NULL) {
                break;
            } else {
                glbWebVar.pmWlDpd[numIf] = atoi(buf);
            }
        }
    } else {
        numIf = 0;
    }
#endif /* BUILD_BCM_WLAN_DPDCTL */

    glbWebVar.pmWlIfNum = numIf;
    return;
}

#ifdef BUILD_BCM_WLAN_DPDCTL
/*
 * Apply the WLAN power management settings
 * Check for changes in gloabl enable/disable
 * Check for individual interface power down/up change
 * Apply if there is change
 */
static int pmWlSetWlDpdCtl(char *query)
{
   int intf;
   char wlcginame[32];
   int pmwlchg = 0;
   char dpdctlcmd[256];
   char cgiVal[4];
   int enable;


   /* Process power down enable first */
   if (cgiGetValueByName(query, "PmWlanDpd", cgiVal) == CGI_STS_OK) {
      enable = (glbWebVar.pmWlIfNum) ? 1 : 0;
      if (enable != atoi(cgiVal)) {
         pmwlchg = 1;
         enable = atoi(cgiVal);
      }
   }

   if (pmwlchg) {
      snprintf(dpdctlcmd, sizeof(dpdctlcmd), "/etc/init.d/wifi.sh dpden %d  > /dev/null",  enable);
      system(dpdctlcmd);
      pmWlDpdCtlInit();
   }
   pmwlchg = 0;

   snprintf(dpdctlcmd, sizeof(dpdctlcmd), "/etc/init.d/wifi.sh dpddn \"");
   for (intf=0; intf < glbWebVar.pmWlIfNum; intf++)
   {
      snprintf(wlcginame, sizeof(wlcginame), "PmWlDpdDn%d", intf);
      if (cgiGetValueByName(query, wlcginame, cgiVal) == CGI_STS_OK) {
         if (glbWebVar.pmWlDpd[intf] != atoi(cgiVal)) {
            pmwlchg = 1;
         }
         glbWebVar.pmWlDpd[intf] = atoi(cgiVal);
         if (glbWebVar.pmWlDpd[intf] == 1) {
             snprintf(dpdctlcmd + strlen(dpdctlcmd), sizeof(dpdctlcmd), "wl%d ",  intf);
         }
      }
   }
   
   if (pmwlchg == 1) {
      snprintf(dpdctlcmd + strlen(dpdctlcmd), sizeof(dpdctlcmd), "\" > /dev/null\n");
      system(dpdctlcmd);
   }

   return 0;
}

/*
 * Build web page table for the wlx power down control based on the number of available interfaces
 * This function is called from the power mangement web page (html/pwrmngt.html)
 */
void cgiBuildWlDpdCtl(int argc __attribute__((unused)), char **argv __attribute__((unused)), char *varValue)
{
   char *p = varValue;
   int intf;

   p += sprintf(p, "            <table border=0 cellspacing=1 cellpadding=3 >\n");

   for (intf=0; intf < WLAN_MAX_NUM_WLIF; intf++)
   {
      if (intf < glbWebVar.pmWlIfNum) {
         p += sprintf(p, "<tr>\n");
      } else {
         p += sprintf(p, "<tr style=\"display:none\">\n");
      }
      p += sprintf(p, "                <td &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp</td>\n");
      p += sprintf(p, "                <td>&nbsp;wl%d&nbsp;&nbsp;&nbsp;&nbsp;<input type='checkbox' name='chkPmWlDpdDn[]' value='PmWlDpdDn%d' %s> &nbsp;&nbsp;Down&nbsp;&nbsp;&nbsp;&nbsp;</td>\n",
          intf, intf, (glbWebVar.pmWlDpd[intf]) ? "checked" : "");
      p += sprintf(p, "                <td class='hd'>Status: <font color='%s'>%s</font></td>\n",
          (glbWebVar.pmWlDpd[intf]) ? "red" : "green", (glbWebVar.pmWlDpd[intf]) ? "Down" : "Up");
      p += sprintf(p, "              </tr>\n");
   }
   p += sprintf(p, "            </table>\n");
}
#else /* !BUILD_BCM_WLAN_DPDCTL */
static int pmWlSetWlDpdCtl(char *query __attribute__((unused)))
{
    return 0;
}

void cgiBuildWlDpdCtl(int argc __attribute__((unused)), char **argv __attribute__((unused)), char *varValue __attribute__((unused)))
{
    return;
}
#endif /* BUILD_BCM_WLAN_DPDCTL */
#endif /* aka SUPPORT_PWRMNGT */
