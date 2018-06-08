/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_gui_main.c
*  author:
*  version:             1.00
*  file description:   support display something on lcd
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_app_message.h"
#include "pb_gui_main.h"
#include "sh1106.h"
#include "pb_util.h"
#include "pb_cfg_proc.h"
#include "pb_ota_main.h"
#include "pb_multimedia.h"
#include "pb_io_main.h"
#include "pb_order_main.h"

/******************************************************************************
* Macro
******************************************************************************/
#define DEFAULT_COL 0
/*Version page*/
#define VERSION_ROW 0
#define DATETIME_ROW 1
#define SN_ROW 2
#define UID_ROW 3
/*Info page*/
#define ORDER_INFO_ROW 0
#define NET_INFO_ROW 1
#define INPUT_STAT_ROW 2
#define OUTPUT_STAT_ROW 3
/*Volume page*/
#define TITLE_ROW 1
#define VOLUME_ROW 2
/*Upgrade page*/
#define UP_TITLE_ROW 0
#define UP_STAGE_ROW 1
#define UP_PROCESS_ROW 2
#define UP_DATE_ROW 3

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MSG_QUEUE_TYPE pb_gui_msg_queue;
static PB_GUI_CONTEXT_TYPE pb_gui_context;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_gui_show_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_show_version(bool update)
{
    if (!update)
    {
        devSH1106.clear();
    }
    
    char temp[32];
    
    //firmware version
    memset(temp, 0x00, sizeof(temp));
    uint16 fmVer = PB_FIRMWARE_VERSION;
    sprintf(temp, "ParkBox V%d.%02d.%02d", (fmVer >> 12), ((fmVer >> 4) & 0xFF), (fmVer & 0x000F));
    devSH1106.show(DEFAULT_COL, VERSION_ROW, temp);

    //SYS date time
    pb_util_get_datetime(temp, sizeof(temp));
    devSH1106.show(DEFAULT_COL, DATETIME_ROW, temp);

    //sn
    memset(temp, 0x00, sizeof(temp));
    memcpy(temp, pb_cfg_proc_get_sn(), PB_SN_LEN);
    temp[PB_SN_LEN] = '\0';
    devSH1106.show(DEFAULT_COL, SN_ROW, temp);

    //uid
    memset(temp, 0x00, sizeof(temp));
    uint8 *pUID = (uint8*)pb_cfg_proc_get_uid();
    sprintf(temp, "%02X%02X%02X%02X%02X%02X%02X%02X",
            pUID[0], pUID[1], pUID[2], pUID[3],
            pUID[4], pUID[5], pUID[6], pUID[7]);
    devSH1106.show(DEFAULT_COL, UID_ROW, temp);
}

/******************************************************************************
* Function    : pb_gui_show_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_show_info(bool update)
{
    if (!update)
    {
        devSH1106.clear();
    }

    char temp[32];
    char serviceStat[12+1];

    //operate state, order num
    memset(temp, 0x00, sizeof(temp));
    memset(serviceStat, 0, sizeof(serviceStat));
    uint8 serverState = pb_order_operation_state();
    if (serverState == PB_ORDER_OPSTAT_CLOSE)
    {
        sprintf(serviceStat, "%s", "CLOSE       ");
    }
    else
    if (serverState == PB_ORDER_OPSTAT_CLOSE_WAIT)
    {
        sprintf(serviceStat, "%s", "CLOSE WAIT  ");
    }
    else
    if (serverState == PB_ORDER_OPSTAT_SERVICE)
    {
        sprintf(serviceStat, "%s", "IN-SERVICE  ");
    }
    else
    {
        sprintf(serviceStat, "%s", "UNKNOWN     ");
    }
    sprintf(temp, "N:%d,%s", pb_order_number(), serviceStat);
    devSH1106.show(DEFAULT_COL, ORDER_INFO_ROW, temp);

    //net status
    memset(temp, 0x00, sizeof(temp));
    uint8 netStat = pb_ota_net_get_stat();
    switch (pb_ota_net_get_act_dev())
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            sprintf(temp, "GPRS:%d,%d,%d      ", netStat, pb_ota_network_get_csq_rssi(), pb_ota_network_get_csq_ber());
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            sprintf(temp, "ETH:%d             ", netStat);
            break;
        }
        default:
            break;
    }
    devSH1106.show(DEFAULT_COL, NET_INFO_ROW, temp);

    //input stat
    memset(temp, 0x00, sizeof(temp));
    sprintf(temp, "IN:%08X    ", pb_io_input_mask());
    devSH1106.show(DEFAULT_COL, INPUT_STAT_ROW, temp);

    //output stat
    memset(temp, 0x00, sizeof(temp));
    sprintf(temp, "OUT:%08X    ", pb_io_output_mask());
    devSH1106.show(DEFAULT_COL, OUTPUT_STAT_ROW, temp);
}

/******************************************************************************
* Function    : pb_gui_show_volume
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_show_volume(bool update)
{
    if (!update)
    {
        devSH1106.clear();
    }

    char temp[32];
    sprintf(temp, "   Volume Set  ");
    devSH1106.show(DEFAULT_COL, TITLE_ROW, temp);

    memset(temp, 0x00, sizeof(temp));
    sprintf(temp, "    VAL = %d   ", pb_multimedia_get_audio_volume());
    devSH1106.show(DEFAULT_COL, VOLUME_ROW, temp);
}

/******************************************************************************
* Function    : pb_gui_show_upgrade
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_show_upgrade(bool update)
{
    if (!update)
    {
        devSH1106.clear();
    }

    char temp[32];

    //Upgrade title
    sprintf(temp, "Firmware  Update");
    devSH1106.show(DEFAULT_COL, UP_TITLE_ROW, temp);
    
    //Upgrade stage
    memset(temp, 0, sizeof(temp));
    switch (pb_gui_context.upStage)
    {
        case PB_GUI_UP_START:
        {
            sprintf(temp, "preparing ...");
            break;
        }
        case PB_GUI_UP_CONNECT_SERVER:
        {
            sprintf(temp, "connecting ...");
            break;
        }
        case PB_GUI_UP_START_DOWNLOAD:
        {
            sprintf(temp, "trasnferring ...");
            break;
        }
        case PB_GUI_UP_DOWNLOADING:
        {
            sprintf(temp, "downloading ...");
            break;
        }
        case PB_GUI_UP_VERIFY:
        {
            sprintf(temp, "verifying ...");
            break;
        }
        case PB_GUI_UP_OK:
        {
            sprintf(temp, "completed !");
            break;
        }
        case PB_GUI_UP_REBOOT:
        {
            sprintf(temp, "rebooting ...");
            break;
        }
    }
    devSH1106.show(DEFAULT_COL, UP_STAGE_ROW, temp);

    //Upgrade process
    memset(temp, 0, sizeof(temp));
    sprintf(temp, "      %d%%      ", pb_gui_context.upProcess);
    devSH1106.show(DEFAULT_COL, UP_PROCESS_ROW, temp);

    //SYS date time
    pb_util_get_datetime(temp, sizeof(temp));
    devSH1106.show(DEFAULT_COL, UP_DATE_ROW, temp);    
}

/******************************************************************************
* Function    : pb_gui_set_upgrade_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_gui_set_upgrade_info(uint8 stage, uint8 process)
{
    pb_gui_context.upStage = stage;
    pb_gui_context.upProcess = process;
}

/******************************************************************************
* Function    : pb_gui_switch_page
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_show_page(bool update)
{
    switch(pb_gui_context.cursor)
    {
        case PB_GUI_MENU_VERSION:
        {
            pb_gui_show_version(update);
            break;
        }
        case PB_GUI_MENU_INFO:
        {
            pb_gui_show_info(update);
            break;
        }
        case PB_GUI_MENU_VOLUME:
        {
            pb_gui_show_volume(update);
            break;
        }
        case PB_GUI_MENU_UPGRADE:
        {
            pb_gui_show_upgrade(update);
            break;
        }
        default:
            break;
    }
}

/******************************************************************************
* Function    : pb_gui_act_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_act_hdlr(PB_MSG_TYPE *pMsg)
{
    if (pMsg->pMsgData == NULL)
    {
        return;
    }
    uint32 action = *(uint32*)pMsg->pMsgData;

    switch (action)
    {
        case PB_GUI_ACT_UPDATE:
        {
            pb_gui_show_page(true);
            break;
        }
        case PB_GUI_ACT_MENU:
        {
            if ( pb_gui_context.cursor != PB_GUI_MENU_UPGRADE)
            {
                pb_gui_context.cursor = (pb_gui_context.cursor + 1) % PB_GUI_MENU_END;
                pb_gui_show_page(false);
            }
            else
            {
                OS_DBG_TRACE(DBG_MOD_PBGUI, DBG_WARN, "Can not switch menu during upgrading");
            }
            break;
        }
        case PB_GUI_ACT_REVERSE:
        {
            devSH1106.clear();
            devSH1106.reverse();
            pb_gui_show_page(true);
            break;
        }
        case PB_GUI_ACT_VOLDOWN:
        {
            if (pb_gui_context.cursor == PB_GUI_MENU_VOLUME)
            {
                pb_multimedia_send_audio_msg(PB_MM_VOL_DOWN, 0);
                pb_gui_show_page(true);
            }
            break;
        }
        case PB_GUI_ACT_VOLUP:
        {
            if (pb_gui_context.cursor == PB_GUI_MENU_VOLUME)
            {
                pb_multimedia_send_audio_msg(PB_MM_VOL_UP, 0);
                pb_gui_show_page(true);
            }
            break;
        }
        case PB_GUI_ACT_UPGRADEMENU:
        {
            //switch to first page
            if (pb_gui_context.cursor == PB_GUI_MENU_UPGRADE)
            {
                pb_gui_context.cursor = PB_GUI_MENU_VERSION;
            }
            else
            {
                pb_gui_context.cursor = PB_GUI_MENU_UPGRADE;
            }
            pb_gui_show_page(false);
            break;
        }
        default:
            break;
    }
}

/******************************************************************************
* Function    : pb_gui_main_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_gui_main_init(void)
{
    pb_gui_msg_queue = os_msg_queue_create(PB_GUI_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));

    devSH1106.init((HAL_SW_I2C_TYPE *)&OLED_I2C);

    memset(&pb_gui_context, 0, sizeof(pb_gui_context));
    pb_gui_context.cursor = 0;
    pb_gui_context.upStage = 0;
    pb_gui_context.upProcess = 0;
    
    pb_gui_show_version(true);
}

/******************************************************************************
* Global Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_gui_main
*
* Author      :
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_gui_main(void *pvParameters)
{
    pb_gui_main_init();

    os_set_task_init(OS_TASK_ITEM_PB_GUI);
    os_wait_task_init_sync();

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;

    while (1)
    {
        memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_gui_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            switch (p_pb_msg->msgID)
            {
                case PB_MSG_GUI_MENU_ACTION_REQ:
                {
                    pb_gui_act_hdlr(p_pb_msg);
                    break;
                }
                default:
                    OS_DBG_ERR(DBG_MOD_PBGUI, "Unknow msg %d", p_pb_msg->msgID);
                    break;
            }
            
            if (p_pb_msg->pMsgData != NULL)
            {
                os_free(p_pb_msg->pMsgData);
            }
        }
    }
}

/******************************************************************************
* Function    : pb_gui_send_menu_act_req
*
* Author      :
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_gui_send_act_req(PB_GUI_ACTIONS action)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = (uint8*)os_malloc(sizeof(uint32));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBGUI, "msg malloc err");
        return;
    }

    msg.msgID = PB_MSG_GUI_MENU_ACTION_REQ;
    memcpy(msg.pMsgData, &action, sizeof(uint32));

    os_msg_queue_send(pb_gui_msg_queue, ( void*)&msg, 0);
}

