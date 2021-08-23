/*
* <:copyright-BRCM:2016:proprietary:standard
*
*    Copyright (c) 2016 Broadcom
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
*
****************************************************************************
*
*  Filename: cli2_voice.c
*
****************************************************************************
*  Description:
*
*
****************************************************************************/

/****************************************************************************
*
*  cli2_voice.c
*
*  PURPOSE:
*
*  NOTES:
*
****************************************************************************/

#ifdef SUPPORT_CLI_CMD

#ifdef BRCM_VOICE_SUPPORT
#ifdef DMP_VOICE_SERVICE_2

/* ---- Include Files ---------------------------------------------------- */
#include "cms_util.h"
#include "cms_core.h"
#include "cms_cli.h"
#include "cli.h"
#include "cms_msg.h"
#include "dal_voice.h"


/* ---- Public Variables ------------------------------------------------- */

/* ---- Constants and Types ---------------------------------------------- */

#define MAX_CLICMD_NAME_SIZE     30

enum {
   BITFLAGS_ALWAYS  = 0x01, /* ALWAYS */
   BITFLAGS_SIP     = 0x02, /* SIP */
   BITFLAGS_GPON    = 0x04, /* GPON */
   BITFLAGS_IMS     = 0x08, /* IMS */
   BITFLAGS_RFC3261 = 0x10, /* RFC3261 */
};

/* DAL function <--> set parameter mapping element */
typedef struct  {
   const char *name;
   CmsRet (*dalFunc)( DAL_VOICE_PARMS *args, char *value );
   CmsRet (*convertFunc)( DAL_VOICE_PARMS *args, int *value ); /* convert second parameter from number to instance */
   int numArgs;
   UBOOL8 global;     /* Indicates a parameter as per voice profile for MDM but a global parameter for Call manager */
   UINT8 bitFlags;    /* Indicates which options the CLI command is for */
   const char *info;
   const char *syntax;
} VOICE_DAL_MAP;

/* ---- Private Variables ------------------------------------------------ */

static UINT8 uActiveBitFlags = BITFLAGS_ALWAYS
#ifdef SIPLOAD
                               | BITFLAGS_SIP
#endif
#ifdef DMP_X_ITU_ORG_GPON_1
                               | BITFLAGS_GPON
#endif
                               ;

/* Table of DAL function mapping */
static VOICE_DAL_MAP voiceDalCliMap[] =
{

   { "-------------",   NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------", "---- Global parameters ----"
   },

   { "cctktracelvl",    dalVoice_SetCCTKTraceLevel,      NULL,
     1, TRUE, BITFLAGS_SIP,
     "CCTK tracelevel", "<Info|Warn|Debug|Off>"
   },

   { "cctktracegrp",    dalVoice_SetCCTKTraceGroup,      NULL,
     1, TRUE, BITFLAGS_SIP,
     "CCTK concat tracegroups", "<CCTK|SCE|Trans|SDP|SIP|Misc|All|None>"
   },

   { "mgtProt",        dalVoice_SetManagementProtocol,   NULL,
     1, TRUE, BITFLAGS_GPON,
     "Protocol used to manage Voice", "<TR69|OMCI>"
   },

   { "loglevel",       dalVoice_SetLoggingLevel,       NULL,
     2, TRUE, BITFLAGS_ALWAYS,
     "Voice module-specific log level", "<general|dsphal|slicslac|cmgr|disp|sipcctk|bos|ept|cms|prov|lhapi> <0-7>"
   },

   { "defaults",
     dalVoice_SetDefaults,       NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "Default VoIP setup", "<None>"
   },

   { "boundIfname",
     dalVoice_SetBoundIfName,    NULL,
     2, FALSE, BITFLAGS_ALWAYS,
     "voice network interface", "<srvPrv#> <LAN|Any_WAN|(WAN IfName, e.g. nas_0_0_35)>"
   },

   { "ipAddrFamily",
     dalVoice_SetIpFamily,       NULL,
     2, FALSE, BITFLAGS_ALWAYS,
     "IP address family", "<srvPrv#> <IPv4|IPv6>"
   },

   { "voiceDnsEnable",      dalVoice_SetVoiceDnsEnable,    NULL,
     1, TRUE, BITFLAGS_SIP,
     "Voice DNS Enable", "<on|off>"
   },

   { "voiceDnsAddrPri",     dalVoice_SetDnsServerAddrPri,  NULL,
     1, TRUE, BITFLAGS_SIP,
     "Primary Voice DNS IP address", "<IP>"
   },

   { "voiceDnsAddrSec",     dalVoice_SetDnsServerAddrSec,  NULL,
     1, TRUE, BITFLAGS_SIP,
     "Secondary Voice DNS IP address", "<IP>" },

   /* Service provider specific parameters */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------",  "---- Voip Service parameters ----"
   },

   { "locale",      dalVoice_SetRegion,         NULL,
     2, FALSE, BITFLAGS_ALWAYS,
     "2 or 3 character code", "<srvPrv#> <region>" },

   { "digitMap",    dalVoice_SetDigitMap,       NULL,
     2, FALSE, BITFLAGS_ALWAYS,
     "Digit map setting", "<srvPrv#> <digit map>" },

   { "attachNetwork",  dalVoice_SetSipClientNetworkAssocIdx,   NULL,
     3, FALSE, BITFLAGS_ALWAYS,
     "Associate client# to network#", "<srvPrv#> <client#> <network#>" },

   { "attachVoipProf",  dalVoice_SetSipNetworkVoipProfileAssocIdx,   dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Associate network# to voip profile#", "<srvPrv#> <network#> <profile#>" },

   /* sip network setting */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_SIP,
     "-------------",  "---- SIP Network parameters ----"
   },

   { "transport",
     dalVoice_SetSipTransport,    dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "transport protocol", "<srvPrv#> <network#> <UDP|TCP|TLS>"
   },

   { "sipUriForTls",
     dalVoice_SetSipUriForTls,   dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "Use SIP URI for TLS",         "<srvPrv#> <network#> <true|false>"
   },

   { "sipMode",
     dalVoice_SetSipMode,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP mode", "<srvPrv#> <network#> <RFC3261|IMS>"
   },

   { "sipDomain",
     dalVoice_SetSipUserAgentDomain,    dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP user agent domain",  "<srvPrv#> <network#> <CPE_domainName>"
   },

   { "sipPort",
     dalVoice_SetSipUserAgentPort,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP user agent port",       "<srvPrv#> <network#> <port>"
   },

   { "proxy",
     dalVoice_SetSipProxyServer,     dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP proxy server",   "<srvPrv#> <network#> <hostName|IP>"
   },

   { "proxyPort",
     dalVoice_SetSipProxyServerPort,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP proxy server port", "<srvPrv#> <network#> <port>"
   },

   { "obProxy",
     dalVoice_SetSipOutboundProxy,   dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP outbound proxy",  "<srvPrv#> <network#> <hostName|IP>"
   },

   { "obProxyPort",
     dalVoice_SetSipOutboundProxyPort,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP outbound proxy port",  "<srvPrv#> <network#> <port>"
   },

   { "reg",
     dalVoice_SetSipRegistrarServer,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP registrar server", "<srvPrv#> <network#> <hostName|IP>"
   },

   { "regPort",
     dalVoice_SetSipRegistrarServerPort,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP registrar server port",  "<srvPrv#> <network#> <port>"
   },

   { "secSipDomain",
     dalVoice_SetSipSecDomainName, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP Secondary domain name",   "<srvPrv#> <network#> <CPE_domainName>"
   },

   { "secProxy",
     dalVoice_SetSipSecProxyAddr, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary proxy IP",   "<srvPrv#> <network#> <IP>"
   },

   { "secProxyPort",
     dalVoice_SetSipSecProxyPort, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary proxy port",  "<srvPrv#> <network#> <port>"
   },

   { "secObProxy",
     dalVoice_SetSipSecOutboundProxyAddr, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary outbound proxy IP",   "<srvPrv#> <network#> <IP>"
   },

   { "secObProxyPort",
     dalVoice_SetSipSecOutboundProxyPort, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary outbound proxy port",  "<srvPrv#> <network#> <port>"
   },

   { "secReg",
     dalVoice_SetSipSecRegistrarAddr, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary registrar IP",   "<srvPrv#> <network#> <IP>"
   },

   { "secRegPort",
     dalVoice_SetSipSecRegistrarPort, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_RFC3261,
     "SIP Secondary registrar port",  "<srvPrv#> <network#> <port>"
   },

   { "pcscf",
     dalVoice_SetSipOutboundProxy,   dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_IMS,
     "SIP PCSCF",  "<srvPrv#> <network#> <hostName|IP>"
   },

   { "pcscfPort",
     dalVoice_SetSipOutboundProxyPort,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_IMS,
     "SIP PCSCF port",  "<srvPrv#> <network#> <port>"
   },

   { "secPcscf",
     dalVoice_SetSipSecOutboundProxyAddr, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_IMS,
     "SIP Secondary PCSCF IP",   "<srvPrv#> <network#> <IP>"
   },

   { "secPcscfPort",
     dalVoice_SetSipSecOutboundProxyPort, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP | BITFLAGS_IMS,
     "SIP Secondary PCSCF port",  "<srvPrv#> <network#> <port>"
   },

   { "realm",
     dalVoice_SetSipRealm, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP realm", "<srvPrv#> <network#> <realm>"
   },

   { "failoverEnable",
     dalVoice_SetSipFailoverEnable, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP failover enable", "<srvPrv#> <network#> <on|off>"
   },

   { "backToPrimary",
     dalVoice_SetSipBackToPrimOption, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "back-to-primary option", "<srvPrv#> <network#> <Disabled|Silent|Deregistration|SilentDeregistration>"
   },

   { "sipOptions",
     dalVoice_SetSipOptionsEnable,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP OPTIONS ping enable", "<srvPrv#> <network#> <on|off>"
   },

   { "confURI",
     dalVoice_SetSipConferencingURI,    dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP conferencing URI",      "<srvPrv#> <network#> <hostName>"
   },

   { "confOption",
     dalVoice_SetSipConferencingOption,  dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP conferencing option", "<srvPrv#> <network#> <Local|ReferParticipants|ReferServer>"
   },

   { "codecList",
     dalVoice_SetSipNetworkCodecList,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "codec priority list",         "<srvPrv#> <network#> <codec(1)[,codec(2)]>"
   },

   { "sipDSCPMark",
     dalVoice_SetSipDSCPMark,    dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP outgoing DSCP mark",   "<srvPrv#> <network#> <mark>"
   },

   { "timerT1",
     dalVoice_SetSipTimerT1,     dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol T1 timer",    "<srvPrv#> <network#> <time in ms>"
   },

   { "timerT2",
     dalVoice_SetSipTimerT2,     dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol T2 timer",    "<srvPrv#> <network#> <time in ms>"
   },

   { "timerT4",
     dalVoice_SetSipTimerT4,     dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol T4 timer",    "<srvPrv#> <network#> <time in ms>"
   },

   { "timerB",
     dalVoice_SetSipTimerB,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol B timer",     "<srvPrv#> <network#> <time in ms>"
   },

   { "timerD",
     dalVoice_SetSipTimerD,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol D timer",     "<srvPrv#> <network#> <time in ms>"
   },

   { "timerF",
     dalVoice_SetSipTimerF,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol F timer",     "<srvPrv#> <network#> <time in ms>"
   },

   { "timerH",
     dalVoice_SetSipTimerH,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol H timer",     "<srvPrv#> <network#> <time in ms>"
   },

   { "timerJ",
     dalVoice_SetSipTimerJ,      dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP protocol J timer",     "<srvPrv#> <network#> <time in ms>"
   },

   { "inviteExpires",
     dalVoice_SetSipInviteExpires, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP Invite Expires",         "<srvPrv#> <network#> <time in ms>"
   },

   { "regRetryInt",
     dalVoice_SetSipRegisterRetryInterval, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP register retryinterval",   "<srvPrv#> <network#> <seconds>"
   },

   { "regExpires",
     dalVoice_SetSipRegisterExpires, dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "Register expires hdr val",    "<srvPrv#> <network#> <seconds>"
   },

   { "tagMatching",
     dalVoice_SetSipToTagMatching,   dalVoice_mapNetworkNumToInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP to tag matching",         "<srvPrv#> <on|off>"
   },

   /* sip client parameters */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_SIP,
     "-------------",  "---- SIP client parameters ----"
   },

   { "lineStatus",
     dalVoice_SetVlEnable,          dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "Activate sip line",      "<srvPrv#> <client#> <on|off>"
   },

   { "extension",
     dalVoice_SetVlSipURI,          dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP extension",    "<srvPrv#> <client#> <URI>"
   },

   { "dispName",
     dalVoice_SetVlCFCallerIDName,  dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP Display Name",  "<srvPrv#> <client#> <Name>"
   },

   { "authName",
     dalVoice_SetVlSipAuthUserName, dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP auth name",   "<srvPrv#> <client#> <name>"
   },

   { "authPwd",
     dalVoice_SetVlSipAuthPassword, dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "SIP auth password",   "<srvPrv#> <client#> <pwd>"
   },

   { "T38",
     dalVoice_SetT38Enable,         dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "enable/disable T38",  "<srvPrv#> <client#> on|off"
   },

   { "Contact",
     dalVoice_SetSipContactUri,     dalVoice_mapAcntNumToClientInst,
     3, FALSE, BITFLAGS_SIP,
     "set override SIP contact header", "<srvPrv#> <client#> <URI>"
   },

   { "resetStats",
     dalVoice_SetCcLineResetStats,     dalVoice_mapAcntNumToLineInst2,
     3, FALSE, BITFLAGS_SIP,
     "Reset RTP and call stats",  "<srvPrv#> <client#> <on>"
   },

   /* voip profile parameters */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------",  "---- VoIP Profile parameters ----"
   },

   { "DTMFMethod",
     dalVoice_SetDTMFMethod,        dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "DTMF digit passing method",   "<srvPrv#> <profile#> <InBand|RFC4733|SIPInfo>"
   },

   { "hookFlashMethod",
     dalVoice_SetHookFlashMethod,   dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Hook flash method",            "<srvPrv#> <profile#> <SIPInfo|None>"
   },

   { "MinRtpPort",
     dalVoice_SetRtpLocalPortMin,       dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTP port min",  "<srvPrv#> <profile#> <min port>"
   },

   { "MaxRtpPort",
     dalVoice_SetRtpLocalPortMax,       dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTP port max",  "<srvPrv#> <profile#> <max port>"
   },

   { "rtpDSCPMark",
     dalVoice_SetRtpDSCPMark,       dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTP outgoing DSCP mark",  "<srvPrv#> <profile#> <mark>"
   },

   { "rtcpEnable",
     dalVoice_SetRtcpEnable,        dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTCP enable", "<srvPrv#> <profile#> <yes|no>"
   },

   { "rtcpInterval",
     dalVoice_SetRtcpInterval,      dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTCP transmit interval (ms)", "<srvPrv#> <profile#> <intervalMs>"
   },

   { "rtcpXrEnable",
     dalVoice_SetRtcpXrEnable,      dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTCP-XR enable", "<srvPrv#> <profile#> <yes|no>"
   },

   { "rtcpXrPubRepAddr",
     dalVoice_SetRtcpXrPubRepAddr,  dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "RTCP-XR publish report addr", "<srvPrv#> <profile#> <addr>"
   },

   { "srtpEnable",
     dalVoice_SetSrtpEnable,        dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "SRTP enable",       "<srvPrv#> <profile#> <yes|no>"
   },

   { "srtpOption",
     dalVoice_SetSrtpOption,        dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "SRTP usage option",       "<srvPrv#> <profile#> <Mandatory|Optional>"
   },

   { "V18",
     dalVoice_SetV18Enable,         dalVoice_mapVoipProfNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "enable/disable V.18 detection",  "<srvPrv#> <profile#> on|off"
   },

   /* codec profile parameters */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------",  "---- Codec Profile parameters ----"
   },

   { "vad",
     dalVoice_SetSilenceSuppression,  NULL,
     2, TRUE, BITFLAGS_ALWAYS,
     "enable vad",  "<srvPrv#> <on|off>"
   },

   { "pTime",
     dalVoice_SetCodecProfPacketPeriod,  dalVoice_mapCpNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "packetization period",    "<srvPrv#> <codecProfile#> <pTime>"
   },

   /* calling feature parameters */
   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------",  "---- Call Feature parameters ----"
   },

   { "MWIEnable",
     dalVoice_SetVlCFMWIEnable,     dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Msg Waiting Indication",  "<srvPrv#> <Feature#> <on|off>"
   },
   { "cfwdNum",
     dalVoice_SetVlCFCallFwdNum,    dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call forward number",     "<srvPrv#> <Feature#> <number>"
   },

   { "cfwdAll",
     dalVoice_SetVlCFCallFwdAll,    dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call forward all",         "<srvPrv#> <feature#> <on|off>"
   },

   { "cfwdNoAns",
     dalVoice_SetVlCFCallFwdNoAns,  dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call forward no answer",   "<srvPrv#> <feature#> <on|off>"
   },

   { "cfwdBusy",
     dalVoice_SetVlCFCallFwdBusy,   dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call forward busy",         "<srvPrv#> <feature#> <on|off>"
   },

   { "callWait",
     dalVoice_SetVlCFCallWaiting,   dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call waiting",               "<srvPrv#> <feature#> <on|off>"
   },

   { "callreturn",
     dalVoice_SetVlCFCallReturn,   dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call return",               "<srvPrv#> <feature#> <on|off>"
   },

   { "calltf",
     dalVoice_SetVlCFCallTransfer, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "call transfer",               "<srvPrv#> <feature#> <on|off>"
   },

   { "anonBlck",
     dalVoice_SetVlCFAnonCallBlck,  dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Anonymous call rcv blcking", "<srvPrv#> <feature#> <on|off>"
   },

   { "anonCall",
     dalVoice_SetVlCFAnonymousCalling,  dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Anonymous outgng calls",      "<srvPrv#> <feature#> <on|off>"
   },

   { "DND",
     dalVoice_SetVlCFDoNotDisturb,  dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "do not disturb",      "<srvPrv#> <feature#> <on|off>"
   },

   { "warmLine",
     dalVoice_SetVlCFWarmLine ,        dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Warm line",          "<srvPrv#> <feature#> <on|off>"
   },

   { "warmLineDelay",
     dalVoice_SetWarmLineOffhookTimer,  dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Warm line offhook delay timer",  "<srvPrv#> <feature#> <delay(sec)>"
   },

   { "warmLineNum",
     dalVoice_SetVlCFWarmLineNum,       dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Warm line number",   "<srvPrv#> <feature#> <number>"
   },

   { "callBarring",
     dalVoice_SetVlCFCallBarring,        dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Call barring",       "<srvPrv#> <feature#> <on|off>"
   },

   { "callBarrPin",
     dalVoice_SetVlCFCallBarringPin,      dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Call barring pin",   "<srvPrv#> <feature#> <number>"
   },

   { "callBarrDigMap",
     dalVoice_SetVlCFCallBarringDigitMap, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Call barring digit map",   "<srvPrv#> <feature#> <digitmap>"
   },

   { "netPrivacy",      dalVoice_SetVlCFNetworkPrivacy,      dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Network privacy",           "<srvPrv#> <feature#> <on|off>"
   },

   { "callredial",      dalVoice_SetVlCFCallRedial,      dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Call Redial",           "<srvPrv#> <feature#> <on|off>"
   },

   { "euroFlashEnable",
     dalVoice_SetEuroFlashEnable, NULL,
     2, FALSE, BITFLAGS_ALWAYS,
     "European flash enable", "<srvPrv#> <on|off>"
   },

   { "CCBSEnble",
     dalVoice_SetCCBSEnable, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Call complete for busy Subscriber",   "<srvPrv#> <feature#> <yes/no>"
   },

   { "cidNumEnable",    dalVoice_SetVlCFCallId, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Caller ID Number Enable", "<srvPrv#> <feature#> <on|off>"
   },

   { "cidNameEnable",   dalVoice_SetVlCFCallIdName, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Caller ID Name Enable", "<srvPrv#> <feature#> <on|off>"
   },

   { "emergDSCP",   dalVoice_SetEServiceDSCPMark, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "emergency DSCP mark", "<srvPrv#> <feature#> <dscp (1-63)>"
   },

   { "nwHoldTime",   dalVoice_SetEServiceNwHoldTime, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "network hold time", "<srvPrv#> <feature#> <time(mins)>"
   },

   { "nwHoldDisable",   dalVoice_SetEServiceNwHoldDisable, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Network hold disable", "<srvPrv#> <feature#> <yes|no>"
   },

   { "nwHoldbypass",   dalVoice_SetEServiceNwHoldBypass, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Network hold bypass", "<srvPrv#> <feature#> <yes|no>"
   },

   { "noLocInfo",   dalVoice_SetEServiceNoLocInfo, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "No emergency location info", "<srvPrv#> <feature#> <yes|no>"
   },

   { "emerg3wc",    dalVoice_SetEServiceAllow3WayCall, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Allow 3way call in emergency call", "<srvPrv#> <feature#> <yes|no>"
   },

   { "endAllCall",   dalVoice_SetEServiceEndAllCall, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Drop current call and accept incoming emergency call", "<srvPrv#> <feature#> <yes|no>"
   },

   { "emergHowlerDur",  dalVoice_SetEServiceHowlerDuration, dalVoice_mapCallFeatureSetNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "Emergency howler tone duration", "<srvPrv#> <feature#> <time(seconds)>"
   },

   { "-------------",
     NULL,          NULL,
     0, FALSE, BITFLAGS_ALWAYS,
     "-------------",  "---- POTS FXS parameters ----"
   },

   { "rxGain",          dalVoice_SetVoiceFxsLineRxGainStr,         dalVoice_mapPotsFxsNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "rxGain (dB)",     "<srvPrv#> <FXS#> <rxGain>"
   },

   { "txGain",          dalVoice_SetVoiceFxsLineTxGainStr,        dalVoice_mapPotsFxsNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "txGain (dB)",     "<srvPrv#> <FXS#> <txGain>"
   },

   { "linetest",        dalVoice_SetFxsLineTest,         dalVoice_mapPotsFxsNumToInst,
     3, FALSE, BITFLAGS_ALWAYS,
     "MLT test",         "<srvPrv#> <fxs#> <hazardv|foreignv|resistance|off-hook|REN>"
   },


   /* call control extension parameters */
   { NULL,            NULL,                                NULL, 0, FALSE, 0, "ERROR",                              "ERROR"                                         }
};


/* ---- Private Function Prototypes -------------------------------------- */

static void voiceShowCmdSyntax();
static void processVoiceCtlSetCmd(char *cmdLine);
static void processVoiceCtlCreateCmd(char *cmdLine);
static void processVoiceCtlShowCmd(char *cmdLine);
static void processVoiceSaveCmd();
static CmsRet dumpSipNetworkParams(int spNum);
static CmsRet dumpSipClientParams(int spNum);
static CmsRet dumpVoipProfileParams(int spNum);
static CmsRet dumpVoiceCodecList(int spNum);
static CmsRet dumpCallCtlLineExtParams(int spNum);
static CmsRet dumpCallCtlMappingParams(int spNum);
static CmsRet dumpCallCtlFeatureSetParams(int spNum);
static CmsRet dumpServiceProviderParams(int spNum);
static CmsRet dumpVoiceStats(int spNum);
static CmsRet dumpVoiceCallLogs(int spNum);
static CmsRet dumpVoiceLastCallStats(int spNum);
static CmsRet dumpVoiceAllCallStats(int spNum);
static CmsRet dumpVoiceCallLogStatsHelper( DAL_VOICE_PARMS *parms );
static CmsRet dumpVoicePotsFxsDiag(int spNum);
static CmsRet dumpVoicePotsFxs(int spNum);

/* common Voice Diagnostic processing */
static void processVoiceDiagCmd(VoiceDiagType diagType, char *cmdLine);
static int buildCmsMsg(CmsMsgHeader *msg, VoiceDiagMsgBody *info, char *cmdLine);

/* Endpoint Application specific processing */
static void processEptAppCmd(char *cmdLine);

#ifdef BRCM_PROFILER_ENABLED
static void processVoiceCtlProfilerCmd(char *cmdLine);
#endif

static int initCfgPtrArray( char ** argArray, char * argBuffer );

#if DMP_EPON_VOICE_OAM || DMP_X_ITU_ORG_GPON_1
static void processVoiceSendUpldComplete(void);
#endif /* DMP_EPON_VOICE_OAM */

/* ---- Function implementations ----------------------------------------- */

static void voiceShowCmdSyntax()
{
   int i = 0;
   printf("Command syntax: \n");
   printf("voice --help                      -  show the voice command syntax\n");
   printf("voice show                        -  show the voice parameters\n");
   printf("voice show all                    -  show all voice parameters\n");
#ifdef SIPLOAD
   printf("voice show memstats               -  (DEBUG) shows memory allocation statistics\n");
   printf("voice show cctkcmstats            -  (DEBUG) shows Call Manager & CCTK statistics\n");

   printf("voice show network                -  show sip networks\n");
   printf("voice show client                 -  show sip clients\n");
   printf("voice show voipprofile            -  show voip profiles\n");
   printf("voice show codec                  -  show codec profiles\n");
   printf("voice show callctl                -  show call control line/extension\n");
   printf("voice show map                    -  show call control mapping\n");
   printf("voice show line                   -  show FXS lines\n");
   printf("voice show feature                -  show call feature\n");
   printf("voice show calllog                -  show call log summary\n");
   printf("voice show callstats              -  show RTP statistics\n");
   printf("voice show callstats lastcall     -  show call stats from the most recent call\n");
   printf("voice show callstats all          -  show call stats from all of calls in the log\n");
#endif /* SIPLOAD */

   printf("voice start                       -  start the voice application\n");
#if DMP_EPON_VOICE_OAM || DMP_X_ITU_ORG_GPON_1
   printf("voice sendUpldComplete            -  send the upload complete message to ssk\n");
#endif /* DMP_EPON_VOICE_OAM */
   printf("voice stop                        -  stop the voice application\n");
   printf("voice save                        -  store voice params to flash\n");
   printf("voice reboot                      -  restart the voice application\n");
   printf("voice set <param> <arg1> <arg2>.. -  set a provisionable parameter\n");
   printf("voice create <param>              -  create a TR-104v2 object specified by param\n");
   printf("List of voice set params and args:                                          \n");
   /* TODO: Go through DAL mapping tables and print all voice set commands */
   while( voiceDalCliMap[i].name != NULL )
   {
      if (voiceDalCliMap[i].bitFlags == (voiceDalCliMap[i].bitFlags & uActiveBitFlags))
      {
         /* left-aligned */
         printf("%-15s %-27s - %-25s\n",voiceDalCliMap[i].name, voiceDalCliMap[i].syntax, voiceDalCliMap[i].info);
      }
      i++;
   }
}

static void updateImsModeFlag(void)
{
   int spNum = 0;
   CmsRet ret = CMSRET_SUCCESS;

   /* Get number of service providers */
   cmsLck_acquireLock();
   dalVoice_GetNumSrvProv( &spNum );
   cmsLck_releaseLock();

   for (int i = 0; i < spNum; i++)
   {
      int numOfNetwork = 0, vpInst = 0;
      DAL_VOICE_PARMS parms = {0};

      /* Mapping spnum to vpInst and get number of sip networks */
      cmsLck_acquireLock();
      dalVoice_mapSpNumToSvcInst( i, &vpInst );
      parms.op[0] = vpInst;
      dalVoice_GetNumSipNetwork( &parms, &numOfNetwork );
      cmsLck_releaseLock();

      for (int j = 0; j < numOfNetwork; j++)
      {
         int networkInst = 0;

         /* Map network number to instance */
         parms.op[1] = j;
         cmsLck_acquireLock();
         ret = dalVoice_mapNetworkNumToInst( &parms , &networkInst );
         cmsLck_releaseLock();

         if( ret == CMSRET_SUCCESS )
         {
            char objValue[MAX_TR104_OBJ_SIZE] = { 0 };

            parms.op[1] = networkInst;

            /* Get SIP mode */
            cmsLck_acquireLock();
            ret = dalVoice_GetSipMode( &parms , objValue, sizeof(objValue) );
            cmsLck_releaseLock();

            if ( ret == CMSRET_SUCCESS )
            {
               if ( !strncmp( objValue, MDMVS_IMS, sizeof( MDMVS_IMS ) ) )
               {
                  uActiveBitFlags |= BITFLAGS_IMS;
               }
               else
               {
                  uActiveBitFlags |= BITFLAGS_RFC3261;
               }

               return;
            }
         }
      }
   }
}

void processVoiceCtlCmd(char *cmdLine)
{
   char *arg = NULL;
   char *argRemainder = NULL;

   /* cmdLine contains all arguments after "voice".
    * Example: original command line "voice show all"
    * cmdLine = "show all"
    */

   cmsLog_debug("%s() called with cmdLine(%s)\n", __FUNCTION__, cmdLine);

   /* Update IMS mode flag */
   updateImsModeFlag();

   /* Grab the first token. */
   arg = strtok( cmdLine, " " );

   if ( arg )
   {
      /* Grab the remainder of the line */
      argRemainder = strtok( NULL, "" );

      if ( !strcmp(arg, "eptcmd") )
         processVoiceDiagCmd(VOICE_DIAG_EPTCMD, argRemainder);
      else if ( !strcmp(arg, "eptprov") )
         processVoiceDiagCmd(VOICE_DIAG_EPTPROV, argRemainder);
      else if ( !strcmp(arg, "eptprobe") )
         processVoiceDiagCmd(VOICE_DIAG_EPTPROBE, argRemainder);
#ifdef EPTAPPLOAD
      else if ( !strcmp(arg, "eptapp") )
         processEptAppCmd(argRemainder);
#endif /* EPTAPPLOAD */
#ifdef BRCM_PROFILER_ENABLED
      else if ( !strcmp(arg, "profiler") )
         processVoiceCtlProfilerCmd(argRemainder);
#endif
      else if ( !strcmp(arg, "start") )
         dalVoice_voiceStart(cliPrvtMsgHandle);
#if DMP_EPON_VOICE_OAM || DMP_X_ITU_ORG_GPON_1
      else if ( !strcmp(arg, "sendUpldComplete") )
         processVoiceSendUpldComplete();
#endif /* DMP_EPON_VOICE_OAM       */
      else if ( !strcmp(arg, "stop") )
         dalVoice_voiceStop(cliPrvtMsgHandle);
      else if ( !strcmp(arg, "set") )
         processVoiceCtlSetCmd(argRemainder);
      else if ( !strcmp(arg, "create") )
         processVoiceCtlCreateCmd(argRemainder);
      else if ( !strcmp(arg, "show") )
         processVoiceCtlShowCmd(argRemainder);
      else if ( !strcmp(arg, "save") )
         processVoiceSaveCmd();
      else if ( !strcmp(arg, "reboot") )
         dalVoice_voiceReboot(cliPrvtMsgHandle);
#ifdef later
      /* todo: voice team will have to port this */
#ifdef STUN_CLIENT
      else if ( !strcmp(arg, "stunlkup") )
         processVoiceCtlStunLkupCmd(argRemainder);
#endif /* STUN_CLIENT */
#endif /* later */
      else
         voiceShowCmdSyntax();
   }
   else
   {
      voiceShowCmdSyntax();
   }

   cmsLog_debug("processVoiceCtlCmd end\n");

   return;
}

#if DMP_EPON_VOICE_OAM || DMP_X_ITU_ORG_GPON_1
/***************************************************************************
* Function Name: processVoiceSendUpldComplete
* Description  : process the voice sendUpldComplete command.
* Parameters   : none.
* Returns      : none.
****************************************************************************/
static void processVoiceSendUpldComplete(void)
{
   CmsMsgHeader *msgHdr;
   CmsRet ret = CMSRET_INTERNAL_ERROR;

   /* allocate a message body big enough to hold the header */
   msgHdr = (CmsMsgHeader *) cmsMem_alloc(sizeof(CmsMsgHeader) , ALLOC_ZEROIZE);
   if (msgHdr == NULL)
   {
      cmsLog_error("message header allocation failed");
      return;
   }

   msgHdr->src = cmsMsg_getHandleEid(cliPrvtMsgHandle);;
   msgHdr->dst = EID_SSK;
   msgHdr->type = CMS_MSG_CONFIG_UPLOAD_COMPLETE;
   msgHdr->wordData = 0;
   msgHdr->dataLength = 0;

   ret = cmsMsg_send(cliPrvtMsgHandle, msgHdr);

   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_CONFIG_UPLOAD_COMPLETE msg from CLI to SSK, ret=%d", ret);
   }

   cmsMem_free(msgHdr);
}
#endif /* DMP_EPON_VOICE_OAM */

/***************************************************************************
* Function Name: processVoiceCtlSetCmd
* Description  : process the voice set command.
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice set arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processVoiceCtlSetCmd(char *cmdLine)
{
   DAL_VOICE_PARMS argBlk;
   char * arguments[DAL_VOICE_MAX_VOIP_ARGS];
   int i=0, j;
   int vpInst, spNum,accNum,indexOfLastCliArg;
   char *value = NULL;
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
   int pstnNum, pstnInst;
#endif
   int  numSvc;

   /* Parse arguments into a ptr array */
   memset( &argBlk, 0, sizeof(argBlk) );
   memset( arguments, 0, sizeof(arguments) );
   int numArgs = initCfgPtrArray( arguments, cmdLine );

   /* Print all arguments */
   cmsLog_debug("\n");
   cmsLog_debug("Total arguments: %d",numArgs);
   cmsLog_debug("---------------------");
   for ( i = 0; i< numArgs; i++ )
   {
      cmsLog_debug("Arg%d:%s", i, arguments[i]);
   }
   cmsLog_debug("---------------------");
   cmsLog_debug("\n");

   if ( numArgs == 0 )
   {
      printf( "%s:: Incomplete voice set command\n ", __FUNCTION__ );
      /* Show command list */
      voiceShowCmdSyntax();
      return;
   }

   /* Calculate index of the last cli argument */
   indexOfLastCliArg = numArgs - 1;

   /* If value is an empty string then set it to NULL */
   if ( !strcmp( arguments[indexOfLastCliArg], "\"\"" ) )
   {
      strcpy( arguments[indexOfLastCliArg], "" );
   }

   /* Look for command name match in table */
   i = 0;
   while ( voiceDalCliMap[i].name != NULL && arguments[0] != NULL )
   {
      if ( voiceDalCliMap[i].bitFlags == (voiceDalCliMap[i].bitFlags & uActiveBitFlags)
           && strncasecmp(arguments[0], voiceDalCliMap[i].name, MAX_CLICMD_NAME_SIZE) == 0 )
      {
         break;
      }
      i++;
   }

   /* Check if we searched through entire table without finding a match */
   if ( voiceDalCliMap[i].name == NULL )
   {
      cmsLog_error("%ss:: Invalid Parameter Name\n",__FUNCTION__);
      /* Show command list */
      voiceShowCmdSyntax();
      return;
   }

   /* Check if complete arguments are provided for the specific parameter */
   if ( voiceDalCliMap[i].numArgs == ( numArgs - 1 ) )
   {
#ifdef DMP_X_BROADCOM_COM_PSTNENDPOINT_1
      /* pstn parameters, special processing
      ** arguments[0] = paramName
      ** arguments[1] = pstnNum
      ** arguments[2] = value
       */
      if ( strncasecmp( arguments[0], "pstnDialPlan",  MAX_CLICMD_NAME_SIZE ) == 0 ||
           strncasecmp( arguments[0], "pstnRouteRule", MAX_CLICMD_NAME_SIZE ) == 0 ||
           strncasecmp( arguments[0], "pstnRouteData", MAX_CLICMD_NAME_SIZE ) == 0 )
      {
         pstnNum = atoi(arguments[1]);

         cmsLck_acquireLock();
         dalVoice_mapCmPstnLineToPstnInst( pstnNum , &pstnInst );
         cmsLck_releaseLock();

         argBlk.op[0] = pstnInst;
         value = arguments[2];
      }
      else
#endif /* DMP_X_BROADCOM_COM_PSTNENDPOINT_1 */
      if ( strncasecmp( arguments[0], "loglevel", MAX_CLICMD_NAME_SIZE ) == 0 )
      {
         /* Set the loglevel right here */
         cmsLck_acquireLock();
         dalVoice_GetNumSrvProv( &numSvc );
         for( j = 0; j < numSvc ; j++)
         {
            dalVoice_mapSpNumToSvcInst( j, &vpInst );
            argBlk.op[0] = vpInst;
            dalVoice_SetModuleLoggingLevel( &argBlk, arguments[1], arguments[2] );
         }
         cmsLck_releaseLock();

         return;
      }
      else
      {
         /* Fillout arguments structure based on number of arguments */
         switch (voiceDalCliMap[i].numArgs)
         {
            case 0:
            {
               /* Assign dummy values so that check for null passes at the end */
               argBlk.op[0] = 0;
               value = "";
            }
            break;

            case 1:
            {
               /* Global parameter, set vpInst = 0, this will indicate parameter *
                * needs to be changed in all voice profiles if required          */
               argBlk.op[0] = 0;
               value = arguments[1];
            }
            break;

            case 2:
            {
               spNum = atoi(arguments[1]);

               /* Map to vpInst */
               cmsLck_acquireLock();
               dalVoice_mapSpNumToSvcInst( spNum, &vpInst );
               cmsLck_releaseLock();

               argBlk.op[0] = vpInst;
               value = arguments[2];
            }
            break;

            case 3:
            {
               /* Account parameter */
               spNum = atoi(arguments[1]);
               accNum = atoi(arguments[2]);

               /* Map to voice service instance */
               cmsLck_acquireLock();
               dalVoice_mapSpNumToSvcInst( spNum, &vpInst );
               cmsLck_releaseLock();

               argBlk.op[0] = vpInst;
               argBlk.op[1] = accNum; /* pass in accNum, so associated function will convert it to instance */
               if( voiceDalCliMap[i].convertFunc != NULL )
               {
                  int  Inst = 0;
                  CmsRet  ret ;
                  cmsLck_acquireLock();
                  ret = voiceDalCliMap[i].convertFunc( &argBlk, &Inst );
                  cmsLck_releaseLock();
                  if( ret == CMSRET_SUCCESS && Inst > 0 ){
                     argBlk.op[1] = Inst;
                  }
               }
               value = arguments[3];
            }
            break;

            default:
            break;
         }
      }
   }
   else
   {
      printf("%s:: Invalid Number of Arguments\n",__FUNCTION__);
      printf("%s:: Syntax: voice set %s %s \n",__FUNCTION__,voiceDalCliMap[i].name,voiceDalCliMap[i].syntax);
      return;
   }

   /* Execute associated DAL function */
   if ( value != NULL && voiceDalCliMap[i].dalFunc != NULL )
   {
      if ( voiceDalCliMap[i].global && voiceDalCliMap[i].numArgs == 1) /* global setting for all voice services */
      {
         cmsLck_acquireLock();
         dalVoice_GetNumSrvProv( &numSvc );
         /* Set the loglevel right here */
         for( j = 0; j < numSvc ; j++)
         {
            dalVoice_mapSpNumToSvcInst( j, &vpInst );
            argBlk.op[0] = vpInst;
            voiceDalCliMap[i].dalFunc( &argBlk, value );
         }
         cmsLck_releaseLock();
         return;
      }
      else
      {
         /* Aquire Lock */
         cmsLck_acquireLock();

         voiceDalCliMap[i].dalFunc( &argBlk, value );

         /* Release Lock */
         cmsLck_releaseLock();
      }
   }

   return;
}

/***************************************************************************
* Function Name: processVoiceCtlCreateCmd
* Description  : process the voice create command, to create an object of interest
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice set arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processVoiceCtlCreateCmd(char *cmdLine)
{
   DAL_VOICE_PARMS argBlk;
   char * arguments[DAL_VOICE_MAX_VOIP_ARGS];

   int vpInst, netwkInst, indexOfLastCliArg;
   int i=0, j;
   int  numSvc;

   /* Parse arguments into a ptr array */
   memset( &argBlk, 0, sizeof(argBlk) );
   memset( arguments, 0, sizeof(arguments) );
   int numArgs = initCfgPtrArray( arguments, cmdLine );

   /* Print all arguments */
   cmsLog_debug("\n");
   cmsLog_debug("Total arguments: %d",numArgs);
   cmsLog_debug("---------------------");
   for ( i = 0; i< numArgs; i++ )
   {
      cmsLog_debug("Arg%d:%s", i, arguments[i]);
   }
   cmsLog_debug("---------------------");
   cmsLog_debug("\n");

   if ( numArgs == 0 )
   {
      printf( "%s:: Incomplete voice set command\n ", __FUNCTION__ );
      /* Show command list */
      voiceShowCmdSyntax();
      return;
   }

   /* Calculate index of the last cli argument */
   indexOfLastCliArg = numArgs - 1;

   /* If value is an empty string then set it to NULL */
   if ( !strcmp( arguments[indexOfLastCliArg], "\"\"" ) )
   {
      strcpy( arguments[indexOfLastCliArg], "" );
   }

   /* Check if complete arguments are provided for the specific parameter */
   if ( numArgs == 1 && strncasecmp( arguments[0], "network", MAX_CLICMD_NAME_SIZE ) == 0)
   {
      cmsLck_acquireLock();
      dalVoice_GetNumSrvProv( &numSvc );
      for( j = 0; j < numSvc ; j++)
      {
         dalVoice_mapSpNumToSvcInst( j, &vpInst );
         argBlk.op[0] = vpInst;
      }
   
      if (CMSRET_SUCCESS == dalVoice_AddSipNetwork( &argBlk, &netwkInst))
      {
         printf("%s:: created SIP network, inst %d\n",__FUNCTION__, netwkInst);
      }
      else
      {
         printf("%s:: failed to create SIP network\n",__FUNCTION__);
      } 
      cmsLck_releaseLock();

      return;
   }
   else
   {
      printf("%s:: Invalid Number of Arguments (%d)\n",__FUNCTION__, numArgs);
      printf("%s:: Syntax: voice create network \n",__FUNCTION__);
      return;
   }

   return;
}

#ifdef BRCM_PROFILER_ENABLED
/***************************************************************************
* Function Name: processVoiceCtlProfilerCmd
* Description  : process the voice profiler command
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice profiler arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processVoiceCtlProfilerCmd(char *cmdLine)
{
   /* Need to send message to voice app with profiler command */
   char buf[sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody)]={0};
   CmsMsgHeader *msg = (CmsMsgHeader *) buf;
   VoiceDiagMsgBody *info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
   CmsRet ret;

   /* if no cmdLine then return false */
   if ( cmdLine == NULL )
   {
      cmsLog_error("No arguments for Endpt cmd");
      return;
   }

   /* Compose the diag message */
   msg->type = CMS_MSG_VOICE_DIAG;
   msg->src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg->dst = EID_VOICE;
   msg->flags_request = 1;
   msg->flags_bounceIfNotRunning = 1;
   msg->dataLength = sizeof(VoiceDiagMsgBody);

   /* now fill in the data section */
   strncpy(info->cmdLine,cmdLine,sizeof(info->cmdLine));
   info->type = VOICE_DIAG_PROFILE;

   if ((ret = cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, msg,10000)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_VOICE_DIAG msg to voice, ret=%d", ret);
   }
}
#endif  /* BRCM_PROFILER_ENABLED */

/***************************************************************************
* Function Name: processVoiceCtlShowCmd
* Description  : process the voice show command.
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice show arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processVoiceCtlShowCmd(char *cmdLine)
{
   int i, spNum = 0, match = 0;
   char *arg = NULL, *nextarg = NULL;
   UBOOL8 bIsAll = FALSE;

   if ( cmdLine )
   {
      /* Grab the first argument. */
      arg = strtok( cmdLine, " " );
   }

   /* If "voice show all", then set the "all" flag. */
   if ( arg && !strcmp( arg, "all" ) )
   {
      bIsAll = TRUE;
   }

#ifdef SIPLOAD
   /* In CCTK stats need to be retrieved first */
   if ( arg &&
        ( !strcmp( arg, "memstats") ||
          !strcmp( arg, "cctkcmstats" ) ) )
   {
      /* Need to send message to voice app */
      char buf[sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody)]={0};
      CmsMsgHeader *msg = (CmsMsgHeader *) buf;
      VoiceDiagMsgBody *info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);
      msg->type = CMS_MSG_VOICE_DIAG;
      msg->src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
      msg->dst = EID_VOICE;
      msg->flags_request = 1;
      msg->flags_bounceIfNotRunning = 1;
      msg->dataLength = sizeof( VoiceDiagMsgBody );
      info->type = VOICE_DIAG_STATS_SHOW;
      strncpy( (char*)info->cmdLine, arg, BUFLEN_128 );

      if( cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, msg, 2000) != CMSRET_SUCCESS )
      {
         cmsLog_error("Could not send CMS_MSG_VOICE_DIAG msg to voice" );
      }

      match = 1;
   }
#endif /* SIPLOAD */

   cmsLck_acquireLock();
   dalVoice_GetNumSrvProv( &spNum );
   cmsLck_releaseLock();

   for( i = 0; i<spNum; i++ )
   {
      printf( "\n" );
      printf( "Service provider: %d\n", spNum );
      printf( "------------------\n" );

#ifdef SIPLOAD
      if ( arg )
      {
         /* Aquire Lock */
         cmsLck_acquireLock();

         if ( !strcmp( arg, "network") || bIsAll )
         {
            dumpSipNetworkParams( i );
            match = 1;
         }

         if ( !strcmp( arg, "client") || bIsAll )
         {
            dumpSipClientParams( i );
            match = 1;
         }

         if ( !strcmp( arg, "voipprofile") || bIsAll )
         {
            dumpVoipProfileParams( i );
            match = 1;
         }

         if( !strcmp( arg, "codec") || bIsAll )
         {
            dumpVoiceCodecList( i );
            match = 1;
         }

         if( !strcmp( arg, "callctl") || bIsAll )
         {
            dumpCallCtlLineExtParams( i );
            match = 1;
         }

         if( !strcmp( arg, "feature") || bIsAll )
         {
            dumpCallCtlFeatureSetParams( i );
            match = 1;
         }

         if( !strcmp( arg, "map") || bIsAll )
         {
            dumpCallCtlMappingParams( i );
            match = 1;
         }

         /* Show call log summary */
         if( !strcmp( arg, "calllog") )
         {
            dumpVoiceCallLogs( i );
            match = 1;
         }

         /* Check for sub-commands of "callstats" */
         if ( bIsAll )
         {
            /* Show all call log statistics */
            dumpVoiceAllCallStats( i );
            match = 1;
         }
         else if( !strcmp( arg, "callstats") )
         {
            nextarg = strtok( NULL, " " );
            if ( !nextarg )
            {
               /* Show RTP statistics */
               dumpVoiceStats( i );
               match = 1;
            }
            else if ( !strcmp( nextarg, "lastcall" ) )
            {
               /* Show last call stats */
               dumpVoiceLastCallStats( i );
               match = 1;
            }
            else if ( !strcmp( nextarg, "all" ) )
            {
               /* Show all call log statistics */
               dumpVoiceAllCallStats( i );
               match = 1;
            }
         }

         /* Show all of diag parameters */
         if( !strcmp( arg, "linetest") || bIsAll )
         {
            dumpVoicePotsFxsDiag( i );
            match = 1;
         }

         /* Show all of Lines parameters */
         if( !strcmp( arg, "line") || bIsAll )
         {
            dumpVoicePotsFxs( i );
            match = 1;
         }

         /* Release Lock */
         cmsLck_releaseLock();
      }
#endif /* SIPLOAD */

      /* show service provider parameters */
      if ( !match || ( arg && bIsAll ) )
      {
         /* Aquire Lock */
         cmsLck_acquireLock();

         dumpServiceProviderParams( i );

         /* Release Lock */
         cmsLck_releaseLock();
      }
   }
}

/***************************************************************************
* Function Name: processVoiceSaveCmd
* Description  : process the voice save command.
* Parameters   : none.
* Returns      : none.
****************************************************************************/
static void processVoiceSaveCmd()
{
   /* Aquire Lock */
   cmsLck_acquireLock();

   dalVoice_Save();

   /* Release Lock */
   cmsLck_releaseLock();
}

#ifdef later
#ifdef STUN_CLIENT
/***************************************************************************
* Function Name: processVoiceCtlStunLkupCmd
* Description  : process the voice stunlkup command.
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice stunlkup arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
void CliShellCmd::processVoiceCtlStunLkupCmd( char *cmdLine )
{
#ifdef CLI_CMD
   BcmVoice_StunLkup( cmdLine );
#endif
}
#endif /* STUN_CLIENT */
#endif /* later */

/***************************************************************************
* Function Name: processEptAppCmd
* Description  : process the endpoint demo app cmds.
* Parameters   : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice eptapp arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processEptAppCmd(char *cmdLine)
{
   char cp[CLI_MAX_BUF_SZ];
   char *cmd = NULL;

   if ( cmdLine == NULL )
   {
      cmsLog_error( "no arguments to eptapp command" );
      voiceShowCmdSyntax();
   }

   strncpy( cp, cmdLine, CLI_MAX_BUF_SZ );

   /* get the command from cp (cmdLine) */
   cmd = strtok(cp, " ");

   if ( cmd == NULL )
   {
      cmsLog_error( "no eptapp command" );
      voiceShowCmdSyntax();
      return;
   }

   /* search for command and calls appropriate process command.
   ** NOTE: Need to pass in entire cmdLine, as eptapp will strip the
   **       first argument.
   */
   if ( strcasecmp(cmd, "show") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_SHOW, cmdLine);
   else if ( strcasecmp(cmd, "createcnx") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_CREATECNX, cmdLine);
   else if ( strcasecmp(cmd, "deletecnx") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_DELETECNX, cmdLine);
   else if ( strcasecmp(cmd, "modifycnx") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_MODIFYCNX, cmdLine);
   else if ( strcasecmp(cmd, "eptsig") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_EPTSIG, cmdLine);
   else if ( strcasecmp(cmd, "set") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_SET, cmdLine);
   else if ( strcasecmp(cmd, "decttest") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_DECTTEST, cmdLine);
   else if ( strcasecmp(cmd, "vas") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_VAS, cmdLine);
   else if ( strcasecmp(cmd, "vrs") == 0 )
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_VRS, cmdLine);
   else
   {
      /* Is either "help" or malformed */
      processVoiceDiagCmd(VOICE_DIAG_EPTAPP_HELP, cmdLine);
   }

   return;
}

/***************************************************************************
* Function Name: buildCmsMsg
* Description  : common code for populating cms structures
* Parameters   : msg - message to forward.
*              : info - body portion of message
*              : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice eptcmd arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : 0 if success, otherwise non-zero
****************************************************************************/
static int buildCmsMsg(CmsMsgHeader *msg, VoiceDiagMsgBody *info, char *cmdLine)
{
   /* if no cmdLine then return false */
   if ( cmdLine == NULL )
   {
      cmsLog_error("No arguments for Endpt cmd");
      return( -1 );
   }

   /* Compose the diag message */
   msg->type = CMS_MSG_VOICE_DIAG;
   msg->src = cmsMsg_getHandleEid(cliPrvtMsgHandle);
   msg->dst = EID_VOICE;
   msg->flags_request = 1;
   msg->flags_bounceIfNotRunning = 1;
   msg->dataLength = sizeof(VoiceDiagMsgBody);

   /* now fill in the data section */
   strncpy(info->cmdLine,cmdLine,sizeof(info->cmdLine));

   return( 0 );
}

/***************************************************************************
* Function Name: processVoiceDiagCmd
* Description  : common code for messaging Endpoint Application
* Parameters   : diagType - message type used to route the message
*              : cmdLine - command line with first 2 arguments removed
*              :           Example: original CLI cmdLine "voice eptcmd arg1 arg2"
*              :                    cmdLine = "arg1 arg2"
* Returns      : none.
****************************************************************************/
static void processVoiceDiagCmd(VoiceDiagType diagType, char *cmdLine)
{
   /* Need to send message to voice app with eptcmd */
   char buf[sizeof(CmsMsgHeader) + sizeof(VoiceDiagMsgBody)]={0};
   CmsMsgHeader *msg = (CmsMsgHeader *) buf;
   VoiceDiagMsgBody *info = (VoiceDiagMsgBody *) &(buf[sizeof(CmsMsgHeader)]);

   int status = buildCmsMsg(msg, info, cmdLine);
   if (status != 0)
   {
      return;
   }

   info->type = diagType;

   CmsRet ret = cmsMsg_sendAndGetReplyWithTimeout(cliPrvtMsgHandle, msg, 10000);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("could not send CMS_MSG_VOICE_DIAG msg to voice, ret=%d", ret);
   }
}

/***********************************************************
 * This function takes a buffer full of space delimited    *
 * voip configuration parameters and populates an array of *
 * strings as follows:                                     *
 *                argBuffer =  -p 0.0.0.0:5060 -l none     *
 *                             ^  ^            ^  ^        *
 *                             |  |            |  |        *
 * argArray[0][0] <------------+  |            |  |        *
 * argArray[1][0] <---------------+            |  |        *
 * argArray[2][0] <----------------------------+  |        *
 * argArray[3][0] <-------------------------------+        *
 *                                                         *
 * Args: argArray    - Array of strings to hold all args   *
 *       argBuffer   - cmdLine string                      *
 *                                                         *
 * Returns: Number of arguments parsed                     *
 ***********************************************************/
static int initCfgPtrArray(char ** argArray, char * argBuffer)
{
   char *nextArg = NULL;
   int i = 0;

   if ( NULL == argBuffer )
   {
      return 0;
   }

   nextArg = strtok(argBuffer, " ");
   
   while ( nextArg != NULL && i < DAL_VOICE_MAX_VOIP_ARGS )
   {
      argArray[i++] = nextArg;
      nextArg = strtok(NULL, " ");
   }

   return i;
}

/***************************************************************************
* Function Name: dumpServiceProviderParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpServiceProviderParams(int spNum)
{
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;
   int   vpInst = 0;
   unsigned int   numLines = 0, numExt = 0;
   unsigned int   numFxsLines, numFxoLines, numSipClients, numSipNetworks;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Fill in the parameter structure needed for dalVoice_GetXYZ.
   ** Since these are global parameters, the voice profile and
   ** line instance are irrelevant so we hard-code them. */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );
   parms.op[0] = vpInst;

   printf( "\n" );
   printf( "General:\n" );
   printf( "--------\n" );

   if ( dalVoice_GetBoundIfName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "BoundIfName                : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetIpFamily( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "IP address family          : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetLoggingLevel( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Voice logLevel             : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetModuleLoggingLevels( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Voice Module logLevel      : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetCCTKTraceLevel( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "CCTK logLevel              : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetCCTKTraceGroup( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "CCTK logGroup              : %s\n", objValue );
   }


   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetManagementProtocol( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Management Protocol        : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetRegion( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Region                     : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetDigitMap( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "VOIP Dialplan              : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetVoiceDnsEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Voice DNS Server Enable    : %s\n", ( atoi( objValue ) ? MDMVS_ON : MDMVS_OFF ) );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetDnsServerAddrPri( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Voice DNS Server Primary   : %s\n", objValue );
   }

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   if ( dalVoice_GetDnsServerAddrSec( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
   {
      printf( "Voice DNS Server Secondary : %s\n", objValue );
   }

   if ( dalVoice_GetNumPhysFxsEndpt( &numFxsLines ) == CMSRET_SUCCESS )
   {
      printf( "Number of FXS lines        : %d\n", numFxsLines );
   }

   if ( dalVoice_GetNumPhysFxoEndpt( &numFxoLines ) == CMSRET_SUCCESS )
   {
      printf( "Number of FXO lines        : %d\n", numFxoLines );
   }

   if ( dalVoice_GetNumSipClient( &parms, &numSipClients ) == CMSRET_SUCCESS )
   {
      printf( "Number of SIP clients      : %d\n", numSipClients );
   }

   if ( dalVoice_GetNumSipNetwork( &parms, &numSipNetworks ) == CMSRET_SUCCESS )
   {
      printf( "Number of SIP networks     : %d\n", numSipNetworks );
   }

   if ( dalVoice_GetNumOfExtension( &parms, &numExt ) == CMSRET_SUCCESS )
   {
      printf( "Number of CC extensions    : %d\n", numExt );
   }

   if ( dalVoice_GetNumOfLine( &parms, &numLines ) == CMSRET_SUCCESS )
   {
      printf( "Number of CC lines         : %d\n", numLines );
   }

   cmsMem_free( objValue );

   return ret;
}


/***************************************************************************
* Function Name: dumpVoiceCodecList
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpVoiceCodecList(int spNum)
{
   int cpInst=0,  vpInst = 0;
   int numOfCodecProfile = 0;
   int i;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumCodecProfile( &parms, &numOfCodecProfile );
   if( numOfCodecProfile <= 0 )
   {
      printf( "\n" );
      printf( "No valid VoIP profile\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfCodecProfile; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapCpNumToInst ( &parms, &cpInst ) == CMSRET_SUCCESS )
      {
         parms.op[1] = cpInst;
         printf( "Codec Profile %d:\n", i);
         printf( "-----------------\n" );
         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCodecProfileName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Profile Name: %s\n",  objValue );
         }

         if ( dalVoice_GetCodecProfileEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Profile Enabled: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCodecProfPacketPeriod( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   ptime: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCodecProfSilSupp( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Vad Enabled: %s\n", objValue );
         }
      }
   }

   memset( objValue, 0, MAX_TR104_OBJ_SIZE);
   ret = dalVoice_GetVoiceSvcCodecList(&parms, objValue, MAX_TR104_OBJ_SIZE);
   if( ret == CMSRET_SUCCESS && strlen(objValue) > 0)
   {
      printf( "Supported Codec List\n");
      printf( "--------------------\n" );
      printf( "   %s\n", objValue );
   }
   else
   {
      printf( "\n" );
      printf( "No valid codec\n");
   }
   /* Get voiceProfile object */
   cmsMem_free( objValue );

   return (ret);
}



/***************************************************************************
* Function Name: dumpVoipProfileParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpVoipProfileParams(int spNum)
{
   int profileInst=0, numOfProfile, i, vpInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumVoipProfile( &parms, &numOfProfile );
   if( numOfProfile <= 0 )
   {
      printf( "\n" );
      printf( "No valid VoIP profile\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfProfile; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapVoipProfNumToInst ( &parms, &profileInst ) == CMSRET_SUCCESS )
      {
         parms.op[1] = profileInst;
         printf( "VOIP Profile %d:\n", i);
         printf( "----------------\n" );

         if ( dalVoice_GetVoipProfileEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Profile Enabled: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetVoipProfileName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Profile Name: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetDTMFMethod( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   DTMF method : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtpLocalPortMin( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTP port min: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtpLocalPortMax( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTP port max: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtpDSCPMark( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   DSCP mark: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtcpEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTCP enabled: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtcpInterval( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTCP send interval: %s ms\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtcpXrEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTCP-XR enabled: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetRtcpXrPubRepAddr( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   RTCP-XR SIP PUBLISH Report Addr: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSrtpEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SRTP enabled: %s\n", objValue );
         }
         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if (dalVoice_GetSrtpOptionString( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SRTP option: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if (dalVoice_GetV18Enable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   V.18 enabled: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if (dalVoice_GetHookFlashMethod( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Hook flash method: %s\n", objValue );
         }
      }
   }
   cmsMem_free( objValue );

   return (ret);
}


/***************************************************************************
* Function Name: dumpCallCtlLineExtParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpCallCtlLineExtParams(int spNum)
{
   int numOfExt, numOfLines;
   int i, j;
   int lineInst = 0, extInst = 0, vpInst, mapInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue  = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumOfExtension( &parms, &numOfExt );
   if( numOfExt <= 0 )
   {
      printf( "\n" );
      printf( "No valid Call Control Extension\n");
      return  ret;
   }

   dalVoice_GetNumOfLine( &parms, &numOfLines );
   if( numOfLines <= 0 )
   {
      printf( "\n" );
      printf( "No valid Call Control Line\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfExt; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapExtNumToExtInst( vpInst, i, &extInst ) == CMSRET_SUCCESS)
      {
         parms.op[1] = extInst;
         printf( "Call Control Extension %d:\n", i);
         printf( "--------------------------\n" );

         if ( dalVoice_GetCallCtrlExtEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Enabled: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Name: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtStatus( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Status: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtNumber( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Extension Number: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtCallStatus( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Call Status: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtConfStatus( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Conf Call Status: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtProvider( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Provider Name: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlExtCallFeatureSet( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Call Feature: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         /* Not applicable at the moment.
          * if ( dalVoice_GetCallCtrlExtNumberPlan( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
          * {
          *  printf( "   Numbering Plan: %s\n", objValue );
          * }
         */
         printf( "   Numbering Plan: Not applicable\n" );

      }
   }

   for( i = 0; i < numOfLines; i++ )
   {
      printf( "\n" );

      if( dalVoice_mapAcntNumToLineInst( vpInst, i, &lineInst ) == CMSRET_SUCCESS)
      {
         parms.op[0] = vpInst;
         parms.op[1] = lineInst;
         printf( "Call Control Line %d:\n", i);
         printf( "--------------------\n" );

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetCallCtrlLineCallStatus( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Call Status: %s\n", objValue );
         }

         printf( "   Associated extensions: ");
         for (j = 0; j < numOfExt; j++)
         {
            parms.op[0] = vpInst;
            parms.op[1] = i;
            parms.op[2] = j;
            if( dalVoice_mapLineExtToIncomingMapInst( &parms, &mapInst ) == CMSRET_SUCCESS && mapInst > 0)
            {
               printf( "EXT_%d ", j);
            }
         }
      }

      printf( "\n" );
   }

   cmsMem_free( objValue );

   return (ret);
}




/***************************************************************************
* Function Name: dumpSipClientParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpSipClientParams(int spNum)
{
   int clientInst=0, numOfClient, i, vpInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumSipClient( &parms, &numOfClient );
   if( numOfClient <= 0 )
   {
      printf( "\n" );
      printf( "No valid Sip Client\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfClient; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapAcntNumToClientInst ( &parms, &clientInst ) == CMSRET_SUCCESS )
      {
         parms.op[1] = clientInst;
         printf( "SIP Client %d:\n", i);
         printf( "--------------\n" );

         if ( dalVoice_GetSipClientEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Client Enabled: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipClientStatus( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Status: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetVlCFCallerIDName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   DisplayName : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetVlSipURI( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Extension: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetVlSipAuthUserName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Authentication Name: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetVlSipAuthPassword( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Password: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetT38Enable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   T.38 enabled: %s\n", objValue );
         }


         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipContactUri( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP override contact URI: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipClientAttachedNetworkIdx( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Attached Network: SIP_Network_%s\n", objValue );
         }
      }
   }

   cmsMem_free( objValue );

   return (ret);
}



/***************************************************************************
* Function Name: dumpSipNetworkParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpSipNetworkParams(int spNum)
{
   int vpInst=0, networkInst, numOfNetwork, i;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumSipNetwork( &parms, &numOfNetwork );
   if( numOfNetwork <= 0 )
   {
      printf( "\n" );
      printf( "No valid Sip Network\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfNetwork; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      memset( objValue, 0, MAX_TR104_OBJ_SIZE);
      if( dalVoice_mapNetworkNumToInst( &parms , &networkInst ) == CMSRET_SUCCESS )
      {
         UBOOL8 sipModeIms = 0;
         parms.op[1] = networkInst;
         printf( "SIP Network %d:\n", i);
         printf( "---------------\n" );

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipNetworkEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Network status: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipMode( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP mode: %s\n",  objValue );
            if ( !strncmp( objValue, MDMVS_IMS, sizeof( MDMVS_IMS ) ) )
            {
               sipModeIms = 1;
            }
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTransportString( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP Transport: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipUriForTls( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Usage of SIP URI for TLS: %s\n",  objValue );
         }

         if (!sipModeIms)
         {
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipRegistrarServer( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Registrar Server: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipRegistrarServerPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Registrar Port: %s\n", objValue );
            }
         }
         else
         {
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipOutboundProxy( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   PCSCF: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipOutboundProxyPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   PCSCF Port: %s\n", objValue );
            }
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipRegisterExpires( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Registration Expiry: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipRegisterRetryInterval( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Registration Retry Interval: %s\n", objValue );
         }

         if (!sipModeIms)
         {
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipProxyServer( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Proxy Server: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipProxyServerPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Proxy Port: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipOutboundProxy( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Outbound Proxy: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipOutboundProxyPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Outbound Port: %s\n", objValue );
            }
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipUserAgentDomain( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   UserAgent Domain: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipUserAgentPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   UserAgent Port: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipRealm( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Realm: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetSipConferencingURI( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS)
         {
            printf( "   Conf Call URI: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetSipConferencingOption( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS)
         {
            printf( "   Conf Call option: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetSipNetworkCodecList( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS)
         {
            printf( "   Codec List: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetSipNetworkVoipProfileIdx( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS)
         {
            printf( "   VoIP Profile: VoIP_Profile_%s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipDSCPMark( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   DSCP: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerT1( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer T1: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerT2( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer T2: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerT4( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer T4: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerB( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer B : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerD( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer D : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerF( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer F : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerH( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer H : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipTimerJ( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Timer J : %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipInviteExpires( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Invite Expires: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipToTagMatching( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP TagMatch: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipFailoverEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP Failover: %s\n", objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipSecDomainName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Secondary UserAgent Domain: %s\n", objValue );
         }

         if (!sipModeIms)
         {
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecRegistrarAddr( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Registrar: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecRegistrarPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Registrar Port: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecProxyAddr( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Proxy: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecProxyPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Proxy Port: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecOutboundProxyAddr( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Outbound Proxy: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecOutboundProxyPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary Outbound Proxy Port: %s\n", objValue );
            }
         }
         else
         {
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecOutboundProxyAddr( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary PCSCF: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetSipSecOutboundProxyPort( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Secondary PCSCF Port: %s\n", objValue );
            }
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipBackToPrimOption( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   Back-To-Primary Mode: %s\n", objValue );
         }
         
         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if ( dalVoice_GetSipOptionsEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
         {
            printf( "   SIP OPTIONS ping: %s\n", objValue );
         }
      }
   }

   cmsMem_free( objValue );

   return (ret);
}

/***************************************************************************
* Function Name: dumpSipMappingParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpCallCtlMappingParams(int spNum)
{
   int vpInst=0, mapInst, numOfMap, i;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumIncomingMap( &parms, &numOfMap );
   if( numOfMap <= 0 )
   {
      printf( "\n" );
      printf( "No valid Incoming Map\n");
   }
   else
   {
      for( i = 0; i < numOfMap; i++ )
      {
         printf( "\n" );
         parms.op[1] = i;

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_mapIncomingMapNumToInst( &parms , &mapInst ) == CMSRET_SUCCESS )
         {
            parms.op[1] = mapInst;
            printf( "Incoming Map %d:\n", i);
            printf( "----------------\n" );

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetIncomingMapEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Map enable: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetIncomingMapLineNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Line: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetIncomingMapExtNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Extension: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetIncomingMapOrder( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Order: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetIncomingMapTimeout( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Timeout: %s\n", objValue );
            }
         }
      }
   }

   parms.op[0] = vpInst;
   dalVoice_GetNumOutgoingMap( &parms, &numOfMap );
   if( numOfMap <= 0 )
   {
      printf( "\n" );
      printf( "No valid Outgoing Map\n");
   }
   else
   {
      for( i = 0; i < numOfMap; i++ )
      {
         printf( "\n" );
         parms.op[1] = i;

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_mapOutgoingMapNumToInst( &parms , &mapInst ) == CMSRET_SUCCESS )
         {
            parms.op[1] = mapInst;
            printf( "Outgoing Map %d:\n", i);
            printf( "--------------------\n" );

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetOutgoingMapEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Map enable: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetOutgoingMapLineNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Line: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetOutgoingMapExtNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Extension: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetOutgoingMapOrder( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Order: %s\n", objValue );
            }
         }
      }
   }

   printf( "\n" );

   cmsMem_free( objValue );

   return (ret);
}

/***************************************************************************
* Function Name: dumpCallCtlFeatureSetParams
* Description  : Dump service provider specific parmaters.
* These parameters have a per line scope in callmanager and are stored
* at the voice profile level in TR104
****************************************************************************/
static CmsRet dumpCallCtlFeatureSetParams(int spNum)
{
   int vpInst=0, setInst, numOfFeatureSet, i;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue = NULL;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumCallFeatureSet( &parms, &numOfFeatureSet );
   if( numOfFeatureSet <= 0 )
   {
      printf( "\n" );
      printf( "No valid calling feature set\n");
   }
   else
   {
      for( i = 0; i < numOfFeatureSet; i++ )
      {
         printf( "\n" );
         parms.op[1] = i;

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_mapCallFeatureSetNumToInst( &parms , &setInst ) == CMSRET_SUCCESS )
         {
            parms.op[1] = setInst;
            printf( "Calling Feature Set %d:\n", i);
            printf( "-----------------------\n" );

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallId( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallID enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallIdName( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallID Name enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallWaiting( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Call Waiting enabled: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallFwdAll( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallFwd All enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallFwdNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallFwd Number: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallFwdBusy( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallFwd OnBusy enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallFwdNoAns( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallFwd NoAnswer enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallBarring( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Outgoing call barring enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallBarringPin( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Outgoing call barring PIN: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallBarringDigitMap( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Outgoing call barring digit map: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFMWIEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   MWI enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFCallTransfer( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   CallTransfer enabled: %s\n",  objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFDoNotDisturb( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   DoNotDisturb enabled: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFAnonymousCalling( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   AnonymousCall enabled: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFAnonCallBlck( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Anonymous CallBlock enabled: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFWarmLine( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   WarmLine enabled: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetWarmLineOffhookTimer( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   WarmLine offhook delay timer: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFWarmLineNum( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   WarmLine Number: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_CODE_CALLRETURN;        /* last call return */
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Call Return enabled: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_CODE_CALLREDIAL;        /* last call return */
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Call Redial enabled: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_CODE_CCBS;        /* call complete for busy subs */
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Call Complete for Busy Subs enabled: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_ESVC_END_CALL;
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc end all call: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_ESVC_NO_LOC_INFO;
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc no location info: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_ESVC_ENABLE_3WAY;
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc allow 3way call: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_ESVC_NETHOLDDISABLE;
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc disable nwk hold: %s\n", objValue );
            }

            parms.op[2] = DAL_VOICE_FEATURE_ESVC_NETHOLDBYPASS;
            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetVlCFFeatureEnabled( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc bypass nwk hold check: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetEServiceDSCPMark(&parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency Svc DSCP mark: %s\n", objValue );
            }

            memset( objValue, 0, MAX_TR104_OBJ_SIZE);
            if ( dalVoice_GetEServiceHowlerDuration(&parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
            {
               printf( "   Emergency howler tone duration: %s\n", objValue );
            }
         }
      }

      printf( "\n" );
      printf( "Global Calling Features:\n" );
      printf( "------------------------\n" );

      memset( objValue, 0, MAX_TR104_OBJ_SIZE);
      if ( dalVoice_GetEuroFlashEnable( &parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS )
      {
         printf( "   European flash enable: %s\n", objValue );
      }
   }


   cmsMem_free( objValue );

   return (ret);
}

static CmsRet dumpVoiceStats(int spNum)
{
   int numOfLines, i;
   int lineInst = 0, vpInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue  = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetNumOfLine( &parms, &numOfLines );
   if( numOfLines <= 0 )
   {
      printf( "\n" );
      printf( "No valid Call Control Line\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfLines; i++ )
   {
      printf( "\n" );

      if( dalVoice_mapAcntNumToLineInst( vpInst, i, &lineInst ) == CMSRET_SUCCESS)
      {
         printf( "Call Control Line %d:\n", i);
         printf( "--- Call Statistics ----\n" );

         parms.op[1] = lineInst;
         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsRtpPacketSentString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   RTP Packets Sent              : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsRtpPacketRecvString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   RTP Packets Received          : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsRtpPacketLostString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   RTP Packets Lost              : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsRtpBytesSentString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   RTP Total Bytes Sent          : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsRtpBytesRecvString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   RTP Total Bytes Received      : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsInCallRecvString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Incoming Calls Received       : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsInCallConnString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Incoming Calls Connected      : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsInCallFailedString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Incoming Calls Failed         : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsInCallDropString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Incoming Calls Dropped        : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsInTotalCallTimeString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Incoming Total Call Time (sec): %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsOutCallAttemptString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Outgoing Calls Attempted      : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsOutCallConnString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Outgoing Calls Connected      : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsOutCallFailedString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Outgoing Calls Failed         : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsOutCallDropString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Outgoing Calls Dropped        : %s\n", objValue);

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCcLineStatsOutTotalCallTimeString(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Outgoing Total Call Time (sec): %s\n", objValue);
      }

      printf( "\n" );
   }

   cmsMem_free( objValue );

   return ret;
}

/***************************************************************************
* Function Name: dumpVoiceCallLogs
* Description  : Prints out all call log summaries
* Parms        : spNum - service provider instance
* Returns      : CMSRET_SUCCESS if OK, CMSRET_XXX error if failure
****************************************************************************/
static CmsRet dumpVoiceCallLogs(int spNum)
{
   int numOfLogs, maxCallLogs, i;
   int vpInst = -1, logInst = -1;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue  = NULL;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   dalVoice_GetMaxCallLogCount( &parms, objValue, MAX_TR104_OBJ_SIZE );
   maxCallLogs = atoi(objValue);
   dalVoice_GetNumCallLog( &parms, objValue, MAX_TR104_OBJ_SIZE );
   numOfLogs = atoi(objValue);
   if( numOfLogs <= 0 )
   {
      printf( "\nNo Calls\n\n");
      cmsMem_free( objValue );
      return ret;
   }

   printf( "Maximum Call Log: %d\n", maxCallLogs);
   for( i = 0; i < numOfLogs; i++ )
   {
      printf( "\n" );

      parms.op[1] = i;

      if( dalVoice_mapCallLogNumToInst( &parms, &logInst ) == CMSRET_SUCCESS )
      {
         parms.op[1] = logInst;
         printf( "Call Log %d:\n", i);
         printf( "------------\n" );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogDirection(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Call Direction : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogCallingParty(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Calling Number : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogCalledParty(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Called Number  : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogStartDateTime(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Call Start     : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogStopDateTime(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Call End       : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogDuration(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Call Duration  : %s seconds\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogReason(&parms, objValue, MAX_TR104_OBJ_SIZE);
         printf( "   Call Terminate : %s\n", objValue);
      }

      printf( "\n" );
   }

   cmsMem_free( objValue );

   return ret;
}

/***************************************************************************
* Function Name: dumpVoiceLastCallStats
* Description  : Prints out most recent call logs instance
*                (and associated sessions)
* Parms        : spNum - service provider instance
* Returns      : CMSRET_SUCCESS if OK, CMSRET_XXX error if failure
****************************************************************************/
static CmsRet dumpVoiceLastCallStats(int spNum)
{
   int numOfLogs = 0;
   int vpInst = -1, logInst = -1;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char *objValue = NULL;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   ret = dalVoice_GetNumCallLog( &parms, objValue, MAX_TR104_OBJ_SIZE );
   numOfLogs = atoi(objValue);
   if( ret != CMSRET_SUCCESS || numOfLogs <= 0 )
   {
      printf( "\nNo Calls\n\n");
      cmsMem_free( objValue );
      return ret;
   }

   /* Get the last (latest) calllog instance. */
   parms.op[1] = numOfLogs-1;

   ret = dalVoice_mapCallLogNumToInst( &parms, &logInst );
   if( ret == CMSRET_SUCCESS )
   {
      parms.op[1] = logInst;

      printf("\n");
      printf( "Last call (Call Log %d):\n", numOfLogs-1);
      printf( "-------------------------\n" );

      dumpVoiceCallLogStatsHelper( &parms );
   }
   else
   {
      cmsLog_error( "mapCallLogNumToInst() failed\n" );
   }

   printf( "\n" );

   cmsMem_free( objValue );

   return ret;
}

/***************************************************************************
* Function Name: dumpVoiceAllCallStats
* Description  : Prints out all call logs instances
*                (and associated sessions)
* Parms        : spNum - service provider instance
* Returns      : CMSRET_SUCCESS if OK, CMSRET_XXX error if failure
****************************************************************************/
static CmsRet dumpVoiceAllCallStats(int spNum)
{
   int numOfLogs = 0, maxCallLogs = 0, i;
   int vpInst = -1, logInst = -1;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char *objValue = NULL;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   
   ret = dalVoice_GetMaxCallLogCount( &parms, objValue, MAX_TR104_OBJ_SIZE );
   maxCallLogs = atoi(objValue);
   if ( ret != CMSRET_SUCCESS || maxCallLogs < 0)
   {
      printf("Cannot get maximum call log count\n");
      cmsMem_free( objValue );
      return ret;
   }

   ret = dalVoice_GetNumCallLog( &parms, objValue, MAX_TR104_OBJ_SIZE );
   numOfLogs = atoi(objValue);
   if( ret != CMSRET_SUCCESS || numOfLogs <= 0 )
   {
      printf( "\nNo Calls\n\n");
      cmsMem_free( objValue );
      return ret;
   }

   printf( "Maximum Call Log: %d\n", maxCallLogs);
   for( i = 0; i < numOfLogs; i++ )
   {
      parms.op[1] = i;

      ret = dalVoice_mapCallLogNumToInst( &parms, &logInst );
      if( ret == CMSRET_SUCCESS )
      {
         parms.op[1] = logInst;

         printf("\n");
         printf( "Call Log %d:\n", i);
         printf( "------------\n" );

         dumpVoiceCallLogStatsHelper( &parms );
      }
      else
      {
         cmsLog_error( "mapCallLogNumToInst() failed\n" );
         break;
      }
   }

   printf( "\n" );

   cmsMem_free( objValue );

   return ret;
}

/***************************************************************************
* Function Name: dumpVoiceCallLogStatsHelper
* Description  : Helper function that prints out a call log entry given
*                the call log instance.
* Parms        : parms->op[0] = voice service instance
*                parms->op[1] = call log instance
* Returns      : CMSRET_SUCCESS if OK, CMSRET_XXX error if failure
****************************************************************************/
static CmsRet dumpVoiceCallLogStatsHelper( DAL_VOICE_PARMS *parms )
{
   char* objValue  = NULL;
   char* callTrace = NULL;
   int maxCallTraceSize = 16384;
   UBOOL8 bLocalExtStatsAvail = FALSE;
   int nValue = 0, nRecv = 0;
   int nNumSessions = 0, sessionInst = -1, i;
   char szUnavailable[] = "Unavailable";
   CmsRet ret = CMSRET_SUCCESS;

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   /* Print base calllog instance entries */
   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogDirection(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Call Direction : %s\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogCallingParty(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Calling Number : %s\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogCalledParty(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Called Number  : %s\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogStartDateTime(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Call Start     : %s\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogStopDateTime(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Call End       : %s\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogDuration(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Call Duration  : %s seconds\n", objValue );

   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   dalVoice_GetCallLogReason(parms, objValue, MAX_TR104_OBJ_SIZE);
   printf( "   Call Terminate : %s\n", objValue);

   /* Get number of sessions associated with this call log instance. */
   memset(objValue, 0, MAX_TR104_OBJ_SIZE);
   ret = dalVoice_GetNumCallLogSession( parms, objValue, MAX_TR104_OBJ_SIZE );
   nNumSessions = atoi(objValue);
   if ( ret != CMSRET_SUCCESS || nNumSessions <= 0 )
   {
      printf("No sessions for call log entry!\n");
      cmsMem_free( objValue );
      return ret;
   }

   /* Loop through each session */
   for ( i = 0; i < nNumSessions; i++ )
   {
      parms->op[2] = i;

      if( dalVoice_mapCallLogSessionNumToInst( parms, &sessionInst ) == CMSRET_SUCCESS )
      {
         parms->op[2] = sessionInst;

         printf("\n");
         printf("   Session %d:\n", i);
         printf("   -----------\n");

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetCallLogSessionStatsLocalValid( parms, objValue, MAX_TR104_OBJ_SIZE ) == CMSRET_SUCCESS
             && 0 == strcmp( objValue, MDMVS_YES ) )
         {
            bLocalExtStatsAvail = TRUE;
         }

         /* Print MOQCQ, MOSLQ, SnR, RERL */
         if ( bLocalExtStatsAvail )
         {
            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsMOSCQ( parms, objValue, MAX_TR104_OBJ_SIZE );
            nValue = atoi(objValue);
            printf("      MOS-CQ Score                 : %.1f\n", (float)((float)nValue/10.0) );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsMOSLQ( parms, objValue, MAX_TR104_OBJ_SIZE );
            nValue = atoi(objValue);
            printf("      MOS-LQ Score                 : %.1f\n", (float)((float)nValue/10.0) );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsSignalLevel( parms, objValue, MAX_TR104_OBJ_SIZE );
            printf("      Signal Level                 : %s dB\n", objValue );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsNoiseLevel( parms, objValue, MAX_TR104_OBJ_SIZE );
            printf("      Noise Level                  : %s dB\n", objValue );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsRERL( parms, objValue, MAX_TR104_OBJ_SIZE );
            printf("      Residual Echo Return Loss    : %s dB\n", objValue );
         }
         else
         {
            printf("      MOS-CQ Score                 : %s\n", szUnavailable );
            printf("      MOS-LQ Score                 : %s\n", szUnavailable );
            printf("      Signal Level                 : %s\n", szUnavailable );
            printf("      Noise Level                  : %s\n", szUnavailable );
            printf("      Residual Echo Return Loss    : %s\n", szUnavailable );
         }

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpFarEndIpAddress( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Destination RTP IP Address   : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpFarEndUDPPort( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Destination RTP Port         : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpLocalUDPPort( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Local RTP Port               : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpPacketsSent( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Number of Packets Sent       : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpBytesSent( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Number of Octets Sent        : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpPacketsReceived( parms, objValue, MAX_TR104_OBJ_SIZE );
         nRecv = atoi(objValue);
         printf("      Number of Packets Received   : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpBytesReceived( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Number of Octets Received    : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpPacketsLost( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Number of Packets Lost       : %s\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpReceiveInterarrivalJitter( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Interarrival Jitter          : %s msec\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpAverageTxDelay( parms, objValue, MAX_TR104_OBJ_SIZE );
         printf("      Average Tx Delay             : %s msec\n", objValue );

         memset(objValue, 0, MAX_TR104_OBJ_SIZE);
         dalVoice_GetCallLogSessionSrcRtpPacketsDiscarded( parms, objValue, MAX_TR104_OBJ_SIZE );
         if ( nRecv > 0 )
         {
            nValue = (atoi(objValue) * 100) / nRecv;
         }
         else
         {
            nValue = 0;
         }
         printf("      Jitter Buffer Discard Rate   : %d %%\n", nValue );

         /* Get JBRate, JBMax */
         if ( bLocalExtStatsAvail )
         {
            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsJBRate( parms, objValue, MAX_TR104_OBJ_SIZE );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsJBMaximum( parms, objValue, MAX_TR104_OBJ_SIZE );
            printf("      Jitter Buffer Maximum Size   : %s msec\n", objValue );

            memset(objValue, 0, MAX_TR104_OBJ_SIZE);
            dalVoice_GetCallLogSessionStatsRoundTripDelay( parms, objValue, MAX_TR104_OBJ_SIZE );
            printf("      Network Round Trip Delay     : %s msec\n", objValue );
         }
         else
         {
            printf("      Jitter Buffer Adjustment Rate: %s\n", szUnavailable );
            printf("      Jitter Buffer Maximum Size   : %s\n", szUnavailable );
            printf("      Network Round Trip Delay     : %s\n", szUnavailable );
         }

         /* Get call trace */
         /* Allocate memory for the call trace object to be obtained through DAL APIs. */
         callTrace = cmsMem_alloc( maxCallTraceSize, ALLOC_ZEROIZE );
         if ( callTrace )
         {
            memset( callTrace, 0, maxCallTraceSize );
            dalVoice_GetCallLogSessionStatsCallTrace( parms, callTrace, maxCallTraceSize );
            if ( callTrace[0] != '\0' )
            {
               printf("      Call Trace                   :\n");
               printf("##########\n");
               printf("%s", callTrace);
            }
            cmsMem_free( callTrace );
         }
      }
   }

   cmsMem_free( objValue );

   return ret;
}

/***************************************************************************
* Function Name: dumpVoicePotsFxsDiag
* Description  : Dump FXS Diag state and result
****************************************************************************/
static CmsRet dumpVoicePotsFxsDiag(int spNum)
{
   int numOfFxs, i;
   int fxsInst, vpInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue  = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   ret = dalVoice_GetNumPhysFxsEndpt( &numOfFxs );
   if( ret != CMSRET_SUCCESS || numOfFxs <= 0 )
   {
      printf( "\n" );
      printf( "No valid FXS line\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfFxs; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapPotsFxsNumToInst( &parms, &fxsInst ) == CMSRET_SUCCESS)
      {
         parms.op[1] = fxsInst;
         printf( "FXS  line %d:\n", i);
         printf( "-------------\n" );

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetVoiceFxsLineTxGainStr( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   txGain: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetVoiceFxsLineRxGainStr( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   rxGain: %s\n",  objValue );
         }

         if( dalVoice_GetFxsDiagTestState( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   Diag State: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetFxsDiagTestSelector( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   Select Test: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetFxsDiagTestResult( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   Result: %s\n",  objValue );
         }
      }
   }

   cmsMem_free( objValue );

   return (ret);
}

/***************************************************************************
* Function Name: dumpVoicePotsFxs
* Description  : Dump FXS state and result
****************************************************************************/
static CmsRet dumpVoicePotsFxs(int spNum)
{
   int numOfFxs, i;
   int fxsInst, vpInst;
   DAL_VOICE_PARMS parms = {0};
   CmsRet ret = CMSRET_SUCCESS;
   char* objValue  = NULL;

   /* Mapping spnum to vpInst */
   dalVoice_mapSpNumToSvcInst( spNum, &vpInst );

   /* Fill in the parameter structure needed */
   parms.op[0] = vpInst;
   ret = dalVoice_GetNumPhysFxsEndpt( &numOfFxs );
   if( ret != CMSRET_SUCCESS || numOfFxs <= 0 )
   {
      printf( "\n" );
      printf( "No valid FXS line\n");
      return  ret;
   }

   /* Allocate memory for the object to be obtained through DAL APIs. */
   objValue = cmsMem_alloc( MAX_TR104_OBJ_SIZE, ALLOC_ZEROIZE );
   if ( objValue == NULL )
   {
      cmsLog_error( "Could not get allocate memory for object.\n" );
      return ( CMSRET_RESOURCE_EXCEEDED );
   }

   for( i = 0; i < numOfFxs; i++ )
   {
      printf( "\n" );
      parms.op[1] = i;

      if( dalVoice_mapPotsFxsNumToInst( &parms, &fxsInst ) == CMSRET_SUCCESS)
      {
         parms.op[1] = fxsInst;
         printf( "FXS  line %d:\n", i);
         printf( "-------------\n" );

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetVoiceFxsLineTxGainStr( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   txGain: %s\n",  objValue );
         }

         memset( objValue, 0, MAX_TR104_OBJ_SIZE);
         if( dalVoice_GetVoiceFxsLineRxGainStr( &parms, objValue, MAX_TR104_OBJ_SIZE) == CMSRET_SUCCESS )
         {
            printf( "   rxGain: %s\n",  objValue );
         }
      }
   }

   cmsMem_free( objValue );

   return (ret);
}


#endif /* DMP_VOICE_SERVICE_2 */
#endif  /* BRCM_VOICE_SUPPORT */

#endif /* SUPPORT_CLI_CMD */

