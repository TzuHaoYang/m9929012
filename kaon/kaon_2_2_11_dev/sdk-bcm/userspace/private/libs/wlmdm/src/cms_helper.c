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

WlmdmRet set_param_from_pathDesc(const MdmPathDescriptor *pathDesc, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    CmsRet cret;
    PhlGetParamValue_t *get_param = NULL;
    PhlSetParamValue_t set_param;
    UBOOL8 locked_here = FALSE;

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    cret = cmsPhl_getParamValueFlags(pathDesc, OGF_NO_VALUE_UPDATE, &get_param);
    if (cret != CMSRET_SUCCESS)
    {
        char *full_path = NULL;
        cmsMdm_pathDescriptorToFullPath(pathDesc, &full_path);
        cmsLog_error("Failed to get parameter: %s", full_path);
        CMSMEM_FREE_BUF_AND_NULL_PTR(full_path);
        cms_try_unlock(&locked_here);
        return WLMDM_GENERIC_ERROR;
    }

    memcpy(&set_param.pathDesc, pathDesc, sizeof(MdmPathDescriptor));
    set_param.pParamType = strdup(get_param->pParamType);
    set_param.pValue = strdup(value);
    set_param.status = CMSRET_SUCCESS;
    cmsPhl_freeGetParamValueBuf(get_param, 1);

    cret = cmsPhl_setParameterValues(&set_param, 1);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set parameter node: %d", cret);
        ret = WLMDM_GENERIC_ERROR;
    }

    free(set_param.pParamType);
    free(set_param.pValue);
    cms_try_unlock(&locked_here);
    return ret;
}

WlmdmRet get_param_from_pathDesc(const MdmPathDescriptor *pathDesc, char *value, size_t size)
{
    PhlGetParamValue_t *param = NULL;
    UBOOL8 locked_here = FALSE;
    CmsRet cret;

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    cret = cmsPhl_getParamValueFlags(pathDesc, OGF_NO_VALUE_UPDATE, &param);
    if (cret != CMSRET_SUCCESS)
    {
        char *full_path = NULL;
        cmsMdm_pathDescriptorToFullPath(pathDesc, &full_path);
        cmsLog_error("Failed to get parameter: %s", full_path);
        CMSMEM_FREE_BUF_AND_NULL_PTR(full_path);
        cms_try_unlock(&locked_here);
        return WLMDM_GENERIC_ERROR;
    }

    strncpy(value, param->pValue, size - 1);

    cmsPhl_freeGetParamValueBuf(param, 1);

    cms_try_unlock(&locked_here);
    return WLMDM_OK;
}

WlmdmRet cms_try_lock(UBOOL8 *locked_here)
{
    CmsRet cret;
    assert(locked_here);

    if ((cret = cmsLck_acquireLockWithTimeout(WLMDM_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        *locked_here = TRUE;
        return WLMDM_OK;
    }
    else if (cret == CMSRET_WOULD_DEADLOCK)
    {
        /* We ignore CMSRET_WOULD_DEADLOCK, assuming the calling thread already has cms lock. */
        *locked_here = FALSE;
        return WLMDM_OK;
    }
    else
    {
        cmsLog_error("failed to get cms lock, error=%d", cret);
        cmsLck_dumpInfo();
        return WLMDM_GENERIC_ERROR;
    }
}

WlmdmRet cms_try_unlock(UBOOL8 *locked_here)
{
    assert(locked_here);

    if (*locked_here == TRUE)
    {
        cmsLck_releaseLock();
        *locked_here = FALSE;
    }

    return WLMDM_OK;
}

UBOOL8 compare_pathDesc(const MdmPathDescriptor *p, const MdmPathDescriptor *q)
{
    assert(p);
    assert(q);

    if ((p->oid == q->oid) &&
        (0 == strcmp(p->paramName, q->paramName)) &&
        (0 == cmsMdm_compareIidStacks(&(p->iidStack), &(q->iidStack))))
    {
        return TRUE;
    }
    return FALSE;
}
