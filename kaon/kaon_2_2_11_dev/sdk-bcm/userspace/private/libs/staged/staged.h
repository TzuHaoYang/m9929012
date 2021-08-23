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
#ifndef __STAGED_H__
#define __STAGED_H__

#include <semaphore.h>
#include "nvc.h"
#include "os_defs.h"

typedef struct
{
    void    *mapped_region; //mmaped region of the file pointed by fd.
    int     fd;             //the file descriptor of data.
} StagedInfo;

typedef enum staged_ret
{
    STAGED_OK,
    STAGED_NULL_ENTRY,
    STAGED_NOT_FOUND,
    STAGED_INVALID_PARAM,
    STAGED_GENERIC_ERROR
}StagedRet;

/* Function pointer prototype for function passed to staged_raw_execute(). */
typedef int (*staged_raw_func)(char *staged_str, void *data);

/* Function pointer prototype for function passed to staged_commit(). */
typedef int (*staged_commit_func)(void *data);

/* Allocate and initialize StagedInfo data structure.
 * Parameters: staged_file: the name of the file to be initialized.
 * On success, returns pointer to StagedInfo data. Otherwise returns NULL.
 */
StagedInfo* staged_init(const char* staged_file);

/* Free all resources associated with incomming StagedInfo, including StagedInfo itself.
 */
StagedRet staged_free(StagedInfo *si);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * retrieves the value of the corresponding nvname.
 * returns STAGED_OK if we found the value for the given nvname in the mapped list.
 * returns STAGED_NOT_FOUND if if nvname isn't found in mapped list.
 * returns STAGED_NULL_ENTRY if the nvname is found in mapped list, but with a null value
 * returns STAGED_GENERIC_ERROR if there is any error.
 */
StagedRet staged_get(StagedInfo *si, const char *nvname, char *value, size_t size);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * updates the value of the corresponding nvname.
 */
StagedRet staged_set(StagedInfo *si, const char *nvname, const char *value);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * calls foreach_func for each of the JSON name:value pair, and then calls commit_func.
 * On success, the data will be emptied in JSON format, and the function returns STAGED_OK.
 * On error, the data won't be emptied, and the function returns STAGED_GENERIC_ERROR.
 */
StagedRet staged_commit(StagedInfo *si,
                        nvc_for_each_func foreach_func,
                        staged_commit_func commit_func,
                        void *data);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * calls dump_func for each of the JSON name:value pair.
 */
StagedRet staged_dump(StagedInfo *si, nvc_for_each_func dump_func, void *data);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * deletes the corresponding nvname pair from its managed data.
 */
StagedRet staged_delete(StagedInfo *si, const char *nvname);

/* Assuming the data being managed in mapped_region is in JSON format, this function
 * checks if nvname exists in its managed data.
 * Returns TRUE if found, FALSE if not found.
 */
UBOOL8 staged_name_exist(StagedInfo *si, const char *nvname);

/* this function have NO assumption about the format of managed data.
 * it just calles raw_func with a pointer to the manageged data, and raw_func should handle
 * whatever format it is expecting.
 */
StagedRet staged_raw_execute(StagedInfo *si, staged_raw_func raw_func, void *data);

/* The maximum data size StagedInfo will manage. */
enum { MAX_STAGED_FILE_LEN = 1024 * 128 };

#endif
