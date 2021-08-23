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

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "cms_util.h"
#include "cms_phl.h"
#include "cms_lck.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_mgm.h"
#include "os_defs.h"
#include "cms_helper.h"
#include "wlmdm.h"

int get_num_instances(MdmObjectId oid)
{
    static Dev2WifiObject local_wifi_obj = {0x00};
    static UBOOL8 local_obj_cached = FALSE;
    UBOOL8 local_obj_enable = FALSE;    
    Dev2WifiObject *wifi_obj = NULL;
    unsigned int num_instances;


    switch(oid)
    {
        case MDMOID_DEV2_WIFI_RADIO:
        case MDMOID_DEV2_WIFI_SSID:
        case MDMOID_DEV2_WIFI_ACCESS_POINT:
        case MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY:
        case MDMOID_DEV2_WIFI_ACCESS_POINT_WPS:
            local_obj_enable = TRUE;
            break;

#ifdef DMP_DEVICE2_WIFIENDPOINT_1
        /* For Endpoint, it may change in runtime, never get from local copy */
        case MDMOID_DEV2_WIFI_END_POINT:
#endif
        default:
            local_obj_enable = FALSE;
            break;
    }


    if((!local_obj_enable) || (!local_obj_cached))
    {
        CmsRet cret;
        InstanceIdStack iidStack;
        UBOOL8 locked_here = FALSE;

        if (cms_try_lock(&locked_here) != WLMDM_OK)
        {
            return -1;
        }

        INIT_INSTANCE_ID_STACK(&iidStack);

        cret = cmsObj_get(MDMOID_DEV2_WIFI, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&wifi_obj);
        if (cret != CMSRET_SUCCESS)
        {
            cmsLog_error("failed to get WIFI object, ret=%d", cret);
            cms_try_unlock(&locked_here);
            return -1;
        }

        // Backup value we need to local object.
        local_wifi_obj.radioNumberOfEntries       = wifi_obj->radioNumberOfEntries;
        local_wifi_obj.SSIDNumberOfEntries        = wifi_obj->SSIDNumberOfEntries;
        local_wifi_obj.accessPointNumberOfEntries = wifi_obj->accessPointNumberOfEntries;
#ifdef DMP_DEVICE2_WIFIENDPOINT_1    
        /* For Endpoint, it may change in runtime ... */
        local_wifi_obj.endPointNumberOfEntries    = wifi_obj->endPointNumberOfEntries;
#endif
        local_obj_cached = TRUE;
 
        cmsObj_free((void **)&wifi_obj);
        cms_try_unlock(&locked_here);
    }

    wifi_obj = &local_wifi_obj;

    switch(oid)
    {
        case MDMOID_DEV2_WIFI_RADIO:
            num_instances = wifi_obj->radioNumberOfEntries;
            break;

        case MDMOID_DEV2_WIFI_SSID:
            num_instances = wifi_obj->SSIDNumberOfEntries;
            break;

        case MDMOID_DEV2_WIFI_ACCESS_POINT:
        case MDMOID_DEV2_WIFI_ACCESS_POINT_SECURITY:
        case MDMOID_DEV2_WIFI_ACCESS_POINT_WPS:
            num_instances = wifi_obj->accessPointNumberOfEntries;
            break;

#ifdef DMP_DEVICE2_WIFIENDPOINT_1
        case MDMOID_DEV2_WIFI_END_POINT:
            num_instances = wifi_obj->endPointNumberOfEntries;
            break;
#endif

        default:
            num_instances = 0;
            break;
    }

    cmsLog_debug("Got %d instances for oid[%d]", num_instances, oid);

    return num_instances;
}


int get_bssid_num_for_radio(unsigned int radio_index)
{
    static unsigned int local_num_bssid = (-1);
    static int local_radio_index = (-1);

    int num_bssid;
    InstanceIdStack iidStack;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;
    Dev2WifiRadioObject *radio_obj = NULL;

    if((local_radio_index != (-1)) && ((local_radio_index)==(radio_index)))
    {
        return local_num_bssid;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return -1;
    }

    INIT_INSTANCE_ID_STACK(&iidStack);
    PUSH_INSTANCE_ID(&iidStack, radio_index + 1);

    cret = cmsObj_get(MDMOID_DEV2_WIFI_RADIO, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&radio_obj);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get RADIO object instance[%d], ret=%d", radio_index + 1, cret);
        cms_try_unlock(&locked_here);
        return -1;
    }

    num_bssid = radio_obj->X_BROADCOM_COM_WlNumBss;

    /* Backup last one */
    local_radio_index = (int)radio_index;
    local_num_bssid   = num_bssid;
    
    cmsObj_free((void **)&radio_obj);
    cms_try_unlock(&locked_here);

    return num_bssid;
}


/*
 * This function generates nvname using radio_index and bssid_index using iidStack, by following
 * below convention:
 * The iidStack must be of depth 1, as all objects we are concerned about are:
 * 1. Device.Wifi.Radio.{i}.
 * 2. Device.Wifi.SSID.{i}.
 * 3. Device.Wifi.AccessPoint.{i}.
 * 4. Device.Wifi.EndPoint.{i}
 * And their InstanceIdStack have depth 1.
 * We must convert its instance id into below format:
 *----------------------------------------------------------
 *| 1. For non radio properties:       wl%d.%d_confname    |
 *| 2. For radio properties:           wl%d_confname       |
 *| 3. For generic wifi properties:    wl_confname         |
 *----------------------------------------------------------
 *
 * For case 1, we calculate the number of bssid for each radio, and minus it from instance_id:
 * t = instance_id;
 * for each Device.Wifi.Radio.{i}:
 *    num_bssid = Device.Wifi.Radio.{i}.X_BROADCOM_COM_WlNumBss;
 *    if (t + 1 - num_bssid > 0):
 *        t -= num_bssid;
 *    else:
 *        break;
 * radio_index = i;
 * bssid_index = t;
 */
WlmdmRet get_radio_bssid_index(MdmObjectId oid, const InstanceIdStack *iidStack,
                                  int *radio_index, int *bssid_index)
{
    int i, t;
    int num_radio, num_bssid = 0;
    unsigned int instance_id;
    WlmdmRet ret = WLMDM_OK;

    assert(radio_index);
    assert(bssid_index);
    assert(iidStack);

    *radio_index = -1;
    *bssid_index = -1;
    instance_id = PEEK_INSTANCE_ID(iidStack);
    switch(oid)
    {
        case MDMOID_DEV2_WIFI:
        case MDMOID_DEV2_WIFI_BSD_CFG:
        case MDMOID_DEV2_WIFI_WPS_CFG:
        case MDMOID_DEV2_WIFI_NAS_CFG:
        case MDMOID_DEV2_WIFI_DEBUG_MONITOR_CFG:
        case MDMOID_DEV2_WIFI_CEVENTD_CFG:
        case MDMOID_DEV2_WIFI_HAPD_CFG:
        case MDMOID_DEV2_WIFI_SSD_CFG:
        case MDMOID_DEV2_WIFI_WBD_CFG:
            break;

        case MDMOID_DEV2_WIFI_RADIO:
        case MDMOID_DEV2_WIFI_RADIO_ACSD_CFG:
            *radio_index = instance_id - 1;
            break;

        default:
            num_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
            if (num_radio < 0)
            {
                ret = WLMDM_GENERIC_ERROR;
                break;
            }

            t = instance_id;
            for (i = 0; i < num_radio; i++)
            {
                num_bssid = get_bssid_num_for_radio(i);
                if (num_bssid < 0)
                {
                    cmsLog_error("Failed to get number of bssid!\n");
                    return WLMDM_GENERIC_ERROR;
                }

                if (t > num_bssid)
                {
                    t -= num_bssid;
                }
                else
                {
                    break;
                }
            }
            *radio_index = i;
            *bssid_index = t - 1;
    }
    return ret;
}


static UBOOL8 verify_radio_bssid_index(MdmObjectId oid, int radio_index, int bssid_index)
{
    int max_radio, max_bssid;

    max_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
    switch(oid)
    {
        case MDMOID_DEV2_WIFI:
        case MDMOID_DEV2_WIFI_BSD_CFG:
        case MDMOID_DEV2_WIFI_WPS_CFG:
        case MDMOID_DEV2_WIFI_NAS_CFG:
        case MDMOID_DEV2_WIFI_DEBUG_MONITOR_CFG:
        case MDMOID_DEV2_WIFI_CEVENTD_CFG:
        case MDMOID_DEV2_WIFI_HAPD_CFG:
        case MDMOID_DEV2_WIFI_SSD_CFG:
        case MDMOID_DEV2_WIFI_WBD_CFG:
            if ((radio_index < 0) && (bssid_index < 0))
            {
                return TRUE;
            }
            break;

        case MDMOID_DEV2_WIFI_RADIO:
        case MDMOID_DEV2_WIFI_RADIO_ACSD_CFG:
            if ((radio_index >= 0) && (radio_index < max_radio))
            {
                if (bssid_index < 0)
                {
                    return TRUE;
                }
            }
            else
            {
                cmsLog_notice("radio index %d not in range [0, %d)!",
                               radio_index, max_radio);
            }
            break;

        default:
            if ((radio_index >= 0) && (radio_index < max_radio))
            {
                max_bssid = get_bssid_num_for_radio(radio_index);
                if (max_bssid < 0)
                {
                    cmsLog_error("Failed to get bssid num for radio %d!\n", radio_index);
                }
                else
                {
                    /* If bssid_index is -1 and radio_index is in range, we consider it is
                     * a legal case.
                     * For example, wl0_mode => {radio_index:0, bssid_index:-1}
                     * In this case, we will set bssid_index to 0 when getting InstanceIdStack.
                     */
                    if (bssid_index < max_bssid)
                    {
                        return TRUE;
                    }
                    else
                    {
                        cmsLog_notice("bssid index %d exceeds maximum %d for radio %d!",
                                       bssid_index, max_bssid, radio_index);
                    }
                }
            }
            break;
    }
    return FALSE;
}

/*
 * Check for oid and then parsing radio_index and bssid_index into InstanceIdStack.
 * For each oid, the handling logic of radio_index and bssid_index is different.
 * if we are dealing with Device.Wifi, the InstanceIdStack should be just all 0.
 * if we are dealing with Device.Wifi.Radio.{i}, we push radio_index to
 * the InstanceIdStack.
 * If we are dealing with Device.Wifi.SSID.{i}. or other non Radio objects,
 * according to the convention of nvram name, we must do a conversion to fold {radio_index, bssid_index} into the
 * InstanceId of the object.
 * The rule of conversion is as follows:
 *   for (i=0; i<radio_index-1; i++)
 *   {
 *       instance_id += Device.Wifi.Radio.{i}.bssid_num;
 *   }
 *   instance_id += bssid_index;
 * Returns WLMDM_OK if iidStack is calculated.
 * Returns WLMDM_GENERIC_ERROR if the checking of indexes with oid failed.
 *
 */
WlmdmRet get_iidStack(MdmObjectId oid, int radio_index, int bssid_index, InstanceIdStack *iidStack)
{
    WlmdmRet ret = WLMDM_OK;
    int i, t, id;

    assert(iidStack);

    if (FALSE == verify_radio_bssid_index(oid, radio_index, bssid_index))
    {
        return WLMDM_NOT_FOUND;
    }

    INIT_INSTANCE_ID_STACK(iidStack);
    switch (oid)
    {
        case MDMOID_DEV2_WIFI:
        case MDMOID_DEV2_WIFI_BSD_CFG:
        case MDMOID_DEV2_WIFI_WPS_CFG:
        case MDMOID_DEV2_WIFI_NAS_CFG:
        case MDMOID_DEV2_WIFI_DEBUG_MONITOR_CFG:
        case MDMOID_DEV2_WIFI_CEVENTD_CFG:
        case MDMOID_DEV2_WIFI_HAPD_CFG:
        case MDMOID_DEV2_WIFI_SSD_CFG:
        case MDMOID_DEV2_WIFI_WBD_CFG:
            break;

        case MDMOID_DEV2_WIFI_RADIO:
        case MDMOID_DEV2_WIFI_RADIO_ACSD_CFG:
            PUSH_INSTANCE_ID(iidStack, radio_index + 1);
            break;

        default:
           /* If bssid_index is -1 and radio_index is in range, we consider it is
            * a legal case.
            * For example, wl0_mode => {radio_index:0, bssid_index:-1}
            * In this case, we will set bssid_index to 0 when getting InstanceIdStack.
            */
            bssid_index = (bssid_index < 0) ? 0 : bssid_index;
            id = 0;
            for (i = 0; i < radio_index; i++)
            {
                t = get_bssid_num_for_radio(i);
                if (t < 0)
                {
                    cmsLog_error("bssid num for radio %d is less than 0!\n", i);
                    ret = WLMDM_GENERIC_ERROR;
                }
                else
                {
                    cmsLog_debug("%s, %d: radio[%d] has %d bssid\n", __FUNCTION__, __LINE__, i, t);
                    id += t;
                }
            }
            id += bssid_index;
            PUSH_INSTANCE_ID(iidStack, id + 1);
            break;
    }
    return ret;
}

void get_wlnvram_pathDesc(MdmPathDescriptor *pathDesc)
{
    assert(pathDesc);
    pathDesc->oid = MDMOID_DEV2_WIFI;
    INIT_INSTANCE_ID_STACK(&(pathDesc->iidStack));
    strncpy((char *)&pathDesc->paramName, "X_BROADCOM_COM_WlNvram", sizeof(pathDesc->paramName));
}

void get_wlUnsetNvram_pathDesc(MdmPathDescriptor *pathDesc)
{
    assert(pathDesc);
    pathDesc->oid = MDMOID_DEV2_WIFI;
    INIT_INSTANCE_ID_STACK(&(pathDesc->iidStack));
    strncpy((char *)&pathDesc->paramName, "X_BROADCOM_COM_WlUnsetNvram", sizeof(pathDesc->paramName));
}

