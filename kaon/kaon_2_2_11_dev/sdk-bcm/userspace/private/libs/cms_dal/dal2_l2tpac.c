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

#include "cms_core.h"
#include "cms_dal.h"
#include "cms_util.h"
#include "dal.h"
#include "dal_wan.h"

#ifdef DMP_X_BROADCOM_COM_L2TPAC_1    /* this file touches TR181 objects */

CmsRet dalL2tpAc_addL2tpAcInterface_dev2(const WEB_NTWK_VAR *webVar)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    Dev2PppInterfaceObject *pppIntf=NULL;
    Dev2PppInterfaceL2tpObject *l2tpIntf=NULL;
    CmsRet ret;
    
    /* Add PPP interface instance*/
    if ((ret = cmsObj_addInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Failed to add PPP intf Instance, ret = %d", ret);
       return ret;
    } 
    
    /* Get PPP interface object */
    if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE, &iidStack, 0, (void **) &pppIntf)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Failed to get pppIntfObj, ret = %d", ret);
       cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
       return ret;
    }
      
    /* Get L2TP interface object */
    if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_L2TP, &iidStack, 0, (void **) &l2tpIntf)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to get l2tpIntf, ret = %d", ret);
        cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
        cmsObj_free((void **) &pppIntf);    
        return ret;
    }
    
    /* Get PPPOE interface object if needed*/
    if (cmsUtl_strlen(webVar->pppServerName) > 0 )
    {
        Dev2PppInterfacePpoeObject *pppoeObj=NULL;
      
        /* Get PPPoE object */
        if ((ret = cmsObj_get(MDMOID_DEV2_PPP_INTERFACE_PPOE, &iidStack, 0, (void **) &pppoeObj)) != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get pppoeObj, ret = %d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
            cmsObj_free((void **) &pppIntf);
            cmsObj_free((void **) &l2tpIntf);    
            return ret;
        }
   
        CMSMEM_REPLACE_STRING(pppoeObj->serviceName, webVar->pppServerName);
        
        /* Set PPPoE object */
         ret = cmsObj_set(pppoeObj,  &iidStack);
         cmsObj_free((void **) &pppoeObj);    
         if (ret != CMSRET_SUCCESS)
         {
            cmsLog_error("Failed to set pppoeObj. ret=%d", ret);
            cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
            cmsObj_free((void **) &pppIntf);
            cmsObj_free((void **) &l2tpIntf);       
            return ret;      
         } 
	}
    
    CMSMEM_REPLACE_STRING(pppIntf->username, webVar->pppUserName);
    CMSMEM_REPLACE_STRING(pppIntf->password, webVar->pppPassword);
    CMSMEM_REPLACE_STRING(pppIntf->authenticationProtocol, cmsUtl_numToPppAuthString(webVar->pppAuthMethod));    
    
    /* get ppp debug flag */
    pppIntf->X_BROADCOM_COM_Enable_Debug = webVar->enblPppDebug;
    
    pppIntf->enable = TRUE;
    
    /* fill in service name */
    CMSMEM_REPLACE_STRING(pppIntf->name, "PPPoL2tpAc");   /* todo. just assign some thing for now */
    
    /* Set and Activate PPP interface */
    if ((ret = cmsObj_set(pppIntf, &iidStack)) != CMSRET_SUCCESS)
    {
       cmsLog_error("Set pppIntf failed , ret = %d", ret);
       cmsObj_free((void **) &pppIntf);
    }
    else
    {
	   cmsLog_debug("cmsObj_set pppIntf ok.");
       cmsObj_free((void **) &pppIntf);

       /* enable the L2TPAC config object and set the tunnel name and Lns Ip Address after
       * WanPPPConn object is created since L2tpd needs ppp username/password info
       */
       CMSMEM_REPLACE_STRING(l2tpIntf->tunnelName, webVar->tunnelName);
       CMSMEM_REPLACE_STRING(l2tpIntf->lnsIpAddress, webVar->lnsIpAddress);
       l2tpIntf->enable = TRUE;
              
       if ((ret = cmsObj_set(l2tpIntf, &iidStack)) != CMSRET_SUCCESS)
       {
          cmsLog_error("Failed to set l2tpIntf object, ret = %d", ret);
          cmsObj_free((void **) &l2tpIntf);
       }
       else
       {         
          cmsLog_debug("Set l2tpIntf ok.");
          cmsObj_free((void **) &l2tpIntf);
       }
	}
   
    if (ret != CMSRET_SUCCESS)
    {
       cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack);
    }
   
    return ret;
}

CmsRet dalL2tpAc_deleteL2tpAcInterface_dev2(const WEB_NTWK_VAR *webVar)
{
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    //Dev2PppInterfaceObject *pppIntf=NULL;
    Dev2PppInterfaceL2tpObject *l2tpIntf=NULL;
    //void *obj = NULL;
    CmsRet ret = CMSRET_SUCCESS;
    
    cmsLog_debug("Deleting %s", webVar->tunnelName);

   if ((ret = dalWan_getWanL2tpAcObject(&iidStack, (void **) &l2tpIntf)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not get L2tpAcIntf object, ret =%d", ret);
   }
   else
   {
      /* Delete L2TP interface */
      CMSMEM_FREE_BUF_AND_NULL_PTR(l2tpIntf->tunnelName);
      CMSMEM_FREE_BUF_AND_NULL_PTR(l2tpIntf->lnsIpAddress);
      CMSMEM_REPLACE_STRING(l2tpIntf->intfStatus, MDMVS_DOWN);
      l2tpIntf->enable = FALSE;
      if ((ret = cmsObj_set(l2tpIntf, &iidStack)) != CMSRET_SUCCESS)
      {
          cmsLog_error("Failed to set l2tpIntf object, ret = %d", ret);
      }
      cmsObj_free((void **) &l2tpIntf);
      
      /* If the tunnel is removed, just delete the ppp interface */
      if ((ret = cmsObj_deleteInstance(MDMOID_DEV2_PPP_INTERFACE, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsLog_error("Failed to delete dev2ppp Object, ret = %d", ret);
      }
   }

   
   return ret;    	
}

#endif /* DMP_X_BROADCOM_COM_L2TPAC_1 */

