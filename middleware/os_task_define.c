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
#include "os_trace_log.h"
#include "pb_prot_main.h"
#include "pb_ota_main.h"
#include "pb_fota.h"
#include "pb_io_monitor_main.h"
#include "pb_gui_main.h"
#include "pb_multimedia.h"
#include "pb_function_polling.h"
#include "rgb_led_task.h"
#include "pb_io_main.h"
#include "pb_order_main.h"
#if (PB_BLE_ENABLE == 1)
#include "pb_ble_main.h"
#endif /*PB_BLE_ENABLE*/

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
/*
*task function, parameters, task name, stack size, priority, handler
*/
static OS_TASK_INFO_TYPE os_task_info[OS_TASK_ITEM_END] = 
{
    /*task main functino,         parameter, task name,        stack size,         priority,      task handler*/
    {
        pb_prot_main,                NULL,    "pbPROT",      (OS_STACK_1K*2), (tskIDLE_PRIORITY+5), NULL
    },
    {
        pb_order_main,               NULL,    "pbOrder",      (OS_STACK_1K*2), (tskIDLE_PRIORITY+5), NULL
    },
    {
        pb_ota_main,                 NULL,    "pbOTA",       (OS_STACK_1K*2), (tskIDLE_PRIORITY+4), NULL
    },
    {
        pb_fota_main,                NULL,    "pbFOTA",      (OS_STACK_1K*4), (tskIDLE_PRIORITY+3), NULL
    },
    {
        pb_io_main,                  NULL,    "pbIO",        (OS_STACK_1K*2), (tskIDLE_PRIORITY+4), NULL
    },
    {
        pb_io_monitor_main,          NULL,    "pbIOMonitor", (OS_STACK_1K*2), (tskIDLE_PRIORITY+2), NULL
    },
    #if (PB_BLE_ENABLE == 1)
    {
        pb_ble_main,          NULL,    "pbBLE", (OS_STACK_1K*6), (tskIDLE_PRIORITY+2), NULL
    },
    #endif /*PB_BLE_ENABLE*/
    {
        pb_gui_main,                 NULL,    "pbGUI",       (OS_STACK_1K),   (tskIDLE_PRIORITY+1), NULL
    },
    {
        pb_multimedia_main,          NULL,    "pbMM",        (OS_STACK_1K), (tskIDLE_PRIORITY+2), NULL
    },
    {
        pb_function_polling_main,    NULL,    "pbFPOLL",     (OS_STACK_1K*2), (tskIDLE_PRIORITY),   NULL
    },
    {
        rgbled_task,                 NULL,    "rgbTASK",     (OS_STACK_1K),   (tskIDLE_PRIORITY+1),  NULL
    }
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : os_task_create_all
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : create all task defined in os_task_info
******************************************************************************/
void os_task_create_all(void)
{
    for (uint8 idx = OS_TASK_ITEM_BEGIN; idx < OS_TASK_ITEM_END; ++idx)
    {
        if (os_task_info[idx].function != NULL)
        {
            os_task_create(os_task_info[idx].function,
                                    os_task_info[idx].param,
                                    os_task_info[idx].name,
                                    os_task_info[idx].stackSize,
                                    os_task_info[idx].priority,
                                    &os_task_info[idx].hdlr);
        }
    }
}

/******************************************************************************
* Function    : os_task_print_free_stack
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void os_task_print_free_stack(void)
{
    for (uint8 idx = OS_TASK_ITEM_BEGIN; idx < OS_TASK_ITEM_END; ++idx)
    {
        uint32 freeStack;
        freeStack = os_task_free_stack(os_task_info[idx].hdlr);
        OS_INFO("%s FREE[%d]", os_task_info[idx].name, freeStack*4);
    }
}

/******************************************************************************
* Function    : os_task_print_free_heap
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void os_task_print_free_heap(void)
{
    OS_INFO("SYS HEAP FREE[%d]", os_sys_free_heap());
}

