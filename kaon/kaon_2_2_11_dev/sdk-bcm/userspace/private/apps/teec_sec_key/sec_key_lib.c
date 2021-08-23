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

#include <tee_client_api.h>
#include "sec_key_lib.h"

/*****************************************************************************
*  FUNCTION:   SK_CreateObject
*
*  PURPOSE:    Create a crypto object (key) to perfrom crypto operations
*
*  PARAMETERS: sess       - [IN]  Pointer to the open session
*              object_id  - [IN]  ID to identify the crypto object (max 32 bytes)
*              object_len - [IN]  Length of the ID
*              handle     - [OUT] Handle to the crypto session 
*                                 with this crypto object
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CreateObject(TEEC_Session *sess, void* object_id, uint8_t object_len, SK_SessionHandle *handle)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(sess && object_id && handle) || (object_len > SK_MAX_OBJ_LEN))
      return TEEC_ERROR_BAD_PARAMETERS;

   /* Initialize the input params */
   memset(&handle->tee_ses, 0, sizeof(TEEC_Session));
   handle->tee_ses_id = 0xFFFFFFFF;

   memset(&op, 0, sizeof(op));

   op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].tmpref.buffer = object_id;
   op.params[0].tmpref.size = object_len;

   if( (result = TEEC_InvokeCommand(sess, TA_SK_CREATE_OBJECT, &op, &err_origin)) == TEEC_SUCCESS) {
      memcpy(&handle->tee_ses, sess, sizeof(TEEC_Session));
      handle->tee_ses_id = op.params[1].value.a;
   }
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_OpenObject
*
*  PURPOSE:    Open a crypto object (key) to perfrom crypto operations
*
*  PARAMETERS: sess       - [IN]  Pointer to the open session
*              object_id  - [IN]  ID to identify the crypto object (max 32 bytes)
*              object_len - [IN]  Length of the ID
*              handle     - [OUT] Handle to the crypto session 
*                                 with this crypto object
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_OpenObject(TEEC_Session *sess, void* object_id, uint8_t object_len, SK_SessionHandle *handle)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(sess && object_id && handle) || (object_len > SK_MAX_OBJ_LEN))
      return TEEC_ERROR_BAD_PARAMETERS;

   /* Initialize the input params */
   memset(&handle->tee_ses, 0, sizeof(TEEC_Session));
   handle->tee_ses_id = 0xFFFFFFFF;
   memset(&op, 0, sizeof(op));

   op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].tmpref.buffer = object_id;
   op.params[0].tmpref.size = object_len;

   if( (result = TEEC_InvokeCommand(sess, TA_SK_OPEN_OBJECT, &op, &err_origin)) == TEEC_SUCCESS){
      memcpy(&handle->tee_ses, sess, sizeof(TEEC_Session));
      handle->tee_ses_id = op.params[1].value.a;
   }
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_SetPrivKey
*
*  PURPOSE:    Import a private key to a crypto object
*
*  PARAMETERS:
*              handle  - [IN]  Handle to the crypto session
*                              with a crypto object
*              key     - [IN]  Private key to import
*              key_len - [IN]  Length of the private key
*              iv      - [IN]  Initialization Vector        (for symmetric key)
*              iv_len  - [IN]  Initialization Vector Length (for symmetric key)
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_SetPrivKey(SK_SessionHandle *handle, void* key, int key_len, void* iv, int iv_len)
{
   TEEC_Operation op;
   uint32_t err_origin;

   if (!(handle && key) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(op));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = key;
   op.params[1].tmpref.size = key_len;
   op.params[2].tmpref.buffer = iv;
   op.params[2].tmpref.size = iv_len;

   return TEEC_InvokeCommand(&handle->tee_ses, TA_SK_SET_PRIV_KEY, &op, &err_origin);
}

/*****************************************************************************
*  FUNCTION:   SK_SetPubKey
*
*  PURPOSE:    Import a public key to a crypto object
*
*  PARAMETERS:
*              handle  - [IN]  Handle to the crypto session
*                              with a crypto object
*              key     - [IN]  Public key to import
*              key_len - [IN]  Length of the public key
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_SetPubKey(SK_SessionHandle *handle, void* key, int key_len)
{
   TEEC_Operation op;
   uint32_t err_origin;

   if (!(handle && key) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(op));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = key;
   op.params[1].tmpref.size = key_len;

   return TEEC_InvokeCommand(&handle->tee_ses, TA_SK_SET_PUBL_KEY, &op, &err_origin);
}

/*****************************************************************************
*  FUNCTION:   SK_SetCertificate
*
*  PURPOSE:    Import a certificate to a crypto object
*
*  PARAMETERS:
*              handle  - [IN]  Handle to the crypto session
*                              with a crypto object
*              key     - [IN]  Certificate to import
*              key_len - [IN]  Length of the Certificate
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_SetCertificate(SK_SessionHandle *handle, void* cert, int cert_len)
{
   TEEC_Operation op;
   uint32_t err_origin;

   if (!(handle && cert) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(op));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = cert;
   op.params[1].tmpref.size = cert_len;

   return TEEC_InvokeCommand(&handle->tee_ses, TA_SK_SET_CERT, &op, &err_origin);
}

/*****************************************************************************
 * Helper function
 ******************************************************************************/
static TEEC_Result SetPublicParams(SK_SessionHandle *handle, void* buffer, int buffer_size, TEEC_Operation *op)
{
   if (!(handle && buffer) || (handle->tee_ses_id == 0xFFFFFFFF))
     return TEEC_ERROR_BAD_PARAMETERS;

   memset(op, 0, sizeof(TEEC_Operation));
   op->paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
                                     TEEC_VALUE_OUTPUT, TEEC_NONE);

   op->params[0].value.a = handle->tee_ses_id;
   op->params[1].tmpref.buffer = buffer;
   op->params[1].tmpref.size = buffer_size;

   return TEEC_SUCCESS;
}


/*****************************************************************************
*  FUNCTION:   SK_GetCertificate
*
*  PURPOSE:    Obtain a certificate from a crypto object
*
*  PARAMETERS:
*              handle      - [IN]   Handle to the crypto session
*                                   with a crypto object
*              buffer      - [OUT]  Buffer to hold the certificate
*              buffer_len  - [IN]   Length of the "buffer"
*              cert_len    - [OUT]  Actual length of the certificate
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_GetCertificate(SK_SessionHandle *handle, void* buffer, int buffer_len, int *cert_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   *cert_len = 0;

   if((result = SetPublicParams(handle, buffer, buffer_len, &op)) != TEEC_SUCCESS)
      return result;

   if ((result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_GET_CERT, &op, &err_origin)) == TEEC_SUCCESS)
     *cert_len = op.params[2].value.a;

   return result;
}


/*****************************************************************************
*  FUNCTION:   SK_GetPublicKey
*
*  PURPOSE:    Obtain a public key from a crypto object
*
*  PARAMETERS:
*              handle      - [IN]   Handle to the crypto session
*                                   with a crypto object
*              buffer      - [OUT]  Buffer to hold the public key
*              buffer_len  - [IN]   Length of the "buffer"
*              key_len     - [OUT]  Actual length of the public key
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_GetPublicKey(SK_SessionHandle *handle, void* buffer, int buffer_len, int *key_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   *key_len = 0;

   if((result = SetPublicParams(handle, buffer, buffer_len, &op)) != TEEC_SUCCESS)
     return result;

   if ((result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_GET_PUBL_KEY, &op, &err_origin)) == TEEC_SUCCESS)
     *key_len = op.params[2].value.a;

   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_GetPublicKey
*
*  PURPOSE:    Obtain size of the public content inside the crypto object
*
*  PARAMETERS:
*              handle      - [IN]   Handle to the crypto session
*                                   with a crypto object
*              obj_len     - [OUT]  Length of the public content in bytes
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_GetObjectLength(SK_SessionHandle *handle, int *obj_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   *obj_len = 0;

   if (!handle || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_OUTPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;

   if ((result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_GET_LENGTH, &op, &err_origin)) == TEEC_SUCCESS)
      *obj_len = op.params[1].value.a;

   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_CryptoInit
*
*  PURPOSE:    Initialize the crypto processing
*
*  PARAMETERS:
*              handle - [IN] Handle to the crypto session
*                            with a crypto object
*              algo   - [IN] Crypto algorithm
*              mode   - [IN] Crypto operation mode
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CryptoInit(SK_SessionHandle *handle, SK_CryptoAlgo algo, SK_OperationMode mode)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!handle || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].value.a = algo;
   op.params[1].value.b = mode;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CRYPTO_INIT, &op, &err_origin);
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_CryptoUpdate
*
*  PURPOSE:    Start/Continue the crypto processing
*
*  PARAMETERS:
*              handle - [IN]     Handle to the crypto session
*                                with a crypto object
*              input  - [IN]     Buffer to the input data
*              in_len - [IN]     Length of the "input" buffer
*              output - [OUT]    Buffer to the output data
*              out_len- [IN/OUT] Length of the "output" buffer
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CryptoUpdate(SK_SessionHandle *handle, uint8_t *input, uint32_t in_len, uint8_t *output, uint32_t *out_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = input;
   op.params[1].tmpref.size = in_len;
   op.params[2].tmpref.buffer = output;
   op.params[2].tmpref.size = *out_len;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CRYPTO_UPDATE, &op, &err_origin);
   *out_len = op.params[2].tmpref.size;
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_CryptoFinal
*
*  PURPOSE:    End the crypto processing
*
*  PARAMETERS:
*              handle - [IN]     Handle to the crypto session
*                                with a crypto object
*              input  - [IN]     Buffer to the input data
*              in_len - [IN]     Length of the "input" buffer
*              output - [OUT]    Buffer to the output data
*              out_len- [IN/OUT] Length of the "output" buffer
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CryptoFinal(SK_SessionHandle *handle, uint8_t *input, uint32_t in_len, uint8_t *output, uint32_t *out_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = input;
   op.params[1].tmpref.size = in_len;
   op.params[2].tmpref.buffer = output;
   op.params[2].tmpref.size = *out_len;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CRYPTO_FINAL, &op, &err_origin);
   *out_len = op.params[2].tmpref.size;
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_CryptoSign
*
*  PURPOSE:    Digitally sign the data using asymmetric key
*
*  PARAMETERS:
*              handle - [IN]     Handle to the crypto session
*                                with a crypto object
*              input  - [IN]     Buffer to the input data
*              in_len - [IN]     Length of the "input" buffer
*              output - [OUT]    Buffer to the signature
*              out_len- [IN/OUT] Length of the "signature" buffer
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CryptoSign(SK_SessionHandle *handle, uint8_t *input, uint32_t in_len, uint8_t *output, uint32_t *out_len)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_MEMREF_TEMP_INOUT, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = input;
   op.params[1].tmpref.size = in_len;
   op.params[2].tmpref.buffer = output;
   op.params[2].tmpref.size = *out_len;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CRYPTO_SIGN, &op, &err_origin);
   *out_len = op.params[2].tmpref.size;
   return result;
}


/*****************************************************************************
*  FUNCTION:   SK_CryptoVerify
*
*  PURPOSE:    Verify signature using asymmetric key
*
*  PARAMETERS:
*              handle   - [IN]     Handle to the crypto session
*                                  with a crypto object
*              input    - [IN]     Buffer to the input data
*              in_len   - [IN]     Length of the "input" buffer
*              sign     - [IN]     Buffer to the signature
*              sign_len - [IN]     Length of the "signature" buffer
*              match    - [OUT]    1: Sign matched, 0: Sign mismatched
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CryptoVerify(SK_SessionHandle *handle, uint8_t *input, uint32_t in_len, uint8_t *sign, uint32_t sign_len, uint32_t *match)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                    TEEC_MEMREF_TEMP_INPUT, TEEC_VALUE_OUTPUT);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].tmpref.buffer = input;
   op.params[1].tmpref.size = in_len;
   op.params[2].tmpref.buffer = sign;
   op.params[2].tmpref.size = sign_len;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CRYPTO_VERIFY, &op, &err_origin);
   *match = op.params[3].value.a;
   return result;
}


/*****************************************************************************
*  FUNCTION:   SK_GenerateKey
*
*  PURPOSE:    Generate a private key for symmetric crypto operation
*
*  PARAMETERS:
*              handle       - [IN] Handle to the crypto session
*                                  with a crypto object
*              key_len_bits - [IN] Length of ky in bits
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_GenerateKey(SK_SessionHandle *handle, uint32_t key_len_bits)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF) || (key_len_bits > 1024))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].value.a = key_len_bits;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_GENERATE_KEY, &op, &err_origin);
   return result;
}


/*****************************************************************************
*  FUNCTION:   SK_GenerateKeyPair
*
*  PURPOSE:    Generate a private/public key for asymmetric crypto operation
*
*  PARAMETERS:
*              handle       - [IN] Handle to the crypto session
*                                  with a crypto object
*              key_type     - [IN] Key generation type
*              key_len_bits - [IN] Length of ky in bits
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_GenerateKeyPair(SK_SessionHandle *handle, SK_CryptoKey key_type, uint32_t key_len_bits)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF) || (key_len_bits < 1024) || (key_type != SK_RSA_KEY))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;
   op.params[1].value.a = key_len_bits;
   op.params[1].value.b = key_type;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_GENERATE_KEY_PAIR, &op, &err_origin);
   return result;
}

/*****************************************************************************
*  FUNCTION:   SK_CloseObject
*
*  PURPOSE:    Close an alreday created/opened crypto object
*
*  PARAMETERS:
*              handle       - [IN] Handle to the crypto session
*                                  with a crypto object
*
*  RETURNS:    TEE_SUCCESS on success, error code otherwise
*
*****************************************************************************/
TEEC_Result SK_CloseObject(SK_SessionHandle *handle)
{
   TEEC_Operation op;
   uint32_t err_origin;
   TEEC_Result result = TEEC_ERROR_GENERIC;

   if (!(handle) || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(TEEC_Operation));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;

   result = TEEC_InvokeCommand(&handle->tee_ses, TA_SK_CLOSE_OBJECT, &op, &err_origin);
   return result;
}


TEEC_Result SK_DebugObject(SK_SessionHandle *handle)
{
   TEEC_Operation op;
   uint32_t err_origin;

   if (!handle || (handle->tee_ses_id == 0xFFFFFFFF))
      return TEEC_ERROR_BAD_PARAMETERS;

   memset(&op, 0, sizeof(op));
   op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
                                    TEEC_NONE, TEEC_NONE);

   op.params[0].value.a = handle->tee_ses_id;

   return TEEC_InvokeCommand(&handle->tee_ses, TA_SK_OBJ_DEBUG, &op, &err_origin);
}


