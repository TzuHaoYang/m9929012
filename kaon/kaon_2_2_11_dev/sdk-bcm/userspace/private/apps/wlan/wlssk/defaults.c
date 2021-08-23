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

#include "wlssk.h"
#include "bcmnvram.h"
#include "wlsysutil.h"
#include "nvram_api.h"
#ifdef RDK_BUILD
#include <wlioctl_defs.h>
#endif

extern int act_wl_cnt;

struct nvrams defaults[] = {
    { "ure_disable",     "1", NVRAM_FLAG_NORMAL },
    { "router_disable",  "0", NVRAM_FLAG_NORMAL },
    { "hide_hnd_header", "y", NVRAM_FLAG_NORMAL },
#ifdef __CONFIG_VISUALIZATION__
    { "wlVisualization", "1", NVRAM_FLAG_NORMAL }, /* enable wifi insight page */
#endif
#ifdef __CONFIG_HSPOT__
    { "wlPassPoint", "1",  NVRAM_FLAG_NORMAL },    /* enable Passpoint web page */

	{ "oplist",		"Wi-Fi Alliance!eng|"
	"\x57\x69\x2d\x46\x69\xe8\x81\x94\xe7\x9b\x9f!chi", NVRAM_FLAG_PER_SSID }, /* Operator Friendly Name List */

	{ "osu_frndname",   "SP Red Test Only!eng|"
	"\x53\x50\x20\xEB\xB9\xA8\xEA\xB0\x95\x20\xED\x85\x8C"
	"\xEC\x8A\xA4\xED\x8A\xB8\x20\xEC\xA0\x84\xEC\x9A\xA9!kor", NVRAM_FLAG_PER_SSID}, /* OSU Friendly Name */

	{ "osu_servdesc",   "Free service for test purpose!eng|"
	"\xED\x85\x8C\xEC\x8A\xA4\xED\x8A\xB8\x20\xEB\xAA\xA9"
	"\xEC\xA0\x81\xEC\x9C\xBC\xEB\xA1\x9C\x20\xEB\xAC\xB4"
	"\xEB\xA3\x8C\x20\xEC\x84\x9C\xEB\xB9\x84\xEC\x8A\xA4!kor", NVRAM_FLAG_PER_SSID}, /* OSU Serv Desc */

	{ "concaplist", "1:0:0;6:20:1;6:22:0;"
	"6:80:1;6:443:1;6:1723:0;6:5060:0;"
	"17:500:1;17:5060:0;17:4500:1;50:0:1", NVRAM_FLAG_PER_SSID }, /* Connection Capability List */

    { "qosmapie",   "35021606+8,15;0,7;255,255;16,31;32,39;255,255;40,47;255,255",  NVRAM_FLAG_PER_SSID }, /* QoS Map IE */

	{ "ouilist",    "506F9A:1;001BC504BD:1", NVRAM_FLAG_PER_SSID },	/* Roaming Consortium List */
#endif  /* __CONFIG_HSPOT__ */
    { "bw",     "1", NVRAM_FLAG_PER_RADIO },
    { "bw_cap",     "1", NVRAM_FLAG_PER_RADIO },
#ifdef BUILD_HND_EAP
    { "rxchain_pwrsave_enable", "0",   NVRAM_FLAG_PER_RADIO },
    { "dtim",                   "3",   NVRAM_FLAG_PER_RADIO },
    { "maxassoc",               "512", NVRAM_FLAG_PER_RADIO },
    { "bss_maxassoc",           "512", NVRAM_FLAG_PER_SSID  },
#endif /* BUILD_HND_EAP */
#ifdef WLTEST
    { "chanspec", "11",   NVRAM_FLAG_PER_RADIO },
#else
    { "chanspec", "0",   NVRAM_FLAG_PER_RADIO },
#endif
    {0, 0, 0}
};

static void set_defaults_per_radio(struct nvrams nv)
{
    int i;
    char nvname[NVNAME_SIZE];

    for (i = 0 ; i < act_wl_cnt ; i++)
    {
        snprintf(nvname, NVNAME_SIZE, "wl%d_%s", i, nv.name);
        nvram_set(nvname, nv.value);
    }
}

static void set_defaults_per_ssid(struct nvrams nv)
{
    int i, j;
    char nvname[NVNAME_SIZE];

    for (i = 0 ; i < act_wl_cnt ; i++)
    {
        int numSSID;

        numSSID = wlgetVirtIntfNo(i);
        snprintf(nvname, NVNAME_SIZE, "wl%d_%s", i, nv.name);
        nvram_set(nvname, nv.value);

        for (j = 1 ; j < numSSID ; j++)
        {
            snprintf(nvname, NVNAME_SIZE, "wl%d.%d_%s", i, j, nv.name);
            nvram_set(nvname, nv.value);
        }
    }
}


void set_nvram_defaults(void)
{
    int i = 0;

    for (i = 0 ; defaults[i].name != NULL ; i++)
    {
        switch (defaults[i].flag)
        {
        case NVRAM_FLAG_NORMAL:
            nvram_set(defaults[i].name, defaults[i].value);
            break;
        case NVRAM_FLAG_PER_RADIO:
            set_defaults_per_radio(defaults[i]);
            break;
        case NVRAM_FLAG_PER_SSID:
            set_defaults_per_ssid(defaults[i]);
            break;
        default:
            break;
        }
    }
#ifdef RDK_BUILD
    {
        /* RDK_BUILD assume wl0 is on band 2G and wl1 on 5G
         * it is assumed already so, thus no band support check
         * here. If indeed wl0 is single band on 5G, need to use
         * kernel nvram or environment variable "wl_unitlist" to
         * switch radios to right order and meet the 2G/5G requirment.
         */
       char buf[32];
       snprintf(buf,32,"%d",WLC_BAND_2G);
       nvram_set("wl0_nband",buf);
       snprintf(buf,32,"%d",WLC_BAND_5G);
       nvram_set("wl1_nband",buf);
    }
#endif
}
