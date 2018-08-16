/******************************************************************************
*
*     Copyright (c) 2017 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_io_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   control gpio
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2017-9-19             1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_util.h"
#include "pb_order_main.h"
#include "pb_prot_type.h"
#include "pb_io_main.h"
#include "pb_cfg_proc.h"
#include "pb_multimedia.h"
#include "rgb_led_task.h"
#include "pb_prot_main.h"
#include "pb_order_hotp.h"
#if (PB_ORDER_CONTAINER_LIST == 1)
#include "pb_order_list.h"
#elif (PB_ORDER_CONTAINER_DB == 1)
#include "pb_order_db.h"
#else
#error Need order container
#endif

/******************************************************************************
* Macros
******************************************************************************/
#define PB_ORDER_UPDATE_INTERVAL (1*SEC2MSEC)
#define PB_ORDER_OVER_DELAY (5*MIN2MSEC)

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
const char *PB_COE_INFO_TYPE[PB_COE_INFO_END] = 
{
    "Unknown",
    "EngPwd",
    "OfflinePwd",
    "Pwd",
    "ServerOpen",
    "EngServerOpen"
};

static OS_MSG_QUEUE_TYPE pb_order_msg_queue;
static PB_ORDER_CONTEXT_TYPE pb_order_context;

static void pb_order_operation_state_update(void);
/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_order_booking
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_booking(PB_PROT_ORDER_TYPE *pOrder)
{
    os_mutex_lock(&pb_order_context.orderMutex);
    
    //advance 5 minutes
    pOrder->startTime -= PB_ORDER_START_ADJUST;

    PB_ORDER.add(pOrder);

    //check operate state
    pb_order_operation_state_update();

    os_mutex_unlock(&pb_order_context.orderMutex);
}

/******************************************************************************
* Function    : pb_order_cancel
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_cancel(PB_PROT_ORDER_TYPE *pOrder)
{
    os_mutex_lock(&pb_order_context.orderMutex);

    PB_ORDER.remove(pOrder);

    os_mutex_unlock(&pb_order_context.orderMutex);
}

/******************************************************************************
* Function    : pb_order_clear
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_clear(void)
{
    PB_ORDER.clear();
}

/******************************************************************************
* Function    : pb_order_print
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_print(void)
{
    PB_ORDER.print();
}

/******************************************************************************
* Function    : pb_order_number
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_order_number(void)
{
    return PB_ORDER.size();
}

/******************************************************************************
* Function    : pb_order_nearest_start_time
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 pb_order_nearest_start_time(void)
{
    return PB_ORDER.header()->startTime;
}

/******************************************************************************
* Function    : pb_order_operation_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
PB_ORDER_OPSTAT_TYPE pb_order_operation_state(void)
{
    return pb_order_context.state;
}

/******************************************************************************
* Function    : pb_order_control_bgm
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_control_bgm(void)
{
    if (pb_cfg_proc_get_cmd()->muo.autoBGM == PB_MUO_ACT_AUTO_BGM_ON)
    {
        if (pb_order_operation_state() == PB_ORDER_OPSTAT_SERVICE)
        {
            pb_multimedia_send_audio_msg(PB_MM_PLAY_BGM, true);
        }
        else
        {
            pb_multimedia_send_audio_msg(PB_MM_STOP, 0);
        }
    }
}

/******************************************************************************
* Function    : pb_order_operation_need_enter_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_operation_need_enter_close(void)
{
    return (bool)(0 == PB_ORDER.validNumByTime(pb_util_get_timestamp()));
}

/******************************************************************************
* Function    : pb_order_operation_need_enter_service
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_operation_need_enter_service(void)
{
    return (bool)(0 != PB_ORDER.validNumByTime(pb_util_get_timestamp()));
}

/******************************************************************************
* Function    : pb_order_operation_enter_close_wait
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_operation_enter_close_wait(void)
{
    pb_order_context.state = PB_ORDER_OPSTAT_CLOSE_WAIT;

    //start over delay timer
    os_tmr_start(pb_order_context.orderOverDelayTmr);
    //stop all media
    pb_multimedia_send_audio_msg(PB_MM_STOP, 0);
    //play over media
    pb_multimedia_send_audio_msg(PB_MM_PLAY_ORDER_OVER, 0);

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Enter CLOSE_WAIT");
}

/******************************************************************************
* Function    : pb_order_operation_enter_in_service
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_operation_enter_in_service(void)
{
    pb_order_context.state = PB_ORDER_OPSTAT_SERVICE;
    
    //stop over delay timer
    os_tmr_stop(pb_order_context.orderOverDelayTmr);
    //control bgm
    pb_order_control_bgm();

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Enter IN-SERVICE");
}

/******************************************************************************
* Function    : pb_order_operation_state_update
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_order_operation_state_update(void)
{
    switch (pb_order_context.state)
    {
        case PB_ORDER_OPSTAT_SERVICE:
        {
            //current service is in-service, and valid order num is 0, need enter close_wait
            if (pb_order_operation_need_enter_close())
            {
                pb_order_operation_enter_close_wait();
            }
            break;
        }
        case PB_ORDER_OPSTAT_CLOSE_WAIT:
        case PB_ORDER_OPSTAT_CLOSE:
        default:
        {
            //current service is not in-service, and valid order num is not 0, need enter in-service
            if (pb_order_operation_need_enter_service())
            {
                pb_order_operation_enter_in_service();
            }
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_order_verify_local
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check the pass's validity
******************************************************************************/
static uint8 pb_order_verify_keyboard(uint32 password, uint32 *orderID)
{
    uint8 passwordType = PB_ORDER.verifyPassword(pb_util_get_timestamp(), password, orderID);

    switch (passwordType)
    {
        case PB_ORDER_VERIFY_PW_CANCELED:
        case PB_ORDER_VERIFY_PW_VALID:
        {
            break;
        }
        case PB_ORDER_VERIFY_PW_UNKNOWN:
        {
            passwordType = pb_order_hotp_verify_password(password, orderID);
            break;
        }
        default:break;
    }

    return passwordType;
}

/******************************************************************************
* Function    : pb_order_verify_server
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_order_verify_server(uint32 data)
{
    if (data == 1)
    {
        return PB_ORDER_VERIFY_SER_OPEN;
    }
    else
    {
        return PB_ORDER_VERIFY_SER_CLOSE;
    }
}

/******************************************************************************
* Function    : pb_order_keyboard_beep
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_keyboard_beep(PB_ORDER_KEYBOARD_BEEP_TYPE beep)
{
    switch (beep)
    {
        case PB_ORDER_KEYBOARD_BEEP_PASS:
        {
            PB_KEYBOARD.write(0x02);
            PB_KEYBOARD.write(0x33);
            break;
        }
        case PB_ORDER_KEYBOARD_BEEP_REFUSE:
        {
            PB_KEYBOARD.write(0x02);
            PB_KEYBOARD.write(0x31);
            break;
        }
        default:break;
    }
}

/******************************************************************************
* Function    : pb_order_pass_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : 
******************************************************************************/
static void pb_order_verify(PB_MSG_TYPE *pMsg)
{
    PB_ORDER_VERIFY_PARAM *pParam = (PB_ORDER_VERIFY_PARAM *)pMsg->pMsgData;
    uint8 verifyRes = PB_ORDER_VERIFY_PW_UNKNOWN;
    
    switch (pParam->type)
    {
        case PB_ORDER_VERIFY_SERVER:
        {
            verifyRes = pb_order_verify_server(pParam->data);
            break;
        }
        case PB_ORDER_VERIFY_KEYBOARD:
        {
            verifyRes = pb_order_verify_keyboard(pParam->data, &(pParam->consumerID));
            break;
        }
        default:return;
    }

    bool needOpenDevBox = false;
    bool needOpenDoor = false;
    bool needCloseDoor = false;
    bool needPlayWelcome = false;
    bool needBlinkPass = false;
    bool needBlinkRefuse = false;
    uint8 doorOperateType = PB_PROT_DSE_UNKNOW;

    bool needSendUIE = false;
    uint8 userInputType = PB_PROT_UIE_UNKNOWN;
    char userInputData[PB_UIE_DATA_LEN] = {0};
    uint8 coeInfoType = PB_COE_INFO_UNKNOWN;

    //check verify result
    switch (verifyRes)
    {
        case PB_ORDER_VERIFY_PW_ENG:
        {
            needOpenDevBox = true;

            needOpenDoor = true;
            doorOperateType = PB_PROT_DSE_PASSWD;
            
            needPlayWelcome = true;
            needBlinkPass = true;

            needSendUIE = true;
            userInputType = PB_PROT_UIE_ENG_PASS;
            sprintf(userInputData, "%08d", pParam->data);

            //eng password
            coeInfoType = PB_COE_INFO_ENG_PWD;
            break;
        }
        case PB_ORDER_VERIFY_PW_OFFLINE:
        {
            needOpenDoor = true;
            doorOperateType = PB_PROT_DSE_PASSWD;

            needPlayWelcome = true;
            needBlinkPass = true;

            needSendUIE = true;
            userInputType = PB_PROT_UIE_HOTP_PASS;
            sprintf(userInputData, "%04d", pParam->data);

            // offline consumer password
            coeInfoType = PB_COE_INFO_OFFLINE_PWD;
            break;
        }
        case PB_ORDER_VERIFY_PW_VALID:
        {
            needOpenDoor = true;
            doorOperateType = PB_PROT_DSE_PASSWD;

            needPlayWelcome = true;
            needBlinkPass = true;

            needSendUIE = true;
            userInputType = PB_PROT_UIE_KEYBOARD;
            sprintf(userInputData, "%04d", pParam->data);

            // consumer password
            coeInfoType = PB_COE_INFO_PWD;
            break;
        }
        case PB_ORDER_VERIFY_SER_OPEN:
        {
            needOpenDoor = true;
            doorOperateType = PB_PROT_DSE_SERVER;

            if (pb_order_operation_state() != PB_ORDER_OPSTAT_CLOSE)
            {
                needPlayWelcome = true;
                // consumer server open
                coeInfoType = PB_COE_INFO_SERVER_OPEN;
            }
            else
            {
                // eng server open
                coeInfoType = PB_COE_INFO_ENG_SERVER_OPEN;
            }
            needBlinkPass = true;
            break;
        }
        case PB_ORDER_VERIFY_SER_CLOSE:
        {
            needCloseDoor = true;
            doorOperateType = PB_PROT_DSE_SERVER;
            break;
        }
        default:
        case PB_ORDER_VERIFY_PW_CANCELED:
        {
            doorOperateType = PB_PROT_DSE_PASSWD;
            needBlinkRefuse = true;

            needSendUIE = true;
            userInputType = PB_PROT_UIE_CANCELED_PASS;
            sprintf(userInputData, "%04d", pParam->data);
            break;
        }
        case PB_ORDER_VERIFY_PW_UNKNOWN:
        {
            doorOperateType = PB_PROT_DSE_PASSWD;
            needBlinkRefuse = true;

            needSendUIE = true;
            userInputType = PB_PROT_UIE_KEYBOARD;
            sprintf(userInputData, "%04d", pParam->data);
            break;
        }
    }

    //open device box lock
    if (needOpenDevBox)
    {
        pb_io_dev_lock_sw(PB_IO_DEVBOX_OPEN);
    }
    //open door
    if (needOpenDoor)
    {
        pb_io_door_lock_sw(PB_IO_DOOR_OPEN, doorOperateType);
    }
    //close door
    if (needCloseDoor)
    {
        pb_io_door_lock_sw(PB_IO_DOOR_CLOSE, doorOperateType);
    }
    //play welcome media
    if (needPlayWelcome)
    {
        pb_multimedia_send_audio_msg(PB_MM_PLAY_WELCOME, 0);
    }
    //blink green RGB and beep pass
    if (needBlinkPass)
    {
        rgbled_send_act_req(RGBLED_BLINK_GREEN);
        pb_order_keyboard_beep(PB_ORDER_KEYBOARD_BEEP_PASS);
    }
    //blink red RGB and beep refuse
    if (needBlinkRefuse)
    {
        rgbled_send_act_req(RGBLED_BLINK_RED);
        pb_order_keyboard_beep(PB_ORDER_KEYBOARD_BEEP_REFUSE);
    }
    //send user input event RSP
    if (needSendUIE)
    {
        pb_prot_send_uie_req(userInputType, (uint8*)userInputData);
    }

    if (coeInfoType != PB_COE_INFO_UNKNOWN)
    {
        char coeInfo[PB_COE_INFO_LEN] = {0};
        snprintf(coeInfo, PB_COE_INFO_LEN, 
                    "{"
                    "\"type\":\"%s\","
                    "\"pwd\":\"%s\","
                    "\"oid\":\"%d\","
                    "\"sn\":\"%d\""
                    "}",
                    PB_COE_INFO_TYPE[coeInfoType],
                    userInputData,
                    pParam->consumerID,
                    pParam->operationID);
        pb_prot_send_coe_req(PB_COE_OPEN_DOOR, 
                                            pParam->operationID,
                                            pParam->consumerID,
                                            (uint8*)coeInfo);
        OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "COE:%s", coeInfo);
    }
}

/******************************************************************************
* Function    : pb_order_over_delay_tmr_hdlr
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_order_over_delay_tmr_hdlr(OS_TMR_TYPE tmr)
{
    pb_order_context.state = PB_ORDER_OPSTAT_CLOSE;
    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Enter CLOSE");
}

/******************************************************************************
* Function    : pb_order_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_update(void)
{
    if (!os_mutex_try_lock(&pb_order_context.orderMutex, 0))
    {
        OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "***order check wait updating***");
        return;
    }
    //update expire order
    PB_ORDER.update();
    
    os_mutex_unlock(&pb_order_context.orderMutex);
}

/******************************************************************************
* Function    : pb_order_update_tmr_hdlr
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_order_update_tmr_hdlr(OS_TMR_TYPE tmr)
{
    pb_order_update();
    
    //check operate state
    pb_order_operation_state_update();

    //update offline password by timestamp
    pb_order_hotp_update();
    pb_order_hotp_try_to_send_buffer_order();
}

/******************************************************************************
* Function    : pb_order_send_msg_to_order_mod
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_order_send_verify_req(uint8 type, uint32 data, uint32 operationID, uint32 consumerID)
{
    PB_MSG_TYPE msg;
    msg.msgID = PB_MSG_ORDER_VERIFY_REQ;
    msg.pMsgData = (uint8 *)os_malloc(sizeof(PB_ORDER_VERIFY_PARAM));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "order verify malloc error");
        return;
    }

    PB_ORDER_VERIFY_PARAM *param = (PB_ORDER_VERIFY_PARAM *)msg.pMsgData;
    param->type = type;
    param->data = data;
    param->operationID = operationID;
    param->consumerID = consumerID;

    os_msg_queue_send(pb_order_msg_queue, ( void*)&msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Send verify req");
}

/******************************************************************************
* Function    : pb_order_main_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_main_init(void)
{
    PB_ORDER.init();
    pb_order_msg_queue = os_msg_queue_create(PB_ORDER_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));

    pb_order_context.state = PB_ORDER_OPSTAT_CLOSE;
    os_mutex_lock_init(&pb_order_context.orderMutex);
    pb_order_context.orderUpdateTmr= os_tmr_create(PB_ORDER_UPDATE_INTERVAL, pb_order_update_tmr_hdlr, true);
    pb_order_context.orderOverDelayTmr = os_tmr_create(PB_ORDER_OVER_DELAY, pb_order_over_delay_tmr_hdlr, false);

    pb_order_hotp_init();
}

/******************************************************************************
* Function    : pb_order_main
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : manage order
******************************************************************************/
void pb_order_main(void *pvParameters)
{
    pb_order_main_init();

    os_set_task_init(OS_TASK_ITEM_PB_ORDER);
    os_wait_task_init_sync();

    //start order check
    os_tmr_start(pb_order_context.orderUpdateTmr);

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_order_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Recv msg %d", p_pb_msg->msgID);
            switch (pb_msg.msgID)
            {
                case PB_MSG_ORDER_VERIFY_REQ:
                {
                    pb_order_verify(&pb_msg);
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_PBORDER, "Unknow msg %d", pb_msg.msgID);
                    break;
                }
            }

            if (pb_msg.pMsgData != NULL)
            {
                os_free(pb_msg.pMsgData);
            }
        }
    }
}

