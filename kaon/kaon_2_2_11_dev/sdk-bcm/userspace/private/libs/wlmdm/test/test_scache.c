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
#include "cms_helper.h"
#include "wlmdm_lib.h"
#include "scache.h"

enum { TEST_SIZE = 127 };

PhlSetParamValue_t test_param[TEST_SIZE];

static void rand_str(char *dest, unsigned int len)
{
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned int i;

    for (i = 0; i < len; i++)
    {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        dest[i] = charset[index];
    }
}

static void init_test_param()
{
    int i;
    int len = 64;

    for (i = 0; i < TEST_SIZE; i++)
    {
        test_param[i].pathDesc.oid = i;
        test_param[i].pathDesc.iidStack.currentDepth = 1;
        test_param[i].pathDesc.iidStack.instance[0] = i + 4;
        rand_str((char *)&(test_param[i].pathDesc.paramName), MAX_MDM_PARAM_NAME_LENGTH);
        test_param[i].pParamType = cmsMem_strdup("string");
        test_param[i].pValue = (char *) cmsMem_alloc(len, ALLOC_ZEROIZE);
        if (test_param[i].pValue == NULL)
        {
            printf("Failed to malloc buffer for pValue!\n");
            return;
        }
        rand_str(test_param[i].pValue, len - 1);
    }
}

static WlmdmRet verify_func(PhlSetParamValue_t *list, unsigned int size)
{
    unsigned int i;
    PhlSetParamValue_t *p;
    if (size != TEST_SIZE)
    {
        return WLMDM_GENERIC_ERROR;
    }

    for (i = 0; i < size; i++)
    {
        p = list + i;
        if (FALSE == compare_pathDesc(&(p->pathDesc), &(test_param[i].pathDesc)))
        {
            printf("Failed to verify pathDesc for entry %d\n", i);
            return WLMDM_GENERIC_ERROR;
        }
        if (0 != strcmp(p->pValue, test_param[i].pValue))
        {
            printf("Failed to verify pValue for entry %d\n", i);
            return WLMDM_GENERIC_ERROR;
        }
    }

    return WLMDM_OK;
}

int main(void)
{
    WlmdmRet ret;
    ParamTable table;
    PhlSetParamValue_t *p;
    int i;

    init_test_param();

    ret = scache_init(&table);
    if (ret != WLMDM_OK)
    {
        printf("Failed to initialize scache table!\n");
        exit(-1);
    }

    for (i = 0; i < TEST_SIZE; i++)
    {
        p = &test_param[i];
        scache_update(&table, p);
    }

    if (WLMDM_OK == scache_apply(&table, verify_func))
    {
        printf("Test passed!\n");
    }
    else
    {
        printf("Test failed: verification error!\n");
    }

    scache_free(&table);

    return 0;
}
