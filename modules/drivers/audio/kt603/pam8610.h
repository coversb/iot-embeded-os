/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pam8610.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   pam8610 operation
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-27      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PAM8610_H__
#define __PAM8610_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    bool (*init)(void);
    void (*sw)(bool sw);
}DEV_TYPE_PA;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const DEV_TYPE_PA devPA;

#endif /* __PAM8610_H__ */
