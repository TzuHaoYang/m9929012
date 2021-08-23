/*
* <:copyright-BRCM:2019:proprietary:standard
* 
*    Copyright (c) 2019 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
 
*/


#ifndef __PMD_JSON_H__
#define __PMD_JSON_H__


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <json.h>
#include "pmd_json.h"
#include "pmd.h"


/* #define PMD_JSON_DEBUG */
#if defined(PMD_JSON_DEBUG)
#define DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ((void)0)
#endif

#define JENKINS_CHANGELIST_FILE_PATH "/etc/JENKINS_CHANGELIST"

#define CALIBRATION_FILE_VERSION 8 /* should be incremented when changing the calibration file parameters */

#define LEGACY_CALIBRATION_FILE_MAX_SUPPORTED_VERSION 7 /* DO NOT CHANGE this legacy */
#define LEGACY_CALIBRATION_FILE_MIN_SUPPORTED_VERSION 4 /* DO NOT CHANGE this legacy */
#define CAL_FILE_WATERMARK 0xcafe2bed

#define PMD_JSON_ERROR   -1
#define PMD_JSON_SUCCESS  0

#define PMD_JSON_MANIFEST                 "manifest"
#define PMD_JSON_MANIFEST_CALIBRATION_FILE_VER           "calibration_file_version"
#define PMD_JSON_MANIFEST_CALIBRATION_UTC_DATE_TIME      "calibration_utc_date_time_yymmdd_hhmm"
#define PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_RELEASE "calibration_tool_suite_release"
#define PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_CL      "calibration_tool_suite_cl"
#define PMD_JSON_MANIFEST_FW_VERSION                     "fw_version"
#define PMD_JSON_MANIFEST_HOST_EXPECTED_FW_VER           "host_expected_fw_version"
#define PMD_JSON_MANIFEST_HOST_RELEASE                   "host_release"
#define PMD_JSON_MANIFEST_HOST_CL                        "host_cl"

#define PMD_JSON_FAQ_LEVEL0_DAC           "faq_level0_dac"
#define PMD_JSON_FAQ_LEVEL1_DAC           "faq_level1_dac"
#define PMD_JSON_FILE_WATERMARK           "file_watermark"
#define PMD_JSON_BIAS                     "bias"
#define PMD_JSON_MOD                      "mod"

#define PMD_JSON_APD                      "apd"
#define PMD_JSON_APD_TYPE    "type"
#define PMD_JSON_APD_VOLTAGE "voltage"

#define PMD_JSON_MPD_CHANNEL_CONFIG       "mpd_channel_config"
#define PMD_JSON_MPD_CHANNEL_CONFIG_VGA       "vga"
#define PMD_JSON_MPD_CHANNEL_CONFIG_TIA       "tia"
#define PMD_JSON_MPD_CHANNEL_CONFIG_DACRANGE  "dacrange"
#define PMD_JSON_MPD_CHANNEL_CONFIG_CALIBCTRL "calibctrl"

#define PMD_JSON_MPD_GAINS                "mpd_gains"
#define PMD_JSON_MPD_GAINS_BIAS_GAIN "bias_gain"
#define PMD_JSON_MPD_GAINS_MOD_GAIN  "mod_gain"

#define PMD_JSON_APDOI_CTRL               "apdoi_ctrl"

#define PMD_JSON_RSSI_COEFFICIENTS        "rssi_coefficients"
#define PMD_JSON_RSSI_COEFFICIENTS_A "a"
#define PMD_JSON_RSSI_COEFFICIENTS_B "b"
#define PMD_JSON_RSSI_COEFFICIENTS_C "c"

#define PMD_JSON_TEMP_0                   "temp_0"
#define PMD_JSON_TEMP_COFF_H              "temp_coff_h"
#define PMD_JSON_TEMP_COFF_L              "temp_coff_l"
#define PMD_JSON_ESC_THR                  "esc_thr"
#define PMD_JSON_ROGUE_THR                "rogue_thr"
#define PMD_JSON_LEVEL_0_DAC              "tracking_level0_dac"
#define PMD_JSON_AVG_LEVEL_1_DAC          "tracking_avg_level_dac"

#define PMD_JSON_LOS_THR                  "los_thr"
#define PMD_JSON_LOS_THR_ASSERT   "assert"
#define PMD_JSON_LOS_THR_DEASSERT "deassert"

#define PMD_JSON_SAT_POS                  "sat_pos"
#define PMD_JSON_SAT_POS_HIGH "high"
#define PMD_JSON_SAT_POS_LOW  "low"
#define PMD_JSON_SAT_NEG                  "sat_neg"
#define PMD_JSON_SAT_NEG_HIGH "high"
#define PMD_JSON_SAT_NEG_LOW  "low"

#define PMD_JSON_EYE_SHAPING_COEFFICIENTS "eye_shaping_coefficients"
#define PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE          "edge_rate"
#define PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_RATE "rate"
#define PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_DLOAD "dload"
#define PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_WEIGHT "preemphasis_weight"
#define PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_DELAY  "preemphasis_delay"

#define PMD_JSON_DUTY_CYCLE               "duty_cycle"
#define PMD_JSON_TX_POWER                 "tx_power"
#define PMD_JSON_BIAS0                    "bias0"
#define PMD_JSON_TEMP_OFFSET              "temp_offset"
#define PMD_JSON_BIAS_DELTA_I             "bias_delta_i"
#define PMD_JSON_SLOPE_EFFICIENCY         "slope_efficiency"
#define PMD_JSON_RES2TEMP                 "res2temp"

#define PMD_JSON_ADF_LOS_THRESHOLDS       "adf_los_thresholds"
#define PMD_JSON_ADF_LOS_THRESHOLDS_ASSERT   "assert"
#define PMD_JSON_ADF_LOS_THRESHOLDS_DEASSERT "deassert"
#define PMD_JSON_ADF_LOS_LEAKY_BUCKET     "adf_los_leaky_bucket"
#define PMD_JSON_ADF_LOS_LEAKY_BUCKET_ASSERT       "assert"
#define PMD_JSON_ADF_LOS_LEAKY_BUCKET_LB_BUCKET_SZ "lb_bucket_sz"

#define PMD_JSON_COMPENSATION             "compensation"
#define PMD_JSON_COMPENSATION_ENABLE0      "enable0"
#define PMD_JSON_COMPENSATION_ENABLE1      "enable1"
#define PMD_JSON_COMPENSATION_DIE_TEMP_REF "die_temp_ref"
#define PMD_JSON_COMPENSATION_COEFF1_Q8    "coeff1_q8"
#define PMD_JSON_COMPENSATION_COEFF2_Q8    "coeff2_q8"
#define PMD_JSON_TEMP2APD                 "temperature_to_apd_conversion_table"


/* Legacy calibration file version is 7 */
/* DO NOT CHANGE the legacy struct!!!   */
typedef struct
{
    flash_int  watermark;       
    flash_int  version;
    flash_word level_0_dac;     /* used for first burst + CID tracking */
    flash_word level_1_dac;
    flash_word bias;
    flash_word mod;
    flash_word apd;
    flash_word mpd_config;
    flash_word mpd_gains;
    flash_word apdoi_ctrl;
    flash_int rssi_a_cal;      /* optic_power = a * rssi + b */
    flash_int rssi_b_cal;
    flash_int rssi_c_cal;
    flash_word temp_0;
    flash_word coff_h;    /* Temperate coefficient high */
    flash_word coff_l;    /* Temperate coefficient low */
    flash_word esc_th;
    flash_word rogue_th;
    flash_word avg_level_0_dac; /* used for force tracking */
    flash_word avg_level_1_dac;
    flash_word dacrange;
    flash_word los_thr;
    flash_word sat_pos;
    flash_word sat_neg;
    flash_word_mult edge_rate;
    flash_word_mult preemphasis_weight;
    flash_int_mult  preemphasis_delay;
    flash_word duty_cycle;
    flash_word calibctrl;
    flash_word tx_power;
    flash_word bias0;
    flash_word temp_offset;
    flash_word bias_delta_i;
    flash_word slope_efficiency;
    flash_temp_table temp_table;
    flash_int adf_los_thresholds;
    flash_word adf_los_leaky_bucket;
    flash_int compensation;
} pmd_calibration_legacy;


#endif /* __PMD_JSON_H__ */
