/***********************************************************************
 *
 *  Copyright (c) 2008-2010  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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

#include "httpd.h"
#include "cgi_main.h"
#include <board.h>

#if defined(CONFIG_BCM96858) || defined(CONFIG_BCM96856)
#define TENG
#endif

static char *types[] = {
    "Auto detect",
    "GBE",
#ifdef TENG
    "XGS-PON",
    "NGPON2",
    "XGPON",
#endif
    "GPON",
    "EPON",
};

/* Must be in the order of 'types' */
static char *types_scratchpad[] = {
    RDPA_WAN_TYPE_VALUE_AUTO,
    RDPA_WAN_TYPE_VALUE_GBE,
#ifdef TENG
    RDPA_WAN_TYPE_VALUE_XGS,
    RDPA_WAN_TYPE_VALUE_NGPON2,
    RDPA_WAN_TYPE_VALUE_XGPON1,
#endif
    RDPA_WAN_TYPE_VALUE_GPON,
    RDPA_WAN_TYPE_VALUE_EPON,
};

/* Must be in the order of 'types' */
static char *rates_supported[][10] = {
    { 0 },                 /* RDPA_WAN_TYPE_VALUE_AUTO, */
    {                      /* RDPA_WAN_TYPE_VALUE_GBE, */
#ifdef TENG
     "1010",
#endif
     "0101", 0},

#ifdef TENG
    { "1010", 0},          /* RDPA_WAN_TYPE_VALUE_XGS, */
#endif

#ifdef TENG
    {"1010", "1025", 0},   /* RDPA_WAN_TYPE_VALUE_NGPON2, */
#endif

#ifdef TENG
    {"1025", 0},           /* RDPA_WAN_TYPE_VALUE_XGPON1, */
#endif
    
    {"2510", 0},           /* RDPA_WAN_TYPE_VALUE_GPON, */

    {                      /* RDPA_WAN_TYPE_VALUE_EPON, */
#ifdef TENG
    "1010", "1001",
#endif
     "0201", "0101", 0},
};

char *rate_str_to_nice_str(char *rate_str)
{
    if (!strcmp("1010", rate_str))
        return "10/10";

    if (!strcmp("1001", rate_str))
        return "10/1";

    if (!strcmp("1025", rate_str))
        return "10/2.5";

    if (!strcmp("2510", rate_str))
        return "2.5/1";

    if (!strcmp("0101", rate_str))
        return "1/1";

    if (!strcmp("0201", rate_str))
        return "2/1";

    return rate_str;
}

static int emacs_supported = 
#if defined(CONFIG_BCM96838)
6
#elif defined(CONFIG_BCM96848)
6
#elif defined(CONFIG_BCM96846)
5
#elif defined(CONFIG_BCM96856)
6
#elif defined(CONFIG_BCM96858)
7
#elif defined(CONFIG_BCM96878)
5
#else
#error "Need to add number of emacs on platform"
#endif
;

void cgiWanSelectCfg(char *query, FILE *fs) 
{
    char buf[BUFLEN_32] = "";
    char *type = "", *rate = "";
    int type_id, i, dualwan = 0;
    CmsRet ret;

    cgiGetValueByName(query, "type", buf);
    type_id = atoi(buf);
    if (type_id >= 0 && type_id < (int)sizeof(types))
        type = types_scratchpad[type_id]; 
    else
    {
        ret = -1;
        goto Exit;
    }

    cgiGetValueByName(query, "rate", buf);
    i = atoi(buf);
    if (i >= 0)
        rate = rates_supported[type_id][i];

    cgiGetValueByName(query, "dualwan", buf);
    i = atoi(buf);
    if (i >= 0)
        dualwan = i;

    cgiGetValueByName(query, "emac", buf);
    i = atoi(buf);
    if (i >= 0)
        sprintf(buf, "EMAC%d", i);

#ifdef XRDP
    if (i == emacs_supported)
        strcpy(buf, "EPONMAC");
#endif
    if (i == emacs_supported + 1)
        strcpy(buf, "NONE");

    if (dualwan && (ret = cmsPsp_set(RDPA_GBE_WAN_EMAC_PSP_KEY, buf, strlen(buf))))
    {
        goto Exit;
    }

    if ((ret = cmsPsp_set(RDPA_WAN_OEMAC_PSP_KEY, buf, strlen(buf))))
    {
        goto Exit;
    }

    if ((ret = cmsPsp_set(RDPA_WAN_TYPE_PSP_KEY, type, strlen(type))))
    {
        goto Exit;
    }

    if ((ret = cmsPsp_set(RDPA_WAN_RATE_PSP_KEY, rate, strlen(rate))))
    {
        goto Exit;
    }

Exit:
    cgiGetCurrSessionKey(0, NULL, NULL);
    fprintf(fs, "%d/%d", ret, glbCurrSessionKey);
    fflush(fs);
}

void cgiWanSelectData(int argc __attribute__((unused)), char **argv __attribute__((unused)), char *varValue)
{
    char buf[BUFLEN_32] = "";
    char emac_selected[10] = "";
    char emacs[256] = "";
    char rate_selected[10] = "";
    char *wan_selected = "";
    char rates[400] = "";
    int dualwan = 0;
    int ret, i, r;

    for (i = 0; i < emacs_supported; i++)
        sprintf(strchr(emacs, 0), "'EMAC%d',", i);
#ifdef XRDP
    sprintf(strchr(emacs, 0), "'EPONMAC',");
#endif
    sprintf(strchr(emacs, 0), "'NONE',");

    for (i = 0; i < (int)(sizeof(types)/sizeof(types[0])); i++)
    {
        sprintf(strchr(rates, 0), "'%s': [\n", types[i]);
        for (r = 0; rates_supported[i][r] != 0; r++)
            sprintf(strchr(rates, 0), "'%s', ", rate_str_to_nice_str(rates_supported[i][r]));
        sprintf(strchr(rates, 0), "],\n");
    }

    if ((ret = cmsPsp_get(RDPA_GBE_WAN_EMAC_PSP_KEY, buf, sizeof(buf)-1)) > 0)
    {
        buf[ret] = '\0';
        if (strcmp("", buf) && strcmp("NONE", buf))
            dualwan = 1;
    }

    if ((ret = cmsPsp_get(RDPA_WAN_OEMAC_PSP_KEY, buf, sizeof(buf)-1)) > 0)
    {
        buf[ret] = '\0';
        strncpy(emac_selected, buf, ret);
    }

    if ((ret = cmsPsp_get(RDPA_WAN_TYPE_PSP_KEY, buf, sizeof(buf)-1)) > 0)
    {
        buf[ret] = '\0';
        for (i = 0; i < sizeof(types)/sizeof(types[0]); i++)
        {
            if (!strcmp(types_scratchpad[i], buf))
                wan_selected = types[i];
        }
    }

    if ((ret = cmsPsp_get(RDPA_WAN_RATE_PSP_KEY, buf, sizeof(buf)-1)) > 0)
    {
        buf[ret] = '\0';
        strncpy(rate_selected, buf, ret);
    }
    
    sprintf(varValue, "{"
        "ae_emac: '%d',"
        "rate_selected: '%s',"
        "wan_selected: '%s',"
        "emac_selected: '%s',"
        "rates: {"
        "%s"
        "},"
        "dualwan: %d,"
        "emacs: [ %s ],"
        "}", emacs_supported, rate_str_to_nice_str(rate_selected), wan_selected, emac_selected, rates, dualwan, emacs);
}

