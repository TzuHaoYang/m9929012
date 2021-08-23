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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "httpd.h"
#include "cgi_main.h"
#include "cgi_wl.h"

#include <bcmnvram.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_dal.h"

#ifdef SUPPORT_WLAN_VISUALIZATION
#include <vis_gui.h>
#endif

void wlParseSetUrl(char *path)
{
    char *query = strchr(path, '?');
    char *name, *value, *next;

    if (query) {
        for (value = ++query; value; value = next) {
            name = strsep(&value, "=");
            if (name) {
                next = value;
                value = strsep(&next, "&");
                if (!value) {
                    value = next;
                    next = NULL;
                }
                cgiUrlDecode(value);
            } else
                next = NULL;
        }
    }
}


void do_wl_status_get(char *path, FILE *fs)
{
    if(nvram_get("_wlrestart_"))
        fputs("0",fs);
    else
        fputs("1",fs);
}

void do_wl_cgi(char *path, FILE *fs)
{
    char filename[WEB_BUF_SIZE_MAX];
    char* query = NULL;
    char* ext = NULL;
    char *beginPtr=NULL;
    int offset=0;
    char tmpBuf[BUFLEN_64];
#ifdef SUPPORT_QUICKSETUP
    int doQuickSetup = FALSE;
#endif

    cmsLog_debug("Enter: path=%s", path);

    query = strchr(path, '?');

    cgiGetValueByName(query, "sessionKey", tmpBuf);
    cgiUrlDecode(tmpBuf);
    cgiSetVar("sessionKey", tmpBuf);

    if ( query != NULL ) {
        wlParseSetUrl(path);
    }

    filename[0] = '\0';

    /*
     * On real system, path begins with /webs/, but on DESKTOP_LINUX, path
     * has some linux fs path then /webs/.  Find the offset.  On real system,
     * offset will be 0.
     */
    beginPtr = strstr(path, "/webs/");
    if (beginPtr == NULL) {
        cmsLog_error("Could not find /webs/ prefix in path %s", path);
        /* we could not find the expected /webs prefix, complain but still
         * use it as is.
         */
        beginPtr = path;
    }
    offset = beginPtr - path;

    ext = strchr(&(path[offset]), '.');
    if ( ext != NULL ) {
        *ext = '\0';
        strcpy(filename, &(path[offset]));
        cmsLog_debug("filename=%s", filename);

#ifdef SUPPORT_QUICKSETUP
        if ( strcmp(filename, "/webs/quicksetup") == 0 ) {
            doQuickSetup = TRUE;
        }
#endif

        strcat(filename, ".html");

#ifdef SUPPORT_QUICKSETUP
        if(doQuickSetup == TRUE) {
            glbWlRestartNeeded = TRUE;
            do_ej("/webs/quicksetuptestsucc.html", fs);
            return;
        }
#endif

        do_ej(filename, fs);
    }
}


#ifdef SUPPORT_WIFIWAN

/* XXX TODO: most of the code in this section is data model independent.
 * However, there is a little bit which is TR98 specific.  Need to break that out.
 */
static void writeWifiWanCfgScript(FILE *fs, char *addLoc, char *removeLoc)
{

    /* write html header */
    fprintf(fs, "<html><head>\n");
    fprintf(fs, "<link rel=stylesheet href='stylemain.css' type='text/css'>\n");
    fprintf(fs, "<link rel=stylesheet href='colors.css' type='text/css'>\n");
    fprintf(fs, "<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");

    /* show wl label of front/rear panel */
    fprintf(fs, "<script language='javascript' src='portName.js'></script>\n");
    fprintf(fs, "<script language='javascript'>\n");
    fprintf(fs, "<!-- hide\n\n");

    /*generate new session key in glbCurrSessionKey*/
    cgiGetCurrSessionKey(0,NULL,NULL);

    /* show wl label of front/rear panel */
    fprintf(fs, "var brdId = '%s';\n", glbWebVar.boardID);
    fprintf(fs, "var intfDisp = '';\n");
    fprintf(fs, "var brdIntf = '';\n");

    fprintf(fs, "function addClick() {\n");
    fprintf(fs, "   var code = 'location=\"' + '%s.html' + '\"';\n", addLoc);    /* for atm, cfgatm, ptm, cfgptm */
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

    fprintf(fs, "   var loc = '%s.cmd?action=remove&rmLst=' + lst;\n\n", removeLoc); /* for atm, dslatm, ptm, dslptm */
    fprintf(fs, "   loc += '&sessionKey=%d';\n",glbCurrSessionKey);
    fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
    fprintf(fs, "   eval(code);\n");
    fprintf(fs, "}\n\n");
    fprintf(fs, "// done hiding -->\n");
    fprintf(fs, "</script>\n");

    /* write body title */
    fprintf(fs, "<title></title>\n</head>\n<body>\n<blockquote>\n<form>\n");
    fprintf(fs, "<center>\n");
}

static void cgiWifiWanCfgView(FILE *fs)
{
    WanDevObject *wanDev=NULL;
    WanCommonIntfCfgObject *wanCommon=NULL;
    WanWifiIntfObject *wlIntf = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    NameList *ifList = NULL;
    UBOOL8 isSet = FALSE;
    CmsRet ret;

    /* write Java Script */
    writeWifiWanCfgScript(fs, "cfgwlwan", "wifiwan");

    /* write table */
    fprintf(fs, "<b>WiFi WAN Interface Configuration</b><br><br>\n");
    fprintf(fs, "Choose Add, or Remove to configure WiFi WAN interfaces.<br>\n");
    fprintf(fs, "Allow one WiFi as layer 2 wan interface.<br><br>\n");
    fprintf(fs, "<table border='1' cellpadding='4' cellspacing='0'>\n");
    /* write table header */
    fprintf(fs, "   <tr align='center'>\n");
    fprintf(fs, "      <td class='hd'>Interface/(Name)</td>\n");
    fprintf(fs, "      <td class='hd'>Connection Mode</td>\n");
    fprintf(fs, "      <td class='hd'>Remove</td>\n");
    fprintf(fs, "   </tr>\n");


    while (cmsObj_getNext(MDMOID_WAN_DEV, &iidStack, (void **)&wanDev) == CMSRET_SUCCESS) {
        if( (ret = cmsObj_get(MDMOID_WAN_COMMON_INTF_CFG, &iidStack, 0, (void **)&wanCommon)) != CMSRET_SUCCESS ) {
            cmsLog_error("Cannot get WanCommonIntfCfgObject, ret = %d", ret);
            cmsObj_free((void **)&wanDev);
            break;
        } else {
            cmsObj_free((void **)&wanDev);
            if (cmsUtl_strcmp(wanCommon->WANAccessType, MDMVS_X_BROADCOM_COM_WIFI)) {
                /* it is a not an wifi wan device, so skip it. */
                cmsObj_free((void **) &wanCommon);
                continue;
            } else {
                cmsObj_free((void **) &wanCommon);
                if( (ret = cmsObj_get(MDMOID_WAN_WIFI_INTF, &iidStack, 0, (void **)&wlIntf)) != CMSRET_SUCCESS ) {
                    cmsLog_error("Cannot get WanWlIntfObject, ret = %d", ret);
                    cmsObj_free((void **)&wanDev);
                    break; /* break out of while loop so we send a response to caller */
                } else {
                    if (wlIntf->ifName != NULL) {
                        fprintf(fs, "   <tr align='center'>\n");

                        /* show wl label of front/rear panel */
                        /* fprintf(fs, "      <td>%s</td>\n", wlIntf->ifName); */
                        fprintf(fs, "<script language='javascript'>\n");
                        fprintf(fs, "<!-- hide\n");
                        fprintf(fs, "brdIntf = brdId + '|' + '%s';\n", wlIntf->ifName);
                        fprintf(fs, "intfDisp = getUNameByLName(brdIntf);\n");
                        fprintf(fs, "document.write('<td>%s/' + intfDisp + '</td>');\n", wlIntf->ifName);
                        fprintf(fs, "// done hiding -->\n");
                        fprintf(fs, "</script>\n");

                        fprintf(fs, "      <td>%s</td>\n", wlIntf->connectionMode);

                        /* remove check box */
                        fprintf(fs, "      <td align='center'><input type='checkbox' name='rml' value='%s'></td>\n",
                                wlIntf->ifName);
                        fprintf(fs, "   </tr>\n");

                        isSet = TRUE; /* set Wl layer2 wan interface already */
                    }
                    /* free wlIntf */
                    cmsObj_free((void **) &wlIntf);
                }
            }
        }
    } /* while loop over WANDevice. */

    fprintf(fs, "</table><br>\n");

    /* check available Wl layer2 interface, if there is no iface, don't show add button */
    /* if (cmsDal_getAvailableL2WlIntf(&ifList) == CMSRET_SUCCESS && ifList != NULL)*/
    /* Only support one Wl layer2 interface */
    if (!isSet) {
        fprintf(fs, "<input type='button' onClick='addClick()' value='Add'>\n");
        cmsDal_freeNameList(ifList);
    }

    fprintf(fs, "<input type='button' onClick='removeClick(this.form.rml)' value='Remove'>\n");

    fprintf(fs, "</center>\n</form>\n</blockquote>\n</body>\n</html>\n");
    fflush(fs);
}

static CmsRet cgiWifiWanAdd(char *query, FILE *fs)
{
    char connectionMode[BUFLEN_8];

    cgiGetValueByName(query, "ifname",  glbWebVar.wanL2IfName);
    cgiGetValueByName(query, "connMode",  connectionMode);
    glbWebVar.connMode = atoi(connectionMode);

    if (dalWifiWan_addWlInterface(&glbWebVar) != CMSRET_SUCCESS) {
        do_ej("/webs/wifiwanadderr.html", fs);
        return CMSRET_INTERNAL_ERROR;
    } else {
        cmsLog_debug("dalWifiWan_addWlWanInterface ok.");
        /*
         * Wl add was successful, tell handle_request to save the config
         * before releasing the lock.
         */
        glbSaveConfigNeeded = TRUE;
    }

    return CMSRET_SUCCESS;

}

static CmsRet cgiWifiWanRemove(char *query, FILE *fs)
{
    char *pToken = NULL, *pLast = NULL;
    char lst[BUFLEN_1024];
    CmsRet ret=CMSRET_SUCCESS;

    cgiGetValueByName(query, "rmLst", lst);
    pToken = strtok_r(lst, ", ", &pLast);

    while (pToken != NULL) {
        strcpy(glbWebVar.wanL2IfName, pToken);

        if ((ret = dalWifiWan_deleteWlInterface(&glbWebVar)) == CMSRET_REQUEST_DENIED) {
            do_ej("/webs/wifiwandelerr.html", fs);
            return ret;
        } else if (ret != CMSRET_SUCCESS) {
            cmsLog_error("dalWifiWan_deleteWlInterface failed for  failed for %s (ret=%d)", glbWebVar.wanL2IfName, ret);
            return ret;
        }

        pToken = strtok_r(NULL, ", ", &pLast);

    } /* end of while loop over list of connections to delete */

    /*
     * Whether or not there were errors during the delete,
     * save our config.
     */
    glbSaveConfigNeeded = TRUE;

    return ret;
}

void cgiWifiWanCfg(char *query, FILE *fs)
{
    char action[BUFLEN_256];

    cgiGetValueByName(query, "action", action);
    if (cmsUtl_strcmp(action, "add") == 0) {
        if (cgiWifiWanAdd(query, fs) != CMSRET_SUCCESS) {
            return;
        }
    } else if (cmsUtl_strcmp(action, "remove") == 0) {
        if (cgiWifiWanRemove(query, fs) != CMSRET_SUCCESS) {
            return;
        }
    }

    /* for WiFi WAN Interface display */
    cgiWifiWanCfgView(fs);
}

void cgiGetAvailableL2WlIntf(int argc __attribute__((unused)), char **argv __attribute__((unused)), char *varValue)
{
    NameList *nl, *ifList = NULL;
    int first = 1;

    varValue[0] = '\0';
    if (dalWifiWan_getAvailableL2WlIntf(&ifList) != CMSRET_SUCCESS || ifList == NULL) {
        return;
    }

    nl = ifList;
    while (nl != NULL) {
        if (!first)
            strcat(varValue,"|");
        else
            first = 0;
        strcat(varValue, nl->name);
        nl = nl->next;
    }
    cmsDal_freeNameList(ifList);
}
#endif /* SUPPORT_WIFIWAN */


