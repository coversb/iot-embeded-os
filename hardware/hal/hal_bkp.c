/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_bkp.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   backup register
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_bkp.h"

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
void hal_bkp_init(void)
{
    #if ( BOARD_BKP_ENABLE == 1 )
    /* Enable PWR and BKP clocks */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    #endif /*BOARD_BKP_ENABLE*/
}

/******************************************************************************
* Function    : hal_bkp_read
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 hal_bkp_read(uint32 addr)
{
    uint32 data = 0;

    #if ( BOARD_BKP_ENABLE == 1 )
    PWR_BackupAccessCmd(ENABLE);
    data = BKP_ReadBackupRegister((uint16)addr);
    PWR_BackupAccessCmd(DISABLE);
    #endif /*BOARD_BKP_ENABLE*/

    return data;
}

/******************************************************************************
* Function    : hal_bkp_write
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void hal_bkp_write(uint32 addr, uint32 data)
{
    #if ( BOARD_BKP_ENABLE == 1 )
    PWR_BackupAccessCmd(ENABLE);
    BKP_WriteBackupRegister((uint16)addr, (uint16)data);
    PWR_BackupAccessCmd(DISABLE);
    #endif /*BOARD_BKP_ENABLE*/
}

