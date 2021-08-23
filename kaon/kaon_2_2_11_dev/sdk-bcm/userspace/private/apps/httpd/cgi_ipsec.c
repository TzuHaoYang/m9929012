/***********************************************************************
 *
 *  Copyright (c) 2005-2007  Broadcom Corporation
 *  All Rights Reserved
 *
<:label-BRCM:2011:proprietary:standard

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "cms.h"
#include "httpd.h"
#include "cgi_cmd.h"
#include "cgi_ipsec.h"
#include "cgi_main.h"
#include "secapi.h"
#include "syscall.h"
#include "cms_util.h"
#include "cms_core.h"
#include "cms_dal.h"

#ifdef SUPPORT_IPSEC

extern WEB_NTWK_VAR glbWebVar;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

struct cgiIPSecHashList {
   char *proc; /* /proc/crypto keywords */
   char *ph1;  /* ph1 option value expected by html page */
   char *ph2;  /* ph2 option value expected by html page */
   char *text; /* actual text displayed on the html page */
};

struct cgiIPSecHashList bcmIPSecHashList[] = {
   {
      .proc = "md5",
      .ph1  = "md5",
      .ph2  = "hmac_md5",
      .text = "MD5"
   },
   {
      .proc = "sha1",
      .ph1  = "sha1",
      .ph2  = "hmac_sha1",
      .text = "SHA1"
   },
   {
      .proc = "sha256",
      .ph1  = "sha256",
      .ph2  = "hmac_sha256",
      .text = "SHA256"
   },
   {
      .proc = "sha384",
      .ph1  = "sha384",
      .ph2  = "hmac_sha384",
      .text = "SHA384"
   },
   {
      .proc = "sha512",
      .ph1  = "sha512",
      .ph2  = "hmac_sha512",
      .text = "SHA512"
   }
};

struct cgiIPSecEncryptList {
   char *proc;   /* /proc/crypto keywords */
   char *option; /* option value expected by html page */
   char *text;   /* actual text displayed on the html page */
};

struct cgiIPSecEncryptList bcmIPSecEncryptList[] = {
   {
      .proc   = "des",
      .option = "des",
      .text   = "DES"
   },
   {
      .proc   = "des3_ede",
      .option = "3des",
      .text   = "3DES"
   },
   {
      .proc   = "aes",
      .option = "aes",   /* aes128, aes192, aes256 */
      .text   = "AES - " /* AES - 128, AES - 192, AES - 256 */
   },
   {
      .proc   = "cipher_null",
      .option = "null_enc",
      .text   = "NULL Enryption"
   }
};

void writeIPSecScript(FILE *fs);

void writeIPSecHeader(FILE *fs)
{
   fprintf(fs, "<html><head>\n");
   fprintf(fs, "<link rel=stylesheet href='stylemain.css' type='text/css'>\n");
   fprintf(fs, "<link rel=stylesheet href='colors.css' type='text/css'>\n");
   fprintf(fs, "<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>\n");
   fprintf(fs, "<script language=\"javascript\" src=\"portName.js\"></script>\n");
}

void writeIPSecBody(FILE *fs)
{
   fprintf(fs, "</head>\n<body>\n<blockquote>\n<form>\n");
}

int bcmIPSecNumEntries();
int bcmIPSecNumEntries_igd();
int bcmIPSecNumEntries_dev2();
#if defined(SUPPORT_DM_LEGACY98)
#define bcmIPSecNumEntries()  bcmIPSecNumEntries_igd()
#elif defined(SUPPORT_DM_HYBRID)
#define bcmIPSecNumEntries()  bcmIPSecNumEntries_igd()
#elif defined(SUPPORT_DM_PURE181)
#define bcmIPSecNumEntries()  bcmIPSecNumEntries_dev2()
#elif defined(SUPPORT_DM_DETECT)
#define bcmIPSecNumEntries()   (cmsMdm_isDataModelDevice2() ? \
                              bcmIPSecNumEntries_dev2()   : \
                              bcmIPSecNumEntries_igd())
#endif

void fillIPSecInfo(FILE *fs);
void fillIPSecInfo_igd(FILE *fs);
void fillIPSecInfo_dev2(FILE *fs);
#if defined(SUPPORT_DM_LEGACY98)
#define fillIPSecInfo(f)  fillIPSecInfo_igd((f))
#elif defined(SUPPORT_DM_HYBRID)
#define fillIPSecInfo(f)  fillIPSecInfo_igd((f))
#elif defined(SUPPORT_DM_PURE181)
#define fillIPSecInfo(f)  fillIPSecInfo_dev2((f))
#elif defined(SUPPORT_DM_DETECT)
#define fillIPSecInfo(f)   (cmsMdm_isDataModelDevice2() ? \
                         fillIPSecInfo_dev2((f))   : \
                         fillIPSecInfo_igd((f)))
#endif


#if !defined(SUPPORT_DM_PURE181)
void fillIPSecInfo_igd(FILE *fs)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj = NULL;
   CmsRet ret;

   while ( (ret = cmsObj_getNext
         (MDMOID_IP_SEC_CFG, &iidStack, (void **) &ipsecObj)) == CMSRET_SUCCESS) 
   {
      fprintf(fs, "   <tr>\n");
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->connName);
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->ipVer);
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->tunMode);
      if (strcmp(ipsecObj->keyExM, "manual") == 0) {
         fprintf(fs, "      <td>Manual</td>");
      } else {
         if (!strcmp(ipsecObj->authM, "pre_shared_key")) {
            fprintf(fs, "      <td>Pre-Shared Key</td>\n");
         } else {
            fprintf(fs, "      <td>Certificate(%s)</td>\n", ipsecObj->certificateName);
         }
      }
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->localGwIf);
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->remoteGWAddress);
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->localIPAddress);
      fprintf(fs, "      <td>%s</td>\n", ipsecObj->remoteIPAddress);
      fprintf(fs, "      <td align='center'><input type='checkbox' name='rml' value='%s'></td>\n", ipsecObj->connName);
      fprintf(fs, "   </tr>\n");
      // Free the mem allocated this object by the get API.
      cmsObj_free((void **) &ipsecObj);
   }
}

int bcmIPSecNumEntries_igd()
{
   CmsRet ret = CMSRET_SUCCESS;
   int numTunCfg = 0;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj = NULL;

   while ( (ret = cmsObj_getNext
         (MDMOID_IP_SEC_CFG, &iidStack, (void **) &ipsecObj)) == CMSRET_SUCCESS) 
   {
      numTunCfg++;
      // Free the mem allocated this object by the get API.
      cmsObj_free((void **) &ipsecObj);
   }
   return numTunCfg;
}
#endif

// Print the content of drop down list for certificate selection in edit page
void cgiPrintCertList(char *print)
{
   char *prtloc = print;

#ifdef SUPPORT_CERT
   CertificateCfgObject *certObj;
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   int wsize;

   while (cmsObj_getNext(MDMOID_CERTIFICATE_CFG, &iidStack,
                         (void **) &certObj) == CMSRET_SUCCESS)
   {
      if (strcmp(certObj->type, CERT_TYPE_SIGNED) == 0) {
         sprintf(prtloc, "<option value=\"%s\">%s</option>\n%n", 
                 certObj->name, certObj->name, &wsize); prtloc +=wsize;
      }
      cmsObj_free((void **) &certObj);
    }
#endif
    *prtloc = 0;
}

/*
 * This routine loads the names of all IPSec tunnels into an
 * array in Javascript format
 */
void cgiLoadIPSecList_igd(char *varValue)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj=NULL;
   char *prtloc = varValue;
   int  wsize;

   cmsLog_debug("Enter");

   sprintf(prtloc, "[%n", &wsize); prtloc +=wsize;
   if (cmsObj_getNext(MDMOID_IP_SEC_CFG, &iidStack, (void **) &ipsecObj) == CMSRET_SUCCESS)
   {
      sprintf(prtloc, "\"%s\"%n", ipsecObj->connName, &wsize); prtloc +=wsize;
      cmsObj_free((void **) &ipsecObj);
      while (cmsObj_getNext(MDMOID_IP_SEC_CFG, &iidStack,
                            (void **) &ipsecObj) == CMSRET_SUCCESS)
      {
         sprintf(prtloc, ", \"%s\"%n", ipsecObj->connName, &wsize); prtloc +=wsize;
         cmsObj_free((void **) &ipsecObj);
       }
   }
   sprintf(prtloc, "]%n", &wsize); prtloc +=wsize;
   *prtloc = 0;
}

/*
 * This routine loads the SPI of all manually configured IPSec tunnels into an
 * array in Javascript format
 */
void cgiLoadSPIList_igd(char *varValue)
{
   InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
   IPSecCfgObject *ipsecObj=NULL;
   char *prtloc = varValue;
   int  wsize;

   cmsLog_debug("Enter");

   sprintf(prtloc, "[%n", &wsize); prtloc +=wsize;
   if (cmsObj_getNext(MDMOID_IP_SEC_CFG, &iidStack, (void **) &ipsecObj) == CMSRET_SUCCESS)
   {
      sprintf(prtloc, "\"%s\"%n", ipsecObj->SPI, &wsize); prtloc +=wsize;
      cmsObj_free((void **) &ipsecObj);
      while (cmsObj_getNext(MDMOID_IP_SEC_CFG, &iidStack,
                            (void **) &ipsecObj) == CMSRET_SUCCESS)
      {
         sprintf(prtloc, ", \"%s\"%n", ipsecObj->SPI, &wsize); prtloc +=wsize;
         cmsObj_free((void **) &ipsecObj);
       }
   }
   sprintf(prtloc, "]%n", &wsize); prtloc +=wsize;
   *prtloc = 0;
}

/*
 * This routine creates the list of options for PH1 and PH2 integrity
 * algorithms that should be offered on the IPSec WebGUI configuration
 * page. The routine reads from /proc/crypto and parses for the supported
 * entries that ends with either -iproc or -spu. Those are the crypto algos
 * registered by the underlying SPU driver (e.g. *-iproc entries are created
 * by spudd/impl4 driver and *-spu are created by spudd/impl2 and spudd/impl3
 * drivers). This allows displaying the proper algorithms based on what the
 * driver registers to the kernel. In the case where the algorithms aren't
 * supported by the underyling driver (including the case where the HW
 * assisted engine doesn't exist or isn't enabled), the function will check
 * if the equivalent features are supported by the kernel and adjust
 * accordingly.
 */
void cgiLoadIPSecHashList(int argc __attribute__((unused)), char **argv, char *varValue)
{
   FILE *fp; 
   char line[256];
   char *prtloc = varValue;
   int  wsize;
   char *optionValue;
   unsigned int i;
   char *found = NULL;

   fp = fopen("/proc/crypto", "r");
   if (!fp) {
      cmsLog_error("Failed to open /proc/crypto");
      return;
   }

   /* search for supported entries and create the option list in the right order */
   for (i = 0; i < ARRAY_SIZE(bcmIPSecHashList); i++) {
      found = NULL;
      while (fgets(line, sizeof(line), fp)) {
         /* check if the line contains the given sha type. */
         if (strstr(line, bcmIPSecHashList[i].proc)) {  /* found the entry */
            /* parse for supported keywords */
            if (strstr(line, "spu") || strstr(line, "iproc")) {
              found = "(hw)"; /* found HW assistance, stop looking */
              break;
            }
            else if (strstr(line, "generic")) {
              found = "(sw)";
              /* don't break... keep searching for HW support */
            }
         }
      }
      if (found) {
         if (cmsUtl_strcmp(argv[2], "ph1") == 0) {
            optionValue = bcmIPSecHashList[i].ph1;
         } else {
            optionValue = bcmIPSecHashList[i].ph2;
         }
         sprintf(prtloc, "<option value=\"%s\">%s %s</option>\n%n",
                 optionValue, bcmIPSecHashList[i].text, found, &wsize); prtloc +=wsize;
      }

      /* Reset fp to the beginning of the file */
      fseek(fp, 0, SEEK_SET);
   }
   fclose(fp);
}

/*
 * This routine creates the list of options for PH1 and PH2 encryption
 * algorithms that should be offered on the IPSec WebGUI configuration
 * page. The routine reads from /proc/crypto and parses for the supported
 * entries that ends with either -iproc or -spu. Those are the crypto algos
 * registered by the underlying SPU driver (e.g. *-iproc entries are created
 * by spudd/impl4 driver and *-spu are created by spudd/impl2 and spudd/impl3
 * drivers). This allows displaying the proper algorithms based on what the
 * driver registers to the kernel. In the case where the algorithms aren't
 * supported by the underyling driver (including the case where the HW
 * assisted engine doesn't exist or isn't enabled), the function will check
 * if the equivalent features are supported by the kernel and adjust
 * accordingly.
 */
void cgiLoadIPSecEncryptList(int argc __attribute__((unused)), char **argv, char *varValue)
{
   FILE *fp; 
   char line[256];
   char *prtloc = varValue;
   int  wsize;
   unsigned int i, j;
   char *found = NULL;

   fp = fopen("/proc/crypto", "r");
   if (!fp) {
      cmsLog_error("Failed to open /proc/crypto");
      return;
   }

   /* search for supported entries and create the option list in the right order */
   for (i = 0; i < ARRAY_SIZE(bcmIPSecEncryptList); i++) {
      found = NULL;
      while (fgets(line, sizeof(line), fp)) {
         /* check if the line contains the given crypto type. */
         if (strstr(line, bcmIPSecEncryptList[i].proc)) {  /* found the entry */
            /* parse for supported keywords */
            if (strstr(line, "spu") || strstr(line, "iproc")) {
              found = "(hw)";
              break; /* found HW assistance, stop looking */
            }
            else if (strstr(line, "generic")) {
              found = "(sw)";
              /* don't break... keep searching for HW support */
            }
         }
      }
      if (found) {
         if (strcmp(bcmIPSecEncryptList[i].option, "aes") == 0) {
            const char *aes_key[] = { "128", "192", "256" };
            for (j = 0; j < (sizeof(aes_key) / sizeof(const char *)); j++) {
              sprintf(prtloc, "<option value=\"%s%s\">%s%s %s</option>\n%n",
                      bcmIPSecEncryptList[i].option, aes_key[j],
                      bcmIPSecEncryptList[i].text, aes_key[j], found,
                      &wsize); prtloc +=wsize;
            }
         } else if ((cmsUtl_strcmp(argv[2], "ph2") == 0) ||
                    ((cmsUtl_strcmp(argv[2], "ph1") == 0) &&
                     (strcmp(bcmIPSecEncryptList[i].option, "null_enc"))))
         {
            sprintf(prtloc, "<option value=\"%s\">%s %s</option>\n%n",
                    bcmIPSecEncryptList[i].option, bcmIPSecEncryptList[i].text, found,
                    &wsize); prtloc +=wsize;
         }
      }

      /* Reset fp to the beginning of the file */
      fseek(fp, 0, SEEK_SET);
   }
   fclose(fp);
}

void cgiIPSec(char *query, FILE *fs) 
{
   char action[BUFLEN_264];

   cgiGetValueByName(query, "action", action);

   if ( strcmp(action, "add") == 0 )
   {
      cgiIPSecAdd(query, fs);
   }
   else if ( strcmp(action, "remove") == 0 )
   {
      cgiIPSecRemove(query, fs);
   }
   else if ( strcmp(action, "save") == 0 )
   {
      cgiIPSecSave(fs);
   }
   else
   {
      cgiIPSecView(fs);
   }
}


void cgiIPSecAdd(char *query, FILE *fs) 
{
   char cmdPR[BUFLEN_128];
   char ph1KeyTime[BUFLEN_16];
   char ph2KeyTime[BUFLEN_16];
   CmsRet ret = CMSRET_SUCCESS;

   cmdPR[0] = ph1KeyTime[0] = ph2KeyTime[0] = '\0';
   cgiGetValueByName(query, "ipsConnName", glbWebVar.ipsConnName);
   cgiGetValueByName(query, "ipsIpver", glbWebVar.ipsIpver);
   cgiGetValueByName(query, "ipsLocalGwIf", glbWebVar.ipsLocalGwIf);
   cgiGetValueByName(query, "ipsTunMode", glbWebVar.ipsTunMode);
   cgiGetValueByName(query, "ipsRemoteGWAddr", glbWebVar.ipsRemoteGWAddr);
   cgiGetValueByName(query, "ipsLocalIPMode", glbWebVar.ipsLocalIPMode);
   cgiGetValueByName(query, "ipsLocalIP", glbWebVar.ipsLocalIP);
   cgiGetValueByName(query, "ipsLocalMask", glbWebVar.ipsLocalMask);
   cgiGetValueByName(query, "ipsRemoteIPMode", glbWebVar.ipsRemoteIPMode);
   cgiGetValueByName(query, "ipsRemoteIP", glbWebVar.ipsRemoteIP);
   cgiGetValueByName(query, "ipsRemoteMask", glbWebVar.ipsRemoteMask);
   cgiGetValueByName(query, "ipsKeyExM", glbWebVar.ipsKeyExM);
   cgiGetValueByName(query, "ipsAuthM", glbWebVar.ipsAuthM);
   cgiGetValueByName(query, "ipsPSK", glbWebVar.ipsPSK);
   cgiGetValueByName(query, "ipsCertificateName", glbWebVar.ipsCertificateName);
   cgiGetValueByName(query, "ipsPerfectFSEn", glbWebVar.ipsPerfectFSEn);
   cgiGetValueByName(query, "ipsManualEncryptionAlgo", 
                     glbWebVar.ipsManualEncryptionAlgo);
   cgiGetValueByName(query, "ipsManualEncryptionKey", 
                     glbWebVar.ipsManualEncryptionKey);
   cgiGetValueByName(query, "ipsManualAuthAlgo", glbWebVar.ipsManualAuthAlgo);
   cgiGetValueByName(query, "ipsManualAuthKey", glbWebVar.ipsManualAuthKey);
   cgiGetValueByName(query, "ipsSPI", glbWebVar.ipsSPI);
   cgiGetValueByName(query, "ipsPh1Mode", glbWebVar.ipsPh1Mode);
   cgiGetValueByName(query, "ipsPh1EncryptionAlgo", glbWebVar.ipsPh1EncryptionAlgo);
   cgiGetValueByName(query, "ipsPh1IntegrityAlgo", glbWebVar.ipsPh1IntegrityAlgo);
   cgiGetValueByName(query, "ipsPh1DHGroup", glbWebVar.ipsPh1DHGroup);
   cgiGetValueByName(query, "ipsPh1KeyTime", ph1KeyTime);
   glbWebVar.ipsPh1KeyTime = atoi(ph1KeyTime);
   cgiGetValueByName(query, "ipsPh2EncryptionAlgo", glbWebVar.ipsPh2EncryptionAlgo);
   cgiGetValueByName(query, "ipsPh2IntegrityAlgo", glbWebVar.ipsPh2IntegrityAlgo);
   cgiGetValueByName(query, "ipsPh2DHGroup", glbWebVar.ipsPh2DHGroup);
   cgiGetValueByName(query, "ipsPh2KeyTime", ph2KeyTime);
   glbWebVar.ipsPh2KeyTime = atoi(ph2KeyTime);

   if (bcmIPSecNumEntries() < MAX_IPSEC_TUNNELS) {
      /*
       * Create a new object instance and add it to the
       * tunnel configuration.
       */
      ret = dalIPSec_addTunnel(&glbWebVar);
   } else {
      cmsLog_error("IPSec Error: Tunnel table full");
      sprintf(cmdPR, "Configure IPSec Tunnel error because max tunnels exceeded");
      return;
   }
   if ( ret != CMSRET_SUCCESS ) 
   {
      sprintf(cmdPR, "Add IPSec Tunnel named %s failed. Status: %d.", 
              glbWebVar.ipsConnName, ret);
      cgiWriteMessagePage(fs, "IPSec Tunnel Add Error", cmdPR, 
                          "ipsec.cmd?action=view");
   } 
   else 
   {
      glbSaveConfigNeeded = TRUE;
      cgiIPSecView(fs);
   }

}


void cgiIPSecRemove(char *query, FILE *fs) 
{
   char *pToken = NULL, *pLast = NULL;
   char lst[BUFLEN_1024], cmd[BUFLEN_264];
   CmsRet ret = CMSRET_SUCCESS;

   cgiGetValueByName(query, "rmLst", lst);
   pToken = strtok_r(lst, ",", &pLast);
   while ( pToken != NULL )
   {
      ret = dalIPSec_deleteTunnel(pToken);
      if ( ret != CMSRET_SUCCESS )
         break;

      pToken = strtok_r(NULL, ",", &pLast);
   }

   if ( ret == CMSRET_SUCCESS )
   {
      glbSaveConfigNeeded = TRUE;
      cgiIPSecView(fs);
   }
   else
   {
      sprintf(cmd, "Cannot remove IPSec configuration entry.<br>" 
                   "Status: %d.", ret);
      cgiWriteMessagePage(fs, "IPSec Config Remove Error", 
                          cmd, "ipsec.cmd?action=view");
   }
}



void cgiIPSecSave(FILE *fs)
{
   glbSaveConfigNeeded = TRUE;
   cgiIPSecView(fs);
}


void cgiIPSecView(FILE *fs) 
{
   writeIPSecHeader(fs);
   writeIPSecScript(fs);
   writeIPSecBody(fs);
   fprintf(fs, "<b>IPSec Tunnel Mode Connections</b><br><br>\n");
   fprintf(fs, "Add, remove or enable/disable IPSec tunnel connections from this page.<br><br>\n");
   fprintf(fs, "<center>\n");
   fprintf(fs, "<table border='1' cellpadding='4' cellspacing='0'>\n");
   /* write table header */
   fprintf(fs, "   <tr>\n");
   fprintf(fs, "      <td class='hd'>Connection Name</td>\n");
   fprintf(fs, "      <td class='hd'>IP Version</td>\n");
   fprintf(fs, "      <td class='hd'>Tunnel Mode</td>\n");
   fprintf(fs, "      <td class='hd'>Key Exchange Method</td>\n");
   fprintf(fs, "      <td class='hd'>Local Gateway Interface</td>\n");
   fprintf(fs, "      <td class='hd'>Remote Gateway</td>\n");
   fprintf(fs, "      <td class='hd'>Local Addresses</td>\n");
   fprintf(fs, "      <td class='hd'>Remote Addresses</td>\n");
   fprintf(fs, "      <td class='hd'>Remove</td>\n");
   fprintf(fs, "   </tr>\n");

   /* write table body */
   fillIPSecInfo(fs);
   
   fprintf(fs, "</table><br>\n");
   fprintf(fs, "<input type='button' onClick='addClick()' value='Add New Connection'>\n");
   fprintf(fs, "<input type='button' onClick='removeClick(this.form.rml)' value='Remove'><br><br>\n");
   
   fprintf(fs, "</center>\n");
   fprintf(fs, "</form>\n");
   fprintf(fs, "</blockquote>\n");
   fprintf(fs, "</body>\n");
   fprintf(fs, "</html>\n");
   fflush(fs);
}

void writeIPSecScript(FILE *fs) 
{
   fprintf(fs, "<script language='javascript'>\n");
   fprintf(fs, "<!-- hide\n\n");

   /*generate new session key in glbCurrSessionKey*/
   cgiGetCurrSessionKey(0,NULL,NULL);

   fprintf(fs, "function addClick() {\n");
   fprintf(fs, "   var loc = 'ipsconfig.html';\n\n");
   fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");

   fprintf(fs, "function removeClick(rml) {\n");
   fprintf(fs, "   var lst = '';\n");
   fprintf(fs, "   if (rml.length > 0)\n");
   fprintf(fs, "      for (i = 0; i < rml.length; i++) {\n");
   fprintf(fs, "         if ( rml[i].checked == true )\n");
   fprintf(fs, "            lst += rml[i].value + ',';\n");
   fprintf(fs, "      }\n");
   fprintf(fs, "   else if ( rml.checked == true )\n");
   fprintf(fs, "      lst = rml.value;\n");
   fprintf(fs, "   var loc = 'ipsec.cmd?action=remove&rmLst=' + lst;\n\n");
   fprintf(fs, "   loc += '&sessionKey=%d';\n",glbCurrSessionKey);
   fprintf(fs, "   var code = 'location=\"' + loc + '\"';\n");
   fprintf(fs, "   eval(code);\n");
   fprintf(fs, "}\n\n");
   fprintf(fs, "// done hiding -->\n");
   fprintf(fs, "</script>\n");
}

#endif /* SUPPORT_IPSEC */
