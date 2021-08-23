/******************************************************************************
 *
<:copyright-BRCM:2018:proprietary:standard

   Copyright (c) 2018 Broadcom
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
******************************************************************************/

#include <stdio.h>
#include "os_defs.h"
#include "gponctl_api.h"
#include "gponctl_api_trace.h"
#include "cms_actionlog.h"


#define __print(fmt, arg...)                                            \
    do {                                                                \
        calLog_library(fmt, ##arg);                                     \
    } while(0)

#define __trace(cmd, fmt, arg...)                                       \
    do {                                                                \
        if(cmsActionTraceEnable_g)                                      \
            __print(#cmd " " fmt, ##arg);                               \
    } while(0)

#define apiTrace1p(p) \
    { \
        (void)p; \
        __trace(gponctl_api, "%s, %p\n", __FUNCTION__, p); \
    }


#ifdef CMS_ACTION_LOG
static int cmsActionTraceEnable_g = 1;
#else
static int cmsActionTraceEnable_g = 0;
#endif


void gponCtl_getEventStatusTrace(BCM_Ploam_EventStatusInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_maskEventTrace(BCM_Ploam_EventMaskInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setSFSDThresholdTrace(BCM_Ploam_SFSDthresholdInfo *threshold)
{
    apiTrace1p(threshold);

    __trace(==>gponctl, "%s --sf %u --sd %u\n",
      "setSfSdThreshold",
      threshold->sf_exp, threshold->sd_exp);
}

void gponCtl_getSFSDThresholdTrace(BCM_Ploam_SFSDthresholdInfo *threshold)
{
    apiTrace1p(threshold);
}

void gponCtl_startAdminStateTrace(BCM_Ploam_StartInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --oper %u\n",
      "start",
      info->initOperState);
}

void gponCtl_stopAdminStateTrace(BCM_Ploam_StopInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --gasp %u\n",
      "stop",
      info->sendDyingGasp);
}

void gponCtl_getControlStatesTrace(BCM_Ploam_StateInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setTO1TO2Trace(BCM_Ploam_TO1TO2Info *info)
{
    apiTrace1p(info);
}

void gponCtl_getTO1TO2Trace(BCM_Ploam_TO1TO2Info *info)
{
    apiTrace1p(info);
}

void gponCtl_setDWELL_TIMERTrace(BCM_Ploam_DWELL_TIMERInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getDWELL_TIMERTrace(BCM_Ploam_DWELL_TIMERInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getRebootFlagsTrace(BCM_Ploam_RebootFlagsInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setTO6Trace(BCM_Ploam_TO6Info *info)
{
    apiTrace1p(info);
}

void gponCtl_getTO6Trace(BCM_Ploam_TO6Info *info)
{
    apiTrace1p(info);
}

void gponCtl_getMessageCountersTrace(BCM_Ploam_MessageCounters *counters)
{
    apiTrace1p(counters);
}

void gponCtl_getGtcCountersTrace(BCM_Ploam_GtcCounters *counters)
{
    apiTrace1p(counters);
}

void gponCtl_getFecCountersTrace(BCM_Ploam_fecCounters *counters)
{
    apiTrace1p(counters);
}

void gponCtl_getGemPortCountersTrace(BCM_Ploam_GemPortCounters *counters)
{
    apiTrace1p(counters);
}

void gponCtl_getStatsTrace(BCM_Ploam_StatCounters *stats)
{
    apiTrace1p(stats);
}

void gponCtl_configGemPortTrace(BCM_Ploam_CfgGemPortInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --index %u --id %u --alloc %u --dsonly %u --mcast %u --encring %u\n",
      "configGemPort",
      info->gemPortIndex,
      info->gemPortID,
      info->allocID,
      info->isDsOnly,
      info->isMcast,
      info->encRing);
}

void gponCtl_configDsGemPortEncryptionByIXTrace(BCM_Ploam_GemPortEncryption *conf)
{
    apiTrace1p(conf);
}

void gponCtl_configDsGemPortEncryptionByIDTrace(BCM_Ploam_GemPortEncryption *conf)
{
    apiTrace1p(conf);
}

void gponCtl_deconfigGemPortTrace(BCM_Ploam_DecfgGemPortInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --index %u --id %u\n",
      "deconfigGemPort",
      info->gemPortIndex,
      info->gemPortID);
}

void gponCtl_enableGemPortTrace(BCM_Ploam_EnableGemPortInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --index %u --id %u --enable %u\n",
      "enableGemPort",
      info->gemPortIndex,
      info->gemPortID,
      info->enable);
}

void gponCtl_getGemPortTrace(BCM_Ploam_GemPortInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getAllocIdsTrace(BCM_Ploam_AllocIDs *allocIds)
{
    apiTrace1p(allocIds);
}

void gponCtl_getOmciPortTrace(BCM_Ploam_OmciPortInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getTcontCfgTrace(BCM_Ploam_TcontInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_configTcontAllocIdTrace(BCM_Ploam_TcontAllocIdInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --tcontIdx %u --allocId %u\n",
      "configTcontAllocId",
      info->tcontIdx,
      info->allocID);
}

void gponCtl_deconfigTcontAllocIdTrace(BCM_Ploam_TcontAllocIdInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl, "%s --tcontIdx %u --allocId %u\n",
      "deconfigTcontAllocId",
      info->tcontIdx,
      info->allocID);
}

void gponCtl_setGemBlockLengthTrace(BCM_Ploam_GemBlkLenInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getGemBlockLengthTrace(BCM_Ploam_GemBlkLenInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setTodInfoTrace(BCM_Ploam_TimeOfDayInfo *info)
{
    apiTrace1p(info);

    __trace(==>gponctl,
      "%s --enable %u --usrEvent %u --pulseWidth %u "
      "--sf %u --timeNs %u --timeSecMSB %u --timeSecLSB %u\n",
      "configTod",
      info->enable,
      info->enableUsrEvent,
      info->pulseWidth,
      info->superframe,
      info->tStampN.nanoSeconds,
      info->tStampN.secondsMSB,
      info->tStampN.secondsLSB);
}

void gponCtl_getTodInfoTrace(BCM_Ploam_TimeOfDayInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getSRIndicationTrace(BCM_Ploam_SRIndInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getPowerManagementParamsTrace(BCM_Ploam_PowerManagementParams *info)
{
    apiTrace1p(info);
}

void gponCtl_setPowerManagementParamsTrace(BCM_Ploam_PowerManagementParams *info)
{
    apiTrace1p(info);

    __trace(==>gponctl,
      "%s --mode_cap %u --mode_conf %u --itransinit %u "
      "--itxinit %u --ilowpower %u --irxoff %u --iaware %u "
      "--ihold %u --ilowpower_doze %u --ilowpower_wsleep %u\n",
      "setPWMParams",
      info->mode_cap,
      info->mode_conf,
      info->itransinit,
      info->itxinit,
      info->ilowpower,
      info->irxoff,
      info->iaware,
      info->ihold,
      info->ilowpower_doze,
      info->ilowpower_wsleep);
}

void gponCtl_getOnuIdTrace(BCM_Ploam_GetOnuIdInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getFecModeTrace(BCM_Ploam_GetFecModeInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getEncryptionKeyTrace(BCM_Ploam_GetEncryptionKeyInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setMcastEncryptionKeysTrace(BCM_Ploam_McastEncryptionKeysInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_setSerialPasswdTrace(BCM_Ploam_SerialPasswdInfo *info)
{
    int i;
    char oneByte[4];
    char sn[32] = {0};
    char passwd[128] = {0};

    apiTrace1p(info);

    sprintf(sn,
      "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
      info->serialNumber[0], info->serialNumber[1],
      info->serialNumber[2], info->serialNumber[3],
      info->serialNumber[4], info->serialNumber[5],
      info->serialNumber[6], info->serialNumber[7]);

    for (i = 0; i < BCM_PLOAM_PASSWORD_SIZE_BYTES; i++)
    {
        if (i < BCM_PLOAM_PASSWORD_SIZE_BYTES - 1)
        {
            sprintf(oneByte, "%02x-", info->password[i]);
            strncat(passwd, oneByte, 3);
        }
        else
        {
            sprintf(oneByte, "%02x", info->password[i]);
            strncat(passwd, oneByte, 2);
        }
    }

    __trace(==>gponctl, "%s --sn %s --pwd %s\n",
      "setSnPwd",
      sn, passwd);
}

void gponCtl_getSerialPasswdTrace(BCM_Ploam_SerialPasswdInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getPloamDriverVersionTrace(BCM_Gpon_DriverVersionInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_generatePrbsSequenceTrace(BCM_Ploam_GenPrbsInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getOmciCountersTrace(BCM_Omci_Counters *counters)
{
    apiTrace1p(counters);
}

void gponCtl_getOmciDriverVersionTrace(BCM_Gpon_DriverVersionInfo *info)
{
    apiTrace1p(info);
}

void gponCtl_getEncryptStateUpdateTrace(BCM_PloamGemEncryptUpd *info)
{
    apiTrace1p(info);
}

void gponCtl_getKeyEncryptionKeyTrace(BCM_Ploam_Kek *info)
{
    apiTrace1p(info);
}

void gponCtl_setOmciCtrlMasterSessionKeyTrace(BCM_PLoam_OmciCtrlMsk *info)
{
    char buf[64] = {0};

    sprintf(buf,
      "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-"
      "%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
      info->msk[0], info->msk[1],
      info->msk[2], info->msk[3],
      info->msk[4], info->msk[5],
      info->msk[6], info->msk[7],
      info->msk[8], info->msk[9],
      info->msk[10], info->msk[11],
      info->msk[12], info->msk[13],
      info->msk[14], info->msk[15]);

    apiTrace1p(info);

    __trace(==>gponctl, "%s --valid %u --key %s\n",
      "setMsk",
      info->isValid,
      buf);
}
