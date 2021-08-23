/***********************************************************************
 *
 *  Copyright (c) 2013  Broadcom Corporation
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
 
#ifdef DMP_DEVICE2_BASELINE_1

#ifdef DMP_DEVICE2_VLANTERMINATION_1

#include "cms.h"
#include "cms_mdm.h"
#include "cms_obj.h"
#include "cms_phl.h"
#include "cms_qdm.h"
#include "cms_util.h"
#include "cms_core.h"


#ifdef SUPPORT_LANVLAN

UBOOL8 qdmVlan_isLanVlanPresentLocked_dev2()
{
   cmsLog_notice("Dev2 LAN VLAN is not implemented yet, return FALSE");

   return FALSE;
}

#endif  /* SUPPORT_LANVLAN */

CmsRet qdmVlan_getVlanTermInfoByIntfNameLocked_dev2(const char *intfName,
                                                    SINT32 *vlanId,
                                                    SINT32 *vlan801p,
                                                    UINT32 *vlanTPID)
{
   MdmPathDescriptor pathDesc;
   char *ipIntfFullPath=NULL;
   char higherLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char lowerLayerBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: intfName=%s", intfName);

   ret = qdmIntf_intfnameToFullPathLocked(intfName, FALSE, &ipIntfFullPath);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get fullpath for %s, ret=%d", intfName, ret);
      return ret;
   }

   cmsUtl_strncpy(higherLayerBuf, ipIntfFullPath, sizeof(higherLayerBuf));
   CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);

   while (!found)
   {
      ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(higherLayerBuf,
                                                              lowerLayerBuf,
                                                       sizeof(lowerLayerBuf));

      if ((ret != CMSRET_SUCCESS) || (cmsUtl_strlen(lowerLayerBuf) == 0))
      {
         cmsLog_error("Hit bottom or error on %s", higherLayerBuf);
         return ret;
      }
      else
      {
         cmsLog_debug("higher %s ==> lower %s", higherLayerBuf, lowerLayerBuf);
      }

      /* convert fullpath to pathDesc */
      memset((void *) &pathDesc, 0, sizeof(pathDesc));
      ret = cmsMdm_fullPathToPathDescriptor(lowerLayerBuf, &pathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("Could not convert fullpath %s", lowerLayerBuf);
         return ret;
      }

      if (pathDesc.oid == MDMOID_DEV2_VLAN_TERMINATION)
      {
         Dev2VlanTerminationObject *vlanTermObj=NULL;

         found = TRUE;

         ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, 0, (void **)&vlanTermObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Get of vlanTerm obj at %s failed, ret=%d",
                  cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
         }
         else
         {
            *vlanId = vlanTermObj->VLANID;
            *vlan801p = vlanTermObj->X_BROADCOM_COM_Vlan8021p;
            *vlanTPID = vlanTermObj->TPID;
            cmsLog_debug("Got vlanid %d, vlan801p %d, vlanTPID %d on %s", *vlanId, *vlan801p, *vlanTPID, lowerLayerBuf);
            cmsObj_free((void **)&vlanTermObj);
         }
      }
      else
      {
         /* go down to the next layer of the intf stack */
         cmsUtl_strncpy(higherLayerBuf, lowerLayerBuf, sizeof(higherLayerBuf));
      }
   }

   cmsLog_debug("Exit ret %d", ret);
   
   return ret;
   
}




SINT32 qdmVlan_getVlanIdByIntfNameLocked_dev2(const char *intfName)
{
   SINT32 vlanId=-1;
   SINT32 vlan801p = -1;
   UINT32 vlanTPID = -1;
   
   cmsLog_debug("Entered: intfName=%s", intfName);
   qdmVlan_getVlanTermInfoByIntfNameLocked_dev2(intfName,  &vlanId, &vlan801p, &vlanTPID);
   
   return vlanId;
}


SINT32 qdmVlan_getVlan801pByIntfNameLocked_dev2(const char *intfName)
{
   SINT32 vlanId=-1;
   SINT32 vlan801p = -1;
   UINT32 vlanTPID = -1;
   
   cmsLog_debug("Entered: intfName=%s", intfName);
   qdmVlan_getVlanTermInfoByIntfNameLocked_dev2(intfName, &vlanId, &vlan801p, &vlanTPID);
   
   return vlan801p;
}

UINT32 qdmVlan_getVlanTPIDByIntfNameLocked_dev2(const char *intfName)
{
   SINT32 vlanId=-1;
   SINT32 vlan801p = -1;
   UINT32 vlanTPID = -1;
   
   cmsLog_debug("Entered: intfName=%s", intfName);
   qdmVlan_getVlanTermInfoByIntfNameLocked_dev2(intfName,  &vlanId, &vlan801p, &vlanTPID);
   
   return vlanTPID;
}


CmsRet qdmVlan_getVlanInfoLocked_dev2(const MdmPathDescriptor *pathDescriptor,
                                      UINT32 *vlanTpid,
                                      SINT32 *vlanId,
                                      SINT32 *vlanPr)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   UBOOL8 found = FALSE;

   /* initialize output values */
   *vlanTpid = C_TAG_TPID_STANDARD;
   *vlanId   = -1;
   *vlanPr   = -1;

   if (pathDescriptor == NULL)
   {
      cmsLog_error("Input parameter pathDescriptor is NULL.");
      return CMSRET_INVALID_ARGUMENTS;
   }

   cmsLog_debug("Entered: oid=%d iidStack=%s", pathDescriptor->oid,
                cmsMdm_dumpIidStack(&pathDescriptor->iidStack));

   pathDesc = *pathDescriptor;
   
   while (!found)
   {
#ifdef DMP_DEVICE2_VLANBRIDGE_1
      if (pathDesc.oid == MDMOID_DEV2_BRIDGE_PORT)
      {
         Dev2BridgePortObject *brPortObj=NULL;

         ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                          (void **)&brPortObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get failed. DEV2_BRIDGE_PORT iidStack=%s ret=%d",
                         cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
            return ret;
         }

         if (brPortObj->managementPort)
         {
            cmsObj_free((void **)&brPortObj);
         }
         else if (cmsUtl_strcasestr(brPortObj->lowerLayers, "VLANTermination") != NULL)
         {
            INIT_PATH_DESCRIPTOR(&pathDesc);
            ret = cmsMdm_fullPathToPathDescriptor(brPortObj->lowerLayers, &pathDesc);
            cmsObj_free((void **)&brPortObj);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("cmsMdm_fullPathToPathDescriptor failed. ret=%d", ret);
               return ret;
            }
         }
         else
         {
            InstanceIdStack brIidStack;
            InstanceIdStack brVlanPortIidStack = EMPTY_INSTANCE_ID_STACK;
            Dev2BridgeVlanPortObject *brVlanPortObj = NULL;
            char *brPortFullPath = NULL;
            UBOOL8 foundVlanPort = FALSE;
            UBOOL8 untagged = TRUE;
            UBOOL8 priorityTagging = FALSE;
            UINT32 tpid = C_TAG_TPID_STANDARD;
            SINT32 vid = -1;
            SINT32 pbits = -1;

            found = TRUE;

            tpid  = brPortObj->TPID;
            pbits = brPortObj->defaultUserPriority;
            priorityTagging = brPortObj->priorityTagging;

            cmsObj_free((void **)&brPortObj);

            cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &brPortFullPath);
   
            /* The parent Bridge object is 1 level above the Port object. */
            brIidStack = pathDesc.iidStack;
            POP_INSTANCE_ID(&brIidStack);

            /* find the associated vlan port */
            while (!foundVlanPort &&
                   cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_VLAN_PORT,
                                  &brIidStack,
                                  &brVlanPortIidStack,
                                  OGF_NO_VALUE_UPDATE,
                                  (void **)&brVlanPortObj) == CMSRET_SUCCESS)
            {
               if (!cmsUtl_strcmp(brVlanPortObj->port, brPortFullPath))
               {
                  foundVlanPort = TRUE;

                  if (!IS_EMPTY_STRING(brVlanPortObj->VLAN))
                  {
                     Dev2BridgeVlanObject *brVlanObj = NULL;

                     INIT_PATH_DESCRIPTOR(&pathDesc);
                     ret = cmsMdm_fullPathToPathDescriptor(brVlanPortObj->VLAN, &pathDesc);
                     if (ret != CMSRET_SUCCESS)
                     {
                        cmsLog_error("fullPathToPathDescriptor failed. path=%s ret=%d",
                                     brVlanPortObj->VLAN, ret);
                     }
                     else
                     {
                        ret = cmsObj_get(MDMOID_DEV2_BRIDGE_VLAN,
                                         &pathDesc.iidStack,
                                         OGF_NO_VALUE_UPDATE,
                                         (void **)&brVlanObj);
                        if (ret != CMSRET_SUCCESS)
                        {
                           cmsLog_error("cmsObj_get failed. DEV2_BRIDGE_VLAN, iidStack=%s",
                                        cmsMdm_dumpIidStack(&pathDesc.iidStack));
                        }
                        else
                        {
                           untagged = brVlanPortObj->untagged;
                           vid = brVlanObj->VLANID;
                           cmsObj_free((void **)&brVlanObj);
                        }
                     }
                  }
               }
               cmsObj_free((void **)&brVlanPortObj);
            }

            CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

            if (untagged)
            {
               if (priorityTagging)
               {
                  /* priority tagging only. */
                  vid = 0;
               }
               else
               {
                  /* untagged */
                  vid = -1;
               }
            }

            *vlanTpid = tpid;
            *vlanId   = vid;
            *vlanPr   = pbits;

            break;   /* out of while (!found) */
         }
      }  /* if (pathDesc.oid == MDMOID_DEV2_BRIDGE_PORT) */
#endif

      if (pathDesc.oid == MDMOID_DEV2_VLAN_TERMINATION)
      {
         Dev2VlanTerminationObject *vlanTermObj = NULL;

         found = TRUE;

         ret = cmsObj_get(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE,
                          (void **)&vlanTermObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get failed. DEV2_VLAN_TERMINATION, iidStack=%s ret=%d",
                         cmsMdm_dumpIidStack(&pathDesc.iidStack), ret);
            return ret;
         }
         else
         {
            *vlanTpid = vlanTermObj->TPID;
            *vlanId   = vlanTermObj->VLANID;
            *vlanPr   = vlanTermObj->X_BROADCOM_COM_Vlan8021p;
            cmsObj_free((void **)&vlanTermObj);
         }
      }
      else
      {
         char *higherLayer = NULL;
         char lowerLayer[MDM_SINGLE_FULLPATH_BUFLEN]={0};

         /* go down to the lower layer */
         cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &higherLayer);

         ret = qdmIntf_getFirstLowerLayerFromFullPathLocked_dev2(higherLayer,
                                                lowerLayer, sizeof(lowerLayer));
         if ((ret != CMSRET_SUCCESS) || IS_EMPTY_STRING(lowerLayer))
         {
            cmsLog_error("Hit bottom or error on %s", higherLayer);
            CMSMEM_FREE_BUF_AND_NULL_PTR(higherLayer);
            return ret;
         }
         
         cmsLog_debug("higher %s ==> lower %s", higherLayer, lowerLayer);
         CMSMEM_FREE_BUF_AND_NULL_PTR(higherLayer);

         cmsMdm_fullPathToPathDescriptor(lowerLayer, &pathDesc);
      }
   }  /* while (!found) */
   
   cmsLog_debug("Got vlanid=%d vlanPr=%d, vlanTpid=0x%x", *vlanId, *vlanPr, *vlanTpid);
   
   return CMSRET_SUCCESS;
}
#endif /* DMP_DEVICE2_VLANTERMINATION_1 */

#endif /* DMP_BASELINE_1 */

