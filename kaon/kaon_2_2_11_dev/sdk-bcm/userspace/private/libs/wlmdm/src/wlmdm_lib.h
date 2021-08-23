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

#ifndef __WLMDM_LIB_H__
#define __WLMDM_LIB_H__
#include "cms_eid.h"

typedef enum wlmdm_ret
{
    WLMDM_OK,
    WLMDM_NULL_ENTRY,
    WLMDM_NOT_FOUND,
    WLMDM_INVALID_PARAM,
    WLMDM_GENERIC_ERROR
} WlmdmRet;

/* Initialize wlmdm context. This includes initializing StagedInfo which wlmdm will need.
 * Returns WLMDM_OK on success, otherwise returns related error code.
 * Any other WLMDM API should be called after wlmdm_init() returns success.
 */
WlmdmRet wlmdm_init();

/* Free wlmdm context. This should be called after user has finished its current wlmdm operations.*/
void wlmdm_destroy();

/* Update the value of NVRAM configurations corresponding to nvname.
 * Returns WLMDM_OK on success, otherwise returns error code.
 */
WlmdmRet wlmdm_nvram_set(const char *nvname, const char *value);

/* Unset the NVRAM configurations corresponding to nvname.
 * Returns WLMDM_OK on success, otherwise returns error code.
 */
WlmdmRet wlmdm_nvram_unset(const char *nvname);

/* Commit all NVRAM configurations into MDM data model.
 * Returns WLMDM_OK if the operation succeeds, otherwise returns the error code.
 */
WlmdmRet wlmdm_nvram_commit();

/* Retrieves the NVRAM value for the given nvname. The size of the returned value is specified in 
 * parameters, and wlmdm_nvram_get won't exceed the size.
 * Returns WLMDM_OK on success.
 * Returns WLMDM_NOT_FOUND if the function cannot find the corresponding NVRAM configuration.
 * Returns other error code on errors.
 */
WlmdmRet wlmdm_nvram_get(const char *nvname, char *value, size_t size);

/* Dumps out all NVRAM configurations from MDM and staged data.*/
void wlmdm_nvram_dump();

/* Similiar to wlmdm_nvram_dump, this function dumps out all NVRAM configurations from MDM and staged data,
 * but will store them into the specified buffer.
 */
void wlmdm_nvram_getall(char *buf, size_t size);

/* A helper function to return the string format of wlmdm error code. */
const char *wlmdm_error_str(WlmdmRet ret);

#endif
