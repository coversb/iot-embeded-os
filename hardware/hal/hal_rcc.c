/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_rcc.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   rcc hal
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rcc.h"
#include "os_trace_log.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : hal_bkp_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void hal_rcc_enable(uint32 rcc, uint32 bus)
{
    switch (bus)
    {
        case BOARD_RCC_AHB:
        {
            #if defined(BOARD_STM32F1XX)
            RCC_AHBPeriphClockCmd(rcc, ENABLE);
            #endif
            break;
        }
        case BOARD_RCC_AHB1:
        {
            #if defined(BOARD_STM32F4XX)
            RCC_AHB1PeriphClockCmd(rcc, ENABLE);
            #endif
            break;
        }
        case BOARD_RCC_AHB2:
        {
            #if defined(BOARD_STM32F4XX)
            RCC_AHB2PeriphClockCmd(rcc, ENABLE);
            #endif
            break;
        }
        case BOARD_RCC_APB1:
        {
            RCC_APB1PeriphClockCmd(rcc, ENABLE);
            break;
        }
        case BOARD_RCC_APB2:
        {
            RCC_APB2PeriphClockCmd(rcc, ENABLE);
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_HAL, "BAD RCC %d, %X", rcc, bus);
            break;
        }
    }
}

