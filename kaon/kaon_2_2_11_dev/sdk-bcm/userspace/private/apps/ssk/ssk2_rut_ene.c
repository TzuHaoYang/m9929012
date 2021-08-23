/* 
* <:copyright-BRCM:2018:proprietary:standard
* 
*    Copyright (c) 2018 Broadcom 
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



/*
 * ssk2_rut_ene.c
 *
 *  Created on:  Aug.20, 2018
 *  Author: Fuguo Xu <fuguo.xu@broadcom.com>
 */


/*
 * This file deals with EE(2nd,3rd... tr69c domain) extended handling for 
 * MULTIPLE_TR69C_SUPPORT case,
 * This is for TR181.
 */



#ifdef DMP_DEVICE2_BASELINE_1

#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "ssk.h"



#ifdef SUPPORT_TR69C

#if (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)
void setAcs2Params_dev2(const char *acsURL, const char *provisioningCode,
                  UINT32 minWaitInterval, UINT32 intervalMultiplier)
{
   E2E_Dev2ManagementServerObject *mgmtObj=NULL;
   InstanceIdStack iidStack=EMPTY_INSTANCE_ID_STACK;
   UBOOL8 doSet=FALSE;
   CmsRet ret;

   /* save to E2E object */
   if (CMSRET_SUCCESS != (ret = cmsObj_get(MDMOID_E2_DEV2_MANAGEMENT_SERVER,
                                    &iidStack, 0, (void **) &mgmtObj)))
   {
      cmsLog_error("Could not get E2_DEV2_MANAGEMENT_SERVER obj, ret=%d", ret);
      return;
   }

   if (cmsUtl_strlen(acsURL) > 0 && cmsUtl_strcmp(mgmtObj->URL, acsURL))
   {
      CMSMEM_REPLACE_STRING(mgmtObj->URL, acsURL);
      doSet = TRUE;
   }

   if (minWaitInterval != 0)
   {
      mgmtObj->CWMPRetryMinimumWaitInterval = minWaitInterval;
      doSet = TRUE;
   }

   if (intervalMultiplier != 0)
   {
      mgmtObj->CWMPRetryIntervalMultiplier = intervalMultiplier;
      doSet = TRUE;
   }

   if (doSet)
   {
      cmsLog_debug("Setting new value(s) in E2E ManagementServer Object");
      if (CMSRET_SUCCESS != (ret = cmsObj_set(mgmtObj, &iidStack)))
      {
         cmsLog_error("Could not set mgmtObj, ret=%d", ret);
      }
   }
   else
   {
      cmsLog_debug("no change in E2E ManagementServerObject, do not set");
   }

   cmsObj_free((void **) &mgmtObj);

   /* now do the same for DeviceInfo.ProvisioningCode */
   if (cmsUtl_strlen(provisioningCode) > 0)
   {
      Dev2DeviceInfoObject *deviceInfoObj=NULL;

      // Technically do not need to init iidStack since both the
      // previous object (MDMOID_DEV2_MANAGEMENT_SERVER) and this object
      // (MDMOID_DEV2_DEVICE_INFO) are Type 0 objects, so iidStack is always
      // empty.  But do it anyways just to show good form.
      INIT_INSTANCE_ID_STACK(&iidStack);
      if (CMSRET_SUCCESS != (ret = cmsObj_get(MDMOID_DEV2_DEVICE_INFO,
                                   &iidStack, 0, (void **) &deviceInfoObj)))
      {
         cmsLog_error("Could not get DEV2_DEVICE_INFO obj, ret=%d", ret);
         return;
      }

      /* save to E2E parameter */
      CMSMEM_REPLACE_STRING(deviceInfoObj->X_BROADCOM_COM_E2E_ProvisioningCode, provisioningCode);

      cmsLog_debug("Setting E2E provisioning code in deviceInfo Object");
      if (CMSRET_SUCCESS != (ret = cmsObj_set(deviceInfoObj, &iidStack)))
      {
         cmsLog_error("Could not set deviceInfoObj, ret=%d", ret);
      }

      cmsObj_free((void **) &deviceInfoObj);
   }

   return;
}
#endif // (DMP_X_BROADCOM_COM_MULTIPLE_TR69C_SUPPORT_1 == 2)

#endif /* SUPPORT_TR69C */




#endif  /* DMP_DEVICE2_BASELINE_1 */

