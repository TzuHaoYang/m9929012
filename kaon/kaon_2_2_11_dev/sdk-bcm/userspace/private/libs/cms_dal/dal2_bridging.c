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

#ifdef DMP_DEVICE2_BRIDGE_1

#include "cms_core.h"
#include "qdm_intf.h"
#include "cms_util.h"

#include "cms_dal.h"
#include "dal.h"
#include "dal2_wan.h"
#include "qdm_ipintf.h"




/* in rut2_bridging.c */
extern CmsRet rutBridge_addIntfNameToBridge_dev2(const char *intfName, const char *brIntfName);
extern CmsRet rutBridge_addFullPathToBridge_dev2(const char *fullPath, const char *brIntfName);
extern void rutBridge_deleteIntfNameFromBridge_dev2(const char *intfName);

extern CmsRet rutBridge_addPortToBridge_dev2(const char *llayerFullPath,
                                             const char *brIntfName,
                                             UINT32 tpid,
                                             SINT32 vid,
                                             SINT32 defaultUserPriority,
                                             MdmPathDescriptor *brPortPathDesc);

extern CmsRet rutBridge_movePortToBridge_dev2(const char *brPortName,
                                              const char *llayerFullPath,
                                              const char *brIntfName,
                                              UINT32 tpid,
                                              SINT32 vid,
                                              SINT32 defaultUserPriority,
                                              MdmPathDescriptor *brPortPathDesc);

#ifdef DMP_DEVICE2_VLANBRIDGE_1
extern CmsRet rutBridge_deleteVlanPort_dev2(const InstanceIdStack *brPortIidStack);
#endif

static CmsRet dalBridge_movePortToBridge_dev2(const char *brPortName,
                                              const char *llayerFullPath,
                                              const char *brIntfName,
                                              UINT32 tpid,
                                              SINT32 vid,
                                              SINT32 defaultUserPriority,
                                              MdmPathDescriptor *brPortPathDesc);

static CmsRet dalBridge_findPortByName_dev2(const char *portName,
                                            UBOOL8 *managementPort,
                                            MdmPathDescriptor *brPortPathDesc,
                                            MdmPathDescriptor *llayerPathDesc);

static UBOOL8 dalBridge_isUnlinkLanVlan(const char *ifName);


CmsRet dalBridge_addIntfNameToBridge_dev2(const char *intfName, const char *brIntfName)
{
   return (rutBridge_addIntfNameToBridge_dev2(intfName, brIntfName));
}


CmsRet dalBridge_addFullPathToBridge_dev2(const char *fullPath, const char *brIntfName)
{
   return (rutBridge_addFullPathToBridge_dev2(fullPath, brIntfName));
}


CmsRet dalBridge_addPortToBridge_dev2(const char *llayerFullPath,
                                      const char *brIntfName,
                                      UINT32 tpid,
                                      SINT32 vid,
                                      SINT32 defaultUserPriority,
                                      MdmPathDescriptor *brPortPathDesc)
{
   return (rutBridge_addPortToBridge_dev2(llayerFullPath,
                                          brIntfName,
                                          tpid,
                                          vid,
                                          defaultUserPriority,
                                          brPortPathDesc));
}


/** Move a port from its bridge to another bridge.
 * This function is called by DAL when doing interface grouping.
 *
 * @param brPortName (IN) name of the port to be moved.
 * @param llayerFullPath (IN) interface on which the port is to be moved.
 * @param brIntfName (IN) name of the destination bridge
 * @param tpid (IN) TPID assigned to this bridge port.
 * @param vid (IN) VLAN ID.
 * @param defaultUserPriority (IN) Bridge port default user priority.
 * @param brPortPathDesc (OUT) Path descriptor of the added bridge port object.
 *
 * @return CmsRet
 */
CmsRet dalBridge_movePortToBridge_dev2(const char *brPortName,
                                       const char *llayerFullPath,
                                       const char *brIntfName,
                                       UINT32 tpid,
                                       SINT32 vid,
                                       SINT32 defaultUserPriority,
                                       MdmPathDescriptor *brPortPathDesc)
{
   return (rutBridge_movePortToBridge_dev2(brPortName,
                                           llayerFullPath,
                                           brIntfName,
                                           tpid,
                                           vid,
                                           defaultUserPriority,
                                           brPortPathDesc));
}


#ifdef DMP_DEVICE2_VLANBRIDGE_1
CmsRet dalBridge_deleteVlanPort_dev2(const InstanceIdStack *brPortIidStack)
{
   return rutBridge_deleteVlanPort_dev2(brPortIidStack);
}
#endif

void dalBridge_deleteIntfNameFromBridge_dev2(const char *intfName)
{
   rutBridge_deleteIntfNameFromBridge_dev2(intfName);
}


CmsRet dalBridge_addBridge_dev2(IntfGrpBridgeMode mode, const char *groupName)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   char *brPortFullPath=NULL;
   char ethLinkFullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char ipIntfFullPathBuf[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   MdmPathDescriptor ipIntfPathDesc=EMPTY_PATH_DESCRIPTOR;
   char ipAddrBuf[CMS_IPADDR_LENGTH]={0};
   char minAddrBuf[CMS_IPADDR_LENGTH]={0};
   char maxAddrBuf[CMS_IPADDR_LENGTH]={0};
   char subnetBuf[CMS_IPADDR_LENGTH]={0};
   UINT32 bridgeNum;
   CmsRet ret;
   char brIfName[CMS_IFNAME_LENGTH]={0};

   cmsLog_debug("Entered: intf groupName=%s", groupName);

   dalLan_getBridgeIfNameFromBridgeName_dev2(groupName, brIfName);
   if (!IS_EMPTY_STRING(brIfName))
   {
      cmsLog_notice("Add bridge entry failed. groupName %s already exists", groupName);
      return CMSRET_INVALID_ARGUMENTS;
   }

   /* Add Bridge.{i}. and enable it */
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new Bridge, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_BRIDGE, &iidStack, 0, (void **)&brObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get new Bridge obj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE, &iidStack);
      return ret;
   }

   brObj->enable = TRUE;
   brObj->X_BROADCOM_COM_Mode = mode;

   ret = cmsObj_set(brObj, &iidStack);
   cmsObj_free((void **) &brObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not enable Bridge obj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE, &iidStack);
      return ret;
   }

   /*
    * Once we enable the bridge, the RCL handler will create a brx interface.
    * Get the object again so we know what x is.
    */
   if ((ret = cmsObj_get(MDMOID_DEV2_BRIDGE, &iidStack, 0, (void **)&brObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get new Bridge obj, ret=%d", ret);
      return ret;
   }

   bridgeNum = atoi(&(brObj->X_BROADCOM_COM_IfName[2]));
   cmsLog_debug("bridgeName=%s bridgeNum=%d",
                brObj->X_BROADCOM_COM_IfName, bridgeNum);

   cmsObj_free((void **) &brObj);


   /* Add Bridge.{i}.Port.{i} management port */
   if ((ret = cmsObj_addInstance(MDMOID_DEV2_BRIDGE_PORT, &iidStack)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not create new Bridge Port, ret=%d", ret);
      return ret;
   }

   if ((ret = cmsObj_get(MDMOID_DEV2_BRIDGE_PORT, &iidStack, 0, (void **)&brPortObj)) != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get new Bridge Port obj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &iidStack);
      return ret;
   }

   brPortObj->enable = TRUE;
   brPortObj->managementPort = TRUE;

   ret = cmsObj_set(brPortObj, &iidStack);
   cmsObj_free((void **) &brPortObj);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not enable Bridge obj, ret=%d", ret);
      cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &iidStack);
      return ret;
   }

   /* make fullpath to bridge port, which is the lowerlayers of Eth.Link */
   {
      MdmPathDescriptor brPortPathDesc=EMPTY_PATH_DESCRIPTOR;
      brPortPathDesc.oid = MDMOID_DEV2_BRIDGE_PORT;
      brPortPathDesc.iidStack = iidStack;
      cmsMdm_pathDescriptorToFullPathNoEndDot(&brPortPathDesc, &brPortFullPath);
   }


   /* Add Ethernet.Link which points down to management port */
   ret = dalEth_addEthernetLink_dev2(brPortFullPath,
                              ethLinkFullPathBuf, sizeof(ethLinkFullPathBuf));

   CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);

   /* Add IP.Interface which points down to Ethernet.Link */
   {
      UBOOL8 supportIpv4=TRUE;
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
      UBOOL8 supportIpv6 = TRUE;
#else
      UBOOL8 supportIpv6=FALSE;
#endif

      ret = dalIp_addIntfObject_dev2(supportIpv4,
                                     supportIpv6,
                                     groupName,
                                     FALSE, NULL,  /* isWanBridgeSerivce, bridgeName */
                                     FALSE,  /* firewall XXX TODO: what about LAN side firewall? */
                                     FALSE, FALSE, /* igmp, igmpSource: XXX TODO: what about snooping? */
                                     FALSE, FALSE, /* mld, mldSource */
                                     ethLinkFullPathBuf,
                                     ipIntfFullPathBuf,
                                     sizeof(ipIntfFullPathBuf),
                                     &ipIntfPathDesc);
      if (ret != CMSRET_SUCCESS)
      {
         cmsLog_error("dalIp_addIntfObj_dev2 failed, ret=%d", ret);
      }
   }

   /*
    * Now add some default IPv4 addresses for the newly created bridge, this
    * must be done after the IP.Interface is created.
    * use same hardcoded values and algorithm as rutLan_addBridge
    */
   sprintf(ipAddrBuf, "192.168.%d.1", bridgeNum+1);
   sprintf(subnetBuf, "255.255.255.0");
   dalIp_addIpIntfIpv4Address_dev2(&ipIntfPathDesc.iidStack,
                                   ipAddrBuf, subnetBuf);


   /*
    * Also tell dhcpd to start serving on this subnet.
    */
   sprintf(minAddrBuf, "192.168.%d.2", bridgeNum+1);
   sprintf(maxAddrBuf, "192.168.%d.254", bridgeNum+1);
   dalLan_addDhcpdSubnet_dev2(ipIntfFullPathBuf,
                              ipAddrBuf, minAddrBuf, maxAddrBuf, subnetBuf);

   cmsLog_debug("Exit: ret=%d", ret);

   return ret;
}


void dalBridge_deleteBridge_dev2(const char *groupName)
{
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   char brIntfName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 found=FALSE;
   CmsRet ret;

   cmsLog_debug("Entered: intfGroup=%s", groupName);

   /*
    * First find the right IP.Interface object for the groupName.
    */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, &ipIntfIidStack,
                               OGF_NO_VALUE_UPDATE,
                               (void **)&ipIntfObj)) == CMSRET_SUCCESS)
   {
      if (!ipIntfObj->X_BROADCOM_COM_Upstream &&
          !cmsUtl_strcmp(ipIntfObj->X_BROADCOM_COM_GroupName, groupName))
      {
         found = TRUE;
         strcpy(brIntfName, ipIntfObj->name);
      }

      cmsObj_free((void **)&ipIntfObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find groupName %s", groupName);
      return;
   }


   /* Tell dhcpd not to service this interface anymore */
   {
      char *ipIntfFullPath=NULL;
      MdmPathDescriptor pathDesc=EMPTY_PATH_DESCRIPTOR;
      pathDesc.oid = MDMOID_DEV2_IP_INTERFACE;
      pathDesc.iidStack = ipIntfIidStack;
      cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, &ipIntfFullPath);

      dalLan_deleteDhcpdSubnet_dev2(ipIntfFullPath);
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfFullPath);
   }


   /* delete the IP.Interface obj */
   ret = cmsObj_deleteInstance(MDMOID_DEV2_IP_INTERFACE, &ipIntfIidStack);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Delete of IP.Interface failed, ret=%d", ret);
      /* complain but keep going */
   }

  /* delete Ethernet.Link which points down to management port */
   ret = dalEth_deleteEthernetLinkByName_dev2(brIntfName);

   /*
    * Find the Device.Bridge object matching the linux ifName (e.g. br1).
    * Once we find it, we can delete the entire sub-tree.
    */
   cmsLog_debug("find %s and delete sub-tree", brIntfName);
   found = FALSE;
   {
      InstanceIdStack brIidStack = EMPTY_INSTANCE_ID_STACK;
      Dev2BridgeObject *brObj=NULL;

      while (!found &&
             (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                        OGF_NO_VALUE_UPDATE,
                                        (void **) &brObj) == CMSRET_SUCCESS))
      {
         if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
         {
            found = TRUE;
			
            ret = cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE, &brIidStack);
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("Could not delete Bridge object, r2=%d", ret);
            }
         }
         cmsObj_free((void **) &brObj);
      }
   }

   if (!found)
   {
      cmsLog_error("Could not find Bridge %s to delete", brIntfName);
   }

   cmsLog_debug("Exit:");
   return;
}


CmsRet dalBridge_addFilterDhcpVendorId_dev2(const char *groupName, const char *aggregateString)
{
   cmsLog_debug("Entered: groupName=%s aggreg=%s", groupName, aggregateString);

   /* XXX TODO: when the DHCP vendorId field is added to IP.Interface obj,
     * just add this string to the field.
     *
     */

   return CMSRET_SUCCESS;
}


void dalBridge_deleteFilterDhcpVendorId_dev2(const char *bridgeName)
{
   cmsLog_debug("Entered: bridgeName=%s", bridgeName);

   /* XXX TODO: when the DHCP vendorId field is added to IP.Interface obj,
    * just zero it out.
    * Should i also move all the interfaces that were moved to do DHCP
    * vendor ID?  too much work....
    */

   return;
}


static UBOOL8 getWanIpInterfaceObjByIfName(const char *ifName,
                                           Dev2IpInterfaceObject **ipIntfObj,
                                           InstanceIdStack *iidStack)
{
   UBOOL8 found=FALSE;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_IP_INTERFACE, iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) ipIntfObj) == CMSRET_SUCCESS)
   {
      if ((*ipIntfObj)->X_BROADCOM_COM_Upstream &&
          !cmsUtl_strcmp((*ipIntfObj)->name, ifName))
      {
         found = TRUE;
         /* don't free obj, pass back to caller who is responsible for free */
      }
      else
      {
         cmsObj_free((void **) ipIntfObj);
      }
   }

   return found;
}


CmsRet dalBridge_assocFilterIntfToBridge_dev2(const char *ifName, const char *grpName)
{
   CmsRet ret=CMSRET_SUCCESS;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   char ipIntfLlayer[MDM_SINGLE_FULLPATH_BUFLEN]={0};
   char brIntfName[CMS_IFNAME_LENGTH]={0};
   UBOOL8 isUnlinkLanVlan = FALSE;
   UBOOL8 isBridgeService = FALSE;
   UBOOL8 foundWan=FALSE;

   cmsLog_debug("Entered: ifName=%s intfgroup=%s", ifName, grpName);

   /* convert grpName to brIntfName via QDM */
   ret = qdmIpIntf_getBridgeIntfNameByGroupNameLocked_dev2(grpName, brIntfName);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert groupName %s to intfName", grpName);
      return ret;
   }

   /* Find out whether ifName is a WAN bridge service or not. */
   foundWan = getWanIpInterfaceObjByIfName(ifName, &ipIntfObj, &ipIntfIidStack);
   if (foundWan)
   {
      isBridgeService = ipIntfObj->X_BROADCOM_COM_BridgeService;
      cmsObj_free((void **)&ipIntfObj);
   }
   else
   {
      isUnlinkLanVlan = dalBridge_isUnlinkLanVlan(ifName);
   }

   if (!foundWan || isBridgeService)   
   {
      MdmPathDescriptor brPortPathDesc = EMPTY_PATH_DESCRIPTOR;
      MdmPathDescriptor llayerPathDesc = EMPTY_PATH_DESCRIPTOR;
      UBOOL8 managementPort = FALSE;

      if (!isUnlinkLanVlan)
      {
         ret = dalBridge_findPortByName_dev2(ifName,
                                 &managementPort, &brPortPathDesc, &llayerPathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("dalBridge_findPortByName_dev2 failed. port=%s ret=%d", ifName, ret);
            return ret;
         }

         if (managementPort)
         {
            cmsLog_debug("Cannot move management port %s to bridge %s", ifName, brIntfName);
            return CMSRET_SUCCESS;
         }
      }

      
      if (isUnlinkLanVlan ||
          llayerPathDesc.oid == MDMOID_DEV2_VLAN_TERMINATION ||
          !cmsUtl_strncmp(ifName, WLAN_IFC_STR, strlen(WLAN_IFC_STR)))
      {
         /* This interface is either an unlink LanVlan interface or a bridge
          * port terminated by a VLANTermination object or a WLAN interface.
          * If the interface is not an unlink LanVlan interface, we want to
          * remove it from its current bridge before add to the new bridge.
          */
         if (!isUnlinkLanVlan)
         {
            /* remove this interface from its current bridge */
            dalBridge_deleteIntfNameFromBridge_dev2(ifName);
	      }

         /* Add this interface to its new bridge. */
         ret = dalBridge_addIntfNameToBridge_dev2(ifName, brIntfName);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("AddIntfNameToBridge failed, ret=%d", ret);
            return ret;
         }
      }
      else
      {
         char *llayerFullPath = NULL;
         UINT32 vlanTpid = C_TAG_TPID_STANDARD;
         SINT32 vlanId = -1;
         SINT32 vlanPr = -1;

         /* This interface is not terminated by a VLANTermination object.
          * It may be configured with VLAN Bridge objects.
          */
         ret = qdmVlan_getVlanInfoLocked_dev2(&brPortPathDesc,
                                              &vlanTpid, &vlanId, &vlanPr);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("qdmVlan_getVlanInfoLocked_dev2 failed. ret=%d", ret);
            return ret;
         }

         cmsMdm_pathDescriptorToFullPathNoEndDot(&llayerPathDesc, &llayerFullPath);

         ret = dalBridge_movePortToBridge_dev2(ifName,
                                               llayerFullPath,
                                               brIntfName,
                                               vlanTpid,
                                               vlanId,
                                               vlanPr,
                                               &brPortPathDesc);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed adding bridge port object to %s. ret=%d", brIntfName, ret);
         }
         else if (isBridgeService)
         {
            char *brPortFullPath = NULL;

            /* Get the new bridge port full path. */
            cmsMdm_pathDescriptorToFullPathNoEndDot(&brPortPathDesc, &brPortFullPath);

            /* save brPortFullPath for ip interface lowerlayers */
            cmsUtl_strncpy(ipIntfLlayer, brPortFullPath, sizeof(ipIntfLlayer));
            CMSMEM_FREE_BUF_AND_NULL_PTR(brPortFullPath);
         }

         CMSMEM_FREE_BUF_AND_NULL_PTR(llayerFullPath);
      }
   }

   /* If ifName is a WAN bridge service interface, we need to update
    * the X_BROADCOM_COM_BridgeName as part of this move.
    */
   if (ret == CMSRET_SUCCESS)
   {
      CmsRet r2;

      ipIntfObj = NULL;
      INIT_INSTANCE_ID_STACK(&ipIntfIidStack);
      if (getWanIpInterfaceObjByIfName(ifName, &ipIntfObj, &ipIntfIidStack))
      {
         CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_BridgeName, brIntfName);
         CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_GroupName, grpName);
         if (!IS_EMPTY_STRING(ipIntfLlayer))
         {
            CMSMEM_REPLACE_STRING(ipIntfObj->lowerLayers, ipIntfLlayer);
         }
         r2 = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
         if (r2 != CMSRET_SUCCESS)
         {
            cmsLog_error("Setting new bridgeName failed, r2=%d", r2);
         }
         cmsObj_free((void **) &ipIntfObj);
      }
   }

   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}


CmsRet dalBridge_disassocAllFilterIntfFromBridge_dev2(const char *grpName)
{
   CmsRet ret=CMSRET_SUCCESS;
   char brIntfName[CMS_IFNAME_LENGTH]={0};
   Dev2BridgeObject *brObj=NULL;
   Dev2BridgePortObject *brPortObj=NULL;
   InstanceIdStack brIidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack=EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack brPortIidStack2=EMPTY_INSTANCE_ID_STACK;
   char wanIntfName[CMS_IFNAME_LENGTH]={0};
   char l2Ifname[BUFLEN_64]={0};
   char *p = NULL;
   Dev2IpInterfaceObject *ipIntfObj = NULL;
   InstanceIdStack ipIntfIidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 found=FALSE;

   cmsLog_debug("Entered: grpName=%s", grpName);

   /* convert grpName to brIntfName via QDM */
   ret = qdmIpIntf_getBridgeIntfNameByGroupNameLocked_dev2(grpName, brIntfName);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not convert groupName %s to intfName", grpName);
      return ret;
   }


   /* Find bridge with matching intf name */
   while (!found &&
          (ret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &brIidStack,
                                     OGF_NO_VALUE_UPDATE,
                                     (void **) &brObj) == CMSRET_SUCCESS))
   {
      if (!cmsUtl_strcmp(brObj->X_BROADCOM_COM_IfName, brIntfName))
      {
         found = TRUE;
      }
      /* we don't need the bridge object, so always free it */
      cmsObj_free((void **) &brObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge %s", brIntfName);
      return ret;
   }


   /*
    * Move all the interfaces back to default bridge br0
    */
   while (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_BRIDGE_PORT,
                                       &brIidStack,
                                       &brPortIidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brPortObj) == CMSRET_SUCCESS)
   {
      if (!brPortObj->managementPort)
      {
         cmsLog_debug("Move interface %s to br0", brPortObj->name);

         if (cmsUtl_strcasestr(brPortObj->lowerLayers, "VLANTermination") != NULL)
         {
            /* This port is pointing to a VLANTermination object. We can
             * simply do the following.
             */
            cmsObj_deleteInstance(MDMOID_DEV2_BRIDGE_PORT, &brPortIidStack);

            cmsUtl_strncpy(l2Ifname, brPortObj->name, sizeof(l2Ifname));
            p = strchr(l2Ifname, '.');
            if (p) *p = '\0';

            /* If this is a LAN port (downstream), it must be a LanVlan
             * interface pointing to a VLANTermination object. We don't
             * want to move it back to br0.
             */
            if (qdmIntf_isLayer2IntfNameUpstreamLocked_dev2(l2Ifname))
            {
               ret = dalBridge_addIntfNameToBridge_dev2(brPortObj->name, "br0");
               if (ret != CMSRET_SUCCESS)
               {
                  cmsLog_error("AddIntfNameToBridge failed, ret=%d portName=%s",
                               ret, brPortObj->name);
               }
            }
         }
         else
         {
            ret = dalBridge_assocFilterIntfToBridge_dev2(brPortObj->name, "Default");
            if (ret != CMSRET_SUCCESS)
            {
               cmsLog_error("dalBridge_assocFilterIntfToBridge_dev2 failed, ret=%d", ret);
               cmsObj_free((void **) &brPortObj);
               return ret;
            }
         }
		 
         brPortIidStack = brPortIidStack2;
      }
      else
      {
         brPortIidStack2 = brPortIidStack;
      }
      cmsObj_free((void **) &brPortObj);
   }

   qdmIpIntf_getWanIntfNameByGroupNameLocked_dev2(grpName, wanIntfName);
  
   if (getWanIpInterfaceObjByIfName(wanIntfName, &ipIntfObj, &ipIntfIidStack))
   {
      CmsRet r2;

      cmsLog_debug("cmsObj_set ipIntfObj %s to default group", ipIntfObj->name);
  
      if (ipIntfObj->X_BROADCOM_COM_BridgeService)
      {
         CMSMEM_REPLACE_STRING(ipIntfObj->X_BROADCOM_COM_BridgeName, "br0");
      }
      else
      {
         CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_BridgeName);
      }
  	
      CMSMEM_FREE_BUF_AND_NULL_PTR(ipIntfObj->X_BROADCOM_COM_GroupName);
  	
      r2 = cmsObj_set((void *) ipIntfObj, &ipIntfIidStack);
      if (r2 != CMSRET_SUCCESS)
      {
         cmsLog_error("Set of IP.Interface bridgeName failed, r2=%d", r2);
      }
	  
      cmsObj_free((void **) &ipIntfObj);
   }

   cmsLog_debug("Exit: ret=%d", ret);
   return ret;
}


CmsRet dalBridge_findPortByName_dev2(const char *portName,
                                     UBOOL8 *managementPort,
                                     MdmPathDescriptor *brPortPathDesc,
                                     MdmPathDescriptor *llayerPathDesc)
{
   CmsRet ret = CMSRET_SUCCESS;
   InstanceIdStack brPortIidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2BridgePortObject *brPortObj = NULL;   
   UBOOL8 found = FALSE;

   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE_PORT,
                              &brPortIidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&brPortObj) == CMSRET_SUCCESS)
   {
      if (!cmsUtl_strcmp(brPortObj->name, portName))
      {
         found = TRUE;
         *managementPort = brPortObj->managementPort;
         brPortPathDesc->oid = MDMOID_DEV2_BRIDGE_PORT;
         brPortPathDesc->iidStack = brPortIidStack;
         cmsMdm_fullPathToPathDescriptor(brPortObj->lowerLayers, llayerPathDesc);
      }
      cmsObj_free((void **)&brPortObj);
   }

   if (!found)
   {
      cmsLog_error("Could not find bridge port %s", portName);
      ret = CMSRET_INVALID_ARGUMENTS;
   }

   return ret;
}


UBOOL8 dalBridge_isUnlinkLanVlan(const char *ifName)
{
   CmsRet ret = CMSRET_SUCCESS;
   MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   Dev2InterfaceStackObject *intfStackObj = NULL;
   Dev2VlanTerminationObject *vlanTerminationObj = NULL;
   char lowerLayers[BUFLEN_1024*2]={0};
   UBOOL8 found = FALSE;   

   /* loop thru interface stack to find the list of lower layer vlanTermination
    * fullpaths.
    */
   while (cmsObj_getNextFlags(MDMOID_DEV2_INTERFACE_STACK,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &intfStackObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcasestr(intfStackObj->lowerLayer, "VLANTermination") != NULL)
      {
         if (IS_EMPTY_STRING(lowerLayers))
         {
            cmsUtl_strncat(lowerLayers, BUFLEN_1024*2, intfStackObj->lowerLayer);
         }
         else
         { 
            cmsUtl_strncat(lowerLayers, BUFLEN_1024*2, ",");
            cmsUtl_strncat(lowerLayers, BUFLEN_1024*2, intfStackObj->lowerLayer); 
         }
      }
      cmsObj_free((void **)&intfStackObj);
   }

   /* loop thru interface stack again to find the higher layer vlanTermination
    * fullpath that are not in the lower layer fullpath list.
    */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (!found &&
          cmsObj_getNextFlags(MDMOID_DEV2_INTERFACE_STACK,
                              &iidStack,
                              OGF_NO_VALUE_UPDATE,
                              (void **) &intfStackObj) == CMSRET_SUCCESS)
   {
      if (cmsUtl_strcasestr(intfStackObj->higherLayer, "VLANTermination") != NULL &&
          cmsUtl_strstr(lowerLayers, intfStackObj->higherLayer) == NULL)
      {
         cmsMdm_fullPathToPathDescriptor(intfStackObj->higherLayer, &pathDesc);
         ret = cmsObj_get(MDMOID_DEV2_VLAN_TERMINATION,
                          &pathDesc.iidStack,
                          OGF_NO_VALUE_UPDATE,
                          (void **) &vlanTerminationObj);
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("cmsObj_get failed. ret=%d DEV2_VLAN_TERMINATION iidStack=%s",
                         ret, cmsMdm_dumpIidStack(&pathDesc.iidStack));
         }
         else
         {
            if (!cmsUtl_strcmp(vlanTerminationObj->name, ifName))
            {
               found = TRUE;
            }
            cmsObj_free((void **)&vlanTerminationObj);
         }
      }
      cmsObj_free((void **)&intfStackObj);
   }

   return found;
}

#endif  /* DMP_DEVICE2_BRIDGE_1 */

#endif  /* DMP_DEVICE2_BASELINE_1 */

