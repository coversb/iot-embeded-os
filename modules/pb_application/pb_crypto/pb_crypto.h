/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_crypto.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   crypto
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_CRYPTO_H__
#define __PB_CRYPTO_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Enums
******************************************************************************/
    
/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern uint8* pb_crypto_get_version(void);
extern void pb_crypto_set_key(uint8 *key);
extern uint8* pb_crypto_get_key(void);
extern uint16 pb_encrypt(void *inData, uint16 inLen, void *keyData, void *outData);
extern uint16 pb_decrypt(void *inData, uint16 inLen, void *keyData, void *outData);

#endif /* __PB_ENCRYPT_H__ */
