/*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
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
:>
*/
#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_ETHLAG_1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <net/if_arp.h>
#include <net/route.h>

#include "cms_dal.h"
#include "cms_boardcmds.h"
#include "cms_util.h"
#include "cms_qdm.h"

#include "cgi_cmd.h"
#include "cgi_ntwk.h"
#include "cgi_main.h"
#include "syscall.h"
#include "sysdiag.h"
#include "httpd.h"
#include "cgi_main.h"
#include "cgi_util.h"
#include "cgi_sts.h"


static CmsRet cgiEthLagAdd(char *query, FILE *fs) 
{
   CmsRet ret = CMSRET_SUCCESS;
   char str[BUFLEN_512];
   
   cgiGetValueByName(query, "enbWanEthLag", str);
   glbWebVar.enbWanEthLag = !cmsUtl_strcmp(str, "WAN");
   cgiGetValueByName(query, "ethIfName1",  glbWebVar.ethIfName1);
   cgiGetValueByName(query, "ethIfName2",  glbWebVar.ethIfName2);
   cgiGetValueByName(query, "ethLagMode",  glbWebVar.ethLagMode);
   cgiGetValueByName(query, "lacp_rate",  glbWebVar.lacp_rate);
   cgiGetValueByName(query, "xmitHashPolicy",  glbWebVar.xmitHashPolicy);
   cgiGetValueByName(query, "miimon", str);
   glbWebVar.miimon = atoi(str);

   ret = dalEthLag_addInterface(&glbWebVar);
   
   if (ret == CMSRET_SUCCESS)
   {
      /*
       * Wan add was successful, tell handle_request to save the config
       * before releasing the lock.
       */
      glbSaveConfigNeeded = TRUE;

      /* 
      * reload the default glbWebVar value
      */
      cmsDal_getAllInfo(&glbWebVar);
   }
   else
   {
      char errMsg[BUFLEN_128];
      
      snprintf(errMsg, sizeof(errMsg)-1, "Need to select the correct parameters");
      cgiWriteMessagePage(fs, "EthLag Interface Add error", errMsg, "ethlagcfg.cmd?action=view");
      cmsLog_error("Failed to add eth lag interfacet");
   }

   return ret;   
}

static CmsRet cgiEthLagRemove(char *query, FILE *fs) 
{
   char *pToken = NULL, *pLast = NULL;
   char lst[BUFLEN_1024];
   char errMsg[BUFLEN_256+BUFLEN_128];
   CmsRet ret=CMSRET_SUCCESS;
   
   cgiGetValueByName(query, "rmLst", lst);
   pToken = strtok_r(lst, ", ", &pLast);

   while (pToken != NULL)
   {
      cmsUtl_strncpy(glbWebVar.ethLagIfName, pToken, sizeof(glbWebVar.ethLagIfName));

      if ((ret = dalEthLag_deleteInterface(&glbWebVar)) == CMSRET_REQUEST_DENIED)
      {
         snprintf(errMsg, sizeof(errMsg)-1, "All WAN services on %s must be removed prior to move.", glbWebVar.ethLagIfName);
         cgiWriteMessagePage(fs, "EthLag Interface delete error", errMsg, "ethLagCfg.cmd");
         cmsLog_error("dalEthLag_delete failed for %s (ret=%d)", pToken, ret);
         break;
      }
      else if (ret != CMSRET_SUCCESS)
      {
         snprintf(errMsg, sizeof(errMsg)-1, "Incorrect parameters ?");
         cgiWriteMessagePage(fs, "EthLag Interface delete error", errMsg, "ethLagCfg.cmd");
         cmsLog_error("dalEthLag_delete failed for %s (ret=%d)", pToken, ret);
         break;
      }      
      pToken = strtok_r(NULL, ", ", &pLast);

   } /* end of while loop over list of connections to delete */

   /* 
   * reload the default glbWebVar value
   */
   cmsDal_getAllInfo(&glbWebVar);

   /*
    * Whether or not there were errors during the delete,
    * save our config.
    */
   glbSaveConfigNeeded = TRUE;

   return ret;
}


static void writeEthLagCfgScript(FILE *fs)
{
   /* write html header */
   fprintf(fs, "<html><head>\n");
   fprintf(fs, "<link rel=stylesheet href='stylemain.css' type='text/css'>\n");
   fprintf(fs, "<link rel=stylesheet href='colors.css' type='text/css'>\n");
   fprintf(fs, "<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");

   /* show eth label of front/rear panel */
   fprintf(fs, "<script language='javascript' src='portName.js'></script>\n");
   fprintf(fs, "<script language='javascript'>\n");
   fprintf(fs, "<!-- hide\n\n");

   
   /*generate new session key in glbCurrSessionKey*/
   cgiGetCurrSessionKey(0,NULL,NULL);

   fprintf(fs, "var brdId = '%s';\n", glbWebVar.boardID);
   fprintf(fs, "var intfDisp = '';\n");
   fprintf(fs, "var brdIntf = '';\n");

   fprintf(fs, "function addClick() {\n");
   fprintf(fs, "   var code = 'location=\"' + 'cfgethlag.html' + '\"';\n");    
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");

   fprintf(fs, "function removeClick(rml) {\n");
   fprintf(fs, "   var lst = '';\n");
   fprintf(fs, "   if (rml.length > 0)\n");
   fprintf(fs, "      for (i = 0; i < rml.length; i++) {\n");
   fprintf(fs, "         if ( rml[i].checked == true )\n");
   fprintf(fs, "            lst += rml[i].value + ', ';\n");
   fprintf(fs, "      }\n");
   fprintf(fs, "   else if ( rml.checked == true )\n");
   fprintf(fs, "      lst = rml.value;\n");
   fprintf(fs, "   var loc = 'ethLagCfg.cmd?action=remove&rmLst=' + lst;\n\n");
   fprintf(fs, "   loc += '&sessionKey=%d';\n",glbCurrSessionKey);
   fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
//~~~   fprintf(fs, "   alert('code='+code);\n");   
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");

   fprintf(fs, "// done hiding -->\n");
   fprintf(fs, "</script>\n");

}

void cgiEthLagCfgView(FILE *fs)
{
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   Dev2EthLAGObject *lagObj=NULL;
   SINT32 intfCount = 0;
   
   /* write Java Script */
   writeEthLagCfgScript(fs);

   /* write body */
   fprintf(fs, "<title></title>\n</head>\n<body>\n<blockquote>\n<form>\n");

   /* write table */
   fprintf(fs, "<center>\n");
   fprintf(fs, "<b>Ethernet LAG Interface Setup</b><br><br>\n");
   fprintf(fs, "Choose Add, Remove a LAG Interface over 2 selected Etherenet ports.<br><br>\n");
   fprintf(fs, "<table border='1' cellpadding='4' cellspacing='0'>\n");

   /* write table header */
   fprintf(fs, "   <tr align='center'>\n");
   fprintf(fs, "      <td class='hd'>LAG Interface</td>\n");
   fprintf(fs, "      <td class='hd'>LAG Type</td>\n");
   fprintf(fs, "      <td class='hd'>Eth port 1</td>\n");
   fprintf(fs, "      <td class='hd'>Eth port 2</td>\n");   
   fprintf(fs, "      <td class='hd'>Mode</td>\n");
   fprintf(fs, "      <td class='hd'>Lacp Rate</td>\n"); 
   fprintf(fs, "      <td class='hd'>Xmit Hash Policy</td>\n"); 
   fprintf(fs, "      <td class='hd'>MiiMon</td>\n"); 
   fprintf(fs, "      <td class='hd'>Remove</td>\n");
   // later fprintf(fs, "      <td class='hd'>Edit</td>\n");   
   fprintf(fs, "   </tr>\n");

   while (cmsObj_getNext(MDMOID_LA_G, &iidStack, (void **)&lagObj) == CMSRET_SUCCESS)
   {
      cmsLog_debug(" lagObj->name %s, lagObj->lowerLayers %s", lagObj->name, lagObj->lowerLayers);
      fprintf(fs, "      <td>%s</td>\n", lagObj->name);      
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_Upstream ? "WAN" : "LAN");      
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_EthIfName1);
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_EthIfName2);      
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_Mode);
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_LacpRate);
      fprintf(fs, "      <td>%s</td>\n", lagObj->X_BROADCOM_COM_XmitHashPolicy);
      fprintf(fs, "      <td>%d</td>\n", lagObj->X_BROADCOM_COM_Miimon);
      /* show remove checkbox */
      fprintf(fs, "      <td align='center'><input type='checkbox' name='rml' value='%s'></td>\n", lagObj->name);
      fprintf(fs, "  </tr>\n");
      intfCount++;
      cmsObj_free((void **) &lagObj);
   }

   
   fprintf(fs, "</table><br><br>\n");

   if (intfCount < CMS_MAX_ETHLAG_INTF)
   {
      fprintf(fs, "<input type='button' onClick='addClick()' value='Add'>\n");
   } 
   else
   {
      fprintf(fs, "<input type='button' onClick='addClick()' value='Add' disabled='1'>\n");   
   }      
   fprintf(fs, "<input type='button' onClick='removeClick(this.form.rml)' value='Remove'>\n");
   fprintf(fs, "</center>\n</form>\n</blockquote>\n</body>\n</html>\n");

   fflush(fs);

}

void cgiEthLagCfg(char *query, FILE *fs)
{
   char action[BUFLEN_256];

   cgiGetValueByName(query, "action", action);
   

   if ( !cmsUtl_strcmp(action, "add") )
   {
      if (cgiEthLagAdd(query, fs) != CMSRET_SUCCESS) 
      {
         return;
      }   
   }      
   else if ( !cmsUtl_strcmp(action, "remove") )
   {
      if (cgiEthLagRemove(query, fs) != CMSRET_SUCCESS) 
      {
         return;
      }   
   }     

   cgiEthLagCfgView( fs);
   
   
}


void cgiGetConfiguredEthWanForEthlag(int argc __attribute__((unused)), char **argv __attribute__((unused)), char *varValue) 
{
   Dev2EthernetInterfaceObject *ethIntfObj=NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

/* For boards without WAN Ethernet LAG support such as 63158, just return varVal = "" enve if the WAN 
* layer 2 Ethernet interface is enabled.
* 
*/
#if defined(SUPPORT_ETHLAG_LAN_ONLY)
   cmsLog_debug("WAN ethlag is not supported" );
   return;
#endif

   while (cmsObj_getNextFlags(MDMOID_DEV2_ETHERNET_INTERFACE,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &ethIntfObj) == CMSRET_SUCCESS)
   {
      if (ethIntfObj->upstream &&
          ethIntfObj->enable &&
          (!cmsUtl_strcmp(ethIntfObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANONLY) ||
           !cmsUtl_strcmp(ethIntfObj->X_BROADCOM_COM_WanLan_Attribute, MDMVS_WANANDLAN)))
      {
         if (cmsUtl_strlen(varValue) > 0)
         {
            cmsUtl_strcat(varValue, ",");
         }
         cmsUtl_strcat(varValue, ethIntfObj->name);
      }
      cmsObj_free((void **)&ethIntfObj);
   }

   cmsLog_debug(" WAN Ethernet interface=%s", varValue);

   return;
   
}

#endif  /* DMP_DEVICE2_ETHLAG_1 */
#endif  /* DMP_DEVICE2_BASELINE_1 */


