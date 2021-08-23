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
#include "wlmdm.h"
#include "vmap.h"
#include "table.h"
#include "constants.h"


struct expected
{
    const char *nvname;
    Value param_val;
    Value nvram_val;
};

const struct expected test[] =
{
    {
      "closed",
      {"0", MPT_BOOLEAN},
      {"1", MPT_BOOLEAN}
    },
    {
      "wme",
      {"0", MPT_INTEGER},
      {"off", MPT_STRING}
    },
    {
      "nband",
      {"5GHz", MPT_STRING},
      {"1", MPT_INTEGER}
    },
    {
      "reg_mode",
      {"2", MPT_INTEGER},
      {"d", MPT_STRING}
    }
};

enum { DATA_SIZE = sizeof(test) / sizeof(test[0]) };

static int test_param_get()
{
    size_t i;
    const NvmParamMapping *p;
    char *param_val;
    int ret = 0;

    for (i = 0; i < DATA_SIZE; i++)
    {
        printf("test param get on %s\n", test[i].nvname);
        p = table_search_conf_name(test[i].nvname);
        if (p == NULL)
        {
            printf("Test Failed: unable to get param_info!\n");
            return -1;
        }

        param_val = vmap_nvram_to_param(p, test[i].nvram_val.data);
        if (param_val == NULL)
        {
            printf("Failed to find param_val for %s!\n", test[i].nvname);
            continue;
        }

        if (0 != strcmp(param_val, test[i].param_val.data))
        {
            printf("Test failed for case[%d]:%s.\n", i, test[i].nvname);
            ret = -1;
        }
        cmsMem_free(param_val);
    }
    return ret;
}

static int test_nvram_get()
{
    size_t i;
    const NvmParamMapping *p;
    char *nvram_val;
    int ret = 0;

    for (i = 0; i < DATA_SIZE; i++)
    {
        printf("test nvram get on %s\n", test[i].nvname);
        p = table_search_conf_name(test[i].nvname);
        if (p == NULL)
        {
            printf("Test Failed: unable to get param_info!\n");
            return -1;
        }

        nvram_val = vmap_param_to_nvram(p, test[i].param_val.data);
        if (nvram_val == NULL)
        {
            printf("Failed to find nvram_val for %s!\n", test[i].nvname);
            continue;
        }
        if (0 != strcmp(nvram_val, test[i].nvram_val.data))
        {
            printf("Test failed for case[%d]:%s.\n", i, test[i].nvname);
            ret = -1;
        }
        cmsMem_free(nvram_val);
    }
    return ret;
}

int main(void)
{
    int ret = 0;

    ret = test_param_get();
    if (!ret)
    {
        ret = test_nvram_get();
    }

    if (!ret)
    {
        printf("Test Passed!\n");
    }

    return 0;
}
