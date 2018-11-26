/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_order_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   manage user order
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-06-01     1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_ORDER_MAIN_H__
#define __PB_ORDER_MAIN_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_ORDER_OPSTAT_UNKNOWN = 0,         //unknown
    PB_ORDER_OPSTAT_CLOSE,                     //close
    PB_ORDER_OPSTAT_CLOSE_WAIT,           //close in delay
    PB_ORDER_OPSTAT_SERVICE                   //in-service
}PB_ORDER_OPSTAT_TYPE;  //operation state

typedef enum
{
    PB_ORDER_VERIFY_SERVER = 0,
    PB_ORDER_VERIFY_KEYBOARD,
    PB_ORDER_VERIFY_SERVER_WITHOUT_AUDIO
}PB_ORDER_VERIFY_TYPE;

typedef enum
{
    PB_ORDER_VERIFY_PW_CANCELED = 0,
    PB_ORDER_VERIFY_PW_VALID,
    PB_ORDER_VERIFY_PW_OFFLINE,
    PB_ORDER_VERIFY_PW_ENG,
    PB_ORDER_VERIFY_SER_OPEN,
    PB_ORDER_VERIFY_SER_CLOSE,
    PB_ORDER_VERIFY_PW_CLEANER,
    PB_ORDER_VERIFY_PW_UNKNOWN = 0xFF
} PB_ORDER_VERIFY_RES;

typedef enum
{
    PB_ORDER_KEYBOARD_BEEP_REFUSE = 0,
    PB_ORDER_KEYBOARD_BEEP_PASS
} PB_ORDER_KEYBOARD_BEEP_TYPE;

typedef enum
{
    PB_ORDER_CONSUMER_UNKNOWN = 0
}PB_ORDER_CONSUMER_TYPE;

typedef enum
{
    PB_ORDER_OPERATION_UNKNOWN = 0
}PB_ORDER_OPERATION_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 type;
    uint32 data;
    uint32 operationID;
    uint32 consumerID;
}PB_ORDER_VERIFY_PARAM;

typedef struct
{
    PB_ORDER_OPSTAT_TYPE state;
    OS_MUTEX_TYPE orderMutex;
    OS_TMR_TYPE orderUpdateTmr;
    OS_TMR_TYPE orderOverDelayTmr;
}PB_ORDER_CONTEXT_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_order_main(void *params);

extern void pb_order_send_verify_req(uint8 type, uint32 data, uint32 operationID, uint32 consumerID);
extern void pb_order_booking(PB_PROT_ORDER_TYPE *pOrder);
extern void pb_order_cancel(PB_PROT_ORDER_TYPE *pOrder);
extern void pb_order_clear(void);
extern void pb_order_print(void);
extern uint16 pb_order_number(void);
extern uint32 pb_order_nearest_start_time(void);

extern PB_ORDER_OPSTAT_TYPE pb_order_operation_state(void);
extern void pb_order_control_bgm(void);

extern void pb_order_send_coe(uint8 coeInfoType, char *userInputData, uint32 operationID, uint32 consumerID);

#endif /* __PB_ORDER_MAIN_H__ */

