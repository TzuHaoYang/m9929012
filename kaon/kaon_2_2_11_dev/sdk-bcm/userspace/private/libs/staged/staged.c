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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "debug.h"
#include "staged.h"
#include "nvc.h"

static int acquireFileLock(int fd)
{
    int ret, result;

trylock:
    ret = lockf(fd, F_LOCK, 0);
    if (ret == -1)
    {
        if(errno == EINTR)
        {
            log_debug("lockf() failed to lock due to EINTR, so redo lockf()!");
            goto trylock;
        }
        else
            log_error("lockf() failed to lock! error=%s", strerror(errno));

        result = -1;
    }
    else
        result = 0;

    return result;
}

static int releaseFileLock(int fd)
{
    int ret;

    ret = lockf(fd, F_ULOCK, 0);
    if (ret == -1)
    {
        log_error("lockf() failed to release! error=%s",
                                        strerror(errno));
        return -1;
    }
    return 0;
}

StagedInfo* staged_init(const char* staged_file)
{
    int fd;
    int ret;
    StagedInfo *si = NULL;
    struct stat st;

    if((staged_file == NULL) || (strlen(staged_file) == 0))
    {
        log_error("Invalid parameter");
        return NULL;
    }

    si = (StagedInfo *) malloc(sizeof(StagedInfo));
    if(si == NULL)
    {
        log_error("Failed to allocate StagedInfo");
        return NULL;
    }
    memset(si, 0, sizeof(StagedInfo));

    fd = open(staged_file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        log_error("Failed to open temporary staged file %s", staged_file);
        staged_free(si);
        return NULL;
    }
    si->fd = fd;

    if (fstat(fd, &st) == -1)
    {
        log_error("Failed to get info about staged file, error=%s", strerror(errno));
        close(fd);
        staged_free(si);
        return NULL;
    }

    if (st.st_size != MAX_STAGED_FILE_LEN)
    {
        if (acquireFileLock(si->fd) == -1)
        {
            close(fd);
            staged_free(si);
            return NULL;
        }
        ret = posix_fallocate(fd, 0, MAX_STAGED_FILE_LEN);
        if (ret != 0)
        {
            log_error("Failed to fallocate staged file! error=%s", strerror(errno));
            releaseFileLock(si->fd);
            close(fd);
            staged_free(si);
            return NULL;
        }
        releaseFileLock(si->fd);
    }

    si->mapped_region = mmap(NULL, MAX_STAGED_FILE_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (si->mapped_region == (void *) -1)
    {
        log_error("Failed to mmap to staged file! Error=%s", strerror(errno));
        si->mapped_region = NULL;
        close(fd);
        staged_free(si);
        return NULL;
    }

    return si;
}

StagedRet staged_free(StagedInfo *si)
{
    StagedRet result = STAGED_OK;
    int ret;

    if(si == NULL)
        return result;

    if (si->mapped_region != NULL)
    {
        ret = munmap(si->mapped_region, MAX_STAGED_FILE_LEN);
        if (ret != 0)
        {
            result = STAGED_GENERIC_ERROR;
            log_error("Failed to munmap staged file! Error=%s", strerror(errno));
        }
        else
        {
            si->mapped_region = NULL;
        }
    }
    close(si->fd);
    free(si);
    return result;
}

StagedRet staged_set(StagedInfo *si, const char *nvname, const char *value)
{
    StagedRet result = STAGED_OK;
    char *staged_str, *buf;

    log_debug("nvname=%s, value=%s", nvname, value);

    if(acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *) si->mapped_region;

        log_debug("processing mapped string");
        buf = nvc_list_update(staged_str, nvname, value);
        if (buf != NULL)
        {
            log_debug("updating mapped string");
            memset(staged_str, 0x00, MAX_STAGED_FILE_LEN);
            strcpy(staged_str, buf);
            free(buf);
            msync(si->mapped_region, MAX_STAGED_FILE_LEN, MS_SYNC);
            log_debug("done updating mapped string");
        }
        else
        {
            result = STAGED_GENERIC_ERROR;
        }
        releaseFileLock(si->fd);
    }
    return result;
}

StagedRet staged_delete(StagedInfo *si, const char *nvname)
{
    StagedRet result = STAGED_OK;
    char *staged_str, *buf;

    log_debug("nvname=%s", nvname);

    if(acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *) si->mapped_region;
        buf = nvc_list_delete(staged_str, nvname);
        if (buf != NULL)
        {
            memset(staged_str, 0x00, MAX_STAGED_FILE_LEN);
            strcpy(staged_str, buf);
            free(buf);
            msync(si->mapped_region, MAX_STAGED_FILE_LEN, MS_SYNC);
        }
        else
        {
            result = STAGED_GENERIC_ERROR;
        }
        releaseFileLock(si->fd);
    }
    return result;
}

/*
 * returns STAGED_OK if we found the value for the given nvname in the mapped list.
 * returns STAGED_NOT_FOUND if if nvname isn't found in mapped list.
 * returns STAGED_NULL_ENTRY if the nvname is found in mapped list, but with a null value
 * returns STAGED_GENERIC_ERROR if there is any error.
 */
StagedRet staged_get(StagedInfo *si, const char *nvname, char *value, size_t size)
{
    StagedRet result;
    char *staged_str, *buf;
    UBOOL8 exist;

    log_debug("nvname=%s", nvname);

    if (acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *)(si->mapped_region);

        buf = nvc_list_get(staged_str, nvname, &exist);
        if (buf != NULL)
        {
            result = STAGED_OK;
            if (strlen(buf) < size)
            {
                strncpy(value, buf, size);
            }
            free(buf);
        }
        else if (exist == FALSE)
        {
            result = STAGED_NOT_FOUND;
        }
        else
        {
            result = STAGED_NULL_ENTRY;
        }
        releaseFileLock(si->fd);
    }
    return result;
}

StagedRet staged_commit(StagedInfo *si,
                        nvc_for_each_func foreach_func,
                        staged_commit_func commit_func,
                        void *data)
{
    StagedRet result = STAGED_OK;
    int ret;
    char *staged_str;

    if (acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *) si->mapped_region;
        ret = nvc_list_for_each(staged_str, foreach_func, data);
        if (ret == 0)
        {
            if (commit_func != NULL)
            {
                ret = commit_func(data);
            }
        }

        if (ret == 0)
        {
            memset(staged_str, 0x00, MAX_STAGED_FILE_LEN);
            strcpy(staged_str, "{}");
            msync(si->mapped_region, MAX_STAGED_FILE_LEN, MS_SYNC);
        }
        else
        {
            result = STAGED_GENERIC_ERROR;
        }
        releaseFileLock(si->fd);
    }
    return result;
}

StagedRet staged_dump(StagedInfo *si, nvc_for_each_func dump_func, void *data)
{
    StagedRet result = STAGED_OK;
    char *staged_str;

    if (acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *)(si->mapped_region);
        if (0 != nvc_list_for_each(staged_str, dump_func, data))
        {
            result = STAGED_GENERIC_ERROR;
        }
        releaseFileLock(si->fd);
    }
    return result;
}

UBOOL8 staged_name_exist(StagedInfo *si, const char *nvname)
{
    UBOOL8 result = FALSE;
    char *staged_str;

    if (acquireFileLock(si->fd) == -1)
    {
        result = FALSE;
    }
    else
    {
        staged_str = (char *) si->mapped_region;
        result = nvc_list_exist(staged_str, nvname);
        releaseFileLock(si->fd);
    }
    return result;
}

StagedRet staged_raw_execute(StagedInfo *si, staged_raw_func raw_func, void *data)
{
    StagedRet result = STAGED_OK;
    char *staged_str;

    if (acquireFileLock(si->fd) == -1)
    {
        result = STAGED_GENERIC_ERROR;
    }
    else
    {
        staged_str = (char *) si->mapped_region;
        result = raw_func(staged_str, data);
        releaseFileLock(si->fd);
    }
    return result;
}

