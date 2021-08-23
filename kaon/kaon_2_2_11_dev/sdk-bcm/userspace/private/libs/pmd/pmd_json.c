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


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <json.h>
#include <errno.h>
#include "pmd_json.h"
#include "pmd.h"
#include "pmd_version.h"


#define OFFSET_OF(structure_type, structure_field) (&(((structure_type *)0)->structure_field))
#define INFO_STR_LENGTH 256


static void pmd_calibration_resistance_temperature_table_to_json(struct json_object *calibration_json_object,
    const uint32_t res2temp[TEMP_TABLE_SIZE]);
static void pmd_calibration_apd_temperature_table_to_json(struct json_object *calibration_json_object,
    const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE]);
static void replace_or_create_object_containing_object(struct json_object **new_object,
    struct json_object *calibration_json_object, const char *json_object_key_name, struct json_object *contained_jobj,
    const char *contained_json_key_name);
static int load_or_create_pmd_calibration_json(struct json_object **calibration_json_object);
static void set_json_pmd_manifest(struct json_object *calibration_json_object);
static int get_pmd_firmware_version(uint32_t *fw_version);
static int parse_pmd_calibration_script_manifest(char *script_release, uint32_t *script_cl, char *script_datetime);
static int get_host_expected_pmd_firmware_version(uint32_t *expected_pmd_fw_ver);
int load_pmd_calibration_data(pmd_calibration_parameters *calibration_binary);
static void replace_or_create_array_element_containing_object(struct json_object **new_array_element,
    struct json_object *calibration_json_object, const char *json_array_key_name, uint16_t element_index,
    struct json_object *contained_jobj, const char *contained_json_key_name);
static void replace_or_create_array_element(struct json_object **new_jarray,
    struct json_object *calibration_json_object, const char *json_key_name, struct json_object *jarray_element,
    uint16_t element_index);
static int load_pmd_json_calibration_file_to_struct(pmd_calibration_parameters *calibration_binary);
static void pmd_calibration_json_to_struct(const struct json_object *calibration_json_object,
    pmd_calibration_parameters *calibration_binary);
static int verify_type_and_get_object_by_key_from_object(void *jobj_destination, const char *json_key_name,
    const enum json_type expected_jobj_type, const struct json_object *jobj_source);
static int verify_and_get_int_by_key(int *int_destination, const char *json_key_name,
    const struct json_object *calibration_json_object);
static int copy_boolean_from_json(int *bool_destination, const char *json_key_name,
    const struct json_object *calibration_json_object);
static int get_json_object_from_json(struct json_object **jobj_destination, const char *json_key_name,
    const struct json_object *calibration_json_object);
static int get_json_array_from_json(struct json_object **jarray_destination, const char *json_key_name,
    const struct json_object *calibration_json_object);
static int verify_object_type_and_get_value(void *result, const struct json_object *jobj,
    const enum json_type expected_jobj_type, const char *info_str);
static void generate_pmd_json_calibration_file(void);
static int read_binary_file_to_buffer(const char *filename, void *binary_data, const long max_data_length);
static void pmd_calibration_struct_to_json(const pmd_calibration_legacy *calibration_binary,
    const temperature_to_apd_table *temp2apd_table, struct json_object **calibration_json_object);
static void create_json_array_from_resistance_temperature_table(struct json_object **jarray,
    const uint32_t res2temp[TEMP_TABLE_SIZE]);
static void create_json_array_from_apd_temperature_table(struct json_object **jarray,
    const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE]);
#if defined(PMD_JSON_DEBUG)
static void print_binary_buffer(const void *binary_data, const long data_length);
#endif
static struct json_object* create_pmd_rssi_coefficients_json_object(struct json_object *a_jint,
    struct json_object *b_jint, struct json_object *c_jint);
static struct json_object* create_pmd_mpd_channel_config_vga_tia_json_object(struct json_object *vga_jint,
    struct json_object *tia_jint);
static struct json_object* create_pmd_apd_json_object(int32_t val);
static struct json_object* create_pmd_mpd_gains_json_object(int32_t val);
static struct json_object* create_pmd_los_thr_json_object(int32_t val);
static struct json_object* create_pmd_sat_pos_json_object(int32_t val);
static struct json_object* create_pmd_sat_neg_json_object(int32_t val);
static struct json_object* create_pmd_adf_los_thresholds_json_object(int32_t val);
static struct json_object* create_pmd_adf_los_leaky_bucket_json_object(int32_t val);
static struct json_object* create_pmd_compensation_json_object(int32_t val);
static struct json_object* create_pmd_edge_rate_json_object(int32_t val);
void pmd_calibration_json_file_set_scripts_manifest(const char *calibration_manifest);
void pmd_calibration_json_file_stamp_scripts_manifest(const char *calibration_manifest);
void pmd_calibration_json_file_reset_manifest(void);
void pmd_calibration_delete_files(void);
static void pmd_calibration_remove_file(const char* file_path);


static char json_file_manifest[PMD_BUF_MAX_SIZE] = {0};


int pmd_calibration_resistance_temperature_table_to_json_file(const uint32_t res2temp[TEMP_TABLE_SIZE])
{
    int ret;
    struct json_object *calibration_json_object;

    ret = load_or_create_pmd_calibration_json(&calibration_json_object);
    if (ret)
    {
        return PMD_JSON_ERROR;
    }

    pmd_calibration_resistance_temperature_table_to_json(calibration_json_object, res2temp);

    ret = json_object_to_file_ext(PMD_CALIBRATION_JSON_FILE_PATH, calibration_json_object, JSON_C_TO_STRING_SPACED |
        JSON_C_TO_STRING_PRETTY);
    if (ret)
    {
        fprintf(stderr, "Updating the PMD JSON file %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());
    }

    json_object_put(calibration_json_object);
    return ret;
}


static void pmd_calibration_resistance_temperature_table_to_json(struct json_object *calibration_json_object,
    const uint32_t res2temp[TEMP_TABLE_SIZE])
{
    struct json_object *jarray;

    create_json_array_from_resistance_temperature_table(&jarray, res2temp);
    json_object_object_add_ex(calibration_json_object, PMD_JSON_RES2TEMP, jarray, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
}


int pmd_calibration_apd_temperature_table_to_json_file(const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE])
{
    int ret;
    struct json_object *calibration_json_object;

    ret = load_or_create_pmd_calibration_json(&calibration_json_object);
    if (ret)
    {
        return PMD_JSON_ERROR;
    }

    pmd_calibration_apd_temperature_table_to_json(calibration_json_object, temp2apd_table);

    ret = json_object_to_file_ext(PMD_CALIBRATION_JSON_FILE_PATH, calibration_json_object, JSON_C_TO_STRING_SPACED |
        JSON_C_TO_STRING_PRETTY);
    if (ret)
    {
        fprintf(stderr, "Updating the PMD JSON file %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());
    }

    json_object_put(calibration_json_object);
    return ret;
}


static void pmd_calibration_apd_temperature_table_to_json(struct json_object *calibration_json_object,
    const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE])
{
    struct json_object *jarray;

    create_json_array_from_apd_temperature_table(&jarray, temp2apd_table);
    json_object_object_add_ex(calibration_json_object, PMD_JSON_TEMP2APD, jarray, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
}


int pmd_calibration_parameter_to_json_file(pmd_calibration_parameters_index parameter_index, int32_t val,
    uint16_t set_index)
{
    struct json_object *calibration_json_object, *jobj = NULL;
    char *json_name = NULL;
    int ret;

    ret = load_or_create_pmd_calibration_json(&calibration_json_object);
    if (ret)
    {
        return PMD_JSON_ERROR;
    }

    switch (parameter_index)
    {
        /* objects of multiple different PMD parameters */
        case PMD_CALIBRATION_FILE_VER:
            json_name = PMD_JSON_MANIFEST_CALIBRATION_FILE_VER;
            {
                struct json_object *manifest_jobj = NULL;

                jobj = json_object_new_int(val);
                replace_or_create_object_containing_object(&manifest_jobj, calibration_json_object, PMD_JSON_MANIFEST,
                    jobj, json_name);
                jobj = manifest_jobj;
                json_name = PMD_JSON_MANIFEST;
            }
            break;
        case PMD_RSSI_A: /*fall through*/
        case PMD_RSSI_B: /*fall through*/
        case PMD_RSSI_C:
            switch (parameter_index)
            {
                case PMD_RSSI_A:
                    json_name = PMD_JSON_RSSI_COEFFICIENTS_A;
                    break;
                case PMD_RSSI_B:
                    json_name = PMD_JSON_RSSI_COEFFICIENTS_B;
                    break;
                case PMD_RSSI_C:
                    json_name = PMD_JSON_RSSI_COEFFICIENTS_C;
                    break;
                default:
                    break;
            }
            {
                struct json_object *rssi_jobj = NULL;

                jobj = json_object_new_int(val);
                replace_or_create_object_containing_object(&rssi_jobj, calibration_json_object,
                    PMD_JSON_RSSI_COEFFICIENTS, jobj, json_name);
                jobj = rssi_jobj;
                json_name = PMD_JSON_RSSI_COEFFICIENTS;
            }
            break;
        case PMD_MPD_CONFIG:
            {
                struct json_object *channel_jobj = NULL;
                int vga = (val & 0xc00) >> 10;
                int tia = (val & 0xF000) >> 12;

                jobj = json_object_new_int(vga);
                replace_or_create_object_containing_object(&channel_jobj, calibration_json_object,
                    PMD_JSON_MPD_CHANNEL_CONFIG, jobj, PMD_JSON_MPD_CHANNEL_CONFIG_VGA);
                /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'PMD_JSON_MPD_CHANNEL_CONFIG' key is already
                   used above */
                json_object_object_add_ex(calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG, channel_jobj,
                    JSON_C_OBJECT_KEY_IS_CONSTANT);

                jobj = json_object_new_int(tia);
                replace_or_create_object_containing_object(&channel_jobj, calibration_json_object,
                    PMD_JSON_MPD_CHANNEL_CONFIG, jobj, PMD_JSON_MPD_CHANNEL_CONFIG_TIA);
                jobj = channel_jobj;
                json_name = PMD_JSON_MPD_CHANNEL_CONFIG;
            }
            break;
        case PMD_DACRANGE:
            {
                struct json_object *channel_jobj = NULL;

                jobj = json_object_new_int(val);
                replace_or_create_object_containing_object(&channel_jobj, calibration_json_object,
                    PMD_JSON_MPD_CHANNEL_CONFIG, jobj, PMD_JSON_MPD_CHANNEL_CONFIG_DACRANGE);
                jobj = channel_jobj;
                json_name = PMD_JSON_MPD_CHANNEL_CONFIG;
            }
            break;
        case PMD_MPD_CALIBCTRL:
            {
                struct json_object *channel_jobj = NULL;

                jobj = json_object_new_int(val);
                replace_or_create_object_containing_object(&channel_jobj, calibration_json_object,
                    PMD_JSON_MPD_CHANNEL_CONFIG, jobj, PMD_JSON_MPD_CHANNEL_CONFIG_CALIBCTRL);
                jobj = channel_jobj;
                json_name = PMD_JSON_MPD_CHANNEL_CONFIG;
            }
            break;

        /* objects of int object fields */
        case PMD_APD:
            json_name = PMD_JSON_APD;
            jobj = create_pmd_apd_json_object(val);
            break;
        case PMD_MPD_GAINS:
            json_name = PMD_JSON_MPD_GAINS;
            jobj = create_pmd_mpd_gains_json_object(val);
            break;
        case PMD_LOS_THR:
            json_name = PMD_JSON_LOS_THR;
            jobj = create_pmd_los_thr_json_object(val);
            break;
        case PMD_SAT_POS:
            json_name = PMD_JSON_SAT_POS;
            jobj = create_pmd_sat_pos_json_object(val);
            break;
        case PMD_SAT_NEG:
            json_name = PMD_JSON_SAT_NEG;
            jobj = create_pmd_sat_neg_json_object(val);
            break;
        case PMD_ADF_LOS_THRESHOLDS:
            json_name = PMD_JSON_ADF_LOS_THRESHOLDS;
            jobj = create_pmd_adf_los_thresholds_json_object(val);
            break;
        case PMD_ADF_LOS_LEAKY_BUCKET:
            json_name = PMD_JSON_ADF_LOS_LEAKY_BUCKET;
            jobj = create_pmd_adf_los_leaky_bucket_json_object(val);
            break;
        case PMD_COMPENSATION:
            json_name = PMD_JSON_COMPENSATION;
            jobj = create_pmd_compensation_json_object(val);
            break;

        /* array objects */
        case PMD_EDGE_RATE:
            {
                struct json_object *jarray = NULL;

                json_name = PMD_JSON_EYE_SHAPING_COEFFICIENTS;
                jobj = create_pmd_edge_rate_json_object(val);
                replace_or_create_array_element_containing_object(&jarray, calibration_json_object, json_name,
                    set_index, jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE);
                jobj = jarray;
            }
            break;
        case PMD_PREEMPHASIS_WEIGHT:
            {
                struct json_object *jarray = NULL;

                json_name = PMD_JSON_EYE_SHAPING_COEFFICIENTS;
                jobj = json_object_new_int(val);
                replace_or_create_array_element_containing_object(&jarray, calibration_json_object, json_name,
                    set_index, jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_WEIGHT);
                jobj = jarray;
            }
            break;
        case PMD_PREEMPHASIS_DELAY:
            {
                struct json_object *jarray = NULL;

                json_name = PMD_JSON_EYE_SHAPING_COEFFICIENTS;
                jobj = json_object_new_int(val);
                replace_or_create_array_element_containing_object(&jarray, calibration_json_object, json_name,
                    set_index, jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_DELAY);
                jobj = jarray;
            }
            break;
        default:
            jobj = json_object_new_int(val); /* relevant for the next switch statement */
            break;
    }

    switch (parameter_index)
    {
        /* int objects */
        case PMD_FILE_WATERMARK:
            json_name = PMD_JSON_FILE_WATERMARK;
            break;
        case PMD_FAQ_LEVEL0_DAC:
            json_name = PMD_JSON_FAQ_LEVEL0_DAC;
            break;
        case PMD_FAQ_LEVEL1_DAC:
            json_name = PMD_JSON_FAQ_LEVEL1_DAC;
            break;
        case PMD_BIAS:
            json_name = PMD_JSON_BIAS;
            break;
        case PMD_MOD:
            json_name = PMD_JSON_MOD;
            break;
        case PMD_APDOI_CTRL:
            json_name = PMD_JSON_APDOI_CTRL;
            break;
        case PMD_TEMP_0:
            json_name = PMD_JSON_TEMP_0;
            break;
        case PMD_TEMP_COFF_H:
            json_name = PMD_JSON_TEMP_COFF_H;
            break;
        case PMD_TEMP_COFF_L:
            json_name = PMD_JSON_TEMP_COFF_L;
            break;
        case PMD_ESC_THR:
            json_name = PMD_JSON_ESC_THR;
            break;
        case PMD_ROGUE_THR:
            json_name = PMD_JSON_ROGUE_THR;
            break;
        case PMD_LEVEL_0_DAC:
            json_name = PMD_JSON_LEVEL_0_DAC;
            break;
        case PMD_AVG_LEVEL_1_DAC:
            json_name = PMD_JSON_AVG_LEVEL_1_DAC;
            break;
        case PMD_DUTY_CYCLE:
            json_name = PMD_JSON_DUTY_CYCLE;
            break;
        case PMD_TX_POWER:
            json_name = PMD_JSON_TX_POWER;
            break;
        case PMD_BIAS0:
            json_name = PMD_JSON_BIAS0;
            break;
        case PMD_TEMP_OFFSET:
            json_name = PMD_JSON_TEMP_OFFSET;
            break;
        case PMD_BIAS_DELTA_I:
            json_name = PMD_JSON_BIAS_DELTA_I;
            break;
        case PMD_SLOPE_EFFICIENCY:
            json_name = PMD_JSON_SLOPE_EFFICIENCY;
            break;
        default:
            break;
    }

    if (!json_name || !jobj)
    {
        ret = PMD_JSON_ERROR;
        goto parameter_to_json_file_exit;
    }

    /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'json_name' key might already be used */
    json_object_object_add_ex(calibration_json_object, json_name, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT);

    ret = json_object_to_file_ext(PMD_CALIBRATION_JSON_FILE_PATH, calibration_json_object, JSON_C_TO_STRING_SPACED |
        JSON_C_TO_STRING_PRETTY);
    if (ret)
    {
        fprintf(stderr, "Updating the PMD JSON file %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());
    }

parameter_to_json_file_exit:
    json_object_put(calibration_json_object);
    return ret;
}


static void replace_or_create_object_containing_object(struct json_object **new_object,
    struct json_object *calibration_json_object, const char *json_object_key_name, struct json_object *contained_jobj,
    const char *contained_json_key_name)
{
    int ret;

    ret = get_json_object_from_json(new_object, json_object_key_name, calibration_json_object);
    if (ret)
    {
        *new_object = json_object_new_object();
    }
    else
    {
        /* Increment the reference count */
        *new_object = json_object_get(*new_object);
    }

    /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'contained_json_key_name' key might already be used */
    json_object_object_add_ex(*new_object, contained_json_key_name, contained_jobj, JSON_C_OBJECT_KEY_IS_CONSTANT);
}


static int load_or_create_pmd_calibration_json(struct json_object **calibration_json_object)
{
    if (access(PMD_CALIBRATION_JSON_FILE_PATH, F_OK))
    {
        *calibration_json_object = json_object_new_object();
        set_json_pmd_manifest(*calibration_json_object);
        return PMD_JSON_SUCCESS;
    }

    *calibration_json_object = json_object_from_file(PMD_CALIBRATION_JSON_FILE_PATH);
    if (!*calibration_json_object)
    {
        fprintf(stderr, "Load PMD JSON data from %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());

        return PMD_JSON_ERROR;
    }

    return PMD_JSON_SUCCESS;
}


static void set_json_pmd_manifest(struct json_object *calibration_json_object)
{
    /* Invoke this function only with a new and empty calibration_json_object!!! */
    /* The JSON_C_OBJECT_ADD_KEY_IS_NEW flag is used!!!                          */
    char jenkins_host_cl[PMD_BUF_MAX_SIZE] = {0};
    char script_release[PMD_BUF_MAX_SIZE] = {0};
    char script_datetime[PMD_BUF_MAX_SIZE] = {0};
    struct json_object *jobj = NULL;
    struct json_object *manifest_json_object;
    uint32_t fw_version = 0;
    uint32_t expected_pmd_fw_ver = 0;
    uint32_t script_cl = 0;
    int ret;

    manifest_json_object = json_object_new_object();

    jobj = json_object_new_int(CALIBRATION_FILE_VERSION);
    json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_FILE_VER, jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    /* The next following are new non-legacy calibration parameters */

    ret = parse_pmd_calibration_script_manifest(script_release, &script_cl, script_datetime);
    if (ret)
    {
        jobj = NULL;
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_UTC_DATE_TIME, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_RELEASE, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_CL, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    }
    else
    {
        jobj = json_object_new_string(script_datetime);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_UTC_DATE_TIME, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

        jobj = json_object_new_string(script_release);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_RELEASE, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

        jobj = json_object_new_int(script_cl);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_CALIBRATION_TOOL_SUITE_CL, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    }

    ret = get_pmd_firmware_version(&fw_version);
    if (ret)
    {
        jobj = NULL;
    }
    else
    {
        jobj = json_object_new_int(fw_version);
    }
    json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_FW_VERSION, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    ret = get_host_expected_pmd_firmware_version(&expected_pmd_fw_ver);
    if (ret)
    {
        jobj = NULL;
    }
    else
    {
        jobj = json_object_new_int(expected_pmd_fw_ver);
    }
    json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_HOST_EXPECTED_FW_VER, jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (expected_pmd_fw_ver != fw_version)
    {
        fprintf(stderr, "ERROR: The version of the loaded PMD firmware doesn't match the PMD firmware version expected "
            "by the host!!!\n");
    }

    jobj = json_object_new_string(BRCM_RELEASETAG);
    json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_HOST_RELEASE, jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    ret = read_binary_file_to_buffer(JENKINS_CHANGELIST_FILE_PATH, jenkins_host_cl, PMD_BUF_MAX_SIZE);
    if (!ret)
    {
        jobj = json_object_new_string(jenkins_host_cl);
        json_object_object_add_ex(manifest_json_object, PMD_JSON_MANIFEST_HOST_CL, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
            JSON_C_OBJECT_ADD_KEY_IS_NEW);
    } /* else don't add a NULL object if no JENKINS host CL is available */

    json_object_object_add_ex(calibration_json_object, PMD_JSON_MANIFEST, manifest_json_object,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
}


static int parse_pmd_calibration_script_manifest(char *script_release, uint32_t *script_cl, char *script_datetime)
{
    int i;
    char *p;

    for (i = 0; (PMD_BUF_MAX_SIZE > i) && (';' != json_file_manifest[i]); i++)
        ;
    if (PMD_BUF_MAX_SIZE == i)
    {
        DEBUG_PRINTF("no ; in json_file_manifest.\n");
        return PMD_JSON_ERROR;
    }

    memcpy(script_release, json_file_manifest, i);
    i++;
    script_release[i] = 0;
    DEBUG_PRINTF("script_release in json_file_manifest is %s.\n", script_release);

    if (('0' > json_file_manifest[i]) || ('9' < json_file_manifest[i]))
    {
        DEBUG_PRINTF("no script_cl in json_file_manifest.\n");
        return PMD_JSON_ERROR;
    }

    *script_cl = strtol(json_file_manifest + i, &p, 10);
    DEBUG_PRINTF("script_cl in json_file_manifest is %lu.\n", (unsigned long)(*script_cl));

    if ((!p) || (';' != *p))
    {
        DEBUG_PRINTF("no 2nd ; in json_file_manifest.\n");
        return PMD_JSON_ERROR;
    }

    strcpy(script_datetime, p + 1);
    DEBUG_PRINTF("script_datetime in json_file_manifest is %s.\n", script_datetime);

    return PMD_JSON_SUCCESS;
}


#define PMD_FW_VERSION_LENGTH 4
static int get_pmd_firmware_version(uint32_t *fw_version)
{
    unsigned char buf[PMD_FW_VERSION_LENGTH];
    pmd_params fw_version_param = {.offset = PMD_FW_VERSION_GET_MSG, .len = PMD_FW_VERSION_LENGTH, .buf = buf};
    int ret = -1, laser_dev_fd;

    laser_dev_fd = open(LASER_DEV, O_RDWR);
    if (ret != laser_dev_fd)
    {
        ret = ioctl(laser_dev_fd, PMD_IOCTL_MSG_READ, &fw_version_param);
        close(laser_dev_fd);
    }

    if (!ret)
    {
        *fw_version = *(uint32_t *)(fw_version_param.buf);
        *fw_version = ((*fw_version & (uint32_t)0x000000ffUL) << 8) | ((*fw_version & (uint32_t)0x0000ff00UL) >> 8) |
             ((*fw_version & (uint32_t)0x00ff0000UL) << 8) | ((*fw_version & (uint32_t)0xff000000UL) >> 8);
    }

    return ret;
}


static int get_host_expected_pmd_firmware_version(uint32_t *expected_pmd_fw_ver)
{
#if defined(PMD_VERSION_SW)
    char *p = PMD_VERSION_SW;

    while (*p)
    {
        if (('0' <= *p) && ('9' >= *p))
        {
            *expected_pmd_fw_ver = strtol(p, &p, 10);
            return PMD_JSON_SUCCESS;
        }
        p++;
    }
    fprintf(stderr, "The host expected PMD firmware version is corrupted.\n");
#else
    fprintf(stderr, "The host expected PMD firmware version is unavailable.\n");
#endif
    return PMD_JSON_ERROR;
}


static void replace_or_create_array_element_containing_object(struct json_object **new_jarray,
    struct json_object *calibration_json_object, const char *json_array_key_name, uint16_t element_index,
    struct json_object *contained_jobj, const char *contained_json_key_name)
{
    struct json_object *new_array_element = NULL;
    int ret;

    ret = get_json_array_from_json(new_jarray, json_array_key_name, calibration_json_object);
    if (ret)
    {
        new_array_element = json_object_new_object();
    }
    else
    {
        char info_str[INFO_STR_LENGTH] = {0};

        new_array_element = json_object_array_get_idx(*new_jarray, element_index);
        snprintf(info_str, INFO_STR_LENGTH, "index [%d] in array \"%s\"", element_index, json_array_key_name);
        ret = verify_object_type_and_get_value(&new_array_element, new_array_element, json_type_object, info_str);
        if (ret)
        {
            new_array_element = json_object_new_object();
        }
        else
        {
            /* Increment the reference count */
            new_array_element = json_object_get(new_array_element);
        }
    }
    /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'contained_json_key_name' key might already be used */
    json_object_object_add_ex(new_array_element, contained_json_key_name, contained_jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT);

    replace_or_create_array_element(new_jarray, calibration_json_object, json_array_key_name, new_array_element,
        element_index);
}


static void replace_or_create_array_element(struct json_object **new_jarray,
    struct json_object *calibration_json_object, const char *json_key_name, struct json_object *jarray_element,
    uint16_t element_index)
{
    int ret;

    if (CAL_MULT_LEN <= element_index)
    {
        fprintf(stderr, "Error: Can't put '%s' object at array index %d (max index is %d).\n", json_key_name,
            element_index, CAL_MULT_LEN - 1);
        *new_jarray = NULL;
        return;
    }

    ret = get_json_array_from_json(new_jarray, json_key_name, calibration_json_object);
    if (ret)
    {
        *new_jarray = json_object_new_array();
    }
    else
    {
        /* Increment the reference count */
        *new_jarray = json_object_get(*new_jarray);
    }

    ret = json_object_array_put_idx(*new_jarray, element_index, jarray_element);
    if (ret)
    {
        fprintf(stderr, "Error: Failed to put '%s' object at array index %d.\n", json_key_name, element_index);
        json_object_put(*new_jarray);
        *new_jarray = NULL;
    }
}


int set_pmd_wan_type(net_port_t *net_port)
{
    int ret, laser_dev_fd;
    pmd_calibration_parameters calibration_binary = {0};
    pmd_params param;

    laser_dev_fd = open(LASER_DEV, O_RDWR);
    if (-1 == laser_dev_fd)
    {
        /* Does not fail if PMD /dev/laser is not preset. */
        return PMD_JSON_SUCCESS;
    }

    ret = load_pmd_calibration_data(&calibration_binary);
    if (ret)
    {
        param.buf = NULL;
        fprintf(stderr, "Error: Failed to load PMD calibration data, ret=%d\n", ret);
    }
    else
    {
        param.buf = (unsigned char *)&calibration_binary;
    }

    param.net_port = *net_port;

    ret = ioctl(laser_dev_fd, PMD_IOCTL_SET_WAN_TYPE, &param);
    if (ret)
    {
        if (errno == EINVAL)
            ret = 0;
        else
            fprintf(stderr, "Error: Failed to configure PMD with wan type, ret=%d\n", ret);
    }

    close(laser_dev_fd);

    return ret;
}


int load_pmd_calibration_data(pmd_calibration_parameters *calibration_binary)
{
    if (access(PMD_CALIBRATION_JSON_FILE_PATH, F_OK))
    {
        fprintf(stderr, "Warning: PMD JSON calibration file is missing.\nTry generating %s from %s.\n",
            PMD_CALIBRATION_JSON_FILE_PATH, PMD_CALIBRATION_FILE_PATH);
        generate_pmd_json_calibration_file();
    }

    return load_pmd_json_calibration_file_to_struct(calibration_binary);
}


static int load_pmd_json_calibration_file_to_struct(pmd_calibration_parameters *calibration_binary)
{
    struct json_object *calibration_json_object;

    calibration_json_object = json_object_from_file(PMD_CALIBRATION_JSON_FILE_PATH);
    if (!calibration_json_object)
    {
        fprintf(stderr, "Load PMD JSON data from %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());

        return PMD_JSON_ERROR;
    }

    printf("PMD JSON data from %s loaded successfully.\n", PMD_CALIBRATION_JSON_FILE_PATH);
#if defined(PMD_JSON_DEBUG)
    printf("\n%s\n", json_object_to_json_string_ext(calibration_json_object, JSON_C_TO_STRING_SPACED |
        JSON_C_TO_STRING_PRETTY));
#endif

    pmd_calibration_json_to_struct(calibration_json_object, calibration_binary);
    json_object_put(calibration_json_object);

    return PMD_JSON_SUCCESS;
}


#define JSON_INT_TO_BINARY(legacy_value_name, json_key_name) do                              \
    {                                                                                        \
        ret = verify_and_get_int_by_key(&int_value, json_key_name, calibration_json_object); \
        if (!ret)                                                                            \
        {                                                                                    \
            calibration_binary->legacy_value_name.valid = 1;                                 \
            calibration_binary->legacy_value_name.val = int_value; /*may cast to int16_t */  \
        }                                                                                    \
    } while(0)

static void pmd_calibration_json_to_struct(const struct json_object *calibration_json_object,
    pmd_calibration_parameters *calibration_binary)
{
    int ret, ret2, ret3, ret4, ret5, i, int_value;
    struct json_object *jobj, *jarray;
    char info_str[INFO_STR_LENGTH] = {0};

    memset(calibration_binary, 0x00, sizeof(pmd_calibration_parameters));

    /* int objects */
    JSON_INT_TO_BINARY(watermark, PMD_JSON_FILE_WATERMARK);
    JSON_INT_TO_BINARY(level_0_dac, PMD_JSON_FAQ_LEVEL0_DAC);
    JSON_INT_TO_BINARY(level_1_dac, PMD_JSON_FAQ_LEVEL1_DAC);
    JSON_INT_TO_BINARY(bias, PMD_JSON_BIAS);
    JSON_INT_TO_BINARY(mod, PMD_JSON_MOD);
    JSON_INT_TO_BINARY(apdoi_ctrl, PMD_JSON_APDOI_CTRL);
    JSON_INT_TO_BINARY(temp_0, PMD_JSON_TEMP_0);
    JSON_INT_TO_BINARY(coff_h, PMD_JSON_TEMP_COFF_H);
    JSON_INT_TO_BINARY(coff_l, PMD_JSON_TEMP_COFF_L);
    JSON_INT_TO_BINARY(esc_th, PMD_JSON_ESC_THR);
    JSON_INT_TO_BINARY(rogue_th, PMD_JSON_ROGUE_THR);
    JSON_INT_TO_BINARY(avg_level_0_dac, PMD_JSON_LEVEL_0_DAC);
    JSON_INT_TO_BINARY(avg_level_1_dac, PMD_JSON_AVG_LEVEL_1_DAC);
    JSON_INT_TO_BINARY(duty_cycle, PMD_JSON_DUTY_CYCLE);
    JSON_INT_TO_BINARY(tx_power, PMD_JSON_TX_POWER);
    JSON_INT_TO_BINARY(bias0, PMD_JSON_BIAS0);
    JSON_INT_TO_BINARY(temp_offset, PMD_JSON_TEMP_OFFSET);
    JSON_INT_TO_BINARY(bias_delta_i, PMD_JSON_BIAS_DELTA_I);
    JSON_INT_TO_BINARY(slope_efficiency, PMD_JSON_SLOPE_EFFICIENCY);

    /* objects of multiple different PMD parameters */
    ret = get_json_object_from_json(&jobj, PMD_JSON_MANIFEST, calibration_json_object);
    if (!ret)
    {
        int file_version;

        ret = verify_and_get_int_by_key(&file_version, PMD_JSON_MANIFEST_CALIBRATION_FILE_VER, jobj);
        if (!ret)
        {
            calibration_binary->version.val = file_version;
            calibration_binary->version.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_RSSI_COEFFICIENTS, calibration_json_object);
    if (!ret)
    {
        int coefficient;

        ret = verify_and_get_int_by_key(&coefficient, PMD_JSON_RSSI_COEFFICIENTS_A, jobj);
        if (!ret)
        {
            calibration_binary->rssi_a_cal.val = coefficient;
            calibration_binary->rssi_a_cal.valid = 1;
        }
        ret = verify_and_get_int_by_key(&coefficient, PMD_JSON_RSSI_COEFFICIENTS_B, jobj);
        if (!ret)
        {
            calibration_binary->rssi_b_cal.val = coefficient;
            calibration_binary->rssi_b_cal.valid = 1;
        }
        ret = verify_and_get_int_by_key(&coefficient, PMD_JSON_RSSI_COEFFICIENTS_C, jobj);
        if (!ret)
        {
            calibration_binary->rssi_c_cal.val = coefficient;
            calibration_binary->rssi_c_cal.valid = 1;
        }
    }

    /* objects of int object fields */
    ret = get_json_object_from_json(&jobj, PMD_JSON_APD, calibration_json_object);
    if (!ret)
    {
        int type, voltage;

        ret = verify_and_get_int_by_key(&type, PMD_JSON_APD_TYPE, jobj);
        ret2 = verify_and_get_int_by_key(&voltage, PMD_JSON_APD_VOLTAGE, jobj);
        if (!ret && !ret2)
        {
            uint16_t en = 0x800;
            calibration_binary->apd.val = type << 10 | voltage | en;
            calibration_binary->apd.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_MPD_CHANNEL_CONFIG, calibration_json_object);
    if (!ret)
    {
        int vga, tia;

        ret = verify_and_get_int_by_key(&vga, PMD_JSON_MPD_CHANNEL_CONFIG_VGA, jobj);
        ret2 = verify_and_get_int_by_key(&tia, PMD_JSON_MPD_CHANNEL_CONFIG_TIA, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->mpd_config.val = (tia & 0xf) << 12 | (vga & 0x3) << 10;
            calibration_binary->mpd_config.valid = 1;
        }

        ret = verify_and_get_int_by_key(&int_value, PMD_JSON_MPD_CHANNEL_CONFIG_DACRANGE, jobj);
        if (!ret)
        {
            calibration_binary->dacrange.val = int_value;
            calibration_binary->dacrange.valid = 1;
        }

        ret = verify_and_get_int_by_key(&int_value, PMD_JSON_MPD_CHANNEL_CONFIG_CALIBCTRL, jobj);
        if (!ret)
        {
            calibration_binary->calibctrl.val = int_value;
            calibration_binary->calibctrl.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_MPD_GAINS, calibration_json_object);
    if (!ret)
    {
        int bias_gain, mod_gain;

        ret = verify_and_get_int_by_key(&bias_gain, PMD_JSON_MPD_GAINS_BIAS_GAIN, jobj);
        ret2 = verify_and_get_int_by_key(&mod_gain, PMD_JSON_MPD_GAINS_MOD_GAIN, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->mpd_gains.val = mod_gain << 8 | bias_gain;
            calibration_binary->mpd_gains.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_LOS_THR, calibration_json_object);
    if (!ret)
    {
        int assert, deassert;

        ret = verify_and_get_int_by_key(&assert, PMD_JSON_LOS_THR_ASSERT, jobj);
        ret2 = verify_and_get_int_by_key(&deassert, PMD_JSON_LOS_THR_DEASSERT, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->los_thr.val = (deassert & 0xff) << 8 | (assert & 0xff);
            calibration_binary->los_thr.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_SAT_POS, calibration_json_object);
    if (!ret)
    {
        int high, low;

        ret = verify_and_get_int_by_key(&high, PMD_JSON_SAT_POS_HIGH, jobj);
        ret2 = verify_and_get_int_by_key(&low, PMD_JSON_SAT_POS_LOW, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->sat_pos.val = (low & 0xff) << 8 | (high & 0xff);
            calibration_binary->sat_pos.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_SAT_NEG, calibration_json_object);
    if (!ret)
    {
        int high, low;

        ret = verify_and_get_int_by_key(&high, PMD_JSON_SAT_NEG_HIGH, jobj);
        ret2 = verify_and_get_int_by_key(&low, PMD_JSON_SAT_NEG_LOW, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->sat_neg.val = (low & 0xff) << 8 | (high & 0xff);
            calibration_binary->sat_neg.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_ADF_LOS_THRESHOLDS, calibration_json_object);
    if (!ret)
    {
        int assert, deassert;

        ret = verify_and_get_int_by_key(&assert, PMD_JSON_ADF_LOS_THRESHOLDS_ASSERT, jobj);
        ret2 = verify_and_get_int_by_key(&deassert, PMD_JSON_ADF_LOS_THRESHOLDS_DEASSERT, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->adf_los_thresholds.val = (deassert << 16) | assert;
            calibration_binary->adf_los_thresholds.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_ADF_LOS_LEAKY_BUCKET, calibration_json_object);
    if (!ret)
    {
        int assert, lb_bucket_sz;

        ret = verify_and_get_int_by_key(&assert, PMD_JSON_ADF_LOS_LEAKY_BUCKET_ASSERT, jobj);
        ret2 = verify_and_get_int_by_key(&lb_bucket_sz, PMD_JSON_ADF_LOS_LEAKY_BUCKET_LB_BUCKET_SZ, jobj);
        if (!ret && !ret2)
        {
            calibration_binary->adf_los_leaky_bucket.val = (lb_bucket_sz & 0xff) << 8 | (assert & 0xff);
            calibration_binary->adf_los_leaky_bucket.valid = 1;
        }
    }

    ret = get_json_object_from_json(&jobj, PMD_JSON_COMPENSATION, calibration_json_object);
    if (!ret)
    {
        int compensation_enable0, compensation_enable1, die_temp_ref, compensation_coeff1_q8, compensation_coeff2_q8;

        ret = copy_boolean_from_json(&compensation_enable0, PMD_JSON_COMPENSATION_ENABLE0, jobj);
        ret2 = copy_boolean_from_json(&compensation_enable1, PMD_JSON_COMPENSATION_ENABLE1, jobj);
        ret3 = verify_and_get_int_by_key(&die_temp_ref, PMD_JSON_COMPENSATION_DIE_TEMP_REF, jobj);
        ret4 = verify_and_get_int_by_key(&compensation_coeff1_q8, PMD_JSON_COMPENSATION_COEFF1_Q8, jobj);
        ret5 = verify_and_get_int_by_key(&compensation_coeff2_q8, PMD_JSON_COMPENSATION_COEFF2_Q8, jobj);
        if (!ret && !ret2 && !ret3 && !ret4 && !ret5)
        {
            calibration_binary->compensation.val = (compensation_coeff2_q8 << 24) | (compensation_coeff1_q8 << 16) |
                (die_temp_ref << 8) | (compensation_enable1 << 1) | compensation_enable0;
            calibration_binary->compensation.valid = 1;
        }
    }


    /* ------------- */
    /* array objects */
    /* ------------- */

    /* arrays of objects */
    ret = get_json_array_from_json(&jarray, PMD_JSON_EYE_SHAPING_COEFFICIENTS, calibration_json_object);
    if (!ret)
    {
        for (i = 0; i < CAL_MULT_LEN; i++)
        {
            struct json_object *jarray_element;

            jarray_element = json_object_array_get_idx(jarray, i);
            if (!jarray_element)
            {
                DEBUG_PRINTF("Notice: JSON array '%s' is empty at index %d. (expected json_type_object).\n",
                    PMD_JSON_EYE_SHAPING_COEFFICIENTS, i);
                continue;
            }

            ret = verify_type_and_get_object_by_key_from_object(&jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE,
                json_type_object, jarray_element);
            if (!ret)
            {
                int rate, dload;

                ret = verify_and_get_int_by_key(&rate, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_RATE, jobj);
                ret2 = verify_and_get_int_by_key(&dload, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_DLOAD, jobj);
                if (!ret && !ret2)
                {
                    calibration_binary->edge_rate.val[i] = (dload & 0xff) << 8 | (rate & 0xff);
                    calibration_binary->edge_rate.valid[i] = 1;
                }
            }

            ret = verify_and_get_int_by_key(&int_value, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_WEIGHT,
                jarray_element);
            if (!ret)
            {
                calibration_binary->preemphasis_weight.val[i] = int_value;
                calibration_binary->preemphasis_weight.valid[i] = 1;
            }

            ret = verify_and_get_int_by_key(&int_value, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_DELAY,
                jarray_element);
            if (!ret)
            {
                calibration_binary->preemphasis_delay.val[i] = int_value;
                calibration_binary->preemphasis_delay.valid[i] = 1;
            }
        }
    }

    /* arrays of int objects */
    ret = get_json_array_from_json(&jarray, PMD_JSON_RES2TEMP, calibration_json_object);
    if (!ret)
    {
        calibration_binary->temp_table.valid = 1;

        for (i = 0; i < TEMP_TABLE_SIZE; i++)
        {            
            jobj = json_object_array_get_idx(jarray, i);
            snprintf(info_str, INFO_STR_LENGTH, "index [%d] in array \"%s\"", i, PMD_JSON_RES2TEMP);
            ret = verify_object_type_and_get_value(&int_value, jobj, json_type_int, info_str);
            if (ret)
            {
                fprintf(stderr, "Error: Invalid %s table.\n", PMD_JSON_RES2TEMP);
                calibration_binary->temp_table.valid = 0;
                break; /* ignore incompletet table */
            }

            calibration_binary->temp_table.val[i] = int_value;
        }
    }

    ret = get_json_array_from_json(&jarray, PMD_JSON_TEMP2APD, calibration_json_object);
    if (!ret)
    {
        calibration_binary->temp2apd_table.valid = 1;

        for (i = 0; i < APD_TEMP_TABLE_SIZE; i++)
        {            
            jobj = json_object_array_get_idx(jarray, i);
            snprintf(info_str, INFO_STR_LENGTH, "index [%d] in array \"%s\"", i, PMD_JSON_TEMP2APD);
            ret = verify_object_type_and_get_value(&int_value, jobj, json_type_int, info_str);
            if (ret)
            {
                fprintf(stderr, "Error: Invalid %s table.\n", PMD_JSON_TEMP2APD);
                calibration_binary->temp2apd_table.valid = 0;
                break; /* ignore incompletet table */
            }

            calibration_binary->temp2apd_table.val[i] = int_value;
        }
    }
}


static int verify_type_and_get_object_by_key_from_object(void *jobj_destination, const char *json_key_name,
    const enum json_type expected_jobj_type, const struct json_object *jobj_source)
{
    struct json_object *jobj;
    json_bool jbool;
    char info_str[INFO_STR_LENGTH] = {0};

    jbool = json_object_object_get_ex(jobj_source, json_key_name, &jobj);
    if (!jbool)
    {
        DEBUG_PRINTF("Warning: JSON key '%s' is missing.\n", json_key_name);
        return PMD_JSON_ERROR;
    }

    snprintf(info_str, INFO_STR_LENGTH, "value for JSON key \"%s\"", json_key_name);
    return verify_object_type_and_get_value(jobj_destination, jobj, expected_jobj_type, info_str);
}


static int verify_and_get_int_by_key(int *int_destination, const char *json_key_name,
    const struct json_object *calibration_json_object)
{
    return verify_type_and_get_object_by_key_from_object(int_destination, json_key_name, json_type_int,
        calibration_json_object);
}


static int copy_boolean_from_json(int *bool_destination, const char *json_key_name,
    const struct json_object *calibration_json_object)
{
    struct json_object *jobj;
    json_bool jbool;
    char info_str[INFO_STR_LENGTH] = {0};

    jbool = json_object_object_get_ex(calibration_json_object, json_key_name, &jobj);
    if (!jbool)
    {
        DEBUG_PRINTF("Warning: JSON key '%s' is missing.\n", json_key_name);
        return PMD_JSON_ERROR;
    }

    snprintf(info_str, INFO_STR_LENGTH, "value for JSON key \"%s\"", json_key_name);
    return verify_object_type_and_get_value(bool_destination, jobj, json_type_boolean, info_str);
}


static int get_json_object_from_json(struct json_object **jobj_destination, const char *json_key_name,
    const struct json_object *calibration_json_object)
{
    json_bool jbool;
    char info_str[INFO_STR_LENGTH] = {0};

    jbool = json_object_object_get_ex(calibration_json_object, json_key_name, jobj_destination);
    if (!jbool)
    {
        DEBUG_PRINTF("Warning: JSON key '%s' is missing.\n", json_key_name);
        return PMD_JSON_ERROR;
    }

    snprintf(info_str, INFO_STR_LENGTH, "value for JSON key \"%s\"", json_key_name);
    return verify_object_type_and_get_value(jobj_destination, *jobj_destination, json_type_object, info_str);
}


static int get_json_array_from_json(struct json_object **jarray_destination, const char *json_key_name,
    const struct json_object *calibration_json_object)
{
    json_bool jbool;
    char info_str[INFO_STR_LENGTH] = {0};

    jbool = json_object_object_get_ex(calibration_json_object, json_key_name, jarray_destination);
    if (!jbool)
    {
        DEBUG_PRINTF("Warning: JSON key '%s' is missing.\n", json_key_name);
        return PMD_JSON_ERROR;
    }

    snprintf(info_str, INFO_STR_LENGTH, "value for JSON key \"%s\"", json_key_name);
    return verify_object_type_and_get_value(jarray_destination, *jarray_destination, json_type_array, info_str);
}


static int verify_object_type_and_get_value(void *result, const struct json_object *jobj,
    const enum json_type expected_jobj_type, const char *info_str)
{
    /* DO NOT USE for NULL JSON objects */
    enum json_type jobj_type;

    if (!jobj)
    {
        DEBUG_PRINTF("Notice: There is no JSON object '%s'. Expected %s object.\n", info_str,
            json_type_to_name(expected_jobj_type));
        return PMD_JSON_ERROR;
    }

    jobj_type = json_object_get_type(jobj);
    if (json_type_null == jobj_type)
    {
        DEBUG_PRINTF("Notice: JSON object '%s' is of type json_type_null. Expected %s.\n", info_str,
            json_type_to_name(expected_jobj_type));
        return PMD_JSON_ERROR;
    }
    if (expected_jobj_type != jobj_type)
    {
        fprintf(stderr, "Error: JSON object '%s' is of type %s. Expected %s.\n", info_str,
            json_type_to_name(jobj_type), json_type_to_name(expected_jobj_type));
        return PMD_JSON_ERROR;
    }

    switch (expected_jobj_type)
    {
        case json_type_int:
            *(int *)result = json_object_get_int(jobj);
            break;
        case json_type_boolean:
            *(json_bool *)result = json_object_get_boolean(jobj);
            break;
        case json_type_object:
            *(const struct json_object **)result = jobj;
            break;
        case json_type_array:
            *(const struct json_object **)result = jobj;
            break;
        case json_type_string:
            *(const char **)result = json_object_get_string((struct json_object *)jobj);
            break;
        case json_type_double:
            *(double *)result = json_object_get_double(jobj);
            break;
        default:
            fprintf(stderr, "Error: JSON object '%s' of expected type (%d) - %s is not supported.\n", info_str,
                (int)expected_jobj_type, json_type_to_name(expected_jobj_type));
            return PMD_JSON_ERROR;
    }
    
    return PMD_JSON_SUCCESS;
}


static void generate_pmd_json_calibration_file(void)
{
    pmd_calibration_legacy legacy_calibration_binary = {0};
    temperature_to_apd_table temp2apd_table = {0};
    struct json_object *calibration_json_object = NULL;
    int ret;

    ret = read_binary_file_to_buffer(PMD_CALIBRATION_FILE_PATH, &legacy_calibration_binary,
        sizeof(pmd_calibration_legacy));
    if (ret)
    {
        fprintf(stderr, "The PMD is not calibrated!!!\n");
        /* DO NOT generate a default JSON file. There may be no PMD on the board! */
        return;
    }
    if (!legacy_calibration_binary.version.valid || !legacy_calibration_binary.watermark.valid)
    {
        fprintf(stderr, "The PMD legacy calibration file is corrupted!!!\n");
        /* DO NOT generate a default JSON file. There may be no PMD on the board! */
        return;
    }
    if (CAL_FILE_WATERMARK != legacy_calibration_binary.watermark.val)
    {
        fprintf(stderr, "The PMD legacy calibration file watermark is corrupted!!!\n");
        /* DO NOT generate a default JSON file. There may be no PMD on the board! */
        return;
    }
    if (LEGACY_CALIBRATION_FILE_MAX_SUPPORTED_VERSION < legacy_calibration_binary.version.val ||
        LEGACY_CALIBRATION_FILE_MIN_SUPPORTED_VERSION > legacy_calibration_binary.version.val)
    {
        fprintf(stderr, "The PMD legacy calibration file version is corrupted!!!\n");
        /* DO NOT generate a default JSON file. There may be no PMD on the board! */
        return;
    }

    ret = read_binary_file_to_buffer(PMD_TEMP2APD_FILE_PATH, temp2apd_table.val,
        (long)OFFSET_OF(temperature_to_apd_table, valid));
    if (ret)
    {
        fprintf(stderr, "The temperature to APD table is not ready!!!\n");
        /* DO NOT generate a default JSON file. There may be no PMD on the board! */
        /* APD is optional */
    }
    else
    {
        temp2apd_table.valid = 1;
    }

    pmd_calibration_struct_to_json(&legacy_calibration_binary, &temp2apd_table, &calibration_json_object);

#if defined(PMD_JSON_DEBUG)
    printf("\n\n\n#########  AUTO GENERATED JSON FILE #########\n");
    printf("PMD calibration binary legacy file size %zu bytes.\n%s\n\n\n", sizeof(pmd_calibration_legacy),
        json_object_to_json_string_ext(calibration_json_object, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
    printf("\n#############################################\n\n\n");
#endif

    ret = json_object_to_file_ext(PMD_CALIBRATION_JSON_FILE_PATH, calibration_json_object, JSON_C_TO_STRING_SPACED |
        JSON_C_TO_STRING_PRETTY);
    if (ret)
    {
        fprintf(stderr, "Creating PMD JSON file %s failed.\n", PMD_CALIBRATION_JSON_FILE_PATH);
        fprintf(stderr, "Error: '%s'.\n", json_util_get_last_err());
    }

    json_object_put(calibration_json_object);
}


static int read_binary_file_to_buffer(const char *filename, void *binary_data, const long max_data_length)
{
    FILE *bin_file;
    long file_Length;
    size_t num_of_read_elements;
    off_t fseek_ret;
    int ret;

    memset(binary_data, 0x00, max_data_length);

    bin_file = fopen(filename, "rb");
    if (!bin_file)
    {
        fprintf(stderr, "Unable to open the %s file.\n", filename);
        ret = PMD_JSON_ERROR;
        goto exit_read_binary;
    }
    
    fseek_ret = fseek(bin_file, 0, SEEK_END);
    if (PMD_JSON_ERROR == fseek_ret)
    {
        fprintf(stderr, "Can't retrieve the %s file size.\n", filename);
        ret = PMD_JSON_ERROR;
        goto exit_read_binary;
    }

    file_Length = ftell(bin_file);
    rewind(bin_file);

    if (max_data_length < file_Length)
    {
        fprintf(stderr, "The size (%ld) of the %s file is bigger than expected (%ld).\n", file_Length,
            filename, max_data_length);
        ret = PMD_JSON_ERROR;
        goto exit_read_binary;
    }

    num_of_read_elements = fread(binary_data, file_Length, 1, bin_file);
    if (1 != num_of_read_elements)
    {
        fprintf(stderr, "Failure in reading the %s file.\n", filename);
        ret = PMD_JSON_ERROR;
        goto exit_read_binary;
    }
#if defined(PMD_JSON_DEBUG)
    print_binary_buffer(binary_data, max_data_length);
#endif

    ret = PMD_JSON_SUCCESS;

exit_read_binary:
    if (bin_file)
        fclose(bin_file);

    return ret;
}


#if defined(PMD_JSON_DEBUG)
static void print_binary_buffer(const void *binary_data, const long data_length)
{
    int i;

    printf("\n");
    for (i = 0; i < data_length; i++)
    {
        if (0 == i % 16)
        {
            printf("\n%3d: ", i);
        }
        else if (0 == i % 8)
        {
            printf("  ");
        }
        else if (0 == i % 4)
        {
            printf(" ");
        }
        printf("%02X ", ((unsigned char *)binary_data)[i]);
    }
    printf("\n\n");
}
#endif


#define ADD_LEGACY_INT_TO_JSON(legacy_value_name, json_name) do                     \
    {                                                                               \
        if (calibration_binary->legacy_value_name.valid)                            \
        {                                                                           \
            jint = json_object_new_int(calibration_binary->legacy_value_name.val);  \
        }                                                                           \
        else                                                                        \
        {                                                                           \
            jint = NULL;                                                            \
        }                                                                           \
        json_object_object_add_ex(*calibration_json_object, json_name, jint,        \
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);          \
    } while(0)

static void pmd_calibration_struct_to_json(const pmd_calibration_legacy *calibration_binary,
    const temperature_to_apd_table *temp2apd_table, struct json_object **calibration_json_object)
{
    struct json_object *jint, *jarray, *jobj, *manifest_jobj, *a_jint, *b_jint, *c_jint, *channel_jobj = NULL;
    int i;

    *calibration_json_object = json_object_new_object();

    /* int objects */
    ADD_LEGACY_INT_TO_JSON(watermark, PMD_JSON_FILE_WATERMARK);
    ADD_LEGACY_INT_TO_JSON(level_0_dac, PMD_JSON_FAQ_LEVEL0_DAC);
    ADD_LEGACY_INT_TO_JSON(level_1_dac, PMD_JSON_FAQ_LEVEL1_DAC);
    ADD_LEGACY_INT_TO_JSON(bias, PMD_JSON_BIAS);
    ADD_LEGACY_INT_TO_JSON(mod, PMD_JSON_MOD);
    ADD_LEGACY_INT_TO_JSON(apdoi_ctrl, PMD_JSON_APDOI_CTRL);
    ADD_LEGACY_INT_TO_JSON(temp_0, PMD_JSON_TEMP_0);
    ADD_LEGACY_INT_TO_JSON(coff_h, PMD_JSON_TEMP_COFF_H);
    ADD_LEGACY_INT_TO_JSON(coff_l, PMD_JSON_TEMP_COFF_L);
    ADD_LEGACY_INT_TO_JSON(esc_th, PMD_JSON_ESC_THR);
    ADD_LEGACY_INT_TO_JSON(rogue_th, PMD_JSON_ROGUE_THR);
    ADD_LEGACY_INT_TO_JSON(avg_level_0_dac, PMD_JSON_LEVEL_0_DAC);
    ADD_LEGACY_INT_TO_JSON(avg_level_1_dac, PMD_JSON_AVG_LEVEL_1_DAC);
    ADD_LEGACY_INT_TO_JSON(duty_cycle, PMD_JSON_DUTY_CYCLE);
    ADD_LEGACY_INT_TO_JSON(tx_power, PMD_JSON_TX_POWER);
    ADD_LEGACY_INT_TO_JSON(bias0, PMD_JSON_BIAS0);
    ADD_LEGACY_INT_TO_JSON(temp_offset, PMD_JSON_TEMP_OFFSET);
    ADD_LEGACY_INT_TO_JSON(bias_delta_i, PMD_JSON_BIAS_DELTA_I);
    ADD_LEGACY_INT_TO_JSON(slope_efficiency, PMD_JSON_SLOPE_EFFICIENCY);

    /* objects of multiple different PMD parameters */
    manifest_jobj = json_object_new_object();
    if (!calibration_binary->version.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = json_object_new_int(calibration_binary->version.val);
    }
    json_object_object_add_ex(manifest_jobj, PMD_JSON_MANIFEST_CALIBRATION_FILE_VER, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_MANIFEST, manifest_jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->rssi_a_cal.valid)
    {
        a_jint = NULL;
    }
    else
    {
        a_jint = json_object_new_int(calibration_binary->rssi_a_cal.val);
    }
    if (!calibration_binary->rssi_b_cal.valid)
    {
        b_jint = NULL;
    }
    else
    {
        b_jint = json_object_new_int(calibration_binary->rssi_b_cal.val);
    }
    if (!calibration_binary->rssi_c_cal.valid)
    {
        c_jint = NULL;
    }
    else
    {
        c_jint = json_object_new_int(calibration_binary->rssi_c_cal.val);
    }
    jobj = create_pmd_rssi_coefficients_json_object(a_jint, b_jint, c_jint);
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_RSSI_COEFFICIENTS, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->mpd_config.valid)
    {
        a_jint = NULL;
        b_jint = NULL;
    }
    else
    {
        int val = calibration_binary->mpd_config.val;
        a_jint = json_object_new_int((val & 0xc00) >> 10); /* vga */
        b_jint = json_object_new_int((val & 0xF000) >> 12); /* tia */
    }
    /* must come before dacrange and calibctrl */
    jobj = create_pmd_mpd_channel_config_vga_tia_json_object(a_jint, b_jint);
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->dacrange.valid)
    {
        jint = NULL;
    }
    else
    {
        jint = json_object_new_int(calibration_binary->dacrange.val);
    }
    replace_or_create_object_containing_object(&channel_jobj, *calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG,
        jint, PMD_JSON_MPD_CHANNEL_CONFIG_DACRANGE);
    /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'PMD_JSON_MPD_CHANNEL_CONFIG' key is already used above */
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG, channel_jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT);

    if (!calibration_binary->calibctrl.valid)
    {
        jint = NULL;
    }
    else
    {
        jint = json_object_new_int(calibration_binary->calibctrl.val);
    }
    replace_or_create_object_containing_object(&channel_jobj, *calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG,
        jint, PMD_JSON_MPD_CHANNEL_CONFIG_CALIBCTRL);
    /* DO NOT USE the JSON_C_OBJECT_ADD_KEY_IS_NEW flag. The 'PMD_JSON_MPD_CHANNEL_CONFIG' key is already used above */
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_MPD_CHANNEL_CONFIG, channel_jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT);

    /* objects of int object fields */
    if (!calibration_binary->apd.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_apd_json_object(calibration_binary->apd.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_APD, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->mpd_gains.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_mpd_gains_json_object(calibration_binary->mpd_gains.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_MPD_GAINS, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->los_thr.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_los_thr_json_object(calibration_binary->los_thr.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_LOS_THR, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->sat_pos.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_sat_pos_json_object(calibration_binary->sat_pos.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_SAT_POS, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->sat_neg.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_sat_neg_json_object(calibration_binary->sat_neg.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_SAT_NEG, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->adf_los_thresholds.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_adf_los_thresholds_json_object(calibration_binary->adf_los_thresholds.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_ADF_LOS_THRESHOLDS, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->adf_los_leaky_bucket.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_adf_los_leaky_bucket_json_object(calibration_binary->adf_los_leaky_bucket.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_ADF_LOS_LEAKY_BUCKET, jobj,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!calibration_binary->compensation.valid)
    {
        jobj = NULL;
    }
    else
    {
        jobj = create_pmd_compensation_json_object(calibration_binary->compensation.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_COMPENSATION, jobj, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);


    /* ------------- */
    /* array objects */
    /* ------------- */

    /* arrays of objects */
    jarray = json_object_new_array();
    for (i = 0; i < CAL_MULT_LEN; i++)
    {
        struct json_object *jarray_element;

        jarray_element = json_object_new_object();

        if (!calibration_binary->edge_rate.valid[i])
        {
            jobj = NULL;
        }
        else
        {
            jobj = create_pmd_edge_rate_json_object(calibration_binary->edge_rate.val[i]);
        }
        json_object_object_add_ex(jarray_element, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE, jobj,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

        if (!calibration_binary->preemphasis_weight.valid[i])
        {
            jint = NULL;
        }
        else
        {
            jint = json_object_new_int(calibration_binary->preemphasis_weight.val[i]);
        }
        json_object_object_add_ex(jarray_element, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_WEIGHT, jint,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

        if (!calibration_binary->preemphasis_delay.valid[i])
        {
            jint = NULL;
        }
        else
        {
            jint = json_object_new_int(calibration_binary->preemphasis_delay.val[i]);
        }
        json_object_object_add_ex(jarray_element, PMD_JSON_EYE_SHAPING_COEFFICIENTS_PREEMPHASIS_DELAY, jint,
            JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

        json_object_array_add(jarray, jarray_element);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_EYE_SHAPING_COEFFICIENTS, jarray,
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    /* arrays of int objects */
    if (!calibration_binary->temp_table.valid)
    {
        jarray = NULL;
    }
    else
    {
        create_json_array_from_resistance_temperature_table(&jarray, calibration_binary->temp_table.val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_RES2TEMP, jarray, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    if (!temp2apd_table->valid)
    {
        jarray = NULL;
    }
    else
    {
        create_json_array_from_apd_temperature_table(&jarray, temp2apd_table->val);
    }
    json_object_object_add_ex(*calibration_json_object, PMD_JSON_TEMP2APD, jarray, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
}


static void create_json_array_from_resistance_temperature_table(struct json_object **jarray,
    const uint32_t res2temp[TEMP_TABLE_SIZE])
{
    int i;
    struct json_object *jint;

    *jarray = json_object_new_array();
    for (i = 0; i < TEMP_TABLE_SIZE; i++)
    {
        jint = json_object_new_int(res2temp[i]);
        json_object_array_add(*jarray, jint);
    }
}


static void create_json_array_from_apd_temperature_table(struct json_object **jarray,
    const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE])
{
    int i;
    struct json_object *jint;

    *jarray = json_object_new_array();
    for (i = 0; i < APD_TEMP_TABLE_SIZE; i++)
    {
        jint = json_object_new_int(temp2apd_table[i]);
        json_object_array_add(*jarray, jint);
    }
}


static struct json_object* create_pmd_rssi_coefficients_json_object(struct json_object *a_jint,
    struct json_object *b_jint, struct json_object *c_jint)
{
    struct json_object *rssi_jobj;

    rssi_jobj = json_object_new_object();
    json_object_object_add_ex(rssi_jobj, PMD_JSON_RSSI_COEFFICIENTS_A, a_jint, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(rssi_jobj, PMD_JSON_RSSI_COEFFICIENTS_B, b_jint, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(rssi_jobj, PMD_JSON_RSSI_COEFFICIENTS_C, c_jint, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return rssi_jobj;
}


static struct json_object* create_pmd_mpd_channel_config_vga_tia_json_object(struct json_object *vga_jint,
    struct json_object *tia_jint)
{
    struct json_object *channel_jobj;

    channel_jobj = json_object_new_object();
    json_object_object_add_ex(channel_jobj, PMD_JSON_MPD_CHANNEL_CONFIG_VGA, vga_jint, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(channel_jobj, PMD_JSON_MPD_CHANNEL_CONFIG_TIA, tia_jint, JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return channel_jobj;
}


static struct json_object* create_pmd_apd_json_object(int32_t val)
{
    struct json_object *jobj;
    int type = (val & 400) >> 10;
    int voltage = val & 0x3ff;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_APD_TYPE, json_object_new_int(type), JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_APD_VOLTAGE, json_object_new_int(voltage), JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_mpd_gains_json_object(int32_t val)
{
    struct json_object *jobj;
    int bias_gain = val & 0xff; /* used as float after dividing by 256 */
    int mod_gain  = val >> 8;   /* used as float after dividing by 256 */

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_MPD_GAINS_BIAS_GAIN, json_object_new_int(bias_gain),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_MPD_GAINS_MOD_GAIN, json_object_new_int(mod_gain),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);    

    return jobj;
}


static struct json_object* create_pmd_los_thr_json_object(int32_t val)
{
    struct json_object *jobj;
    int assert = val & 0xff;
    int deassert = (val & 0xff00) >> 8;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_LOS_THR_ASSERT, json_object_new_int(assert), JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_LOS_THR_DEASSERT, json_object_new_int(deassert),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_sat_pos_json_object(int32_t val)
{
    struct json_object *jobj;
    int high = val & 0xff;
    int low = (val & 0xff00) >> 8;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_SAT_POS_HIGH, json_object_new_int(high), JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_SAT_POS_LOW, json_object_new_int(low), JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_sat_neg_json_object(int32_t val)
{
    struct json_object *jobj;
    int high = val & 0xff;
    int low = (val & 0xff00) >> 8;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_SAT_NEG_HIGH, json_object_new_int(high), JSON_C_OBJECT_KEY_IS_CONSTANT
        | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_SAT_NEG_LOW, json_object_new_int(low), JSON_C_OBJECT_KEY_IS_CONSTANT |
        JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_adf_los_thresholds_json_object(int32_t val)
{
    struct json_object *jobj;
    int assert = val & 0xffff;
    int deassert = (val & 0xffff0000) >> 16;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_ADF_LOS_THRESHOLDS_ASSERT, json_object_new_int(assert),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_ADF_LOS_THRESHOLDS_DEASSERT, json_object_new_int(deassert),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_adf_los_leaky_bucket_json_object(int32_t val)
{
    struct json_object *jobj;
    int assert = val & 0xff;
    int lb_bucket_sz = (val & 0xff00) >> 8;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_ADF_LOS_LEAKY_BUCKET_ASSERT, json_object_new_int(assert),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_ADF_LOS_LEAKY_BUCKET_LB_BUCKET_SZ, json_object_new_int(lb_bucket_sz),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_compensation_json_object(int32_t val)
{
    struct json_object *jobj;
    int compensation_enable0 = val & 1;
    int compensation_enable1 = (val >> 1) & 1;
    int die_temp_ref = (val >> 8) & 0xff;
    int compensation_coeff1_q8 = (val >> 16) & 0xff;
    int compensation_coeff2_q8 = (val >> 24) & 0xff;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_COMPENSATION_ENABLE0,
        json_object_new_boolean(compensation_enable0), JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_COMPENSATION_ENABLE1,
        json_object_new_boolean(compensation_enable1), JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_COMPENSATION_DIE_TEMP_REF, json_object_new_int(die_temp_ref),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_COMPENSATION_COEFF1_Q8,
        json_object_new_int(compensation_coeff1_q8), JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_COMPENSATION_COEFF2_Q8,
        json_object_new_int(compensation_coeff2_q8), JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


static struct json_object* create_pmd_edge_rate_json_object(int32_t val)
{
    struct json_object *jobj;
    int rate = val & 0xff;
    int dload = (val & 0xff00) >> 8;

    jobj = json_object_new_object();
    json_object_object_add_ex(jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_RATE, json_object_new_int(rate),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);
    json_object_object_add_ex(jobj, PMD_JSON_EYE_SHAPING_COEFFICIENTS_EDGE_RATE_DLOAD, json_object_new_int(dload),
        JSON_C_OBJECT_KEY_IS_CONSTANT | JSON_C_OBJECT_ADD_KEY_IS_NEW);

    return jobj;
}


void pmd_calibration_json_file_set_scripts_manifest(const char *calibration_manifest)
{
    if (json_file_manifest[0])
    {
        fprintf(stderr, "Warning: The PMD calibration scripts already started with the following manifest.\nConsider "
            "invoking 'laser calibration --pmdreset' and try again.\nManifest: [%s]\n", json_file_manifest);
    }

    if (!calibration_manifest)
    {
        fprintf(stderr, "Error: NULL PMD calibration scripts manifest.\n");
        return;
    }

    strncpy(json_file_manifest, calibration_manifest, PMD_BUF_MAX_SIZE - 1);
}


void pmd_calibration_json_file_stamp_scripts_manifest(const char *calibration_manifest)
{
    if (!json_file_manifest[0])
    {
        fprintf(stderr, "Warning: The PMD calibration scripts were not started properly.\n");
        return;
    }

    if (!calibration_manifest)
    {
        fprintf(stderr, "Error: NULL PMD calibration scripts manifest.\n");
        return;
    }

    if (!strncmp(json_file_manifest, calibration_manifest, PMD_BUF_MAX_SIZE))
    {
        /* TODO TBD stamp/seal the JSON file */
        printf("Stamping the PMD calibration JSON file.\n");
    }
    else
    {
        fprintf(stderr, "Error: PMD calibration scripts manifest mismatch. Can't stamp the JSON file.\n");
    }

    memset(json_file_manifest, 0x00, PMD_BUF_MAX_SIZE);
}


void pmd_calibration_json_file_reset_manifest(void)
{
    memset(json_file_manifest, 0x00, PMD_BUF_MAX_SIZE);
    printf("The PMD calibration scripts manifest was cleared successfully.\n");
}


void pmd_calibration_delete_files(void)
{
    pmd_calibration_remove_file(PMD_CALIBRATION_JSON_FILE_PATH);
    pmd_calibration_remove_file(PMD_CALIBRATION_FILE_PATH);
    pmd_calibration_remove_file(PMD_TEMP2APD_FILE_PATH);
    sleep(2); /* allow log messages to be printed before kernel prints interfere */
}


static void pmd_calibration_remove_file(const char* file_path)
{
    int ret;

    ret = remove(file_path);
    if (ret)
    {
        fprintf(stderr, "Error: cannot remove the %s PMD calibration file. %s\n", file_path, strerror(errno));
        return;
    }
    printf("The PMD calibration file %s was deleted successfully.\n", file_path);
}
