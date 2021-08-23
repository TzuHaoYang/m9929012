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
#ifndef __CMS_HELPER_H__
#define __CMS_HELPER_H__

#include "wlmdm.h"

/************************************************************************
 * Common cms_helper APIs
 ************************************************************************/

/* Try to aquire CMS lock if the lock hasn't been acquired by the current process.
 * By using this function, libwlmdm.so assumes that there is only one thread in each process accessing
 * CMS. So libwlmdm.so is not thread-safe. See the desgin document for wlmdm for more details.
 * Returns WLMDM_OK if the lock has been acquired, either by this function or it has been acquired by
 * the calling process.
 * Otherwise returns error code.
 */
WlmdmRet cms_try_lock(UBOOL8 *locked_here);

/* Release the CMS lock if the lock has been aquired locally, as indicated by locked_here parameter. */
WlmdmRet cms_try_unlock(UBOOL8 *locked_here);

/* Get the value of a parameter from MDM.
 * This function will try to acquire CMS lock by itself.
 */
WlmdmRet get_param_from_pathDesc(const MdmPathDescriptor *pathDesc, char *value, size_t size);

/* Set the value of a parameter into MDM.
 * This function will try to acquire CMS lock by itself.
 */
WlmdmRet set_param_from_pathDesc(const MdmPathDescriptor *pathDesc, const char *value);

/* An utility to compare if two MdmPathDescriptor is the same.
 * Returns TRUE if p is same as q, otherwise returns FALSE.
 */
UBOOL8 compare_pathDesc(const MdmPathDescriptor *p, const MdmPathDescriptor *q);

/******************************************************************************************
 * DM dependent helper APIs. Those should be implemented separately in both tr181_helper.c
 * and tr98_helper.c, each working with specified data model.
 *****************************************************************************************/

/*
 * Get the number of instances for a given oid. The oid here is DM dependent, so does the
 * implementation of the function.
 * Returns the number of instances if the operation is successful.
 * Otherwise return -1;
 */
int get_num_instances(MdmObjectId oid);

/* Get the number of bssid instances for a given radio index.
 * Returns the number of instances if the operation is successful.
 * Otherwise return -1;
 */
int get_bssid_num_for_radio(unsigned int radio_index);

/* Get radio index and bssid index for a given iidStack.
 * Returns WLMDM_OK on sucess, radio_index and bssid_index will be set.
 * Otherwise returns an error code, but the radio_index and bssid_index may be set to -1.
 */
WlmdmRet get_radio_bssid_index(MdmObjectId oid, const InstanceIdStack *iidStack,
                               int *radio_index, int *bssid_index);

/*
 * Get the iidStack for a given radio index and bssid index.
 * The function will verify if the radio index and bssid index is a valid one according to
 * MDM data.
 * Returns WLMDM_OK on sucess, and iidStack is set.
 * Otherwise returns an error code.
 */
WlmdmRet get_iidStack(MdmObjectId oid, int radio_index, int bssid_index, InstanceIdStack *iidStack);

/*
 * Get the pathDesc for unmapped NVRAM storage node in the MDM.
 */
void get_wlnvram_pathDesc(MdmPathDescriptor *pathDesc);

/*
 * Get the pathDesc for unset NVRAM storage node in the MDM.
 */
void get_wlUnsetNvram_pathDesc(MdmPathDescriptor *pathDesc);

#endif
