/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_crypto.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   crypto
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "sha256.h"
#include "hmac_sha256.h"
#include "aes.h"
#include "pb_app_config.h"
#include "pb_crypto.h"
#include "pb_cfg_proc.h"

/******************************************************************************
* Macros
******************************************************************************/
#if (PB_PROT_AES == 1)
#define PB_ENCRYPTO_VERSION "ParkBox crypto 1.00"
#else
#define PB_ENCRYPTO_VERSION "ParkBox crypto 0.00"
#endif /*PB_PROT_AES*/

#define PB_ENCRYPT_KEY_LEN 32

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint8 pb_encrypt_key[PB_ENCRYPT_KEY_LEN];

/******************************************************************************
* Local Functions
******************************************************************************/

/******************************************************************************
* Function    : pb_encrypt
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : AES-256 encrypt
******************************************************************************/
uint16 pb_encrypt(void *inData, uint16 inLen, void *keyData, void *outData)
{
    #if (PB_PROT_AES == 1)
    return aes_encrypt(inData, inLen, keyData, outData);
    #else
    //implement later
    memcpy(outData, inData, inLen);
    return inLen;
    #endif /*PB_PROT_AES*/
}

/******************************************************************************
* Function    : pb_decrypt
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : AES-256 decrypt
******************************************************************************/
uint16 pb_decrypt(void *inData, uint16 inLen, void *keyData, void *outData)
{
    #if (PB_PROT_AES == 1)
    return aes_decrypt(inData, inLen, keyData, outData);
    #else
    //implement later
    memcpy(outData, inData, inLen);
    return inLen;
    #endif /*PB_PROT_AES*/
}

/******************************************************************************
* Function    : pb_get_crypto_version
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get encrypt version
******************************************************************************/
uint8* pb_crypto_get_version(void)
{
    return (uint8*)PB_ENCRYPTO_VERSION;
}

/******************************************************************************
* Function    : pb_encrypt_set_key
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : set encrypt key
******************************************************************************/
void pb_crypto_set_key(uint8 *key)
{
    uint8 uid[32];

    memset(uid, 0x00, sizeof(uid));
    memcpy(uid, pb_cfg_proc_get_uid(), PB_UID_LEN);

    memset(pb_encrypt_key, 0, sizeof(pb_encrypt_key));
    hmac_sha256(key, PB_ENCRYPT_KEY_LEN, uid, sizeof(uid),  pb_encrypt_key, SHA256_DIGEST_SIZE);
}

/******************************************************************************
* Function    : pb_encrypt_get_key
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get encrypt key
******************************************************************************/
uint8* pb_crypto_get_key(void)
{
    return pb_encrypt_key;
}

