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

#include "cms.h"
#include "cms_util.h"
#include "os_defs.h"

#include "wlmdm.h"
#include "nvn.h"
#include "conv.h"
#include "staged.h"
#include "gen_wlmdm_mapping.h"

static StagedInfo *staged_info = NULL;
static const char STAGED_FILE[] = "/tmp/nvm_staged";

WlmdmRet wlmdm_init()
{
    if (!cmsMdm_isInitialized())
    {
        cmsLog_error("cms MDM must be initialized before using libwlmdm!");
        return WLMDM_INVALID_PARAM;
    }

    if (staged_info == NULL)
    {
        staged_info = staged_init(STAGED_FILE);
    }

    if (staged_info == NULL)
    {
        wlmdm_destroy();
        return WLMDM_GENERIC_ERROR;
    }
    return WLMDM_OK;
}

void wlmdm_destroy()
{
    StagedRet ret;

    ret = staged_free(staged_info);
    if (ret == STAGED_OK)
    {
        staged_info = NULL;
    }
}

WlmdmRet wlmdm_nvram_set(const char *nvname, const char *value)
{
    if (nvname == NULL)
    {
        return WLMDM_INVALID_PARAM;
    }

    cmsLog_debug("name=%s, value=%s", nvname, value);
    if (STAGED_OK == staged_set(staged_info, nvname, value))
    {
        return WLMDM_OK;
    }
    return WLMDM_GENERIC_ERROR;
}

WlmdmRet wlmdm_nvram_unset(const char *nvname)
{
    return wlmdm_nvram_set(nvname, NULL);
}

static int wlmdm_staged_foreach(const char *nvname, const char *value, void *data __attribute__((unused)))
{
    int ret;

    if (value == NULL)
    {
        ret = (conv_unset(nvname) == WLMDM_OK) ? 0 : -1;
    }
    else
    {
        ret = (conv_set(nvname, value) == WLMDM_OK) ? 0 : -1;
    }
    return ret;
}

static int wlmdm_staged_commit(void *data __attribute__ ((unused)))
{
    if (WLMDM_OK == conv_save())
    {
        return 0;
    }
    return -1;
}

WlmdmRet wlmdm_nvram_commit()
{
    StagedRet ret;
    conv_prepare_save();
    ret = staged_commit(staged_info, &wlmdm_staged_foreach, &wlmdm_staged_commit, NULL);
    if (ret == STAGED_OK)
    {
        return WLMDM_OK;
    }
    else
    {
        return WLMDM_GENERIC_ERROR;
    }
}

WlmdmRet wlmdm_nvram_get(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    StagedRet r;

    if ((nvname == NULL) || (value == NULL) || (size <= 0))
    {
        cmsLog_error("Invalid parameter!");
        return WLMDM_INVALID_PARAM;
    }

    cmsLog_debug("name=%s", nvname);
    /* Search staged_list for uncommitted changes first.*/
    r = staged_get(staged_info, nvname, value, size);
    if (r == STAGED_OK)
    {
        ret = WLMDM_OK;
    }
    else if (r == STAGED_NOT_FOUND)
    {
       /*
        * nvname not found in staged_list.
        * Search MDM for corresponding parameter node instance instead.
        */
        ret = conv_get(nvname, value, size);
    }
    else
    {
        ret = WLMDM_NOT_FOUND;
    }

    return ret;
}

static int print_nvram(const char *nvname, const char *value, void *data)
{
    if (nvname == NULL)
    {
        printf("invalid nvname\n");
    }
    else
    {
        if(data)
        {
            /* if data is specified, it would be the stream for writing to a buffer */
            FILE *stream = (FILE*) data;

            if(value)
                fprintf(stream, "%s=%s\n", nvname, value);
        }
        else
        {
            if(value)
                printf("%s=%s\n", nvname, value);
        }
    }
    return 0;
}

static int filter_print_nvram(const char *nvname, const char *value, void *data)
{
    if (FALSE == staged_name_exist(staged_info, nvname))
    {
        if (FALSE == conv_unset_name_exist(nvname))
        {
            return print_nvram(nvname, value, data);
        }
    }
    return 0;
}

/*
 * Dump out all NVRAM configurations.
 * Search order:
 * 1. Dump out all "set" configurations from staged list.
 * 2. Dump out all NVRAM configurations in nvm_param_mapping_table by locating all their subtrees in MDM.
 *    if such configurations satisfy below conditions:
 *     Its corresponding NVRAM configuration has not been recorded in staged list and Device.Wifi.X_BROADCOM_COM_WlUnsetNvram.
 * 3. Dump out all special NVRAM configurations, by using data from MDM,
 *    if such configurations satisfy below conditions:
 *     Its corresponding NVRAM configuration has not been recorded in staged list and Device.Wifi.X_BROADCOM_COM_WlUnsetNvram.
 * 4. Dump out all NVRAM configurations from MDM parameter node:
 *     Device.Wifi.X_BRODACOM_COM_WlNVRAM
 *    if such configurations satisfy below conditions:
 *     Its corresponding NVRAM configuration has not been recorded in staged list and Device.Wifi.X_BROADCOM_COM_WlUnsetNvram.
 *
 */
void wlmdm_nvram_dump()
{

    /* Dump out all "set" NVRAM configurations in staged record. */
    staged_dump(staged_info, &print_nvram, NULL);

    /* Dump out NVRAM configurations in nvm_param_mapping_table[]
     * Dump out NVRAM configurations from MDM parameter node instance:
     * "Device.Wifi.X_BRODACOM_COM_WlNVRAM"
     */
    conv_foreach(&filter_print_nvram, NULL);

    return;

}

void wlmdm_nvram_getall(char *buf, size_t size)
{
    FILE *stream = NULL;
    size_t buff_size;
    char *buff_ptr = NULL;

    stream = open_memstream(&buff_ptr, &buff_size);
    if (stream == NULL)
    {
        printf("%s:%d open_memstream() error!! \n", __func__, __LINE__);
        return;
    }

    /* Dump out all "set" NVRAM configurations in staged record. */
    staged_dump(staged_info, &print_nvram, (void*)stream);

    /* Dump out NVRAM configurations in nvm_param_mapping_table[]
     * Dump out NVRAM configurations from MDM parameter node instance:
     * "Device.Wifi.X_BRODACOM_COM_WlNVRAM"
     */
    conv_foreach(&filter_print_nvram, (void*)stream);

    fflush(stream);
    fclose(stream);
    strncpy(buf, buff_ptr, (buff_size > size) ? size : buff_size);
    free(buff_ptr);

    return;
}

const char *wlmdm_error_str(WlmdmRet ret)
{
    switch (ret)
    {
        case WLMDM_OK:
            return "success";

        case WLMDM_NOT_FOUND:
            return "nvname or its value is not found";

        case WLMDM_INVALID_PARAM:
            return "invalid parameter";

        case WLMDM_GENERIC_ERROR:
            return "generic error";

        default:
            return "";
    }
}
