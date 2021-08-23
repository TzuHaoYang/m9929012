/***********************************************************************
 *
 *  Copyright (c) 2006-2018  Broadcom Corporation
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
#include <stdint.h>
#include <string.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>
#include <signal.h>
#include "otp_ioctl.h"

#define OTP_DEVICE_NAME  "/dev/otp"

int cmdBtrmMrktId(int isSet, int isMid, int id)
{
    int otpFd;
    int rc;

    OTP_IOCTL_PARMS ioctlParms;
    ioctlParms.value = -1;

    otpFd = open(OTP_DEVICE_NAME, O_RDWR);
    if ( otpFd != -1 )
    {
        if (isSet)
        {
            if (isMid)
            {
                /* Fusing a manufacturing market id */
                ioctlParms.element = OTP_MID_BITS;
                ioctlParms.value     = id;
            }
            else
            {
                /* Fusing an operator market id */
                ioctlParms.element = OTP_OID_BITS;
                ioctlParms.value     = id;
            }
            rc = ioctl(otpFd, OTP_IOCTL_SET, &ioctlParms);
        }
        else
        {
            if (isMid)
            {
                /* Retrieving the manufacturing market id */
                ioctlParms.element = OTP_MID_BITS;
            }
            else
            {
                /* Retrieving the operator market id */
                ioctlParms.element = OTP_OID_BITS;
            }
            rc = ioctl(otpFd, OTP_IOCTL_GET, &ioctlParms);
        }

        if (rc < 0)
            printf("%s@%d otp failure\n", __FUNCTION__, __LINE__);
    }
    else
       printf("Unable to open device %s", OTP_DEVICE_NAME);

    close(otpFd);

    return ioctlParms.value;
}


int cmdBtrmSingleBit(int isSet, int isBtrm)
{
    int otpFd;
    int rc;

    OTP_IOCTL_PARMS ioctlParms;
    ioctlParms.value = -1;

    otpFd = open(OTP_DEVICE_NAME, O_RDWR);
    if ( otpFd != -1 )
    {
        if (isBtrm)
            ioctlParms.element = OTP_BTRM_ENABLE_BIT;
        else
            ioctlParms.element = OTP_OPERATOR_ENABLE_BIT;

        if (isSet)
            rc = ioctl(otpFd, OTP_IOCTL_SET, &ioctlParms);
        else
            rc = ioctl(otpFd, OTP_IOCTL_GET, &ioctlParms);

        if (rc < 0)
            printf("%s@%d otp failure\n", __FUNCTION__, __LINE__);
    }
    else
       printf("Unable to open device %s", OTP_DEVICE_NAME);

    close(otpFd);

    return ioctlParms.value;
}
void cmdBtrmCfgHelp(void)
{
    fprintf(stdout,
     "\nUsage: btrmcfg set btrm_enable\n"
     "       btrmcfg set op_enable\n"
     "       btrmcfg set mid <16bit market identifier>\n"
     "       btrmcfg set oid <16bit market identifier>\n"
     "       btrmcfg get btrm_enable\n"
     "       btrmcfg get op_enable\n"
     "       btrmcfg get mid\n"
     "       btrmcfg get oid\n"
     "       btrmcfg --help\n");
}

int main(int argc, char *argv[])
{
    int result = -1;

    /* parse the command line and build the argument vector */
    if (argv[1] == NULL)
    {
        cmdBtrmCfgHelp();
    }
    else if ((strcasecmp(argv[1], "get") == 0) && (argc > 2))
    {
        if (strcasecmp(argv[2], "btrm_enable") == 0)
        {
            result = cmdBtrmSingleBit(0,1);
            fprintf(stdout,"\nThe customer bootrom enable otp bit is fused to %d\n", result);
        }
	else if (strcasecmp(argv[2], "op_enable") == 0)
        {
            result = cmdBtrmSingleBit(0,0);
            fprintf(stdout,"\nThe operator enable otp bit is fused to %d\n", result);
        }
        else if (strcasecmp(argv[2], "mid") == 0)
        {
            result = cmdBtrmMrktId(0, 1, 0);
            fprintf(stdout,"\nThe manufacturing market id bits are fused to 0x%x\n", result);
        }
        else if (strcasecmp(argv[2], "oid") == 0)
        {
            result = cmdBtrmMrktId(0, 0, 0);
            fprintf(stdout,"\nThe operator market id bits are fused to 0x%x\n", result);
        }
	else
            cmdBtrmCfgHelp();
    }
    else if ((strcasecmp(argv[1], "set") == 0) && (argc > 2))
    {
        if (strcasecmp(argv[2], "btrm_enable") == 0)
            cmdBtrmSingleBit(1,1);
	else if (strcasecmp(argv[2], "op_enable") == 0)
            cmdBtrmSingleBit(1,0);
	else if ((strcasecmp(argv[2], "mid") == 0) || (strcasecmp(argv[2], "oid") == 0))
        {
            if (argc > 3)
            {
		sscanf(argv[3], "0x%x", &result);
                if (strcasecmp(argv[2], "mid") == 0)
                    cmdBtrmMrktId(1, 1, result);
                else
                    cmdBtrmMrktId(1, 0, result);
            }
	    else
                cmdBtrmCfgHelp();
        }
	else
            cmdBtrmCfgHelp();
    }
    else
      cmdBtrmCfgHelp();

    return result;
}

