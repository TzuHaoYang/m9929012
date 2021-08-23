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
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "json_tokener.h"
#include "json_object_iterator.h"
#include "nvc.h"

/*
 * nvc_list is a JSON object of fixed format defined below:
 *       {
 *           name:val,
 *           name:val,
 *           name:val, …
 *           name:val
 *       }
 *
 * name: The name of a NVRAM configuration. It is a JSON string.
 * value: the value of the configuration. It can be either null or a JSON string.
 * When value equals to null, it means name is unset.
 * Otherwise value is the string representation of its NVRAM value.
 * This module is responsible to parse the JSON string and
 * provide search/append/replace/delete interfaces to its caller.
 *
 */


/* Search in nvc_list for name.
 * If non-NULL value is found, return the coresponding value as a new string.
 * Otherwise return NULL.
 * The user is responsible to free the returned string buffer.
 */

char *nvc_list_get(const char *nvc_list, const char *nvname, UBOOL8 *exist)
{
    char *result = NULL;
    json_object* jlist = NULL;
    json_object* jval = NULL;

    *exist = FALSE;
    if (nvc_list == NULL)
    {
        return NULL;
    }

    jlist = json_tokener_parse(nvc_list);
    if(NULL == jlist)
    {
        log_debug("Failed to parse nvc_list: %s", nvc_list);
        return NULL;
    }

    if(!json_object_object_get_ex(jlist, nvname, &jval))
    {
        goto exit;
    }
    else
    {
        json_type val_type;
        *exist = TRUE;
        val_type = json_object_get_type(jval);
        if (val_type != json_type_null)
        {
            const char *id = json_object_get_string(jval);
            result = calloc(MAX_NVRAM_VALUE_SIZE, sizeof(char));
            if (result != NULL)
            {
                strncpy(result, id, MAX_NVRAM_VALUE_SIZE - 1);
            }
        }
    }
exit:
    json_object_put(jlist);
    return result;
}

UBOOL8 nvc_list_exist(const char *nvc_list, const char *nvname)
{
    UBOOL8 exist;
    char *value;

    value = nvc_list_get(nvc_list, nvname, &exist);
    if (value != NULL)
    {
        free(value);
    }
    return exist;
}

/*
 * Delete specified nameval_pair from nvc_list.
 * Return the new string if the operation succeeded, otherwise return NULL.
 * The caller is responsible to free the returned buffer.
 */
char *nvc_list_delete(const char *nvc_list, const char *nvname)
{
    char *result = NULL;
    const char *t;
    json_object* jlist = NULL;

    if (nvc_list == NULL)
    {
        return NULL;
    }

    jlist = json_tokener_parse(nvc_list);
    if(NULL == jlist)
    {
        log_debug("Failed to parse nvc_list: %s", nvc_list);
        return NULL;
    }

    json_object_object_del(jlist, nvname);

    t = json_object_to_json_string(jlist);
    if (t != NULL)
    {
        int slen;
        slen = strlen(t);
        result = calloc(slen + 1, sizeof(char));
        if (result != NULL)
        {
            strcpy(result, t);
        }
    }

    json_object_put(jlist);
    return result;
}

/*
 * Update nvc_list with nvname and value.
 * That is, if nvname can be found as key in nvc_list, update its value.
 * Otherwise, append nvname:value to nvc_list.
 * Returns the updated string if operation succeeded.
 * Otherwise return NULL.
 * The caller is responsible to free the returned string buffer.
 */
char* nvc_list_update(const char *nvc_list, const char *nvname, const char *value)
{
    char *result = NULL;
    json_object *jlist;

    if ((nvc_list == NULL) || (strlen(nvc_list) == 0))
    {
        jlist = json_object_new_object();
    }
    else
    {
        jlist = json_tokener_parse(nvc_list);
        if(NULL == jlist)
        {
            log_debug("Failed to parse nvc_list: %s", nvc_list);
            return NULL;
        }
    }

    nvc_list_update_object(jlist, nvname, value);
    result = nvc_list_object_to_string(jlist);
    json_object_put(jlist);
    return result;
}

int nvc_list_update_object(json_object *jlist, const char *nvname, const char *value)
{
    json_object *jval;

    assert(jlist);

    if (!json_object_object_get_ex(jlist, nvname, &jval))
    {
        /* key nvname not exist, append it to jlist. */
        if (value != NULL)
        {
            jval = json_object_new_string(value);
        }
        else
        {
            jval = NULL;
        }
        json_object_object_add(jlist, nvname, jval);
    }
    else
    {
        /* key nvname exist, modify it in jlist. */
        json_object_object_del(jlist, nvname);
        if (value != NULL)
        {
            jval = json_object_new_string(value);
        }
        else
        {
            jval = NULL;
        }
        json_object_object_add(jlist, nvname, jval);
    }
    return 0;
}

char *nvc_list_object_to_string(json_object *jlist)
{
    const char *t;
    char *result = NULL;

    assert(jlist);
    t = json_object_to_json_string(jlist);
    if (t != NULL)
    {
        int slen;
        slen = strlen(t);
        result = calloc(slen + 1, sizeof(char));
        if (result != NULL)
        {
            strcpy(result, t);
        }
    }
    return result;
}

int nvc_list_for_each(const char *nvc_list, nvc_for_each_func for_each_func, void *data)
{
    json_object *jlist;
    struct json_object_iterator it, iend;

    if (nvc_list == NULL)
    {
        return -1;
    }

    jlist = json_tokener_parse(nvc_list);
    if(NULL == jlist)
    {
        log_debug("Failed to parse nvc_list: %s", nvc_list);
        return -1;
    }

    it = json_object_iter_begin(jlist);
    iend = json_object_iter_end(jlist);

    while (!json_object_iter_equal(&it, &iend))
    {
        const char *name, *val;
        json_object *jval;
        name = json_object_iter_peek_name(&it);
        jval = json_object_iter_peek_value(&it);
        if (jval != NULL)
        {
            val = json_object_get_string(jval);
        }
        else
        {
            val = NULL;
        }
        for_each_func(name, val, data);
        json_object_iter_next(&it);
    }

    json_object_put(jlist);
    return 0;
}

UBOOL8 nvc_list_validate(const char *nvc_list)
{
    UBOOL8 result = TRUE;
    json_object *jlist;

    jlist = json_tokener_parse(nvc_list);
    if (jlist == NULL)
        result = FALSE;
    else
        json_object_put(jlist);

    return result;
}
