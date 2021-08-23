/******************************************************************************
 *
 * <:copyright-BRCM:2018:proprietary:standard
 * 
 *    Copyright (c) 2018 Broadcom 
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
 *****************************************************************************/

#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <tee_client_api.h>

#include "sec_key_lib.h"

uint8_t sample_cert[] = "\
>>start*****************************************************************************  \n \
 ********************************************************************************end<<\n \
";

uint8_t test_priv_key[] = "-----BEGIN RSA PRIVATE KEY-----MIICWwIBAAKBgQC7hf6yAwSyXf+vzay1TaZy9HpQO/V87KssobUda4QKoivwDJHI31S0vsHUQk3hd8u4fNhCSvvXhjN6VjOWHn09LTZXBx0eO5kJjiAdiTrCC/dZ9/fyzxQN9P/TFVSCwBdETYk638Cz5pe4jWKk1zpW1+mlGWtMU+R7woFN5TGkPQIDAQABAoGAXv2bbTUWNfxjvvxi1lyFSooJQ4d77VI9y5gGlOaXtoM72pejaaunzv8qVIlZxjLW4ZdsPPia5iw2+2ubKho7udjXXSn+KtL/aLRrO99FN9xvQyYZvrjzew8/PjAc9SaJE1EsWZBIl5eyIhgoxKLlueKxQXMtqAhCS6dGqNDPqYkCQQDuvqac9GStQbOch4Nf1jjm9TnaJ0SQRSeR8FOuqUTedtCnUd5JaXWLHZjs0PxTfP1T3UL3T63WYklJ7Ptf8na3AkEAyROgjL3i1VhXKdfFXzmDaPeUYIbI2A+o0HNo45VRAmH14D0bzrGe/kPOhHZapf2QO2YoQ6S/ZxzHWUzGsA5oqwJAS7t0dLNdwEvoIs4l0V/N+w1tBZORP0aAj92xXeVZ8Eu2Um042wa1/6Qq32xHrmAmp9S3KmY/GZNh9i5TaxMprwJABQtE/8LTqd5pcVdEUDs1HLD5O+KlryXVakU64FqiiZjDfjiKNgkmn+I7j/8YDwnpUFQjtm6vjnqeVZMjeM8juQJAcKv9zoT/QnpQp/7q4bOEbMDbm04uLlT7Lr85etOVExFzjkbMBD0dD2mih5r3HxVPmOFMfKlO8L+6k2Ooo5yzCw==-----END RSA PRIVATE KEY-----";

uint8_t test_publ_key[] = "-----BEGIN PUBLIC KEY-----MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC7hf6yAwSyXf+vzay1TaZy9HpQO/V87KssobUda4QKoivwDJHI31S0vsHUQk3hd8u4fNhCSvvXhjN6VjOWHn09LTZXBx0eO5kJjiAdiTrCC/dZ9/fyzxQN9P/TFVSCwBdETYk638Cz5pe4jWKk1zpW1+mlGWtMU+R7woFN5TGkPQIDAQAB-----END PUBLIC KEY-----";



/* This UUID is generated with uuidgen
   the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html */
#define TA_SEC_KEY_UUID { 0x8aaaf201, 0x2450, 0x11e4, \
      { 0xab, 0xe2, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b} }


/* The TAFs ID implemented in this TA */
int main(int argc, char *argv[])
{
   TEEC_Result res;
   TEEC_Context ctx;
   TEEC_Session sess;
   TEEC_UUID uuid = TA_SEC_KEY_UUID;
   uint32_t err_origin;
   int fd;
   char input[128];
   char *cmd, *param1, *param2, *param3, *param4;

   argc = argc;
   argv = argv;

   SK_SessionHandle ses_handle;

   // FIFO file path
   char * secfifo = "/data/sec_fifo";
   char * secfifo1 = "/data/sec_fifo1";

   /* Create FIFO to receive command for this app */
   if (access(secfifo, F_OK) == -1){
      mkfifo(secfifo, 0666);
      fd = open(secfifo, O_RDONLY);
   }
   else{
      mkfifo(secfifo1, 0666);
      fd = open(secfifo1, O_RDONLY);
   }

   /* Initialize a context connecting us to the TEE */
   res = TEEC_InitializeContext(NULL, &ctx);
   if (res != TEEC_SUCCESS)
      errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

   /*
    * Open a session to the "hello world" TA, the TA will print "hello
    * world!" in the log when the session is created.
    */
   res = TEEC_OpenSession(&ctx, &sess, &uuid,
                          TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
   if (res != TEEC_SUCCESS)
      errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
           res, err_origin);

   while(1){
      int i = 0;
      int more = 0;

      /* Parse command and the parameters */
      while (!(more = read(fd, input, sizeof(input)))) usleep(100000);
      cmd = input;
      while((i < more) && (input[i] != ' ') && (input[i] != '\n')) i++;
      input[i] = 0;
      param1 = (i < more) ? &input[++i] : NULL;
      while((i < more) && (input[i] != ' ') && (input[i] != '\n')) i++;
      input[i] = 0;
      param2 = (i < more) ? &input[++i] : NULL;
      while((i < more) && (input[i] != ' ') && (input[i] != '\n')) i++;
      input[i] = 0;
      param3 = (i < more) ? &input[++i] : NULL;
      while((i < more) && (input[i] != ' ') && (input[i] != '\n')) i++;
      input[i] = 0;
      param4 = (i < more) ? &input[++i] : NULL;
      while((i < more) && (input[i] != ' ') && (input[i] != '\n')) i++;
      input[i] = 0;

      if(1){
        printf("input: >%s<  ", cmd);
        if(param1) printf(">%s<  ", param1);
        if(param2) printf(">%s<  ", param2);
        if(param3) printf(">%s<  ", param3);
        if(param4) printf(">%s<  ", param4);
        printf("\n");
      }

      /* Process the user request */
      switch(atoi(cmd)){
      case 0:
        printf("Bye!!");
        TEEC_CloseSession(&sess);
        TEEC_FinalizeContext(&ctx);
        return 0;

      case 1:
        printf("Invoking TA to create an object \n");
        if (TEEC_SUCCESS == SK_CreateObject (&sess, param1, strlen(param1), &ses_handle)){
           printf ("Success: Created an object\n");
           SK_CloseObject(&ses_handle);
        }
        else
           printf ("Failure: Object not created\n");
        break;

      case 2:
        printf("Invoking TA to look for an object \n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)){
           printf ("Success: Found an object\n");
           SK_CloseObject(&ses_handle);
        }
        else
           printf ("Failure: Object not found\n");
        break;

      case 3:
        printf("Invoking TA to set a public key \n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           SK_SetPubKey(&ses_handle, test_publ_key, strlen((char*)test_publ_key));
           SK_CloseObject(&ses_handle);
        }
        break;

      case 4:
        printf("Invoking TA to set a digital certificate \n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           printf("Try setting digital certificate\n");
           SK_SetCertificate(&ses_handle, sample_cert, sizeof(sample_cert));
           SK_CloseObject(&ses_handle);
        }
        break;

      case 5:
        printf("Invoking TA to set private key \n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           SK_SetPrivKey(&ses_handle, test_priv_key, strlen((char*)test_priv_key), NULL, 0);
           SK_CloseObject(&ses_handle);
        }
        break;

      case 6:
        printf("Invoking TA to generate private key \n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           SK_GenerateKey(&ses_handle, atoi(param2));
           SK_CloseObject(&ses_handle);
           printf("Success in generating private key\n");
        }
        else {
           printf("Failed: in generating private key\n");
        }
        break;

      case 7:
        printf("Invoking TA to do encryption\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[64];
           uint8_t crypt_text[64];
           uint32_t out_len;

           printf("Opened the object\n");
           fpr = fopen(param2, "r");
           fpw = fopen(param3, "w");

           printf("Opened the files\n");
           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_AES_CTR, SK_MODE_ENCRYPT);

           while((out_len = fread(plain_text, 1, 64, fpr)) == 64) {
             SK_CryptoUpdate(&ses_handle, plain_text, sizeof(plain_text), crypt_text, &out_len);
             fwrite(crypt_text, 1, out_len, fpw);
           }
           SK_CryptoFinal(&ses_handle, plain_text, out_len, crypt_text, &out_len);
           fwrite(crypt_text, 1, out_len, fpw);

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;

      case 8:
        printf("Invoking TA to do decryption\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[64];
           uint8_t crypt_text[64];
           uint32_t out_len;

           fpr = fopen(param2, "r");
           fpw = fopen(param3, "w");

           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_AES_CTR, SK_MODE_DECRYPT);

           while((out_len = fread(crypt_text, 1, 64, fpr)) == 64) {
             SK_CryptoUpdate(&ses_handle, crypt_text, sizeof(crypt_text), plain_text, &out_len);
             fwrite(plain_text, 1, out_len, fpw);
           }
           SK_CryptoFinal(&ses_handle, crypt_text, out_len, plain_text, &out_len);
           fwrite(plain_text, 1, out_len, fpw);

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;

      case 9:
        printf("Invoking TA to do asymmetric encryption\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[128];
           uint8_t crypt_text[128];
           uint32_t out_len;

           fpr = fopen(param2, "r");
           fpw = fopen(param3, "w");

           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_RSA, SK_MODE_ENCRYPT);

           while((out_len = fread(plain_text, 1, 128, fpr)) == 128) {
             SK_CryptoUpdate(&ses_handle, plain_text, sizeof(plain_text), crypt_text, &out_len);
             fwrite(crypt_text, 1, out_len, fpw);
           }
           SK_CryptoFinal(&ses_handle, plain_text, out_len, crypt_text, &out_len);
           fwrite(crypt_text, 1, out_len, fpw);

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;

      case 10:
        printf("Invoking TA to do asymmetric decryption\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[128];
           uint8_t crypt_text[128];
           uint32_t out_len;

           fpr = fopen(param2, "r");
           fpw = fopen(param3, "w");

           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_RSA, SK_MODE_DECRYPT);

           while((out_len = fread(plain_text, 1, 128, fpr)) == 128) {
             SK_CryptoUpdate(&ses_handle, plain_text, sizeof(plain_text), crypt_text, &out_len);
             fwrite(crypt_text, 1, out_len, fpw);
           }
           SK_CryptoFinal(&ses_handle, plain_text, out_len, crypt_text, &out_len);
           fwrite(crypt_text, 1, out_len, fpw);

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;


      case 11:
        printf("Invoking TA to do asymmetric sign\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[32];
           uint8_t crypt_text[128];
           uint32_t out_len;

           fpr = fopen(param2, "r");
           fpw = fopen(param3, "w");

           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_RSA_SHA256, SK_MODE_SIGN);

           if ((out_len = fread(plain_text, 1, 32, fpr)) == 32) {
             out_len = atoi(param4);
             SK_CryptoSign(&ses_handle, plain_text, sizeof(plain_text), crypt_text, &out_len);
             fwrite(crypt_text, 1, out_len, fpw);
           }

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;

      case 12:
        printf("Invoking TA to do asymmetric verify\n");
        if (TEEC_SUCCESS == SK_OpenObject (&sess, param1, strlen(param1), &ses_handle)) {
           FILE *fpr = NULL, *fpw = NULL;
           uint8_t plain_text[32];
           uint8_t sign_text[128];
           uint32_t sign_len;
           uint32_t match;

           fpr = fopen(param2, "r");
           fpw = fopen(param3, "r");

           if (!(fpw && fpr)){
              printf ("Error accessing files %s %s\n", param1, param2);
              SK_CloseObject(&ses_handle);
              if(fpr) fclose(fpr);
              if(fpw) fclose(fpw);
              return 0;
           }

           SK_CryptoInit(&ses_handle, SK_RSA_SHA256, SK_MODE_VERIFY);

           sign_len = atoi(param4);
           if ((fread(plain_text, 1, 32, fpr)) == 32) {
             if ((fread(sign_text, 1, sign_len, fpw)) == sign_len) {
                SK_CryptoVerify(&ses_handle, plain_text, sizeof(plain_text), sign_text, sign_len, &match);
                if (match)
                  printf("Signature Matched!!\n");
                else
                  printf("Signature FAILED\n");
             }
           }

           fclose(fpr);
           fclose(fpw);
           SK_CloseObject(&ses_handle);
        }
        break;

      }
   }

   return 0;
}

