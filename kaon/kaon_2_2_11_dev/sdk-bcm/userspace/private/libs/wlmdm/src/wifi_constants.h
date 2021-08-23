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
#ifndef __WIFI_CONSTANTS_H__
#define __WIFI_CONSTANTS_H__

typedef enum nwifi_security_mode
{
    NWIFI_SECURITY_MODE_OPEN = 0,
    NWIFI_SECURITY_MODE_WEP_64, /*  */
    NWIFI_SECURITY_MODE_WEP_128,
    NWIFI_SECURITY_MODE_WPA_Personal,
    NWIFI_SECURITY_MODE_WPA2_Personal,
    NWIFI_SECURITY_MODE_WPA_WPA2_Personal,
    NWIFI_SECURITY_MODE_WPA_Enterprise,
    NWIFI_SECURITY_MODE_WPA2_Enterprise,
    NWIFI_SECURITY_MODE_WPA_WPA2_Enterprise
} NWIFI_SECURITY_MODE;

typedef enum wifi_bands
{
    BAND_A = 1,
    BAND_B = 2,
    BAND_6G = 4
} WIFI_BANDS;

typedef enum wifi_bandwidths
{
    WL_N_BW_20ALL = 0,
    WL_N_BW_40ALL,
    WL_N_BW_20IN2G_40IN5G,
    WL_V_BW_80IN5G
} WIFI_BANDWIDTH;

typedef enum wifi_reg_modes
{
    REG_MODE_OFF = 0,    /* disabled 11h/d mode */
    REG_MODE_H,          /* use 11h mode */
    REG_MODE_D           /* use 11d mode */
} WIFI_REG_MODES;

typedef enum wifi_keybits
{
    WL_BIT_KEY_128 = 0,
    WL_BIT_KEY_64
} WIFI_KEYBITS;

typedef enum wifi_bss_idx
{
    MAIN_BSS_IDX = 0,
    GUEST_BSS_IDX = 1,
    GUEST1_BSS_IDX = 2,
    GUEST2_BSS_IDX = 3
} WIFI_BSS_IDX;

typedef enum wifi_phy_types
{
    WL_PHYTYPE_A = 0,
    WL_PHYTYPE_B = 1,
    WL_PHYTYPE_G = 2,
    WL_PHYTYPE_N = 4,
    WL_PHYTYPE_LP = 5,
    WL_PHYTYPE_SSN = 6,
    WL_PHYTYPE_HT = 7,
    WL_PHYTYPE_LCN = 8,
    WL_PHYTYPE_LCN40 = 10,
    WL_PHYTYPE_AC = 11
} WIFI_PHY_TYPES;

static const char WL_FLT_MAC_OFF[] = "disabled";
static const char WL_FLT_MAC_ALLOW[] = "allow";
static const char WL_FLT_MAC_DENY[] = "deny";

static const char WL_BASIC_RATE_DEFAULT[] = "default";
static const char WL_BASIC_RATE_ALL[] = "all";
static const char WL_BASIC_RATE_1_2[] = "12";
static const char WL_BASIC_RATE_WIFI_2[] = "wifi2";

static const char WL_OPMODE_AP[] = "ap";
static const char WL_OPMODE_WDS[] = "wds";
static const char WL_OPMODE_WET[] = "wet";
static const char WL_OPMODE_WET_MACSPOOF[] = "wetmacspoof";

#endif
