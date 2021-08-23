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
#include <stdlib.h>
#include <assert.h>

#include "cms_util.h"
#include "table.h"
#include "wlmdm.h"
#include "gen_wlmdm_mapping.h"


static int nvname_cmp(const void *a1, const void *a2)
{
    const NvmParamMapping *b1, *b2;
    char *s1, *s2;

    b1 = (NvmParamMapping *)a1;
    b2 = (NvmParamMapping *)a2;
    s1 = b1->nv.name;
    s2 = b2->nv.name;
    assert(s1);
    assert(s2);
    return strcmp(s1, s2);
}

/*
 * nvm_param_mapping_table_sorted[] is generated in order by nvname.
 * We just do a binary search on the table.
 */
const NvmParamMapping *table_search_conf_name(const char *name)
{
    NvmParamMapping key;
    NvmParamMapping *result;

    key.nv.name = strdup(name);
    if (key.nv.name == NULL)
    {
        cmsLog_error("Couldn't allocate buffer!");
        return NULL;
    }

    result = bsearch(&key, nvm_param_mapping_table_sorted,
            nvm_param_mapping_table_size, sizeof(NvmParamMapping),
            &nvname_cmp);

    free(key.nv.name);

    return result;
}

/*
 * nvm_param_mapping_table[] is generated in order by oid.
 * When we retrieve data from MDM using PHL layer for the whole list,
 * we can speed up the process by leveraging the getCache object in PHL.
 * To fully utilize the cache, we iterate nvm_param_mapping_table, which is ordered by oid.
 */
WlmdmRet table_foreach(table_foreach_func conf_func, nvc_for_each_func instance_func, void *data)
{
    unsigned int i;
    for (i = 0; i < nvm_param_mapping_table_size; i++)
    {
        conf_func(&nvm_param_mapping_table[i], instance_func, data);
    }
    return WLMDM_OK;
}
