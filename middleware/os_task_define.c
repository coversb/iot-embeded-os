/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          os_task_define.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   define all task
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-18      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "os_task_define.h"
#include "pb_io_monitor_main.h"

/******************************************************************************
* Macros
******************************************************************************/
/*TASK INFO DEFINE begin*/
//pb_prot_main task
#define PB_PROT_STACK (OS_STACK_1K*4)   //bytes
#define PB_PROT_NAME "pb_prot"
#define PB_PROT_PRIO (tskIDLE_PRIORITY+3)
#define PB_PROT_MSGQUE_SIZE 15

//pb_input_monitor_main task
#define PB_IO_MONITOR_STACK (OS_STACK_1K*2)   //bytes
#define PB_IO_MONITOR_NAME "pb_input_monitor"
#define PB_IO_MONITOR_PRIO (tskIDLE_PRIORITY+2)
#define PB_IO_MONITOR_POLL_INTERVAL (DELAY_1_MS*10)
#define PB_IO_MONITOR_EVENT_INTERVAL DELAY_1_S

//monitor task
#define PB_MONITOR_STACK (OS_STACK_1K*2)   //bytes
#define PB_MONITOR_NAME "pb_monitor"
#define PB_MONITOR_PRIO (tskIDLE_PRIORITY)
/*TASK INFO DEFINE end*/
/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
/*
*task function, parameters, task name, stack size, priority, handler
*/
static OS_TASK_INFO_TYPE os_task_info[OS_TASK_ITEM_END] = 
{
    /*task main functino, parameter, task name, stack size, priority, task handler*/
    {
        pb_io_monitor_main, NULL, "pbIoMonitor", (OS_STACK_1K*2), (tskIDLE_PRIORITY+2), NULL
    }
};

/******************************************************************************
* Local Functions
******************************************************************************/
void os_task_create_all(void)
{
    for (uint8 idx = OS_TASK_ITEM_BEGIN; idx < OS_TASK_ITEM_END; ++idx)
    {
        os_task_create(os_task_info[idx].function,
                                os_task_info[idx].param,
                                os_task_info[idx].name,
                                os_task_info[idx].stackSize,
                                os_task_info[idx].priority,
                                os_task_info[idx].hdlr);
    }
}

