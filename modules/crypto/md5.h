/******************************************************************************
*        
*     Copyright (c) 2017 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_md5.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   MD5 algorithm
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2017-9-22             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __MD5_H__
#define __MD5_H__

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
/* Any 32-bit or wider unsigned integer data type will do */
typedef unsigned int MD5_u32plus;

typedef struct 
{   
    MD5_u32plus lo, hi; 
    MD5_u32plus a, b, c, d; 
    unsigned char buffer[64];  
    MD5_u32plus block[16];
}MD5_CTX;

/******************************************************************************
* Extern functions
******************************************************************************/
extern void md5(const uint8 *message, uint32 len, uint8 *output);

#endif /*__MD5_H__*/

