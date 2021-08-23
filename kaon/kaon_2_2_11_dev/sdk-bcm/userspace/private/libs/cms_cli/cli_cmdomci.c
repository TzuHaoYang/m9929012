/***********************************************************************
 *
 *  Copyright (c) 2007 Broadcom
 *  All Rights Reserved
 *
 *  <:label-BRCM:2011:proprietary:standard
 *
 *   This program is the proprietary software of Broadcom and/or its
 *   licensors, and may only be used, duplicated, modified or distributed pursuant
 *   to the terms and conditions of a separate, written license agreement executed
 *   between you and Broadcom (an "Authorized License").  Except as set forth in
 *   an Authorized License, Broadcom grants no license (express or implied), right
 *   to use, or waiver of any kind with respect to the Software, and Broadcom
 *   expressly reserves all rights in and to the Software and all intellectual
 *   property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 *   NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 *   BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
 *
 *   Except as expressly set forth in the Authorized License,
 *
 *   1. This program, including its structure, sequence and organization,
 *      constitutes the valuable trade secrets of Broadcom, and you shall use
 *      all reasonable efforts to protect the confidentiality thereof, and to
 *      use this information only in connection with your use of Broadcom
 *      integrated circuit products.
 *
 *   2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
 *      AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
 *      WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 *      RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
 *      ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
 *      FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
 *      COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
 *      TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
 *      PERFORMANCE OF THE SOFTWARE.
 *
 *   3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *      ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 *      INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
 *      WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 *      IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
 *      OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *      SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
 *      SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
 *      LIMITED REMEDY.
 *  :>
 *
 ************************************************************************/

/*****************************************************************************
*    Description:
*
*      OMCI CLI command implementation.
*
*****************************************************************************/

#include "number_defs.h"
UINT32  cli_gpon_tcont_max;

#ifdef SUPPORT_CLI_CMD

#ifdef DMP_X_ITU_ORG_GPON_1 /* aka SUPPORT_OMCI */

/* ---- Include Files ----------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "cms.h"
#include "cms_log.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_msg.h"

#include "cli.h"
#include "omci_api.h"
#include "omciutl_cmn.h"
#include "omcicust_defs.h"
#include "omcicfg_defs.h"
#ifdef BUILD_BCMIPC
#include "omci_ipc.h"
#endif
#include "ponsys_api.h"


/* ---- Private Constants and Types --------------------------------------- */

#define CLIAPP_LOCK_TIMEOUT 6000 /* ms */

#define MSG_RECV_TIMEOUT_MS 60000

#define MAX_OPTS 12

#define GRAB_END_STR "[GRAB:END]\n"

#define GRAB_LINE_SIZE_MAX 1024

#define SEND_FILE_OVHD 64

#define ETH_PORT_TYPE_RG     "rg"
#define ETH_PORT_TYPE_ONT    "ont"
#define ETH_PORT_TYPE_RG_ONT "rgont"

#define GPON_TCONT_ALL 0xffff

/* #define OMCI_DEBUG_CMD */


/* ---- Macro API definitions --------------------------------------------- */


/* ---- Private Function Prototypes --------------------------------------- */

static void cmdOmciPmHandler(SINT32 argc, char *argv[]);


/* ---- Private Variables ------------------------------------------------- */

static const char omciusage[] = "\nUsage:\n";

static const char omciHelpSummary[] =
"Usage: omci <command> or omci <command> --help\n\n"
"OMCI MIB or behavior configuration:\n"
"        omci auth          OMCI authentication settings\n"
"        omci bridge        OMCI bridge setting\n"
"        omci eth           OMCI MIB ETH port mode (sfu|rg) and instances setting\n"
"        omci mcast         OMCI multicast setting\n"
"        omci mibsync       OMCI MIB data sync counter\n"
"        omci misc          OMCI miscellaneous settings\n"
"        omci pm            OMCI performance monitoring settings\n"
"        omci promiscmode   OMCI response result code option\n"
"        omci tcont         OMCI MIB T-CONT ME instances setting\n"
"        omci tmoption      Traffic management configuration options\n"
#if defined(DMP_X_ITU_ORG_VOICE_1)
"        omci voice         OMCI MIB voice model options\n"
#endif /* DMP_X_ITU_ORG_VOICE_1 */
"        omci unipathmode   OMCI data model parsing option\n"
"\n"
"OMCI troubleshooting:\n"
"        omci capture       OMCI message capture and playback function\n"
"        omci ddi           Developer debug interface\n"
"        omci debug         OMCI debug options\n"
"        omci grab          Uploading a capture file by sending it to the console\n"
"        omci rawmode       Accepting and relaying OMCI raw messages via WEB UI\n"
"        omci send          Sending the content of a binary file to OMCID\n"
"        omci settable      Sending 'Set table' command to OMCID\n"
"        omci test          OMCI testing functions\n";

static const char omcigrab[] =
"        omci grab --filetype <txt|bin> --filename <full_dest_file_name>\n";

static const char omcicapture[] =
"        omci capture control --state <on [--overwrite <y|n>] | off>\n\
        omci capture replay --state <on [--filename <full_src_file_name>] | off>\n\
        omci capture view [--filename <full_src_file_name>] \n\
        omci capture save [--filename <partial_dest_file_name>]\n\
        omci capture restore [--filename <partial_src_file_name>]\n";

static const char omcisend[] =
"        omci send --filename <file_name>\n";

static const char omcitcontfmt[] =
"        omci tcont --portmax <0..%d> --startid <0..65535>\n"
"        omci tcont --tcontid <0..%d|all> --policy <sp|wrr> [--maxqnum <0..%d>\n"
"                   --maxspqnum <0..%d>]\n"
"        omci tcont --schedulernum <0..%d>\n"
"                     number of schedulers between PQ and T-CONT\n"
"        omci tcont --qinit <on|off>\n"
"                     on: init queues when the T-CONT is set\n"
"                     off: init queue when the queue is used by a GEM port\n"
"        omci tcont --allocidInit <255|65535>\n"
"        omci tcont --status\n";

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
static const char omciethhgufmt[] =
"        omci eth --startid <0..65535> --veip <0|1>\n"
"        omci eth --port <0..%d> --type <rg|ont|rgont>\n";
#else
static const char omciethsfufmt[] =
"        omci eth --flexid --mode <on|off>\n"
"        omci eth --flexid --portid <0..%d> --meid <0..65535>\n";
#endif   /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */

static const char omciethfmt[] =
"        omci eth --portmax <0..%d>\n"
"        omci eth --slotid --fe <0..255> --ge <0..255> --xge <0..255>\n"
"        omci eth --show\n";

static const char omcimibsync[] =
"        omci mibsync --help\n\
        omci mibsync --get\n";

static const char omciunipathmode[] =
"        omci unipathmode --help\n\
        omci unipathmode --status\n\
        omci unipathmode --mode <on|off>\n";

static const char omcipromiscmode[] =
"        omci promiscmode --help\n\
        omci promiscmode --status\n\
        omci promiscmode --mode <on|off>\n";

static const char omcirawmode[] =
"        omci rawmode --help\n\
        omci rawmode --status\n\
        omci rawmode --mode <on|off>\n";

static const char omcitmoption[] =
"        omci tmoption --help\n"
"        omci tmoption --status\n"
"        omci tmoption --usmode <0(priority)|1(rate)|2(priority and rate)>\n"
"        omci tmoption --dsqueue <0(skip)|1(pbit)|2(extended pbit)>\n"
"        omci tmoption --flexmode <val>\n"
"                        val: QoS configuration flexibility in ONU2-G ME\n";

static const char omcibridge[] =
"        omci bridge --help\n\
        omci bridge --status\n\
        omci bridge --fwdmask <0..65535>\n";

static const char omciauth[] =
"        omci auth --help\n"
"        omci auth --status\n"
"        omci auth --psk <secret(32 chars)>\n"
"        omci auth --cmac --m <msg(n*2 chars)>\n"
"        omci auth --msk --clear\n";

static const char omcimisc[] =
"        omci misc --help\n"
"        omci misc --status\n"
"        omci misc --extvlandefault <0(disable)|1(us)|2(ds)|3(both)>\n"
"        omci misc --omccversion <hex> (default 0)\n"
"        omci misc --extmsg <on|off>\n"
"                    Enable/disble extended message set\n";

static const char omcidebug[] =
"        omci debug --help\n"
"        omci debug --status\n"
"        omci debug --dump <class ID>\n"
"        omci debug --mkerr <swdlerr1(Window hole error)|swdlerr2(Download \n"
"                     section msg rsp error)|swdlerr3(SW image CRC error)>\n"
#if defined(DMP_X_ITU_ORG_VOICE_1)
"        omci debug --module <all|omci|model|vlan|mib|flow|rule|mcast|voice|file>\n"
"                   --state <on|off>\n"
#else    // DMP_X_ITU_ORG_VOICE_1
"        omci debug --module <all|omci|model|vlan|mib|flow|rule|mcast|file>\n"
"                   --state <on|off>\n"
#endif   // DMP_X_ITU_ORG_VOICE_1
"        omci debug --info <0(omci mib)|1(internal data)>\n";

#if defined(DMP_X_ITU_ORG_VOICE_1)
static const char omcivoice[] =
"        omci voice --help\n\
        omci voice --status\n\
        omci voice --model <0(omci path)|1(ip path)\n";
#endif   // DMP_X_ITU_ORG_VOICE_1

static const char omcipm[] =
"        omci pm --help\n"
"        omci pm --getAlarmSeq\n"
"        omci pm --setAlarmSeq <1..255>\n";

static const char omcimcast[] =
"        omci mcast --help\n\
        omci mcast --status\n\
        omci mcast --hostctrl <on|off>\n\
        omci mcast --joinfwd <on|off>\n";

static const char omcitest[] =
"        omci test --genalarm --meclass <class ID> --meid <meid>\n"
"                  --num <alarmnum> --state <0|1>\n"
"        omci test --genavc --meclass <class ID> --meid <meid>\n"
"                  --attrmask <attrmask>\n";

static const char omcisettable[] =
"        omci settable --tcid <tcid> --meclass <class ID> --meid <meid>\n"
"                      --index <0-based attrindex> --val <entry>\n";

static const char omciddi[] =
"        omci ddi funcname arg0 0xarg1 \"arg2\" ...\n";

#ifdef OMCI_DEBUG_CMD
static const char omcimedebug[] =
"        omci medebug --getme <oid> <meid>\n"
"                     --getnextme <oid>\n"
"                     --setme <oid> <meid> <index> <int|str>\n";
#endif /* OMCI_DEBUG_CMD */


/* ---- Functions --------------------------------------------------------- */

/***************************************************************************
 * Function Name: cmdOmciTcontGenHelp
 * Description  : Generate and print help information about the T-CONT
 ***************************************************************************/
static void cmdOmciTcontGenHelp(void)
{
    char tcontHelp[BUFLEN_1024];
    UINT32 tcontIdMax = cli_gpon_tcont_max - 1;

    sprintf(tcontHelp, omcitcontfmt, cli_gpon_tcont_max, tcontIdMax,
      GPON_PHY_US_PQ_MAX, GPON_PHY_US_PQ_MAX, GPON_PHY_US_TS_MAX);
    printf("%s%s", omciusage, tcontHelp);
}

/***************************************************************************
 * Function Name: cmdOmciEthGenHelp
 * Description  : Generate and print help information about ETH ports.
 ***************************************************************************/
static void cmdOmciEthGenHelp(void)
{
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
    char ethHguHelp[BUFLEN_1024];
    char ethHelp[BUFLEN_1024];

    sprintf(ethHguHelp, omciethhgufmt, GPON_HGU_ETH_PORT_MAX - 1);
    sprintf(ethHelp, omciethfmt, GPON_HGU_ETH_PORT_MAX);
    printf("%s%s%s", omciusage, ethHguHelp, ethHelp);
#else
    char ethSfuHelp[BUFLEN_1024];
    char ethHelp[BUFLEN_1024];

    sprintf(ethSfuHelp, omciethsfufmt, GPON_PHY_ETH_PORT_MAX - 1);
    sprintf(ethHelp, omciethfmt, GPON_PHY_ETH_PORT_MAX);
    printf("%s%s%s", omciusage, ethSfuHelp, ethHelp);
#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */
}

/***************************************************************************
 * Function Name: cmdOmciHelp
 * Description  : Prints help information about the OMCI commands
 ***************************************************************************/
static void cmdOmciHelp(char *help)
{
    if (help == NULL || strcasecmp(help, "--help") == 0)
    {
        printf("%s",  omciHelpSummary);
    }
    else if (strcasecmp(help, "auth") == 0)
    {
        printf("%s%s", omciusage, omciauth);
    }
    else if (strcasecmp(help, "bridge") == 0)
    {
        printf("%s%s", omciusage, omcibridge);
    }
    else if (strcasecmp(help, "capture") == 0)
    {
        printf("%s%s", omciusage, omcicapture);
    }
    else if (strcasecmp(help, "ddi") == 0)
    {
        printf("%s%s", omciusage, omciddi);
    }
    else if (strcasecmp(help, "debug") == 0)
    {
        printf("%s%s", omciusage, omcidebug);
    }
    else if (strcasecmp(help, "eth") == 0)
    {
        cmdOmciEthGenHelp();
    }
    else if (strcasecmp(help, "grab") == 0)
    {
        printf("%s%s", omciusage, omcigrab);
    }
    else if (strcasecmp(help, "mcast") == 0)
    {
        printf("%s%s", omciusage, omcimcast);
    }
#ifdef OMCI_DEBUG_CMD
    else if (strcasecmp(help, "medebug") == 0)
    {
        printf("%s%s", omciusage, omcimedebug);
    }
#endif /* OMCI_DEBUG_CMD */
    else if (strcasecmp(help, "mibsync") == 0)
    {
        printf("%s%s", omciusage, omcimibsync);
    }
    else if (strcasecmp(help, "misc") == 0)
    {
        printf("%s%s", omciusage, omcimisc);
    }
    else if (strcasecmp(help, "pm") == 0)
    {
        printf("%s%s", omciusage, omcipm);
    }
    else if (strcasecmp(help, "promiscmode") == 0)
    {
        printf("%s%s", omciusage, omcipromiscmode);
    }
    else if (strcasecmp(help, "rawmode") == 0)
    {
        printf("%s%s", omciusage, omcirawmode);
    }
    else if (strcasecmp(help, "send") == 0)
    {
        printf("%s%s", omciusage, omcisend);
    }
    else if (strcasecmp(help, "settable") == 0)
    {
        printf("%s%s", omciusage, omcisettable);
    }
    else if (strcasecmp(help, "tcont") == 0)
    {
        cmdOmciTcontGenHelp();
    }
    else if (strcasecmp(help, "test") == 0)
    {
        printf("%s%s", omciusage, omcitest);
    }
    else if (strcasecmp(help, "tmoption") == 0)
    {
        printf("%s%s", omciusage, omcitmoption);
    }
    else if (strcasecmp(help, "unipathmode") == 0)
    {
        printf("%s%s", omciusage, omciunipathmode);
    }
#if defined(DMP_X_ITU_ORG_VOICE_1)
    else if (strcasecmp(help, "voice") == 0)
    {
        printf("%s%s", omciusage, omcivoice);
    }
#endif
}

/***************************************************************************
 * Function Name: cmdOmciCaptureControl
 *
 * Description  : Processes the OMCI msg capture state command from the CLI
                  that controls the capturing of OMCI msgs from the OLT.

                  Capture can only be performed when other features that
                  access the internal capture file are not active.  This
                  includes playback, display, download and upload.
 ***************************************************************************/
static CmsRet cmdOmciCaptureControl(char *pStateOpt, char *pStateArg,
  char *pOvrOpt, char *pOvrArg)
{
    CmsMsgHeader MsgHdr;
    CmsRet Ret = CMSRET_SUCCESS;

    MsgHdr.dst                  = EID_OMCID;
    MsgHdr.src                  = EID_CONSOLED;
    MsgHdr.dataLength           = 0;
    MsgHdr.flags.all            = 0;
    MsgHdr.flags.bits.request   = TRUE;
    MsgHdr.next                 = 0;
    MsgHdr.sequenceNumber       = 0;
    MsgHdr.wordData             = 0;

    if ((NULL == pStateOpt) || (NULL == pStateArg))
    {
        // Invalid argument
        cmsLog_error("omci capture control: Invalid argument\n");
        cmdOmciHelp("capture");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (strcasecmp(pStateOpt, "--state") != 0)
    {
        // Missing argument
        cmsLog_error("omci capture control: Missing argument\n");
        cmdOmciHelp("capture");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (strcasecmp(pStateArg, "on") == 0)
    {
        // send capture ON msg via CMS
        MsgHdr.type = CMS_MSG_OMCI_CAPTURE_STATE_ON;

        if (NULL != pOvrOpt)
        {
            if ((NULL == pOvrArg) || (strcasecmp(pOvrOpt, "--overwrite") != 0))
            {
                // Invalid argument
                cmsLog_error("omci capture control: Invalid argument\n");
                cmdOmciHelp("capture");
                return CMSRET_INVALID_ARGUMENTS;
            }

            if (strcasecmp(pOvrArg, "y") == 0)
            {
                MsgHdr.wordData = TRUE;
            }
            else if (strcasecmp(pOvrArg, "n") == 0)
            {
                MsgHdr.wordData = FALSE;
            }
            else
            {
                // Invalid argument
                cmsLog_error("omci capture control: Invalid argument\n");
                cmdOmciHelp("capture");
                Ret = CMSRET_INVALID_ARGUMENTS;
            }
        }
    }
    else if (strcasecmp(pStateArg, "off") == 0)
    {
        // send capture OFF msg via CMS
        MsgHdr.type = CMS_MSG_OMCI_CAPTURE_STATE_OFF;
    }
    else
    {
        // Invalid argument
        cmsLog_error("omci capture control: Invalid argument\n");
        cmdOmciHelp("capture");
        Ret = CMSRET_INVALID_ARGUMENTS;
    }

    // send msg if processed valid
    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, &MsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: cmdOmciCaptureReplay
 *
 * Description  : Processes the OMCI msg playback command from the CLI that
                  controls the playback of OMCI msgs from a file.

                  Playback can only be performed when other features that
                  access the internal capture file are not active.  This
                  includes Capture, display, download and upload.
 ***************************************************************************/
static CmsRet cmdOmciCaptureReplay(char *pStateOpt, char *pStateArg,
  char *pFilenameOpt, char *pFilenameArg)
{
    CmsRet Ret = CMSRET_SUCCESS;
    UINT16 MsgSize = sizeof(CmsMsgHeader) + 128;
    char Buf[MsgSize];
    char *pData = Buf + sizeof(CmsMsgHeader);
    CmsMsgHeader *pMsgHdr = (CmsMsgHeader *)Buf;
    static FILE *pFileHandle = NULL;

    memset(Buf, 0x0, MsgSize);
    pMsgHdr->dst                    = EID_OMCID;
    pMsgHdr->src                    = EID_CONSOLED;
    pMsgHdr->dataLength             = 128;
    pMsgHdr->flags.all              = 0;
    pMsgHdr->flags.bits.request     = TRUE;
    pMsgHdr->next                   = 0;
    pMsgHdr->sequenceNumber         = 0;
    pMsgHdr->wordData               = 0;

    if ((NULL == pStateOpt) || (NULL == pStateArg))
    {
        // Invalid argument
        cmsLog_error("omci capture replay: Invalid argument\n");
        cmdOmciHelp("capture");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (strcasecmp(pStateOpt, "--state") != 0)
    {
        // Missing argument
        cmsLog_error("omci capture replay: Missing argument\n");
        cmdOmciHelp("capture");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (strcasecmp(pStateArg, "on") == 0)
    {
        // send capture replay ON msg via CMS
        pMsgHdr->type = CMS_MSG_OMCI_CAPTURE_REPLAY_ON;

        // check for user provided file name
        if (NULL != pFilenameOpt)
        {
            if ((strcasecmp(pFilenameOpt, "--filename") != 0) ||
              (NULL == pFilenameArg))
            {
                // Invalid argument
                cmsLog_error("omci capture replay: Invalid filename argument\n");
                cmdOmciHelp("capture");
                return CMSRET_INVALID_ARGUMENTS;
            }

            if (strlen(pFilenameArg) > 128)//check string length
            {
                printf("omci capture replay: Invalid filename argument %s too long\n",
                  pFilenameArg);
                return CMSRET_INVALID_ARGUMENTS;
            }

            pFileHandle = fopen(pFilenameArg, "r");

            // file is valid if we opened it so go ahead and process it
            if (NULL != pFileHandle)
            {
                pMsgHdr->wordData = TRUE;
                fclose(pFileHandle);
                strcat(pData, pFilenameArg);
            }
            else
            {
                printf("omci capture replay: can't open file %s\n", pFilenameArg);
                Ret = CMSRET_INVALID_ARGUMENTS;
            }
        }
    }
    else if (strcasecmp(pStateArg, "off") == 0)
    {
        // send capture replay OFF msg via CMS
        pMsgHdr->type = CMS_MSG_OMCI_CAPTURE_REPLAY_OFF;
        pFileHandle = NULL;
    }
    else
    {
        // Invalid argument
        cmsLog_error("omci capture replay: Invalid argument\n");
        cmdOmciHelp("capture");
        Ret = CMSRET_INVALID_ARGUMENTS;
        pFileHandle = NULL;
    }

    // send msg if processed valid
    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, pMsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: cmdOmciCaptureView
 *
 * Description  : Processes the OMCI msg show command from the CLI that
                  controls the display of OMCI msgs from a file.

                  Display can only be performed when other features that
                  access the internal capture file are not active.  This
                  includes Capture, Playback, download and upload.

                  this command supports an optional filename option and arg
                  that allows the user to view from a filename different from
                  the internal file.
 ***************************************************************************/
static CmsRet cmdOmciCaptureView(char *pFilenameOpt, char *pFilenameArg)
{
    CmsRet Ret = CMSRET_SUCCESS;
    UINT16 MsgSize = sizeof(CmsMsgHeader) + 128;
    char Buf[MsgSize];
    char *pData = Buf + sizeof(CmsMsgHeader);
    CmsMsgHeader *pMsgHdr = (CmsMsgHeader *)Buf;
    static FILE *pFileHandle = NULL;

    memset(Buf, 0x0, MsgSize);
    pMsgHdr->dst                    = EID_OMCID;
    pMsgHdr->src                    = EID_CONSOLED;
    pMsgHdr->dataLength             = 128;
    pMsgHdr->flags.all              = 0;
    pMsgHdr->flags.bits.request     = TRUE;
    pMsgHdr->next                   = 0;
    pMsgHdr->sequenceNumber         = 0;
    pMsgHdr->wordData               = 0;

    // send capture ON msg via CMS
    pMsgHdr->type = CMS_MSG_OMCI_CAPTURE_VIEW;

    if (NULL != pFilenameOpt)
    {
        if ((strcasecmp(pFilenameOpt, "--filename") != 0) ||
          (NULL == pFilenameArg))
        {
            // Invalid argument
            printf("omci capture view: Invalid argument\n");
            cmdOmciHelp("capture");
            return CMSRET_INVALID_ARGUMENTS;
        }

        if (strlen(pFilenameArg) > 128)//check string length
        {
            printf("omci capture view: Invalid filename argument %s too long\n",
              pFilenameArg);
            return CMSRET_INVALID_ARGUMENTS;
        }

        pFileHandle = fopen(pFilenameArg, "r");

        // file is valid if we opened it so go ahead and process it
        if (NULL != pFileHandle)
        {
            pMsgHdr->wordData = TRUE;
            fclose(pFileHandle);
            strcat(pData, pFilenameArg);
        }
        else
        {
            printf("omci capture view: can't open file %s\n", pFilenameArg);
            Ret = CMSRET_INVALID_ARGUMENTS;
        }
    }

    // send msg if processed valid
    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, pMsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: cmdOmciCaptureSave
 *
 * Description  : Processes the OMCI download command from the CLI that
                  controls the downloading of the OMCI msg capture file from
                  the host.

                 Save can only be performed when other features that
                 access the internal capture file are not active.  This
                 includes Capture, Playback, display and upload.
 ***************************************************************************/
static CmsRet cmdOmciCaptureSave(char *pFilenameOpt, char * pFilenameArg)
{
    CmsRet Ret = CMSRET_SUCCESS;
    UINT16 MsgSize = sizeof(CmsMsgHeader) + 128;
    char Buf[MsgSize];
    char *pData = Buf + sizeof(CmsMsgHeader);
    CmsMsgHeader *pMsgHdr = (CmsMsgHeader *)Buf;

    memset(Buf, 0x0, MsgSize);
    pMsgHdr->dst                 = EID_OMCID;
    pMsgHdr->src                 = EID_CONSOLED;
    pMsgHdr->dataLength          = 128;
    pMsgHdr->flags.all           = 0;
    pMsgHdr->flags.bits.request  = TRUE;
    pMsgHdr->next                = 0;
    pMsgHdr->sequenceNumber      = 0;
    pMsgHdr->wordData            = 0;

    // send download msg via CMS
    pMsgHdr->type = CMS_MSG_OMCI_CAPTURE_DOWNLOAD;

    if (NULL != pFilenameOpt)
    {
        if (strcasecmp(pFilenameOpt, "--filename") == 0)
        {
            if (NULL != pFilenameArg)
            {
                pMsgHdr->wordData = TRUE;
                strcat(pData, pFilenameArg);
            }
            else
            {
                // Missing argument
                cmsLog_error("omci capture save: Missing filename argument\n");
                Ret = CMSRET_INVALID_ARGUMENTS;
                cmdOmciHelp("capture");
            }
        }
        else
        {
            // Invalid argument
            cmsLog_error("omci capture save: Invalid argument\n");
            cmdOmciHelp("capture");
            Ret = CMSRET_INVALID_ARGUMENTS;
        }
    }

    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, pMsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: cmdOmciRestore
 *
 * Description  : Processes the OMCI upload command from the CLI that
                  controls the downloading of the OMCI msg capture file from
                  the host.

                  Restore can only be performed when other features that
                  access the internal capture file are not active.  This
                  includes Capture, Playback, display and Save.
 ***************************************************************************/
static CmsRet cmdOmciCaptureRestore(char *pFilenameOpt, char * pFilenameArg)
{
    CmsRet Ret = CMSRET_SUCCESS;
    UINT16 MsgSize = sizeof(CmsMsgHeader) + 128;
    char Buf[MsgSize];
    char *pData = Buf + sizeof(CmsMsgHeader);
    CmsMsgHeader *pMsgHdr = (CmsMsgHeader *)Buf;

    memset(Buf, 0x0, MsgSize);
    pMsgHdr->dst                    = EID_OMCID;
    pMsgHdr->src                    = EID_CONSOLED;
    pMsgHdr->dataLength             = 128;
    pMsgHdr->flags.all              = 0;
    pMsgHdr->flags.bits.request     = TRUE;
    pMsgHdr->next                   = 0;
    pMsgHdr->sequenceNumber         = 0;
    pMsgHdr->wordData               = 0;

    // send upload msg via CMS
    pMsgHdr->type = CMS_MSG_OMCI_CAPTURE_UPLOAD;

    if (NULL != pFilenameOpt)
    {
        if (strcasecmp(pFilenameOpt, "--filename") == 0)
        {
            if (NULL != pFilenameArg)
            {
                pMsgHdr->wordData = TRUE;
                strcat(pData, pFilenameArg);
            }
            else
            {
                // Missing argument
                cmsLog_error("omci capture restore: Missing filename argument\n");
                Ret = CMSRET_INVALID_ARGUMENTS;
                cmdOmciHelp("capture");
            }
        }
        else
        {
            // Invalid argument
            cmsLog_error("omci capture restore: Invalid argument\n");
            cmdOmciHelp("capture");
            Ret = CMSRET_INVALID_ARGUMENTS;
        }
    }

    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, pMsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: cmdOmciGrab
 * Description  : Grabs the contents of a binary file from CONSOLE Daemon
 ***************************************************************************/
static CmsRet cmdOmciGrab(char *pFileTypeOpt, char *pFileType,
  char *pFileNameOpt,char *pFileName)
{
    CmsRet ret = CMSRET_SUCCESS;

    if ((NULL == pFileTypeOpt) || (NULL == pFileType) ||
      (NULL == pFileNameOpt) || (NULL == pFileName))
    {
        cmdOmciHelp("grab");
        return CMSRET_INVALID_ARGUMENTS;
    }

    // filename opt and arg apply to both filetype options so eval up front
    if ((NULL == pFileName) || (strcasecmp(pFileNameOpt, "--filename") != 0))
    {
        cmdOmciHelp("grab");
        return CMSRET_INVALID_ARGUMENTS;
    }

    // file options are good so process if file type are good
    if (strcasecmp(pFileType, "txt") == 0)
    {
        FILE *outputFile;
        char *lineBuf;

        outputFile = fopen(pFileName, "w");
        if (outputFile == NULL)
        {
            cmsLog_error("Failed to Open %s: %s\n", pFileName, strerror(errno));
            ret = CMSRET_INVALID_ARGUMENTS;
        }
        else
        {
            lineBuf = cmsMem_alloc(GRAB_LINE_SIZE_MAX, ALLOC_ZEROIZE);

            if (lineBuf != NULL)
            {
                while (TRUE)
                {
                    if ((fgets(lineBuf, GRAB_LINE_SIZE_MAX, stdin) == NULL)
                      && (!feof(stdin)))
                    {
                        cmsLog_error("ERROR: fgets\n");
                        fprintf(outputFile, "ERROR: fgets\n");
                        ret = CMSRET_INTERNAL_ERROR;
                        break;
                    }

                    if (0 == strcmp(GRAB_END_STR, lineBuf))
                    {
                        printf("breaking out\n");
                        break;
                    }

                    if (strlen(lineBuf) == GRAB_LINE_SIZE_MAX-1)
                    {
                        cmsLog_error("ERROR: Line Is Too Long! (%zu)\n",
                          strlen(lineBuf));
                        fprintf(outputFile, "ERROR: Line Is Too Long! (%zu)\n",
                          strlen(lineBuf));
                        ret = CMSRET_INTERNAL_ERROR;
                        break;
                    }

                    if (fputs(lineBuf, outputFile) == EOF)
                    {
                        cmsLog_error("ERROR: fputs\n");
                        ret = CMSRET_INTERNAL_ERROR;
                        break;
                    }
                }

                cmsMem_free(lineBuf);
            }
            else
            {
                cmsLog_error("Could not Allocate Memory");
                fprintf(outputFile, "Failed to Allocate Memory!\n");
                ret = CMSRET_RESOURCE_EXCEEDED;
            }

            fclose(outputFile);
        }
    }
    else if (strcasecmp(pFileType, "bin") == 0)
    {
        int fdWrite = -1;
        UINT8 *buf = NULL;
        UINT16 msgSize = sizeof(CmsMsgHeader) + OMCI_PACKET_A_SIZE;
        UINT32 bufSize = 0;
        // OMCI message is stored in file as hex string that has
        // double size of OMCI message
        UINT32 strSize = (msgSize * 2) + 1;
        char hexStr[strSize];

        if ((fdWrite = open(pFileName,
            O_CREAT | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
        {
            printf("Failed to open '%s' for write: %s\n", pFileName, strerror(errno));
            return CMSRET_INVALID_ARGUMENTS;
        }

        // initialize hexStr
        memset(hexStr, 0, strSize);

        // read the OMCI commands from stdin and write it to fileName
        while (fgets(hexStr, strSize, stdin) != NULL)
        {
            // length of line in OMCI script should be double of msgSize
            // so that after calling cmsUtl_hexStringToBinaryBuf
            // bufSize should be equal msgSize
            if (strlen(hexStr) == (msgSize * 2))
            {
                ret = cmsUtl_hexStringToBinaryBuf(hexStr, &buf, &bufSize);
                if (ret == CMSRET_SUCCESS)
                {
                    if (write(fdWrite, (void *)buf, bufSize) < 0)
                    {
                        CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
                        printf("write() failed: %s\n", strerror(errno));
                        break;
                    }
                }
                CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
            }
            else
            {
                break;
            }

            // read pass newline character
            if (fgets(hexStr, strSize, stdin) == NULL)
            {
                break;
            }
            // initialize hexStr
            memset(hexStr, 0, strSize);
        }

        close(fdWrite);
    }
    else
    {
        ret = CMSRET_INVALID_ARGUMENTS;
        cmdOmciHelp("grab");
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciCapture
 *
 * Description  : Processes the OMCI msg capture command from the CLI that
                  controls the processing of options to the command.

                  Capture can only be performed when other features that
                  access the internal capture file are not active.  This
                  includes playback, display, download and upload.
 ***************************************************************************/
static CmsRet cmdOmciCapture(char *pCmd, char *pOpt1, char *pArg1,
  char *pOpt2, char *pArg2)
{
    CmsRet Ret = CMSRET_SUCCESS;

    if (NULL == pCmd)
    {
        // Missing arguments
        cmsLog_error("omci capture: Missing arguments\n");
        cmdOmciHelp("capture");
    }
    else if (strcasecmp(pCmd, "control") == 0)
    {
        Ret = cmdOmciCaptureControl(pOpt1, pArg1, pOpt2, pArg2);
    }
    else if (strcasecmp(pCmd, "replay") == 0)
    {
        Ret = cmdOmciCaptureReplay(pOpt1, pArg1, pOpt2, pArg2);
    }
    else if (strcasecmp(pCmd, "view") == 0)
    {
        Ret = cmdOmciCaptureView(pOpt1, pArg1);
    }
    else if (strcasecmp(pCmd, "save") == 0)
    {
        Ret = cmdOmciCaptureSave(pOpt1, pArg1);
    }
    else if (strcasecmp(pCmd, "restore") == 0)
    {
        Ret = cmdOmciCaptureRestore(pOpt1, pArg1);
    }
    else
    {
        // Invalid argument
        cmsLog_error("omci capture: Invalid argument\n");
        cmdOmciHelp("capture");
    }

    return Ret;
}

/***************************************************************************
* Function Name: cmdOmciDgbMkErr
*
* Description  : Processes the OMCI dbg mkerr command from the CLI that
*                controls the processing of options to the command.
*
* omci debug --mkerr <swdlerr1(Window hole error)|
* swdlerr2(Download section msg rsp error)|swdlerr3(SW image CRC error)>
*
***************************************************************************/
static CmsRet cmdOmciDgbMkErr(char *pCmd)
{
    CmsRet Ret = CMSRET_SUCCESS;
    CmsMsgHeader MsgHdr;

    MsgHdr.dst                  = EID_OMCID;
    MsgHdr.src                  = EID_CONSOLED;
    MsgHdr.dataLength           = 0;
    MsgHdr.flags.all            = 0;
    MsgHdr.flags.bits.request   = TRUE;
    MsgHdr.next                 = 0;
    MsgHdr.sequenceNumber       = 0;
    MsgHdr.wordData             = 0;

    if (NULL == pCmd)
    {
        // Missing argument
        cmsLog_error("omci dbg mkerr: Missing argument\n");
        cmdOmciHelp("debug");
        Ret = CMSRET_INVALID_ARGUMENTS;
    }
    else if (strcasecmp(pCmd, "swdlerr1") == 0)
    {
        printf("Option is set to 1\n");
        // send msg via CMS
        MsgHdr.type = CMS_MSG_OMCI_DEBUG_MKERR_SWDLERR1;
    }
    else if (strcasecmp(pCmd, "swdlerr2") == 0)
    {
        printf("Option is set to 2\n");
        // send msg via CMS
        MsgHdr.type = CMS_MSG_OMCI_DEBUG_MKERR_SWDLERR2;
    }
    else if (strcasecmp(pCmd, "swdlerr3") == 0)
    {
        printf("Option is set to 3\n");
        // send msg via CMS
        MsgHdr.type = CMS_MSG_OMCI_DEBUG_MKERR_SWDLERR3;
    }
    else
    {
        // Missing argument
        cmsLog_error("omci dbg mkerr: invalid argument\n");
        cmdOmciHelp("debug");
        Ret = CMSRET_INVALID_ARGUMENTS;
    }

    // send msg if processed valid
    if (CMSRET_SUCCESS == Ret)
    {
        Ret = cmsMsg_send(cliPrvtMsgHandle, &MsgHdr);
    }

    return Ret;
}

/***************************************************************************
 * Function Name: getFileSize
 * Description  : Returns the size of a file
 ***************************************************************************/
static int getFileSize(char *fileName)
{
    struct stat fileStatus;

    if (stat(fileName, &fileStatus) != 0)
    {
        return -1;
    }

    return (int)(fileStatus.st_size);
}

/***************************************************************************
 * Function Name: cmdOmciSend
 * Description  : Sends the contents of a binary file to the OMCI Daemon
 ***************************************************************************/
static CmsRet cmdOmciSend(char *pFileNameOpt, char *pFileNameArg)
{
    int fileSize = 0;
    int ch;
    UINT16 i = 0, j = 0;
    UINT16 msgSize = sizeof(CmsMsgHeader) + OMCI_PACKET_A_SIZE;
    char buf[msgSize];
    FILE  *fs = NULL;
    CmsRet ret = CMSRET_SUCCESS;

    if ((NULL == pFileNameOpt) || (NULL == pFileNameArg) ||
      (strcasecmp(pFileNameOpt, "--filename") != 0))
    {
        // Missing argument
        cmsLog_error("omci send: Invalid argument\n");
        cmdOmciHelp("send");
        return ret;
    }

    if ((fs = fopen(pFileNameArg, "rb")) == NULL)
    {
        cmsLog_error("Failed to open %s for read", pFileNameArg);
        return CMSRET_INVALID_ARGUMENTS;
    }

    if ((fileSize = getFileSize(pFileNameArg)) > 0)
    {
        while ((ch = fgetc(fs)) != EOF)
        {
            buf[i++] = ch;
            if (i == msgSize)
            {
                j++;
                if ((ret = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)buf))
                  != CMSRET_SUCCESS)
                {
                    cmsLog_error("cmsMsg_send() failed, ret=%d", ret);
                }
                i = 0;
            }
        }
    }
    else
    {
        cmsLog_error("File %s is empty or could not stat", pFileNameArg);
        ret = CMSRET_INTERNAL_ERROR;
    }

    if (NULL != fs)
    {
        fclose(fs);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontMaxNum
 * Description  : configure maximum number of TConts and the first TCont
 *                managed entity ID.
 ***************************************************************************/
static CmsRet cmdOmciTcontMaxNum(UINT32 tcontNumMax, UINT32 startId)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            obj->numberOfTConts = tcontNumMax;
            obj->tcontManagedEntityId = startId;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontQinit
 * Description  : configure upstream priority queue init options.
 ***************************************************************************/
static CmsRet cmdOmciTcontQinit(UBOOL8 flag)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            obj->queueInit = flag;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontAllocIdInit
 * Description  : configure alloc-id init value.
 ***************************************************************************/
static CmsRet cmdOmciTcontAllocIdInit(UINT16 initValue)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            obj->allocIdInitValue = initValue;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontTrafficSchedulers
 * Description  : enable T-CONT QoS with Traffic Schedulers
 ***************************************************************************/
static CmsRet cmdOmciTcontTrafficSchedulers(UINT32 tsNumPerTcont)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            obj->trafficSchedulers = tsNumPerTcont;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontPolicy
 * Description  : configure scheduling policy of TCont.
 ***************************************************************************/
static CmsRet cmdOmciTcontPolicy(UINT32 tcontId, UINT32 policy, UINT32 maxQnum,
  UINT32 maxSpQnum)
{
    CmsRet ret;
    UINT8 *tcontPoliciesBufP = NULL;
    UINT32 tcontPoliciesBufLen;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;
    char *hexStr = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            ret = cmsUtl_hexStringToBinaryBuf(obj->tcontPolicies,
              &tcontPoliciesBufP, &tcontPoliciesBufLen);
            if (ret != CMSRET_SUCCESS)
            {
               cmsObj_free((void **)&obj);
               cmsLck_releaseLock();
               CMSMEM_FREE_BUF_AND_NULL_PTR(tcontPoliciesBufP);
               cmsLog_error("Could not convert HEXBINARY "
                 "omciSys.tcontPolicies, ret = %d", ret);
               return ret;
            }

            omciSetTcontPolicy(tcontPoliciesBufP, tcontId, policy);
            omciSetTcontPrioQueuesNum(tcontPoliciesBufP, tcontId, maxQnum);
            omciSetTcontSpQueuesNum(tcontPoliciesBufP, tcontId, maxSpQnum);

            if ((ret = cmsUtl_binaryBufToHexString(tcontPoliciesBufP,
              tcontPoliciesBufLen, &hexStr)) == CMSRET_SUCCESS)
            {
                CMSMEM_REPLACE_STRING_FLAGS(obj->tcontPolicies, hexStr, 0);
            }
            else
            {
                cmsLog_error("Could not convert BINARY to HEXBINARY "
                  "omciSys.tcontPolicies, ret = %d", ret);
            }

            CMSMEM_FREE_BUF_AND_NULL_PTR(hexStr);

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();

            cmsMem_free(tcontPoliciesBufP);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontPolicyAll
 * Description  : configure scheduling policy of all T-CONTs.
 ***************************************************************************/
static void cmdOmciTcontPolicyAll(UINT32 policy, UINT32 maxQnum,
  UINT32 maxSpQnum)
{
    UINT32 tcontId;

    for (tcontId = 0; tcontId < cli_gpon_tcont_max; tcontId++)
    {
        cmdOmciTcontPolicy(tcontId, policy, maxQnum, maxSpQnum);
    }
}

/***************************************************************************
 * Function Name: cmdOmciTcontStatus
 * Description  : Display T-CONT configuration status.
 ***************************************************************************/
static CmsRet cmdOmciTcontStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;
    UINT32 tcontIdx = 0;
    UINT32 policy = OMCI_TCONT_POL_POLICY_SP;
    UINT32 maxQnum = 0;
    UINT32 maxSpQnum = 0;
    UINT8 *tcontPoliciesBufP = NULL;
    UINT32 tcontPoliciesBufLen;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) !=
      CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return ret;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void *)&obj)) == CMSRET_SUCCESS)
    {
        printf("\n========= T-CONT config status =========\n\n");
        printf("  Number of T-CONTs: %d\n",
          obj->numberOfTConts);
        printf("  Starting ME id   : 0x%04x\n",
          obj->tcontManagedEntityId);
        printf("  Queue init mode  : %s\n",
          (obj->queueInit)? "on" : "off");
        printf("  AllocId init value  : 0x%x\n",
          obj->allocIdInitValue);
        printf("  Number of schedulers between PQ and T-CONT: %d\n",
          obj->trafficSchedulers);

        ret = cmsUtl_hexStringToBinaryBuf(obj->tcontPolicies,
          &tcontPoliciesBufP, &tcontPoliciesBufLen);
        if (ret == CMSRET_SUCCESS)
        {
            if (tcontPoliciesBufLen != cli_gpon_tcont_max * sizeof(UINT16))
            {
                printf(" T-CONT policy not configured, policy size %d does "
                  "not match T-CONT number %d * 2\n",
                  tcontPoliciesBufLen, obj->numberOfTConts);
            }
            else
            {
                printf("  T-CONT (index, policy, total queue #, SP queue #)\n");
                for (tcontIdx = 0; tcontIdx < cli_gpon_tcont_max; tcontIdx++)
                {
                    policy = omciGetTcontPolicy(tcontPoliciesBufP, tcontIdx);
                    maxQnum = omciGetTcontPrioQueuesNum(tcontPoliciesBufP,
                      tcontIdx);
                    maxSpQnum = omciGetTcontSpQueuesNum(tcontPoliciesBufP,
                      tcontIdx);
                    printf("  (%2d, %3s, %2d, %2d)", tcontIdx,
                      (policy == OMCI_TCONT_POL_POLICY_SP) ? "sp" :
                      (policy == OMCI_TCONT_POL_POLICY_WRR) ? "wrr" : "wfq",
                      maxQnum, maxSpQnum);
                    if (((tcontIdx + 1) % 3) == 0)
                    {
                        printf("\n");
                    }
                }
                printf("\n\n");
            }
        }
        else
        {
            cmsLog_error("Could not convert HEXBINARY "
              "omciSys.tcontPolicies, ret = %d", ret);
        }

        CMSMEM_FREE_BUF_AND_NULL_PTR(tcontPoliciesBufP);
        cmsObj_free((void**)&obj);
    }

    cmsLck_releaseLock();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTcontHandler
 * Description  : T-CONT configuration command handler.
 ***************************************************************************/
void cmdOmciTcontHandler(SINT32 argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("tcont");
    }
    else if (strcasecmp(argv[1], "--portmax") == 0)
    {
        if ((argc != 5) || (argv[2] == NULL) || (argv[3] == NULL) ||
          (argv[4] == NULL))
        {
            cmsLog_error("Tcont: Missing arguments\n");
            cmdOmciHelp("tcont");
        }
        else if (strcasecmp(argv[3], "--startid") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[3]);
            cmdOmciHelp("tcont");
        }
        else
        {
            UINT32 tcontNumMax = strtoul(argv[2], (char **)NULL, 0);
            UINT32 startId = strtoul(argv[4], (char **)NULL, 0);

            if (tcontNumMax > cli_gpon_tcont_max)
            {
                cmsLog_error("Tcont: Invalid argument, tcontNum %d, max %d",
                  tcontNumMax, cli_gpon_tcont_max);
                return;
            }

            if (startId >= INVALID_MEID)
            {
                cmsLog_error("Tcont: Invalid argument, startId %d, max 0x%x",
                  startId, INVALID_MEID);
                return;
            }

            cmdOmciTcontMaxNum(tcontNumMax, startId);
        }
    }
    else if (strcasecmp(argv[1], "--tcontid") == 0)
    {
        UINT32 tcontId = 0;
        UINT32 policy = OMCI_TCONT_POL_POLICY_SP;
        UINT32 maxQnum = 0;
        UINT32 maxSpQnum = 0;

        if ((argc != 5) && (argc != 9))
        {
            cmsLog_error("Tcont: Missing or invalid arguments\n");
            cmdOmciHelp("tcont");
            return;
        }

        /* argc >= 5 */
        if ((argv[2] == NULL) || (argv[3] == NULL) || (argv[4] == NULL))
        {
            cmsLog_error("Tcont: Missing arguments\n");
            cmdOmciHelp("tcont");
            return;
        }
        else if (strcasecmp(argv[3], "--policy") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[3]);
            cmdOmciHelp("tcont");
            return;
        }
        else if (strcasecmp(argv[4], "sp") != 0 &&
          strcasecmp(argv[4], "wrr") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[4]);
            cmdOmciHelp("tcont");
            return;
        }

        if (strcasecmp(argv[2], "all") == 0)
        {
            tcontId = GPON_TCONT_ALL;
        }
        else
        {
            tcontId = strtoul(argv[2], (char **)NULL, 0);
            if (tcontId >= cli_gpon_tcont_max)
            {
                cmsLog_error("Tcont: Invalid argument, tcontIdx %d, max %d",
                  tcontId, cli_gpon_tcont_max - 1);
                 return;
            }
        }

        if (strcasecmp(argv[4], "wrr") == 0)
        {
            policy = OMCI_TCONT_POL_POLICY_WRR;
        }

        if (argc == 5)
        {
            if (tcontId == GPON_TCONT_ALL)
            {
                cmdOmciTcontPolicyAll(policy, OMCI_DFLT_PQ_PER_TCONT, 0);
            }
            else
            {
                cmdOmciTcontPolicy(tcontId, policy, OMCI_DFLT_PQ_PER_TCONT, 0);
            }
            return;
        }

        /* argc == 9 */
        if (strcasecmp(argv[5], "--maxqnum") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[5]);
            cmdOmciHelp("tcont");
            return;
        }
        else if (strcasecmp(argv[7], "--maxspqnum") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[7]);
            cmdOmciHelp("tcont");
            return;
        }

        maxQnum = strtoul(argv[6], (char **)NULL, 0);
        maxSpQnum = strtoul(argv[8], (char **)NULL, 0);
        if ((maxQnum > GPON_PHY_US_PQ_MAX) || (maxSpQnum > GPON_PHY_US_PQ_MAX))
        {
            cmsLog_error("Tcont: Invalid argument, maxQnum %d or maxSpQnum %d, "
              "max %d\n", maxQnum, maxSpQnum, GPON_PHY_US_PQ_MAX);
            return;
        }

        if (tcontId == GPON_TCONT_ALL)
        {
            cmdOmciTcontPolicyAll(policy, maxQnum, maxSpQnum);
        }
        else
        {
            cmdOmciTcontPolicy(tcontId, policy, maxQnum, maxSpQnum);
        }
    }
    else if (strcasecmp(argv[1], "--qinit") == 0)
    {
        if ((argc != 3) || (argv[2] == NULL))
        {
            cmsLog_error("Tcont: Missing arguments\n");
            cmdOmciHelp("tcont");
        }
        else if (strcasecmp(argv[2], "on") != 0 &&
          strcasecmp(argv[2], "off") != 0)
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[2]);
            cmdOmciHelp("tcont");
        }
        else
        {
            UBOOL8 flag = FALSE;

            if (strcasecmp(argv[2], "on") == 0)
            {
                flag = TRUE;
            }

            cmdOmciTcontQinit(flag);
        }
    }
    else if (strcasecmp(argv[1], "--allocidInit") == 0)
    {
        if ((argc != 3) || (argv[2] == NULL))
        {
            cmsLog_error("Tcont: Missing arguments\n");
            cmdOmciHelp("tcont");
        }
        else if (strcasecmp(argv[2], "255") == 0)
        {
            cmdOmciTcontAllocIdInit(0xFF);
        }
        else if (strcasecmp(argv[2], "65535") == 0)
        {
            cmdOmciTcontAllocIdInit(0xFFFF);
        }
        else
        {
            cmsLog_error("Tcont: Invalid argument '%s'\n", argv[2]);
            cmdOmciHelp("tcont");
        }
    }
    else if (strcasecmp(argv[1], "--schedulernum") == 0)
    {
        if ((argc != 3) || (argv[2] == NULL))
        {
            cmsLog_error("Tcont: Missing arguments\n");
            cmdOmciHelp("tcont");
        }
        else
        {
            UINT32 tschdPerTcont = strtoul(argv[2], (char **)NULL, 0);

            if (tschdPerTcont > GPON_PHY_US_TS_MAX)
            {
                cmsLog_error("Tcont: Invalid argument %d, max %d\n",
                  tschdPerTcont, GPON_PHY_US_TS_MAX);
                return;
            }

            cmdOmciTcontTrafficSchedulers(tschdPerTcont);
        }
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciTcontStatus();
    }
    else
    {
        cmsLog_error("Tcont: Invalid argument '%s'\n", argv[1]);
        cmdOmciHelp("tcont");
    }
}

/***************************************************************************
 * Function Name: cmdOmciEthMaxNum
 * Description  : configure maximum number of Ethernet ports.
 ***************************************************************************/
static void cmdOmciEthMaxNum(char *ethmaxStr)
{
    UINT32 portMax = 0;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    portMax = strtoul(ethmaxStr, (char**)NULL, 0);
    if (portMax > GPON_PHY_ETH_PORT_MAX)
    {
        cmsLog_error("Ethernet: Invalid argument, max %d",
          GPON_PHY_ETH_PORT_MAX);
        return;
    }

    ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void *)&obj)) == CMSRET_SUCCESS)
    {
        obj->numberOfEthernetPorts = portMax;
        cmsObj_set(obj, &iidStack);
        cmsObj_free((void**)&obj);

        cmsMgm_saveConfigToFlash();
    }

    cmsLck_releaseLock();
}

/***************************************************************************
 * Function Name: cmdOmciEthFlexMeIdModeConfig
 * Description  : configure PPTP ETH UNI flexible ME id mode.
 ***************************************************************************/
static void cmdOmciEthFlexMeIdModeConfig(char *flexIdModeStr)
{
    UBOOL8 flexMeIdMode = FALSE;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if (strcasecmp(flexIdModeStr, "on") == 0)
    {
        flexMeIdMode = TRUE;
    }

    ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        obj->ethUniFlexIdMode = flexMeIdMode;
        cmsObj_set(obj, &iidStack);
        cmsObj_free((void**)&obj);

        cmsMgm_saveConfigToFlash();
    }

    cmsLck_releaseLock();
}

/***************************************************************************
 * Function Name: cmdOmciEthFlexMeIdConfig
 * Description  : configure PPTP ETH UNI flexible ME id.
 ***************************************************************************/
static void cmdOmciEthFlexMeIdConfig(char *portIdxStr, char *meIdStr)
{
    UINT32 portIdx = 0;
    UINT32 meId = 0;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    portIdx = strtoul(portIdxStr, (char**)NULL, 0);
    if (portIdx >= GPON_PHY_ETH_PORT_MAX)
    {
        cmsLog_error("Ethernet: Invalid argument, portIdx %d, max %d",
          portIdx, GPON_PHY_ETH_PORT_MAX - 1);
        return;
    }

    meId = strtoul(meIdStr, (char**)NULL, 0);
    if (meId >= INVALID_MEID)
    {
        cmsLog_error("Ethernet: Invalid argument, meId %d", meId);
        return;
    }

    ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        switch (portIdx)
        {
            case 0:
                obj->ethernetManagedEntityId1 = meId;
                break;
            case 1:
                obj->ethernetManagedEntityId2 = meId;
                break;
            case 2:
                obj->ethernetManagedEntityId3 = meId;
                break;
            case 3:
                obj->ethernetManagedEntityId4 = meId;
                break;
            case 4:
                obj->ethernetManagedEntityId5 = meId;
                break;
            case 5:
                obj->ethernetManagedEntityId6 = meId;
                break;
            case 6:
                obj->ethernetManagedEntityId7 = meId;
                break;
            case 7:
                obj->ethernetManagedEntityId8 = meId;
                break;
            default:
                cmsLog_error("Ethernet: Invalid argument, portIdx %d"
                  "max %d for flexid mode",
                  portIdx, 8);
                break;
        }

        cmsObj_set(obj, &iidStack);
        cmsObj_free((void**)&obj);

        cmsMgm_saveConfigToFlash();
    }

    cmsLck_releaseLock();
}

/***************************************************************************
 * Function Name: cmdOmciEthSlotIdConfig
 * Description  : configure PPTP ETH UNI slot id.
 ***************************************************************************/
static void cmdOmciEthSlotIdConfig(char *feSlotIdStr, char *geSlotIdStr,
  char *xgeSlotIdStr)
{
    UINT32 feSlotId = RESERVED_SLOTID;
    UINT32 geSlotId = RESERVED_SLOTID;
    UINT32 xgeSlotId = RESERVED_SLOTID;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    feSlotId = strtoul(feSlotIdStr, (char**)NULL, 0);
    geSlotId = strtoul(geSlotIdStr, (char**)NULL, 0);
    xgeSlotId = strtoul(xgeSlotIdStr, (char**)NULL, 0);

    if ((feSlotId > RESERVED_SLOTID) || (geSlotId > RESERVED_SLOTID) ||
      (xgeSlotId > RESERVED_SLOTID))
    {
        cmsLog_error("Ethernet: Invalid argument, feSlotId %d, geSlotId %d, "
          "xgeSlotId %d, max %d",
          feSlotId, geSlotId, xgeSlotId, RESERVED_SLOTID);
        return;
    }

    ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        obj->ethUniFeSlotId = feSlotId;
        obj->ethUniGbeSlotId = geSlotId;
        obj->ethUni10GSlotId = xgeSlotId;

        cmsObj_set(obj, &iidStack);
        cmsObj_free((void**)&obj);

        cmsMgm_saveConfigToFlash();
    }

    cmsLck_releaseLock();
}

/***************************************************************************
 * Function Name: cmdOmciEthPortStatus
 * Description  : Show ETH UNI config status.
 ***************************************************************************/
static void cmdOmciEthPortStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      != CMSRET_SUCCESS)
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
        return;
    }

    if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
      (void*)&obj)) == CMSRET_SUCCESS)
    {
        printf("   Number of Ethernet ports: %d\n",
          obj->numberOfEthernetPorts);
        printf("   Ethernet UNI slot id      (FE:%d, GE:%d, XGE:%d)\n",
          obj->ethUniFeSlotId, obj->ethUniGbeSlotId, obj->ethUni10GSlotId);

#ifndef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
        switch (obj->ethUniFlexIdMode)
        {
            case 1:
                printf("   Flexible ME id mode is on\n");
                break;
            case 0:
                printf("   Flexible ME id mode is off\n");
                break;
            default:
                printf("   Flexible ME id mode %d is invalid\n",
                  obj->ethUniFlexIdMode);
                break;
        }

        printf("   Ethernet UNI ME id config (1:%d, 2:%d, 3:%d, 4:%d)\n"
               "                             (5:%d, 6:%d, 7:%d, 8:%d)\n",
          obj->ethernetManagedEntityId1, obj->ethernetManagedEntityId2,
          obj->ethernetManagedEntityId3, obj->ethernetManagedEntityId4,
          obj->ethernetManagedEntityId5, obj->ethernetManagedEntityId6,
          obj->ethernetManagedEntityId7, obj->ethernetManagedEntityId8);
#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */

#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
        printf("   PPTP_ETH_UNI0 as VEIP: %d, VEIP ME id: %d\n",
          obj->veipPptpUni0, obj->veipManagedEntityId1);

        rutOmci_printPorts();
#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */

        cmsObj_free((void**)&obj);
    }

    cmsLck_releaseLock();
}

/***************************************************************************
 * Function Name: cmdOmciEthVeip
 * Description  : configure PPTP_ETH_UNI0 as VEIP mode.
 ***************************************************************************/
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
static void cmdOmciEthVeip(char *startid, char *veip)
{
    UINT32 startId = 0;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    startId = strtoul(startid, (char **)NULL, 0);
    if (startId >= INVALID_MEID)
    {
        cmsLog_error("Ethernet: Invalid argument, startId %d, max 0x%x",
          startId, INVALID_MEID);
        return;
    }

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            if (!strcmp(veip, "1"))
            {
                //1st eth port as veip
                obj->veipPptpUni0 = 1;
                obj->veipManagedEntityId1 = startId;
            }
            else
            {
                obj->veipPptpUni0 = 0;
                obj->veipManagedEntityId1 = GPON_FIRST_VEIP_MEID;
            }
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }
}

/***************************************************************************
 * Function Name: cmdOmciEthPortTypeShow
 * Description  : show port type (rg, ont, or rgont) for all ethernet ports.
 ***************************************************************************/
static void cmdOmciEthPortTypeShow(void)
{
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        rutOmci_printPorts();
        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }
}

/***************************************************************************
 * Function Name: cmdOmciEthPortTypeConfig
 * Description  : configure (rg, ont, or rgont) type for the given ETH port.
 ***************************************************************************/
static void cmdOmciEthPortTypeConfig(char *ethPort, char *type)
{
    UINT32 port = 0;
    OmciEthPortType portType = OMCI_ETH_PORT_TYPE_NONE;
    OmciEthPortType_t eth;
    CmsRet ret = CMSRET_INVALID_ARGUMENTS;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // It is unlikely that a HGU device has more than 8 ETH UNI ports
    port = strtoul(ethPort, (char **)NULL, 0);
    if (port > GPON_HGU_ETH_PORT_MAX)
    {
        cmsLog_error("Ethernet port (%d) is out of range <0..8>", port);
        return;
    }

    if (strcmp(type, ETH_PORT_TYPE_RG) == 0)
        portType = OMCI_ETH_PORT_TYPE_RG;
    else if (strcmp(type, ETH_PORT_TYPE_ONT) == 0)
        portType = OMCI_ETH_PORT_TYPE_ONT;
    else
        portType = OMCI_ETH_PORT_TYPE_RG_ONT;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            eth.types.all = obj->ethernetTypes;

            switch (port)
            {
                case 0:
                    eth.types.ports.eth0 = portType;
                    break;
                case 1:
                    eth.types.ports.eth1 = portType;
                    break;
                case 2:
                    eth.types.ports.eth2 = portType;
                    break;
                case 3:
                    eth.types.ports.eth3 = portType;
                    break;
                case 4:
                    eth.types.ports.eth4 = portType;
                    break;
                case 5:
                    eth.types.ports.eth5 = portType;
                    break;
                case 6:
                    eth.types.ports.eth6 = portType;
                    break;
                case 7:
                    eth.types.ports.eth7 = portType;
                    break;
                default:
                    break;
            }

            obj->ethernetTypes = eth.types.all;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();

        cmdOmciEthPortTypeShow();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }
}

#endif   /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */

/***************************************************************************
 * Function Name: cmdOmciEthHandler
 * Description  : ETH command handler.
 ***************************************************************************/
void cmdOmciEthHandler(SINT32 argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("eth");
    }
#ifdef DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1
    else if (strcasecmp(argv[1], "--startid") == 0)
    {
        if ((argc < 5) || (argv[2] == NULL) || (argv[3] == NULL) ||
          (argv[4] == NULL))
        {
            cmsLog_error("Ethernet: Missing arguments\n");
            cmdOmciHelp("eth");
        }
        else if (strcasecmp(argv[3], "--veip") != 0)
        {
            cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[3]);
            cmdOmciHelp("eth");
        }

        cmdOmciEthVeip(argv[2], argv[4]);
    }
    else if (strcasecmp(argv[1], "--port") == 0)
    {
        if ((argc < 5) || (argv[2] == NULL) || (argv[3] == NULL) ||
          (argv[4] == NULL))
        {
            cmsLog_error("Ethernet: Missing arguments\n");
            cmdOmciHelp("eth");
        }
        else if (strcasecmp(argv[3], "--type") != 0)
        {
            cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[3]);
            cmdOmciHelp("eth");
        }
        else if (strcasecmp(argv[4], ETH_PORT_TYPE_RG_ONT) != 0 &&
          strcasecmp(argv[4], ETH_PORT_TYPE_RG) != 0 &&
          strcasecmp(argv[4], ETH_PORT_TYPE_ONT) != 0)
        {
            cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[4]);
            cmdOmciHelp("eth");
        }
        else
        {
            cmdOmciEthPortTypeConfig(argv[2], argv[4]);
        }
    }
#endif /* DMP_X_BROADCOM_COM_GPONRG_OMCI_FULL_1 */
    else if (strcasecmp(argv[1], "--portmax") == 0)
    {
        if ((argc != 3) || (argv[2] == NULL))
        {
            cmsLog_error("Ethernet: Missing arguments\n");
            cmdOmciHelp("eth");
        }
        else
        {
            cmdOmciEthMaxNum(argv[2]);
        }
    }
    else if (strcasecmp(argv[1], "--flexid") == 0)
    {
        if ((argc < 4) || (argv[2] == NULL) || (argv[3] == NULL))
        {
            cmsLog_error("Ethernet: Missing arguments\n");
            cmdOmciHelp("eth");
        }
        else if (strcasecmp(argv[2], "--mode") == 0)
        {
            if (strcasecmp(argv[3], "on") != 0 &&
              strcasecmp(argv[3], "off") != 0)
            {
                cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[3]);
                cmdOmciHelp("eth");
            }
            else
            {
                cmdOmciEthFlexMeIdModeConfig(argv[3]);
            }
        }
        else if (strcasecmp(argv[2], "--portid") == 0)
        {
            if ((argc < 6) || (argv[2] == NULL) || (argv[3] == NULL) ||
              (argv[4] == NULL) || (argv[5] == NULL))
            {
                cmsLog_error("Ethernet: Missing arguments\n");
                cmdOmciHelp("eth");
            }
            else if (strcasecmp(argv[4], "--meid") != 0)
            {
                cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[3]);
                cmdOmciHelp("eth");
            }
            else
            {
                cmdOmciEthFlexMeIdConfig(argv[3], argv[5]);
            }
        }
    }
    else if (strcasecmp(argv[1], "--slotid") == 0)
    {
        if ((argc < 8) || (argv[2] == NULL) || (argv[3] == NULL) ||
          (argv[4] == NULL) || (argv[5] == NULL) ||
          (argv[6] == NULL) || (argv[7] == NULL))
        {
            cmsLog_error("Ethernet: Missing arguments\n");
            cmdOmciHelp("eth");
        }
        else if ((strcasecmp(argv[2], "--fe") != 0) ||
          (strcasecmp(argv[4], "--ge") != 0) ||
          (strcasecmp(argv[6], "--xge") != 0))
        {
            cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[3]);
            cmdOmciHelp("eth");
        }
        else
        {
            cmdOmciEthSlotIdConfig(argv[3], argv[5], argv[7]);
        }
    }
    else if (strcasecmp(argv[1], "--show") == 0)
    {
        cmdOmciEthPortStatus();
    }
    else
    {
        cmsLog_error("Ethernet: Invalid argument '%s'\n", argv[1]);
        cmdOmciHelp("eth");
    }
}

/***************************************************************************
 * Function Name: cmdOmciMibSyncGet
 * Description  : get value of MibDataSync attribute in OntData object
 ***************************************************************************/
static CmsRet cmdOmciMibSyncGet(void)
{
    CmsMsgHeader cmdMsg;
    CmsRet ret = CMSRET_SUCCESS;

    memset(&cmdMsg, 0, sizeof(CmsMsgHeader));
    cmdMsg.type = CMS_MSG_OMCI_DUMP_INFO_REQ;
    cmdMsg.src = EID_CONSOLED;
    cmdMsg.dst = EID_OMCID;
    cmdMsg.flags_request = 1;
    cmdMsg.dataLength = 0;
    cmdMsg.wordData = (UINT32)MDMOID_ONT_DATA;

    ret = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)&cmdMsg);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Send CMS_MSG_OMCI_DUMP_INFO_REQ failed, ret=%d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciUniPathModeStatus
 * Description  : show current uni path mode configuration
 ***************************************************************************/
static CmsRet cmdOmciUniPathModeStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            switch (obj->uniDataPathMode)
            {
            case 1:
                printf("   UNI data path mode is configured to on\n\n");
                break;
            case 0:
                printf("   UNI data path mode is configured to off\n\n");
                break;
            default:
                printf("   UNI data path mode %d is invalid \n\n",
                  obj->uniDataPathMode);
                break;
            }
            cmsObj_free((void **)&obj);
        }
        else
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciUniPathModeConfig
 * Description  : configure UNI data path mode:
 *                TRUE - check the UNI ports that MEs on their ANI-UNI path
 *                       have been updated.
 *                FALSE - check the whole OMCI MIB.
 ***************************************************************************/
static CmsRet cmdOmciPathModeConfig(UINT32 uniDataPathMode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->uniDataPathMode = uniDataPathMode;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciUniPathModeStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciPromiscModeStatus
 * Description  : show current Promisc mode configuration
 ***************************************************************************/
static CmsRet cmdOmciPromiscModeStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            switch (obj->promiscMode)
            {
            case 0:
                printf("   Promisc mode is configured to off\n\n");
                break;
            case 1:
                printf("   Promisc mode is configured to on\n\n");
                break;
            default:
                printf("   Promisc mode is not configured yet\n\n");
                break;
            }
            cmsObj_free((void **)&obj);
        }
        else
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciPromiscModeConfigOmci
 * Description  : send promisc mode set request to OMCI.
 ***************************************************************************/
static CmsRet cmdOmciPromiscModeConfigOmci(UINT32 promiscMode)
{
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader reqMsg;
    CmsMsgHeader *respMsg = NULL;

    memset(&reqMsg, 0, sizeof(CmsMsgHeader));
    reqMsg.type = CMS_MSG_OMCI_PROMISC_SET_REQUEST;
    reqMsg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
    reqMsg.dst = EID_OMCID;
    reqMsg.flags_request = 1;
    reqMsg.dataLength = 0;
    reqMsg.sequenceNumber = 1;
    reqMsg.wordData = promiscMode;

    if ((ret = cmsMsg_send(cliPrvtMsgHandle, &reqMsg)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not send out CMS_MSG_OMCI_PROMISC_SET_REQUEST, "
          "ret=%d from CONSOLED to OMCID", ret);
        goto out;
    }
    cmsLog_notice("Send CMS_MSG_OMCI_PROMISC_SET_REQUEST from CONSOLED to OMCID");

    if ((ret = cmsMsg_receiveWithTimeout(cliPrvtMsgHandle, &respMsg,
      MSG_RECV_TIMEOUT_MS)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_PROMISC_SET_RESPONSE, "
          "ret=%d from OMCID to CONSOLED", ret);
        goto out;
    }
    if (respMsg->type != CMS_MSG_OMCI_PROMISC_SET_RESPONSE)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_PROMISC_SET_RESPONSE, "
        "type=%d", respMsg->type);
    }

out:
    CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciPromiscModeConfig
 * Description  : configure OMCI promisc mode in DB.
 ***************************************************************************/
static CmsRet cmdOmciPromiscModeConfig(UBOOL8 promiscMode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->promiscMode = promiscMode;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();

        ret = cmdOmciPromiscModeConfigOmci(promiscMode);

        cmdOmciPromiscModeStatus();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciRawModeStatus
 * Description  : show current raw mode configuration
 ***************************************************************************/
static CmsRet cmdOmciRawModeStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            if (obj->omciRawEnable == TRUE)
                printf("   OMCI raw is enabled - HTTPD should accept OMCI raw messages.\n\n");
            else
                printf("   OMCI raw is disabled - HTTPD should NOT accept OMCI raw messages.\n\n");
            cmsObj_free((void **)&obj);
        }
        else
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciRawModeConfig
 * Description  : configure raw mode:
 *                TRUE - allow OMCI raw messages be accepted by HTTPD.
 *                FALSE - disallow OMCI raw messages be accpeted by HTTPD.
 ***************************************************************************/
static CmsRet cmdOmciRawModeConfig(UBOOL8 rawMode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            obj->omciRawEnable = rawMode;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciRawModeStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTmOptionStatus
 * Description  : show current OMCI upstream traffic management option.
 ***************************************************************************/
static CmsRet cmdOmciTmOptionStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0, (void*)&obj))
          == CMSRET_SUCCESS)
        {
            printf("   OMCI upstream traffic management option:\n");
            if (obj->trafficManagementOption == OMCI_TRAFFIC_MANAGEMENT_PRIO)
            {
                printf("    (0) Priority controlled upstream traffic.\n\n");
            }
            else if (obj->trafficManagementOption == OMCI_TRAFFIC_MANAGEMENT_RATE)
            {
                printf("    (1) Rate controlled upstream traffic.\n\n");
            }
            else if (obj->trafficManagementOption == OMCI_TRAFFIC_MANAGEMENT_PRIO_RATE)
            {
                printf("    (2) Priority and rate controlled.\n\n");
            }
            else
            {
                printf("    Invalid option.\n\n");
            }

            printf("   QoS action when the downstream queue in "
              "GEM port network CTP ME is invalid:\n");
            if (obj->dsInvalidQueueAction == OMCI_DS_INVALID_QUEUE_ACTION_NONE)
            {
                printf("    (0) Do not set queue.\n\n");
            }
            else if (obj->dsInvalidQueueAction == OMCI_DS_INVALID_QUEUE_ACTION_PBIT)
            {
                printf("    (1) Set queue ID = packet pbit (HGU).\n\n");
            }
            else if (obj->dsInvalidQueueAction == OMCI_DS_INVALID_QUEUE_ACTION_PBIT_EXT)
            {
                printf("    (2) Set queue ID = packet pbit (SFU and HGU).\n\n");
            }
            else
            {
                printf("    Invalid option.\n\n");
            }
            cmsObj_free((void **)&obj);
        }
        else
        {
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTmOptionConfig
 * Description  : configure OMCI upstream traffic management option.
 ***************************************************************************/
static CmsRet cmdOmciTmOptionConfig(UINT32 tmOption)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0, (void*)&obj))
          == CMSRET_SUCCESS)
        {
            obj->trafficManagementOption = tmOption;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciTmOptionStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciDsQosConfig
 * Description  : configure OMCI downstream queue setting option.
 ***************************************************************************/
static CmsRet cmdOmciDsQosConfig(UINT32 qosAction)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->dsInvalidQueueAction = qosAction;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciTmOptionStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciDebugStatus
 * Description  : show debug status for all modules
 ***************************************************************************/
static CmsRet cmdOmciDebugStatus(void)
{
    omciDebug_t debug;
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void *)&obj)) == CMSRET_SUCCESS)
        {
            debug.flags.all = obj->debugFlags;
            cmsObj_free((void **)&obj);
            printf("\n=========== OMCI Debug Status ===========\n\n");
            if (debug.flags.bits.omci == OMCID_DEBUG_ON)
                printf("   OMCI messages:          ON\n");
            else
                printf("   OMCI messages:          OFF\n");
            if (debug.flags.bits.model == OMCID_DEBUG_ON)
                printf("   MODEL messages:         ON\n");
            else
                printf("   MODEL messages:         OFF\n");
            if (debug.flags.bits.vlan == OMCID_DEBUG_ON)
                printf("   VLAN messages:          ON\n");
            else
                printf("   VLAN messages:          OFF\n");
            if (debug.flags.bits.mib == OMCID_DEBUG_ON)
                printf("   MIB messages:           ON\n");
            else
                printf("   MIB messages:           OFF\n");
            if (debug.flags.bits.flow == OMCID_DEBUG_ON)
                printf("   FLOW messages:          ON\n");
            else
                printf("   FLOW messages:          OFF\n");
            if (debug.flags.bits.rule == OMCID_DEBUG_ON)
                printf("   RULE messages:          ON\n");
            else
                printf("   RULE messages:          OFF\n");
            if (debug.flags.bits.mcast == OMCID_DEBUG_ON)
                printf("   MULTICAST messages:     ON\n");
            else
                printf("   MULTICAST messages:     OFF\n");
#if defined(DMP_X_ITU_ORG_VOICE_1)
            if (debug.flags.bits.voice == OMCID_DEBUG_ON)
                printf("   VOICE messages:         ON\n");
            else
                printf("   VOICE messages:         OFF\n");
#endif    // DMP_X_ITU_ORG_VOICE_1
            if (debug.flags.bits.file == OMCID_DEBUG_ON)
                printf("   Messages are logged to: FILE\n");
            else
                printf("   Messages are logged to: CONSOLE\n");
            printf("\n=========================================\n\n");
        }
        else
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciDebugDump
 * Description  : Write objects in MDM that has the given class ID to console
 ***************************************************************************/
static CmsRet cmdOmciDebugDump(const char *id)
{
    UINT32 classId;
    CmsMsgHeader cmdMsg;
    CmsRet ret = CMSRET_SUCCESS;

    classId = (UINT32)strtoul(id, (char**)NULL, 0);
    if ((classId >= OMCI_ME_STD_CLASS_MAX) && 
      (classId < OMCI_ME_VEN_CLASS_START))
    {
        cmsLog_notice("Invalid ME Class: %d", classId);
        return CMSRET_INVALID_ARGUMENTS;
    }

    memset(&cmdMsg, 0, sizeof(CmsMsgHeader));
    cmdMsg.type = CMS_MSG_OMCI_DUMP_INFO_REQ;
    cmdMsg.src = EID_CONSOLED;
    cmdMsg.dst = EID_OMCID;
    cmdMsg.flags_request = 1;
    cmdMsg.dataLength = 0;
    cmdMsg.wordData = (UINT32)classId;

    ret = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)&cmdMsg);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Send CMS_MSG_OMCI_DUMP_INFO_REQ failed, ret=%d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciDebugInfo
 * Description  : Dump OMCI internal data
 ***************************************************************************/
static CmsRet cmdOmciDebugInfo(char *numValStr)
{
    CmsMsgHeader cmdMsg;
    UINT32 flag = 0;
    CmsRet ret = CMSRET_SUCCESS;

    if (numValStr != NULL)
    {
        flag = atoi(numValStr);
    }

    memset(&cmdMsg, 0, sizeof(CmsMsgHeader));
    cmdMsg.type = CMS_MSG_OMCI_DUMP_INFO_REQ;
    cmdMsg.src = EID_CONSOLED;
    cmdMsg.dst = EID_OMCID;
    cmdMsg.flags_request = 1;
    cmdMsg.dataLength = 0;
    cmdMsg.wordData = (UINT32)flag;

    ret = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)&cmdMsg);
    if (ret != CMSRET_SUCCESS)
    {
        cmsLog_error("Send CMS_MSG_OMCI_DUMP_INFO_REQ failed, ret=%d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciDebug
 * Description  : show/hide OMCI, MODEL, VLAN, etc. debug messages
 ***************************************************************************/
static CmsRet cmdOmciDebug(char *module, char *state)
{
    omciDebug_t debug;
    CmsRet ret = CMSRET_SUCCESS;
    CmsMsgHeader reqMsg;
    CmsMsgHeader *respMsg = NULL;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    memset(&reqMsg, 0, sizeof(CmsMsgHeader));
    reqMsg.type = CMS_MSG_OMCI_DEBUG_GET_REQUEST;
    reqMsg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
    reqMsg.dst = EID_OMCID;
    reqMsg.flags_request = 1;
    reqMsg.dataLength = 0;
    reqMsg.sequenceNumber = 1;

    if ((ret = cmsMsg_send(cliPrvtMsgHandle, &reqMsg)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not send out CMS_MSG_OMCI_DEBUG_GET_REQUEST, "
          "ret=%d from CONSOLED to OMCID", ret);
        goto out;
    }
    cmsLog_notice("Send CMS_MSG_OMCI_DEBUG_GET_REQUEST from CONSOLED to OMCID");

    if ((ret = cmsMsg_receiveWithTimeout(cliPrvtMsgHandle, &respMsg,
      MSG_RECV_TIMEOUT_MS)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_DEBUG_GET_RESPONSE, "
          "ret=%d from OMCID to CONSOLED", ret);
        goto out;
    }
    if (respMsg->type != CMS_MSG_OMCI_DEBUG_GET_RESPONSE)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_DEBUG_GET_RESPONSE, "
          "type=%d", respMsg->type);
        goto out;
    }
    cmsLog_notice("Receive CMS_MSG_OMCI_DEBUG_GET_RESPONSE from OMCID to CONSOLED");

    debug.flags.all = respMsg->wordData;
    CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);

    if (strcasecmp(module, "all") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.omci = debug.flags.bits.model =
                debug.flags.bits.vlan = debug.flags.bits.mib =
                debug.flags.bits.flow = debug.flags.bits.rule =
                debug.flags.bits.mcast = debug.flags.bits.voice =
                debug.flags.bits.file = OMCID_DEBUG_ON;
        else
            debug.flags.bits.omci = debug.flags.bits.model =
                debug.flags.bits.vlan = debug.flags.bits.mib =
                debug.flags.bits.flow = debug.flags.bits.rule =
                debug.flags.bits.mcast = debug.flags.bits.voice =
                debug.flags.bits.file = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "omci") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.omci = OMCID_DEBUG_ON;
        else
            debug.flags.bits.omci = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "model") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.model = OMCID_DEBUG_ON;
        else
            debug.flags.bits.model = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "vlan") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.vlan = OMCID_DEBUG_ON;
        else
            debug.flags.bits.vlan = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "mib") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.mib = OMCID_DEBUG_ON;
        else
            debug.flags.bits.mib = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "flow") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.flow = OMCID_DEBUG_ON;
        else
            debug.flags.bits.flow = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "rule") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.rule = OMCID_DEBUG_ON;
        else
            debug.flags.bits.rule = OMCID_DEBUG_OFF;
    }
    else if (strcasecmp(module, "mcast") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.mcast = OMCID_DEBUG_ON;
        else
            debug.flags.bits.mcast = OMCID_DEBUG_OFF;
    }
#if defined(DMP_X_ITU_ORG_VOICE_1)
    else if (strcasecmp(module, "voice") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.voice = OMCID_DEBUG_ON;
        else
            debug.flags.bits.voice = OMCID_DEBUG_OFF;
    }
#endif    // DMP_X_ITU_ORG_VOICE_1
    else if (strcasecmp(module, "file") == 0)
    {
        if (strcasecmp(state, "on") == 0)
            debug.flags.bits.file = OMCID_DEBUG_ON;
        else
            debug.flags.bits.file = OMCID_DEBUG_OFF;
    }

    memset(&reqMsg, 0, sizeof(CmsMsgHeader));
    reqMsg.type = CMS_MSG_OMCI_DEBUG_SET_REQUEST;
    reqMsg.src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
    reqMsg.dst = EID_OMCID;
    reqMsg.flags_request = 1;
    reqMsg.dataLength = 0;
    reqMsg.sequenceNumber = 1;
    reqMsg.wordData = debug.flags.all;

    if ((ret = cmsMsg_send(cliPrvtMsgHandle, &reqMsg)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Could not send out CMS_MSG_OMCI_DEBUG_SET_REQUEST, "
          "ret=%d from CONSOLED to OMCID", ret);
        goto out;
    }
    cmsLog_notice("Send CMS_MSG_OMCI_DEBUG_SET_REQUEST from CONSOLED to OMCID");

    if ((ret = cmsMsg_receiveWithTimeout(cliPrvtMsgHandle, &respMsg,
      MSG_RECV_TIMEOUT_MS)) != CMSRET_SUCCESS)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_DEBUG_SET_RESPONSE, "
          "ret=%d from OMCID to CONSOLED", ret);
        goto out;
    }
    if (respMsg->type != CMS_MSG_OMCI_DEBUG_SET_RESPONSE)
    {
        cmsLog_error("Failed to receive CMS_MSG_OMCI_DEBUG_SET_RESPONSE, "
          "type=%d", respMsg->type);
        goto out;
    }
    cmsLog_notice("Receive CMS_MSG_OMCI_DEBUG_SET_RESPONSE from OMCID to CONSOLED");

    debug.flags.all = respMsg->wordData;
    CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);

    printf("\n=========== OMCI Debug Status ===========\n\n");
    if (debug.flags.bits.omci == OMCID_DEBUG_ON)
        printf("   OMCI messages:          ON\n");
    else
        printf("   OMCI messages:          OFF\n");
    if (debug.flags.bits.model == OMCID_DEBUG_ON)
        printf("   MODEL messages:         ON\n");
    else
        printf("   MODEL messages:         OFF\n");
    if (debug.flags.bits.vlan == OMCID_DEBUG_ON)
        printf("   VLAN messages:          ON\n");
    else
        printf("   VLAN messages:          OFF\n");
    if (debug.flags.bits.mib == OMCID_DEBUG_ON)
        printf("   MIB messages:           ON\n");
    else
        printf("   MIB messages:           OFF\n");
    if (debug.flags.bits.flow == OMCID_DEBUG_ON)
        printf("   FLOW messages:          ON\n");
    else
        printf("   FLOW messages:          OFF\n");
    if (debug.flags.bits.rule == OMCID_DEBUG_ON)
        printf("   RULE messages:          ON\n");
    else
        printf("   RULE messages:          OFF\n");
    if (debug.flags.bits.mcast == OMCID_DEBUG_ON)
        printf("   MULTICAST messages:     ON\n");
    else
        printf("   MULTICAST messages:     OFF\n");
#if defined(DMP_X_ITU_ORG_VOICE_1)
    if (debug.flags.bits.voice == OMCID_DEBUG_ON)
        printf("   VOICE messages:         ON\n");
    else
        printf("   VOICE messages:         OFF\n");
#endif    // DMP_X_ITU_ORG_VOICE_1
    if (debug.flags.bits.file == OMCID_DEBUG_ON)
        printf("   Messages are logged to: FILE\n");
    else
        printf("   Messages are logged to: CONSOLE\n");
    printf("\n=========================================\n\n");

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack,
          0, (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->debugFlags = debug.flags.all;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

out:
    CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMcastHostctrlStatus
 * Description  : show current mcast host control mode configuration
 ***************************************************************************/
static CmsRet cmdOmciMcastHostctrlStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    McastCfgObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void *)&obj))
          == CMSRET_SUCCESS)
        {
            if (obj->mcastHostControl== TRUE)
                printf("   OMCI host controlled multicast is enabled.\n\n");
            else
                printf("   OMCI host controlled multicast is disabled.\n\n");
            cmsObj_free((void **)&obj);
        }
        else
            cmsLog_error("Could not cmsObj_get McastCfgObject, ret=%d", ret);

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMcastHostctrlConfig
 * Description  : configure mcast mode:
 *                TRUE - Enable Host controlled multicast.
 *                FALSE - Disable Host controlled multicast.
 ***************************************************************************/
static CmsRet cmdOmciMcastHostctrlConfig(UBOOL8 mcastMode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    McastCfgObject *obj = NULL;

    // Attempt to lock MDM.
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_MCAST_CFG, &iidStack, 0, (void *)&obj))
          == CMSRET_SUCCESS)
        {
            obj->mcastHostControl = mcastMode;

            cmsObj_set(obj, &iidStack);
            cmsObj_free((void **)&obj);

            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciMcastHostctrlStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTestAvc
 * Description  : Request OMCID to generate debug AVC message.
 ***************************************************************************/
static CmsRet cmdOmciTestAvc(UINT16 meClass, UINT16 meId, UINT16 attrMask)
{
    CmsRet ret = CMSRET_SUCCESS;
    char buf[sizeof(CmsMsgHeader) + sizeof(OmciMsgGenCmd)];
    CmsMsgHeader *msgHdrP = (CmsMsgHeader*)buf;
    OmciMsgGenCmd *bodyP = (OmciMsgGenCmd*)(msgHdrP + 1);
    UINT32 dataLength = sizeof(OmciMsgGenCmd);

    memset(buf, 0, sizeof(CmsMsgHeader) + dataLength);
    msgHdrP->type = CMS_MSG_OMCI_DEBUG_OMCI_MSG_GEN;
    msgHdrP->dst = EID_OMCID;
    msgHdrP->src = EID_CONSOLED;
    msgHdrP->dataLength = dataLength;
    msgHdrP->flags_request = TRUE;
    bodyP->msgType = OMCI_MSG_TYPE_ATTRIBUTEVALUECHANGE;
    bodyP->meClass = meClass;
    bodyP->meInst = meId;
    bodyP->attrMask = attrMask;

    ret = cmsMsg_send(cliPrvtMsgHandle, msgHdrP);
    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTestAlarm
 * Description  : Request OMCID to generate debug alarm message.
 ***************************************************************************/
static CmsRet cmdOmciTestAlarm(UINT16 meClass, UINT16 meId, UINT16 alarmNum,
  UBOOL8 setFlagB)
{
    CmsRet ret = CMSRET_SUCCESS;
    char buf[sizeof(CmsMsgHeader) + sizeof(OmciMsgGenCmd)];
    CmsMsgHeader *msgHdrP = (CmsMsgHeader*)buf;
    OmciMsgGenCmd *bodyP = (OmciMsgGenCmd*)(msgHdrP + 1);
    UINT32 dataLength = sizeof(OmciMsgGenCmd);

    memset(buf, 0, sizeof(CmsMsgHeader) + dataLength);
    msgHdrP->type = CMS_MSG_OMCI_DEBUG_OMCI_MSG_GEN;
    msgHdrP->dst = EID_OMCID;
    msgHdrP->src = EID_CONSOLED;
    msgHdrP->dataLength = dataLength;
    msgHdrP->flags_request = TRUE;
    bodyP->msgType = OMCI_MSG_TYPE_ALARM;
    bodyP->meClass = meClass;
    bodyP->meInst = meId;
    bodyP->alarmNum = alarmNum;
    bodyP->flagB = setFlagB;

    ret = cmsMsg_send(cliPrvtMsgHandle, msgHdrP);
    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciVoiceModelSet
 * Description  : Set voice model (OMCI path or IP path).
 ***************************************************************************/
static CmsRet cmdOmciVoiceModelSet(UINT8 model)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->voiceModelOption = model;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }
        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret=%d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciVoiceModelGet
 * Description  : show current voice model configuration
 ***************************************************************************/
static CmsRet cmdOmciVoiceModelGet(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            (obj->voiceModelOption == 0) ? \
              printf("   OMCI path.\n\n") : printf("   IP path.\n\n");
            cmsObj_free((void **)&obj);
        }
        else
        {
            cmsLog_error("cmsObj_get(OmciSystem) failed, ret=%d", ret);
        }
        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout() failed, ret=%d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciTmOptionHandler
 * Description  : TM option command handler.
 ***************************************************************************/
void cmdOmciTmOptionHandler(SINT32 argc __attribute__((unused)), char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("tmoption");
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("tmoption");
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciTmOptionStatus();
    }
    else if (strcasecmp(argv[1], "--usmode") == 0)
    {
        if (argv[2] == NULL)
        {
            cmsLog_error("TM option: Missing arguments\n");
            cmdOmciHelp("tmoption");
        }
        else
        {
            if (strcasecmp(argv[2], "0") == 0)
            {
                cmdOmciTmOptionConfig(0);
            }
            else if (strcasecmp(argv[2], "1") == 0)
            {
                cmdOmciTmOptionConfig(1);
            }
            else if (strcasecmp(argv[2], "2") == 0)
            {
                cmdOmciTmOptionConfig(2);
            }
            else
            {
                cmsLog_error("TM option: Invalid argument '%s'\n", argv[2]);
                cmdOmciHelp("tmoption");
            }
        }
    }
    else if (strcasecmp(argv[1], "--dsqueue") == 0)
    {
        if (argv[2] == NULL)
        {
            cmsLog_error("TM option: Missing arguments\n");
            cmdOmciHelp("tmoption");
        }
        else
        {
            if (strcasecmp(argv[2], "0") == 0)
            {
                cmdOmciDsQosConfig(0);
            }
            else if (strcasecmp(argv[2], "1") == 0)
            {
                cmdOmciDsQosConfig(1);
            }
            else if (strcasecmp(argv[2], "2") == 0)
            {
                cmdOmciDsQosConfig(2);
            }
            else
            {
                cmsLog_error("TM option: Invalid argument '%s'\n", argv[2]);
                cmdOmciHelp("tmoption");
            }
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciTestHandler
 * Description  : OMCI avc/alarm command handler.
 ***************************************************************************/
void cmdOmciTestHandler(SINT32 argc, char *argv[])
{
    UINT16 val0;
    UINT16 val1;
    UINT16 val2;
    UINT16 val3;

    if (argv[1] == NULL)
    {
        cmdOmciHelp("test");
    }
    else if (strcasecmp(argv[1], "--genavc") == 0)
    {
        if ((argc == 8) &&
          (strcasecmp(argv[2], "--meclass") == 0) &&
          (strcasecmp(argv[4], "--meid") == 0) &&
          (strcasecmp(argv[6], "--attrmask") == 0))
        {
            val0 = strtoul(argv[3], (char**)NULL, 0);
            val1 = strtoul(argv[5], (char**)NULL, 0);
            val2 = strtoul(argv[7], (char**)NULL, 0);
            cmdOmciTestAvc(val0, val1, val2);
        }
        else
        {
            cmdOmciHelp("test");
        }
    }
    else if (strcasecmp(argv[1], "--genalarm") == 0)
    {
        if ((argc == 10) &&
          (strcasecmp(argv[2], "--meclass") == 0) &&
          (strcasecmp(argv[4], "--meid") == 0) &&
          (strcasecmp(argv[6], "--num") == 0) &&
          (strcasecmp(argv[8], "--state") == 0))
        {
            val0 = strtoul(argv[3], (char**)NULL, 0);
            val1 = strtoul(argv[5], (char**)NULL, 0);
            val2 = strtoul(argv[7], (char**)NULL, 0);
            val3 = strtoul(argv[9], (char**)NULL, 0);
            cmdOmciTestAlarm(val0, val1, val2, val3);
        }
        else
        {
            cmdOmciHelp("test");
        }
    }
    else
    {
        cmdOmciHelp("test");
    }
}

/***************************************************************************
 * Function Name: cmdOmciBridgeStatus
 * Description  : show current bridge setting.
 ***************************************************************************/
static CmsRet cmdOmciBridgeStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            printf("   Bridge forwarding mask: 0x%08x\n",
              obj->bridgeGroupFwdMask);
            cmsObj_free((void**)&obj);
        }
        else
        {
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciBridgeConfig
 * Description  : configure bridge forwarding mask.
 ***************************************************************************/
static CmsRet cmdOmciBridgeConfig(UINT16 mask)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->bridgeGroupFwdMask = mask;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciBridgeStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciBridgeHandler
 * Description  : OMCI bridge command handler.
 ***************************************************************************/
void cmdOmciBridgeHandler(SINT32 argc __attribute__((unused)), char *argv[])
{
    UINT16 val0;

    if (argv[1] == NULL)
    {
        cmdOmciHelp("bridge");
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("bridge");
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciBridgeStatus();
    }
    else if (strcasecmp(argv[1], "--fwdmask") == 0)
    {
        if (argv[2] == NULL)
        {
            cmsLog_error("bridge: Missing arguments\n");
            cmdOmciHelp("bridge");
        }
        else
        {
            val0 = strtoul(argv[2], (char**)NULL, 0);
            cmdOmciBridgeConfig(val0);
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciMiscStatus
 * Description  : show current miscellaneous setting.
 ***************************************************************************/
static CmsRet cmdOmciMiscStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            printf("   Extended VLAN default rule mode: ");
            if (obj->extVlanDefaultRuleEnable == OMCI_FLOW_UPSTREAM)
            {
                printf("Upstream enabled\n");
            }
            else if (obj->extVlanDefaultRuleEnable == OMCI_FLOW_DOWNSTREAM)
            {
                printf("Downstream enabled\n");
            }
            else if (obj->extVlanDefaultRuleEnable == OMCI_FLOW_BOTH)
            {
                printf("Upstream and downstream enabled\n");
            }
            else
            {
                printf("Upstream and downstream disabled %u\n",
                obj->extVlanDefaultRuleEnable);
            }

            printf("   OMCC version: 0x%x\n", obj->omccVersion);

            printf("   Extended message set: ");
            if (obj->extMsgSetEnable == TRUE)
            {
                printf("enabled\n");
            }
            else
            {
                printf("disabled\n");
            }

            cmsObj_free((void**)&obj);
        }
        else
        {
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMiscExtVlanDefaultRuleConfig
 * Description  : configure Extended VLAN default rule enable mode.
 ***************************************************************************/
static CmsRet cmdOmciMiscExtVlanDefaultRuleConfig(UINT8 mode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack,
          0, (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->extVlanDefaultRuleEnable = mode;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciMiscStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMiscExtVlanDefaultRuleHandler
 * Description  : OMCI miscellaneous EXT VLAN command handler.
 ***************************************************************************/
void cmdOmciMiscExtVlanDefaultRuleHandler(SINT32 argc __attribute__((unused)),
  char *argv[])
{
    UINT32 val = 0;

    if (argv[2] == NULL)
    {
        cmsLog_error("Misc: Missing arguments\n");
        cmdOmciHelp("misc");
    }
    else
    {
        val = strtoul(argv[2], (char**)NULL, 0);
        if (val > OMCI_FLOW_BOTH)
        {
            cmsLog_error("Misc: Invalid argument '%s'\n", argv[2]);
            cmdOmciHelp("misc");
        }
        else
        {
            cmdOmciMiscExtVlanDefaultRuleConfig(val);
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciMiscOmccVersionConfig
 * Description  : configure omcc version.
 ***************************************************************************/
static CmsRet cmdOmciMiscOmccVersionConfig(UINT32 version)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack,
          0, (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->omccVersion = version;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciMiscStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMiscOmccVersionHandler
 * Description  : OMCC version set command handler.
 ***************************************************************************/
void cmdOmciMiscOmccVersionHandler(SINT32 argc __attribute__((unused)),
  char *argv[])
{
    char *argvP = NULL;
    UINT32 val = 0;

    if (argv[2] == NULL)
    {
        cmsLog_error("Misc: Missing arguments\n");
        cmdOmciHelp("misc");
    }
    else
    {
        argvP = argv[2];
        if ((argvP[0] == '0') && ((argvP[1] == 'x') || (argvP[1] == 'X')))
        {
            /* Argument in hex. */
            val = strtoul(&argvP[0], (char**)NULL, 16);
        }
        else
        {
            /* Argument in decimal. */
            val = strtoul(&argvP[0], (char**)NULL, 0);
        }

        cmdOmciMiscOmccVersionConfig(val);
    }
}


/***************************************************************************
 * Function Name: cmdOmciMiscExtMsgSetModeConfig
 * Description  : configure extended message set enable mode.
 ***************************************************************************/
static CmsRet cmdOmciMiscExtMsgSetModeConfig(UBOOL8 mode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack,
          0, (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->extMsgSetEnable = mode;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciMiscStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMiscExtMsgSetHandler
 * Description  : OMCI extended message set command handler.
 ***************************************************************************/
void cmdOmciMiscExtMsgSetHandler(SINT32 argc __attribute__((unused)),
  char *argv[])
{
    if (argv[2] == NULL)
    {
        cmsLog_error("Misc: Missing arguments\n");
        cmdOmciHelp("misc");
    }
    else
    {
        if (strcasecmp(argv[2], "on") != 0 &&
          strcasecmp(argv[2], "off") != 0)
        {
            cmsLog_error("Misc: Invalid argument '%s'\n", argv[2]);
            cmdOmciHelp("misc");
        }
        else
        {
            if (strcasecmp(argv[2], "on") == 0)
            {
                cmdOmciMiscExtMsgSetModeConfig(TRUE);
            }
            else if (strcasecmp(argv[2], "off") == 0)
            {
                cmdOmciMiscExtMsgSetModeConfig(FALSE);
            }
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciAuthStatus
 * Description  : show current authentication setting.
 ***************************************************************************/
static CmsRet cmdOmciAuthStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;
    int retCode = OMCI_IPC_RET_SUCCESS;
    int ipcRet;
    OmciDebugData debugData;

    /* Print PSK. */
    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            printf("   Pre-shared secret: ");
            if (strlen(obj->psk) == 0)
            {
                printf("NULL(len=0)\n");
            }
            else
            {
                printf("%s(len=%d)\n", obj->psk, strlen(obj->psk));
            }

            cmsObj_free((void**)&obj);
        }
        else
        {
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    /* Print MSK. */
    debugData.msgType = OMCI_DEBUG_DATA_GET_MSK;
    debugData.dataLen = 0;
    ipcRet = omciIpc_sendDebugData(&debugData, &retCode);
    if ((ipcRet < 0) || (retCode != OMCI_IPC_RET_SUCCESS))
    {
        cmsLog_error("omciIpc_sendDebugData() failed, ret=%d", ipcRet);
        ret = CMSRET_INTERNAL_ERROR;
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciAuthPskConfig
 * Description  : configure pre-shared secret.
 ***************************************************************************/
static CmsRet cmdOmciAuthPskConfig(char *bufP)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            CMSMEM_REPLACE_STRING(obj->psk, bufP);
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciAuthPskHandler
 * Description  : OMCI authentication PSK command handler.
 ***************************************************************************/
void cmdOmciAuthPskHandler(SINT32 argc __attribute__((unused)), char *argv[])
{
    UINT8 *buf = NULL;
    UINT32 bufSize = 0;
    CmsRet ret = CMSRET_SUCCESS;

    if (argv[2] == NULL)
    {
        cmsLog_error("Auth: Missing arguments\n");
        cmdOmciHelp("auth");
    }
    else
    {
        ret = cmsUtl_hexStringToBinaryBuf(argv[2], &buf, &bufSize);
        if ((ret == CMSRET_SUCCESS) && (bufSize == ONU_PSK_LEN))
        {
            cmdOmciAuthPskConfig(argv[2]);
        }
        else
        {
            cmsLog_error("Invalid hex string, ret=%d, bufsize=%d",
              ret, bufSize);
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
    }
}

/***************************************************************************
 * Function Name: cmdOmciAuthCmacCalcReq
 * Description  : Request OMCI to calculate CMAC based on psk and
 *                input message.
 ***************************************************************************/
static CmsRet cmdOmciAuthCmacCalcReq(UINT8 *dataBuf, UINT32 bufSize)
{
    int retCode = OMCI_IPC_RET_SUCCESS;
    int ipcRet;
    CmsRet ret = CMSRET_SUCCESS;
    OmciDebugData debugData;

    if (bufSize > OMCI_DEBUG_MSG_DATA_LEN_MAX)
    {
        cmsLog_error("Maximum data len %d", OMCI_DEBUG_MSG_DATA_LEN_MAX);
        return CMSRET_INVALID_ARGUMENTS;
    }

    debugData.msgType = OMCI_DEBUG_DATA_CMAC_MSG;
    debugData.dataLen = bufSize;
    memcpy(debugData.data, dataBuf, bufSize);
    ipcRet = omciIpc_sendDebugData(&debugData, &retCode);
    if ((ipcRet < 0) || (retCode != OMCI_IPC_RET_SUCCESS))
    {
        cmsLog_error("omciIpc_sendDebugData() failed, ret=%d", ipcRet);
        ret = CMSRET_INTERNAL_ERROR;
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciAuthCmacHandler
 * Description  : OMCI authentication CMAC command handler.
 ***************************************************************************/
void cmdOmciAuthCmacHandler(SINT32 argc, char *argv[])
{
    UINT8 *buf = NULL;
    UINT32 bufSize = 0;
    CmsRet ret = CMSRET_SUCCESS;

    if ((argc == 4) && (strcasecmp(argv[2], "--m") == 0))
    {
        ret = cmsUtl_hexStringToBinaryBuf(argv[3], &buf, &bufSize);
        if (ret == CMSRET_SUCCESS)
        {
            cmdOmciAuthCmacCalcReq(buf, bufSize);
        }
        else
        {
            cmsLog_error("Invalid hex string, ret=%d, bufsize=%d",
              ret, bufSize);
        }

        CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
    }
    else
    {
        cmsLog_error("Auth: Missing arguments\n");
        cmdOmciHelp("auth");
    }
}

/***************************************************************************
 * Function Name: cmdOmciAuthMskHandler
 * Description  : OMCI authentication MSK command handler.
 ***************************************************************************/
void cmdOmciAuthMskHandler(SINT32 argc, char *argv[])
{
    int retCode = OMCI_IPC_RET_SUCCESS;
    int ipcRet;
    OmciDebugData debugData;

    if ((argc == 3) && (strcasecmp(argv[2], "--clear") == 0))
    {
        debugData.msgType = OMCI_DEBUG_DATA_CLR_MSK;
        debugData.dataLen = 0;
        ipcRet = omciIpc_sendDebugData(&debugData, &retCode);
        if ((ipcRet < 0) || (retCode != OMCI_IPC_RET_SUCCESS))
        {
            cmsLog_error("omciIpc_sendDebugData() failed, ret=%d", ipcRet);
        }
    }
    else
    {
        cmsLog_error("Auth: Invalid arguments\n");
        cmdOmciHelp("auth");
    }
}

/***************************************************************************
 * Function Name: cmdOmciAuthHandler
 * Description  : OMCI authentication command handler.
 ***************************************************************************/
void cmdOmciAuthHandler(SINT32 argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("auth");
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("auth");
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciAuthStatus();
    }
    else if (strcasecmp(argv[1], "--psk") == 0)
    {
        cmdOmciAuthPskHandler(argc, argv);
    }
    else if (strcasecmp(argv[1], "--cmac") == 0)
    {
        cmdOmciAuthCmacHandler(argc, argv);
    }
    else if (strcasecmp(argv[1], "--msk") == 0)
    {
        cmdOmciAuthMskHandler(argc, argv);
    }
}

/***************************************************************************
 * Function Name: cmdOmciMiscHandler
 * Description  : OMCI miscellaneous command handler.
 ***************************************************************************/
void cmdOmciMiscHandler(SINT32 argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("misc");
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("misc");
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciMiscStatus();
    }
    else if (strcasecmp(argv[1], "--extvlandefault") == 0)
    {
        cmdOmciMiscExtVlanDefaultRuleHandler(argc, argv);
    }
    else if (strcasecmp(argv[1], "--omccversion") == 0)
    {
        cmdOmciMiscOmccVersionHandler(argc, argv);
    }
    else if (strcasecmp(argv[1], "--extmsg") == 0)
    {
        cmdOmciMiscExtMsgSetHandler(argc, argv);
    }
}

/***************************************************************************
 * Function Name: cmdOmciDdiHandler
 * Description  : OMCI developer debug interface handler.
 ***************************************************************************/
void cmdOmciDdiHandler(char *cmdLine)
{
    int cmdLen;
    OmciDebugData debugData;
    int retCode = OMCI_IPC_RET_SUCCESS;
    int ipcRet;

    cmdLen = strlen(cmdLine);
    if (cmdLen > OMCI_DEBUG_MSG_DATA_LEN_MAX)
    {
        cmsLog_error("Maximum data len %d", OMCI_DEBUG_MSG_DATA_LEN_MAX);
        return;
    }

    debugData.msgType = OMCI_DEBUG_DATA_DDI;
    debugData.dataLen = cmdLen;
    memcpy(debugData.data, cmdLine, cmdLen);
    ipcRet = omciIpc_sendDebugData(&debugData, &retCode);
    if ((ipcRet < 0) || (retCode != OMCI_IPC_RET_SUCCESS))
    {
        cmsLog_error("omciIpc_sendDebugData() failed, ret=%d", ipcRet);
    }
}

#ifdef OMCI_DEBUG_CMD

/***************************************************************************
 * Function Name: cmdOmciIpcDebugGetMe
 * Description  : OMCI IPC get ME.
 ***************************************************************************/
void cmdOmciIpcDebugGetMe(UINT32 oid, UINT32 meId)
{
    OmciGenObject *genObjP = NULL;
    int retCode;
    int ipcRet = 0;

    ipcRet = omciIpc_getMeObj(oid, meId, (void**)&genObjP, &retCode, NULL);
    if (ipcRet >= 0)
    {
        printf("oid = %u, meId = %u, retcode = %d\n", oid, meId, retCode);
    }

    cmsMem_free((void*)genObjP);
}

/***************************************************************************
 * Function Name: cmdOmciIpcDebugGetNextMe
 * Description  : OMCI IPC get next ME.
 ***************************************************************************/
void cmdOmciIpcDebugGetNextMe(UINT32 oid)
{
    UINT32 meId;
    OmciGenObject *genObjP = NULL;
    int retCode;
    int ipcRet = 0;

    meId = INVALID_MEID;
    while (ipcRet >= 0)
    {
        ipcRet = omciIpc_getMeObjNext(oid, meId, (void**)&genObjP,
          &retCode, NULL);
        if (ipcRet >= 0)
        {
            meId = genObjP->managedEntityId;
            printf("oid = %u, meId = %u, retcode = %d\n",
              oid, meId, retCode);
            cmsMem_free((void*)genObjP);
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciIpcDebugSetMe
 * Description  : OMCI IPC set ME.
 * Examples: (assuming oid 2103 is MDMOID_ONT_G (ME class 256)
 *     omci medebug --setme 2103 0 2 yyy
 *     omci medebug --setme 2103 0 4 5
 *     omci medebug --setme 2103 0 4 5
 *     omci debug --dump 256
 ***************************************************************************/
void cmdOmciIpcDebugSetMe(UINT32 oid, UINT32 meId, UINT32 index, void *dataP)
{
    OmciGenObject *genObjP = NULL;
    int retCode;
    omciObjInfo_t info;
    OmciParamInfo_t *paramInfoList = NULL;
    SINT32 paramNum = 0;
    int ipcRet = 0;
    UINT8 *xp;

    ipcRet = omciIpc_getMeObj(oid, meId, (void**)&genObjP, &retCode, &info);
    if (ipcRet < 0)
    {
        printf("ME not found (oid=%u, meId=%u), retcode = %d\n",
          oid, meId, retCode);
        return;
    }

    ipcRet = omciIpc_getParamInfo(oid, &paramInfoList, &paramNum);
    if (ipcRet < 0)
    {
        cmsLog_error("get param info for object oid %d failed", oid);
        goto out1;
    }

    if (index > (UINT32)paramNum)
    {
        cmsLog_error("Invalid index %u, max %u", index, paramNum);
        goto out1;
    }

    if (isParamPtrType(paramInfoList[index].type) == TRUE)
    {
        xp = ((UINT8*)genObjP) + paramInfoList[index].offsetInObject;
        *(UINT8**)xp = (UINT8*)dataP;
    }
    else if (isParamSize32bit(paramInfoList[index].type) == TRUE)
    {
        xp = ((UINT8*)genObjP) + paramInfoList[index].offsetInObject;
        *(UINT32*)xp = (UINT32)strtoul(dataP, (char **)NULL, 0);
    }
    else if (paramInfoList[index].type == _MPT_BOOLEAN)
    {
        xp = ((UINT8*)genObjP) + paramInfoList[index].offsetInObject;
        *(UBOOL8*)xp = (UINT8)strtoul(dataP, (char **)NULL, 0);
    }
    else
    {
        printf("Set on param type %u not supported\n",
          paramInfoList[index].type);
        goto out1;
    }

    ipcRet = omciIpc_setMeObj(genObjP, info.objSize, &retCode);
    if (ipcRet < 0)
    {
        printf("Set ME failed, (oid=%u, meId=%u), retcode = %d\n",
          oid, meId, retCode);
    }

out1:
    omciIpc_free(genObjP);
    omciIpc_free(paramInfoList);
}

/***************************************************************************
 * Function Name: cmdOmciIpcDebugHandler
 * Description  : IPC debug command handler.
 ***************************************************************************/
void cmdOmciIpcDebugHandler(SINT32 argc, char **argv)
{
    UINT32 oid;
    UINT32 meId;
    UINT32 index;

    if (argc < 3)
    {
        cmdOmciHelp("medebug");
        return;
    }

    if ((argc == 4) && (strcasecmp(argv[1], "--getme") == 0))
    {

        oid = strtoul(argv[2], (char**)NULL, 0);
        meId = strtoul(argv[3], (char**)NULL, 0);
        cmdOmciIpcDebugGetMe(oid, meId);
    }
    else if ((argc == 3) && (strcasecmp(argv[1], "--getnextme") == 0))
    {
        oid = strtoul(argv[2], (char**)NULL, 0);
        cmdOmciIpcDebugGetNextMe(oid);
    }
    else if ((argc == 6) && (strcasecmp(argv[1], "--setme") == 0))
    {
        oid = strtoul(argv[2], (char**)NULL, 0);
        meId = strtoul(argv[3], (char**)NULL, 0);
        index = strtoul(argv[4], (char**)NULL, 0);
        cmdOmciIpcDebugSetMe(oid, meId, index, (void*)argv[5]);
    }
    else
    {
        cmdOmciHelp("medebug");
    }
}
#endif /* OMCI_DEBUG_CMD */

/***************************************************************************
 * Function Name: cmdOmciVoiceHandler
 * Description  : Voice model command handler.
 ***************************************************************************/
void cmdOmciVoiceHandler(SINT32 argc, char **argv)
{
    UINT16 val0;

    if ((argc == 3) && (argv[1] != NULL) &&
      (strcasecmp(argv[1], "--model") == 0))
    {
        val0 = strtoul(argv[2], (char**)NULL, 0);
        if (val0 <= 1)
        {
            cmdOmciVoiceModelSet(val0);
        }
    }
    else if ((argc == 2) && (argv[1] != NULL) &&
      (strcasecmp(argv[1], "--status") == 0))
    {
        cmdOmciVoiceModelGet();
    }
    else
    {
        cmdOmciHelp("voice");
    }
}

/***************************************************************************
 * Function Name: cmdOmciJoinForceForwardStatus
 * Description  : show current upstream join force forwarding option.
 ***************************************************************************/
static CmsRet cmdOmciJoinForceForwardStatus(void)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT))
      == CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            printf("   Upstream join force forwarding option:\n");
            if (obj->joinForceForward == FALSE)
            {
                printf("    (0) Forwarding based on OMCI "
                       "'unauthorized join request behaviour' attribute.\n\n");
            }
            else
            {
                printf("    (1) Force forwarding.\n\n");
            }

            cmsObj_free((void**)&obj);
        }
        else
        {
            cmsLog_error("Could not cmsObj_get OmciSystemObject, ret=%d", ret);
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciJoinForceForwardConfig
 * Description  : configure upstream join mode:
 *                TRUE - force forwarding.
 *                FALSE - forwarding based on OMCI "unauthorized join
 *                  request behaviour" attribute.
 ***************************************************************************/
static CmsRet cmdOmciJoinForceForwardConfig(UBOOL8 mode)
{
    CmsRet ret;
    InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
    BcmOmciConfigSystemObject *obj = NULL;

    if ((ret = cmsLck_acquireLockWithTimeout(CLIAPP_LOCK_TIMEOUT)) ==
      CMSRET_SUCCESS)
    {
        if ((ret = cmsObj_get(MDMOID_BCM_OMCI_CONFIG_SYSTEM, &iidStack, 0,
          (void*)&obj)) == CMSRET_SUCCESS)
        {
            obj->joinForceForward = mode;
            cmsObj_set(obj, &iidStack);
            cmsObj_free((void**)&obj);
            cmsMgm_saveConfigToFlash();
        }

        cmsLck_releaseLock();
    }
    else
    {
        cmsLog_notice("cmsLck_acquireLockWithTimeout failed ret: %d", ret);
    }

    cmdOmciJoinForceForwardStatus();

    return ret;
}

/***************************************************************************
 * Function Name: cmdOmciMcastHandler
 * Description  : OMCI multicast command handler.
 ***************************************************************************/
void cmdOmciMcastHandler(SINT32 argc __attribute__((unused)), char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("mcast");
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("mcast");
    }
    else if (strcasecmp(argv[1], "--status") == 0)
    {
        cmdOmciMcastHostctrlStatus();
        cmdOmciJoinForceForwardStatus();
    }
    else if (strcasecmp(argv[1], "--hostctrl") == 0)
    {
        if (argv[2] == NULL)
        {
            cmsLog_error("Multicast: Missing arguments\n");
            cmdOmciHelp("mcast");
        }
        else
        {
            if (strcasecmp(argv[2], "on") != 0 &&
              strcasecmp(argv[2], "off") != 0)
            {
                cmsLog_error("Multicast: Invalid argument '%s'\n", argv[2]);
                cmdOmciHelp("mcast");
            }
            else
            {
                if (strcasecmp(argv[2], "on") == 0)
                {
                    cmdOmciMcastHostctrlConfig(TRUE);
                }
                else if (strcasecmp(argv[2], "off") == 0)
                {
                    cmdOmciMcastHostctrlConfig(FALSE);
                }
            }
        }
    }
    else if (strcasecmp(argv[1], "--joinfwd") == 0)
    {
        if (argv[2] == NULL)
        {
            cmsLog_error("Multicast: Missing arguments\n");
            cmdOmciHelp("mcast");
        }
        else
        {
            if (strcasecmp(argv[2], "on") != 0 &&
              strcasecmp(argv[2], "off") != 0)
            {
                cmsLog_error("Multicast: Invalid argument '%s'\n", argv[2]);
                cmdOmciHelp("mcast");
            }
            else
            {
                if (strcasecmp(argv[2], "on") == 0)
                {
                    cmdOmciJoinForceForwardConfig(TRUE);
                }
                else if (strcasecmp(argv[2], "off") == 0)
                {
                    cmdOmciJoinForceForwardConfig(FALSE);
                }
            }
        }
    }
}

/***************************************************************************
 * Function Name: cmdOmciSetTable
 * Description  : Generate OMCI Set table request message.
 ***************************************************************************/
static void cmdOmciSetTable(UINT16 tcId, UINT16 meClass, UINT16 meInst,
  UINT16 index, UINT8 *bufP, UINT32 bufSize)
{
    UINT16 msgSize = sizeof(CmsMsgHeader) + sizeof(omciPacket);
    UINT16 meAttrMask = 0;
    char buf[msgSize];
    CmsMsgHeader *msgP = (CmsMsgHeader*)buf;
    CmsMsgHeader *respMsg;
    omciPacket *packetP = (omciPacket*)(msgP + 1);
    UINT32 crc32;
    CmsRet ret = CMSRET_SUCCESS;

    memset(buf, 0x0, msgSize);
    msgP->type = CMS_MSG_OMCI_COMMAND_REQUEST;
    msgP->src = EID_CONSOLED;
    msgP->dst = EID_OMCID;
    msgP->dataLength = sizeof(omciPacket);
    msgP->flags_event = 0;
    msgP->flags_request = 0;
    msgP->sequenceNumber = 0;

    OMCI_HTONS(&packetP->tcId, tcId);
    packetP->msgType = OMCI_MSG_TYPE_AR(OMCI_MSG_TYPE_SETTABLE);
    packetP->devId = OMCI_PACKET_DEV_ID_B;
    OMCI_HTONS(&packetP->classNo, meClass);
    OMCI_HTONS(&packetP->instId, meInst);

    OMCI_HTONS(&packetP->B.msgLen, bufSize + sizeof(meAttrMask));
    meAttrMask = 0x8000 >> index;
    OMCI_HTONS(&packetP->B.msg[0], meAttrMask);
    memcpy(&packetP->B.msg[OMCI_SET_OVERHEAD], bufP, bufSize);

    packetP->src_eid = EID_CONSOLED;
    crc32 = omciUtl_getCrc32(-1, (char*)packetP,
      OMCI_PACKET_SIZE(packetP) - OMCI_PACKET_MIC_SIZE);
    OMCI_HTONL(OMCI_PACKET_CRC(packetP), crc32);
    ret = cmsMsg_send(cliPrvtMsgHandle, msgP);
    ret = (ret == CMSRET_SUCCESS) ? cmsMsg_receiveWithTimeout(cliPrvtMsgHandle,
      &respMsg, MSG_RECV_TIMEOUT_MS) : ret;
    if (ret == CMSRET_SUCCESS)
    {
        if (respMsg->type != CMS_MSG_OMCI_COMMAND_RESPONSE)
        {
            cmsLog_error("Failed to receive rsp, type=0x%x", respMsg->type);
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(respMsg);
    }
    else
    {
        cmsLog_error("cmsMsg_send() or cmsMsg_receiveWithTimeout() failure, "
          "ret=%d", ret);
    }
}

/***************************************************************************
 * Function Name: cmdOmciSetTableHandler
 * Description  : OMCI Set table command handler.
 ***************************************************************************/
void cmdOmciSetTableHandler(SINT32 argc, char *argv[])
{
    UINT16 val0;
    UINT16 val1;
    UINT16 val2;
    UINT16 val3;
    UINT8 *buf = NULL;
    UINT32 bufSize;
    CmsRet ret = CMSRET_SUCCESS;

    if ((argc == 11) &&
      (strcasecmp(argv[1], "--tcId") == 0) &&
      (strcasecmp(argv[3], "--meclass") == 0) &&
      (strcasecmp(argv[5], "--meid") == 0) &&
      (strcasecmp(argv[7], "--index") == 0) &&
      (strcasecmp(argv[9], "--val") == 0))
    {
        val0 = strtoul(argv[2], (char**)NULL, 0);
        val1 = strtoul(argv[4], (char**)NULL, 0);
        val2 = strtoul(argv[6], (char**)NULL, 0);
        val3 = strtoul(argv[8], (char**)NULL, 0);
        ret = cmsUtl_hexStringToBinaryBuf(argv[10], &buf, &bufSize);
        if (ret == CMSRET_SUCCESS)
        {
            cmdOmciSetTable(val0, val1, val2, val3, buf, bufSize);
        }
        else
        {
            cmsLog_error("Invalid hex string, ret=%d", ret);
        }
        CMSMEM_FREE_BUF_AND_NULL_PTR(buf);
    }
    else
    {
        cmdOmciHelp("settable");
    }
}

/***************************************************************************
 * Function Name: processOmciCmd
 * Description  : Parses OMCI commands
 ***************************************************************************/
void processOmciCmd(char *cmdLine)
{
    SINT32 argc = 0;
    char *argv[MAX_OPTS]={NULL};
    char *last = NULL;
    char cmdLineNew[OMCI_DEBUG_MSG_DATA_LEN_MAX];

    if (cli_gpon_tcont_max == 0)
    {
        rdpa_user_system_resources system_resources;
        int ret;

        if ((ret = get_rdpa_user_system_resources(&system_resources)))
        {
            cmsLog_error("get_rdpa_user_system_resources() failed, using default cli_gpon_tcont_max=%d", cli_gpon_tcont_max);
        }
        else
        {
            cli_gpon_tcont_max = system_resources.num_tcont - 1;
        }
    }

    strncpy(cmdLineNew, cmdLine, OMCI_DEBUG_MSG_DATA_LEN_MAX - 1);
    /* parse the command line and build the argument vector */
    argv[0] = strtok_r(cmdLine, " ", &last);

    if (argv[0] != NULL)
    {
        for (argc=1; argc<MAX_OPTS; ++argc)
        {
            argv[argc] = strtok_r(NULL, " ", &last);

            if (argv[argc] == NULL)
            {
                break;
            }

            cmsLog_debug("arg[%d]=%s", argc, argv[argc]);
        }
    }

    if (NULL == argv[0])
    {
        cmdOmciHelp(NULL);
    }
    else if (strcasecmp(argv[0], "--help") == 0)
    {
        cmdOmciHelp(argv[0]);
    }
    /* Troubleshooting commands. */
    else if (strcasecmp(argv[0], "capture") == 0)
    {
        cmdOmciCapture(argv[1], argv[2], argv[3], argv[4], argv[5]);
    }
    else if (strcasecmp(argv[0], "ddi") == 0)
    {
        if (argc < 2)
        {
            cmdOmciHelp("ddi");
        }
        else if (strcasecmp(argv[1], "--help") == 0)
        {
            cmdOmciHelp("ddi");
        }
        else
        {
            /* Skip the "ddi" command. */
            cmdOmciDdiHandler(&cmdLineNew[4]);
        }
    }
    else if (strcasecmp(argv[0], "debug") == 0)
    {
        if (argv[1] == NULL)
            cmdOmciHelp("debug");
        else if (strcasecmp(argv[1], "--mkerr") == 0)
            cmdOmciDgbMkErr(argv[2]);
        else if (strcasecmp(argv[1], "--help") == 0)
            cmdOmciHelp("debug");
        else if (strcasecmp(argv[1], "--status") == 0)
            cmdOmciDebugStatus();
        else if (strcasecmp(argv[1], "--dump") == 0)
        {
            if (argv[2] == NULL)
            {
                cmsLog_error("Debug: Missing arguments\n");
                cmdOmciHelp("debug");
            }
            else
                cmdOmciDebugDump(argv[2]);
        }
        else if (strcasecmp(argv[1], "--module") == 0)
        {
            if (argc < 5 ||
                argv[2] == NULL ||
                argv[3] == NULL ||
                argv[4] == NULL)
            {
                cmsLog_error("Debug: Missing arguments\n");
                cmdOmciHelp("debug");
            }
            else if (strcasecmp(argv[2], "all") != 0 &&
                strcasecmp(argv[2], "omci") != 0 &&
                strcasecmp(argv[2], "model") != 0 &&
                strcasecmp(argv[2], "vlan") != 0 &&
                strcasecmp(argv[2], "mib") != 0 &&
                strcasecmp(argv[2], "flow") != 0 &&
                strcasecmp(argv[2], "rule") != 0 &&
                strcasecmp(argv[2], "mcast") != 0 &&
                strcasecmp(argv[2], "voice") != 0 &&
                strcasecmp(argv[2], "file") != 0)
            {
                cmsLog_error("Debug: Invalid argument '%s'\n", argv[2]);
                cmdOmciHelp("debug");
            }
            else if (strcasecmp(argv[3], "--state") != 0)
            {
                cmsLog_error("Debug: Invalid argument '%s'\n", argv[3]);
                cmdOmciHelp("debug");
            }
            else if (strcasecmp(argv[4], "on") != 0 &&
                strcasecmp(argv[4], "off") != 0)
            {
                cmsLog_error("Debug: Invalid argument '%s'\n", argv[4]);
                cmdOmciHelp("debug");
            }
            else
                cmdOmciDebug(argv[2], argv[4]);
        }
        else if (strcasecmp(argv[1], "--info") == 0)
        {
            if (argc < 3)
            {
                cmsLog_error("Debug: Missing arguments\n");
                cmdOmciHelp("debug");
            }
            else
            {
                cmdOmciDebugInfo(argv[2]);
            }
        }
        else
        {
            cmsLog_error("Debug: Invalid argument '%s'\n", argv[1]);
            cmdOmciHelp("debug");
        }
    }
    else if (strcasecmp(argv[0], "grab") == 0)
    {
        cmdOmciGrab(argv[1], argv[2], argv[3], argv[4]);
    }
#ifdef OMCI_DEBUG_CMD
    else if (strcasecmp(argv[0], "medebug") == 0)
    {
        cmdOmciIpcDebugHandler(argc, argv);
    }
#endif /* OMCI_DEBUG_CMD */
    else if (strcasecmp(argv[0], "rawmode") == 0)
    {
        if (argv[1] == NULL)
            cmdOmciHelp("rawmode");
        else if (strcasecmp(argv[1], "--help") == 0)
            cmdOmciHelp("rawmode");
        else if (strcasecmp(argv[1], "--status") == 0)
            cmdOmciRawModeStatus();
        else if (strcasecmp(argv[1], "--mode") == 0)
        {
            if (argv[2] == NULL)
            {
                cmsLog_error("Raw mode: Missing arguments\n");
                cmdOmciHelp("rawmode");
            }
            else
            {
                if (strcasecmp(argv[2], "on") != 0 &&
                    strcasecmp(argv[2], "off") != 0)
                {
                    cmsLog_error("Raw mode: Invalid argument '%s'\n", argv[2]);
                    cmdOmciHelp("rawmode");
                }
                else
                {
                    if (strcasecmp(argv[2], "on") == 0)
                    {
                        cmdOmciRawModeConfig(TRUE);
                    }
                    else if (strcasecmp(argv[2], "off") == 0)
                    {
                        cmdOmciRawModeConfig(FALSE);
                    }
                }
            }
        }
    }
    else if (strcasecmp(argv[0], "send") == 0)
    {
        cmdOmciSend(argv[1], argv[2]);
    }
    else if (strcasecmp(argv[0], "settable") == 0)
    {
        cmdOmciSetTableHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "test") == 0)
    {
        cmdOmciTestHandler(argc, argv);
    }
    /* OMCI MIB or behavior configuration. */
    else if (strcasecmp(argv[0], "auth") == 0)
    {
        cmdOmciAuthHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "bridge") == 0)
    {
        cmdOmciBridgeHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "eth") == 0)
    {
        cmdOmciEthHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "mcast") == 0)
    {
        cmdOmciMcastHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "mibsync") == 0)
    {
        if (argv[1] == NULL)
            cmdOmciHelp("mibsync");
        else if (strcasecmp(argv[1], "--help") == 0)
            cmdOmciHelp("mibsync");
        else if (strcasecmp(argv[1], "--get") == 0)
            cmdOmciMibSyncGet();
        /* To set MIB data cync: use WEB GUI OMCI configuration page. */
    }
    else if (strcasecmp(argv[0], "misc") == 0)
    {
        cmdOmciMiscHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "pm") == 0)
    {
        cmdOmciPmHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "promiscmode") == 0)
    {
        if (argv[1] == NULL)
            cmdOmciHelp("promiscmode");
        else if (strcasecmp(argv[1], "--help") == 0)
            cmdOmciHelp("promiscmode");
        else if (strcasecmp(argv[1], "--status") == 0)
            cmdOmciPromiscModeStatus();
        else if (strcasecmp(argv[1], "--mode") == 0)
        {
            if (argv[2] == NULL)
            {
                cmsLog_error("Promisc mode: Missing arguments\n");
                cmdOmciHelp("promiscmode");
            }
            else
            {
                if (strcasecmp(argv[2], "on") != 0 &&
                    strcasecmp(argv[2], "off") != 0)
                {
                    cmsLog_error("Promisc mode: Invalid argument '%s'\n", argv[2]);
                    cmdOmciHelp("promiscmode");
                }
                else
                {
                    if (strcasecmp(argv[2], "on") == 0)
                    {
                        cmdOmciPromiscModeConfig(1);
                    }
                    else if (strcasecmp(argv[2], "off") == 0)
                    {
                        cmdOmciPromiscModeConfig(0);
                    }
                }
            }
        }
    }
    else if (strcasecmp(argv[0], "tcont") == 0)
    {
        cmdOmciTcontHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "tmoption") == 0)
    {
        cmdOmciTmOptionHandler(argc, argv);
    }
    else if (strcasecmp(argv[0], "unipathmode") == 0)
    {
        if (argv[1] == NULL)
            cmdOmciHelp("unipathmode");
        else if (strcasecmp(argv[1], "--help") == 0)
            cmdOmciHelp("unipathmode");
        else if (strcasecmp(argv[1], "--status") == 0)
            cmdOmciUniPathModeStatus();
        else if (strcasecmp(argv[1], "--mode") == 0)
        {
            if (argv[2] == NULL)
            {
                cmsLog_error("UNI data path mode: Missing arguments\n");
                cmdOmciHelp("unipathmode");
            }
            else
            {
                if (strcasecmp(argv[2], "on") != 0 &&
                    strcasecmp(argv[2], "off") != 0)
                {
                    cmsLog_error("UNI data path mode: Invalid argument '%s'\n", argv[2]);
                    cmdOmciHelp("unipathmode");
                }
                else
                {
                    if (strcasecmp(argv[2], "on") == 0)
                    {
                        cmdOmciPathModeConfig(1);
                    }
                    else if (strcasecmp(argv[2], "off") == 0)
                    {
                        cmdOmciPathModeConfig(0);
                    }
                }
            }
        }
    }
#if defined(DMP_X_ITU_ORG_VOICE_1)
    else if (strcasecmp(argv[0], "voice") == 0)
    {
        cmdOmciVoiceHandler(argc, argv);
    }
#endif /* DMP_X_ITU_ORG_VOICE_1 */
    else
    {
        printf("Invalid OMCI CLI Command\n");
        cmdOmciHelp(NULL);
    }
}

/***************************************************************************
 * Function Name: cmdOmciAlarmSeq_Get
 * Description  : Prints OMCI current Alarm Sequence Number.
 ***************************************************************************/
static void cmdOmciAlarmSeq_Get(void)
{
    CmsRet cmsReturn = CMSRET_SUCCESS;
    CmsMsgHeader cmdMsgBuffer;

    // Setup CMS_MSG_OMCIPMD_ALARM_SEQ_GET command.
    memset(&cmdMsgBuffer, 0x0, sizeof(CmsMsgHeader));
    cmdMsgBuffer.type = CMS_MSG_OMCIPMD_ALARM_SEQ_GET;
    cmdMsgBuffer.src = EID_CONSOLED;
    cmdMsgBuffer.dst = EID_OMCID;
    cmdMsgBuffer.flags_request = 1;
    cmdMsgBuffer.dataLength = 0;

    // Attempt to send CMS message & test result.
    cmsReturn = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)&cmdMsgBuffer);
    if (cmsReturn != CMSRET_SUCCESS)
    {
        // Log error.
        cmsLog_error("Send message failure, cmsReturn: %d", cmsReturn);
    }
}

/***************************************************************************
 * Function Name: cmdOmciAlarmSeq_Get
 * Description  : Sets OMCI new Alarm Sequence Number from input arg.
 ***************************************************************************/
static void cmdOmciAlarmSeq_Set(char *numValStr)
{
    CmsRet cmsReturn = CMSRET_SUCCESS;
    CmsMsgHeader cmdMsgBuffer;
    int seqVal = 0;

    // Test for valid pointer.
    if (numValStr != NULL)
    {
        // Convert input string to int.
        seqVal = atoi(numValStr);
    }

    // Test for valid range.
    if ((seqVal > 0) && (seqVal < 256))
    {
        // Setup CMS_MSG_OMCIPMD_ALARM_SEQ_SET command.
        memset(&cmdMsgBuffer, 0x0, sizeof(CmsMsgHeader));
        cmdMsgBuffer.type = CMS_MSG_OMCIPMD_ALARM_SEQ_SET;
        cmdMsgBuffer.src = EID_CONSOLED;
        cmdMsgBuffer.dst = EID_OMCID;
        cmdMsgBuffer.flags_request = 1;
        cmdMsgBuffer.dataLength = 0;
        cmdMsgBuffer.wordData = (UINT32)seqVal;

        // Attempt to send CMS message & test result.
        cmsReturn = cmsMsg_send(cliPrvtMsgHandle, (CmsMsgHeader*)&cmdMsgBuffer);
        if (cmsReturn != CMSRET_SUCCESS)
        {
          // Log error.
          cmsLog_error("Send message failure, cmsReturn: %d", cmsReturn);
        }
    }
    else
    {
        printf("*** cmdOmciAlarmSeq_Set error valid range <1...255> ***\n");
    }
}

/***************************************************************************
 * Function Name: cmdOmciPmHandler
 * Description  : Parses OMCIPM commands
 ***************************************************************************/
static void cmdOmciPmHandler(SINT32 argc, char *argv[])
{
    if (argv[1] == NULL)
    {
        cmdOmciHelp("pm");
    }
    else if (strcasecmp(argv[1], "--getAlarmSeq") == 0)
    {
        cmdOmciAlarmSeq_Get();
    }
    else if (strcasecmp(argv[1], "--setAlarmSeq") == 0)
    {
        if ((argc != 3) || (argv[2] == NULL))
        {
            cmsLog_error("Pm: Missing arguments\n");
            cmdOmciHelp("pm");
        }
        cmdOmciAlarmSeq_Set(argv[2]);
    }
    else if (strcasecmp(argv[1], "--help") == 0)
    {
        cmdOmciHelp("pm");
    }
    else
    {
        printf("Invalid OMCIPM CLI Command\n");
        cmdOmciHelp("pm");
    }
}

#endif /* SUPPORT_OMCI */

#endif /* SUPPORT_CLI_CMD */
