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
 *************************************************************************
 *
 * This file handles nvram name convention processing, aka "nvn".
 *
 ************************************************************************/

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "cms_util.h"
#include "cms_core.h"
#include "os_defs.h"

#include "wlmdm.h"
#include "nvc.h"



/*
 * This function calculates iidStack from the nvram configuration name, by following
 * below convention:
 * case format == "wl%d.%d_XXXX"
 * {
 *     The first number: radio index;
 *     The second number: bssid index.
 * }
 * case format == "wl%d_XXXX"
 * {
 *     The first number: radio index;
 * }
 * all other cases
 * {
 *     fail
 * }
 */

WlmdmRet nvn_disassemble(const char *nvname, int *radio_index,
                           int *bssid_index, char *conf_name, size_t size)
{
    WlmdmRet ret;
    size_t len;
    char buf[MAX_NVRAM_NAME_SIZE] = {0};
    const char nvname_format_0[] = "wl%d.%d_%40s";
    const char nvname_format_1[] = "wl%d_%40s";

    assert(nvname);
    assert(radio_index);
    assert(bssid_index);
    assert(conf_name);

    len = strlen(nvname);
    if (len <= 3)
    {
        cmsLog_debug("Invalid nvram name: %s", nvname);
        return WLMDM_INVALID_PARAM;
    }

    ret = sscanf(nvname, nvname_format_0, radio_index, bssid_index, (char *)&buf);
    if (ret == 3)
    {
        strncpy(conf_name, buf, size - 1);
        return WLMDM_OK;
    }

    ret = sscanf(nvname, nvname_format_1, radio_index, (char *)&buf);
    if (ret == 2)
    {
        *bssid_index = -1;
        strncpy(conf_name, buf, size - 1);
        return WLMDM_OK;
    }

    return WLMDM_GENERIC_ERROR;
}

WlmdmRet nvn_gen(const char *conf_name, int radio_index, int bssid_index,
                 char *nvname, size_t size)
{
    const char nvname_format_0[] = "wl%d.%d_%s";
    const char nvname_format_1[] = "wl%d_%s";

    assert(conf_name);
    assert(nvname);

    if (radio_index >= 0)
    {
        if (bssid_index <= 0)
        {
            snprintf(nvname, size, nvname_format_1, radio_index, conf_name);
        }
        else
        {
            snprintf(nvname, size, nvname_format_0, radio_index, bssid_index, conf_name);
        }
        return WLMDM_OK;
    }
    else
    {
        snprintf(nvname, size, "%s", conf_name);
    }

    return WLMDM_GENERIC_ERROR;
}

