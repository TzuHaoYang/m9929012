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
#include "wifi_constants.h"
#include "special.h"
#include "nvc.h"
#include "nvn.h"
#include "conv.h"
#include "chanspec.h"
#include "cms_helper.h"
#include "wlmdm.h"

static WlmdmRet set_chanspec(const char *nvname, const char *value);
static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_chanspec(nvc_for_each_func foreach_func, void *data);
static WlmdmRet _retrieve_chanspec_from_mdm(unsigned int radio_index,
                                       struct chanspec_t *chanspec);

static WlmdmRet set_key_index(const char *nvname, const char *value);
static WlmdmRet get_key_index(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_key_index(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_key(const char *nvname, const char *value);
static WlmdmRet get_key(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_key(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_country(const char *nvname, const char *value);
static WlmdmRet get_country(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_country(nvc_for_each_func foreach_func, void *data);
static WlmdmRet _parse_country_spec(const char *spec, char *ccode, size_t ccode_size,
                                                      char *rev,   size_t rev_size);

static WlmdmRet set_maclist(const char *nvname, const char *value);
static WlmdmRet get_maclist(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_maclist(nvc_for_each_func foreach_func, void *data);
WlmdmRet _add_to_maclist(char *maclist, size_t size, const char *mac);

static WlmdmRet set_wds_maclist(const char *nvname, const char *value);
static WlmdmRet get_wds_maclist(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_wds_maclist(nvc_for_each_func foreach_func, void *data);

ActionSet handle_chanspec =
{
    .set = set_chanspec,
    .get = get_chanspec,
    .foreach = foreach_chanspec
};

ActionSet handle_key_index =
{
    .set = set_key_index,
    .get = get_key_index,
    .foreach = foreach_key_index
};

ActionSet handle_key =
{
    .set = set_key,
    .get = get_key,
    .foreach = foreach_key
};

ActionSet handle_country =
{
    .set = set_country,
    .get = get_country,
    .foreach = foreach_country
};

ActionSet handle_maclist =
{
    .set = set_maclist,
    .get = get_maclist,
    .foreach = foreach_maclist
};

ActionSet handle_wds_maclist =
{
    .set = set_wds_maclist,
    .get = get_wds_maclist,
    .foreach = foreach_wds_maclist
};

SpecialHandler special_handler_table[] =
{
    {"wl%d_chanspec", &handle_chanspec},
    {"wl%d.%d_key wl%d_key", &handle_key_index},
    {"wl%d.%d_key1 wl%d.%d_key2 wl%d.%d_key3 wl%d.%d_key4 wl%d_key1 wl%d_key2 wl%d_key3 wl%d_key4", &handle_key},
    {"wl%d_country_code wl%d_country_rev", &handle_country},
    {"wl%d.%d_maclist wl%d_maclist", &handle_maclist},
    {"wl%d_wds", &handle_wds_maclist}
};

const unsigned int SPECIAL_TABLE_SIZE = sizeof(special_handler_table) / sizeof(SpecialHandler);

static WlmdmRet set_chanspec(const char *nvname, const char *value)
{
    struct chanspec_t chanspec = {0};
    unsigned int band = 0, nbw = 0, channel = 0;
    int radio_index = -1, bssid_index = -1;
    char buf[32], buf_1[32];
    const char *t;
    WlmdmRet ret;
    int set_sb = 1;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = chanspec_parse_string(value, &chanspec);
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse chanspec from %s", value);
        return ret;
    }

    channel = chanspec.channel;
    band = chanspec.band;

    if (channel != 0)
    {
        switch(chanspec.band_width)
        {
            case WL_CHANSPEC_BW_160:
                /* Calculate the lowest channel number */
                channel = channel - CH_80MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                nbw = WLC_BW_CAP_160MHZ;
                break;

            case WL_CHANSPEC_BW_80:
                /*  Adjust the lowest channel from center channel */
                channel = channel - CH_40MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                nbw = WLC_BW_CAP_80MHZ;
                break;

            case WL_CHANSPEC_BW_40:
                channel = channel - CH_20MHZ_APART + CH_10MHZ_APART;
                channel += chanspec.ctlsb_index * CH_20MHZ_APART;
                nbw = WLC_BW_CAP_40MHZ;
                break;

            case WL_CHANSPEC_BW_20:
                nbw = WLC_BW_CAP_20MHZ;
                set_sb = 0;
                break;
        }

        if (set_sb)
        {
            t = chanspec_get_sb_string(chanspec.ctlsb_index, chanspec.band_width);
            if (t == NULL)
            {
                cmsLog_error("Failed to retrieve proper side band index!");
                return -1;
            }

            ret = nvn_gen("nctrlsb", radio_index, bssid_index, (char *)&buf, sizeof(buf));
            if (ret == WLMDM_OK)
            {
                cmsLog_debug("conv_set_mapped %s=%s", buf, t);
                conv_set_mapped((char *)&buf, t);
            }
        }

        ret = nvn_gen("nband", radio_index, bssid_index, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            switch (band)
            {
            case WL_CHANSPEC_BAND_2G:
                conv_set_mapped((char *)&buf, "2");
                break;
            case WL_CHANSPEC_BAND_5G:
                conv_set_mapped((char *)&buf, "1");
                break;
            case WL_CHANSPEC_BAND_6G:
                conv_set_mapped((char *)&buf, "4");
                break;
            default:
                cmsLog_error("unexpected band in chanspec: %d", band);
                break;
            }
        }

        ret = nvn_gen("bw", radio_index, bssid_index, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            sprintf(buf_1, "%d", nbw);
            conv_set((char *)&buf, (char *)&buf_1);
        }
    }

    ret = nvn_gen("channel", radio_index, bssid_index, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        sprintf(buf_1, "%d", channel);
        cmsLog_debug("conv_set_mapped %s=%s", buf, buf_1);
        conv_set_mapped((char *)&buf, (char *)&buf_1);
    }
    return WLMDM_OK;
}

static int _get_channel(int radio_index)
{
    MdmPathDescriptor pathDesc;
    char buf[MAX_NVRAM_VALUE_SIZE];
    WlmdmRet ret;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_WL_BASE_CFG;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, 1);
    PUSH_INSTANCE_ID(&pathDesc.iidStack, radio_index + 1);
    strncpy((char *)&pathDesc.paramName, "WlChannel", sizeof(pathDesc.paramName));

    ret = get_param_from_pathDesc(&pathDesc, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return -1;
    }
    return (int) strtol((char *)&buf, NULL, 10);
}

static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size)
{
    struct chanspec_t chanspec = {0};
    int radio_index, bssid_index;
    char buf[32];
    WlmdmRet ret;
    int channel = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    channel = _get_channel(radio_index);
    if (channel < 0)
    {
        cmsLog_error("Failed to get \"WlChannel\" from MDM");
        return WLMDM_GENERIC_ERROR;
    }
    else if(channel == 0)
    {
        snprintf(value, size, "%d", 0);
        return WLMDM_OK;
    }

    ret = _retrieve_chanspec_from_mdm(radio_index, &chanspec);
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = chanspec_to_string(&chanspec, value, size);

    return ret;
}

static WlmdmRet foreach_chanspec(nvc_for_each_func foreach_func, void *data)
{
    struct chanspec_t chanspec = {0};
    unsigned int num_radio;
    unsigned int i;
    char nvname[MAX_NVRAM_NAME_SIZE], value[MAX_NVRAM_VALUE_SIZE];
    WlmdmRet ret;
    int channel = 0;

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        channel = _get_channel(i);
        if (channel < 0)
        {
            cmsLog_error("Failed to get \"WlChannel\" from MDM");
            continue;
        }
        else if(channel == 0)
        {
            snprintf(value, sizeof(value), "%d", 0);
        }
        else
        {
            ret = _retrieve_chanspec_from_mdm(i, &chanspec);
            if (ret != WLMDM_OK)
            {
                continue;
            }

            ret = chanspec_to_string(&chanspec, (char *)&value, sizeof(value));
            if (ret != WLMDM_OK)
            {
                continue;
            }
        }
        ret = nvn_gen("chanspec", i, -1, (char *)&nvname, sizeof(nvname));
        if (ret == WLMDM_OK)
        {
            foreach_func(nvname, value, data);
        }
    }
    return WLMDM_OK;
}

static WlmdmRet _retrieve_chanspec_from_mdm(unsigned int radio_index,
                                       struct chanspec_t *chanspec)
{
    WlmdmRet ret;
    char buf[32], buf_1[32];

    assert(chanspec);

    ret = nvn_gen("nband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            if (0 == strcmp((char *)buf_1, "1"))
            {
                chanspec->band = WL_CHANSPEC_BAND_5G;
            }
            else if (0 == strcmp((char *)buf_1, "2"))
            {
                chanspec->band = WL_CHANSPEC_BAND_2G;
            }
            else
            {
                chanspec->band = WL_CHANSPEC_BAND_6G;
            }
        }
    }

    ret = nvn_gen("bw", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get((char *)&buf, (char *)&buf_1, sizeof(buf_1));

        if (ret == WLMDM_NOT_FOUND)
        {
            /* set to 20MHz as default value */
            chanspec->band_width = WL_CHANSPEC_BW_20;
        }
        else if (ret == WLMDM_OK)
        {
            long int b;
            b = strtol((char *)&buf_1, NULL, 10);
            if ((b >= 0) && (b < LONG_MAX))
            {
                /*
                 * translate bw_cap to chanspec.band_width.
                 * for example: bw_cap = 0x7 -> 20/40/80MHz
                 * chanspec.band_width will be WL_CHANSPEC_BW_80 as max bandwidth of the specified capability
                 */
                chanspec->band_width = WL_BW_CAP_160MHZ(b) ? WL_CHANSPEC_BW_160 :
                                       (WL_BW_CAP_80MHZ(b) ? WL_CHANSPEC_BW_80 :
                                       (WL_BW_CAP_40MHZ(b) ? WL_CHANSPEC_BW_40 :
                                       (WL_BW_CAP_20MHZ(b) ? WL_CHANSPEC_BW_20 : 0)));
            }
            else
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
            }
        }
    }

    ret = nvn_gen("nctrlsb", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            ret = chanspec_get_sb_index((char *)buf_1);
            chanspec->ctlsb_index = ret;
        }
    }

    ret = nvn_gen("channel", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret == WLMDM_OK)
    {
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
        {
            long int b;
            unsigned int channel;

            b = strtol((char *)&buf_1, NULL, 10);
            if ((b < 0) || (b == LONG_MAX))
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
            }
            else
            {
                channel = (unsigned int) b;
                switch (chanspec->band_width)
                {
                    case WL_CHANSPEC_BW_160:
                        /* Calculate the lowest channel number */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_80MHZ_APART - CH_10MHZ_APART;
                        break;

                    case WL_CHANSPEC_BW_80:
                        /*  Adjust the lowest channel from center channel */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_40MHZ_APART - CH_10MHZ_APART;
                        break;

                    case WL_CHANSPEC_BW_40:
                        /*  Adjust the lowest channel from center channel */
                        channel -= chanspec->ctlsb_index * CH_20MHZ_APART;
                        /* Then use bw to calculate the center channel */
                        channel = channel + CH_20MHZ_APART - CH_10MHZ_APART;
                        break;

                    default:
                        break;
                }
                chanspec->channel = channel;
            }
        }
    }
    return WLMDM_OK;
}

static WlmdmRet set_key_index(const char *nvname, const char *value)
{
    WlmdmRet ret;
    MdmPathDescriptor path_vintf;
    int radio_index = -1, bssid_index = -1;
    char buf[32];
    long int b;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    /*special handle for bssid_index since nvn_disassemble will set bssid_index = -1 when meet "wlx_xxxx"*/
    if (bssid_index ==-1)
    {
        bssid_index = 0;
    }

    INIT_PATH_DESCRIPTOR(&path_vintf);
    path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
    PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, radio_index + 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, bssid_index + 1);

    strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
    ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }
    b = strtol((char *)&buf, NULL, 10);
    if ((b >= 0) && (b < LONG_MAX))
    {
        memset(&path_vintf.paramName, 0x00, sizeof(path_vintf.paramName));
        if (b == WL_BIT_KEY_64)
        {
            strncpy((char *)&path_vintf.paramName, "WlKeyIndex64", sizeof(path_vintf.paramName));
            ret = set_param_from_pathDesc(&path_vintf, value);
        }
        else if (b == WL_BIT_KEY_128)
        {
            strncpy((char *)&path_vintf.paramName, "WlKeyIndex128", sizeof(path_vintf.paramName));
            ret = set_param_from_pathDesc(&path_vintf, value);
        }
        else
        {
            ret = WLMDM_GENERIC_ERROR;
        }
    }
    else
    {
        cmsLog_error("Failed to convert %s to proper long int value!", buf);
    }
    return ret;
}

static WlmdmRet get_key_index(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    MdmPathDescriptor path_vintf;
    int radio_index = -1, bssid_index = -1;
    char buf[32];
    long int b;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    /*special handle for bssid_index since nvn_disassemble will set bssid_index = -1 when meet "wlx_xxxx"*/
    if (bssid_index ==-1)
    {
        bssid_index = 0;
    }

    INIT_PATH_DESCRIPTOR(&path_vintf);
    path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
    PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, radio_index + 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, bssid_index + 1);

    strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
    ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }
    b = strtol((char *)&buf, NULL, 10);
    if ((b >= 0) && (b < LONG_MAX))
    {
        memset(&path_vintf.paramName, 0x00, sizeof(path_vintf.paramName));
        if (b == WL_BIT_KEY_64)
        {
            strncpy((char *)&path_vintf.paramName, "WlKeyIndex64", sizeof(path_vintf.paramName));
            ret = get_param_from_pathDesc(&path_vintf, value, size);
        }
        else if (b == WL_BIT_KEY_128)
        {
            strncpy((char *)&path_vintf.paramName, "WlKeyIndex128", sizeof(path_vintf.paramName));
            ret = get_param_from_pathDesc(&path_vintf, value, size);
        }
        else
        {
            ret = WLMDM_GENERIC_ERROR;
        }
    }
    else
    {
        cmsLog_error("Failed to convert %s to proper long int value!", buf);
    }
    return ret;
}

static WlmdmRet foreach_key_index(nvc_for_each_func foreach_func, void *data)
{
    int i, j, num_radio, num_bssid;
    MdmPathDescriptor path_vintf;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    char buf[32];
    long int b;
    WlmdmRet ret = WLMDM_OK;

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        num_bssid = get_bssid_num_for_radio(i);
        for (j = 0; j < num_bssid; j++)
        {
            INIT_PATH_DESCRIPTOR(&path_vintf);
            path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
            PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
            PUSH_INSTANCE_ID(&path_vintf.iidStack, i + 1);
            PUSH_INSTANCE_ID(&path_vintf.iidStack, j + 1);

            strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
            ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
            if (ret != WLMDM_OK)
            {
                return WLMDM_GENERIC_ERROR;
            }

            b = strtol((char *)&buf, NULL, 10);
            if ((b >= 0) && (b < LONG_MAX))
            {
                memset(&path_vintf.paramName, 0x00, sizeof(path_vintf.paramName));
                if ((b == WL_BIT_KEY_64) || (b == WL_BIT_KEY_128))
                {
                    if (b == WL_BIT_KEY_64)
                    {
                        strncpy((char *)&path_vintf.paramName, "WlKeyIndex64", sizeof(path_vintf.paramName));
                    }
                    else
                    {
                        strncpy((char *)&path_vintf.paramName, "WlKeyIndex128", sizeof(path_vintf.paramName));
                    }
                    ret = get_param_from_pathDesc(&path_vintf, (char *)&value, sizeof(value));
                    if (ret == WLMDM_OK)
                    {
                        memset((void *)&nvname, 0x00, sizeof(nvname));
                        /*when j==0, set the prefix to wl%d*/
                        if (j == 0)
                        {
                            snprintf(nvname, sizeof(nvname) - 1, "wl%d_key", i);
                        }
                        else
                        {
                            snprintf(nvname, sizeof(nvname) - 1, "wl%d.%d_key", i, j);
                        }
                        foreach_func((char *)&nvname, (char *)&value, data);
                    }
                }
                else
                {
                    ret = WLMDM_GENERIC_ERROR;
                }
            }
            else
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf);
            }
        }
    }
    return ret;
}

static WlmdmRet set_key(const char *nvname, const char *value)
{
    WlmdmRet ret;
    MdmPathDescriptor path_vintf;
    MdmPathDescriptor path_keys;
    int radio_index = -1, bssid_index = -1, key_index;
    char buf[32];
    long int b;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }
    
    /*special handle for bssid_index since nvn_disassemble will set bssid_index = -1 when meet "wlx_xxxx"*/
    if (bssid_index ==-1)
    {
        bssid_index = 0;
    }
    
    if (0 == sscanf(buf, "key%d", &key_index))
    {
        return WLMDM_GENERIC_ERROR;
    }

    INIT_PATH_DESCRIPTOR(&path_vintf);
    path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
    PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, radio_index + 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, bssid_index + 1);

    strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
    ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }
    b = strtol((char *)&buf, NULL, 10);
    if ((b >= 0) && (b < LONG_MAX))
    {
        memset(&path_vintf.paramName, 0x00, sizeof(path_vintf.paramName));
        if ((b == WL_BIT_KEY_64) || (b == WL_BIT_KEY_128))
        {
            INIT_PATH_DESCRIPTOR(&path_keys);
            if (b == WL_BIT_KEY_64)
            {
                path_keys.oid = MDMOID_WL_KEY64_CFG;
                strncpy((char *)&path_keys.paramName, "WlKey64", sizeof(path_keys.paramName));
            }
            else if (b == WL_BIT_KEY_128)
            {
                path_keys.oid = MDMOID_WL_KEY128_CFG;
                strncpy((char *)&path_keys.paramName, "WlKey128", sizeof(path_keys.paramName));
            }
            PUSH_INSTANCE_ID(&path_keys.iidStack, 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, radio_index + 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, bssid_index + 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, key_index);
            ret = set_param_from_pathDesc(&path_keys, value);
        }
        else
        {
            ret = WLMDM_GENERIC_ERROR;
        }
    }
    else
    {
        cmsLog_error("Failed to convert %s to proper long int value!", buf);
    }
    return ret;
}

static WlmdmRet get_key(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    MdmPathDescriptor path_vintf;
    MdmPathDescriptor path_keys;
    int radio_index = -1, bssid_index = -1, key_index;
    char buf[32];
    long int b;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    /*special handle for bssid_index since nvn_disassemble will set bssid_index = -1 when meet "wlx_xxxx"*/
    if (bssid_index ==-1)
    {
        bssid_index = 0;
    }

    if (0 == sscanf(buf, "key%d", &key_index))
    {
        return WLMDM_GENERIC_ERROR;
    }

    INIT_PATH_DESCRIPTOR(&path_vintf);
    path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
    PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, radio_index + 1);
    PUSH_INSTANCE_ID(&path_vintf.iidStack, bssid_index + 1);

    strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
    ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }
    b = strtol((char *)&buf, NULL, 10);
    if ((b >= 0) && (b < LONG_MAX))
    {
        memset(&path_vintf.paramName, 0x00, sizeof(path_vintf.paramName));
        if ((b == WL_BIT_KEY_64) || (b == WL_BIT_KEY_128))
        {
            INIT_PATH_DESCRIPTOR(&path_keys);
            if (b == WL_BIT_KEY_64)
            {
                path_keys.oid = MDMOID_WL_KEY64_CFG;
                strncpy((char *)&path_keys.paramName, "WlKey64", sizeof(path_keys.paramName));
            }
            else if (b == WL_BIT_KEY_128)
            {
                path_keys.oid = MDMOID_WL_KEY128_CFG;
                strncpy((char *)&path_keys.paramName, "WlKey128", sizeof(path_keys.paramName));
            }
            PUSH_INSTANCE_ID(&path_keys.iidStack, 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, radio_index + 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, bssid_index + 1);
            PUSH_INSTANCE_ID(&path_keys.iidStack, key_index);
            ret = get_param_from_pathDesc(&path_keys, value, size);
        }
        else
        {
            ret = WLMDM_GENERIC_ERROR;
        }
    }
    else
    {
        cmsLog_error("Failed to convert %s to proper long int value!", buf);
    }
    return ret;
}

static WlmdmRet foreach_key(nvc_for_each_func foreach_func, void *data)
{
    int i, j, k, num_radio, num_bssid;
    MdmPathDescriptor path_vintf;
    MdmPathDescriptor path_keys;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    char buf[32];
    long int b;
    WlmdmRet ret = WLMDM_OK;

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        num_bssid = get_bssid_num_for_radio(i);
        for (j = 0; j < num_bssid; j++)
        {
            INIT_PATH_DESCRIPTOR(&path_vintf);
            path_vintf.oid = MDMOID_WL_VIRT_INTF_CFG;
            PUSH_INSTANCE_ID(&path_vintf.iidStack, 1);
            PUSH_INSTANCE_ID(&path_vintf.iidStack, i + 1);
            PUSH_INSTANCE_ID(&path_vintf.iidStack, j + 1);

            strncpy((char *)&path_vintf.paramName, "WlKeyBit", sizeof(path_vintf.paramName));
            ret = get_param_from_pathDesc(&path_vintf, (char *)&buf, sizeof(buf));
            if (ret != WLMDM_OK)
            {
                return WLMDM_GENERIC_ERROR;
            }

            b = strtol((char *)&buf, NULL, 10);
            if ((b >= 0) && (b < LONG_MAX))
            {
                if ((b == WL_BIT_KEY_64) || (b == WL_BIT_KEY_128))
                {
                    INIT_PATH_DESCRIPTOR(&path_keys);
                    if (b == WL_BIT_KEY_64)
                    {
                        path_keys.oid = MDMOID_WL_KEY64_CFG;
                        strncpy((char *)&path_keys.paramName, "WlKey64", sizeof(path_keys.paramName));
                    }
                    else if (b == WL_BIT_KEY_128)
                    {
                        path_keys.oid = MDMOID_WL_KEY128_CFG;
                        strncpy((char *)&path_keys.paramName, "WlKey128", sizeof(path_keys.paramName));
                    }
                    PUSH_INSTANCE_ID(&path_keys.iidStack, 1);
                    PUSH_INSTANCE_ID(&path_keys.iidStack, i + 1);
                    PUSH_INSTANCE_ID(&path_keys.iidStack, j + 1);
                    for (k = 0; k < 4; k++)
                    {
                        memset((void *)&nvname, 0x00, sizeof(nvname));
                        /*when j==0, set the prefix to wl%d*/
                        if (j == 0)
                        {
                            snprintf((char *)&nvname, sizeof(nvname), "wl%d_key%d", i, k + 1);
                        }
                        else
                        {
                            snprintf((char *)&nvname, sizeof(nvname), "wl%d.%d_key%d", i, j, k + 1);
                        }
                        PUSH_INSTANCE_ID(&path_keys.iidStack, k + 1);
                        ret = get_param_from_pathDesc(&path_keys, (char *)&value, sizeof(value));
                        if (ret == WLMDM_OK)
                        {
                            foreach_func((char *)&nvname, (char *)&value, data);
                        }
                        POP_INSTANCE_ID(&path_keys.iidStack);
                    }
                }
                else
                {
                    ret = WLMDM_GENERIC_ERROR;
                }
            }
            else
            {
                cmsLog_error("Failed to convert %s to proper long int value!", buf);
            }
        }
    }
    return ret;
}


static WlmdmRet set_country(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    MdmPathDescriptor path;
    int radio_index, bssid_index;
    char buf[16] = {0};
    char name[16] = {0};
    char country_code[16] = {0};
    char country_rev[16] = {0};

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&name, sizeof(name));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    INIT_PATH_DESCRIPTOR(&path);
    path.oid = MDMOID_WL_BASE_CFG;
    PUSH_INSTANCE_ID(&path.iidStack, 1);
    PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
    strncpy((char *)&path.paramName, "WlCountry", sizeof(path.paramName));

    ret = get_param_from_pathDesc(&path, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = _parse_country_spec((char *)&buf, (char *)&country_code, sizeof(country_code),
                                            (char *)&country_rev,  sizeof(country_rev));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    /* Overwrite data using NVRAM set values. */
    if (0 == strcmp(name, "country_code"))
    {
        strncpy((char *)country_code, value, sizeof(country_code) - 1);
    }
    else if (0 == strcmp(name, "country_rev"))
    {
        strncpy((char *)country_rev, value, sizeof(country_rev) - 1);
    }
    snprintf((char *)&buf, sizeof(buf), "%s/%s", country_code, country_rev);
    ret = set_param_from_pathDesc(&path, (char *)&buf);
    return ret;
}

static WlmdmRet get_country(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    MdmPathDescriptor path;
    int radio_index, bssid_index;
    char buf[16] = {0};
    char name[16] = {0};
    char country_code[16] = {0};
    int country_rev[16] = {0};

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&name, sizeof(name));
    if (ret != WLMDM_OK)
    {
        cmsLog_notice("Failed to parse nvname: %s", nvname);
        return ret;
    }

    INIT_PATH_DESCRIPTOR(&path);
    path.oid = MDMOID_WL_BASE_CFG;
    PUSH_INSTANCE_ID(&path.iidStack, 1);
    PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
    strncpy((char *)&path.paramName, "WlCountry", sizeof(path.paramName));

    ret = get_param_from_pathDesc(&path, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = _parse_country_spec((char *)&buf, (char *)&country_code, sizeof(country_code),
                                            (char *)&country_rev, sizeof(country_rev));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    if (0 == strcmp(name, "country_code"))
    {
        strncpy(value, country_code, size);
    }
    else if (0 == strcmp(name, "country_rev"))
    {
        strncpy(value, (char *)&country_rev, size);
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }
    return ret;
}

static WlmdmRet foreach_country(nvc_for_each_func foreach_func, void *data)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    MdmPathDescriptor path;
    int num_radio, i;
    char buf[16] = {0};
    char nvname[MAX_NVRAM_NAME_SIZE];
    char country_code[16];
    char country_rev[16];

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        INIT_PATH_DESCRIPTOR(&path);
        path.oid = MDMOID_WL_BASE_CFG;
        PUSH_INSTANCE_ID(&path.iidStack, 1);
        PUSH_INSTANCE_ID(&path.iidStack, i + 1);
        strncpy((char *)&path.paramName, "WlCountry", sizeof(path.paramName));

        ret = get_param_from_pathDesc(&path, (char *)&buf, sizeof(buf));
        if (ret != WLMDM_OK)
        {
            return ret;
        }

        ret = _parse_country_spec((char *)&buf, (char *)&country_code, sizeof(country_code),
                                                (char *)&country_rev,  sizeof(country_rev));
        if (ret != WLMDM_OK)
        {
            return ret;
        }

        snprintf(nvname, sizeof(nvname), "wl%d_country_code", i);
        foreach_func(nvname, country_code, data);

        snprintf(nvname, sizeof(nvname), "wl%d_country_rev", i);
        foreach_func(nvname, country_rev, data);
    }
    return ret;
}

static WlmdmRet _parse_country_spec(const char *spec, char *ccode, size_t ccode_size,
                                                      char *rev,   size_t rev_size)
{
    char *anchor;
    size_t ccode_len;

    memset(ccode, 0x00, ccode_size);
    memset(rev, 0x00, rev_size);

    /*initial country rev to "0" since we can't parse it from "const char *spec" sometimes*/
    strncpy(rev, "0", rev_size - 1);

    anchor = strchr(spec, '/');
    if (anchor)
    {
        strncpy(rev, anchor + 1, rev_size - 1);
        ccode_len = (size_t)(anchor - spec);
    }
    else
    {
        ccode_len = (size_t)strlen(spec);
    }

    if ((ccode_len > 3) || (ccode_len + 1 > ccode_size))
    {
        cmsLog_error("country code length doesn't fit");
        return WLMDM_GENERIC_ERROR;
    }

    memcpy(ccode, spec, ccode_len);
    ccode[ccode_len] = '\0';
    return WLMDM_OK;
}

static WlmdmRet set_maclist(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    CmsRet cret;
    MdmPathDescriptor path;
    InstanceIdStack iidStack;
    int i, radio_index, bssid_index;
    char *dup, *mac, *saveptr;
    WlMacFltObject *macflt_obj = NULL;
    UBOOL8 locked_here = FALSE;
    char ifname[16] = {0};
    char ssid[16] = {0};

    i = sscanf(nvname, "wl%d.%d_maclist", &radio_index, &bssid_index);
    if (i != 2)
    {
        return WLMDM_GENERIC_ERROR;
    }

    INIT_PATH_DESCRIPTOR(&path);
    path.oid = MDMOID_LAN_WLAN;
    PUSH_INSTANCE_ID(&path.iidStack, 1);
    PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
    strncpy((char *)&path.paramName, "X_BROADCOM_COM_IfName", sizeof(path.paramName));
    ret = get_param_from_pathDesc(&path, (char *)&ifname, sizeof(ifname));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    if (bssid_index == MAIN_BSS_IDX)
    {
        INIT_PATH_DESCRIPTOR(&path);
        path.oid = MDMOID_LAN_WLAN;
        PUSH_INSTANCE_ID(&path.iidStack, 1);
        PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
        strncpy((char *)&path.paramName, "SSID", sizeof(path.paramName));
    }
    else if ((bssid_index >= GUEST_BSS_IDX) && (bssid_index <= GUEST2_BSS_IDX))
    {
        INIT_PATH_DESCRIPTOR(&path);
        path.oid = MDMOID_LAN_WLAN;
        PUSH_INSTANCE_ID(&path.iidStack, 1);
        PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
        if (0 == (bssid_index - GUEST_BSS_IDX))
        {
            strncpy((char *)&path.paramName, "X_BROADCOM_COM_GuestSSID", sizeof(path.paramName));
        }
        else
        {
            snprintf((char *)&path.paramName, sizeof(path.paramName),
                     "X_BROADCOM_COM_Guest%dSSID", bssid_index - GUEST_BSS_IDX);
        }
    }
    else if (bssid_index > GUEST2_BSS_IDX)
    {
        INIT_PATH_DESCRIPTOR(&path);
        path.oid = MDMOID_LAN_WLAN_VIRT_MBSS;
        PUSH_INSTANCE_ID(&path.iidStack, 1);
        PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
        PUSH_INSTANCE_ID(&path.iidStack, bssid_index - GUEST2_BSS_IDX);
        strncpy((char *)&path.paramName, "MbssGuestSSID", sizeof(path.paramName));
    }
    else
    {
        return WLMDM_GENERIC_ERROR;
    }

    ret = get_param_from_pathDesc(&path, (char *)&ssid, sizeof(ssid));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    /* Clear mac filter instances in MDM. */
    INIT_PATH_DESCRIPTOR(&path);
    path.oid = MDMOID_WL_MAC_FLT;
    PUSH_INSTANCE_ID(&path.iidStack, 1);
    PUSH_INSTANCE_ID(&path.iidStack, radio_index + 1);
    PUSH_INSTANCE_ID(&path.iidStack, bssid_index + 1);
    INIT_INSTANCE_ID_STACK(&iidStack);
    cret = cmsObj_getNextInSubTree(path.oid, &path.iidStack,
                                   &iidStack, (void **)&macflt_obj);
    while (cret == CMSRET_SUCCESS)
    {
        cret = cmsObj_deleteInstance(path.oid, &iidStack);
        cmsObj_free((void **)&macflt_obj);

        INIT_INSTANCE_ID_STACK(&iidStack);
        cret = cmsObj_getNextInSubTree(path.oid, &path.iidStack,
                                       &iidStack, (void **)&macflt_obj);
    }

    /* Add new mac filter instances to MDM using NVRAM configurations. */
    dup = strdup(value);
    if (dup == NULL)
    {
        cmsLog_error("Failed to allocate memory!");
        return WLMDM_GENERIC_ERROR;
    }

    for (mac = strtok_r(dup, " ", &saveptr); mac != NULL; mac = strtok_r(NULL, " ", &saveptr))
    {
        INIT_PATH_DESCRIPTOR(&path);
        path.oid = MDMOID_WL_MAC_FLT;
        PUSH_INSTANCE_ID(&(path.iidStack), 1);
        PUSH_INSTANCE_ID(&(path.iidStack), radio_index + 1);
        PUSH_INSTANCE_ID(&(path.iidStack), bssid_index + 1);
        cret = cmsObj_addInstance(path.oid, &path.iidStack);
        if (cret != CMSRET_SUCCESS)
        {
            ret = WLMDM_GENERIC_ERROR;
            goto exit;
        }

        cret = cmsObj_get(path.oid, &path.iidStack, 0, (void **)&macflt_obj);
        if (cret != CMSRET_SUCCESS)
        {
            ret = WLMDM_GENERIC_ERROR;
            goto exit;
        }

        REPLACE_STRING_IF_NOT_EQUAL(macflt_obj->wlIfcname, ifname);
        REPLACE_STRING_IF_NOT_EQUAL(macflt_obj->wlMacAddr, mac);
        REPLACE_STRING_IF_NOT_EQUAL(macflt_obj->wlSsid, ssid);

        cret = cmsObj_set(macflt_obj, &path.iidStack);
        cmsObj_free((void **)&macflt_obj);
        if (cret != CMSRET_SUCCESS)
        {
            ret = WLMDM_GENERIC_ERROR;
            goto exit;
        }
    }

exit:
    free(dup);
    cms_try_unlock(&locked_here);
    return ret;
}

static WlmdmRet get_maclist(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    CmsRet cret;
    InstanceIdStack iidStackParent, iidStack;
    int i, radio_index, bssid_index;
    WlMacFltObject *macflt_obj = NULL;
    UBOOL8 locked_here = FALSE;

    i = sscanf(nvname, "wl%d.%d_maclist", &radio_index, &bssid_index);
    if (i != 2)
    {
        return WLMDM_GENERIC_ERROR;
    }

    memset(value, 0x00, size);
    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    INIT_INSTANCE_ID_STACK(&iidStackParent);
    PUSH_INSTANCE_ID(&iidStackParent, 1);
    PUSH_INSTANCE_ID(&iidStackParent, radio_index + 1);
    PUSH_INSTANCE_ID(&iidStackParent, bssid_index + 1);
    INIT_INSTANCE_ID_STACK(&iidStack);
    cret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, &iidStackParent,
                                   &iidStack, (void **)&macflt_obj);
    while (cret == CMSRET_SUCCESS)
    {
        if (macflt_obj->wlMacAddr != NULL)
        {
            ret = _add_to_maclist(value, size, macflt_obj->wlMacAddr);
        }
        cmsObj_free((void **)&macflt_obj);
        if (ret != WLMDM_OK)
        {
            break;
        }
        cret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, &iidStackParent,
                                       &iidStack, (void **)&macflt_obj);
    }
    cms_try_unlock(&locked_here);
    return ret;
}

static WlmdmRet foreach_maclist(nvc_for_each_func foreach_func, void *data)
{
    WlmdmRet ret = WLMDM_OK;
    CmsRet cret;
    InstanceIdStack iidStackParent, iidStack;
    int i, j, num_radio, num_bssid;
    WlMacFltObject *macflt_obj = NULL;
    char nvname[MAX_NVRAM_NAME_SIZE], value[MAX_NVRAM_VALUE_SIZE];
    UBOOL8 locked_here = FALSE;

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        num_bssid = get_bssid_num_for_radio(i);
        for (j = 0; j < num_bssid; j++)
        {
            INIT_INSTANCE_ID_STACK(&iidStackParent);
            PUSH_INSTANCE_ID(&iidStackParent, 1);
            PUSH_INSTANCE_ID(&iidStackParent, i + 1);
            PUSH_INSTANCE_ID(&iidStackParent, j + 1);
            INIT_INSTANCE_ID_STACK(&iidStack);

            memset(value, 0x00, sizeof(value));
            if (cms_try_lock(&locked_here) != WLMDM_OK)
            {
                return WLMDM_GENERIC_ERROR;
            }

            cret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, &iidStackParent,
                                           &iidStack, (void **)&macflt_obj);
            while (cret == CMSRET_SUCCESS)
            {
                if (macflt_obj->wlMacAddr != NULL)
                {
                    ret = _add_to_maclist(value, sizeof(value), macflt_obj->wlMacAddr);
                }
                cmsObj_free((void **)&macflt_obj);
                if (ret != WLMDM_OK)
                {
                    break;
                }
                cret = cmsObj_getNextInSubTree(MDMOID_WL_MAC_FLT, &iidStackParent,
                                               &iidStack, (void **)&macflt_obj);
            }
            cms_try_unlock(&locked_here);
            memset(nvname, 0x00, sizeof(nvname));
            /*when j==0, set the prefix to wl%d*/
            if (j == 0)
            {
                snprintf(nvname, sizeof(nvname) - 1, "wl%d_maclist", i);
            }
            else
            {
                snprintf(nvname, sizeof(nvname) - 1, "wl%d.%d_maclist", i, j);
            }
            foreach_func(nvname, value, data);
        }
    }
    return ret;
}


/* maclist should be a string which follows below format:
 * "mac_addr_0 mac_addr_1 mac_addr_2...mac_addr_x"
 * mac addresses are separeted by ' ' in maclist string.
 */
WlmdmRet _add_to_maclist(char *maclist, size_t size, const char *mac)
{
    size_t len_l, len_m;

    assert(maclist);
    assert(mac);
    len_l = strlen(maclist);
    len_m = strlen(mac);
    /* We will append mac string to maclist, and add a separator in front of it.
     * Also count in the terminator '\0', we will use up len_l + len_m + 2 bytes
     * in the target buffer 'maclist' if the operation is successful.
     */
    if (len_l + len_m + 2 > size)
    {
        return WLMDM_INVALID_PARAM;
    }
    strcat(maclist, " ");
    strcat(maclist, mac);
    return WLMDM_OK;
}

static WlmdmRet set_wds_maclist(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    CmsRet cret;
    InstanceIdStack iidStack;
    int i, radio_index;
    char *dup, *mac, *saveptr;
    WlStaticWdsCfgObject *wdscfg_obj = NULL;
    UBOOL8 locked_here = FALSE;

    i = sscanf(nvname, "wl%d_wds", &radio_index);
    if (i != 1)
    {
        return WLMDM_GENERIC_ERROR;
    }

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    dup = strdup(value);
    if (dup == NULL)
    {
        cmsLog_error("Failed to allocate memory!");
        return WLMDM_GENERIC_ERROR;
    }
    mac = strtok_r(dup, " ", &saveptr);
    /* There are a fixed number of WDS_CFG instances, which is currently 4. */
    for (i = 0; i < 4; i++)
    {
        INIT_INSTANCE_ID_STACK(&iidStack);
        PUSH_INSTANCE_ID(&iidStack, 1);
        PUSH_INSTANCE_ID(&iidStack, radio_index + 1);
        PUSH_INSTANCE_ID(&iidStack, i + 1);
        cret = cmsObj_get(MDMOID_WL_STATIC_WDS_CFG, &iidStack, 0, (void **)&wdscfg_obj);
        if (cret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to get instance %d of MDMOID_WL_STATIC_WDS_CFG!", i + 1);
            ret = WLMDM_GENERIC_ERROR;
            goto exit;
        }

        if (mac != NULL)
        {
            REPLACE_STRING_IF_NOT_EQUAL(wdscfg_obj->wlMacAddr, mac);
            mac = strtok_r(NULL, " ", &saveptr);
        }
        else
        {
            REPLACE_STRING_IF_NOT_EQUAL(wdscfg_obj->wlMacAddr, "");
        }

        cret = cmsObj_set(wdscfg_obj, &iidStack);
        cmsObj_free((void **)&wdscfg_obj);
        if (cret != CMSRET_SUCCESS)
        {
            ret = WLMDM_GENERIC_ERROR;
            goto exit;
        }
    }

exit:
    free(dup);
    cms_try_unlock(&locked_here);
    return ret;
}

static WlmdmRet get_wds_maclist(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret = WLMDM_GENERIC_ERROR;
    CmsRet cret;
    InstanceIdStack iidStackParent, iidStack;
    int i, radio_index;
    WlStaticWdsCfgObject *wdscfg_obj = NULL;
    UBOOL8 locked_here = FALSE;

    i = sscanf(nvname, "wl%d_wds", &radio_index);
    if (i != 1)
    {
        return WLMDM_GENERIC_ERROR;
    }

    memset(value, 0x00, size);
    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    INIT_INSTANCE_ID_STACK(&iidStackParent);
    PUSH_INSTANCE_ID(&iidStackParent, 1);
    PUSH_INSTANCE_ID(&iidStackParent, radio_index + 1);
    INIT_INSTANCE_ID_STACK(&iidStack);
    cret = cmsObj_getNextInSubTree(MDMOID_WL_STATIC_WDS_CFG, &iidStackParent,
                                   &iidStack, (void **)&wdscfg_obj);
    while (cret == CMSRET_SUCCESS)
    {
        if (wdscfg_obj->wlMacAddr != NULL)
        {
            ret = _add_to_maclist(value, size, wdscfg_obj->wlMacAddr);
        }
        cmsObj_free((void **)&wdscfg_obj);
        if (ret != WLMDM_OK)
        {
            break;
        }
        cret = cmsObj_getNextInSubTree(MDMOID_WL_STATIC_WDS_CFG, &iidStackParent,
                                       &iidStack, (void **)&wdscfg_obj);
    }
    cms_try_unlock(&locked_here);
    return ret;
}

static WlmdmRet foreach_wds_maclist(nvc_for_each_func foreach_func, void *data)
{
    WlmdmRet ret = WLMDM_OK;
    CmsRet cret;
    InstanceIdStack iidStackParent, iidStack;
    int i, num_radio;
    WlStaticWdsCfgObject *wdscfg_obj = NULL;
    char nvname[MAX_NVRAM_NAME_SIZE], value[MAX_NVRAM_VALUE_SIZE];
    UBOOL8 locked_here = FALSE;

    num_radio = get_num_instances(MDMOID_WLAN_ADAPTER);
    for (i = 0; i < num_radio; i++)
    {
        INIT_INSTANCE_ID_STACK(&iidStackParent);
        PUSH_INSTANCE_ID(&iidStackParent, 1);
        PUSH_INSTANCE_ID(&iidStackParent, i + 1);
        INIT_INSTANCE_ID_STACK(&iidStack);

        memset(value, 0x00, sizeof(value));
        if (cms_try_lock(&locked_here) != WLMDM_OK)
        {
            return WLMDM_GENERIC_ERROR;
        }

        cret = cmsObj_getNextInSubTree(MDMOID_WL_STATIC_WDS_CFG, &iidStackParent,
                                       &iidStack, (void **)&wdscfg_obj);
        while (cret == CMSRET_SUCCESS)
        {
            if (wdscfg_obj->wlMacAddr != NULL)
            {
                ret = _add_to_maclist(value, sizeof(value), wdscfg_obj->wlMacAddr);
            }
            cmsObj_free((void **)&wdscfg_obj);
            if (ret != WLMDM_OK)
            {
                break;
            }
            cret = cmsObj_getNextInSubTree(MDMOID_WL_STATIC_WDS_CFG, &iidStackParent,
                                           &iidStack, (void **)&wdscfg_obj);
        }
        cms_try_unlock(&locked_here);
        memset(nvname, 0x00, sizeof(nvname));
        snprintf(nvname, sizeof(nvname) - 1, "wl%d_wds", i);
        foreach_func(nvname, value, data);
    }
    return ret;
}
