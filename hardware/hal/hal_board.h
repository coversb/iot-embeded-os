/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_board.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   board init support
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-12      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HAL_BOARD_H__
#define __HAL_BOARD_H__
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
    HAL_PWRON_NORMAL = 0,
    HAL_PWRON_WDG,
    HAL_PWRON_KEY,
    HAL_PWRON_SOFTRST
}HAL_PWRON_STATUS;

/******************************************************************************
* Extern variable
******************************************************************************/
extern void hal_board_init(void);
extern void hal_board_nvic_set_irq(uint8 IRQChannel, uint8 PreemptionPriority, uint8 SubPriority, FunctionalState Cmd);
extern uint8 hal_board_get_boot_type(void);
extern void hal_board_reset(void);

#endif /*__PB_DRV_BOARD_H__*/

