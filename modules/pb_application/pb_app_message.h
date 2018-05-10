/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_app_message.h 
*  author:              Chen Hao
*  version:             1.00
*  file description:   message defination
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_APP_MESSAGE_H__
#define __PB_APP_MESSAGE_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macro
******************************************************************************/
#define PB_PROT_MSGQUE_SIZE 15
#define PB_OTA_MSGQUE_SIZE 10
#define PB_FOTA_MSGQUE_SIZE 10
#define PB_GUI_MSGQUE_SIZE 5
#define PB_MM_MSGQUE_SIZE 10

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_MESSAGE_NONE = 0,
    /*pb_prot_main msg begin*/
    PB_MSG_PROT_FIRMWARE_CONFIRM_REQ,
    PB_MSG_PROT_ANALYSE_DATA,
    PB_MSG_PROT_SEND_HBP,
    PB_MSG_PROT_SEND_RSP,
    /*pb_prot_main msg end*/
    /*pb_ota_main msg begin*/
    PB_MSG_OTA_NET_DEV_RESET,
    PB_MSG_OTA_NET_CONFIG,
    PB_MSG_OTA_NET_CONNECT,
    PB_MSG_OTA_NET_CLOSE,
    PB_MSG_OTA_NET_SEND,
    PB_MSG_OTA_NET_RECV,
    PB_MSG_OTA_CELL_LOCATION_REQ,
    PB_MSG_OTA_GSMINFO_REQ,
    /*pb_ota_main msg end*/
    /*pb_fota_main msg begin*/
    PB_MSG_FOTA_FIRMWARE_UPGRADE_REQ,
    PB_MSG_FOTA_START_UPGRADE_REQ,
    /*pb_fota_main msg end*/
    /*pb_io_main msg begine*/
    PB_MSG_IO_USR_ACCESS_CHECK,
    PB_MSG_IO_DOOR_SW_REQ,
    PB_MSG_IO_DEVICEBOX_SW_REQ,
    PB_MSG_IO_EQUIPMENT_SW_REQ,
    /*pb_io_main msg end*/
    /*pb_io_rgbbox msg begine*/
    PB_MSG_IO_RGBBOX_BLINK,
    /*pb_io_rgbbox msg end*/
    /*pb_order_main msg begine*/
    PB_MSG_ORDER_PASS_CHECK_REQ,
    PB_MSG_ORDER_OPERATION_STATE_CHECK_REQ,
    /*pb_order_main msg end*/
    /*pb_gui_main msg begine*/
    PB_MSG_GUI_MENU_ACTION_REQ,
    /*pb_gui_main msg end*/
    /*pb_multimedia msg begine*/
    PB_MSG_MM_AUDIO_CTRL_REQ,
    PB_MSG_MM_AUDIO_MONITOR_REQ,
    /*pb_multimedia msg end*/

    PB_MESSAGE_NUM    
}PB_QUEUE_MSG_ID_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint32 msgID;
    uint8 *pMsgData;
    uint32 msgData;
}PB_MSG_TYPE;

#endif /* __PB_APP_MESSAGE_H__ */

