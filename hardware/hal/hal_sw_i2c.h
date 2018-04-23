/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          hal_sw_i2c.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   software i2c
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HAL_SW_I2C_H__
#define __HAL_SW_I2C_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "board_config.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    void (*init)(void);
    bool (*readByte)(uint8 devAddr, uint8 regAddr, uint8 *pdata);
    bool (*read)(uint8 devAddr, uint8 regAddr, uint8 *pdata, uint32 len);
    bool (*wirteByte)(uint8 devAddr, uint8 regAddr, uint8 data);
}HAL_SW_I2C_TYPE;

/******************************************************************************
* Extern variable
******************************************************************************/
extern const HAL_SW_I2C_TYPE swI2C1;

#endif /*__HAL_SW_I2C_H__*/

