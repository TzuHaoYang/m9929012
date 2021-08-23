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

#ifndef _bcmwifi_channels_h_
#define _bcmwifi_channels_h_

#include "os_defs.h"
#include "wlmdm.h"

typedef struct chanspec_t
{
    unsigned int band;
    unsigned int band_width;
    unsigned int ctlsb_index;             //control side band index
    unsigned int channel;                 //center channel number
    unsigned int chan1_index;             //used for 80+80MHZ case
    unsigned int chan2_index;             //used for 80+80MHZ case
} chanspec_t;

/* channel defines */
enum
{
    CH_UPPER_SB = 0x01,
    CH_LOWER_SB = 0x02
};

enum
{
    CH_EWA_VALID = 0x04
};

enum
{
    CH_5MHZ_APART  = 1,
    CH_10MHZ_APART = 2,
    CH_20MHZ_APART = 4,
    CH_40MHZ_APART = 8,
    CH_80MHZ_APART = 16,
    CH_160MHZ_APART = 32,
};

enum
{
    CH_MIN_2G_CHANNEL = 1,        /* Min channel in 2G band */
    CH_MAX_2G_CHANNEL = 14,       /* Max channel in 2G band */
    CH_MIN_5G_CHANNEL = 36,       /* Min channel in 5G band */
    CH_MIN_6G_CHANNEL = 1,        /* Min channel in 6G band */
    CH_MAX_6G_CHANNEL = 233,      /* Max channel in 6G band */
    CH_MIN_6G_40M_CHANNEL = 3,    /* Min 40MHz center channel in 6G band */
    CH_MAX_6G_40M_CHANNEL = 227,  /* Max 40MHz center channel in 6G band */
    CH_MIN_6G_80M_CHANNEL = 7,    /* Min 80MHz center channel in 6G band */
    CH_MAX_6G_80M_CHANNEL = 215,  /* Max 80MHz center channel in 6G band */
    CH_MIN_6G_160M_CHANNEL = 15,  /* Min 160MHz center channel in 6G band */
    CH_MAX_6G_160M_CHANNEL = 207, /* Max 160MHz center channel in 6G band */
};

/* maximum # channels the s/w supports.
 * this is + 1 rounded up to a multiple of NBBY (8).
 * DO NOT MAKE it > 255: channels are uint8's all over
 */
enum
{
    MAXCHANNEL = 224
};

enum WL_CHANSPEC_CTL_SB_160MHZ
{
    WL_CHANSPEC_CTL_SB_LLL = 0,
    WL_CHANSPEC_CTL_SB_LLU = 1,
    WL_CHANSPEC_CTL_SB_LUL = 2,
    WL_CHANSPEC_CTL_SB_LUU = 3,
    WL_CHANSPEC_CTL_SB_ULL = 4,
    WL_CHANSPEC_CTL_SB_ULU = 5,
    WL_CHANSPEC_CTL_SB_UUL = 6,
    WL_CHANSPEC_CTL_SB_UUU = 7,
};

enum WL_CHANSPEC_CTL_SB_80MHZ
{
    WL_CHANSPEC_CTL_SB_LL = 0,
    WL_CHANSPEC_CTL_SB_LU = 1,
    WL_CHANSPEC_CTL_SB_UL = 2,
    WL_CHANSPEC_CTL_SB_UU = 3
};

enum WL_CHANSPEC_CTL_SB_40MHZ
{
    WL_CHANSPEC_CTL_SB_L = 0,
    WL_CHANSPEC_CTL_SB_U = 1
};

#define WL_CHANSPEC_CTL_SB_NONE     WL_CHANSPEC_CTL_SB_LLL

struct sb_mapper
{
    int sb;
    const char *nval;
};

enum WL_CHANSPEC_BAND
{
    WL_CHANSPEC_BAND_2G = 0,
    WL_CHANSPEC_BAND_4G = 1,
    WL_CHANSPEC_BAND_5G = 2,
    WL_CHANSPEC_BAND_6G = 3
};

typedef enum WL_CHANSPEC_BW {
    WL_CHANSPEC_BW_5 = 0,
    WL_CHANSPEC_BW_10 = 1,
    WL_CHANSPEC_BW_20 = 2,
    WL_CHANSPEC_BW_40 = 3,
    WL_CHANSPEC_BW_80 = 4,
    WL_CHANSPEC_BW_160 = 5,
    WL_CHANSPEC_BW_8080 = 6,
    WL_CHANSPEC_BW_2P5 = 7
} WL_CHANSPEC_BW;

enum WF_NUM_SIDEBANDS
{
    /**
     *  No of sub-band vlaue of the specified Mhz chanspec
     */
    WF_NUM_SIDEBANDS_40MHZ = 2,
    WF_NUM_SIDEBANDS_80MHZ = 4,
    WF_NUM_SIDEBANDS_8080MHZ = 4,
    WF_NUM_SIDEBANDS_160MHZ = 8
};


enum
{
    CHANSPEC_STR_LEN = 20
};

#define WLC_BW_20MHZ_BIT        (1<<0)
#define WLC_BW_40MHZ_BIT        (1<<1)
#define WLC_BW_80MHZ_BIT        (1<<2)
#define WLC_BW_160MHZ_BIT       (1<<3)

/* Bandwidth capabilities */
#define WLC_BW_CAP_20MHZ        (WLC_BW_20MHZ_BIT)
#define WLC_BW_CAP_40MHZ        (WLC_BW_40MHZ_BIT | WLC_BW_CAP_20MHZ)
#define WLC_BW_CAP_80MHZ        (WLC_BW_80MHZ_BIT | WLC_BW_CAP_40MHZ)
#define WLC_BW_CAP_160MHZ       (WLC_BW_160MHZ_BIT | WLC_BW_CAP_80MHZ)

#define WL_BW_CAP_20MHZ(bw_cap)    (((bw_cap) & WLC_BW_20MHZ_BIT) ? TRUE : FALSE)
#define WL_BW_CAP_40MHZ(bw_cap)    (((bw_cap) & WLC_BW_40MHZ_BIT) ? TRUE : FALSE)
#define WL_BW_CAP_80MHZ(bw_cap)    (((bw_cap) & WLC_BW_80MHZ_BIT) ? TRUE : FALSE)
#define WL_BW_CAP_160MHZ(bw_cap)   (((bw_cap) & WLC_BW_160MHZ_BIT) ? TRUE : FALSE)

/**
 * Convert chanspec string representation to chanspec_t data.
 *
 * @param   a     pointer to input string
 *
 * @returns 0 if we successfully parsed chanspec from input string.
 *  returns -1 otherwise
 */
WlmdmRet chanspec_parse_string(const char *a, chanspec_t *chanspec);

/**
 * Verify the chanspec fields are valid.
 *
 * Verify the chanspec is using a legal set field values, i.e. that the chanspec
 * specified a band, bw, ctl_sb and channel and that the combination could be
 * legal given some set of circumstances.
 *
 * @param   chanspec   input chanspec to verify
 *
 * @return FALSE if the chanspec is malformed, TRUE if it looks good.
 */
UBOOL8 chanspec_isvalid(chanspec_t *chanspec);


WlmdmRet chanspec_to_string(chanspec_t *chspec, char *buf, size_t size);
unsigned int chanspec_channel(chanspec_t *chspec);
unsigned int chanspec_primary80_channel(chanspec_t *chanspec);
unsigned int chanspec_ctlchan(chanspec_t *chspec);
const char *chanspec_to_bw_str(chanspec_t *chspec);
const char *chanspec_get_sb_string(unsigned int sb_index, WL_CHANSPEC_BW bw);
int chanspec_get_sb_index(const char *nval);

#endif
