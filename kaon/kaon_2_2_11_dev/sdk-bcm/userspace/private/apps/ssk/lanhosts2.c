/***********************************************************************
 *
 *  Copyright (c) 2008  Broadcom Corporation
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

#ifdef DMP_DEVICE2_BASELINE_1


#include "cms.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "qdm_intf.h"
#include "ssk.h"
#include "oal.h"


/** This file contains functions to update the Device.Hosts.Host.{i}. object
 */


#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1


static CmsRet deleteDeviceEntry(const DhcpdHostInfoMsgBody *body)
{
    UBOOL8 found = FALSE;
    Dev2ManagementServerManageableDeviceObject *deviceObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    while (found == FALSE &&
            (ret = cmsObj_getNext(MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,
                                  &iidStack, (void **)&deviceObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(deviceObj->manufacturerOUI, body->oui) == 0 &&
                cmsUtl_strcmp(deviceObj->serialNumber, body->serialNum) == 0 &&
                cmsUtl_strcmp(deviceObj->productClass, body->productClass) == 0)
        {
            found = TRUE;
            cmsLog_debug("Delete manageable device with oui=%s serialNumber=%s productClass=%s",
                         body->oui, body->serialNum, body->productClass);
            cmsObj_deleteInstance(MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE, &iidStack);
        }

        cmsObj_free((void **)&deviceObj);
    }

    if (found == FALSE)
    {
        cmsLog_error("Could not find manageable device with oui=%s serialNumber=%s productClass=%s",
                     body->oui, body->serialNum, body->productClass);
    }

    return ret;
}


static CmsRet addDeviceEntry(const DhcpdHostInfoMsgBody *body, const char *hostFullPath)
{
    UBOOL8 found = FALSE;
    Dev2ManagementServerManageableDeviceObject *deviceObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    while (found == FALSE &&
            (ret = cmsObj_getNextFlags(MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,
                                       &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **)&deviceObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(deviceObj->manufacturerOUI, body->oui) == 0 &&
                cmsUtl_strcmp(deviceObj->serialNumber, body->serialNum) == 0 &&
                cmsUtl_strcmp(deviceObj->productClass, body->productClass) == 0)
        {
            found = TRUE;
        }

        cmsObj_free((void **)&deviceObj);
    }

    /* only add manageable device when it does not exist */
    if (found == FALSE)
    {
        INIT_INSTANCE_ID_STACK(&iidStack);

        if ((ret = cmsObj_addInstance(MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,
                                      &iidStack)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Add instance of manageable device failed ret=%d", ret);
        }
        else
        {
            cmsObj_get(MDMOID_DEV2_MANAGEMENT_SERVER_MANAGEABLE_DEVICE,
                       &iidStack, OGF_NO_VALUE_UPDATE, (void **) &deviceObj);

            CMSMEM_REPLACE_STRING(deviceObj->manufacturerOUI, body->oui);
            CMSMEM_REPLACE_STRING(deviceObj->serialNumber, body->serialNum);
            CMSMEM_REPLACE_STRING(deviceObj->productClass, body->productClass);
            CMSMEM_REPLACE_STRING(deviceObj->host, hostFullPath);

            if ((ret = cmsObj_set(deviceObj, &iidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Set of manageable device failed, ret=%d", ret);
            }

            cmsObj_free((void **) &deviceObj);
        }
    }

    return ret;
}


#endif   /* DMP_DEVICE2_DEVICEASSOCIATION_1 */


#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1


static CmsRet deleteDHCPv4ServerPoolClient
(const DhcpdHostInfoMsgBody *body,
 const InstanceIdStack *iidPool)
{
    UBOOL8 found = FALSE;
    Dev2Dhcpv4ServerPoolClientObject *objClient = NULL;
    InstanceIdStack iidClient = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    while (found == FALSE &&
            cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                         iidPool,
                                         &iidClient,
                                         OGF_NO_VALUE_UPDATE,
                                         (void **) &objClient) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(objClient->chaddr, body->macAddr) == 0)
        {
            found = TRUE;

            ret = cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                                        &iidClient);

            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Could not delete Dev2Dhcpv4ServerPoolClientObject, ret=%d",
                             ret);
            }
        }

        cmsObj_free((void **)&objClient);
    }

    return ret;
}


static CmsRet addServerPoolClientIPv4Address
(const DhcpdHostInfoMsgBody *body,
 const InstanceIdStack *iidClient)
{
    char dateTimeBuf[BUFLEN_64];
    Dev2Dhcpv4ServerPoolClientIPv4AddressObject *objAddr = NULL;
    InstanceIdStack iidAddr = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    memcpy(&iidAddr, iidClient, sizeof(InstanceIdStack));

    // add entry to the server pool client table
    ret = cmsObj_addInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS,
                             &iidAddr);

    if (ret == CMSRET_SUCCESS)
    {
        ret = cmsObj_get(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS,
                         &iidAddr,
                         0,
                         (void **) &objAddr);

        if (ret == CMSRET_SUCCESS)
        {
            // update IP address
            CMSMEM_REPLACE_STRING(objAddr->IPAddress, body->ipAddr);

            // update lease time remaining
            memset(dateTimeBuf, 0, sizeof(dateTimeBuf));
            cmsTms_getXSIDateTime(body->leaseTimeRemaining,
                                  dateTimeBuf,
                                  sizeof(dateTimeBuf));
            CMSMEM_REPLACE_STRING(objAddr->leaseTimeRemaining, dateTimeBuf);

            // update entry information
            ret = cmsObj_set(objAddr, &iidAddr);

            // free entry that is already gotten
            cmsObj_free((void **)&objAddr);

            if (ret != CMSRET_SUCCESS)
            {
                // delete entry if set is failed
                cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT_I_PV4_ADDRESS,
                                      &iidAddr);
                cmsLog_error("Could not set Dev2Dhcpv4ServerPoolClientIPv4AddressObject, ret=%d", ret);
            }
        }
        else
        {
            cmsLog_error("Could not get Dev2Dhcpv4ServerPoolClientIPv4AddressObject, ret=%d", ret);
        }
    }
    else
    {
        cmsLog_error("Could not add Dev2Dhcpv4ServerPoolClientIPv4AddressObject, ret=%d", ret);
    }

    return ret;
}


static CmsRet addDHCPv4ServerPoolClient
(const DhcpdHostInfoMsgBody *body,
 const InstanceIdStack *iidPool)
{
    Dev2Dhcpv4ServerPoolClientObject *objClient = NULL;
    InstanceIdStack iidClient = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    memcpy(&iidClient, iidPool, sizeof(InstanceIdStack));

    // add entry to the server pool client table
    ret = cmsObj_addInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                             &iidClient);

    if (ret == CMSRET_SUCCESS)
    {
        ret = cmsObj_get(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT,
                         &iidClient,
                         0,
                         (void **) &objClient);

        if (ret == CMSRET_SUCCESS)
        {
            // update MAC address
            CMSMEM_REPLACE_STRING(objClient->chaddr, body->macAddr);

            // update active
            objClient->active = TRUE;

            // update entry information
            ret = cmsObj_set(objClient, &iidClient);

            // free entry that is already gotten
            cmsObj_free((void **)&objClient);

            if (ret == CMSRET_SUCCESS)
            {
                // add server pool client IPv4 address
                ret = addServerPoolClientIPv4Address(body, &iidClient);
            }
            else
            {
                // delete entry if set is failed
                cmsObj_deleteInstance(MDMOID_DEV2_DHCPV4_SERVER_POOL_CLIENT, &iidClient);
                cmsLog_error("Could not set Dev2Dhcpv4ServerPoolClientObject, ret=%d", ret);
            }
        }
        else
        {
            cmsLog_error("Could not get Dev2Dhcpv4ServerPoolClientObject, ret=%d", ret);
        }
    }
    else
    {
        cmsLog_error("Could not add Dev2Dhcpv4ServerPoolClientObject, ret=%d", ret);
    }

    return ret;
}


static CmsRet processDHCPv4ServerPool_dev2(const DhcpdHostInfoMsgBody *body)
{
    Dev2Dhcpv4ServerPoolObject *objPool = NULL;
    InstanceIdStack iidPool = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    if (cmsUtl_isValidIpAddress(AF_INET6, body->ipAddr))
    {
        return ret;
    }
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */

    // System has only 1 DHCPV4 server
    ret = cmsObj_getNextFlags(MDMOID_DEV2_DHCPV4_SERVER_POOL,
                              &iidPool,
                              OGF_NO_VALUE_UPDATE,
                              (void **)&objPool);

    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not get Dev2Dhcpv4ServerPoolObject, ret=%d", ret);
        return ret;
    }

    // just need Dhcpv4ServerPool iidStack, so delete its object
    cmsObj_free((void **)&objPool);

    if (body->deleteHost)
    {
        /* this is the delete case */
        cmsLog_debug("Delete existing client entry with macAddr=%s", body->macAddr);
        ret = deleteDHCPv4ServerPoolClient(body, &iidPool);
    }
    else
    {
        /* this is the add case */
        cmsLog_debug("Delete client entry with macAddr=%s if it is found", body->macAddr);
        ret = deleteDHCPv4ServerPoolClient(body, &iidPool);
        cmsLog_debug("Add new client entry with macAddr=%s", body->macAddr);
        ret = addDHCPv4ServerPoolClient(body, &iidPool);
    }

    return ret;
}


#endif   /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */


#ifdef DMP_DEVICE2_HOSTS_2


static CmsRet deleteHostEntry(const DhcpdHostInfoMsgBody *body)
{
    char *layer3Interface = NULL;
    UBOOL8 layer2 = FALSE, found = FALSE;
    Dev2HostObject *hostObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    ret = qdmIntf_intfnameToFullPathLocked_dev2(body->ifName, layer2, &layer3Interface);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not find layer 3 full path name for IP interface %s", body->ifName);
        return CMSRET_INVALID_ARGUMENTS;
    }

     while (found == FALSE &&
            (ret = cmsObj_getNextFlags(MDMOID_DEV2_HOST, &iidStack, OGF_NO_VALUE_UPDATE, (void **)&hostObj)) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(hostObj->layer3Interface, layer3Interface) == 0 &&
                cmsUtl_strcmp(hostObj->physAddress, body->macAddr) == 0)
        {
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
            UBOOL8 foundAddr = FALSE;
            InstanceIdStack iidAddr = EMPTY_INSTANCE_ID_STACK;
            Dev2HostIpv4AddressObject *ipv4AddrObj = NULL;
            InstanceIdStack v4IidStack = EMPTY_INSTANCE_ID_STACK;
            Dev2HostIpv6AddressObject *ipv6AddrObj = NULL;
            InstanceIdStack v6IidStack = EMPTY_INSTANCE_ID_STACK;

            found = TRUE;

            if (cmsUtl_isValidIpv4Address(hostObj->IPAddress))
            {
                while (foundAddr == FALSE &&
                        cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV4_ADDRESS,&iidStack,&v4IidStack,OGF_NO_VALUE_UPDATE,
                                                     (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
                {
                    if (cmsUtl_strcmp(ipv4AddrObj->IPAddress, hostObj->IPAddress) == 0)
                        foundAddr = TRUE;
                    cmsObj_free((void **) &ipv4AddrObj);
                }

                if(foundAddr==FALSE) {
                    /* this is so not gonna happen */
                    cmsLog_error("Could not find Dev2HostIpv4AddressObject for IP address %s", hostObj->IPAddress);
                    break;
                }
                else
                {
                    /* mark for deletion of Dev2HostIpv4AddressObject */
                    iidAddr = v4IidStack;

                    foundAddr = FALSE;
                    INIT_INSTANCE_ID_STACK(&v4IidStack);

                    /* find a Device.Hosts.Host.{i}.IPv4Address.{i}. as a substitute for Device.Hosts.Host.{i}. */
                    while (foundAddr == FALSE &&
                            cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV4_ADDRESS,&iidStack,&v4IidStack,OGF_NO_VALUE_UPDATE,
                                                         (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
                    {
                        if (cmsUtl_strcmp(ipv4AddrObj->IPAddress, hostObj->IPAddress) != 0)
                        {
                            foundAddr = TRUE;
                            hostObj->leaseTimeRemaining = 0;
                            CMSMEM_REPLACE_STRING(hostObj->IPAddress, ipv4AddrObj->IPAddress);
                        }

                        cmsObj_free((void **) &ipv4AddrObj);
                    }

                    if(foundAddr==FALSE) {
                        /* try Device.Hosts.Host.{i}.IPv6Address.{i}. if none of Device.Hosts.Host.{i}.IPv4Address.{i}. is applicable */
                        if (cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV6_ADDRESS,&iidStack,&v6IidStack,OGF_NO_VALUE_UPDATE,
                                                         (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
                        {
                            foundAddr = TRUE;
                            hostObj->leaseTimeRemaining = 0;
                            CMSMEM_REPLACE_STRING(hostObj->hostName, "");
                            CMSMEM_REPLACE_STRING(hostObj->IPAddress, ipv6AddrObj->IPAddress);

                            cmsObj_free((void **) &ipv6AddrObj);
                        }

                        if(foundAddr==FALSE) {
                            /* neither Device.Hosts.Host.{i}.IPv4Address.{i}. nor Device.Hosts.Host.{i}.IPv6Address.{i}. exists */
                            cmsLog_debug("Delete Dev2HostObject for layer3Interface=%s, macAddr=%s",
                                         layer3Interface, body->macAddr);
                            cmsObj_deleteInstance(MDMOID_DEV2_HOST, &iidStack);
                        }
                    }

                    if(foundAddr==TRUE) {
                        if ((ret = cmsObj_set(hostObj, &iidStack)) != CMSRET_SUCCESS)
                        {
                            cmsLog_error("Set of Dev2HostObject failed, ret=%d", ret);
                        }

                        cmsLog_debug("Delete Dev2HostIpv4AddressObject for layer3Interface=%s, macAddr=%s",
                                     layer3Interface, body->macAddr);
                        cmsObj_deleteInstance(MDMOID_DEV2_HOST_IPV4_ADDRESS, &iidAddr);
                    }
                }
            }
            else if (cmsUtl_isValidIpAddress(AF_INET6, hostObj->IPAddress))
            {
                while (foundAddr == FALSE &&
                        cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV6_ADDRESS,&iidStack,&v6IidStack,OGF_NO_VALUE_UPDATE,
                                                     (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
                {
                    if (cmsUtl_strcmp(ipv6AddrObj->IPAddress, hostObj->IPAddress) == 0)
                        foundAddr = TRUE;
                    cmsObj_free((void **) &ipv6AddrObj);
                }

                if(foundAddr==FALSE) {
                    /* this is so not gonna happen */
                    cmsLog_error("Could not find Dev2HostIpv6AddressObject for IP address %s", hostObj->IPAddress);
                    break;
                }
                else
                {
                    /* mark for deletion of Dev2HostIpv6AddressObject */
                    iidAddr = v6IidStack;

                    foundAddr = FALSE;
                    INIT_INSTANCE_ID_STACK(&v6IidStack);

                    /* find a Device.Hosts.Host.{i}.IPv6Address.{i}. as a substitute for Device.Hosts.Host.{i}. */
                    while (foundAddr == FALSE &&
                            cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV6_ADDRESS,&iidStack,&v6IidStack,OGF_NO_VALUE_UPDATE,
                                                         (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
                    {
                        if (cmsUtl_strcmp(ipv6AddrObj->IPAddress, hostObj->IPAddress) != 0)
                        {
                            foundAddr = TRUE;
                            hostObj->leaseTimeRemaining = 0;
                            CMSMEM_REPLACE_STRING(hostObj->hostName, "");
                            CMSMEM_REPLACE_STRING(hostObj->IPAddress, ipv6AddrObj->IPAddress);
                        }

                        cmsObj_free((void **) &ipv6AddrObj);
                    }

                    if(foundAddr==FALSE) {
                        /* none of Device.Hosts.Host.{i}.IPv6Address.{i}. is applicable */
                        cmsLog_debug("Delete Dev2HostObject for layer3Interface=%s, macAddr=%s",
                                     layer3Interface, body->macAddr);
                        cmsObj_deleteInstance(MDMOID_DEV2_HOST, &iidStack);
                    }

                    if(foundAddr==TRUE) {
                        if ((ret = cmsObj_set(hostObj, &iidStack)) != CMSRET_SUCCESS)
                        {
                            cmsLog_error("Set of Dev2HostObject failed, ret=%d", ret);
                        }

                        cmsLog_debug("Delete Dev2HostIpv6AddressObject for layer3Interface=%s, macAddr=%s",
                                     layer3Interface, body->macAddr);
                        cmsObj_deleteInstance(MDMOID_DEV2_HOST_IPV6_ADDRESS, &iidAddr);
                    }
                }
            }
#else
            found = TRUE;
            cmsLog_debug("Delete Dev2HostObject for layer3Interface=%s, macAddr=%s",
                         layer3Interface, body->macAddr);
            cmsObj_deleteInstance(MDMOID_DEV2_HOST, &iidStack);
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
        }

        cmsObj_free((void **)&hostObj);
    }

    if (found == FALSE)
    {
        cmsLog_error("Could not find Dev2HostObject for layer 3 full path name %s with MAC address %s",
                     layer3Interface, body->macAddr);
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR(layer3Interface);

    return ret;
}


static inline CmsRet _updateHostEntry(const DhcpdHostInfoMsgBody *body, char **hostFullPath,
                                      Dev2HostObject* hostObj,InstanceIdStack *iidStack)
{
    char *layer1Interface = NULL;
    char *layer3Interface = NULL;
    UBOOL8 layer2 = FALSE;
    UBOOL8 found = FALSE;
    Dev2HostIpv4AddressObject *ipv4AddrObj = NULL;
    InstanceIdStack v4IidStack = EMPTY_INSTANCE_ID_STACK;
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    Dev2HostIpv6AddressObject *ipv6AddrObj = NULL;
    InstanceIdStack v6IidStack = EMPTY_INSTANCE_ID_STACK;
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
    MdmPathDescriptor pathDesc = EMPTY_PATH_DESCRIPTOR;
    UINT8 macNum[MAC_ADDR_LEN]= {0};
    char portName[CMS_IFNAME_LENGTH]= {0};
    CmsRet ret = CMSRET_SUCCESS;


#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    if (cmsUtl_isValidIpAddress(AF_INET6, body->ipAddr) && cmsUtl_isValidIpv4Address(hostObj->IPAddress))
    {
        /* Existing DHCPv4 info remains unchanged even if host got SLAAC or DHCPv6 address too. */
    }
    else
    {
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
    hostObj->leaseTimeRemaining = body->leaseTimeRemaining;
    hostObj->active = TRUE;

    CMSMEM_REPLACE_STRING(hostObj->IPAddress, body->ipAddr);
    CMSMEM_REPLACE_STRING(hostObj->physAddress, body->macAddr);
    CMSMEM_REPLACE_STRING(hostObj->hostName, body->hostName);

    if ((ret = qdmIntf_intfnameToFullPathLocked_dev2(body->ifName,
               layer2, &layer3Interface)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not find layer 3 full path name for IP interface %s", body->ifName);
    }
    else
    {
        CMSMEM_REPLACE_STRING(hostObj->layer3Interface, layer3Interface);
        CMSMEM_FREE_BUF_AND_NULL_PTR(layer3Interface);
    }

    /* TODO: DHCPClient, associatedDevice, addressSource, interfaceType */
    CMSMEM_REPLACE_STRING(hostObj->DHCPClient, "");
    CMSMEM_REPLACE_STRING(hostObj->associatedDevice, "");
    //CMSMEM_REPLACE_STRING(hostObj->addressSource, body->addressSource);
    //CMSMEM_REPLACE_STRING(hostObj->interfaceType, body->interfaceType);

    cmsUtl_macStrToNum(body->macAddr, macNum);
    if (oal_getPortNameFromMacAddr(body->ifName, macNum, portName) == CMSRET_SUCCESS)
    {
#ifdef SUPPORT_LANVLAN
#ifdef BRCM_WLAN
        char *p = strchr(portName, '.');
        /* don't do this for wireless interface */
        if ( p != NULL  && !strstr(portName,WLAN_IFC_STR))  // strip the wl1.x to wl1
            *p='\0';
#endif /* BRCM_WLAN */
#endif
        layer2 = TRUE;
        if (qdmIntf_intfnameToFullPathLocked_dev2(portName, layer2, &layer1Interface) == CMSRET_SUCCESS)
        {
            CMSMEM_REPLACE_STRING(hostObj->layer1Interface, layer1Interface);
            CMSMEM_FREE_BUF_AND_NULL_PTR(layer1Interface);
        }
        else
        {
            CMSMEM_REPLACE_STRING(hostObj->layer1Interface, "");
            cmsLog_notice("Could not find layer 2 full path name for IP interface %s", portName);
        }
    }
    else
    {
        cmsLog_error("oal_getPortnameFromMacAddr failed, ret=%d", ret);
    }

    if ((ret = cmsObj_set(hostObj, iidStack)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Set of Dev2HostObject failed, ret=%d", ret);
    }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    }

    if (cmsUtl_isValidIpv4Address(body->ipAddr))
    {
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */
    while (found == FALSE &&
            cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV4_ADDRESS,iidStack,&v4IidStack,OGF_NO_VALUE_UPDATE,
                                         (void **) &ipv4AddrObj) == CMSRET_SUCCESS)
    {
        if (cmsUtl_strcmp(ipv4AddrObj->IPAddress, hostObj->IPAddress) == 0)
            found = TRUE;
        cmsObj_free((void **) &ipv4AddrObj);
    }

    if(found==FALSE) {
        v4IidStack = *iidStack;
        if ((ret = cmsObj_addInstance(MDMOID_DEV2_HOST_IPV4_ADDRESS, &v4IidStack)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Add instance of Dev2HostIpv4AddressObject failed ret=%d", ret);
        }
        else
        {

            cmsObj_get(MDMOID_DEV2_HOST_IPV4_ADDRESS, &v4IidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv4AddrObj);

            CMSMEM_REPLACE_STRING(ipv4AddrObj->IPAddress, hostObj->IPAddress);

            if ((ret = cmsObj_set(ipv4AddrObj, &v4IidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Set of Dev2HostIpv4AddressObject failed, ret=%d", ret);
            }

            cmsObj_free((void **) &ipv4AddrObj);
        }
    }
#ifdef DMP_X_BROADCOM_COM_DEV2_IPV6_1
    }
    else if (cmsUtl_isValidIpAddress(AF_INET6, body->ipAddr))
    {
        while (found == FALSE &&
                cmsObj_getNextInSubTreeFlags(MDMOID_DEV2_HOST_IPV6_ADDRESS,iidStack,&v6IidStack,OGF_NO_VALUE_UPDATE,
                                             (void **) &ipv6AddrObj) == CMSRET_SUCCESS)
        {
            if (cmsUtl_strcmp(ipv6AddrObj->IPAddress, body->ipAddr) == 0)
                found = TRUE;
            cmsObj_free((void **) &ipv6AddrObj);
        }

        if(found==FALSE) {
            v6IidStack = *iidStack;
            if ((ret = cmsObj_addInstance(MDMOID_DEV2_HOST_IPV6_ADDRESS, &v6IidStack)) != CMSRET_SUCCESS)
            {
                cmsLog_error("Add instance of Dev2HostIpv6AddressObject failed ret=%d", ret);
            }
            else
            {

                cmsObj_get(MDMOID_DEV2_HOST_IPV6_ADDRESS, &v6IidStack, OGF_NO_VALUE_UPDATE, (void **) &ipv6AddrObj);

                CMSMEM_REPLACE_STRING(ipv6AddrObj->IPAddress, body->ipAddr);

                if ((ret = cmsObj_set(ipv6AddrObj, &v6IidStack)) != CMSRET_SUCCESS)
                {
                    cmsLog_error("Set of Dev2HostIpv6AddressObject failed, ret=%d", ret);
                }

                cmsObj_free((void **) &ipv6AddrObj);
            }
        }
    }
#endif   /* DMP_X_BROADCOM_COM_DEV2_IPV6_1 */


    /* generate full path for this host entry */
    pathDesc.oid = MDMOID_DEV2_HOST;
    pathDesc.iidStack = *iidStack;
    if ((ret = cmsMdm_pathDescriptorToFullPathNoEndDot(&pathDesc, hostFullPath)) != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsMdm_fullPathToPathDescriptor returns error. ret=%d", ret);
    }

    return ret;
}


static CmsRet updateHostEntry(const DhcpdHostInfoMsgBody *body, char **hostFullPath)
{
    UBOOL8 found = FALSE;
    Dev2HostObject *hostObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    while (found == FALSE &&
            (ret = cmsObj_getNextFlags(MDMOID_DEV2_HOST, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **)&hostObj)) == CMSRET_SUCCESS)
    {
        if ( cmsUtl_strcmp(hostObj->physAddress, body->macAddr) == 0) {
            found = TRUE;
            ret=_updateHostEntry(body,hostFullPath,hostObj,&iidStack);
        }
        cmsObj_free((void **)&hostObj);
    }

    if (found == FALSE) {
        cmsLog_debug("Could not find Dev2HostObject with MAC address %s", body->macAddr);
        ret = CMSRET_OBJECT_NOT_FOUND;
    }

    return ret;
}


static CmsRet addHostEntry(const DhcpdHostInfoMsgBody *body, char **hostFullPath)
{

    Dev2HostObject *hostObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    CmsRet ret = CMSRET_SUCCESS;

    if ((ret = cmsObj_addInstance(MDMOID_DEV2_HOST, &iidStack)) != CMSRET_SUCCESS) {
        cmsLog_error("Add instance of Dev2HostObject failed ret=%d", ret);
    } else
    {
        cmsObj_get(MDMOID_DEV2_HOST, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &hostObj);
        if(hostObj) {
            ret=_updateHostEntry(body,hostFullPath,hostObj,&iidStack);
            cmsObj_free((void **) &hostObj);
        }
    }

    return ret;
}


void processLanHostInfoMsg_dev2(CmsMsgHeader *msg)
{
    DhcpdHostInfoMsgBody *body = (DhcpdHostInfoMsgBody *) (msg + 1);
    CmsRet ret = CMSRET_SUCCESS;

    if (msg->dataLength != sizeof(DhcpdHostInfoMsgBody))
    {
        cmsLog_error("bad data length, got %d expected %d, drop msg",
                     msg->dataLength, sizeof(DhcpdHostInfoMsgBody));
        return;
    }

    cmsLog_debug("Entered: delete=%d ifName=%s ipAddr=%s macAddr=%s interfaceType=%s remaining=%d",
                 body->deleteHost, body->ifName, body->ipAddr, body->macAddr, body->interfaceType, body->leaseTimeRemaining);
    cmsLog_debug("oui=%s serialNum=%s productClass=%s",
                 body->oui,body->serialNum,body->productClass);

    if ((ret = cmsLck_acquireAllZoneLocksWithBackoff(0, SSK_LOCK_TIMEOUT)) != CMSRET_SUCCESS)
    {
        cmsLog_error("failed to get lock, ret=%d, drop message", ret);
        return;
    }
    cmsLck_setHoldTimeWarnThresh(SSK_LINK_LOCK_WARNTHRESH);

    if (body->deleteHost)
    {
        /* this is the delete case */
        cmsLog_debug("Delete existing host entry with macAddr=%s", body->macAddr);
        deleteHostEntry(body);

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
        if (strlen(body->oui) > 0)
        {
            cmsLog_debug("Delete existing manageable device with oui=%s", body->oui);
            deleteDeviceEntry(body);
        }
#endif   /* DMP_DEVICE2_DEVICEASSOCIATION_1 */
    } /* deleteHost */
    else
    {
        char *hostFullPath = NULL;

        /* this is edit case, only a few fields can change */
        cmsLog_debug("Update existing host entry with macAddr=%s", body->macAddr);
        ret = updateHostEntry(body, &hostFullPath);

        if (ret == CMSRET_OBJECT_NOT_FOUND)
        {
            /* this is add case, object should be created and all fields are changed */
            cmsLog_debug("Add new host entry with macAddr=%s", body->macAddr);
            ret = addHostEntry(body, &hostFullPath);
        }

#ifdef DMP_DEVICE2_DEVICEASSOCIATION_1
        if (strlen(body->oui) > 0)
        {
            cmsLog_debug("Add new manageable device with oui=%s", body->oui);
            addDeviceEntry(body, hostFullPath);
        }
#endif   /* DMP_DEVICE2_DEVICEASSOCIATION_1 */

        CMSMEM_FREE_BUF_AND_NULL_PTR(hostFullPath);
    } /* add or edit */

#ifdef DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1
    processDHCPv4ServerPool_dev2(body);
#endif   /* DMP_DEVICE2_DHCPV4SERVERCLIENTINFO_1 */

#ifdef DMP_DEVICE2_DHCPV6SERVERCLIENTINFO_1
    /* TODO: processDHCPv6ServerPool_dev2() */
#endif   /* DMP_DEVICE2_DHCPV6SERVERCLIENTINFO_1 */

    cmsLck_releaseAllZoneLocks();
    return;
}


#endif   /* DMP_DEVICE2_HOSTS_2 */

#endif   /* DMP_DEVICE2_BASELINE_1 */


