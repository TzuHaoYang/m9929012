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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "cms_util.h"
#include "chanspec.h"


static int channel_80mhz_to_index(unsigned int ch);
static unsigned int center_chan_to_edge(unsigned int bw);
static unsigned int channel_low_edge(unsigned int center_ch, unsigned int bw);
static int channel_to_sb(unsigned int center_ch, unsigned int ctrl_ch, unsigned int bw);
static unsigned int upper_20_sb(unsigned int channel);
static unsigned int lower_20_sb(unsigned int channel);
static unsigned int convert_bw(unsigned int bw);
static UBOOL8 chspec_sb_upper(chanspec_t *chspec);
static unsigned int bw_chspec_to_mhz(chanspec_t *chspec);
static unsigned int channel_to_ctl_chan(unsigned int center_ch, unsigned int bw, unsigned int sb);
static int search_sb_mapper_table(struct sb_mapper *sb_mapper_table, size_t size, const char *nval);

static int read_uint(const char **p, unsigned int *num);
static int read_char(const char **p, char *c);
static char peek_next(const char *p);
static const char *consume_char(const char *p);
static const char *consume_string(const char *p, const char *str);
static void dump_chanspec(struct chanspec_t *chanspec);

static struct sb_mapper sb_mapper_40MHZ[] =
{
    {.sb = 0,  .nval = "lower"},
    {.sb = 1,  .nval = "upper"}
};
enum { SB_MAPPER_40MHZ_SIZE = sizeof(sb_mapper_40MHZ) / sizeof(struct sb_mapper) };

static struct sb_mapper sb_mapper_80MHZ[] =
{
    {.sb = 0,  .nval = "ll"},
    {.sb = 1,  .nval = "lu"},
    {.sb = 2,  .nval = "ul"},
    {.sb = 3,  .nval = "uu"},
};
enum { SB_MAPPER_80MHZ_SIZE = sizeof(sb_mapper_80MHZ) / sizeof(struct sb_mapper) };

static struct sb_mapper sb_mapper_160MHZ[] =
{
    {.sb = 0,  .nval = "lll"},
    {.sb = 1,  .nval = "llu"},
    {.sb = 2,  .nval = "lul"},
    {.sb = 3,  .nval = "luu"},
    {.sb = 4, .nval = "ull"},
    {.sb = 5, .nval = "ulu"},
    {.sb = 6, .nval = "uul"},
    {.sb = 7, .nval = "uuu"}
};
enum { SB_MAPPER_160MHZ_SIZE = sizeof(sb_mapper_160MHZ) / sizeof(struct sb_mapper) };


/* Definitions for D11AC capable Chanspec type */

/* Chanspec ASCII representation with 802.11ac capability:
 * [<band>'g']<channel>['/'<bandwidth>[<primary-sideband>]['/'<1st80channel>'-'<2nd80channel>]]
 * In above syntax, [] means optional. <> means required, if its outer element is present.
 * '' quotes a real character to be matched. Other names not being quoted are defined as below:
 *
 * band:
 *      in regex, it is defined as:
 *      [2345]
 *      2, 3, 4, 5 for 2.4GHz, 3GHz, 4GHz, and 5GHz respectively.
 * channel:
 *      channel number of the 5MHz, 10MHz, 20MHz bandwidth channel,
 *      or primary channel number of the 40MHz, 80MHz, 160MHz, or 80+80MHz bandwidth channel.
 *      For example, for 2.4GHz band, channel can be any value from 1 to 11.
 * bandwidth:
 *      in regex, it is defined as:
 *      (5|10|20|40|80|160|80\+80)
 *      If this field is not present, the default value is 20.
 * primary-sideband:
 *      in regex, it is defined as:
 *      (U|L|u|l)
 *      U or u stands for upper sideband primary, L or l stands for lower sideband primary.
 *      This field is only valid for 2.4GHz band with 40MHz channel bandwidth.
 *      For 2.4GHz band with 40MHz channel, the same primary channel may be the
 *      upper sideband for one 40MHz channel, and the lower sideband for an
 *      overlapping 40MHz channel.  The U/L disambiguates which 40MHz channel
 *      is being specified.
 *      For 40MHz in the 5GHz band and all channel bandwidths greater than
 *      40MHz, the U/L specificaion is not allowed since the channels are
 *      non-overlapping and the primary sub-band is derived from its
 *      position in the wide bandwidth channel.
 * 1st80Channel:
 * 2nd80Channel:
 *      Required for 80+80, otherwise not allowed.
 *      Specifies the center channel of the first and second 80MHz band for 80+80 Bandwidth case.
 *
 * Parsing Rules:
 *     1. We parse chanspec string from LEFT to RIGHT.
 *     2. Once the format can't be analysed correctly, the parsing would fail.
 *     3. If band is not present in chanspec string, we must have channel reading. In this case,
 *        we set band to be 2 if channel <= 14, otherwise we set band to be 5.
 *
 * Examples:
 * In its simplest form, it is a 20MHz channel number: "20", with the implied band
 * to be 5GHz, because the channel number is greater than 14.
 *
 * To allow for backward compatibility with scripts, the old form for
 * 40MHz channels is also allowed: <channel><primary-sideband>
 *
 * 5GHz band Examples:
 *      Chanspec        BW        Center Ch  Channel Range  Primary Ch
 *      5g8             20MHz     8          -              -
 *      52              20MHz     52         -              -
 *      52/40           40MHz     54         52-56          52
 *      56/40           40MHz     54         52-56          56
 *      52/80           80MHz     58         52-64          52
 *      56/80           80MHz     58         52-64          56
 *      60/80           80MHz     58         52-64          60
 *      64/80           80MHz     58         52-64          64
 *      52/160          160MHz    50         36-64          52
 *      36/160          160MGz    50         36-64          36
 *      36/80+80/42-106 80+80MHz  42,106     36-48,100-112  36
 *
 * 2.4GHz band Examples:
 *      Chanspec        BW        Center Ch  Channel Range  Primary Ch
 *      2g8             20MHz     8          -              -
 *      8               20MHz     8          -              -
 *      6               20MHz     6          -              -
 *      6/40l           40MHz     8          6-10           6
 *      6l              40MHz     8          6-10           6
 *      6/40u           40MHz     4          2-6            6
 *      6u              40MHz     4          2-6            6
 */

static const unsigned int wf_chspec_bw_mhz[] = {5, 10, 20, 40, 80, 160, 160};

enum{
    WF_NUM_BW = sizeof(wf_chspec_bw_mhz) / sizeof(unsigned int)
};

/* bandwidth ASCII string */
static const char *wf_chspec_bw_str[] =
{
    "5",
    "10",
    "20",
    "40",
    "80",
    "160",
    "80+80",
#ifdef WL11ULB
    "2.5"
#else /* WL11ULB */
    "na"
#endif /* WL11ULB */
};

/* 40MHz channels in 5GHz band */
static const unsigned int wf_5g_40m_chans[] = {38, 46, 54, 62, 102, 110, 118, 126, 134, 142, 151, 159, 167, 175};
enum
{
    WF_NUM_5G_40M_CHANS = sizeof(wf_5g_40m_chans) / sizeof(unsigned int)
};

/* 80MHz channels in 5GHz band */
static const unsigned int wf_5g_80m_chans[] = {42, 58, 106, 122, 138, 155, 171};
enum
{
    WF_NUM_5G_80M_CHANS = sizeof(wf_5g_80m_chans) / sizeof(unsigned int)
};

/* 160MHz channels in 5GHz band */
static const unsigned int wf_5g_160m_chans[] = {50, 114, 163};
enum
{
    WF_NUM_5G_160M_CHANS = sizeof(wf_5g_160m_chans) / sizeof(unsigned int)
};

struct bw_mapping
{
    unsigned int bw;
    unsigned int spec_bw;
};

struct bw_mapping bw_map[] =
{
    {2,    WL_CHANSPEC_BW_2P5},
    {5,    WL_CHANSPEC_BW_5},
    {10,   WL_CHANSPEC_BW_10},
    {20,   WL_CHANSPEC_BW_20},
    {40,   WL_CHANSPEC_BW_40},
    {80,   WL_CHANSPEC_BW_80},
    {160,  WL_CHANSPEC_BW_160},
};

enum
{
    bw_map_size = sizeof(bw_map) / sizeof(struct bw_mapping)
};

typedef enum ParseState
{
    STATE_START,
    STATE_BAND,
    STATE_CHANNEL,
    STATE_BANDWIDTH,
    STATE_DONE
} ParseState;

/*
 * Given a chanspec string, convert it to a 16-bit chanspec representation.
 */
WlmdmRet chanspec_parse_string(const char *a, chanspec_t *chanspec)
{
    unsigned int chspec_ch, chspec_chan1_index, chspec_chan2_index, chspec_band, chspec_bw, chspec_sb;
    unsigned int num, bw, ctrl_ch;
    unsigned int ch1, ch2;
    char c, sb_ul = '\0';
    int i;
    ParseState state;

    assert(a);

    chspec_ch = 0;
    ctrl_ch = 0;
    chspec_bw = WL_CHANSPEC_BW_20;
    chspec_band = WL_CHANSPEC_BAND_2G;
    chspec_chan1_index = 0;
    chspec_chan2_index = 0;
    chspec_sb = 0;
    ch1 = ch2 = 0;
    bw = 20;
    state = STATE_START;

    while (state != STATE_DONE)
    {
        switch (state)
        {
            case STATE_START:
                /* parse channel num or band */
                if (read_uint(&a, &num) == -1)
                {
                    return WLMDM_GENERIC_ERROR;
                }

                if ('g' == peek_next(a))
                {
                    state = STATE_BAND;
                }
                else
                {
                    state = STATE_CHANNEL;
                }
                break;

            case STATE_BAND:
                /*Consume 'g' */
                a = consume_char(a);
                /* band must be "2", "5" or "6" */
                switch (num) {
                case 2:
                    chspec_band = WL_CHANSPEC_BAND_2G;
                    break;
                case 5:
                    chspec_band = WL_CHANSPEC_BAND_5G;
                    break;
                case 6:
                    chspec_band = WL_CHANSPEC_BAND_6G;
                    break;
                default:
                    return WLMDM_GENERIC_ERROR;
                }

                if (read_uint(&a, &num) == -1)
                {
                    return WLMDM_GENERIC_ERROR;
                }
                state = STATE_CHANNEL;
                break;

            case STATE_CHANNEL:
                ctrl_ch = num;
                if (chspec_band == 0)
                {
                    chspec_band = (ctrl_ch <= CH_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G;
                }

                if (read_char(&a, &c) == -1)
                {
                    chspec_bw = WL_CHANSPEC_BW_20;
                    state = STATE_DONE;
                }
                else
                {
                    if ((c == 'u') || (c == 'l'))
                    {
                        sb_ul = c;
                        chspec_bw = WL_CHANSPEC_BW_40;
                        state = STATE_DONE;
                    }
                    else if (c == '/')
                    {
                        if (read_uint(&a, &bw) == -1)
                        {
                            return WLMDM_GENERIC_ERROR;
                        }
                        state = STATE_BANDWIDTH;
                    }
                    else
                    {
                        cmsLog_error("Don't know how to handle %c", c);
                        return WLMDM_GENERIC_ERROR;
                    }
                }
                break;

            case STATE_BANDWIDTH:
                chspec_bw = convert_bw(bw);
                if (chspec_bw == 0)
                {
                    return WLMDM_GENERIC_ERROR;
                }

                c = peek_next(a);
                if (c == '\0')
                {
                    state = STATE_DONE;
                }
                else if (c == 'u' || c == 'l')
                {
                    a = consume_char(a);
                    sb_ul = c;
                    if ((chspec_band == WL_CHANSPEC_BAND_2G) && (bw == 40))
                    {
                        state = STATE_DONE;
                    }
                }
                else if (c == '+')
                {
                    /* 80+80 Bandwidth, so we are not done yet. */
                    const char plus80[] = "+80/";

                    chspec_bw = WL_CHANSPEC_BW_8080;

                    a = consume_string(a, plus80);
                    if (a == NULL)
                        return WLMDM_GENERIC_ERROR;

                    /* read primary 80MHz channel */
                    if (read_uint(&a, &ch1) == -1)
                        return WLMDM_GENERIC_ERROR;

                    /* must followed by '-' */
                    c = peek_next(a);
                    if (c != '-')
                    {
                        return WLMDM_GENERIC_ERROR;
                    }

                    a = consume_char(a);
                    /* read secondary 80MHz channel */
                    if (read_uint(&a, &ch2) == -1)
                    {
                        return WLMDM_GENERIC_ERROR;
                    }
                    state = STATE_DONE;
                }
                else if (c == '.')
                {
                    /* 2.5 */
                    /* must be looking at '.5'
                     * check and consume this string.
                     */
                    chspec_bw = WL_CHANSPEC_BW_2P5;
                    a = consume_char(a);
                    if ((read_char(&a, &c) == -1) || (c != '5'))
                    {
                        return WLMDM_GENERIC_ERROR;
                    }
                    state = STATE_DONE;
                }

            default:
                break;
        }
    }

    /* Now we have the chanspec string parsed, and have below data:
     * chspec_band, ctrl_ch, chspec_bw, sb_ul, ch1, ch2.
     * chspec_band and chspec_bw are chanspec values.
     * Need to convert ctrl_ch, sb_ul, ch1 and ch2 into
     * a center channel (or two) and sideband.
     */

    /* if a sb u/l string was given, just use that,
     * guaranteed to be bw = 40 by sting parse.
     */
    if (sb_ul != '\0')
    {
        if (sb_ul == 'l')
        {
            chspec_ch = upper_20_sb(ctrl_ch);
            chspec_sb = WL_CHANSPEC_CTL_SB_L;
        }
        else if (sb_ul == 'u')
        {
            chspec_ch = lower_20_sb(ctrl_ch);
            chspec_sb = WL_CHANSPEC_CTL_SB_U;
        }
    }
    /* if the bw is 20, center and sideband are trivial */
    else if (chspec_bw == WL_CHANSPEC_BW_20)
    {
        chspec_ch = ctrl_ch;
        chspec_sb = WL_CHANSPEC_CTL_SB_NONE;
    }
    /* if the bw is 40/80/160, not 80+80, a single method
     * can be used to to find the center and sideband
     */
    else if (chspec_bw != WL_CHANSPEC_BW_8080)
    {
        int sb = -1;

    	if (chspec_band == WL_CHANSPEC_BAND_5G)
    	{
            /* figure out primary sideband based on primary channel and bandwidth */
            const unsigned int *center_ch = NULL;
            int num_ch = 0;

            if (chspec_bw == WL_CHANSPEC_BW_40)
            {
                center_ch = wf_5g_40m_chans;
                num_ch = WF_NUM_5G_40M_CHANS;
            }
            else if (chspec_bw == WL_CHANSPEC_BW_80)
            {
                center_ch = wf_5g_80m_chans;
                num_ch = WF_NUM_5G_80M_CHANS;
            }
            else if (chspec_bw == WL_CHANSPEC_BW_160)
            {
                center_ch = wf_5g_160m_chans;
                num_ch = WF_NUM_5G_160M_CHANS;
            }
            else
            {
                return WLMDM_GENERIC_ERROR;
            }

            for (i = 0; i < num_ch; i ++)
            {
                sb = channel_to_sb(center_ch[i], ctrl_ch, bw);
                if (sb >= 0)
                {
                    chspec_ch = center_ch[i];
                    chspec_sb = sb;
                    break;
                }
            }
        }
        else if (chspec_band == WL_CHANSPEC_BAND_6G)
        {
            unsigned int min_ch;
            unsigned int ch_interval;
            unsigned int center_ch;

            switch (chspec_bw) {
            case WL_CHANSPEC_BW_40:
                min_ch = CH_MIN_6G_40M_CHANNEL;
                ch_interval = CH_40MHZ_APART;
                break;
            case WL_CHANSPEC_BW_80:
                min_ch = CH_MIN_6G_80M_CHANNEL;
                ch_interval = CH_80MHZ_APART;
                break;
            case WL_CHANSPEC_BW_160:
                min_ch = CH_MIN_6G_160M_CHANNEL;
                ch_interval = CH_160MHZ_APART;
                break;
            default:
                return WLMDM_GENERIC_ERROR;
            }

            center_ch = min_ch;
            while (center_ch < CH_MAX_6G_CHANNEL)
            {
                sb = channel_to_sb(center_ch, ctrl_ch, bw);
                if (sb >= 0)
                {
                    chspec_ch = center_ch;
                    chspec_sb = sb;
                    break;
                }
                center_ch += ch_interval;
            }
        }
        /* check for no matching sb/center */
        if (sb < 0)
        {
            return WLMDM_GENERIC_ERROR;
        }
    }
    /* Otherwise, bw is 80+80. Figure out channel pair and sb */
    else
    {
        int ch1_id = 0, ch2_id = 0;
        int sb;

        /* look up the channel ID for the specified channel numbers */
        ch1_id = channel_80mhz_to_index(ch1);
        ch2_id = channel_80mhz_to_index(ch2);

        /* validate channels */
        if (ch1_id < 0 || ch2_id < 0)
            return WLMDM_GENERIC_ERROR;

        /* combine 2 channel IDs in channel field of chspec */
        chspec_chan1_index = ch1_id;
        chspec_chan2_index = ch2_id;

        /* figure out primary 20 MHz sideband */

        /* is the primary channel contained in the 1st 80MHz channel? */
        sb = channel_to_sb(ch1, ctrl_ch, bw);
        if (sb < 0)
        {
            /* no match for primary channel 'ctrl_ch' in segment0 80MHz channel */
            return WLMDM_GENERIC_ERROR;
        }

        chspec_sb = sb;
    }

    chanspec->band = chspec_band;
    chanspec->band_width = chspec_bw;
    chanspec->ctlsb_index = chspec_sb;
    chanspec->channel = chspec_ch;
    chanspec->chan1_index = chspec_chan1_index;
    chanspec->chan2_index = chspec_chan2_index;

    if (!chanspec_isvalid(chanspec))
    {
        cmsLog_error("Parsed chanspec is invalid!");
        dump_chanspec(chanspec);
        return WLMDM_GENERIC_ERROR;
    }

    return WLMDM_OK;
}

/* given a chanspec and a string buffer, format the chanspec as a
 * string, and return the original pointer a.
 * Min buffer length must be CHANSPEC_STR_LEN.
 * On error return NULL
 */
WlmdmRet chanspec_to_string(chanspec_t *chspec, char *buf, size_t size)
{
    const char *band;
    unsigned int ctl_chan;

    if (chanspec_isvalid(chspec) != TRUE)
    {
        cmsLog_error("chanspec is invalid!");
        dump_chanspec(chspec);
        return WLMDM_GENERIC_ERROR;
    }

    if (chspec->band == WL_CHANSPEC_BAND_6G)
    {
        band = "6g";
    }
    else
    {
        band = "";
    }

    /* check for non-default band spec */
    if (((chspec->band == WL_CHANSPEC_BAND_2G) && (chspec->channel > CH_MAX_2G_CHANNEL)) ||
        ((chspec->band == WL_CHANSPEC_BAND_5G) && (chspec->channel <= CH_MAX_2G_CHANNEL)))
    {
        band = (chspec->band == WL_CHANSPEC_BAND_2G) ? "2g" : "5g";
    }

    /* ctl channel */
    ctl_chan = chanspec_ctlchan(chspec);

    /* bandwidth and ctl sideband */
    if (chspec->band_width == WL_CHANSPEC_BW_20)
    {
        snprintf(buf, size, "%s%d", band, ctl_chan);
    }
    else if (chspec->band_width != WL_CHANSPEC_BW_8080)
    {
        const char *bw;
        const char *sb = "";

        bw = chanspec_to_bw_str(chspec);

        /* ctl sideband string instead of BW for 40MHz */
        if (chspec->band_width == WL_CHANSPEC_BW_40 && chspec->band != WL_CHANSPEC_BAND_6G)
        {
            sb = chspec_sb_upper(chspec) ? "u" : "l";
            snprintf(buf, size, "%s%d%s", band, ctl_chan, sb);
        }
        else
        {
            snprintf(buf, size, "%s%d/%s", band, ctl_chan, bw);
        }
    }
    else
    {
        /* 80+80 */
        unsigned int chan1 = chspec->chan1_index;
        unsigned int chan2 = chspec->chan2_index;

        /* convert to channel number */
        chan1 = (chan1 < WF_NUM_5G_80M_CHANS) ? wf_5g_80m_chans[chan1] : 0;
        chan2 = (chan2 < WF_NUM_5G_80M_CHANS) ? wf_5g_80m_chans[chan2] : 0;

        /* Outputs a max of CHANSPEC_STR_LEN chars including '\0'  */
        snprintf(buf, size, "%d/80+80/%d-%d", ctl_chan, chan1, chan2);
    }

    return WLMDM_OK;
}

/*
 * Verify the chanspec is using a legal set of parameters, i.e. that the
 * chanspec specified a band, bw, ctl_sb and channel and that the
 * combination could be legal given any set of circumstances.
 * RETURNS: FALSE if the chanspec is malformed, TRUE if it looks good.
 */
UBOOL8 chanspec_isvalid(chanspec_t *chanspec)
{
    unsigned int chspec_bw = chanspec->band_width;
    unsigned int chspec_ch = chanspec_channel(chanspec);

    /* must be 2G or 5G band */
    if (chanspec->band == WL_CHANSPEC_BAND_2G)
    {
        /* must be valid bandwidth */
        if ((chspec_bw != WL_CHANSPEC_BW_20) && (chspec_bw != WL_CHANSPEC_BW_40))
        {
            return FALSE;
        }
        if (chspec_bw == WL_CHANSPEC_BW_20)
        {
            chanspec->ctlsb_index = 0;
        }
    }
    else if (chanspec->band == WL_CHANSPEC_BAND_5G)
    {
        if (chspec_bw == WL_CHANSPEC_BW_8080)
        {
            unsigned int ch1_id, ch2_id;

            /* channel IDs in 80+80 must be in range */
            ch1_id = chanspec->chan1_index;
            ch2_id = chanspec->chan2_index;
            if ((ch1_id >= WF_NUM_5G_80M_CHANS) || (ch2_id >= WF_NUM_5G_80M_CHANS))
            {
                return FALSE;
            }
        }
        else if ((chspec_bw == WL_CHANSPEC_BW_20) || (chspec_bw == WL_CHANSPEC_BW_40) ||
            (chspec_bw == WL_CHANSPEC_BW_80) || (chspec_bw == WL_CHANSPEC_BW_160))
        {
            if (chspec_bw == WL_CHANSPEC_BW_20)
            {
                chanspec->ctlsb_index = 0;
            }
            if (chspec_ch > MAXCHANNEL)
            {
                return FALSE;
            }
        }
        else
        {
            /* invalid bandwidth */
            return FALSE;
        }
    }
    else if (chanspec->band == WL_CHANSPEC_BAND_6G)
    {
    	unsigned int min_ch;
    	unsigned int ch_interval;

        switch (chspec_bw) {
        case WL_CHANSPEC_BW_20:
            /* channel 2 is allowed exception */
            if (chspec_ch == 2)
                min_ch = chspec_ch;
            else
                min_ch = CH_MIN_6G_CHANNEL;
            ch_interval = CH_20MHZ_APART;
            chanspec->ctlsb_index = 0;
            break;
        case WL_CHANSPEC_BW_40:
            min_ch = CH_MIN_6G_40M_CHANNEL;
            ch_interval = CH_40MHZ_APART;
            break;
        case WL_CHANSPEC_BW_80:
            min_ch = CH_MIN_6G_80M_CHANNEL;
            ch_interval = CH_80MHZ_APART;
            break;
        case WL_CHANSPEC_BW_160:
            min_ch = CH_MIN_6G_160M_CHANNEL;
            ch_interval = CH_160MHZ_APART;
            break;
        default:
            return FALSE;
        }
        if (((chspec_ch - min_ch) % ch_interval) != 0)
        	return FALSE;
    }
    else
    {
        /* band not valid */
        return FALSE;
    }

    /* side band needs to be consistent with bandwidth */
    switch (chspec_bw)
    {
    case WL_CHANSPEC_BW_20:
        if (chanspec->ctlsb_index != WL_CHANSPEC_CTL_SB_NONE)
        {
            return FALSE;
        }
        break;

    case WL_CHANSPEC_BW_40:
        if (chanspec->ctlsb_index > WL_CHANSPEC_CTL_SB_LLU)
        {
            return FALSE;
        }
        break;

    case WL_CHANSPEC_BW_80:
        if (chanspec->ctlsb_index > WL_CHANSPEC_CTL_SB_LUU)
        {
            return FALSE;
        }
        break;

    case WL_CHANSPEC_BW_160:
    case WL_CHANSPEC_BW_8080:
        if(chanspec->ctlsb_index > WL_CHANSPEC_CTL_SB_UUU)
        {
            return FALSE;
        }
        break;

    default:
        break;
    }

    return TRUE;
}

unsigned int chanspec_channel(chanspec_t *chspec)
{
    if (chspec->band_width == WL_CHANSPEC_BW_8080)
    {
        return chanspec_primary80_channel(chspec);
    }
    else
    {
        return chspec->channel;
    }
}

/* given a chanspec, return the bandwidth string */
const char *chanspec_to_bw_str(chanspec_t *chspec)
{
    return wf_chspec_bw_str[chspec->band_width];
}
/*
 * This function returns the 80Mhz channel for the given .
 */
static unsigned int wf_chspec_get80Mhz_ch(unsigned int chan_index)
{
    if (chan_index < WF_NUM_5G_80M_CHANS)
    {
        return wf_5g_80m_chans[chan_index];
    }
    return 0;
}

/*
 * Returns the primary 80 Mhz channel for the provided chanspec
 *
 *    chanspec - Input chanspec for which the 80MHz primary channel has to be retrieved
 *
 *  returns -1 in case the provided channel is 20/40 Mhz chanspec
 */

unsigned int chanspec_primary80_channel(chanspec_t *chanspec)
{
    unsigned int primary80_chan;
    unsigned int center_chan;
    unsigned int sb_index;

    switch(chanspec->band_width)
    {
        case WL_CHANSPEC_BW_80:
            primary80_chan = chanspec->channel;
            break;

        case WL_CHANSPEC_BW_8080:
            /* Channel ID 1 corresponds to frequency segment 0, the primary 80 MHz segment */
            primary80_chan = wf_chspec_get80Mhz_ch(chanspec->chan1_index);
            break;

        case WL_CHANSPEC_BW_160:
            center_chan = chanspec->channel;
            sb_index = chanspec->ctlsb_index;
            /* based on the sb value primary 80 channel can be retrieved
             * if sb is in range 0 to 3 the lower band is the 80Mhz primary band
             */
            if (sb_index < 4)
            {
                primary80_chan = center_chan - CH_40MHZ_APART;
            }
            /* if sb is in range 4 to 7 the upper band is the 80Mhz primary band */
            else
            {
                primary80_chan = center_chan + CH_40MHZ_APART;
            }
            break;

        default:
            /* for 20 and 40 Mhz */
            primary80_chan = -1;
            break;
    }
    return primary80_chan;
}

/*
 * This function returns the channel number that control traffic is being sent on, for 20MHz
 * channels this is just the channel number, for 40MHZ, 80MHz, 160MHz channels it is the 20MHZ
 * sideband depending on the chanspec selected
 */
unsigned int chanspec_ctlchan(chanspec_t *chspec)
{
    unsigned int center_chan;
    unsigned int bw_mhz;
    unsigned int sb;

    assert(chanspec_isvalid(chspec));

    /* Is there a sideband ? */
    if (chspec->band_width <= WL_CHANSPEC_BW_20)
    {
        return chspec->channel;
    }
    else
    {
        sb = chspec->ctlsb_index;

        if (chspec->band_width == WL_CHANSPEC_BW_8080)
        {
            /* For an 80+80 MHz channel, the sideband 'sb' field is an 80 MHz sideband
             * (LL, LU, UL, LU) for the 80 MHz frequency segment 0.
             */
            unsigned int chan_id = chspec->chan1_index;

            bw_mhz = 80;

            /* convert from channel index to channel number */
            center_chan = wf_5g_80m_chans[chan_id];
        }
        else
        {
            bw_mhz = bw_chspec_to_mhz(chspec);
            center_chan = chspec->channel;
        }

        return channel_to_ctl_chan(center_chan, bw_mhz, sb);
    }
}

const char *chanspec_get_sb_string(unsigned int sb_index, WL_CHANSPEC_BW bw)
{
    switch (bw)
    {
        case WL_CHANSPEC_BW_40:
            if (sb_index < SB_MAPPER_40MHZ_SIZE)
            {
                return sb_mapper_40MHZ[sb_index].nval;
            }
            break;

        case WL_CHANSPEC_BW_80:
            if (sb_index < SB_MAPPER_80MHZ_SIZE)
            {
                return sb_mapper_80MHZ[sb_index].nval;
            }
            break;

        case WL_CHANSPEC_BW_160:
            if (sb_index < SB_MAPPER_160MHZ_SIZE)
            {
                return sb_mapper_160MHZ[sb_index].nval;
            }
            break;

        default:
            break;
    }
    return NULL;
}

int chanspec_get_sb_index(const char *nval)
{
    int len;

    /*
     * We use a trick to compare sb_string to corresponding table.
     * The nval length in sb_mapper_80MHZ is always 2, whereas the nval length
     * in sb_mapper_160MHZ is always 3.
     */
    len = strlen(nval);
    switch (len)
    {
        case 2:
            return search_sb_mapper_table((struct sb_mapper *)&sb_mapper_80MHZ, SB_MAPPER_80MHZ_SIZE, nval);

        case 3:
            return search_sb_mapper_table((struct sb_mapper *)&sb_mapper_160MHZ, SB_MAPPER_160MHZ_SIZE, nval);

        default:
            return search_sb_mapper_table((struct sb_mapper *)&sb_mapper_40MHZ, SB_MAPPER_40MHZ_SIZE, nval);
    }
}

static int search_sb_mapper_table(struct sb_mapper *sb_mapper_table, size_t size, const char *nval)
{
    unsigned int i;
    for (i = 0; i < size; i++)
    {
        if (0 == strcmp(sb_mapper_table[i].nval, nval))
        {
            return i;
        }
    }
    return -1;
}
/* convert bandwidth from chanspec to MHz; works with 5MHz to 160MHz (including 80p80) */
static unsigned int bw_chspec_to_mhz(chanspec_t *chspec)
{
    unsigned int bw;

    bw = chspec->band_width;
    return (bw >= WF_NUM_BW ? 0 : wf_chspec_bw_mhz[bw]);
}

/* return control channel given center channel, side band and bandwidth. */
static unsigned int channel_to_ctl_chan(unsigned int center_ch, unsigned int bw, unsigned int sb)
{
    return channel_low_edge(center_ch, bw) + sb * 4;
}

/* return channel number of the low edge of the band
 * given the center channel and BW
 */
static unsigned int channel_low_edge(unsigned int center_ch, unsigned int bw)
{
    return (center_ch - center_chan_to_edge(bw));
}

static unsigned int convert_bw(unsigned int bw)
{
    int i;
    for (i = 0; i < bw_map_size; i++)
    {
        if (bw_map[i].bw == bw)
        {
            return bw_map[i].spec_bw;
        }
    }
    return 0;
}

/* bw in MHz, return the channel count from the center channel to the
 * the channel at the edge of the band
 */
static unsigned int center_chan_to_edge(unsigned int bw)
{
    /* edge channels separated by BW - 10MHz on each side
     * delta from cf to edge is half of that,
     * MHz to channel num conversion is 5MHz/channel
     */
    return (unsigned int)(((bw - 20) / 2) / 5);
}

/* return side band number given center channel, control channel and band_width.
 * return -1 on error
 */
static int channel_to_sb(unsigned int center_ch, unsigned int ctrl_ch, unsigned int bw)
{
    unsigned int lowest = channel_low_edge(center_ch, bw);
    unsigned int sb;

    if ((ctrl_ch - lowest) % 4)
    {
        /* bad ctl channel as it is not multiple of 4 gaps (20MHZ) from the lowest channel */
        return -1;
    }

    sb = ((ctrl_ch - lowest) / 4);

    /* sb must be a index to a 20MHz channel in range */
    if (sb >= (bw / 20))
    {
        /* ctrl_ch must have been too high for the center_ch */
        return -1;
    }

    return sb;
}


/* return index of 80MHz channel from channel number
 * return -1 on error
 */
static int channel_80mhz_to_index(unsigned int ch)
{
    unsigned int i;
    for (i = 0; i < WF_NUM_5G_80M_CHANS; i++)
    {
        if (ch == wf_5g_80m_chans[i])
            return i;
    }

    return -1;
}

/*
 * Read the first part of a string, and try to convert it to unsigned int.
 * Advance the pointer to the string to the firsh character which is not a valid digit.
 * Returns 0 if a number has been found.
 * Returns -1 if the number couldn't be found.
 */
static int read_uint(const char **p, unsigned int *num)
{
    unsigned long val;
    char *endp = NULL;

    assert(p);

    if (*p == NULL)
    {
        return -1;
    }

    val = strtoul(*p, &endp, 10);
    if (endp == *p)
    {
        /* if endp is the initial pointer value, then a number was not read */
        return -1;
    }

    *p = endp;
    *num = (unsigned int) val;

    return 0;
}

/*
 * If the source string has length greater than 0, read the next character and
 * advance the current pointer to the string.
 * Returns 0 if read the next character.
 * Returns -1 if the input string pointer is NULL, or if the input string has length 0.
 */
static int read_char(const char **p, char *c)
{
    size_t len;
    assert(p);

    if (*p == NULL)
    {
        return -1;
    }

    len = strlen(*p);
    if (len > 0)
    {
        *c = **p;
        (*p)++;
        return 0;
    }
    return -1;
}

/*
 * If the source string has length greater than 0, read the next character.
 * Otherwise just return '\0'.
 */
static char peek_next(const char *p)
{
    size_t len;

    assert(p);
    len = strlen(p);
    if (len > 0)
    {
        return *(p++);
    }
    else
    {
        return '\0';
    }
}

/*
 * If the source string has length greater than 0,
 * advance the current pointer to the string and return the pointer.
 * Otherwise returns NULL.
 */
static const char *consume_char(const char *p)
{
    size_t len;
    len = strlen(p);
    if (len > 0)
    {
        return p + 1;
    }
    else
    {
        return NULL;
    }
}

/*
 * If the source string contains substring str,
 * advance the current pointer after the substring and return the pointer.
 * Otherwise returns NULL.
 */
static const char *consume_string(const char *p, const char *str)
{
    int i, len;

    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (p[i] != str[i])
        {
            break;
        }
    }

    if (i == len)
    {
        return p + len;
    }
    return NULL;
}

static unsigned int upper_20_sb(unsigned int channel)
{
    unsigned int result;
    result = channel + CH_10MHZ_APART;
    if (result < MAXCHANNEL)
    {
        return result;
    }
    return 0;
}

static unsigned int lower_20_sb(unsigned int channel)
{
    unsigned int result;
    result = channel - CH_10MHZ_APART;
    if (result > 0)
    {
        return result;
    }
    return 0;
}

static UBOOL8 chspec_sb_upper(chanspec_t *chspec)
{
    if ((chspec->ctlsb_index == WL_CHANSPEC_CTL_SB_U) &&
        (chspec->band_width == WL_CHANSPEC_BW_40))
    {
        return TRUE;
    }
    return FALSE;
}

static void dump_chanspec(struct chanspec_t *chanspec)
{
    printf("band = %d\n", chanspec->band);
    printf("band_width = %d\n", chanspec->band_width);
    printf("ctlsb_index = %d\n", chanspec->ctlsb_index);
    printf("channel = %d\n", chanspec->channel);
    printf("chan1_index = %d\n", chanspec->chan1_index);
    printf("chan2_index = %d\n", chanspec->chan2_index);
}
