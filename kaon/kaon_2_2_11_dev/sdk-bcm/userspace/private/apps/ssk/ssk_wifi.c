/*
* <:copyright-BRCM:2014:proprietary:standard
*
*    Copyright (c) 2014 Broadcom
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
* :>
*/


#ifdef DMP_X_BROADCOM_COM_WIFILAN_1

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"

#include "ssk.h"

#ifndef BUILD_BRCM_UNFWLCFG

#include "wlcsm_lib_api.h"
#include "rut_wlan.h"

void processAssociatedDeviceUpdated_igd(CmsMsgHeader *msg)
{
    CmsRet ret;
    WL_STALIST_SUMMARIES *sta_summaries =(WL_STALIST_SUMMARIES *)(msg+1);

    if ((ret = cmsLck_acquireAllZoneLocksWithBackoff(0, SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS )
    {
        cmsLog_error("could not get lock, ret=%d", ret);
        cmsLck_dumpInfo();
        return;
    }
    cmsLck_setHoldTimeWarnThresh(SSK_LINK_LOCK_WARNTHRESH);

    if(sta_summaries->type==UPDATE_STA_ALL)
        sta_summaries=wlcsm_wl_get_sta_summary(sta_summaries->radioIdx,sta_summaries->ssidIdx);
    if(sta_summaries) {
        rutWifi_AssociatedDeviceUpdate(sta_summaries);
        if(sta_summaries->type==UPDATE_STA_ALL)
            free(sta_summaries);
    }

    cmsLck_releaseAllZoneLocks();
    return;
}
#else

#include "ssk_wifi.h"

void processAssociatedDeviceUpdated_igd(CmsMsgHeader *msg)
{
    CmsRet ret;
    STAEVT_INFO *event = (STAEVT_INFO *) (msg+1);
    InstanceIdStack *p_lanWlanIidStack, lanWlanIidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack adIidStack = EMPTY_INSTANCE_ID_STACK;
    _LanWlanObject *lanWlanObj=NULL;
    _LanWlanAssociatedDeviceEntryObject *assocDevObj=NULL;
    int found = FALSE;

    if ((ret = cmsLck_acquireAllZoneLocksWithBackoff(0, SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS )
    {
        cmsLog_error("could not get lock, ret=%d", ret);
        cmsLck_dumpInfo();
        return;
    }
    cmsLck_setHoldTimeWarnThresh(SSK_LINK_LOCK_WARNTHRESH);

    // find target lanWlan object.
    while ((ret = cmsObj_getNext(MDMOID_LAN_WLAN, &lanWlanIidStack, (void **)&lanWlanObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcasecmp(event->ifname, lanWlanObj->X_BROADCOM_COM_IfName) == 0) // find matched lanWlan object
        {
            cmsLog_debug("find lanWlan ifname:%s", lanWlanObj->X_BROADCOM_COM_IfName);
            found = TRUE;
            break;
        }
        cmsObj_free((void **) &lanWlanObj);
    }

    if (!found)
        goto exit;

    p_lanWlanIidStack=&lanWlanIidStack;

    // add, update or delete LanWlanAssociatedDeviceEntryObject
    if(event->type == STA_CONNECTED) 
    {
        cmsLog_debug( "To add or update stas\n");
        while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, p_lanWlanIidStack, &adIidStack, (void **)&assocDevObj)) == CMSRET_SUCCESS)
        {
            if (cmsUtl_strcasecmp(event->mac, assocDevObj->associatedDeviceMACAddress) == 0) // find matched associated device object
            {
                cmsLog_debug("find associated device MAC:%s", assocDevObj->associatedDeviceMACAddress);
                break;
            }
            cmsObj_free((void **) &assocDevObj);

        }

        if (assocDevObj)
        {
            // Update existed instance
            cmsLog_debug( "find sta and update it\n");
            assocDevObj->X_BROADCOM_COM_Associated = event->type;
            assocDevObj->X_BROADCOM_COM_Authorized = event->type;
            cmsObj_set(assocDevObj, &adIidStack);
            cmsObj_free((void **) &assocDevObj);
        }
        else
        {
            cmsLog_debug( "could not find sta, need to add a new one");
            memcpy(&adIidStack, p_lanWlanIidStack, sizeof(InstanceIdStack));
            if ((ret = cmsObj_addInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, &adIidStack) ) == CMSRET_SUCCESS)
            {
                cmsObj_get(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, &adIidStack, 0, (void **)&assocDevObj);
                CMSMEM_REPLACE_STRING(assocDevObj->associatedDeviceMACAddress, event->mac);
                assocDevObj->X_BROADCOM_COM_Associated= event->type;
                assocDevObj->X_BROADCOM_COM_Authorized= event->type;
                CMSMEM_REPLACE_STRING(assocDevObj->X_BROADCOM_COM_Ifcname, event->ifname);
                CMSMEM_REPLACE_STRING(assocDevObj->X_BROADCOM_COM_Ssid, lanWlanObj->SSID);
                cmsObj_set(assocDevObj, &adIidStack);
                cmsObj_free((void **) &assocDevObj);
            }
            else
            {
                cmsLog_error("could not add _LanWlanAssociatedDeviceEntryObject instance, ret=%d", ret);
            }
        }
    }
    else if (event->type == STA_DISCONNECTED)
    {
        cmsLog_debug( "To delete stas\n");
        while ((ret = cmsObj_getNextInSubTree(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, p_lanWlanIidStack, &adIidStack, (void **)&assocDevObj)) == CMSRET_SUCCESS)
        {
            if (cmsUtl_strcasecmp(event->mac, assocDevObj->associatedDeviceMACAddress) == 0) // find matched associated device object
            {
                cmsLog_debug("find associated device MAC:%s", assocDevObj->associatedDeviceMACAddress);
                break;
            }
            cmsObj_free((void **) &assocDevObj);

        }

        if (assocDevObj)
        {
            // Update existed instance
            cmsLog_debug( "find sta and delete it\n");
            cmsObj_deleteInstance(MDMOID_LAN_WLAN_ASSOCIATED_DEVICE_ENTRY, &adIidStack);
            cmsObj_free((void **) &assocDevObj);
        }
        else
            cmsLog_debug( "could not find sta");
    }

    cmsObj_free((void **) &lanWlanObj);

exit:
    cmsLck_releaseAllZoneLocks();
    return;
}


#endif /* BUILD_BRCM_UNFWLCFG */
#endif /* DMP_X_BROADCOM_COM_WIFILAN_1*/
