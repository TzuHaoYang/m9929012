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
#include "os_defs.h"
#include "cms_helper.h"
#include "scache.h"

static WlmdmRet scache_append(ParamTable *table, const PhlSetParamValue_t *param);

WlmdmRet scache_init(ParamTable *table)
{
    table->nval = 0;
    table->max = 0;
    table->list = NULL;
    return WLMDM_OK;
}

WlmdmRet scache_free(ParamTable *table)
{
    unsigned int i;
    PhlSetParamValue_t *p;

    if (table->list != NULL)
    {
        for (i = 0; i < table->nval; i++)
        {
            p = table->list + i;
            cmsMem_free(p->pParamType);
            cmsMem_free(p->pValue);
        }
    }
    free(table->list);
    table->list = NULL;
    return WLMDM_OK;
}

WlmdmRet scache_apply(ParamTable *table, scache_apply_func apply_func)
{
    WlmdmRet ret = WLMDM_OK;

    ret = apply_func(table->list, table->nval);

    return ret;
}

/* Append param to the end of table->list. If the max size of table isn't enough,
 * enlarge the max size by the rate of TABLE_GROW.
 * We will do a shallow copy of param if the operation is successful.
 */
static WlmdmRet scache_append(ParamTable *table, const PhlSetParamValue_t *param)
{
    if (table->list == NULL)
    {
        table->list = calloc(TABLE_INIT, sizeof(PhlSetParamValue_t));

        if (table->list == NULL)
        {
            return WLMDM_GENERIC_ERROR;
        }
        table->max = TABLE_INIT;
        table->nval = 0;
    }
    else if (table->nval >= table->max)
    {
        /* grow */
        PhlSetParamValue_t *p;
        p = (PhlSetParamValue_t *) realloc(table->list, (TABLE_GROW * table->max * sizeof(PhlSetParamValue_t)));

        if (p == NULL)
        {
            return WLMDM_GENERIC_ERROR;
        }

        table->max *= TABLE_GROW;
        table->list = p;
    }

    /* Shallow copy. The caller shouldn't change or free the pointers (if any) used by param. */
    (table->list)[table->nval] = *param;

    table->nval++;
    return WLMDM_OK;
}

WlmdmRet scache_update(ParamTable *table, const PhlSetParamValue_t *param)
{
    unsigned int i;
    PhlSetParamValue_t *p;

    for (i = 0; i < table->nval; i++)
    {
        p = table->list + i;

        if (TRUE == compare_pathDesc(&(p->pathDesc), &(param->pathDesc)))
        {
            cmsMem_free(p->pParamType);
            cmsMem_free(p->pValue);
            (table->list)[i] = *param;
            break;
        }
    }

    if (i == table->nval)
    {
        return scache_append(table, param);
    }

    return WLMDM_OK;
}
