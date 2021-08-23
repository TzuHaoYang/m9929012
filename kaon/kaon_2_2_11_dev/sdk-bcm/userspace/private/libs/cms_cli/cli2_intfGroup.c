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


/** cmd driven CLI code goes into this file */

#ifdef DMP_DEVICE2_BASELINE_1

#ifdef SUPPORT_CLI_CMD

#include <time.h>

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "qdm_intf.h"
#include "cli.h"

#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
extern UBOOL8 rutOpenVS_isEnabled(void);
extern UBOOL8 rutOpenVS_isOpenVSPorts(const char *ifName);
#endif

UINT32 cliIntfGroup_getNumberOfGroups_dev2(void)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UINT32 numOfBr = 0;

   while ((ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (!ipIntfObj->X_BROADCOM_COM_Upstream)
      {
         numOfBr++;
      }

      cmsObj_free((void **)&ipIntfObj);
   }

   return numOfBr;
}

void cliIntfGroup_getGroupInfo_dev2(UINT32 brIdx,
             char *groupName, UINT32 groupNameLen,
             char *intfList, UINT32 intfListLen, UINT32 *numIntf,
             char *wanIntfList, UINT32 wanIntfListLen, UINT32 *numWanIntf, char *wanIfName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UINT32 i=0;
   UBOOL8 found=FALSE;

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream)
      {
         /* only count the Downstream IP interfaces, since that is what
          * our numOfBr was doing.
          */
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      if (i != brIdx)
      {
         cmsObj_free((void **) &ipIntfObj);
         i++;
         continue;
      }

      /* this is the (LAN) bridge entry we want, gather info */
      found = TRUE;

      if (ipIntfObj->X_BROADCOM_COM_GroupName)
      {
         cmsUtl_strncpy(groupName, ipIntfObj->X_BROADCOM_COM_GroupName, groupNameLen);
      }

      /* Get the LAN side interfaces associated with this intf group */
      {
         Dev2BridgeObject *brObj=NULL;
         Dev2BridgePortObject *brPortObj=NULL;
         InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
         InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
         UBOOL8 found2=FALSE;

         while (!found2 &&
                (cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                           OGF_NO_VALUE_UPDATE,
                                           (void **) &brObj) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, ipIntfObj->name))
            {
               found2 = TRUE;

               /* loop over all the non-mgmt ports in this bridge */
               while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT,
                                           &brIidStack, &brPortIidStack,
                                           (void **) &brPortObj) == CMSRET_SUCCESS)
               {
                  if (!brPortObj->managementPort &&
                      !qdmIpIntf_isIntfNameUpstreamLocked_dev2(brPortObj->name))
                  {
                     if (intfList[0] != '\0')
                     {
                        cmsUtl_strncat(intfList, intfListLen, "|");
                     }
                     cmsUtl_strncat(intfList, intfListLen, brPortObj->name);
                     (*numIntf)++;
                  }

                  cmsObj_free((void **) &brPortObj);
               }
            }

            cmsObj_free((void **) &brObj);
         }
      }

      /*
       * Find the layer 2 or layer 3 WAN interface associated with this
       * intf group.  Special case for "Default" group: list all WAN
       * interfaces not associated with any other intf group.
       */
      {
         Dev2IpInterfaceObject *wanIpIntfObj = NULL;
         InstanceIdStack wanIidStack = EMPTY_INSTANCE_ID_STACK;

         while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &wanIidStack,
                               (void **)&wanIpIntfObj) == CMSRET_SUCCESS)
         {
            if (wanIpIntfObj->X_BROADCOM_COM_Upstream)
            {
               if (!cmsUtl_strcmp(wanIpIntfObj->X_BROADCOM_COM_BridgeName, ipIntfObj->name) ||
                   (IS_EMPTY_STRING(wanIpIntfObj->X_BROADCOM_COM_BridgeName) &&
                    !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default")))
               {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
                  if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(wanIpIntfObj->name)))
                       ;
                  else
                  {
                     if (wanIntfList[0] != '\0')
                     {
                        strcat(wanIntfList, "|");
                     }
                     cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIpIntfObj->name);
                     sprintf(wanIfName, "%s", wanIpIntfObj->name);
                     (*numWanIntf)++;
                  }
#else
                  if (wanIntfList[0] != '\0')
                  {
                     strcat(wanIntfList, "|");
                  }
                  cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIpIntfObj->name);
                  sprintf(wanIfName, "%s", wanIpIntfObj->name);
                  (*numWanIntf)++;
#endif
               }
            }

            cmsObj_free((void **) &wanIpIntfObj);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   return;
}

void cliIntfGroup_getGroupInfoByName_dev2(const char *groupName,
             UINT32 *brIdx,
             char *intfList, UINT32 intfListLen, UINT32 *numIntf,
             char *wanIntfList, UINT32 wanIntfListLen, UINT32 *numWanIntf, char *wanIfName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UINT32 i=0;
   UBOOL8 found=FALSE;

   if (groupName == NULL || *groupName == '\0')
       return;

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream)
      {
         /* only count the Downstream IP interfaces, since that is what
          * our numOfBr was doing.
          */
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      if (strcmp(groupName, ipIntfObj->X_BROADCOM_COM_GroupName) != 0)
      {
         cmsObj_free((void **) &ipIntfObj);
         i++;
         continue;
      }

      /* this is the (LAN) bridge entry we want, gather info */
      found = TRUE;

      *brIdx = i;

      /* Get the LAN side interfaces associated with this intf group */
      {
         Dev2BridgeObject *brObj=NULL;
         Dev2BridgePortObject *brPortObj=NULL;
         InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
         InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
         UBOOL8 found2=FALSE;

         while (!found2 &&
                (cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                           OGF_NO_VALUE_UPDATE,
                                           (void **) &brObj) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, ipIntfObj->name))
            {
               found2 = TRUE;

               /* loop over all the non-mgmt ports in this bridge */
               while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT,
                                           &brIidStack, &brPortIidStack,
                                           (void **) &brPortObj) == CMSRET_SUCCESS)
               {
                  if (!brPortObj->managementPort &&
                      !qdmIpIntf_isIntfNameUpstreamLocked_dev2(brPortObj->name))
                  {
                     if (intfList[0] != '\0')
                     {
                        cmsUtl_strncat(intfList, intfListLen, "|");
                     }
                     cmsUtl_strncat(intfList, intfListLen, brPortObj->name);
                     (*numIntf)++;
                  }

                  cmsObj_free((void **) &brPortObj);
               }
            }

            cmsObj_free((void **) &brObj);
         }
      }

      /*
       * Find the layer 2 or layer 3 WAN interface associated with this
       * intf group.  Special case for "Default" group: list all WAN
       * interfaces not associated with any other intf group.
       */
      {
         Dev2IpInterfaceObject *wanIpIntfObj = NULL;
         InstanceIdStack wanIidStack = EMPTY_INSTANCE_ID_STACK;

         while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &wanIidStack,
                               (void **)&wanIpIntfObj) == CMSRET_SUCCESS)
         {
            if (wanIpIntfObj->X_BROADCOM_COM_Upstream)
            {
               if (!cmsUtl_strcmp(wanIpIntfObj->X_BROADCOM_COM_BridgeName, ipIntfObj->name) ||
                   (IS_EMPTY_STRING(wanIpIntfObj->X_BROADCOM_COM_BridgeName) &&
                    !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default")))
               {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
                  if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(wanIpIntfObj->name)))
                       ;
                  else
                  {
                     if (wanIntfList[0] != '\0')
                     {
                        strcat(wanIntfList, "|");
                     }
                     cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIpIntfObj->name);
                     sprintf(wanIfName, "%s", wanIpIntfObj->name);
                     (*numWanIntf)++;
                  }
#else
                  if (wanIntfList[0] != '\0')
                  {
                     strcat(wanIntfList, "|");
                  }
                  cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIpIntfObj->name);
                  sprintf(wanIfName, "%s", wanIpIntfObj->name);
                  (*numWanIntf)++;
#endif
               }
            }

            cmsObj_free((void **) &wanIpIntfObj);
         }
      }

      cmsObj_free((void **) &ipIntfObj);
   }

   return;
}

UBOOL8 cliIntfGroup_isAvailableLanInterface_dev2(char *lanIfName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found = FALSE;
   UBOOL8 isAvailableInterface = FALSE;
   char groupName[] = "Default";

   if (!lanIfName || strlen(lanIfName) == 0)
      return FALSE;

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream)
      {
         /* only count the Downstream IP interfaces, since that is what
          * our numOfBr was doing.
          */
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      if (strcmp(groupName, ipIntfObj->X_BROADCOM_COM_GroupName) != 0)
      {
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      /* this is the (LAN) bridge entry we want, gather info */
      found = TRUE;

      /* Get the LAN side interfaces associated with this intf group */
      {
         Dev2BridgeObject *brObj=NULL;
         Dev2BridgePortObject *brPortObj=NULL;
         InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
         InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
         UBOOL8 found2=FALSE;

         while (!found2 &&
                (cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                           OGF_NO_VALUE_UPDATE,
                                           (void **) &brObj) == CMSRET_SUCCESS))
         {
            if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, ipIntfObj->name))
            {
               found2 = TRUE;

               /* loop over all the non-mgmt ports in this bridge */
               while (!isAvailableInterface &&
                      cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT,
                                           &brIidStack, &brPortIidStack,
                                           (void **) &brPortObj) == CMSRET_SUCCESS)
               {
                  if (!brPortObj->managementPort &&
                      !qdmIpIntf_isIntfNameUpstreamLocked_dev2(brPortObj->name))
                  {
                     if (strcmp(lanIfName, brPortObj->name) == 0)
                     {
                        isAvailableInterface = TRUE;
                     }
                  }
                  cmsObj_free((void **) &brPortObj);
               }
            }

            cmsObj_free((void **) &brObj);
         }
      }
      cmsObj_free((void **) &ipIntfObj);
   }

   return isAvailableInterface;
}

UBOOL8 cliIntfGroup_isAvailableWanInterface_dev2(char *wanIfName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found = FALSE;
   UBOOL8 isAvailableInterface = FALSE;
   char groupName[] = "Default";

   if (!wanIfName || strlen(wanIfName) == 0)
      return FALSE;

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &iidStack,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (ipIntfObj->X_BROADCOM_COM_Upstream)
      {
         /* only count the Downstream IP interfaces, since that is what
          * our numOfBr was doing.
          */
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      if (strcmp(groupName, ipIntfObj->X_BROADCOM_COM_GroupName) != 0)
      {
         cmsObj_free((void **) &ipIntfObj);
         continue;
      }

      /*
            * Find the layer 2 or layer 3 WAN interface associated with this
            * intf group.  Special case for "Default" group: list all WAN
            * interfaces not associated with any other intf group.
            */
      {
         Dev2IpInterfaceObject *wanIpIntfObj = NULL;
         InstanceIdStack wanIidStack = EMPTY_INSTANCE_ID_STACK;

         while (cmsObj_getNext(MDMOID_DEV2_IP_INTERFACE, &wanIidStack,
                               (void **)&wanIpIntfObj) == CMSRET_SUCCESS)
         {
            if (wanIpIntfObj->X_BROADCOM_COM_Upstream)
            {
               if (!cmsUtl_strcmp(wanIpIntfObj->X_BROADCOM_COM_BridgeName, ipIntfObj->name) ||
                   (IS_EMPTY_STRING(wanIpIntfObj->X_BROADCOM_COM_BridgeName) &&
                    !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, "Default")))
               {
#ifdef DMP_X_BROADCOM_COM_OPENVSWITCH_1
                  if( (rutOpenVS_isEnabled()) && (rutOpenVS_isOpenVSPorts(wanIpIntfObj->name)))
                       ;
                  else
                  {
                     if (strcmp(wanIfName, wanIpIntfObj->name) == 0)
                     {
                        isAvailableInterface = TRUE;
                        cmsObj_free((void **) &wanIpIntfObj);
                        break;
                     }
                  }
#else
                  if (strcmp(wanIfName, wanIpIntfObj->name) == 0)
                  {
                     isAvailableInterface = TRUE;
                     cmsObj_free((void **) &wanIpIntfObj);
                     break;
                  }
#endif
               }
            }
            cmsObj_free((void **) &wanIpIntfObj);
         }
      }
      cmsObj_free((void **) &ipIntfObj);
   }

   return isAvailableInterface;
}

#endif   /* SUPPORT_CLI_CMD */

#endif   /* DMP_DEVICE2_BASELINE_1 */

