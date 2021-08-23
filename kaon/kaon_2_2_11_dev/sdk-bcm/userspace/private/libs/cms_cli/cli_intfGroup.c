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


/*****************************************************************************
*    Description:
*
*      CLI commands for Interface Grouping.
*
*****************************************************************************/

#ifdef SUPPORT_CLI_CMD

/* ---- Include Files ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"

#include "cli.h"


/* ---- Private Constants and Types --------------------------------------- */

#define MAX_OPTS 8


/* ---- Private Function Prototypes --------------------------------------- */


/* ---- Private Variables ------------------------------------------------- */

static const char intfGroupUsage[] = "\nintfgroup -- Interface Grouping Command Line Tool\nUsage:\n";

static const char intfGroupShow[] = 
   "       intfgroup show \n";

static const char intfGroupAddGrp[] = 
   "       intfgroup addgrp --grpname <Group Name> [--wanif <WAN Interface>] [--lanif <LAN Interface 1>|<LAN Interface 2>...] \n\n";

static const char intfGroupDelGrp[] = 
   "       intfgroup delgrp --grpname <Group Name> \n\n";

/* ---- Functions --------------------------------------------------------- */

#ifdef DMP_BASELINE_1
UINT32 cliIntfGroup_getNumberOfGroups_igd(void)
{
   UINT32 numOfBr = 0;
   InstanceIdStack brdIidStack = EMPTY_INSTANCE_ID_STACK;
   L2BridgingEntryObject *pBridgeObj = NULL;
   CmsRet ret;

   while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, &brdIidStack,
                      (void **)&pBridgeObj)) == CMSRET_SUCCESS) {
      numOfBr++;
      cmsObj_free((void **)&pBridgeObj);
   }

   return numOfBr;
}
void cliIntfGroup_getGroupInfo_igd(UINT32 brIdx,
             char *groupName, UINT32 groupNameLen,
             char *intfList, UINT32 intfListLen, UINT32 *numIntf,
             char *wanIntfList, UINT32 wanIntfListLen, UINT32 *numWanIntf, char *wanIfName)
{
   L2BridgingEntryObject          *pBridgeObj = NULL;
   L2BridgingFilterObject         *pBridgeFltObj = NULL;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack fltIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found=FALSE;
   UINT32 i=0;

   while (!found &&
          (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, &iidStack,
                               (void **)&pBridgeObj)) == CMSRET_SUCCESS)
   {
      if (i != brIdx)
      {
         cmsObj_free((void **) &pBridgeObj);
         i++;
         continue;
      }

      /* this is the bridge entry we want, gather info */
      found = TRUE;

      //Group name
      if (pBridgeObj->bridgeName) {
         cmsUtl_strncpy(groupName, pBridgeObj->bridgeName, groupNameLen);
      }

      // Get the interface names associated with this bridge
      INIT_INSTANCE_ID_STACK(&fltIidStack);
      while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &fltIidStack,
                               (void **)&pBridgeFltObj)) == CMSRET_SUCCESS)
      {
         if ((pBridgeFltObj->filterBridgeReference == (SINT32) pBridgeObj->bridgeKey) &&
             (cmsUtl_strcmp(pBridgeFltObj->filterInterface, MDMVS_LANINTERFACES)))
         {
            L2BridgingIntfObject *availIntfObj=NULL;
            InstanceIdStack availIntfIidStack =  EMPTY_INSTANCE_ID_STACK;
            char lanIfName[BUFLEN_32]={0};
            UINT32 key;
            CmsRet r3;

            cmsUtl_strtoul(pBridgeFltObj->filterInterface, NULL, 0, &key);
            if((r3 = dalPMap_getAvailableInterfaceByKey(key, &availIntfIidStack, &availIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not find avail intf for key %u", key);
            }
            else
            {
               /* we only want to list LAN interface in this section.  WAN interfaces will be in a separate drop down selection menu */
               if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_LANINTERFACE))
               {
                  if (intfList[0] != '\0')
                  {
                     strcat(intfList, "|");
                  }
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, lanIfName);
   #ifdef SUPPORT_LANVLAN
                  if (strstr(lanIfName, ETH_IFC_STR))
                  {
			
                     char lanIfName2[BUFLEN_8]={0};
                     snprintf(lanIfName2, sizeof(lanIfName2), ".%d", pBridgeFltObj->X_BROADCOM_COM_VLANIDFilter);
                     cmsUtl_strncat(lanIfName, BUFLEN_32, lanIfName2);
                  }
   #endif
                  cmsUtl_strncat(intfList, intfListLen, lanIfName);
                  (*numIntf)++;
               }
               else if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE))
               {
                  if (wanIntfList[0] != '\0')
                  {
                     strcat(wanIntfList, "|");
                  }
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, wanIfName);
                  cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIfName);
                  (*numWanIntf)++;
               }
               cmsObj_free((void **) &availIntfObj);
            }
         }
         cmsObj_free((void **)&pBridgeFltObj);
      }
      cmsObj_free((void **) &pBridgeObj);
      i++;
   }

   return;
}

void cliIntfGroup_getGroupInfoByName_igd(const char *groupName,
        UINT32 *brIdx,
        char *intfList, UINT32 intfListLen, UINT32 *numIntf,
        char *wanIntfList, UINT32 wanIntfListLen, UINT32 *numWanIntf, char *wanIfName)
{
   L2BridgingEntryObject          *pBridgeObj = NULL;
   L2BridgingFilterObject         *pBridgeFltObj = NULL;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack fltIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found=FALSE;
   UINT32 i=0;

   if (groupName == NULL || *groupName == '\0')
      return;

   while (!found &&
         (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, &iidStack,
                               (void **)&pBridgeObj)) == CMSRET_SUCCESS)
   {
      if (strcmp(pBridgeObj->bridgeName, groupName) != 0)
      {
         cmsObj_free((void **) &pBridgeObj);
         i++;
         continue;
      }

      /* this is the bridge entry we want, gather info */
      found = TRUE;
 
      *brIdx = i;

      // Get the interface names associated with this bridge
      INIT_INSTANCE_ID_STACK(&fltIidStack);
      while ((ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &fltIidStack,
                     (void **)&pBridgeFltObj)) == CMSRET_SUCCESS)
      {
         if ((pBridgeFltObj->filterBridgeReference == (SINT32) pBridgeObj->bridgeKey) &&
                 (cmsUtl_strcmp(pBridgeFltObj->filterInterface, MDMVS_LANINTERFACES)))
         {
            L2BridgingIntfObject *availIntfObj=NULL;
            InstanceIdStack availIntfIidStack =  EMPTY_INSTANCE_ID_STACK;
            char lanIfName[BUFLEN_32]={0};
            UINT32 key;
            CmsRet r3;
 
            cmsUtl_strtoul(pBridgeFltObj->filterInterface, NULL, 0, &key);
            if((r3 = dalPMap_getAvailableInterfaceByKey(key, &availIntfIidStack, &availIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not find avail intf for key %u", key);
            }
            else
            {
               /* we only want to list LAN interface in this section.  WAN interfaces will be in a separate drop down selection menu */
               if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_LANINTERFACE))
               {
                  if (intfList[0] != '\0')
                  {
                    strcat(intfList, "|");
                  }
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, lanIfName);
#ifdef SUPPORT_LANVLAN
                  if (strstr(lanIfName, ETH_IFC_STR))
                  {
                     char lanIfName2[BUFLEN_8]={0};
                     snprintf(lanIfName2, sizeof(lanIfName2), ".%d", pBridgeFltObj->X_BROADCOM_COM_VLANIDFilter);
                     cmsUtl_strncat(lanIfName, BUFLEN_32, lanIfName2);
                  }
#endif
                  cmsUtl_strncat(intfList, intfListLen, lanIfName);
                  (*numIntf)++;
               }
               else if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE))
               {
                  if (wanIntfList[0] != '\0')
                  {
                     strcat(wanIntfList, "|");
                  }
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, wanIfName);
                  cmsUtl_strncat(wanIntfList, wanIntfListLen, wanIfName);
                  (*numWanIntf)++;
               }
               cmsObj_free((void **) &availIntfObj);
            }
         }
           cmsObj_free((void **)&pBridgeFltObj);
      }

      cmsObj_free((void **) &pBridgeObj);
      i++;
   }
   return;
}

UBOOL8 cliIntfGroup_isAvailableLanInterface_igd(char *lanIfName)
{
   L2BridgingEntryObject          *pBridgeObj = NULL;
   L2BridgingFilterObject         *pBridgeFltObj = NULL;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack fltIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found = FALSE;
   UBOOL8 isAvailableInterface = FALSE;
   char groupName[] = "Default";

   if (!lanIfName || strlen(lanIfName) == 0)
      return FALSE;

   while (!found &&
         (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, &iidStack,
                               (void **)&pBridgeObj)) == CMSRET_SUCCESS)
   {
      if (strcmp(pBridgeObj->bridgeName, groupName) != 0)
      {
         cmsObj_free((void **) &pBridgeObj);
         continue;
      }

      /* this is the bridge entry we want, gather info */
      found = TRUE;

      // Get the interface names associated with this bridge
      INIT_INSTANCE_ID_STACK(&fltIidStack);
      while (!isAvailableInterface &&
            (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &fltIidStack,
                     (void **)&pBridgeFltObj)) == CMSRET_SUCCESS)
      {
         if ((pBridgeFltObj->filterBridgeReference == (SINT32) pBridgeObj->bridgeKey) &&
                 (cmsUtl_strcmp(pBridgeFltObj->filterInterface, MDMVS_LANINTERFACES)))
         {
            L2BridgingIntfObject *availIntfObj=NULL;
            InstanceIdStack availIntfIidStack =  EMPTY_INSTANCE_ID_STACK;
            char lanIf[BUFLEN_32]={0};
            UINT32 key;
            CmsRet r3;
 
            cmsUtl_strtoul(pBridgeFltObj->filterInterface, NULL, 0, &key);
            if((r3 = dalPMap_getAvailableInterfaceByKey(key, &availIntfIidStack, &availIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not find avail intf for key %u", key);
            }
            else
            {
               /* we only want to list LAN interface in this section. */
               if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_LANINTERFACE))
               {
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, lanIf);
#ifdef SUPPORT_LANVLAN
                  if (strstr(lanIf, ETH_IFC_STR))
                  {
                     char lanIfName2[BUFLEN_8]={0};
                     snprintf(lanIfName2, sizeof(lanIfName2), ".%d", pBridgeFltObj->X_BROADCOM_COM_VLANIDFilter);
                     cmsUtl_strncat(lanIf, BUFLEN_32, lanIfName2);
                  }
#endif
                  if (strcmp(lanIfName, lanIf) == 0)
                  {
                     isAvailableInterface = TRUE;
                  }
               }
               cmsObj_free((void **) &availIntfObj);
            }
         }
         cmsObj_free((void **)&pBridgeFltObj);
      }
      cmsObj_free((void **) &pBridgeObj);
   }
   return isAvailableInterface;
}

UBOOL8 cliIntfGroup_isAvailableWanInterface_igd(char *wanIfName)
{
   L2BridgingEntryObject          *pBridgeObj = NULL;
   L2BridgingFilterObject         *pBridgeFltObj = NULL;
   InstanceIdStack iidStack    = EMPTY_INSTANCE_ID_STACK;
   InstanceIdStack fltIidStack = EMPTY_INSTANCE_ID_STACK;
   CmsRet ret;
   UBOOL8 found = FALSE;
   UBOOL8 isAvailableInterface = FALSE;
   char groupName[] = "Default";

   if (!wanIfName || strlen(wanIfName) == 0)
      return FALSE;

   while (!found &&
         (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_ENTRY, &iidStack,
                               (void **)&pBridgeObj)) == CMSRET_SUCCESS)
   {
      if (strcmp(pBridgeObj->bridgeName, groupName) != 0)
      {
         cmsObj_free((void **) &pBridgeObj);
         continue;
      }

      /* this is the bridge entry we want, gather info */
      found = TRUE;

      // Get the interface names associated with this bridge
      INIT_INSTANCE_ID_STACK(&fltIidStack);
      while (!isAvailableInterface &&
            (ret = cmsObj_getNext(MDMOID_L2_BRIDGING_FILTER, &fltIidStack,
                     (void **)&pBridgeFltObj)) == CMSRET_SUCCESS)
      {
         if ((pBridgeFltObj->filterBridgeReference == (SINT32) pBridgeObj->bridgeKey) &&
                 (cmsUtl_strcmp(pBridgeFltObj->filterInterface, MDMVS_LANINTERFACES)))
         {
            L2BridgingIntfObject *availIntfObj=NULL;
            InstanceIdStack availIntfIidStack =  EMPTY_INSTANCE_ID_STACK;
            char wanIf[BUFLEN_32]={0};
            UINT32 key;
            CmsRet r3;
 
            cmsUtl_strtoul(pBridgeFltObj->filterInterface, NULL, 0, &key);
            if((r3 = dalPMap_getAvailableInterfaceByKey(key, &availIntfIidStack, &availIntfObj)) != CMSRET_SUCCESS)
            {
               cmsLog_error("could not find avail intf for key %u", key);
            }
            else
            {
               if (!cmsUtl_strcmp(availIntfObj->interfaceType, MDMVS_WANINTERFACE))
               {
                  //attach interface name
                  dalPMap_availableInterfaceReferenceToIfName(availIntfObj->interfaceReference, wanIf);
                  if (strcmp(wanIfName, wanIf) == 0)
                  {
                     isAvailableInterface = TRUE;
                  }
               }
               cmsObj_free((void **) &availIntfObj);
            }
         }
         cmsObj_free((void **)&pBridgeFltObj);
      }
      cmsObj_free((void **) &pBridgeObj);
   }
   return isAvailableInterface;
}
#endif

static void cmd_intfGroupHelp(char *help)
{
   if ((help == NULL) || (strcasecmp(help, "--help") == 0))
   {
      printf("%s%s%s%s", intfGroupUsage, intfGroupShow, intfGroupAddGrp, intfGroupDelGrp);
   }
   else if (strcasecmp(help, "addgrp") == 0)
   {
      printf("%s%s", intfGroupUsage, intfGroupAddGrp);
   }
   else if (strcasecmp(help, "delgrp") == 0)
   {
      printf("%s%s", intfGroupUsage, intfGroupDelGrp);
   }
}

static void cmd_intfGroupShow(void)
{
   char   groupName[BUFLEN_64]={0};
   char   intfList[BUFLEN_512]={0};
   char   wanIntfList[BUFLEN_264]={0};
   char   wanIfName[BUFLEN_32]={0};
   UINT32 numOfIntf;
   UINT32 numOfWanIntf;
   UINT32 numOfBr;
   UINT32 brIdx;

   numOfBr = cliIntfGroup_getNumberOfGroups();   
   cmsLog_debug("numOfBr = %d", numOfBr);

   //print header
   printf("%-32s%-32s%s\n", "Group Name", "WAN Interface", "LAN Interface");

   for (brIdx = 0; brIdx < numOfBr; brIdx++)
   {
      //Initialization for getting interfaces information
      numOfIntf = 0;
      numOfWanIntf = 0;
      memset(groupName, 0, sizeof(groupName));
      memset(intfList, 0, sizeof(intfList));
      memset(wanIntfList, 0, sizeof(wanIntfList));
      memset(wanIfName, 0, sizeof(wanIfName));

      //getting interface group info
      cliIntfGroup_getGroupInfo(brIdx,
                               groupName, sizeof(groupName),
                               intfList, sizeof(intfList), &numOfIntf,
                               wanIntfList, sizeof(wanIntfList), &numOfWanIntf, wanIfName);
      
      cmsLog_debug("intfList=%s", intfList);
      cmsLog_debug("wanIfName=%s", wanIfName);
      cmsLog_debug("wanIntfList=%s", wanIntfList);
      cmsLog_debug("groupName=%s", groupName);

      //group name
      printf("%-32s", groupName);

      //wan interface
      printf("%-32s", wanIntfList);

      //lan interfaces
      printf("%s\n", intfList);
   }

}

static CmsRet cmd_intfGroupAddGrp(const char *groupName, char *wanIntf, char *lanIntf)
{
   CmsRet ret = CMSRET_SUCCESS;

   cmsLog_debug("groupName=%s, wanIntf=%s, lanIntf=%s", groupName, wanIntf, lanIntf);

   // Verify Lan interface
   // lanIntf can be the string which includes one interface or multiple interfaces,
   // For example, "eth0.1" or "eth0.1|wl0"
   if (lanIntf)
   {
      char *pIfName = NULL;
      char *saveptr = NULL;
      char tmp_lanIntf[BUFLEN_1024] = {0};

      cmsUtl_strncpy(tmp_lanIntf, lanIntf, BUFLEN_1024);
      tmp_lanIntf[BUFLEN_1024-1] = '\0';

      pIfName = strtok_r(tmp_lanIntf, "|", &saveptr);

      while (pIfName != NULL)
      {
         if (!cliIntfGroup_isAvailableLanInterface(pIfName))
         {
            cmsLog_error("Lan interface [%s] can't be found in Default group!", pIfName);
            ret = CMSRET_INVALID_ARGUMENTS;
            goto exit;
         }
         pIfName = strtok_r(NULL, "|", &saveptr);
      }
   }

   // Verify wanIntf
   // wanIntf can only be the string which includes one interface 
   if (wanIntf && !cliIntfGroup_isAvailableWanInterface(wanIntf))
   {
      cmsLog_error("Wan interface [%s] can't be found in Default group!", wanIntf);
      ret = CMSRET_INVALID_ARGUMENTS;
      goto exit;
   }

   // Set the L2BridgeEntry object to create the new bridge group.
   ret = dalPMap_addBridge(INTFGRP_BR_HOST_MODE, groupName);
   if (ret != CMSRET_SUCCESS)
   {
       cmsLog_error("Failed to create new bridge group!");
       goto exit;
   }

   // Extract the lan interfaces information.
   if(lanIntf)
   {
      char *pIfName = NULL;
      char *saveptr = NULL;
      char tmp_lanIntf[BUFLEN_1024] = {0};

      cmsUtl_strncpy(tmp_lanIntf, lanIntf, BUFLEN_1024);
      tmp_lanIntf[BUFLEN_1024-1] = '\0';

      pIfName = strtok_r(tmp_lanIntf, "|", &saveptr);

      while (pIfName != NULL)
      {
         cmsLog_debug("associating lan interface %s to bridge %s", pIfName, groupName);

         // associate lan interface to this bridge group
         if ((ret = dalPMap_assocFilterIntfToBridge(pIfName, groupName)) != CMSRET_SUCCESS)
         {
                  cmsLog_error("Configure interface grouping failed. Group name %s cannot set the object", groupName);
                  goto exit;
         }
         pIfName = strtok_r(NULL, "|", &saveptr);
      }
   }

   // associate one wan interface to this bridge group
   if (wanIntf)
   {
      cmsLog_debug("associating wan interface %s to bridge %s", wanIntf, groupName);
      if ((ret = dalPMap_assocFilterIntfToBridge(wanIntf, groupName)) != CMSRET_SUCCESS) 
      {
         cmsLog_error("Configure interface grouping failed. Group name %s cannot set the object", groupName);
         goto exit;   
      }
   }

exit:
   return ret;
}

static CmsRet cmd_intfGroupDelGrp(const char *groupName)
{
   CmsRet ret;

   cmsLog_debug("groupName=%s", groupName);

   if(cmsUtl_strcmp("Default", groupName) == 0)
   {
      /* Default group can not be deleted */
      cmsLog_error("Default group can not be deleted!");
      return CMSRET_REQUEST_DENIED;
   }

   cmsLog_debug("delete all dhcp vendor id filters from bridge %s", groupName);
   dalPMap_deleteFilterDhcpVendorId(groupName);

   cmsLog_debug("disassociate all filter intf from bridge %s", groupName);
   ret = dalPMap_disassocAllFilterIntfFromBridge(groupName); 
   
   cmsLog_debug("delete intf group/bridge %s", groupName);
   dalPMap_deleteBridge(groupName);

   return ret;
}

/*****************************************************************************
*  FUNCTION:  processIntfGroupCmd
*  PURPOSE:   Processes Interface Grouping  CLI commands.
*  PARAMETERS:
*      cmdLine - command string.
*  RETURNS:
*      None.
*  NOTES:
*      None.
*****************************************************************************/
void processIntfGroupCmd(char *cmdLine)
{
   SINT32 argc = 0;
   char *argv[MAX_OPTS] = {NULL};
   char *last = NULL;
   CmsRet ret = CMSRET_SUCCESS;

   cliCmdSaveNeeded = FALSE;

   /* parse the command line and build the argument vector */
   argv[0] = strtok_r(cmdLine, " ", &last);

   if (argv[0] != NULL)
   {
      for (argc = 1; argc < MAX_OPTS; ++argc)
      {
         argv[argc] = strtok_r(NULL, " ", &last);

         if (argv[argc] == NULL)
         {
            break;
         }
      }
   }

   if (argv[0] == NULL)
   {
      cmd_intfGroupHelp(NULL);
   }
   else if (strcasecmp(argv[0], "--help") == 0)
   {
      cmd_intfGroupHelp(argv[0]);
   }
   else if (strcasecmp(argv[0], "show") == 0)
   {
      cmd_intfGroupShow();
   }
   else if (strcasecmp(argv[0], "addgrp") == 0)
   {
      int valid = 0;
      //char *grpName = NULL;
      char *wanIntf = NULL;
      char *lanIntf = NULL;

      if (cmsUtl_strcmp(argv[1], "--grpname") == 0)
      {
         /* argv[2] must be group name */
         if(argv[2] != NULL && strlen(argv[2]) > 0 &&
             strcmp(argv[2], "--wanif") != 0 &&
             strcmp(argv[2], "--lanif") != 0)
         {
            int i;
            valid = 1; /* set valid to 1 fisrt since we got group name */

            for(i = 3; i < argc && argv[i] != NULL; i++)
            {
               if (strcmp(argv[i], "--wanif") == 0)
               {
                  i++;
                  if (argv[i] != NULL && strlen(argv[i]) > 0)
                  {
                     wanIntf = argv[i];
                  }
                  else
                     valid = 0;
               }
               else if (strcmp(argv[i], "--lanif") == 0)
               {
                  i++;
                  if (argv[i] != NULL && strlen(argv[i]) > 0)
                  {
                     lanIntf = argv[i];
                  }
                  else
                     valid = 0;
               }
               else
               {
                  cmsLog_error("unknown argument: %s", argv[i]);
                  valid = 0;
                  break;
               }
            }
         }
      }

      if (valid == 0)
      {
         cmd_intfGroupHelp(argv[0]);
      }
      else
      {
         cmsLog_debug("[%s] [%s] [%s]", argv[2], wanIntf, lanIntf);
         ret = cmd_intfGroupAddGrp(argv[2], wanIntf, lanIntf);

         if (ret == CMSRET_SUCCESS)
         {
            cliCmdSaveNeeded = TRUE;
         }
      }
   }
   else if (strcasecmp(argv[0], "delgrp") == 0)
   {
      int valid = 0;

      if (cmsUtl_strcmp(argv[1], "--grpname") == 0)
      {
         if(argv[2] != NULL && strlen(argv[2]) > 0)
         {
            valid = 1;
            ret = cmd_intfGroupDelGrp(argv[2]);

            if (ret == CMSRET_SUCCESS)
            {
               cliCmdSaveNeeded = TRUE;
            }
         }
      }

      if(valid == 0)
      {
         cmd_intfGroupHelp(argv[0]);
      }
   }
   else
   {
      cmd_intfGroupHelp(NULL);
   }
}

#endif /* SUPPORT_CLI_CMD */
