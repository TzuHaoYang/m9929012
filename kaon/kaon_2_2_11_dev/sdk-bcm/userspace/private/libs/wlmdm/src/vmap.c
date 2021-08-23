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
#include <stdlib.h>
#include <string.h>

#include "cms_util.h"
#include "wlmdm.h"
#include "vmap.h"
#include "gen_value_mapping.h"

static MdmParamTypes get_base_type(MdmParamTypes a);
static UBOOL8 is_type_compatible(MdmParamTypes a, MdmParamTypes b);
static const Value *vmap_get_param_value(const ValueMapper *mapper, const Value *nvram_val);
static const Value *vmap_get_nvram_value(const ValueMapper *mapper, const Value *param_val);

char* vmap_param_to_nvram(const NvmParamMapping *p, const char *value)
{
    char *result = NULL;
    Value param_val = {0};
    const Value *v;

    assert(value);

    param_val.type = p->pv.type;
    param_val.data = cmsMem_strdup(value);
    v = vmap_get_nvram_value(p->mapper, &param_val);
    if (v == NULL)
    {
        cmsLog_error("Failed to do value mapping! param_name=%s, value=%s, type=%d",
                                    p->pv.name, value, p->pv.type);
    }
    else
    {
        result = cmsMem_strdup(v->data);
    }

    cmsMem_free(param_val.data);
    return result;
}

char* vmap_nvram_to_param(const NvmParamMapping *p, const char *value)
{
    char *result = NULL;
    Value nvram_val = {0};
    const Value *v;

    assert(value);

    nvram_val.data = cmsMem_strdup(value);
    nvram_val.type = p->nv.type;

    v = vmap_get_param_value(p->mapper, &nvram_val);
    if (v == NULL)
    {
        cmsLog_error("Failed to do value mapping! nvname=%s, value=%s, type=%d",
                                    p->nv.name, value, p->nv.type);
    }
    else
    {
        result = cmsMem_strdup(v->data);
    }

    cmsMem_free(nvram_val.data);
    return result;
}

static const Value *vmap_get_param_value(const ValueMapper *mapper, const Value *nvram_val)
{
    int i;
    for (i = 0; mapper[i].param.data != NULL; i++)
    {
        if (is_type_compatible(mapper[i].nvram.type, nvram_val->type) &&
            (0 == strcmp(mapper[i].nvram.data, nvram_val->data)))
        {
            return &mapper[i].param;
        }
    }
    return NULL;
}

static const Value *vmap_get_nvram_value(const ValueMapper *mapper, const Value *param_val)
{
    int i;
    for (i = 0; mapper[i].param.data != NULL; i++)
    {
        if (is_type_compatible(mapper[i].param.type, param_val->type) &&
            (0 == strcmp(mapper[i].param.data, param_val->data)))
        {
            return &mapper[i].nvram;
        }
    }
    return NULL;
}

static UBOOL8 is_type_compatible(MdmParamTypes a, MdmParamTypes b)
{
   MdmParamTypes base_a, base_b;
   base_a = get_base_type(a);
   base_b = get_base_type(b);
   if (base_a == base_b)
   {
       return TRUE;
   }
   else
   {
       return FALSE;
   }
}

static MdmParamTypes get_base_type(MdmParamTypes a)
{
    switch (a)
    {
        case MPT_STRING:
        case MPT_DATE_TIME:
        case MPT_BASE64:
        case MPT_HEX_BINARY:
        case MPT_UUID:
        case MPT_IP_ADDR:
        case MPT_MAC_ADDR:
            return MPT_STRING;

        case MPT_BOOLEAN:
        case MPT_INTEGER:
            return MPT_INTEGER;

        case MPT_UNSIGNED_INTEGER:
        case MPT_STATS_COUNTER32:
            return MPT_UNSIGNED_INTEGER;

        case MPT_UNSIGNED_LONG64:
        case MPT_STATS_COUNTER64:
            return MPT_UNSIGNED_LONG64;

        default:
            return a;
    }
}
