/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          os_task_define.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   RTOS task define type
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-18      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __OS_TASK_DEFINE_H__
#define __OS_TASK_DEFINE_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "os_middleware.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_MSGQUE_SIZE 15

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    OS_TASK_ITEM_BEGIN = 0,
    OS_TASK_ITEM_PB_PROT = OS_TASK_ITEM_BEGIN,
    OS_TASK_ITEM_PB_OTA,
    OS_TASK_ITEM_PB_IOMONITOR,
    OS_TASK_ITEM_END
}OS_TASK_ITEM;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    void (*function)(void *);
    void * const param;
    const char * const name;
    uint32 stackSize;
    uint32 priority;
    void * hdlr;
}OS_TASK_INFO_TYPE;

/******************************************************************************
* Global Functions
******************************************************************************/
extern void os_task_create_all(void);
extern void os_task_print_free_stack(void);
extern void os_task_print_free_heap(void);

#endif /* __OS_TASK_DEFINE_H__ */
