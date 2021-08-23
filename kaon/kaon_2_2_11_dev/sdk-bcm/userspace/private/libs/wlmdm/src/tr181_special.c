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

#include "cms.h"
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

#define MAX_PREFIX_LEN        64
#define MAX_PREFIX_ENTR       16

static WlmdmRet set_lan_wps_oob(const char *nvname, const char *value);
static WlmdmRet get_lan_wps_oob(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_wps_oob(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_chanspec(const char *nvname, const char *value);
static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_chanspec(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_lan_ifname(const char *nvname, const char *value);
static WlmdmRet get_lan_ifname(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_ifname(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_lan_ifnames(const char *nvname, const char *value);
static WlmdmRet get_lan_ifnames(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_lan_ifnames(nvc_for_each_func foreach_func, void *data);

static WlmdmRet _set_auto_channel(unsigned int radio_index, UBOOL8 enable);
static int _get_auto_channel(int radio_index);
static WlmdmRet _retrieve_chanspec_from_mdm(unsigned int radio_index,
                                       struct chanspec_t *chanspec);

static WlmdmRet set_mbss_names(const char *nvname, const char *value);
static WlmdmRet get_mbss_names(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_mbss_names(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_mapbss(const char *nvname, const char *value);
static WlmdmRet get_mapbss(const char *nvname, char *value, size_t size);
static UBOOL8 match_mapbss(const char *nvname);
static WlmdmRet foreach_mapbss(nvc_for_each_func foreach_func, void *data);

static WlmdmRet set_nctrlsb(const char *nvname, const char *value);
static WlmdmRet get_nctrlsb(const char *nvname, char *value, size_t size);
static WlmdmRet foreach_nctrlsb(nvc_for_each_func foreach_func, void *data);

extern CmsRet rutBridge_addFullPathToBridge_dev2(const char *fullPath, const char *brIntfName);
extern UBOOL8 match_name(const char *pattern, const char *nvname);

ActionSet handle_lan_wps_oob =
{
    .set = set_lan_wps_oob,
    .get = get_lan_wps_oob,
    .foreach = foreach_lan_wps_oob
};

ActionSet handle_chanspec =
{
    .set = set_chanspec,
    .get = get_chanspec,
    .foreach = foreach_chanspec
};

ActionSet handle_lan_ifname =
{
    .set = set_lan_ifname,
    .get = get_lan_ifname,
    .foreach = foreach_lan_ifname
};

ActionSet handle_lan_ifnames =
{
    .set = set_lan_ifnames,
    .get = get_lan_ifnames,
    .foreach = foreach_lan_ifnames
};

ActionSet handle_mbss_names =
{
    .set = set_mbss_names,
    .get = get_mbss_names,
    .foreach = foreach_mbss_names
};

ActionSet handle_mapbss =
{
    .set = set_mapbss,
    .get = get_mapbss,
    .match = match_mapbss,
    .foreach = foreach_mapbss
};

ActionSet handle_nctrlsb =
{
    .set = set_nctrlsb,
    .get = get_nctrlsb,
    .foreach = foreach_nctrlsb
};

SpecialHandler special_handler_table[] =
{
    { "lan_wps_oob lan%d_wps_oob", &handle_lan_wps_oob },
    { "wl%d_chanspec", &handle_chanspec },
    { "lan_ifname lan%d_ifname", &handle_lan_ifname },
    { "lan_ifnames lan%d_ifnames", &handle_lan_ifnames },
    { "map_bss_names", &handle_mbss_names },
    { "wl%d_nctrlsb", &handle_nctrlsb },
    { NULL, &handle_mapbss}    /*use its own special match: prefix + suffix*/
};

const unsigned int SPECIAL_TABLE_SIZE = sizeof(special_handler_table) / sizeof(SpecialHandler);

/*Parsing prefix string and put in string array res
* Params:
*  @prefixStr: INPUT. prefixes splitted by " ". exp, "fh bh"
*  @res: INPUT. pointer of an string array.
* return: number of entries parsed. -1: failed
*/
static int get_prefixList(const char *prefixStr, char res[][MAX_PREFIX_LEN])
{
   char *dup, *p, *t;
   char *next = NULL;
   unsigned int i = 0;

   if(!prefixStr || !res)
      return -1;

   dup = strdup(prefixStr);
   if(NULL == dup)
      return -1;

   p = strtok_r(dup, " ", &next);
   while(NULL != p)
   {
      t=p;
      while(*t == ' ') t++;
      snprintf(res[i++], MAX_PREFIX_LEN, "%s", t);
      if(i >= MAX_PREFIX_ENTR)
      {
         i = -1;
         break;
      }
      p = strtok_r(NULL, " ", &next);
   }

   free(dup);
   return i;
}

static WlmdmRet set_lan_wps_oob(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    int i, num_ssid, bi_a, bi_b;
    char buf[16] = {0};
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    i = sscanf(nvname, "lan%d_wps_oob", &bi_a);
    if (i == 0)
    {
        bi_a = 0;
    }

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);
    for (i = 0; i < num_ssid; i++)
    {
        INIT_PATH_DESCRIPTOR(&path_ssid);
        path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
        PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
        strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrName", sizeof(path_ssid.paramName));

        ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
        if (ret != 0)
        {
            continue;
        }

        bi_b = buf[2] - '0';
        if(bi_b == bi_a)
        {
            if (!strcmp(value, "enabled"))
            {
                strcpy((char *)&buf, "0");
            }
            else
            {
                strcpy((char *)&buf, "1");
            }
            INIT_PATH_DESCRIPTOR(&path_wps);
            path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
            PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
            strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

            ret = set_param_from_pathDesc(&path_wps, (char *)&buf);
            if (ret != WLMDM_OK)
            {
                break;
            }
        }
    }
    return ret;
}

static WlmdmRet get_lan_wps_oob(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    int i, num_ssid, bi_a, bi_b;
    char buf[16] = {0};
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    i = sscanf(nvname, "lan%d_wps_oob", &bi_a);
    if (i == 0)
    {
        bi_a = 0;
    }

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);
    for (i = 0; i < num_ssid; i++)
    {
        INIT_PATH_DESCRIPTOR(&path_ssid);
        path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
        PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
        strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrName", sizeof(path_ssid.paramName));

        ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
        if (ret != 0)
        {
            continue;
        }

        bi_b = buf[2] - '0';
        if(bi_b == bi_a)
        {
            INIT_PATH_DESCRIPTOR(&path_wps);
            path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
            PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
            strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

            ret = get_param_from_pathDesc(&path_wps, (char *)&buf, sizeof(buf));
            if (ret == WLMDM_OK)
            {
                if (!strcmp((char *)&buf, "0"))
                {
                    strncpy(value, "enabled", size - 1);
                }
                else
                {
                    strncpy(value, "disabled", size - 1);
                }
            }
            return ret;
        }
    }
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet foreach_lan_wps_oob(nvc_for_each_func foreach_func, void *data)
{
    WlmdmRet ret = 0;
    UINT8 flag = 0;
    int bi, i, num_ssid;
    char buf[16] = {0};
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    MdmPathDescriptor path_ssid;
    MdmPathDescriptor path_wps;

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);

    for (i = 0; i < num_ssid; i++)
    {
        INIT_PATH_DESCRIPTOR(&path_ssid);
        path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
        PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
        strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrName", sizeof(path_ssid.paramName));

        ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
        if (ret != WLMDM_OK)
        {
            continue;
        }

        bi = buf[2] - '0';
        if ((bi < 0) || (bi >= 8))
        {
            continue;
        }

        if ((flag & (1 << bi)) != 0)
        {
            continue;
        }

        INIT_PATH_DESCRIPTOR(&path_wps);
        path_wps.oid = MDMOID_DEV2_WIFI_ACCESS_POINT_WPS;
        PUSH_INSTANCE_ID(&path_wps.iidStack, i + 1);
        strncpy((char *)&path_wps.paramName, "X_BROADCOM_COM_Wsc_config_state", sizeof(path_wps.paramName));

        ret = get_param_from_pathDesc(&path_wps, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            if (!strcmp((char *)&buf, "0"))
            {
                strncpy(value, "enabled", sizeof(value) - 1);
            }
            else
            {
                strncpy(value, "disabled", sizeof(value) - 1);
            }
            if (bi == 0)
            {
                strncpy(nvname, "lan_wps_oob", sizeof(nvname) - 1);
            }
            else
            {
                snprintf(nvname, sizeof(nvname) - 1, "lan%d_wps_oob", bi);
            }
            flag = flag | (1 << bi);
            foreach_func(nvname, value, data);
        }
    }
    return ret;
}

static WlmdmRet set_chanspec(const char *nvname, const char *value)
{
    WlmdmRet ret;
    struct chanspec_t chanspec = {0};
    unsigned int band = 0, nbw = 0, channel = 0;
    int radio_index = -1, bssid_index = -1;
    char buf[32], buf_1[32];
    const char *t;
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
                conv_set((char *)&buf, t);
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
            conv_set_mapped((char *)&buf, (char *)&buf_1);
        }
        _set_auto_channel(radio_index, FALSE);
    }
    else
    {
        _set_auto_channel(radio_index, TRUE);
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

static WlmdmRet get_chanspec(const char *nvname, char *value, size_t size)
{
    WlmdmRet ret;
    struct chanspec_t chanspec = {0};
    int radio_index, bssid_index;
    char buf[32];
    int autoChannel = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    autoChannel = _get_auto_channel(radio_index);
    if (autoChannel == -1)
    {
        cmsLog_error("Failed to get \"AutoChannelEnable\" from MDM");
        return ret;
    }
    else if(autoChannel == 1)
    {
        snprintf(value, size, "%d", 0);
        return 0;
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
    int i, num_radio;
    char nvname[MAX_NVRAM_NAME_SIZE], value[MAX_NVRAM_VALUE_SIZE];
    int autoChannel = 0;
    WlmdmRet ret;

    num_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
    for (i = 0; i < num_radio; i++)
    {
        autoChannel = _get_auto_channel(i);
        if (autoChannel == -1)
        {
            cmsLog_error("Failed to get \"AutoChannelEnable\" from MDM");
            continue;
        }
        else if(autoChannel == 1)
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
        ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
        if (ret == WLMDM_OK)
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
        ret = conv_get((char *)&buf, (char *)buf_1, sizeof(buf_1));
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

static WlmdmRet set_lan_ifname(const char *nvname __attribute__((unused)),
                               const char *value __attribute__((unused)))
{
    // set lan_ifname is not allowed
    return WLMDM_GENERIC_ERROR;
}

static WlmdmRet get_lan_ifname(const char *nvname, char *value, size_t size)
{
    CmsRet cret;
    WlmdmRet ret = WLMDM_OK;
    char br_name[32] = {0};
    int bi, found = 0;
    Dev2BridgeObject *brObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 locked_here = FALSE;

    assert(value);
    if (0 == sscanf(nvname, "lan%d_ifname", &bi))
    {
        bi = 0;
    }

    memset(value, 0x00, size);
    snprintf((char *)&br_name, sizeof(br_name), "br%d", bi);

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brObj)) == CMSRET_SUCCESS)
    {
        if (strcmp(brObj->X_BROADCOM_COM_IfName, br_name) == 0)
        {
            found = 1;
            cmsObj_free((void **)&brObj);
            break;
        }
        cmsObj_free((void **)&brObj);
    }

    if (found)
    {
        strncpy(value, br_name, size - 1);
    }
    else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    cms_try_unlock(&locked_here);

    return ret;
}

static WlmdmRet foreach_lan_ifname(nvc_for_each_func foreach_func, void *data)
{
    CmsRet cret;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    UBOOL8 locked_here = FALSE;
    Dev2BridgeObject *brObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brObj)) == CMSRET_SUCCESS)
    {
        long int b;
        int bi;

        memset((void *)&nvname, 0x00, sizeof(nvname));
        memset((void *)&value, 0x00, sizeof(value));

        b = strtol((char *)&brObj->X_BROADCOM_COM_IfName[2], NULL, 10);
        if ((b < 0) || (b == LONG_MAX))
        {
            cmsLog_error("Failed to convert %s to proper long int value!", brObj->X_BROADCOM_COM_IfName[2]);
        }
        else
        {
            bi = (int) b;
            if (bi == 0)
            {
                snprintf(nvname, sizeof(nvname) - 1, "lan_ifname");
            }
            else
            {
                snprintf(nvname, sizeof(nvname) - 1, "lan%d_ifname", bi);
            }

            snprintf(value, sizeof(value) - 1, "%s", brObj->X_BROADCOM_COM_IfName);
            foreach_func((char *)&nvname, (char *)&value, data);
        }
        cmsObj_free((void **)&brObj);
    }
    cms_try_unlock(&locked_here);
    return WLMDM_OK;
}

extern void rutBridge_deleteIntfNameFromBridge_dev2(const char *intfName);

static int isWifiIntf(char *interface)
{
    return (!strncmp(interface, "wl", 2) ||
        !strncmp(interface, "radiotap", 8)||
        !strncmp(interface, "wds", 3));
}

static int hasMatchedName(const char *name, const char *nameList)
{
    int len = strlen(name);
    const char *match = nameList;

    if (!nameList || !name) return 0;
    // Because only valid interface name would be in the nameList,
    // we could take advantage when do string matching. (no ?wlx.x case)
    while (match != NULL)
    {
        match = strstr(match, name);
        if (match)
        {
            if (match[len] == '\0' || match[len] == ' ')
                break;
            else
                match += len;
        }
    }

    return (match ? 1 : 0);
}

static WlmdmRet set_lan_ifnames(const char *nvname, const char *value)
{
    WlmdmRet ret = WLMDM_OK;
    CmsRet cret;
    unsigned int bi;
    int i, num_ssid;
    char buf[BUFLEN_64] = {0};
    char br_name[BUFLEN_32] = {0};
    char *dup, *if_name, *saveptr;
    char *full_path = NULL;
    MdmPathDescriptor path_ssid;
    InstanceIdStack iidStack;
    InstanceIdStack sub_iidStack;
    Dev2BridgeObject *brObj = NULL;
    Dev2BridgePortObject *brPortObj = NULL;
    UBOOL8 found;
    UBOOL8 locked_here = FALSE;

    assert(value);

    if (0 == sscanf(nvname, "lan%d_ifnames", &bi))
    {
        bi = 0;
    }

    cmsLog_debug("nvname:%s value:%s", nvname, value);

    snprintf((char *)&br_name, sizeof(br_name), "br%d", bi);

    // check if any wifi interface should be removed from bridge
    INIT_INSTANCE_ID_STACK(&iidStack);
    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                    OGF_NO_VALUE_UPDATE,
                    (void **) &brObj)) == CMSRET_SUCCESS)
    {
        if (strcmp(brObj->X_BROADCOM_COM_IfName, br_name) == 0)
        {
            INIT_INSTANCE_ID_STACK(&sub_iidStack);
            while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                        &sub_iidStack, (void **)&brPortObj) == CMSRET_SUCCESS)
            {
                if (isWifiIntf(brPortObj->name) && !hasMatchedName(brPortObj->name, value))
                {
                    cmsLog_notice("Remove %s from %s, since it is no longer in %s", brPortObj->name, br_name, value);
                    rutBridge_deleteIntfNameFromBridge_dev2(brPortObj->name);
                    INIT_INSTANCE_ID_STACK(&sub_iidStack);
                }
                cmsObj_free((void **)&brPortObj);
            }
            cmsObj_free((void **)&brObj);
            break;
        }
        cmsObj_free((void **)&brObj);
    }
    cms_try_unlock(&locked_here);

    // check if any wifi interface should be added into bridge.
    dup = strdup(value);
    if (dup == NULL)
    {
        cmsLog_error("Failed to allocate memory!");
        return WLMDM_GENERIC_ERROR;
    }

    num_ssid = get_num_instances(MDMOID_DEV2_WIFI_SSID);
    if_name = strtok_r(dup, " ", &saveptr);
    while (if_name != NULL)
    {
        for (i = 0; i < num_ssid; i++)
        {
            UBOOL8 locked_here = FALSE;
            INIT_PATH_DESCRIPTOR(&path_ssid);
            path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
            PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
            strncpy((char *)&path_ssid.paramName, "Name", sizeof(path_ssid.paramName));

            ret = get_param_from_pathDesc(&path_ssid, (char *)&buf, sizeof(buf));
            if (ret != WLMDM_OK)
            {
                return ret;
            }

            if (strcmp((char *)&buf, if_name) == 0)
            {
                if (cms_try_lock(&locked_here) != WLMDM_OK)
                {
                    return WLMDM_GENERIC_ERROR;
                }

                /*
                 * Search existing bridge port to see if the interface has already been added.
                 * If not found, we proceed to add bridge port for the interface.
                 * Otherwise, skip this interface to the next one.
                 */
                found = FALSE;
                INIT_INSTANCE_ID_STACK(&iidStack);
                while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                                   OGF_NO_VALUE_UPDATE,
                                                   (void **) &brObj)) == CMSRET_SUCCESS)
                {
                    if (strcmp(brObj->X_BROADCOM_COM_IfName, br_name) == 0)
                    {
                        INIT_INSTANCE_ID_STACK(&sub_iidStack);
                        while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                                                       &sub_iidStack, (void **)&brPortObj) == CMSRET_SUCCESS)
                        {
                            if ((!brPortObj->managementPort) && (0 == strcmp(brPortObj->name, if_name)))
                            {
                                found = TRUE;
                            }
                            cmsObj_free((void **)&brPortObj);
                        }
                        cmsObj_free((void **)&brObj);
                        break;
                    }
                    cmsObj_free((void **)&brObj);
                }

                if (found == TRUE)
                {
                    cmsLog_debug("%s already exists in bridge, skip", if_name);
                    cms_try_unlock(&locked_here);
                    continue;
                }

                /* Add the interface name to a new bridge port instance. */
                memset((char *)&path_ssid.paramName, 0x00, sizeof(path_ssid.paramName));
                if (cmsMdm_pathDescriptorToFullPathNoEndDot(&path_ssid, &full_path) != CMSRET_SUCCESS)
                {
                    cms_try_unlock(&locked_here);
                    ret = WLMDM_GENERIC_ERROR;
                    break;
                }

                if (rutBridge_addFullPathToBridge_dev2(full_path, br_name) != CMSRET_SUCCESS)
                {
                    CMSMEM_FREE_BUF_AND_NULL_PTR(full_path);
                    cms_try_unlock(&locked_here);
                    ret = WLMDM_GENERIC_ERROR;
                    break;
                }

                CMSMEM_FREE_BUF_AND_NULL_PTR(full_path);
                cms_try_unlock(&locked_here);

                /* update X_BROADCOM_COM_WlBrName of this SSID object */
                INIT_PATH_DESCRIPTOR(&path_ssid);
                path_ssid.oid = MDMOID_DEV2_WIFI_SSID;
                PUSH_INSTANCE_ID(&path_ssid.iidStack, i + 1);
                strncpy((char *)&path_ssid.paramName, "X_BROADCOM_COM_WlBrName", sizeof(path_ssid.paramName));

                ret = set_param_from_pathDesc(&path_ssid, br_name);
                if (ret != WLMDM_OK)
                {
                    break;
                }
            }
        }
        if_name = strtok_r(NULL, " ", &saveptr);
    }

    free(dup);
    return ret;
}

static WlmdmRet get_lan_ifnames(const char *nvname, char *value, size_t size)
{
    CmsRet cret;
    int bi, total = 0;;
    char br_name[BUFLEN_32] = {0};
    Dev2BridgeObject *brObj = NULL;
    Dev2BridgePortObject *brPortObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack sub_iidStack = EMPTY_INSTANCE_ID_STACK;
    UBOOL8 locked_here = FALSE;

    assert(value);
    if (0 == sscanf(nvname, "lan%d_ifnames", &bi))
    {
        bi = 0;
    }

    memset(value, 0x00, size);
    snprintf((char *)&br_name, sizeof(br_name), "br%d", bi);

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brObj)) == CMSRET_SUCCESS)
    {
        if (strcmp(brObj->X_BROADCOM_COM_IfName, br_name) == 0)
        {
            INIT_INSTANCE_ID_STACK(&sub_iidStack);
            while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                                           &sub_iidStack, (void **)&brPortObj) == CMSRET_SUCCESS)
            {
                if (!brPortObj->managementPort)
                {
                    strncat(value, brPortObj->name, size - total - 1);
                    total = strlen(value);
                    strncat(value, " ", size - total - 1);
                    total++;
                }
                cmsObj_free((void **)&brPortObj);
            }

            cmsObj_free((void **)&brObj);
            break;
        }
        cmsObj_free((void **)&brObj);
    }

    cms_try_unlock(&locked_here);

    return WLMDM_OK;
}

static WlmdmRet foreach_lan_ifnames(nvc_for_each_func foreach_func, void *data)
{
    CmsRet cret;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0};
    char value[MAX_NVRAM_VALUE_SIZE] = {0};
    UBOOL8 locked_here = FALSE;
    Dev2BridgeObject *brObj = NULL;
    Dev2BridgePortObject *brPortObj = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    InstanceIdStack sub_iidStack = EMPTY_INSTANCE_ID_STACK;

    if (cms_try_lock(&locked_here) != WLMDM_OK)
    {
        return WLMDM_GENERIC_ERROR;
    }

    while ((cret = cmsObj_getNextFlags(MDMOID_DEV2_BRIDGE, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &brObj)) == CMSRET_SUCCESS)
    {
        long int b;
        int bi, total;
        memset((void *)&nvname, 0x00, sizeof(nvname));
        memset((void *)&value, 0x00, sizeof(value));

        b = strtol((char *)&brObj->X_BROADCOM_COM_IfName[2], NULL, 10);
        if ((b < 0) || (b == LONG_MAX))
        {
            cmsLog_error("Failed to convert %s to proper long int value!", brObj->X_BROADCOM_COM_IfName[2]);
        }
        else
        {
            bi = (int) b;
            if (bi == 0)
            {
                strncpy(nvname, "lan_ifnames", sizeof(nvname) - 1);
            }
            else
            {
                snprintf(nvname, sizeof(nvname) - 1, "lan%d_ifnames", bi);
            }

            total = 0;
            INIT_INSTANCE_ID_STACK(&sub_iidStack);
            while (cmsObj_getNextInSubTree(MDMOID_DEV2_BRIDGE_PORT, &iidStack,
                                           &sub_iidStack, (void **)&brPortObj) == CMSRET_SUCCESS)
            {
                if (!brPortObj->managementPort)
                {
                    strncat(value, brPortObj->name, sizeof(value) - total - 1);
                    total = strlen(value);
                    strncat(value, " ", sizeof(value) - total - 1);
                    total++;
                }
                cmsObj_free((void **)&brPortObj);
            }
            foreach_func((char *)&nvname, (char *)&value, data);
        }
        cmsObj_free((void **)&brObj);
    }
    cms_try_unlock(&locked_here);
    return WLMDM_OK;
}

static WlmdmRet _set_auto_channel(unsigned int radio_index, UBOOL8 enable)
{
    MdmPathDescriptor pathDesc;
    WlmdmRet ret = 0;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, radio_index + 1);
    strncpy((char *)&pathDesc.paramName, "AutoChannelEnable", sizeof(pathDesc.paramName));

    if (enable == TRUE)
    {
        ret = set_param_from_pathDesc(&pathDesc, "1");
    }
    else
    {
        ret = set_param_from_pathDesc(&pathDesc, "0");
    }
    return ret;

}

static int _get_auto_channel(int radio_index)
{
    MdmPathDescriptor pathDesc;
    char buf[MAX_NVRAM_VALUE_SIZE];
    WlmdmRet ret;

    INIT_PATH_DESCRIPTOR(&pathDesc);
    pathDesc.oid = MDMOID_DEV2_WIFI_RADIO;
    PUSH_INSTANCE_ID(&pathDesc.iidStack, radio_index + 1);
    strncpy((char *)&pathDesc.paramName, "AutoChannelEnable", sizeof(pathDesc.paramName));

    ret = get_param_from_pathDesc(&pathDesc, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return -1;
    }
    return (int) strtol((char *)&buf, NULL, 10);
}

static WlmdmRet set_mbss_names(const char *nvname, const char *value)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_OK;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   char prefixArr[MAX_PREFIX_ENTR][MAX_PREFIX_LEN] = {0};
   int entries, i;
   UBOOL8 locked = FALSE;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   entries = get_prefixList(value, prefixArr);
   if(entries < 0)
      return WLMDM_GENERIC_ERROR;

   if (WLMDM_OK != cms_try_lock(&locked))
      return WLMDM_GENERIC_ERROR;

   /* first pass. clear useless wbd_cfg_mbss instances */
   while (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&mbssObj) == CMSRET_SUCCESS)
   {
      int match = 0;
      for(i=0; i < entries; i++)
         if (strcmp(prefixArr[i], mbssObj->prefix) == 0) // matched. do not delete.
            match = 1;

      if (!match)
      {
         ret = cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack);
         INIT_INSTANCE_ID_STACK(&iidStack);
      }
      cmsObj_free((void **)&mbssObj);
   }

   /* second pass. skip duplicated prefix. */
   INIT_INSTANCE_ID_STACK(&iidStack);
   while (cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE,
                              (void **)&mbssObj) == CMSRET_SUCCESS)
   {
      for(i=0; i < entries; i++)
      {
         if (strcmp(prefixArr[i], mbssObj->prefix) == 0) // matched. no need to add
         {
            memset(prefixArr[i], 0, MAX_PREFIX_LEN);
         }
      }
      cmsObj_free((void **)&mbssObj);
   }

   /* create newly added prefix as new instances */
   for(i=0; i < entries; i++)
   {
      if (strlen(prefixArr[i]) == 0)
         continue;

      INIT_INSTANCE_ID_STACK(&iidStack);
      if ((ret = cmsObj_addInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack)) != CMSRET_SUCCESS)
      {
         cms_try_unlock(&locked);
         return WLMDM_GENERIC_ERROR;
      }
      if ((ret = cmsObj_get(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack, OGF_NO_VALUE_UPDATE, (void **) &mbssObj)) != CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack);
         cms_try_unlock(&locked);
         return WLMDM_GENERIC_ERROR;
      }
      CMSMEM_REPLACE_STRING(mbssObj->prefix, prefixArr[i]);
      if ((ret = cmsObj_set((void *)mbssObj, &iidStack)) != CMSRET_SUCCESS)
      {
         cmsObj_deleteInstance(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack);
         mdmRet = WLMDM_GENERIC_ERROR;
      }
      cmsObj_free((void **)&mbssObj);
   }

   cms_try_unlock(&locked);
   return mdmRet;
}
static WlmdmRet get_mbss_names(const char *nvname, char *value, size_t size)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 locked = FALSE;
   size_t len = 0;

   if(!nvname || !value || (0 != strcmp(nvname, "map_bss_names")))
      return WLMDM_INVALID_PARAM;

   if (WLMDM_OK != cms_try_lock(&locked))
      return WLMDM_GENERIC_ERROR;

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
      snprintf(value + len, size - len, "%s ", mbssObj->prefix);
      cmsObj_free((void **)&mbssObj);
      len = strlen(value);
      if(len >= size)
         break;
   }
   if(len > 0)
      value[len-1] = '\0';

   cms_try_unlock(&locked);
   return WLMDM_OK;
}
static WlmdmRet foreach_mbss_names(nvc_for_each_func foreach_func, void *data)
{
   char val[1024]={0};

   if(foreach_func)
   {
      get_mbss_names("map_bss_names", val, 1024);
      foreach_func("map_bss_names", val, data);
   }

   return WLMDM_OK;
}

static WlmdmRet set_mapbss(const char *nvname, const char *value)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 locked = FALSE, found = FALSE;
   char *dup, *prefix, *suffix;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   dup = strdup(nvname);
   if(NULL == dup)
      return mdmRet;

   prefix = strtok_r(dup, "_", &suffix);

   if (WLMDM_OK != cms_try_lock(&locked))
   {
      free(dup);
      return mdmRet;
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
        if (strcmp(mbssObj->prefix, prefix) == 0)
        {
            found = TRUE;
            break;
        }
        cmsObj_free((void **)&mbssObj);
   }

   if(found)
   {
      if (cmsUtl_strcmp("ssid", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->ssid, value);
      else if (cmsUtl_strcmp("akm", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->akm, value);
      else if (cmsUtl_strcmp("crypto", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->crypto, value);
      else if (cmsUtl_strcmp("wpa_psk", suffix) == 0)
         CMSMEM_REPLACE_STRING(mbssObj->wpsPsk, value);
      else if (cmsUtl_strcmp("map", suffix) == 0)
         mbssObj->map = strtol(value, NULL, 0);
      else if (cmsUtl_strcmp("band_flag", suffix) == 0)
         mbssObj->bandFlag = strtol(value, NULL, 0);

      ret = cmsObj_set(mbssObj, &iidStack);
      cmsObj_free((void **)&mbssObj);
      mdmRet = WLMDM_OK;
   }
   else if(CMSRET_SUCCESS == ret)
      mdmRet = WLMDM_NOT_FOUND;

   cms_try_unlock(&locked);
   free(dup);

   return mdmRet;
}
static WlmdmRet get_mapbss(const char *nvname, char *value, size_t size)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 locked = FALSE, found = FALSE;
   char *dup, *prefix, *suffix;

   if(!nvname || !value)
      return WLMDM_INVALID_PARAM;

   dup = strdup(nvname);
   if(NULL == dup)
      return mdmRet;

   prefix = strtok_r(dup, "_", &suffix);

   if (WLMDM_OK != cms_try_lock(&locked))
   {
      free(dup);
      return mdmRet;
   }

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
        if (strcmp(mbssObj->prefix, prefix) == 0)
        {
            found = TRUE;
            break;
        }
        cmsObj_free((void **)&mbssObj);
   }

   if(found)
   {
      if(cmsUtl_strcmp("ssid", suffix) == 0)
         snprintf(value, size, "%s", mbssObj->ssid);
      else if(cmsUtl_strcmp("akm", suffix) == 0)
         snprintf(value, size, "%s", mbssObj->akm);
      else if(cmsUtl_strcmp("crypto", suffix) == 0)
         snprintf(value, size, "%s", mbssObj->crypto);
      else if(cmsUtl_strcmp("wpa_psk", suffix) == 0)
         snprintf(value, size, "%s", mbssObj->wpsPsk);
      else if(cmsUtl_strcmp("map", suffix) == 0)
         snprintf(value, size, "0x%x", mbssObj->map);
      else if (cmsUtl_strcmp("band_flag", suffix) == 0)
         snprintf(value, size, "0x%x", mbssObj->bandFlag);

      cmsObj_free((void **)&mbssObj);
      mdmRet = WLMDM_OK;
   }
   else if(CMSRET_SUCCESS == ret)
      mdmRet = WLMDM_NOT_FOUND;

   cms_try_unlock(&locked);
   free(dup);

   return mdmRet;
}
static UBOOL8 match_mapbss(const char *nvname)
{
   CmsRet ret = CMSRET_SUCCESS;
   Dev2WifiWbdCfgMbssObject *mbssObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 locked = FALSE, found = FALSE, match = FALSE;
   char *dup, *p, *next;
   char *ptn[]={"map", "band_flag", "ssid", "akm", "crypto", "wpa_psk"};
   char *ptn2 = "wl%d wl%d.%d";
   int nbr = sizeof(ptn)/sizeof(char *), i;

   if(!nvname)
      return FALSE;


   dup = strdup(nvname);
   if(NULL == dup)
      return FALSE;

   p = strtok_r(dup, "_", &next);
   if(0 == strlen(next))
   {
      free(dup);
      return FALSE;
   }

   for(i=0; i<nbr; i++)
   {
      if(0 == strcmp(next, ptn[i]))
      {
         /*Filter out wl0_map or wlx.y_map.*/
         if((0 != i) || !match_name(ptn2, p))
            match = TRUE;

         break;
      }
   }

   if(match)
   {
      match = FALSE;
      if (WLMDM_OK != cms_try_lock(&locked))
      {
         free(dup);
         return FALSE;
      }

      while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                          OGF_NO_VALUE_UPDATE,
                                          (void **) &mbssObj)) == CMSRET_SUCCESS)
      {
           if (strcmp(mbssObj->prefix, p) == 0)
           {
               found = TRUE;
               break;
           }
           cmsObj_free((void **)&mbssObj);
      }
      if(found)
      {
         match = TRUE;
         cmsObj_free((void **)&mbssObj);
      }

      cms_try_unlock(&locked);
   }

   free(dup);
   return match;
}
static WlmdmRet foreach_mapbss(nvc_for_each_func foreach_func, void *data)
{
   CmsRet ret = CMSRET_SUCCESS;
   WlmdmRet mdmRet = WLMDM_GENERIC_ERROR;  /*actually the caller does not care*/
   Dev2WifiWbdCfgMbssObject *mbssObj = NULL;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   UBOOL8 locked = FALSE;
   char name[MAX_NVRAM_NAME_SIZE] = {0};
   char value[MAX_NVRAM_VALUE_SIZE] = {0};

   if(!foreach_func)
      return WLMDM_INVALID_PARAM;

   if (WLMDM_OK != cms_try_lock(&locked))
      return mdmRet;

   while ((ret = cmsObj_getNextFlags(MDMOID_DEV2_WIFI_WBD_CFG_MBSS, &iidStack,
                                       OGF_NO_VALUE_UPDATE,
                                       (void **) &mbssObj)) == CMSRET_SUCCESS)
   {
      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_ssid", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->ssid);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_akm", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->akm);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_crypto", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->crypto);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_psk", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%s", mbssObj->wpsPsk);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_map", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%d", mbssObj->map);
      foreach_func(name, value, data);

      snprintf(name, MAX_NVRAM_NAME_SIZE, "%s_band_flag", mbssObj->prefix);
      snprintf(value, MAX_NVRAM_VALUE_SIZE, "%d", mbssObj->bandFlag);
      foreach_func(name, value, data);

      cmsObj_free((void **)&mbssObj);
   }

   cms_try_unlock(&locked);
   return WLMDM_OK;
}

static WlmdmRet set_nctrlsb(const char *nvname, const char *value)
{
    WlmdmRet ret;
    int radio_index, bssid_index;
    char buf[32]= {0};
    
    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = nvn_gen("mdmsideband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    return conv_set_mapped((char *)&buf, (char *)value);        
}


static WlmdmRet get_nctrlsb(const char *nvname, char *value, size_t size)
{

    WlmdmRet ret;
    struct chanspec_t chanspecbuf = {0};
    struct chanspec_t *chanspec=&chanspecbuf;
    int radio_index, bssid_index;
    char buf[32], buf_1[32];
    char *str = NULL ;
    int autoChannel = 0;
    long int b;
    unsigned int channel = 0;

    assert(nvname);
    assert(value);

    ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        cmsLog_error("Failed to parse nvname: %s", nvname);
        return ret;
    }

    ret = nvn_gen("mdmsideband", radio_index, -1, (char *)&buf, sizeof(buf));
    if (ret != WLMDM_OK)
    {
        return ret;
    }

    ret = conv_get_mapped((char *)&buf , (char *)value, size);
    if (ret != WLMDM_OK)
    {
        return WLMDM_INVALID_PARAM;    
    }

    cmsLog_debug("%s=%s", nvname,value);

    if(strcmp(value, "Auto"))
    { /* Control Sideband != "Auto" */
        return ret;
    }

    //Handle Control Sideband == "Auto"

    /* by default use lowest channel as control channel */
    chanspec->band_width = WL_CHANSPEC_BW_20;
    chanspec->ctlsb_index = 0;

    cmsLog_debug("Auto Control Sideband : %s=%s", nvname,value);

    do{
        ret = nvn_disassemble(nvname, &radio_index, &bssid_index, (char *)&buf, sizeof(buf));
        if (ret != WLMDM_OK)
        {
            cmsLog_error("Failed to parse nvname: %s", nvname);
            return ret;
        }

        ret = nvn_gen("bw", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
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

        autoChannel = _get_auto_channel(radio_index);
        if ((autoChannel != 0))
        { /* Nothing to do with AutoChannel + AutoSideBand */
            cmsLog_debug("No Channel from MDM");
            break;
        }

        ret = nvn_gen("channel", radio_index, -1, (char *)&buf, sizeof(buf));
        if (ret == WLMDM_OK)
        {
            ret = conv_get_mapped((char *)&buf, (char *)&buf_1, sizeof(buf_1));
            if (ret == WLMDM_OK)
            {
                b = strtol((char *)&buf_1, NULL, 10);
                if ((b < 0) || (b == LONG_MAX))
                {
                    cmsLog_error("Failed to convert %s to proper long int value!", buf_1);
                    break;
                }
                else
                {
                    channel = (unsigned int) b;
                }
            }
        }

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

        /* Handle Auto Side Band Select */
        switch (chanspec->band_width)
        {
            case WL_CHANSPEC_BW_40:
                /* Auto side band base on channel on 40Mhz
                   CH 1 -->  7 => "lower"
                   CH 8 --> 14 => "upper"
                   For 5G : use lowest channel as control channel
                */
                if(chanspec->band == WL_CHANSPEC_BAND_2G)
                {
                    chanspec->ctlsb_index = chanspec_get_sb_index((channel < 8) ? "lower" : "upper");
                    cmsLog_debug("Auto sideband CH:%d  SB:%d",channel,chanspec->ctlsb_index);
                }else
                {
                    /* use lowest channel as control channel */
                    chanspec->ctlsb_index = 0; 
                }
                break;
    
            default:
            case WL_CHANSPEC_BW_80:
            case WL_CHANSPEC_BW_160:
                /* use lowest channel as control channel */
                chanspec->ctlsb_index = 0; 
                break;
        }

    }while(0);

    if(str = chanspec_get_sb_string(chanspec->ctlsb_index, chanspec->band_width))
    {
        strncpy(value, str, size - 1);
        ret = WLMDM_OK;
    }else
    {
        ret = WLMDM_GENERIC_ERROR;
    }

    return ret;
}

static WlmdmRet foreach_nctrlsb(nvc_for_each_func foreach_func, void *data)
{
    int i, num_radio;
    char nvname[MAX_NVRAM_NAME_SIZE] = {0}, value[MAX_NVRAM_VALUE_SIZE] = {0};
    WlmdmRet ret;

    num_radio = get_num_instances(MDMOID_DEV2_WIFI_RADIO);
    for (i = 0; i < num_radio; i++)
    {
        ret = nvn_gen("mdmsideband", i, -1, (char *)&nvname, sizeof(nvname));
        if (ret == WLMDM_OK)
        {
            ret = get_nctrlsb(nvname,&value,MAX_NVRAM_VALUE_SIZE);
            if (ret == WLMDM_OK)
            {
                ret = nvn_gen("nctrlsb", i, -1, (char *)&nvname, sizeof(nvname));
                foreach_func(nvname, value, data);
            }
        }
    }
    return WLMDM_OK;
}
