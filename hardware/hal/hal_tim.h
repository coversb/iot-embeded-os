/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          hal_tim.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   hal tim
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-24      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HAL_TIM_H__
#define __HAL_TIM_H__
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
typedef void (*TIM_CALLBACK)(void);
typedef struct
{
    void (*init)(void);
    void (*deinit)(void);
    void (*setCallback)(TIM_CALLBACK callback);
    uint32 (*micros)(void);
    void (*delay)(uint32 us);
}HAL_TIM_TYPE;

typedef struct
{
    void (*init)(void);
    void (*deinit)(void);
    void (*enable)(void);
    void (*disable)(void);
    void (*generateEvent)(void);
}HAL_TIM_PWM_TYPE;

/******************************************************************************
* Extern variable
******************************************************************************/
#if (BOARD_TIM1_PWM_ENABLE == 1)
extern const HAL_TIM_PWM_TYPE tim1Pwm;
#endif /*BOARD_TIM1_PWM_ENABLE*/

#if (BOARD_TIM2_ENABLE == 1)
extern const HAL_TIM_TYPE tim2;
#endif /*BOARD_TIM2_ENABLE*/

#if (BOARD_TIM3_PWM_ENABLE == 1)
extern const HAL_TIM_PWM_TYPE tim3Pwm;
#endif /*BOARD_TIM3_PWM_ENABLE*/

#endif /*__HAL_TIM_H__*/

