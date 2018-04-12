/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_app_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   parkbox app main
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-11      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "hal_board.h"
#include "os_middleware.h"
#include "os_trace_log.h"

/******************************************************************************
* Macros
******************************************************************************/
#define DEBUG_COM hwSerial1

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/

/******************************************************************************
* Function    : hardware_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void hardware_init()
{
    hal_board_init();

    DEBUG_COM.begin(115200);
}

/******************************************************************************
* Function    : pb_monitor_task
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_monitor_task(void *pvParameters)
{
    while (1)
    {
        OS_INFO("pb_monitor_task");
        
        os_scheduler_delay(DELAY_1_S);
    }
}

/******************************************************************************
* Function    : main
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
int main(void)
{
    hardware_init();

    OS_TASK_TYPE taskHdlr;
    os_task_create(pb_monitor_task, NULL, "PB_MONITOR", 2048, 0, &taskHdlr);

    os_task_scheduler();

    return 0;
}
