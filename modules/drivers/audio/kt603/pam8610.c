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
/******************************************************************************
* Include Files
******************************************************************************/
#include "board_config.h"
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_trace_log.h"
#include "pam8610.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pam8610_switch
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : ctrl pam8610 power key
******************************************************************************/
static void pam8610_switch(bool sw)
{
    if (sw == true)
    {
        hal_gpio_set(BOARD_PA_PWR, HAL_GPIO_HIGH);
    }
    else
    {
        hal_gpio_set(BOARD_PA_PWR, HAL_GPIO_LOW);
    }
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "PAM8610 %s", (sw == true) ? "ON" : "OFF");
}

/******************************************************************************
* Function    : pam8610_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : init pam8610 power key
******************************************************************************/
static bool pam8610_init(void)
{
    hal_rcc_enable(BOARD_PA_IO_RCC);
    hal_gpio_set_mode(BOARD_PA_PWR, HAL_GPIO_OUT_PP);
    hal_gpio_set(BOARD_PA_PWR, HAL_GPIO_LOW);

    return true;
}

const DEV_TYPE_PA devPA = 
{
    pam8610_init,
    pam8610_switch
};

