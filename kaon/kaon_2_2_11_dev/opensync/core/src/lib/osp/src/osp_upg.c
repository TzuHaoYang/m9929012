/*
Copyright (c) 2015, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "osp.h"
#include "log.h"

#define CMD_STR_RET 1024
#define CMD_STR_LEN 256
#define URL_STR_LEN 512



osp_upg_status_t last_error;
char file_path[CMD_STR_LEN];


static int cmd_execute(char *cmd, char *cmd_result, uint32_t cmd_result_size)
{
    FILE *f;
	int rc = 0;
    uint32_t buff_size = 0;
	LOG(DEBUG, "%s", __FUNCTION__);

	if (cmd == NULL)
	{
		LOG(ERR, "empty cmd");
		rc = -1;
		goto ret;
	}
    LOG(DEBUG, "excuting cmd: %s", cmd);
    f = popen(cmd, "r");
    if (f == NULL)
    {
        LOG(ERR, "Error running: %s", cmd);
		rc = -1;
        goto ret;
    }

    while (fgets(cmd_result + buff_size, (cmd_result_size - buff_size), f) != NULL)
    {
		buff_size =  strlen(cmd_result);
    }
    LOG(DEBUG, "Result : %s", cmd_result);

    pclose(f);
ret:
	return rc; 
}

/**
 * Validate the command result and return error.
 */
static osp_upg_status_t osp_upg_dl_error_check(char *result, uint32_t result_size)
{
	char *curl_code;
	int curl_err = 0;
	
	if(result == NULL)
	{
		curl_err = 0;
		goto val_check;
	}
	LOG(DEBUG, "(%s): cmd result: %s", __func__, result);
	curl_code = strstr(result, "fail");
	if (curl_code)
	{	
		curl_err = -1;
		goto val_check;
	}
	curl_code = strstr(result, "curl: (");

	if (curl_code)
	{
		curl_code = strtok(curl_code+sizeof("curl: "), ")");
		if (curl_code)
		{
			curl_err = atoi(curl_code);
			LOG(DEBUG, "(%s): curl_err: %d", __func__, curl_err);
		}
    }
val_check:
	switch(curl_err)
	{
		case 0:
			return OSP_UPG_OK;
		case 3:
			return OSP_UPG_URL;	
		case 22:
			return OSP_UPG_DL_FW;
		case 23:
			return OSP_UPG_DL_NOFREE;
		default:
			return OSP_UPG_DL_FW;
	}
	return OSP_UPG_OK;
}

/**
 * Validate the md5sum command result and return error.
 */
static osp_upg_status_t osp_upg_md5sum_error_check(char *result, uint32_t result_size)
{
	char	*strerr;
	
	if(result == NULL)
	{
		return OSP_UPG_INTERNAL;
	}
	LOG(DEBUG, "(%s): cmd result: %s", __func__, result);
/*
	strerr = strstr(result, "FAILED");
	if (strerr)
		return OSP_UPG_IMG_FAIL;
*/
	strerr = strstr(result, "WARNING");
	if (strerr)
		return OSP_UPG_MD5_FAIL;
	strerr = strstr(result, "OK");
	if (strerr)
		return OSP_UPG_OK;

	return OSP_UPG_OK;
}
#if 0
/**
 * Validate the bcm_flasher command result and return error.
 */
static osp_upg_status_t osp_upg_upgrade_error_check(char *result, uint32_t result_size)
{
	char	*strerr;
	
	if(result == NULL)
	{
		return OSP_UPG_INTERNAL;
	}
	LOG(DEBUG, "(%s): cmd result: %s", __func__, result);
	
	strerr = strstr(result, "ERROR!!! Could not open");
	if (strerr)
		return OSP_UPG_FL_CHECK;
	strerr = strstr(result, "ERROR[validateWfiTag");
	if (strerr)
		return OSP_UPG_IMG_FAIL;
	strerr = strstr(result, "ERROR!!! Did not successfully write image to NAND");
	if (strerr)
		return OSP_UPG_FL_WRITE;
	strerr = strstr(result, "ERROR!!!");
	if (strerr)
		return OSP_UPG_FL_WRITE;
	
	return OSP_UPG_OK;
}

#endif


/**
 * Validate the command result and return error.
 */
static osp_upg_status_t osp_upg_commit_error_check(char *result, uint32_t result_size)
{
	char	*strerr;
	
	if(result == NULL)
	{
		return OSP_UPG_INTERNAL;
	}
	LOG(DEBUG, "(%s): cmd result: %s", __func__, result);
	strerr = strstr(result, "ERROR!!! Could not find image_update metadata");
	if (strerr)
		return OSP_UPG_BC_SET;
	return OSP_UPG_OK;
}


/**
 * Check system requirements for upgrade, like no upgrade in progress,
 * available flash space etc.
 */
bool osp_upg_check_system(void)
{ 

	LOG(DEBUG, "osp_upg_check_system ");
    return true;
}

/**
 * Download an image suitable for upgrade from @p uri and store it locally.
 * Upon download and verification completion, invoke the @p dl_cb callback.
 */

bool osp_upg_dl(char *url, uint32_t timeout, osp_upg_cb dl_cb)
{ 
	char curl_cmd[URL_STR_LEN];
	char cmd_result[CMD_STR_RET];
	osp_upg_status_t upg_dl_err = OSP_UPG_OK;
	char *fn; 
	last_error = 0;
	int rc;

	LOG(DEBUG, "(%s): Start downloading from: %s ", __func__, url);
	LOG(DEBUG, "time out %d ", timeout);
	if (url == NULL)
	{
		last_error = OSP_UPG_URL;
		dl_cb(OSP_UPG_DL,  OSP_UPG_URL, 0);
		return false;
	}

	(fn = strrchr(url, '/')) ? ++fn : NULL;
	if (fn == NULL)
	{
		last_error = OSP_UPG_URL;
		dl_cb(OSP_UPG_DL,  OSP_UPG_URL, 0);
		return false;
	}
	if (timeout == 0)
	{
		LOG(DEBUG, "(%s): download time %d", __func__, timeout);
		timeout = 60;
		
	}
	/*down load FW*/
    snprintf(file_path, sizeof(file_path), "/var/tmp/%s", fn);
	LOG(DEBUG, "(%s): into path %s", __func__, file_path);
    snprintf(curl_cmd, sizeof(curl_cmd), 
		"curl -k -Ss --fail --max-time %d -o %s %s 2>&1",
		timeout, file_path, url);
	rc = cmd_execute(curl_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		goto int_err;
	}
	upg_dl_err = osp_upg_dl_error_check(cmd_result, sizeof(cmd_result));
    if (upg_dl_err)
	{
		last_error = upg_dl_err;
		dl_cb(OSP_UPG_DL,  upg_dl_err, 0);
		return false;
	}

	/*down load md5*/
	snprintf(file_path, sizeof(file_path), "/var/tmp/%s", fn);
	LOG(DEBUG, "(%s): file path %s.md5", __func__, file_path);
    snprintf(curl_cmd, sizeof(curl_cmd), "curl -k -Ss --fail -o %s.md5 %s.md5 2>&1",
		file_path,url);
	rc = cmd_execute(curl_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		goto int_err;
	}
	upg_dl_err = osp_upg_dl_error_check(cmd_result, sizeof(cmd_result));
    if (upg_dl_err)
	{
		upg_dl_err = upg_dl_err==OSP_UPG_DL_FW?OSP_UPG_DL_MD5:upg_dl_err;
		last_error = upg_dl_err;
		dl_cb(OSP_UPG_DL,  upg_dl_err, 0);
		return false;
	}
	

	/* check md5 sum for the image*/
	snprintf(curl_cmd, sizeof(curl_cmd), "cd /var/tmp; md5sum -c %s.md5 2>&1", file_path);
	rc = cmd_execute(curl_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		goto int_err;
	}
	upg_dl_err = osp_upg_md5sum_error_check(cmd_result, sizeof(cmd_result));
	if (upg_dl_err)
	{
		LOG(DEBUG, "(%s): md5sum checksum failed", __func__);
		last_error = upg_dl_err;
		dl_cb(OSP_UPG_DL,  upg_dl_err, 0);
		return false;
	}
	
	LOG(DEBUG, "(%s): download complete ", __func__);
	last_error = OSP_UPG_OK;
	dl_cb(OSP_UPG_DL,  OSP_UPG_OK, 100);
    return true;
	
int_err:
	last_error = OSP_UPG_INTERNAL;
	dl_cb(OSP_UPG_DL,  OSP_UPG_INTERNAL, 0);
	return false;
} 

/**
 * Write the previously downloaded image to the system. If the image
 * is encrypted, a password must be specified in @p password.
 *
 * After the image was successfully applied, the @p upg_cb callback is invoked.
 */

bool osp_upg_upgrade(char *password, osp_upg_cb upg_cb)
{ 
	char sys_cmd[CMD_STR_LEN];
	//char cmd_result[CMD_STR_RET];
    osp_upg_status_t status = OSP_UPG_OK;
	last_error = 0;
	last_error = status;
	//int rc;
	
	if (file_path == NULL)
	{
		LOG(DEBUG, "(%s): file name error ", __func__ );
		status = OSP_UPG_INTERNAL;
		goto int_err;
	}
	LOG(DEBUG, "(%s): Start upgrading FW  %s", __func__,file_path );

/*
	snprintf(sys_cmd, sizeof(sys_cmd), "cd /var/tmp; md5sum -c %s.md5 2>&1", file_path);
	rc = cmd_execute(sys_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		status = OSP_UPG_INTERNAL;
		goto int_err;
	}
	status = osp_upg_md5sum_error_check(cmd_result, sizeof(cmd_result));
	if (status)
	{
		LOG(DEBUG, "(%s): md5sum checksum failed", __func__);
		goto int_err;
	}
*/
	if ((password != NULL) && (strlen(password) > 0)) 
	{
		snprintf(sys_cmd, sizeof(sys_cmd), "/bin/writerip -d  %s", file_path);
		if (system(sys_cmd))
		{
			upg_cb(OSP_UPG_UPG,  OSP_UPG_FL_WRITE, 0);
			last_error = OSP_UPG_FL_WRITE;
			return false;
		}
	}
	snprintf(sys_cmd, sizeof(sys_cmd), "bcm_flasher  %s", "/var/tmp/dec_fw");
	if (system(sys_cmd))
	{
		last_error = OSP_UPG_IMG_FAIL;
		upg_cb(OSP_UPG_UPG,  OSP_UPG_IMG_FAIL, 0);
		return false;
	}

#if 0
	rc = cmd_execute(sys_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		status = OSP_UPG_INTERNAL;
		goto int_err;
	}
	status = osp_upg_upgrade_error_check(cmd_result, sizeof(cmd_result));

    if (status)
	{
		LOG(DEBUG, "(%s): bcm_flasher cmd failed", __func__);
		goto int_err;
	}
#endif
	last_error = status;
	upg_cb(OSP_UPG_UPG,  status, 100);
    return true;
	
int_err:
	last_error = status;
	upg_cb(OSP_UPG_UPG,  status, 0);
	return false;
}


/**
 * On dual-boot system, flag the newly flashed image as the active one.
 * This can be a no-op on single image systems.
 */

bool osp_upg_commit(void)
{ 
	char sys_cmd[CMD_STR_LEN];
	char cmd_result[CMD_STR_RET];
	osp_upg_status_t status = OSP_UPG_OK;
	int rc;
	
	LOG(DEBUG, "(%s): Commiting FW ", __func__ );
    snprintf(sys_cmd, sizeof(sys_cmd), "bcm_bootstate 1 2>&1");
	rc = cmd_execute(sys_cmd, cmd_result, sizeof(cmd_result));
	if(rc)
	{
		LOG(DEBUG, "(%s): command execution error", __func__);
		status = OSP_UPG_INTERNAL;
		last_error = status;
		return false;
	}
	status = osp_upg_commit_error_check(cmd_result, sizeof(cmd_result));
    if (status)
	{
		LOG(DEBUG, "(%s): bcm_flasher cmd failed", __func__);
		last_error = status;
		return false;
	}
	
	last_error = OSP_UPG_OK;
    return true;
}

/**
 * Return a more detailed error code related to a failed osp_upg_*() function
 * call. See osp_upg_status_t for a detailed list of error codes.
 */
int osp_upg_errno(void)
{ 
    return last_error;
}

