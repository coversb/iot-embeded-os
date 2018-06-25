/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_gpio.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   gpio
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-12             1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__
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
typedef enum
{
    HAL_GPIO_LOW = 0,
    HAL_GPIO_HIGH = 1
}HAL_GPIO_VAL;

#if defined(BOARD_STM32F1XX)
typedef enum
{
    HAL_GPIO_AIN = GPIO_Mode_AIN,
    HAL_GPIO_IN_FLOATING = GPIO_Mode_IN_FLOATING,
    HAL_GPIO_IPD = GPIO_Mode_IPD,
    HAL_GPIO_IPU = GPIO_Mode_IPU,
    HAL_GPIO_OUT_OD = GPIO_Mode_Out_OD,
    HAL_GPIO_OUT_PP = GPIO_Mode_Out_PP,
    HAL_GPIO_AF_OD = GPIO_Mode_AF_OD,
    HAL_GPIO_AF_PP = GPIO_Mode_AF_PP
}HAL_GPIO_MODE;
#elif defined(BOARD_STM32F4XX)
typedef enum
{
    HAL_GPIO_AIN = 0x0,
    HAL_GPIO_IN_FLOATING = 0x04,
    HAL_GPIO_IPD = 0x28,
    HAL_GPIO_IPU = 0x48,
    HAL_GPIO_OUT_OD = 0x14,
    HAL_GPIO_OUT_OD_UP = 0x54,
    HAL_GPIO_OUT_PP = 0x10,
    HAL_GPIO_OUT_PP_UP = 0x11,
    HAL_GPIO_AF_OD = 0x1C,
    HAL_GPIO_AF_PP = 0x18,
    HAL_GPIO_AF_PP_UP = 0x58,
    HAL_GPIO_AF_PP_DOWN = 0x38
}HAL_GPIO_MODE;
#else
#error HAL_GPIO_MODE
#endif

/******************************************************************************
* Extern variable
******************************************************************************/
#if defined(BOARD_STM32F4XX)
extern void hal_gpio_af_config(GPIO_TypeDef* GPIOx, uint16 GPIO_PinSource, uint8 GPIO_AF);
#endif /*BOARD_STM32F4XX*/

extern void hal_gpio_set_mode(GPIO_TypeDef* io, uint16 pin, HAL_GPIO_MODE mode);
extern void hal_gpio_set(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin, HAL_GPIO_VAL val);
extern HAL_GPIO_VAL hal_gpio_val(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin);

#endif /*__HAL_GPIO_H__*/

