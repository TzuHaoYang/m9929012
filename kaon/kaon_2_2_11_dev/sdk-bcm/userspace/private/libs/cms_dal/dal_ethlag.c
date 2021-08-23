/***********************************************************************
 *
 *  Copyright (c) 2018  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2012:proprietary:standard

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

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "cms_qdm.h"

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_ETHLAG_1

extern CmsRet rutBridge_addLanIntfToBridge_dev2(const char *intfName, const char *brIntfName);

CmsRet dalEthLag_addInterface(const WEB_NTWK_VAR *webVar)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2EthLAGObject *lagObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   //cmsLog_debug("ethLagIfName=%s, webVar->ethIfName1 %s, webVar->ethIfName2 %s  webVar->miimon %d", webVar->ethLagIfName, webVar->ethIfName1, webVar->ethIfName2,  webVar->miimon);

   if ((ret = cmsObj_addInstance(MDMOID_LA_G, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new EthLag object, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_LA_G, &iidStack, 0, (void **) &lagObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Failed to get lagObj, ret = %d", ret);
      cmsObj_deleteInstance(MDMOID_LA_G, &iidStack);
      return ret;
   }

   CMSMEM_REPLACE_STRING(lagObj->X_BROADCOM_COM_EthIfName1, webVar->ethIfName1);
   CMSMEM_REPLACE_STRING(lagObj->X_BROADCOM_COM_EthIfName2, webVar->ethIfName2);
   CMSMEM_REPLACE_STRING(lagObj->X_BROADCOM_COM_Mode, webVar->ethLagMode);
   CMSMEM_REPLACE_STRING(lagObj->X_BROADCOM_COM_LacpRate, webVar->lacp_rate);
   CMSMEM_REPLACE_STRING(lagObj->X_BROADCOM_COM_XmitHashPolicy, webVar->xmitHashPolicy);
   lagObj->X_BROADCOM_COM_Miimon = webVar->miimon;
   lagObj->X_BROADCOM_COM_Upstream = webVar->enbWanEthLag;   
   lagObj->enable = TRUE;
   
   /* set and activate ethLag obj  */
   ret = cmsObj_set(lagObj, &iidStack);   
   cmsObj_free((void **) &lagObj);

   if (ret != CMSRET_SUCCESS)
   {
      CmsRet r2;
      cmsLog_error("Failed to set lagObj, ret = %d, delete the instance", ret);
      if ((r2 = cmsObj_deleteInstance(MDMOID_LA_G, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("could not delete lagObj instance, r2=%d", r2);
      }
   }
   else
   {
      /* for ethlag lan interface, now it has the name, and  add this ethlag interface
      * to the default bridge
      */
      if ((ret = cmsObj_get(MDMOID_LA_G, &iidStack, 0, (void **) &lagObj)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to get lagObj, ret = %d", ret);
         return ret;
      }
      
      if (lagObj->X_BROADCOM_COM_Upstream)
      {
         /* ethLag is a WAN intf and will not be added to bridge */
         cmsLog_debug("WAN ethlag will not be added to bridge");
      }
      else
      {
         char ifNameBuf[CMS_IFNAME_LENGTH]={0};

#ifdef SUPPORT_LANVLAN
            /* for LAN eth port only */
         snprintf(ifNameBuf, sizeof(ifNameBuf), "%s.0", lagObj->name);
#else
         cmsUtl_strncpy(ifNameBuf, lagObj->name, sizeof(ifNameBuf)-1);
#endif  
         if ((ret = rutBridge_addLanIntfToBridge_dev2(ifNameBuf, "br0")) != CMSRET_SUCCESS)
         {
            cmsLog_error("could not addlagObj instance to bridge,ret=%d", ret);
         }
         else
         {
            cmsLog_debug("ethlag obj created successfully at %s.", cmsMdm_dumpIidStack(&iidStack));
         }
      }
      cmsObj_free((void **) &lagObj);
   }

   cmsLog_debug("Exit, ret=%d", ret);
   
   return ret;
}


CmsRet dalEthLag_deleteInterface(const WEB_NTWK_VAR *webVar)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2EthLAGObject *lagObj = NULL;
   CmsRet ret = CMSRET_SUCCESS;
   UBOOL8 found=FALSE;

  cmsLog_debug("Deleting ethLagIfName=%s,", webVar->ethLagIfName);
   while(!found && 
         (ret = cmsObj_getNextFlags(MDMOID_LA_G,
                                    &iidStack, 
                                    OGF_NO_VALUE_UPDATE,
                                    (void **)&lagObj)) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(lagObj->name, webVar->ethLagIfName))
      {
         found = TRUE;

         /*
          * No wan services are allowed to exist on top of this layer 2 intf
          * before we move it back to LAN.
          */
         if (lagObj->X_BROADCOM_COM_Upstream &&
             qdmIpIntf_getNumberOfWanServicesOnLayer2IntfNameLocked_dev2(lagObj->name) != 0)
         {
            cmsLog_error("All WAN services on %s must be removed prior to move", lagObj->name);
            ret = CMSRET_REQUEST_DENIED;;
         }
         else
         {
            cmsObj_deleteInstance(MDMOID_LA_G, &iidStack);
         }
      }

      cmsObj_free((void **)&lagObj);
   }    

   cmsLog_debug("Deleting Exit, ret=%d", ret);
   
   return ret;
}


CmsRet dalEthLag_getInterface(NameList **ifList)
{
    Dev2EthLAGObject *ethLagObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 found=FALSE;

    while (!found &&
           cmsObj_getNextFlags(MDMOID_LA_G, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **) &ethLagObj) == CMSRET_SUCCESS)
    {
       if (ethLagObj->X_BROADCOM_COM_Upstream && ethLagObj->enable)
       {
          found = TRUE;  // only support 1 ethLag WAN intf.  Once we find one, we are done

          if (cmsDal_addNameToNameList(ethLagObj->name, ifList) == NULL)
          {
             cmsDal_freeNameList(*ifList);
             cmsObj_free((void **)&ethLagObj);
             return CMSRET_RESOURCE_EXCEEDED;
          }
       }

       cmsObj_free((void **)&ethLagObj);
    }
    
    return CMSRET_SUCCESS;
}


CmsRet dalEthLag_isEthIntfPartOfEthLag(const char *ethIfName)
{
    Dev2EthLAGObject *ethLagObj=NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 found=FALSE;

    while (!found &&
           cmsObj_getNextFlags(MDMOID_LA_G, &iidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **) &ethLagObj) == CMSRET_SUCCESS)
    {
       if ((!cmsUtl_strcmp(ethLagObj->X_BROADCOM_COM_EthIfName1, ethIfName)) || 
          (!cmsUtl_strcmp(ethLagObj->X_BROADCOM_COM_EthIfName2, ethIfName))) 
       {
          found = TRUE;  
       }
       cmsObj_free((void **)&ethLagObj);
    }
    
    return found;
}

#endif  /* DMP_DEVICE2_ETHLAG_1 */
#endif  /* DMP_DEVICE2_BASELINE_1 */


