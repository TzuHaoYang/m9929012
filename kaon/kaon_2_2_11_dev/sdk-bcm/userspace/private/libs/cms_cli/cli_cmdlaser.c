/***********************************************************************
<:copyright-BRCM:2012:proprietary:standard 

   Copyright (c) 2012 Broadcom 
   All Rights Reserved

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

/** command driven CLI code goes into this file */

#ifdef SUPPORT_CLI_CMD
#ifndef DESKTOP_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define BP_OPTICAL_PARAMS_LEN 48

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"
#include "cms_boardcmds.h"
#include "cli.h"
#include "laser.h"
#include "pmd.h"

#define base(x) ((x >= '0' && x <= '9') ? '0' : \
    (x >= 'a' && x <= 'f') ? 'a' - 10 : \
    (x >= 'A' && x <= 'F') ? 'A' - 10 : \
    '\255')

#define TOHEX(x) (x - base(x))


#define MAX_OPTS 8

/* Kernel Driver Interface Function Prototypes */
static int devLaser_Reset_PMD_into_Calibration_Mode(void);
static int devLaser_Read_Optical_Params_From_Optics(
    unsigned char *pOpticalParams, short len);
static int devLaser_Write_Optical_Params_To_Optics(
    unsigned char *pOpticalParams, short len);
static int devLaser_Read(unsigned long ioctl_cmd, unsigned long *value);
static int get_register(uint8_t client, uint16_t offset, uint16_t len, unsigned char *buf);
static void cmdLaserGeneralParamsSet(char *pszclient, char *pszoffset, char *pszlen, unsigned char *pszbuf);
static void cmdLaserGeneralParamsGet(char *pszclient, char *pszoffset, char *pszlen);
static CmsRet cmdLaserDumpData(void);
static void cmdLaserCalSet(char *pszparam, char *pszval, char *pszval1, char *pszval2, char *pszval3);
static void cmdLaserCalGet(char *pszparam, char *pszval);
static void cmdLaserMsgWrite(char *pszmsg_id,  char *pszlen, unsigned char *pszbuf);
static void cmdLaserMsgRead(char *pszmsg_id, char *pszlen);
static void cmdRssiCalSet(char *pszrssi,  char *pszop);
static void cmdLaserTempToApdSet(char *ApdList);
static void cmdLaserResToTempSet(char *ResList);


static int parse_hexa_and_length(char *pszlen, unsigned char *pszbuf, unsigned short *data_length)
{
    int i;

    *data_length = (unsigned short)strtol(pszlen, NULL, 10);

    *data_length = *data_length < PMD_BUF_MAX_SIZE ? *data_length : PMD_BUF_MAX_SIZE;
    if (strlen((const char *)pszbuf) != *data_length * 2)
        return -1;

    for(i = 0; i < *data_length; i++)
    {
        pszbuf[i*2]=TOHEX(pszbuf[i*2]);
        pszbuf[i*2+1]=TOHEX(pszbuf[i*2+1]);
        pszbuf[i]=((pszbuf[i*2] <<4)+pszbuf[i*2+1]);
    }

    return 0;
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserHelp
*  DESCRIPTION  :   Prints help information about the Laser commands.
*  PARAMETERS   :   None
*  RETURNS      :   0 on success
*
*****************************************************************************/
/* If you want to stringify the result of expansion of a macro argument, you have to use two levels of macros. */
#if !defined(STRINGIFY)
#define STRINGIFY(s) #s
#endif
#if !defined(DEF2STR)
#define DEF2STR(s) STRINGIFY(s)
#endif
static void cmdLaserHelp(char *help)
{
    static const char laserusage[] = "\nUsage:\n"
        "        laser            [--help [<command>]]\n";
    static const char laserload[] =
        "        laser param       --load <filename>\n";
    static const char laserdump[] =
        "        laser param       --dump current\n";
        /*"        laser param       --dump default | current\n"; 'default' is not supported*/
    static const char laserlos[] =
        "        laser los         --read\n";
    static const char laserpower[] =
        "        laser power\n";
    static const char laserpowerrxread[] =
        "        laser power       --rxread\n";
    static const char laserpowertxread[] =
        "        laser power       --txread\n";
    static const char lasertxbiasread[] =
        "        laser txbias      --read\n";
    static const char lasertxmodread[] =
        "        laser txmod       --read\n";
    static const char lasertempread[] =
        "        laser temperature --read\n";
    static const char laservoltageread[] =
        "        laser voltage     --read\n";
    static const char laserGeneralSet[] =
        "        laser general     --set   <client{0-REG,1-IRAM,2-DRAM}> <offset> <len{decimal}> <data>\n";
    static const char laserGeneralGet[] =
        "        laser general     --get   <client{0-REG,1-IRAM,2-DRAM}> <offset> <len{decimal}>\n";
    static const char laserCalSet[] =
        "        laser calibration --set   <parameter{decimal}> <value{hexadecimal}>\n";
    static const char laserCalGet[] =
        "        laser calibration --get   <parameter{decimal}> [<index{hexadecimal}>]\n";
    static const char laserCalPmdParams[] =
        "        laser calibration ?\n";
    static const char laserCalPmdRst[] =
        "        laser calibration --pmdreset {clears the calibration file and enters into pre-calibration mode}\n";
    static const char laserMsgSet[] =
        "        laser msg         --set   <msg_id> <len{decimal number of bytes}> <data>\n";
    static const char laserMsgGet[] =
        "        laser msg         --get   <msg_id> <len{decimal}>\n";
    static const char laserRssiCal[] =
        "        laser rssi cal set <rssi> <optic_power>\n";
    static const char laserTempToApdMap[] =
        "        laser temp2apd    --set   <list of " DEF2STR(APD_TEMP_TABLE_SIZE) " decimal parameters delimited by "
        "',' or ';'>\n";
    static const char laserDumpData[] =
        "        laser dumpdata   [--show]\n";
    static const char laserResToTempMap[] =
        "        laser res2temp    --set   <list of " DEF2STR(TEMP_TABLE_SIZE) " decimal parameters delimited by ',' or"
        " ';'>\n";

    if (!help || !strcasecmp(help, "--help"))
    {
        printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", laserusage, laserload, laserdump, laserlos,
            laserpower, laserpowerrxread, laserpowertxread, lasertxbiasread, lasertxmodread, lasertempread,
            laservoltageread, laserGeneralSet, laserGeneralGet, laserCalSet, laserCalGet, laserCalPmdParams,
            laserCalPmdRst, laserMsgSet, laserMsgGet, laserRssiCal, laserTempToApdMap, laserDumpData, laserResToTempMap);
    }
    else if (!strcasecmp(help, "param"))
    {
        printf("%s%s%s", laserusage, laserload, laserdump);
    }
    else if (!strcasecmp(help, "dump"))
    {
        printf("%s%s", laserusage, laserdump);
    }
    else if (!strcasecmp(help, "los"))
    {
        printf("%s%s", laserusage, laserlos);
    }
    else if (!strcasecmp(help, "load"))
    {
        printf("%s%s", laserusage, laserload);
    }
    else if (!strcasecmp(help, "power"))
    {
        printf("%s%s%s", laserusage, laserpowerrxread, laserpowertxread);
    }
    else if (!strcasecmp(help, "txbias"))
    {
        printf("%s%s", laserusage, lasertxbiasread);
    }
    else if (!strcasecmp(help, "txmod"))
    {
        printf("%s%s", laserusage, lasertxmodread);
    }
    else if (!strcasecmp(help, "temperature"))
    {
        printf("%s%s", laserusage, lasertempread);
    }
    else if (!strcasecmp(help, "voltage"))
    {
        printf("%s%s", laserusage, laservoltageread);
    }
    else if (!strcasecmp(help, "general"))
    {
        printf("%s%s%s", laserusage, laserGeneralSet, laserGeneralGet);
    }
    else if (!strcasecmp(help, "generalset"))
    {
        printf("%s%s", laserusage, laserGeneralSet);
    }
    else if (!strcasecmp(help, "generalget"))
    {
        printf("%s%s", laserusage, laserGeneralGet);
    }
    else if (!strcasecmp(help, "msg"))
    {
        printf("%s%s%s", laserusage, laserMsgSet, laserMsgGet);
    }
    else if (!strcasecmp(help, "msgset"))
    {
        printf("%s%s", laserusage, laserMsgSet);
    }
    else if (!strcasecmp(help, "msgget"))
    {
        printf("%s%s", laserusage, laserMsgGet);
    }
    else if (!strcasecmp(help, "calibration"))
    {
        printf("%s%s%s%s%s", laserusage, laserCalSet, laserCalGet, laserCalPmdParams, laserCalPmdRst);
    }
    else if (!strcasecmp(help, "calibrationset"))
    {
        printf("%s%s", laserusage, laserCalSet);
    }
    else if (!strcasecmp(help, "calibrationget"))
    {
        printf("%s%s", laserusage, laserCalGet);
    }
    else if (!strcasecmp(help, "rssi"))
    {
        printf("%s%s", laserusage, laserRssiCal);
    }
    else if (!strcasecmp(help, "temp2apd"))
    {
        printf("%s%s", laserusage, laserTempToApdMap);
    }
    else if (!strcasecmp(help, "dumpdata"))
    {
        printf("%s%s", laserusage, laserDumpData);
    }
    else if (!strcasecmp(help, "res2temp"))
    {
        printf("%s%s", laserusage, laserResToTempMap);
    }
    else
    {
        cmdLaserHelp(NULL);
    }
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserLoad
*  DESCRIPTION  :   laser load [<filenamearg>]
*               :   Causes optical parameters to be loaded onto BOSA optics 
*               :   device, if present.  Default is to load params from NVRAM
*               :   source, but options can cause loading from external file.
*  PARAMETERS   :   
*  RETURNS      :   0 on success
*
*****************************************************************************/
static CmsRet cmdLaserLoad(char *pFileNameArg)
{
    CmsRet Ret = CMSRET_SUCCESS;
    unsigned char ucOpticalParams[BP_OPTICAL_PARAMS_LEN];

    if (!pFileNameArg)
    {
        printf("Cannot read default optical parameters.\n");
        Ret = CMSRET_INVALID_ARGUMENTS;
        cmdLaserHelp("load");
    }
    else
    {
        /* Read the default optical parameters from a file.  The file format is
         * <hex_offset_0x00_0x2f> <hex_value_0x00_0xff>
         */
        FILE *fp = fopen(pFileNameArg, "r");

        if (fp)
        {
            char buf[16];
            char *p;
            long idx;

            printf("Loading parameters from file %s into laser driver chip.\n",
                pFileNameArg);
            while( fgets(buf, sizeof(buf), fp) )
            {
                p = NULL;
                idx = strtol(buf, &p, 16);
                if (p && idx < sizeof(ucOpticalParams))
                    ucOpticalParams[idx] = strtol(p, NULL, 16) & 0xff;
            }

            fclose( fp );
        }
        else
        {
            printf("Cannot open parameters file %s\n", pFileNameArg);
            Ret = CMSRET_INVALID_ARGUMENTS;
        }
    }

    if (Ret == CMSRET_SUCCESS)
    {
        if (devLaser_Write_Optical_Params_To_Optics(ucOpticalParams, sizeof(ucOpticalParams)))
        {
            printf("Cannot write optical parameters to laser device.\n");
            Ret = CMSRET_INTERNAL_ERROR;
        }
    }

    return Ret;
}

/*****************************************************************************
*
*  FUNCTION     :   cmdLaserDump
*  DESCRIPTION  :   laser param --dump [r|n]
*               :   Causes optical parameters to be dumped to the console.
*  PARAMETERS   :   
*  RETURNS      :   0 on success
*
*****************************************************************************/
static CmsRet cmdLaserDump(char *pDumpArg)
{
    CmsRet Ret = CMSRET_INVALID_ARGUMENTS;
    unsigned char ucOpticalParams[LASER_TOTAL_OPTICAL_PARAMS_LEN];
    int len;

    if (!pDumpArg)
    {
        cmdLaserHelp("dump");
        return CMSRET_SUCCESS;
    }
    
    /* if (!strcmp(pDumpArg, "default"))
    {
        / * not implemented * /
    }
    else */
    if (!strcmp(pDumpArg, "current"))
    {
        /* Read all (R/W and R only) optical parameters from the laser driver
        * chip.
        */
        len = LASER_TOTAL_OPTICAL_PARAMS_LEN;
        if (!devLaser_Read_Optical_Params_From_Optics(ucOpticalParams, sizeof(ucOpticalParams)))
        {
            Ret = CMSRET_SUCCESS;
        }
    }
    else
    {
        cmdLaserHelp("dump");
        return CMSRET_SUCCESS;
    }

    if (Ret == CMSRET_SUCCESS)
    {
        int i;
        for(i = 0; i < len; i++)
            printf("0x%2.2x 0x%2.2x\n", i, ucOpticalParams[i]);
    }
    else
    {
        cmsLog_error("Invalid arguments\n");
    }

    return Ret;
}

/*****************************************************************************
*
*  FUNCTION     :   cmdLaserPowerRead
*  DESCRIPTION  :   Sends a command to the laser driver to read and calculate
*               :   the current Rx or Tx power in micro-watts.
*  PARAMETERS   :   
*  RETURNS      :   0 on success
*
*****************************************************************************/
static CmsRet cmdLaserPowerRead(int ioctl_cmd, unsigned long *pPower)
{
    CmsRet Ret = CMSRET_SUCCESS;

    if (devLaser_Read(ioctl_cmd, pPower))
    {
        Ret = CMSRET_INVALID_ARGUMENTS;
        cmsLog_error("Power not read\n");
    }

    return Ret;
}



/*****************************************************************************
*
*  FUNCTION     :   cmdLaserRead
*  DESCRIPTION  :   Sends a command to the laser driver to read
*               :   the current Tx Bias / Temperature / Voltage.
*  PARAMETERS   :   
*  RETURNS      :   0 on success
*
*****************************************************************************/
static CmsRet cmdLaserRead(int ioctl_cmd, unsigned long *pRead)
{
    CmsRet Ret = CMSRET_SUCCESS;

    if (devLaser_Read(ioctl_cmd, pRead))
        Ret = CMSRET_INVALID_ARGUMENTS;

    if (Ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Current status not read\n");
    }

    return Ret;
}

/***************************************************************************
 * Function Name: processLaserCmd
 * Description  : Parses CMF commands
 ***************************************************************************/
#if defined(PMD_JSON_LIB)
extern void pmd_calibration_json_file_set_scripts_manifest(const char *calibration_manifest);
extern void pmd_calibration_json_file_stamp_scripts_manifest(const char *calibration_manifest);
extern void pmd_calibration_json_file_reset_manifest(void);
extern void pmd_calibration_delete_files(void);
#endif
void processLaserCmd(char *cmdLine)
{
    SINT32 argc = 0;
    char *argv[MAX_OPTS]={NULL};
    char *last = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    /* parse the command line and build the argument vector */
    argv[0] = strtok_r(cmdLine, " ", &last);

    if (argv[0])
    {
        for(argc = 1; argc < MAX_OPTS; ++argc)
        {
            argv[argc] = strtok_r(NULL, " ", &last);

            if (!argv[argc])
            {
                break;
            }

            cmsLog_debug("arg[%d]=%s", argc, argv[argc]);
        }
    }

    if (!argv[0])
    {
        cmdLaserHelp(NULL);
    }
    else if (!strcasecmp(argv[0], "--help"))
    {
        cmdLaserHelp(argv[1]);
    }
    else if (!strcasecmp(argv[0], "param"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--load"))
            {
                ret = cmdLaserLoad(argv[2]);
            }
            else if (!strcasecmp(argv[1], "--dump"))
            {
                ret = cmdLaserDump(argv[2]);
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            } 
            if (ret != CMSRET_SUCCESS)
            {
                cmsLog_error("Unknown Error\n");
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "power"))
    {
        unsigned long power;

        printf("\n");
        if (!argv[1])
        {
            ret = cmdLaserPowerRead(LASER_IOCTL_GET_RX_PWR, &power);
            if (ret == CMSRET_SUCCESS)
                printf("Rx Laser Power           = %lu uW\n", power / 10);

            ret = cmdLaserPowerRead(LASER_IOCTL_GET_TX_PWR, &power);
            if (ret == CMSRET_SUCCESS)
                printf("Tx Laser Power           = %lu uW\n", power / 10);
        }
        else if (!strcasecmp(argv[1], "--rxread"))
        {
            ret = cmdLaserPowerRead(LASER_IOCTL_GET_RX_PWR, &power);
            if (ret == CMSRET_SUCCESS)
            {
                printf("Rx Laser Power           = %f uW\n", power / 10.0);
                printf("                         = %f dBm\n", 10 * log10(power / 10000.0));
            }
        }
        else if (!strcasecmp(argv[1], "--txread"))
        {
            ret = cmdLaserPowerRead(LASER_IOCTL_GET_TX_PWR, &power);
            if (ret == CMSRET_SUCCESS)
                printf("Tx Laser Power           = %f uW\n", power / 10.0);
                printf("                         = %f dBm\n", 10 * log10(power / 10000.0));
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
        printf("\n");
    }
    else if (!strcasecmp(argv[0], "txbias"))
    {
        unsigned long bias;

        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--read"))
            {
                ret = cmdLaserRead(LASER_IOCTL_GET_BIAS_CURRENT, &bias);
                if (ret == CMSRET_SUCCESS)
                    printf("Tx Bias Current          = %ld uA\n", bias*2);
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }           
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "los"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--read"))
            {
                unsigned char buf[8];
                int ret;

                ret = get_register(0, 0x94, 4, buf);
                if (!ret)
                {
                    if (buf[3])
                    {
                        printf("Rx signal is detected (LOS = False)\n");
                    }
                    else
                    {
                        printf("Rx signal is lost (LOS = True)\n");
                    }
                }
                else
                {
                    cmsLog_error("Unknown Error\n");
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }           
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "txmod"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--read"))
            {
                unsigned char buf[8];
                int ret;

                ret = get_register(0, 0x92c, 4, buf);
                if (!ret)
                {
                    uint32_t reg0x92c = ((uint32_t)buf[0])<<24 | ((uint32_t)buf[1])<<16 | ((uint32_t)buf[2])<<8 |
                        (uint32_t)buf[3];
                    float modulation_current = (float)reg0x92c * 80 / 2047;
                    printf("Tx Modulation Current = %f mA\n", modulation_current);
                }
                else
                {
                    cmsLog_error("Unknown Error\n");
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }           
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "voltage"))
    {
        unsigned long voltage;

        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--read"))
            {
                ret = cmdLaserRead(LASER_IOCTL_GET_VOLTAGE, &voltage);
                if (ret == CMSRET_SUCCESS)
                {
                    if (voltage >> 16)
                    {
                        printf("PMD Voltage 3.3V         = %ld mV\n", (voltage & 0xffff) / 10);
                        printf("PMD Voltage 1.0V         = %ld mV\n", (voltage >> 16) / 10);
                    }
                    else
                    {
                        printf("Voltage                  = %ld mV\n", voltage / 10);
                    }
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }           
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "temperature"))
    {
        unsigned long temp;

        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--read"))
            {
                ret = cmdLaserRead(LASER_IOCTL_GET_TEMPTURE, &temp);
                if (ret == CMSRET_SUCCESS)
                {
                    int8_t celsius = (temp >> 8) & 0xff;
                    printf("Temperature              = %d c\n", celsius);
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }           
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "general"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--set"))
            {
                cmdLaserGeneralParamsSet(argv[2], argv[3], argv[4], (unsigned char *)argv[5]);
            }
            else if (!strcasecmp(argv[1], "--get"))
            {
                cmdLaserGeneralParamsGet(argv[2], argv[3], argv[4]);
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "calibration"))
    {
        if (argv[1])
        {       
            if (!strcasecmp(argv[1], "--set"))
            {
                cmdLaserCalSet(argv[2], argv[3], argv[4], argv[5], argv[6]);
            }
            else if (!strcasecmp(argv[1], "--get"))
            {
                cmdLaserCalGet(argv[2], argv[3]);
            }
            else if (!strcasecmp(argv[1], "--pmdreset"))
            {
#if defined(PMD_JSON_LIB)
                pmd_calibration_json_file_reset_manifest();
                pmd_calibration_delete_files();
#endif
                if (devLaser_Reset_PMD_into_Calibration_Mode())
                {
                    cmsLog_error("ERROR: PMD reset into pre-calibration mode failed\n");
                }
            }
            else if (!strcasecmp(argv[1], "?"))
            {
                printf("0:watermark 1:version 2:level_0 3:level_1 4:bias 5:mod 6:apd {type voltage} "
                    "7:mpd config{tia_gain vga_gain} 8:mpd gains {bias mod} 9:apdoi_ctrl\n");
                printf("10:rssi_a_factor 11:rssi_b_factor 12:rssi_c_factor 13: temp_0 14: temp coff high "
                    "15: temp coff low 16:esc thr 17:rogue thr 18:avg_level_0 19:avg_level_1\n");
                printf("20:dacrange 21: los thr {assert deassert} 22: sat pos {high low} 23: sat neg {high low} 24: "
                    "edge {rate dload} 25: preemphasis weight\n");
                printf("26:preemphasis_delay 27:duty_cycle 28: mpd_calibctrl\n");
                printf("29:tx_power 30: bias0 31:temp_offset 32:bias_delta_i 33:slope_efficiency "
                    "35:adf_los_thresholds {assert, deassert} 36:adf_los_leaky_bucket {assert, lb_bucket_sz}\n");
                printf("37:compensation {bit 0(compensation_enable0 | bit 1(compensation_enable1), die temp ref, "
                    "compensation_coeff1_q8, compensation_coeff2_q8}\n");
            }
#if defined(PMD_JSON_LIB)
            else if (!strcasecmp(argv[1], "--start"))
            {
                pmd_calibration_json_file_set_scripts_manifest(argv[2]);
            }
            else if (!strcasecmp(argv[1], "--stop"))
            {
                pmd_calibration_json_file_stamp_scripts_manifest(argv[2]);
            }
#endif
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "msg"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--get"))
            {
                cmdLaserMsgRead(argv[2], argv[3]);
            }
            else if (!strcasecmp(argv[1], "--set"))
            {
                cmdLaserMsgWrite(argv[2], argv[3], (unsigned char *)argv[4]);
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "rssi"))
    {
        if (!argv[1] || !argv[2])
        {
            cmdLaserHelp(argv[0]);
        }
        else if (!strcasecmp(argv[1], "cal") && !strcasecmp(argv[2], "set"))
        {
            cmdRssiCalSet(argv[3], argv[4]);
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "temp2apd"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--get"))
            {
                /* not implemented */
                cmdLaserHelp(argv[0]);
            }
            else if (!strcasecmp(argv[1], "--set"))
            {
                if (argv[2])
                {
                    cmdLaserTempToApdSet(argv[2]);
                }
                else
                {
                    cmsLog_error("Invalid Laser Command\n");
                    cmdLaserHelp(argv[0]);
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else if (!strcasecmp(argv[0], "dumpdata"))
    {
        ret = cmdLaserDumpData();
        if (ret == CMSRET_SUCCESS)
        {
            if (argv[1])
            {
                if (!strcasecmp(argv[1], "--show"))
                {
                    system("cat data/pmd_dump_data");
                }
                else
                {
                    cmdLaserHelp(argv[0]);
                }
            }
        }
        else
        {
            cmsLog_error("Unknown Error\n");
        }
    }
    else if (!strcasecmp(argv[0], "res2temp"))
    {
        if (argv[1])
        {
            if (!strcasecmp(argv[1], "--get"))
            {
                /* not implemented */
                cmdLaserHelp(argv[0]);
            }
            else if (!strcasecmp(argv[1], "--set"))
            {
                if (argv[2])
                {
                    cmdLaserResToTempSet(argv[2]);
                }
                else
                {
                    cmsLog_error("Invalid Laser Command\n");
                    cmdLaserHelp(argv[0]);
                }
            }
            else
            {
                cmsLog_error("Invalid Laser Command\n");
                cmdLaserHelp(argv[0]);
            }
        }
        else
        {
            cmsLog_error("Invalid Laser Command\n");
            cmdLaserHelp(argv[0]);
        }
    }
    else
    {
        cmsLog_error("Invalid Laser Command\n");
        cmdLaserHelp(NULL);
    }
}


/*****************************************************************************
 * Kernel Driver Interface Functions
 *****************************************************************************/

/*****************************************************************************
*
*  FUNCTION     :   devLaser_Reset_PMD_into_Calibration_Mode
*
*  DESCRIPTION  :   This routine is used to reset the PMD into
*                   calibration mode.
*
*  PARAMETERS   :   None.
*
*  RETURNS      :   0 on success
*
*****************************************************************************/
static int devLaser_Reset_PMD_into_Calibration_Mode(void)
{
    int ret = -1;
    int LaserDevFd;

    LaserDevFd = open(LASER_DEV, O_RDWR);
    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, PMD_IOCTL_RESET_INTO_CALIBRATION_MODE, NULL);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*
*  FUNCTION     :   devLaser_Read_Optical_Params_From_Optics
*
*  DESCRIPTION  :   This routine is used to retreive a copy of the optical
*                   parameters from the registers of the BOSA optics device.
*
*  PARAMETERS   :   OUT: pOpticalParams contains the address of char array of
*                   params read from the BOSA optics.
*
*  RETURNS      :   0 on success
*
*****************************************************************************/
static int devLaser_Read_Optical_Params_From_Optics(
    unsigned char *pOpticalParams, short len)
{
    int ret = -1;
    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        LASER_OPTICAL_PARAMS lop = {len, {pOpticalParams}};
        ret = ioctl(LaserDevFd, LASER_IOCTL_GET_OPTICAL_PARAMS, &lop);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_Write_Optical_Params_To_Optics
*               :
*  DESCRIPTION  :   This routine is used to write the contents of the optical
*               :   parameter DB to registers in the optics device
*               :
*  PARAMETERS   :   IN: pOpticalParams contains address of char array of params
*               :   to be written to NVRAM.
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_Write_Optical_Params_To_Optics(
    unsigned char *pOpticalParams, short len)
{
    int ret = -1;
    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        LASER_OPTICAL_PARAMS lop = {len, {pOpticalParams}};
        ret = ioctl(LaserDevFd, LASER_IOCTL_SET_OPTICAL_PARAMS, &lop);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_Read
*               :
*  DESCRIPTION  :   This routine is used to read the current Rx or Tx power
*               :   value.
*               :
*  PARAMETERS   :   IN: IOCTL command (Rx or Tx) and pointer to a word to
*               :   hold the return value
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_Read(unsigned long ioctl_cmd, unsigned long *value)
{
    int ret = -1;
    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, ioctl_cmd, value);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_SetGeneralParams
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_SetGeneralParams(uint8_t client, uint16_t offset, unsigned char *buf, uint16_t len)
{
    pmd_params param = {.client = client, .offset = offset, .len = len, .buf = buf};
    int ret = -1;

    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, PMD_IOCTL_SET_PARAMS, &param);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_GetGeneralParams
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_GetGeneralParams(uint8_t client, uint16_t offset, uint16_t len)
{
    unsigned char buf[PMD_BUF_MAX_SIZE];    
    int ret;
    uint16_t count = 0;

    ret = get_register(client, offset, len, buf);
    if (ret)
    {
        return ret;
    }

    while (count < len)
    {
        if (count && count % 4 == 0)
        {
            printf(" ");
        }
        printf("%02x", buf[count]);
        count++;
    }
    printf("\n");

    return ret;
}

static int get_register(uint8_t client, uint16_t offset, uint16_t len, unsigned char *buf)
{
    pmd_params param = {.client = client, .offset = offset, .len = len, .buf = buf};
    int laser_dev_fd, ret = -1;

    laser_dev_fd = open(LASER_DEV, O_RDWR);
    if (0 > laser_dev_fd)
    {
        return ret;
    }
    
    ret = ioctl(laser_dev_fd, PMD_IOCTL_GET_PARAMS, &param);
    close(laser_dev_fd);
    
    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_SetCalParams
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
#if defined(PMD_JSON_LIB)
extern int pmd_calibration_parameter_to_json_file(pmd_calibration_parameters_index parameter_index, int32_t val,
    uint16_t set_index);
#endif
static int devLaser_SetCalParams(uint16_t offset, int32_t val, uint16_t cal_index)
{
    pmd_params param = {.offset = offset, .len = cal_index, .buf = (unsigned char *)&val};
    int ret;

    int LaserDevFd = open(LASER_DEV, O_RDWR);
    if (0 > LaserDevFd)
    {
        return ERR_CAL_PARAM_CANNOT_OPEN_DEVICE;
    }

    if (CAL_MULT_LEN <= cal_index)
    {
        cmsLog_error("ERROR: PMD can't set parameter %d at index %d (max index is %d).\n", offset, cal_index,
            CAL_MULT_LEN - 1);
        return ERR_CAL_PARAM_INDEX_OUT_OF_RANGE;
    }

    ret = ioctl(LaserDevFd, PMD_IOCTL_CAL_FILE_WRITE, &param);
    close(LaserDevFd);

    if (ret || PMD_HOST_SW_CONTROL == offset || PMD_STATISTICS_COLLECTION == offset)
    {
        return ret;
    }

#if defined(PMD_JSON_LIB)
    ret = pmd_calibration_parameter_to_json_file(offset, val, cal_index);
#endif

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_GetCalParams
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_GetCalParams(uint16_t offset, int32_t cal_index)
{
    int32_t val = 0;
    pmd_params param = {.offset = offset , .len = cal_index, .buf = (unsigned char *)&val};
    int ret;

    int LaserDevFd = open(LASER_DEV, O_RDWR);
    if (0 > LaserDevFd)
    {
        return ERR_CAL_PARAM_CANNOT_OPEN_DEVICE;
    }

    if (CAL_MULT_LEN <= cal_index)
    {
        cmsLog_error("ERROR: PMD can't get parameter %d at index %d (max index is %d).\n", offset, cal_index,
            CAL_MULT_LEN - 1);
        return ERR_CAL_PARAM_INDEX_OUT_OF_RANGE;
    }

    ret = ioctl(LaserDevFd, PMD_IOCTL_CAL_FILE_READ, &param);
    close(LaserDevFd);

    /* negative ioctl file_operations are returned as (-1). So we use positive values and convert */
    ret = -ret;
    if (ret)
    {
        return ret;
    }

    if (offset == PMD_MPD_GAINS)
    {
        float bias_gain, mod_gain;

        bias_gain = (float)(val & 0xff) / 256;
        mod_gain  = (float)(val >> 8 ) / 256;

        printf("bias gain %f mod gain %f\n", bias_gain, mod_gain);
    }
    else if (offset == PMD_APD )
    {
        uint16_t type, voltage;
        type = (val & 400) >> 10;
        voltage = val & 0x3ff;

        printf("apd type %d apd voltage 0x%02x\n", type, voltage);
    }
    else if (offset == PMD_MPD_CONFIG)
    {
        uint16_t tia, vga;
        vga = (val & 0xc00) >> 10;
        tia = (val & 0xF000) >> 12;

        printf("tia %d vga %d\n", tia, vga);
    }
    else if (offset == PMD_RSSI_A || offset == PMD_RSSI_B || offset == PMD_RSSI_C)
    {
        float rssi = (float)val / 256;
        printf("%f\n", rssi);
        return ret;
    }
    else if (offset == PMD_TEMP_COFF_H)
    {

        float alph_h = (float)val / 4096;
        printf("%f\n", alph_h);
        return ret;
    }
    else if (offset == PMD_TEMP_COFF_L)
    {
        float alph_l = (float)val / 4096;
        printf("%f\n", alph_l);
        return ret;
    }
    else if (offset == PMD_DACRANGE)
    {
        printf("dacrange 0x%x\n", val);
    }
    else if (offset == PMD_LOS_THR)
    {
        uint16_t assert, deassert;
        assert = val & 0xff;
        deassert = (val & 0xff00) >> 8;

        printf("assert 0x%x deassert 0x%x\n", assert, deassert);
    }
    else if (offset == PMD_SAT_POS)
    {
        uint16_t high, low;
        high = val & 0xff;
        low = (val & 0xff00) >> 8;

        printf("high 0x%x low 0x%x\n", high, low);
    }
    else if (offset == PMD_SAT_NEG)
    {
        uint16_t high, low;
        high = val & 0xff;
        low = (val & 0xff00) >> 8;

        printf("high 0x%x low 0x%x\n", high, low);
    }
    else if (offset == PMD_EDGE_RATE)
    {
        uint16_t rate, dload;
        rate = val & 0xff;
        dload = (val & 0xff00) >> 8;

        printf("edge rate 0x%x dload rate 0x%x\n", rate, dload);
    }
    else if (offset == PMD_ADF_LOS_LEAKY_BUCKET)
    {
        uint16_t lb_assert_th, lb_bucket_sz;
        lb_assert_th = val & 0xff;
        lb_bucket_sz = (val & 0xff00) >> 8;

        printf("ADF LOS leaky bucket assert threshold 0x%x bucket size 0x%x\n", lb_assert_th , lb_bucket_sz);
    }
    else if (offset == PMD_ADF_LOS_THRESHOLDS)
    {
        uint16_t assert_th, deassert_th;
        assert_th = val & 0xffff;
        deassert_th = (val & 0xffff0000) >> 16;

        printf("ADF LOS assert threshold 0x%x deassert threshold 0x%x\n", assert_th , deassert_th);
    }
    else if(offset == PMD_COMPENSATION)
    {
        uint8_t compensation_enable0 = val & 1;
        uint8_t compensation_enable1 = (val >> 1) & 1;
        int8_t die_tmp_ref = (val >> 8) & 0xff;
        uint8_t compensation_coeff1_q8 = (val >> 16) & 0xff;
        uint8_t compensation_coeff2_q8 = (val >> 24) & 0xff;

        printf("Level0 tracking compensation: enable %d, delta offset enable %d, die temp ref 0x%x, "
            "positive temp change offset 0x%x negative temp change offset 0x%x\n",
            compensation_enable0, compensation_enable1, die_tmp_ref, compensation_coeff1_q8, compensation_coeff2_q8);
    }
    else
    {
        printf("%02x\n", val);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_MsgWrite
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_MsgWrite(uint16_t msg_id, uint16_t len, unsigned char *buf)
{
    pmd_params param = { .offset = msg_id, .len = len, .buf = buf};

    int ret = -1;

    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, PMD_IOCTL_MSG_WRITE, &param);
        close(LaserDevFd);
    }

    return ret;
}

/*****************************************************************************
*  FUNCTION     :   devLaser_MsgRead
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
static int devLaser_MsgRead(uint16_t msg_id, uint16_t len)
{
    unsigned char buf[PMD_BUF_MAX_SIZE];
    pmd_params param = { .offset = msg_id, .len = len, .buf = buf};
    int ret = -1;
    uint16_t count = 0;
    int16_t tmp_16;
    int32_t tmp_32;
    uint32_t tmp_u32;
    float output;

    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, PMD_IOCTL_MSG_READ, &param);
        close(LaserDevFd);
    }

    if (!ret)
    {
        if (msg_id  == PMD_RSSI_GET_MSG)
        {
            tmp_32 = *(int32_t *)(param.buf);
            tmp_32 = ((tmp_32 & (int32_t)0x000000ffUL) << 24) | ((tmp_32 & (int32_t)0x0000ff00UL) <<  8) |
                ((tmp_32 & (int32_t)0x00ff0000UL) >>  8) | (((uint32_t)(tmp_32 & (int32_t)0xff000000UL)) >> 24);
            output = (float)tmp_32 / 65536;
            printf("%f\n", output);
            return ret;
        }
        if (msg_id  == PMD_ESTIMATED_OP_GET_MSG)
        {
            tmp_16 = *(int16_t *)(param.buf);
            tmp_16 = ((tmp_16 & (int16_t)0x00ffU) << 8) | (((uint16_t)(tmp_16 & (int16_t)0xff00U)) >> 8);
            output = (float)tmp_16 / 16;
            printf("%f uW\n", output);
            return ret;
        }
        if (msg_id == PMD_FW_VERSION_GET_MSG)
        {
            tmp_u32 = *(uint32_t *)(param.buf);
            tmp_u32 = ((tmp_u32 & (uint32_t)0x000000ffUL) << 8) | ((tmp_u32 & (uint32_t)0x0000ff00UL) >> 8) |
                 ((tmp_u32 & (uint32_t)0x00ff0000UL) << 8) | (((uint32_t)(tmp_u32 & (uint32_t)0xff000000UL)) >> 8);
            printf("pmd firmware version: %d\n", tmp_u32);
            return ret;    
        }
        while( count < len)
        {
            if (count && count % 4 == 0)
                printf(" ");

            printf("%02x", param.buf[count]);
            count++;
        }
        printf("\n");
    }

    return ret;
}

/*****************************************************************************
*
*  FUNCTION     :   cmdLaserGeneralParamsSet
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserGeneralParamsSet(char *pszclient, char *pszoffset, char *pszlen, unsigned char *pszbuf)
{
    uint8_t client;
    unsigned short offset;
    unsigned short len;

    if (!(pszclient && pszoffset && pszlen && pszbuf))
    {
        cmdLaserHelp("generalset");
        return;
    }

    client = (uint8_t)strtol(pszclient, NULL, 16);
    offset = (unsigned short)strtol(pszoffset, NULL, 16);

    if (parse_hexa_and_length(pszlen, pszbuf, &len))
    {
        printf("General parameters set error: %zu characters were entered whereas %d were expected\n",
            strlen((const char *)pszbuf), 2*len);
        cmsLog_error("General parameters set error: %zu characters were entered whereas %d were expected\n",
            strlen((const char *)pszbuf), 2*len);
        return;
    }

    if (devLaser_SetGeneralParams(client, offset, pszbuf, len))
    {
        printf("General parameters set error\n");
        cmsLog_error("General parameters set error\n");
    }
    else
        printf("-Set general parameters done\n");
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserGeneralParamsGet
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserGeneralParamsGet(char *pszclient, char *pszoffset, char *pszlen)
{
    uint8_t client;
    unsigned short offset;
    short len;

    if (!(pszclient && pszoffset && pszlen))
    {
        cmdLaserHelp("generalget");
        return;
    }

    client = (uint8_t)strtol(pszclient, NULL, 16);
    offset = (unsigned short)strtol(pszoffset, NULL, 16);
    len    = (unsigned short)strtol(pszlen, NULL, 10);

    len = len < PMD_BUF_MAX_SIZE ? len : PMD_BUF_MAX_SIZE;

    if (devLaser_GetGeneralParams(client, offset, len))
    {
        printf("General parameters get error\n");
        cmsLog_error("General parameters get error\n");
    }
    else
        printf("-Get general parameters done\n");
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserDumpData
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :   0 on success
*
*****************************************************************************/
static CmsRet cmdLaserDumpData()
{
    int ret = -1;

    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (LaserDevFd >= 0)
    {
        ret = ioctl(LaserDevFd, PMD_IOCTL_DUMP_DATA);
        close(LaserDevFd);
    }

    return ret;
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserCalSet
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserCalSet(char *pszparam, char *pszval, char *pszval1, char *pszval2, char *pszval3)
{
    int32_t val = 0;
    uint16_t cal_index = 0, param;

    if (!(pszparam && pszval))
    {
        cmdLaserHelp("calibrationset");
        return;
    }

    /* parse the command line and build the argument vector */
    param = (uint16_t)strtol(pszparam, NULL, 10);
    val = (int32_t)strtol(pszval, NULL, 16);

    if (val == CAL_FILE_INVALID_ENTRANCE)
    {
        printf("0x%X is invalid value\n", val);
        return;
    }

    if (pszval1 && pszval2)
    {
        switch (param)
        {
            case PMD_EDGE_RATE:
            {
                uint16_t rate, dload;

                printf("Edge rate: <index> <rate> <dload>\n");
                cal_index = (uint16_t)strtol(pszval, NULL, 16);
                rate = (uint16_t)strtol(pszval1, NULL, 16);
                dload = (uint16_t)strtol(pszval2, NULL, 16);

                val = (dload & 0xff) << 8 | (rate & 0xff);
                break;
            }

            case PMD_COMPENSATION:
            {
                if (pszval3)
                {
                    uint32_t enable_comp_and_delta = strtol(pszval, NULL, 16);
                    uint32_t die_tmp_ref = strtol(pszval1, NULL, 16);
                    uint32_t compensation_coeff1_q8 = strtol(pszval2, NULL, 16);
                    uint32_t compensation_coeff2_q8 = strtol(pszval3, NULL, 16);

                    val = (compensation_coeff2_q8 << 24) | (compensation_coeff1_q8 << 16) | (die_tmp_ref << 8) |
                        enable_comp_and_delta;

                    break; /* break only when (pszval3) */
                }
            }

            default:
                printf("wrong parameters\n");
                cmdLaserHelp("calibrationset");
                return;
        }
    }

    if (pszval1 && !pszval2)
    {
        switch (param)
        {
            case PMD_MPD_CONFIG:
            {
                uint16_t tia, vga;

                printf("MPD configuration: <tia> <vga>\n");
                tia = (uint16_t)strtol(pszval, NULL, 16);
                vga = (uint16_t)strtol(pszval1, NULL, 16);

                val = ((tia & 0xf) << 12) | ((vga & 0x3) << 10);
                break;
            }

            case PMD_MPD_GAINS:
            {
                float bias_gain, mod_gain;
                int16_t tmp_bias, tmp_mod;

                printf("MPD gains: <bias gain> <modulation gain>\n");
                bias_gain  = (float)atof(pszval);
                mod_gain  = (float)atof(pszval1);
                tmp_bias = (int16_t)(bias_gain * 256);
                tmp_mod = (int16_t)(mod_gain * 256);

                val = tmp_mod << 8 | tmp_bias;
                break;
            }

            case PMD_APD:
            {
                uint16_t type = (uint16_t)val;
                uint16_t en = 0x800;
                uint16_t voltage = (uint16_t)strtol(pszval1, NULL, 16);

                printf("APD: <type> <voltage>\n");
                val = type << 10 | voltage | en;
                break;
            }

            case PMD_LOS_THR:
            {
                uint16_t assert = (uint16_t)strtol(pszval, NULL, 16);
                uint16_t deassert = (uint16_t)strtol(pszval1, NULL, 16);

                printf("LOS threshold: <assert> <deassert>\n");
                val = (deassert & 0xff) << 8 | (assert & 0xff);
                break;
            }

            case PMD_SAT_NEG:
            case PMD_SAT_POS:
            case PMD_ADF_LOS_LEAKY_BUCKET:
            {
                uint16_t high = (uint16_t)strtol(pszval, NULL, 16);
                uint16_t low = (uint16_t)strtol(pszval1, NULL, 16);

                val = (low & 0xff) << 8 | (high & 0xff);
                break;
            }

            case PMD_PREEMPHASIS_WEIGHT:
                printf("Preemphasis weight: <index> <weight>\n");
                /* no break here */
            case PMD_PREEMPHASIS_DELAY:
            {
                if (PMD_PREEMPHASIS_DELAY == param)
                    printf("Preemphasis delay: <index> <delay>\n");
                cal_index = (uint16_t)strtol(pszval, NULL, 16);
                val = (int32_t)strtol(pszval1, NULL, 16);
                break;
            }

            case PMD_ADF_LOS_THRESHOLDS:
            {
                uint16_t high = (uint16_t)strtol(pszval, NULL, 16);
                uint16_t low = (uint16_t)strtol(pszval1, NULL, 16);

                val = (low << 16) | high;
                break;
            }

            default:
                printf("wrong parameters\n");
                cmdLaserHelp("calibrationset");
                return;
        }
    }

    if (!pszval1 && !pszval2)
    {
        switch (param)
        {
            case PMD_RSSI_A:
                printf("RSSI A\n");
                /* no break here */
            case PMD_RSSI_B:
                if (PMD_RSSI_B == param)
                    printf("RSSI B\n");
                /* no break here */
            case PMD_RSSI_C:
            {
                float rssi = (float) atof( pszval );

                if (PMD_RSSI_C == param)
                    printf("RSSI C\n");
                val = rssi * 256;
                break;
            }

            case PMD_TEMP_COFF_H:
            {
                float alph_h = (float) atof( pszval );

                printf("Temperature coefficient H\n");
                if (alph_h >= 8)
                {
                    printf("wrong parameter value\n");
                    return;
                }
                else
                    val = alph_h * 4096;

                break;
            }

            case PMD_TEMP_COFF_L:
            {
                float alph_l  = (float) atof( pszval );

                printf("Temperature coefficient L\n");
                if (alph_l >= 8)
                {
                    printf("wrong parameter value\n");
                    return;
                }
                else
                    val = alph_l * 4096;

                break;
            }

            case PMD_DACRANGE:
            {
                printf("DAC range\n");
                break;
            }
            case PMD_FAQ_LEVEL0_DAC:
                printf("Fast acquisition level 0 DAC\n");
                break;
            case PMD_FAQ_LEVEL1_DAC:
                printf("Fast acquisition level 1 DAC\n");
                break;
            case PMD_BIAS:
                printf("Bias\n");
                break;
            case PMD_MOD:
                printf("Modulation\n");
                break;
            case PMD_APDOI_CTRL:
                printf("APD over current threshold\n");
                break;
            case PMD_TEMP_0:
                printf("Temp 0\n");
                break;
            case PMD_ESC_THR:
                printf("Esc threshold\n");
                break;
            case PMD_ROGUE_THR:
                printf("Rogue threshold\n");
                break;
            case PMD_LEVEL_0_DAC:
                printf("Tracking level 0 DAC\n");
                break;
            case PMD_AVG_LEVEL_1_DAC:
                printf("Tracking average level 1 DAC\n");
                break;
            case PMD_DUTY_CYCLE:
                printf("Duty Cycle\n");
                break;
            case PMD_MPD_CALIBCTRL:
                printf("MPD calibration control\n");
                break;
            case PMD_TX_POWER:
                printf("Tx power\n");
                break;
            case PMD_BIAS0:
                printf("Bias 0\n");
                break;
            case PMD_TEMP_OFFSET:
                printf("Temp offset\n");
                break;
            case PMD_BIAS_DELTA_I:
                printf("Bias delta i\n");
                break;
            case PMD_SLOPE_EFFICIENCY:
                printf("Slope efficiency\n");
                break;
            case PMD_HOST_SW_CONTROL:
                printf("Host software control\n");
                break;
            case PMD_STATISTICS_COLLECTION:
                printf("Statistics Collection\n");
                break;

            default:
                printf("wrong parameters\n");
                cmdLaserHelp("calibrationset");
                return;
        }
    }

    if (devLaser_SetCalParams(param, val, cal_index))
    {
        printf("Cal write error\n");
        cmsLog_error("Cal write error\n");
        return;
    }

    printf("-file write done\n");
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserCalGet
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserCalGet(char *pszparam, char *pszval)
{
    uint16_t cal_index = 0, param;
    int rc;

    if (!pszparam)
    {
        cmdLaserHelp("calibrationget");
        return;
    }

    param = (uint16_t)strtol(pszparam, NULL, 10);

    if (pszval)
        cal_index = (uint16_t)strtol(pszval, NULL, 16);

    rc = devLaser_GetCalParams(param, cal_index);
    switch(rc)
    {
        case PMD_OPERATION_SUCCESS:
            printf("-file read done\n");
            break;
        case ERR_CAL_PARAM_VALUE_INVALID:
            printf("The calibration parameter is invalid\n");
            break;
        case ERR_CAL_PARAM_INDEX_OUT_OF_RANGE:
            printf("The calibration parameter index is out of range\n");
            break;
        case ERR_CAL_PARAM_CANNOT_OPEN_DEVICE:
            printf("Can't open the PMD device\n");
            break;
        default:
            printf("Unknown PMD device error\n");
    }
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserMsgWrite
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserMsgWrite(char *pszmsg_id,  char *pszlen, unsigned char *pszbuf)
{
    unsigned short msg_id, len;

    if (!(pszmsg_id && pszlen && pszbuf))
    {
        cmdLaserHelp("msgset");
        return;
    }

    msg_id = (unsigned short)strtol(pszmsg_id, NULL, 16);

    if (parse_hexa_and_length(pszlen, pszbuf, &len))
    {
        printf("Msg write error: %zu characters were entered whereas %d were expected\n", strlen((const char *)pszbuf),
            2*len);
        cmsLog_error("Msg write error: %zu characters were entered whereas %d were expected\n",
            strlen((const char *)pszbuf), 2*len);
        return;
    }

    if (!devLaser_MsgWrite(msg_id, len, pszbuf))
        printf("-msg write done\n");
    else
    {
        printf("Msg write error\n");
        cmsLog_error("Msg write error\n");
    }
}


/*****************************************************************************
*
*  FUNCTION     :   cmdLaserMsgRead
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdLaserMsgRead(char *pszmsg_id, char *pszlen)
{
    unsigned short msg_id, len;

    if (!(pszmsg_id && pszlen))
    {
        cmdLaserHelp("msgget");
        return;
    }

    msg_id = (unsigned short)strtol(pszmsg_id, NULL, 16);
    len    = (unsigned short)strtol(pszlen, NULL, 10);

    len = len < PMD_BUF_MAX_SIZE ? len : PMD_BUF_MAX_SIZE;

    if (devLaser_MsgRead(msg_id, len))
    {
        printf("Msg read error\n");
        cmsLog_error("Msg read error\n");
    }
    else
        printf("-msg read done\n");
}


static int cli_pmd_save_rssi_op_couples(float rssi, float op)
{
    static float rssi_op_couple[2][2] = {};
    static uint16_t index = 0;
    float a,b;
    int32_t i, tmp_a, tmp_b;
    CmsRet Ret = CMSRET_SUCCESS;

    i = index % 2;

    rssi_op_couple[i][0] = rssi;
    rssi_op_couple[i][1] = op;

    printf (" rssi %f  op %f\n", rssi, op);

    if (index >= 1)
    {
        a = (rssi_op_couple[1][1] - rssi_op_couple[0][1]) / (rssi_op_couple[1][0] - rssi_op_couple[0][0]);
        b = rssi_op_couple[0][1] - a * rssi_op_couple[0][0];

        if (a > 1 || b > 1)
        {
            printf("rssi op couple are unreasonable\n");
            return CMSRET_INVALID_ARGUMENTS;
        }

        tmp_a = (int32_t)(a * 256);
        tmp_b = (int32_t)(b * 256);

        printf("\n a %f b %f\n",a ,b);

        if (devLaser_MsgWrite(PMD_RSSI_A_FACTOR_CAL_SET_MSG, 4, (unsigned char *)&tmp_a))
            Ret = CMSRET_INVALID_ARGUMENTS;
        else if (devLaser_MsgWrite(PMD_RSSI_B_FACTOR_CAL_SET_MSG, 4, (unsigned char *)&tmp_b))
            Ret = CMSRET_INVALID_ARGUMENTS;

        if (devLaser_SetCalParams(PMD_RSSI_A, tmp_a, 0))
            Ret = CMSRET_INVALID_ARGUMENTS;
        else if (devLaser_SetCalParams(PMD_RSSI_B, tmp_b, 0))
            Ret = CMSRET_INVALID_ARGUMENTS;
    }
    index++;

    return Ret;
}


/*****************************************************************************
*
*  FUNCTION     :   cmdRssiCalSet
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*  RETURNS      :
*
*****************************************************************************/
static void cmdRssiCalSet(char *pszrssi,  char *pszop)
{
    float rssi;
    float op;

    if (!(pszrssi && pszop))
    {
        cmdLaserHelp("rssi");
        return;
    }

    rssi  = (float) atof( pszrssi );
    op    = (float) atof( pszop );

    if (!cli_pmd_save_rssi_op_couples(rssi, op))
        printf("-RSSI Calibration write done\n");
    else
    {
        printf("RSSI calibration error\n");
        cmsLog_error("RSSI calibration error\n");
    }
}


/*****************************************************************************
*  FUNCTION     :   devLaser_TempToApdWrite
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
#if defined(PMD_JSON_LIB)
extern int pmd_calibration_apd_temperature_table_to_json_file(const uint16_t temp2apd_table[APD_TEMP_TABLE_SIZE]);
#endif
static int devLaser_TempToApdWrite(uint16_t len, unsigned char *buf)
{
    int ret;
    pmd_params param = { .len = len, .buf = buf};
    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (0 > LaserDevFd)
    {
        return ERR_CAL_PARAM_CANNOT_OPEN_DEVICE;
    }
#if defined(PMD_JSON_LIB)
    ret = pmd_calibration_apd_temperature_table_to_json_file((const uint16_t*)buf);
    if (ret)
    {
        return ret;
    }
#endif
    ret = ioctl(LaserDevFd, PMD_IOCTL_TEMP2APD_WRITE, &param);
    close(LaserDevFd);

    return ret;
}


static void cmdLaserTempToApdSet(char *ApdList)
{
    uint16_t pmdTempToApd[APD_TEMP_TABLE_SIZE] = {0};
    int index = 0;
    char *apd;

    apd = strtok(ApdList, " ,;");
    while (apd && (index < APD_TEMP_TABLE_SIZE))
    {
        uint16_t apd16bit;
        
        apd16bit = (uint16_t)atoi(apd);
#if defined(PMD_JSON_LIB)
#if __BYTE_ORDER == __LITTLE_ENDIAN
        /* keep compatibility to the deprecated kernel pmd_temp2apd_file_op in pmd_op.c */
        apd16bit = ((apd16bit & 0x00ffU) << 8) | ((apd16bit & 0xff00U) >> 8);
#endif
#endif
        pmdTempToApd[index] = apd16bit;
        apd = strtok(NULL, " ,;");
        index++;
    }

    if (index < APD_TEMP_TABLE_SIZE)
    {
        printf("missing %d entries\n", APD_TEMP_TABLE_SIZE - index);
        return;
    }

    if (!devLaser_TempToApdWrite(sizeof(pmdTempToApd), (unsigned char*)pmdTempToApd))
        printf("temp2apd set done\n");
    else
        printf("temp2apd set error\n");
}


/*****************************************************************************
*  FUNCTION     :   devLaser_ResToTempWrite
*               :
*  DESCRIPTION  :
*               :
*               :
*  PARAMETERS   :
*               :
*  RETURNS      :   0 on success
*****************************************************************************/
#if defined(PMD_JSON_LIB)
extern int pmd_calibration_resistance_temperature_table_to_json_file(const uint32_t res2temp[TEMP_TABLE_SIZE]);
#endif
static int devLaser_ResToTempWrite(uint16_t len, unsigned char *buf)
{
    int ret;
    pmd_params param = { .len = len, .buf = buf};
    int LaserDevFd = open(LASER_DEV, O_RDWR);

    if (0 > LaserDevFd)
    {
        return ERR_CAL_PARAM_CANNOT_OPEN_DEVICE;
    }
#if defined(PMD_JSON_LIB)
    ret = pmd_calibration_resistance_temperature_table_to_json_file((const uint32_t*)buf);
    if (ret)
    {
        return ret;
    }
#endif
    ret = ioctl(LaserDevFd, PMD_IOCTL_RES2TEMP_WRITE, &param);
    close(LaserDevFd);

    return ret;
}


static void cmdLaserResToTempSet(char *ResList)
{
    uint32_t pmdResToTemp[TEMP_TABLE_SIZE] = {0};
    int index = 0;
    char *res;

    res = strtok (ResList, " ,;");
    while (res && (index < TEMP_TABLE_SIZE))
    {
        pmdResToTemp[index] = (uint32_t)atoi(res);
        res = strtok(NULL, " ,;");
        index++;
    }

    if (index < TEMP_TABLE_SIZE)
    {
        printf("missing %d entries\n", TEMP_TABLE_SIZE - index);
        return;
    }
    
    if (!devLaser_ResToTempWrite(sizeof(pmdResToTemp), (unsigned char*)pmdResToTemp))
      printf("res2temp set done\n");
    else
      printf("res2temp set error\n");
}


#endif /* DESKTOP_LINUX */
#endif /* SUPPORT_CLI_CMD */
