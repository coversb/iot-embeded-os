/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_gpio.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   gpio
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-12      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_gpio.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
#if defined(BOARD_STM32F4XX)
/******************************************************************************
* Function    : hal_gpio_af_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void hal_gpio_af_config(GPIO_TypeDef* GPIOx, uint16 GPIO_PinSource, uint8 GPIO_AF)
{
    GPIO_PinAFConfig(GPIOx, GPIO_PinSource, GPIO_AF);
}
#endif /*BOARD_STM32F4XX*/

/******************************************************************************
* Function    : hal_gpio_set_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set gpio pin mode
******************************************************************************/
void hal_gpio_set_mode(GPIO_TypeDef* io, uint16 pin, HAL_GPIO_MODE mode)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = pin;

    #if defined(BOARD_STM32F1XX)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = (GPIOMode_TypeDef)mode;
    #elif defined(BOARD_STM32F4XX)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    switch(mode & 0xff)
    {
        case HAL_GPIO_AIN:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_IN_FLOATING:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_IPD:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
            break;
        }
        case HAL_GPIO_IPU:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
            break;
        }
        case HAL_GPIO_OUT_OD:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_OUT_OD_UP:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
            break;
        }
        case HAL_GPIO_OUT_PP:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_OUT_PP_UP:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
            break;
        }
        case HAL_GPIO_AF_OD:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_AF_PP:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
        case HAL_GPIO_AF_PP_UP:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
            break;
        }
        case HAL_GPIO_AF_PP_DOWN:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
            GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
            break;
        }
        default:
        {
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
            break;
        }
    }
    #else
    #error hal_gpio_set_mode
    #endif
    
    GPIO_Init(io, &GPIO_InitStructure);
}

/******************************************************************************
* Function    : hal_drv_gpio_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set gpio pin value
******************************************************************************/
void hal_gpio_set(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin, HAL_GPIO_VAL val)
{
    if (val == HAL_GPIO_LOW)
    {
        GPIO_ResetBits(GPIOx, GPIO_Pin);
    }
    else
    if (val == HAL_GPIO_HIGH)
    {
        GPIO_SetBits(GPIOx, GPIO_Pin);
    }
}

/******************************************************************************
* Function    : hal_gpio_val
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get gpio pin value
******************************************************************************/
HAL_GPIO_VAL hal_gpio_val(GPIO_TypeDef* GPIOx, uint16 GPIO_Pin)
{
    return (HAL_GPIO_VAL)GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
}

