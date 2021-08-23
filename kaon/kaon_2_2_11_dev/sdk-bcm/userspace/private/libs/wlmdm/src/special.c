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
#include <limits.h>

#include "cms_util.h"
#include "cms_phl.h"
#include "cms_obj.h"
#include "os_defs.h"
#include "special.h"
#include "nvc.h"
#include "nvn.h"
#include "conv.h"
#include "chanspec.h"
#include "cms_helper.h"

static int consume_digits(const char *nvname, unsigned int *j);

extern SpecialHandler special_handler_table[];
extern const unsigned int SPECIAL_TABLE_SIZE;

WlmdmRet special_set(const char *nvname, const char *value)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        if ((special_handler_table[i].nvname && (match_name(special_handler_table[i].nvname, nvname) == TRUE))
            ||((NULL != special_handler_table[i].actions->match) && (special_handler_table[i].actions->match(nvname))))
        {
            return special_handler_table[i].actions->set(nvname, value);
        }
    }
    return WLMDM_NOT_FOUND;
}

WlmdmRet special_get(const char *nvname, char *value, size_t size)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
       if ((special_handler_table[i].nvname && (match_name(special_handler_table[i].nvname, nvname) == TRUE))
           ||((NULL != special_handler_table[i].actions->match) && (special_handler_table[i].actions->match(nvname))))
        {
            return special_handler_table[i].actions->get(nvname, value, size);
        }
    }
    return WLMDM_NOT_FOUND;
}

WlmdmRet special_foreach(nvc_for_each_func for_each_func, void *data)
{
    unsigned int i;

    for (i = 0; i < SPECIAL_TABLE_SIZE; i++)
    {
        cmsLog_debug("handling %s", special_handler_table[i].nvname);
        special_handler_table[i].actions->foreach(for_each_func, data);
    }
    return WLMDM_OK;
}

static int consume_digits(const char *nvname, unsigned int *j)
{
    int ret = 1;
    while ((nvname[*j] >= '0') && (nvname[*j] <= '9'))
    {
        *j += 1;
        ret = 0;  // find at least one digit
    }
    return ret;
}

UBOOL8 match_name(const char *pattern, const char *nvname)
{
    unsigned int plen, nlen;
    unsigned int i, j;
    char *saveptr;
    char *dup, *p;

    dup = strdup(pattern);
    if (dup == NULL)
    {
        return FALSE;
    }

    nlen = strlen(nvname);
    p = strtok_r(dup, " ", &saveptr);
    while (p != NULL)
    {
        plen = strlen(p);
        j = 0;
        for (i = 0; (i < plen) && (j < nlen); i++)
        {
            if (p[i] == '%' && p[++i] == 'd')
            {
                if (consume_digits(nvname, &j) != 0)
                    break;
            }
            else if (p[i] == nvname[j])
            {
                j++;
            }
            else
            {
                break;
            }
        }

        if ((i == plen) && (j == nlen))
        {
            free(dup);
            return TRUE;
        }
        p = strtok_r(NULL, " ", &saveptr);
    }

    free(dup);
    return FALSE;
}
