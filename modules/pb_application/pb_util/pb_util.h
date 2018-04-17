/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_util.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   utility functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_UTIL_H__
#define __PB_UTIL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "pb_app_message.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern uint16 pb_util_get_crc16(const uint8 *pdata, uint16 len);

#endif /* __PB_UTIL_H__ */

