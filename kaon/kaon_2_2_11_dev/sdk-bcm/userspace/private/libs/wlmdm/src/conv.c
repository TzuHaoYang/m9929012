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
#include "table.h"
#include "wlmdm.h"
#include "nvn.h"
#include "nvc.h"
#include "conv.h"
#include "vmap.h"
#include "scache.h"
#include "cms_helper.h"
#include "special.h"
#include "gen_wlmdm_mapping.h"

static UBOOL8 conv_is_mapped(const char *nvname);
static WlmdmRet conv_set_unmapped(const char *name, const char *value);
static WlmdmRet conv_unset_unmapped(const char *name);
static WlmdmRet conv_get_unmapped(const char *nvname, char *value, size_t value_len);
static WlmdmRet conv_unmapped_foreach(nvc_for_each_func for_each_func, void *data);
static WlmdmRet conv_mapped_foreach(nvc_for_each_func instance_func, void *data);
static WlmdmRet conv_get_unset(const char *nvname);
static WlmdmRet conv_add_unset(const char *nvname);
static WlmdmRet conv_del_unset(const char *nvname);

static char *get_unmapped_list();
static WlmdmRet set_unmapped_list(const char *nlist);
static char* convert_value(MdmParamTypes from, MdmParamTypes to, const char *value);
static WlmdmRet get_nvram_from_pathDesc(const MdmPathDescriptor *pathDesc, const NvmParamMapping *p,
                                     char *value, size_t size);
static WlmdmRet set_unset_list(const char *nlist);
static char *get_unset_list(void);

static ParamTable scache_value_table;
static char *unmapped_list = NULL;
static char *unset_list = NULL;

WlmdmRet conv_get(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;

    ret = conv_get_unset(nvname);
    if (ret == WLMDM_NOT_FOUND)
    {
        ret = special_get(nvname, value, size);
        if (ret == WLMDM_NOT_FOUND)
        {
            ret = conv_get_mapped(nvname, value, size);
            if (ret == WLMDM_NOT_FOUND)
            {
                ret = conv_get_unmapped(nvname, value, size);
            }
        }
    }
    return ret;
}

WlmdmRet conv_set(const char *nvname, const char *value)
{
    WlmdmRet ret;

    conv_del_unset(nvname);

    ret = special_set(nvname, value);
    if (ret == WLMDM_NOT_FOUND)
    {
        ret = conv_set_mapped(nvname, value);
        if (ret == WLMDM_NOT_FOUND)
        {
            ret = conv_set_unmapped(nvname, value);
        }
    }

    return ret;
}

WlmdmRet conv_unset(const char *nvname)
{
    WlmdmRet ret = WLMDM_OK;

    conv_add_unset(nvname);

    if (!conv_is_mapped(nvname))
    {
        ret = conv_unset_unmapped(nvname);
    }
    return ret;
}

WlmdmRet conv_foreach(nvc_for_each_func for_each_func, void *data)
{
    special_foreach(for_each_func, data);
    conv_mapped_foreach(for_each_func, data);
    conv_unmapped_foreach(for_each_func, data);
    return WLMDM_OK;
}

WlmdmRet conv_prepare_save()
{
    scache_init(&scache_value_table);
    unmapped_list = get_unmapped_list();
    unset_list = get_unset_list();
    return WLMDM_OK;
}

static WlmdmRet apply_scache(PhlSetParamValue_t *list, unsigned int size)
{
    WlmdmRet ret = WLMDM_OK;
    UBOOL8 locked_here = FALSE;
    CmsRet cret;

    if (size == 0)
        return ret;

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        ret = WLMDM_GENERIC_ERROR;
        return ret;
    }

    cret = cmsPhl_setParameterValues(list, size);

    if (cret != CMSRET_SUCCESS)
    {
        ret = WLMDM_GENERIC_ERROR;
        cmsLog_error("failed to apply scache table! cret=%d", cret);
    }
    cms_try_unlock(&locked_here);

    return ret;
}

WlmdmRet conv_save()
{
    WlmdmRet ret = WLMDM_OK;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;

    ret = scache_apply(&scache_value_table, apply_scache);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to apply all changes to MDM!");
        goto exit;
    }

    ret = set_unmapped_list(unmapped_list);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to apply unmapped list!");
        goto exit;
    }

    ret = set_unset_list(unset_list);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to apply unset list!");
        goto exit;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        ret = WLMDM_GENERIC_ERROR;
        goto exit;
    }

    if ((cret = cmsMgm_saveConfigToFlash()) != CMSRET_SUCCESS)
    {
        cmsLog_error("saveConfigToFlash failed, cret=%d", cret);
        ret = WLMDM_GENERIC_ERROR;
    }

    cms_try_unlock(&locked_here);

exit:
    scache_free(&scache_value_table);
    if (unmapped_list != NULL)
    {
        free(unmapped_list);
        unmapped_list = NULL;
    }

    if (unset_list != NULL)
    {
        free(unset_list);
        unset_list = NULL;
    }
    return ret;
}

static WlmdmRet conv_unmapped_foreach(nvc_for_each_func for_each_func, void *data)
{
    char *nlist = NULL;

    nlist = get_unmapped_list();

    if (nlist != NULL)
    {
        nvc_list_for_each(nlist, for_each_func, data);
        free(nlist);
    }

    return WLMDM_OK;
}

static WlmdmRet mapped_conf_func(const NvmParamMapping *p, nvc_for_each_func instance_func, void *data)
{
    WlmdmRet ret = WLMDM_OK;
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    int radio_index, bssid_index;
    MdmPathDescriptor pathDesc = {0};
    UBOOL8 locked_here = FALSE;
    void *cms_obj;
    CmsRet cret = CMSRET_SUCCESS;
    UBOOL8 need_instances = FALSE;

    assert(p);

    pathDesc.oid = p->pv.oid;
    INIT_INSTANCE_ID_STACK(&pathDesc.iidStack);

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    need_instances = cmsMdm_isPathDescriptorExist(&pathDesc) ? FALSE : TRUE;
    cms_try_unlock(&locked_here);

    strncpy((char *)&(pathDesc.paramName), p->pv.name, sizeof(pathDesc.paramName));

    if (need_instances == TRUE)
    {
        /* We need to get all instances for each oid found in the table */
        while (cret == CMSRET_SUCCESS)
        {
            if (cms_try_lock(&locked_here) != WLMDM_OK)
            {
                return WLMDM_GENERIC_ERROR;
            }

            cret = cmsObj_getNextFlags(pathDesc.oid, &pathDesc.iidStack, OGF_NO_VALUE_UPDATE, &cms_obj);
            if (cret != CMSRET_SUCCESS)
            {
                cms_try_unlock(&locked_here);
                break;
            }
            /* We only need the iidStack here. So free cms_obj immediately. */
            cmsObj_free(&cms_obj);
            cms_try_unlock(&locked_here);
            ret = get_nvram_from_pathDesc(&pathDesc, p, (char *)&value, sizeof(value));
            if (ret != WLMDM_OK)
            {
                cmsLog_error("Failed to get nvram value for oid[%d]", p->pv.oid);
                break;
            }

            get_radio_bssid_index(p->pv.oid, &pathDesc.iidStack, &radio_index, &bssid_index);
            ret = nvn_gen(p->nv.name, radio_index, bssid_index, (char *)&nvname, sizeof(nvname));
            instance_func((char *)&nvname, (char *)&value, data);
        }
    }
    else
    {
        ret = get_nvram_from_pathDesc(&pathDesc, p, (char *)&value, sizeof(value));
        if (ret != WLMDM_OK)
        {
            cmsLog_error("Failed to get nvram value for oid[%d]", p->pv.oid);
            return ret;
        }

        get_radio_bssid_index(p->pv.oid, &pathDesc.iidStack, &radio_index, &bssid_index);
        nvn_gen(p->nv.name, radio_index, bssid_index, (char *)&nvname, sizeof(nvname));
        instance_func((char *)&nvname, (char *)&value, data);
    }
    return ret;
}

static WlmdmRet conv_mapped_foreach(nvc_for_each_func instance_func, void *data)
{
    return table_foreach(&mapped_conf_func, instance_func, data);
}

WlmdmRet conv_get_mapped(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_OK;
    const NvmParamMapping *p;
    const MdmParamInfo *param_info;
    MdmPathDescriptor pathDesc;
    char name[MAX_NVRAM_NAME_SIZE] = {0};
    int radio_index = -1, bssid_index = -1;

    assert(nvname);
    INIT_PATH_DESCRIPTOR(&pathDesc);

    cmsLog_debug("nvname=%s", nvname);
    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&name, sizeof(name));
    if (ret == WLMDM_OK)
    {
        p = table_search_conf_name(name);
    }
    else
    {
        cmsLog_notice("Failed to disassemble %s", nvname);
        p = table_search_conf_name(nvname);
    }

    if (p == NULL)
    {
        cmsLog_notice("Couldn't find parameter information for nvram: %s", nvname);
        return WLMDM_NOT_FOUND;
    }

    param_info = &(p->pv);
    ret = get_iidStack(param_info->oid, radio_index, bssid_index, &pathDesc.iidStack);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    pathDesc.oid = param_info->oid;
    strncpy((char *)&(pathDesc.paramName), param_info->name, sizeof(pathDesc.paramName));

    ret = get_nvram_from_pathDesc(&pathDesc, p, value, size);

    return ret;
}

static UBOOL8 conv_is_mapped(const char *nvname)
{
    const NvmParamMapping *p;
    char name[MAX_NVRAM_NAME_SIZE] = {0};
    int ret;
    int radio_index = -1, bssid_index = -1;

    assert(nvname);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&name, sizeof(name));
    if (ret == 0)
    {
        p = table_search_conf_name(name);
    }
    else
    {
        cmsLog_notice("Failed to disassemble %s", nvname);
        p = table_search_conf_name(nvname);
    }

    if (p == NULL)
    {
        cmsLog_notice("Couldn't find parameter information for nvram: %s", nvname);
        return FALSE;
    }
    return TRUE;
}

WlmdmRet conv_set_mapped(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    const NvmParamMapping *p;
    MdmPathDescriptor pathDesc;
    const MdmParamInfo *param_info;
    PhlSetParamValue_t *param = NULL;
    char name[MAX_NVRAM_NAME_SIZE] = {0};
    int radio_index = -1, bssid_index = -1;

    assert(nvname);
    assert(value);

    INIT_PATH_DESCRIPTOR(&pathDesc);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&name, sizeof(name));
    if (ret == WLMDM_OK)
    {
        p = table_search_conf_name(name);
    }
    else
    {
        cmsLog_notice("Failed to disassemble %s", nvname);
        p = table_search_conf_name(nvname);
    }

    if (p == NULL)
    {
        cmsLog_debug("Invalid name: %s", nvname);
        return WLMDM_NOT_FOUND;
    }

    param_info = &(p->pv);
    ret = get_iidStack(param_info->oid, radio_index, bssid_index, &pathDesc.iidStack);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    pathDesc.oid = param_info->oid;
    strncpy((char *)&pathDesc.paramName, param_info->name, sizeof(pathDesc.paramName));

    param = cmsMem_alloc(sizeof(PhlSetParamValue_t), ALLOC_ZEROIZE);
    if (param == NULL)
    {
        cmsLog_error("Unable to allocate PhlSetParamValue_t!");
        return WLMDM_GENERIC_ERROR;
    }

    memcpy(&(param->pathDesc), &pathDesc, sizeof(MdmPathDescriptor));
    param->pParamType = cmsMem_strdup((char *)cmsMdm_paramTypeToString(param_info->type));

    if (p->mapper == NULL)
    {
        /* Just do the default value conversion. */
        param->pValue = convert_value(p->nv.type, p->pv.type, value);
    }
    else
    {
        /* We need to do data value conversion using nvm_string_mapper_xxx */
        param->pValue = vmap_nvram_to_param(p, value);
    }

    if (param->pValue == NULL)
    {
        cmsLog_error("Failed to calculate param value from %s!", value);
        ret = WLMDM_GENERIC_ERROR;
    }
    else
    {
        ret = scache_update(&scache_value_table, param);

        if (ret != WLMDM_OK)
        {
            cmsLog_error("Failed to update scache list!");
        }
    }

    cmsMem_free(param);
    return ret;
}

/*
 * All unmapped NVRAM configurations are stored into parameter node:
 * Device.Wifi.X_BROADCOM_COM_WlNvram
 */
static WlmdmRet conv_set_unmapped(const char *name, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    char *new;

    /* unmapped_list should be prepared in conv_prepare_save(). */
    new = nvc_list_update(unmapped_list, name, value);
    if (new != NULL)
    {
        free(unmapped_list);
        unmapped_list = new;
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

/*
 * Search Device.Wifi.X_BROADCOM_COM_WlNvram for unmapped configurations.
 * If found, delete the entry. Otherwise do nothing.
 */
static WlmdmRet conv_unset_unmapped(const char *name)
{
    WlmdmRet ret = WLMDM_OK;
    char *new;

    /* unmapped_list should be prepared in conv_prepare_save(), but it could be NULL */
    if (unmapped_list == NULL)
    {
        return WLMDM_OK;
    }

    new = nvc_list_delete(unmapped_list, name);
    if (new != NULL)
    {
        free(unmapped_list);
        unmapped_list = new;
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

/*
 * Add or Update the given nvram configuration into 
 * Device.Wifi.X_BROADCOM_COM_WlUnsetNvram.
 */
static WlmdmRet conv_add_unset(const char *nvname)
{
    WlmdmRet ret = WLMDM_OK;
    char *new;

    /* unset_list should be prepared in conv_prepare_save(). */
    new = nvc_list_update(unset_list, nvname, NULL);
    if (new != NULL)
    {
        free(unset_list);
        unset_list = new;
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

/*
 * Search Device.Wifi.X_BROADCOM_COM_WlUnsetNvram for the specified nvname.
 * If found, delete the entry. Otherwise do nothing.
 */
static WlmdmRet conv_del_unset(const char *nvname)
{
    WlmdmRet ret = WLMDM_OK;
    char *new;

    /* unset_list should be prepared in conv_prepare_save(), but it could be NULL */
    if (unset_list == NULL)
    {
        return WLMDM_OK;
    }

    new = nvc_list_delete(unset_list, nvname);
    if (new != NULL)
    {
        free(unset_list);
        unset_list = new;
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

/* get the content of Device.Wifi.X_BROADCOM_COM_WlUnsetNvram */
static char *get_unset_list(void)
{
    MdmPathDescriptor pathDesc;
    PhlGetParamValue_t  *pParamValue = NULL;
    char *nlist = NULL;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;


    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return NULL;
    }

    get_wlUnsetNvram_pathDesc(&pathDesc);

    cret = cmsPhl_getParamValueFlags(&pathDesc, OGF_NO_VALUE_UPDATE, &pParamValue);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsPhl_getParamValue error: %d", cret);
    }
    else
    {
        if (cmsUtl_strlen(pParamValue->pValue) > 0)
        {
            nlist = strdup(pParamValue->pValue);
            if (nlist && (nvc_list_validate((const char*) nlist) == FALSE))
            {
                free(nlist);
                nlist = NULL;
            }
        }
        cmsPhl_freeGetParamValueBuf(pParamValue, 1);
    }
    cms_try_unlock(&locked_here);
    return nlist;
}

/* save back to Device.Wifi.X_BROADCOM_COM_WlUnsetNvram */
static WlmdmRet set_unset_list(const char *nlist)
{
    WlmdmRet ret = WLMDM_OK;
    PhlSetParamValue_t paramValue = {0};
    MdmPathDescriptor pathDesc;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;

    if (nlist == NULL)
    {
        return WLMDM_OK;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    get_wlUnsetNvram_pathDesc(&pathDesc);

    memcpy(&paramValue.pathDesc, &pathDesc, sizeof(MdmPathDescriptor));
    paramValue.pParamType = "string";
    paramValue.pValue = strdup(nlist);
    paramValue.status = CMSRET_SUCCESS;

    cret = cmsPhl_setParameterValues(&paramValue, 1);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set parameter node: %d", cret);
        ret = WLMDM_GENERIC_ERROR;
    }

    free(paramValue.pValue);
    cms_try_unlock(&locked_here);
    return ret;
}

UBOOL8 conv_unset_name_exist(const char *nvname)
{
    UBOOL8 result = FALSE;
    char *nlist = NULL;
 
    nlist = get_unset_list();
 
    if (nlist != NULL)
    {
        result = nvc_list_exist(nlist, nvname);
    }
    return result;
}

static char *get_unmapped_list()
{
    MdmPathDescriptor pathDesc;
    PhlGetParamValue_t  *pParamValue = NULL;
    char *nlist = NULL;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;


    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return NULL;
    }

    get_wlnvram_pathDesc(&pathDesc);
    cret = cmsPhl_getParamValue(&pathDesc, &pParamValue);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("cmsPhl_getParamValue error: %d", cret);
    }
    else
    {
        if (cmsUtl_strlen(pParamValue->pValue) > 0)
        {
            nlist = strdup(pParamValue->pValue);
            if (nlist && (nvc_list_validate((const char*) nlist) == FALSE))
            {
                free(nlist);
                nlist = NULL;
            }
        }
        cmsPhl_freeGetParamValueBuf(pParamValue, 1);
    }
    cms_try_unlock(&locked_here);
    return nlist;
}

static WlmdmRet set_unmapped_list(const char *nlist)
{
    WlmdmRet ret = WLMDM_OK;
    PhlSetParamValue_t paramValue = {0};
    MdmPathDescriptor pathDesc;
    CmsRet cret;
    UBOOL8 locked_here = FALSE;

    if (nlist == NULL)
    {
        return WLMDM_OK;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    get_wlnvram_pathDesc(&pathDesc);

    memcpy(&paramValue.pathDesc, &pathDesc, sizeof(MdmPathDescriptor));
    paramValue.pParamType = "string";
    paramValue.pValue = strdup(nlist);
    paramValue.status = CMSRET_SUCCESS;

    cret = cmsPhl_setParameterValues(&paramValue, 1);
    if (cret != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to set parameter node: %d", cret);
        ret = WLMDM_GENERIC_ERROR;
    }

    free(paramValue.pValue);
    cms_try_unlock(&locked_here);
    return ret;
}

/* Search MDM data model's parameter node:
 *    Device.Wifi.X_BROADCOM_COM_WlNvram
 * for the specified nvname.
 * Input:  const char *nvname
 *         size_t size
 * Output: value
 */
static WlmdmRet conv_get_unmapped(const char *nvname, char *value, size_t value_len)
{
   WlmdmRet ret = WLMDM_NOT_FOUND;
   char *nlist = NULL;
   UBOOL8 exist;

   nlist = get_unmapped_list();

   if (nlist != NULL)
   {
       char *val;
       val = nvc_list_get(nlist, nvname, &exist);
       if (val != NULL)
       {
           strncpy(value, val, value_len);
           ret = WLMDM_OK;
           free(val);
       }
       free(nlist);
   }

   return ret;
}

/* Search MDM data model's parameter node:
 *    Device.Wifi.X_BROADCOM_COM_WlUnsetNvram
 * for the specified nvname.
 * Input:  const char *nvname
 *         size_t size
 * Output: value
 */
static WlmdmRet conv_get_unset(const char *nvname)
{
    WlmdmRet ret = WLMDM_NOT_FOUND;
    char *nlist = NULL;
    UBOOL8 exist;

    nlist = get_unset_list();

    if (nlist != NULL)
    {
        char *val;
        val = nvc_list_get(nlist, nvname, &exist);
        if (val != NULL)
        {
            /* the val should be NULL for the unset nvrams */
            ret = WLMDM_GENERIC_ERROR;
	    free(val);
        }
        else if (exist == FALSE)
        {
            ret = WLMDM_NOT_FOUND;
        }
        else
        {
            ret = WLMDM_NULL_ENTRY;
        }
        free(nlist);
    }
    return ret;
}

static char* convert_value(MdmParamTypes from, MdmParamTypes to, const char *value)
{
    char *buf = NULL;
    char *result = NULL;
    unsigned int len;

    assert(value);

    if ((from == MPT_STRING) && (to == MPT_HEX_BINARY))
    {
        /* We are treating the normal string as a binary array of ascii value,
         * and convert it into hexbinary format.
         */
        if (CMSRET_SUCCESS != cmsUtl_binaryBufToHexString((const UINT8 *)value, strlen(value), &result))
        {
            cmsLog_error("Failed to convert %s from MPT_STRING to MPT_HEX_BINARY!", value);
        }
    }
    else if ((from == MPT_HEX_BINARY) && (to == MPT_STRING))
    {
        if (CMSRET_SUCCESS != cmsUtl_hexStringToBinaryBuf(value, (UINT8 **)&buf, &len))
        {
            cmsLog_error("Failed to convert %s from MPT_STRING to MPT_HEX_BINARY!", value);
        }
        else
        {
            result = (char *) cmsMem_alloc(len + 1, ALLOC_ZEROIZE);
            if (result == NULL)
            {
                cmsLog_error("Failed to calloc memory!");
            }
            else
            {
                memcpy(result, buf, len);
            }
            cmsMem_free(buf);
        }
    }
    else if (from == to)
    {
        result = cmsMem_strdup(value);
    }
    else
    {
        cmsLog_error("Don't know how to convert data from type[%s] to type[%s]",
                                                   cmsMdm_paramTypeToString(from),
                                                   cmsMdm_paramTypeToString(to));
    }

    return result;
}

static WlmdmRet get_nvram_from_pathDesc(const MdmPathDescriptor *pathDesc, const NvmParamMapping *p,
                                        char *value, size_t size)
{
    WlmdmRet ret;
    char *s = NULL;

    ret = get_param_from_pathDesc(pathDesc, value, size);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to get param value for oid[%d]", p->pv.oid);
        return WLMDM_GENERIC_ERROR;
    }

    if (p->mapper == NULL)
    {
        s = convert_value(p->pv.type, p->nv.type, value);
    }
    else
    {
        /* We need to do data value conversion using nvm_string_mapper_xxx */
        s = vmap_param_to_nvram(p, value);
    }

    if (s != NULL)
    {
        strncpy(value, s, size - 1);
        cmsMem_free(s);
    }
    else
    {
        cmsLog_error("Failed to convert param value!");
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}
