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
#define EXIT_SUCCESS 0  
#define EXIT_FAILURE 1  

int cmdOtpOps(OTP_IOCTL_ENUM element, int isSet, int addr, uint64_t * data)
{
    int otpFd;
    int rc = 0;

    OTP_IOCTL_PARMS ioctlParms;
    ioctlParms.result = -1;

    otpFd = open(OTP_DEVICE_NAME, O_RDWR);

    if ( otpFd != -1 )
    {
        ioctlParms.element = element;
        ioctlParms.result = 0;
        ioctlParms.id = addr;
        ioctlParms.value = *data;
        rc = ioctl(otpFd, ((isSet)?OTP_IOCTL_SET:OTP_IOCTL_GET), &ioctlParms);

        if (rc < 0)
        {
            printf("%s: otp ioctl failure <%s element:%d addr:%d res:%d> rc:%d\n", __FUNCTION__,
                                                                        (isSet?"SET":"GET"),
                                                                        element,
                                                                        addr,
                                                                        ioctlParms.result,
                                                                        rc);
        }
        else
            *data = ioctlParms.value;
    }
    else
       printf("Unable to open device %s", OTP_DEVICE_NAME);

    close(otpFd);

    return ioctlParms.result;
}


void cmdOtpCtlHelp(void)
{
    fprintf(stdout,
     "\nUsage: \n"
       "      ------------------------------------------\n"
       "       otpctl set row <row addr> <32-bit word>\n"
       "       otpctl get row <row addr>\n"
       "      ------------------------------------------\n"
       "       otpctl set chip_ser_num <32-bit serial number>\n" 
       "       otpctl set jtag_pwd <64-bit JTAG lock password>\n" 
       "       otpctl set jtag_pwd_rdlock\n" 
       "       otpctl set jtag_lock\n" 
       "       otpctl set jtag_perma_lock\n" 
       "       otpctl set otp_cust_lock\n" 
       "       otpctl get chip_ser_num \n" 
       "       otpctl get jtag_pwd\n" 
       "       otpctl get jtag_pwd_rdlock\n" 
       "       otpctl get jtagLockStatus\n" 
       "       otpctl get otp_cust_lock\n" 
       "       otpctl --help\n"
       "Note:\n"
       "       - Refer to Broadcom Secure Boot and OTP documentation for row numbers\n"
       "       - chip_ser_num       :Specify a unique chip serial number. This number is only readable via OTP\n"
       "       - jtag_pwd           :Specify a JTAG unlock password. To be used when JTAG interface is locked via OTP\n"
       "       - jtag_pwd_rdlock    :readlocks the JTAG password, JTAG password can then not be read via OTP\n"
       "       - jtag_lock          :password locks the JTAG interface.\n"
       "                             JTAG can then only be unlocked via debugger by using the configured JTAG password\n"
       "       - jtag_perma_lock    :permanently locks the JTAG interface. JTAG can then never be unlocked \n"
       "       - otp_cust_lock      :permanently read/write locks the JTAG config OTP region\n");
}

int main(int argc, char *argv[])
{
    int result = -1;
    int skip_print = 0;
    uint64_t data = 0;

    /* parse the command line and build the argument vector */
    if (argv[1] == NULL)
    {
        cmdOtpCtlHelp();
    }
    else if ((strcasecmp(argv[1], "get") == 0) && (argc > 2))
    {
        if (argc > 3 )
        {
            if (strcasecmp(argv[2], "row") == 0)  
            {
                result = cmdOtpOps(OTP_ROW, 0, atoi(argv[3]), &data);
            }
            else
                cmdOtpCtlHelp();
        }
        else if ( argc > 2 )
        {
            if (strcasecmp(argv[2], "chip_ser_num") == 0)  
            {
                result = cmdOtpOps(OTP_CHIP_SERIAL_NUM, 0, 0, &data);
            }
            else if (strcasecmp(argv[2], "jtag_pwd") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PWD, 0, 0, &data);
            }
            else if (strcasecmp(argv[2], "jtag_pwd_rdlock") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PWD_RDLOCK, 0, 0, &data);
                printf("OTP JTAG PWD READLOCK Status: %s\n", (data)?"read LOCKED":"readable");
		skip_print = 1;
            }
            else if (strcasecmp(argv[2], "jtagLockStatus") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PERMLOCK, 0, 0, &data);
                if( result == 0 )
                {
                    /* If not permalocked, then check for password lock */
                    if( !data )
                    {
                        result = cmdOtpOps(OTP_JTAG_PWD_LOCK, 0, 0, &data);
                        if( result == 0 )
                        {
                            if (data )
                                printf("OTP JTAG LOCK Status: Password Locked\n");
                            else
                                printf("OTP JTAG LOCK Status: Unlocked\n");
                        }
                    }
                    else
                        printf("OTP JTAG LOCK Status: Permanently locked\n");

		    skip_print = 1;
                }
            }
            else if (strcasecmp(argv[2], "otp_cust_lock") == 0)  
            {
                result = cmdOtpOps(OTP_CUST_LOCK, 0, 0, &data);
                printf("JTAG config OTP region status: %s\n", (data)?"LOCKED":"NOT LOCKED");
		skip_print = 1;
            }
            else
                cmdOtpCtlHelp();
        }
        else
            cmdOtpCtlHelp();

        if( result == 0 && !skip_print )
        {
            if( data > 0xFFFFFFFF )
                printf("OTP Data: 0x%016llx\n", data);
            else
                printf("OTP Data: 0x%08x\n", (uint32_t)data);

        }
    }
    else if ((strcasecmp(argv[1], "set") == 0))
    {
        if(argc > 4)
        {
            data = strtoull(argv[4], NULL, 16);

            if (strcasecmp(argv[2], "row") == 0)
            {
                result = cmdOtpOps(OTP_ROW, 1, atoi(argv[3]), &data);
            }
            else
                cmdOtpCtlHelp();
        }
        else if ( argc > 3 )
        {
            data = strtoull(argv[3], NULL, 16);
            
            if (strcasecmp(argv[2], "chip_ser_num") == 0)  
            {
                result = cmdOtpOps(OTP_CHIP_SERIAL_NUM, 1, 0, &data);
            }
            else if (strcasecmp(argv[2], "jtag_pwd") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PWD, 1, 0, &data);
            }
            else
                cmdOtpCtlHelp();
        }
        else if ( argc > 2 )
        {
            if (strcasecmp(argv[2], "jtag_pwd_rdlock") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PWD_RDLOCK, 1, 0, &data);
            }
            else if (strcasecmp(argv[2], "jtag_lock") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PWD_LOCK, 1, 0, &data);
            }
            else if (strcasecmp(argv[2], "jtag_perma_lock") == 0)  
            {
                result = cmdOtpOps(OTP_JTAG_PERMLOCK, 1, 0, &data);
            }
            else if (strcasecmp(argv[2], "otp_cust_lock") == 0)  
            {
                result = cmdOtpOps(OTP_CUST_LOCK, 1, 0, &data);
            }
            else
                cmdOtpCtlHelp();
        }
        else
            cmdOtpCtlHelp();

    }
    else
      cmdOtpCtlHelp();

    printf("\nOTP Result: %d\n", result);

    return result;
}

