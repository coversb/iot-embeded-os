/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          ir_remote_control.h
*  author:              Chen hao
*  version:             1.00
*  file description:   ir driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-24      1.00                    Chen hao
*
******************************************************************************/
#ifndef __IR_REMOTE_CONTROL_H__
#define __IR_REMOTE_CONTROL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define IR_KEY_LENGTH_MAX  4

/******************************************************************************
* Enums
******************************************************************************/

typedef enum
{
    IR_RC5 = 0,
    IR_RC6,
    IR_NEC,
    IR_NUM,
}IR_PROTOCOL_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    IR_PROTOCOL_TYPE protocol;
    uint16 keyData[IR_KEY_LENGTH_MAX];
}IR_KEY_TYPE;

typedef void (*IR_USDELAY)(uint32 delay);

typedef struct
{
    bool (*init)(const HAL_TIM_PWM_TYPE *pwm);
    void (*deinit)(void);
    void (*setDelay)(IR_USDELAY delay);
    void (*transmit)(IR_KEY_TYPE *key);
}DEV_TYPE_IR_CONTROL;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const DEV_TYPE_IR_CONTROL devIRCtrl;

#endif /* __PB_DEV_IR_H__ */

