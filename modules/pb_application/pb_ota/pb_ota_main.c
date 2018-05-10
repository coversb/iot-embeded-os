/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ota_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   over the air communication
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hal_board.h"
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_app_message.h"
#include "pb_util.h"
#include "pb_ota_main.h"
#include "pb_prot_main.h"
#include "pb_cfg_proc.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_OTA_NET_CSQ_INTERVAL 5   //sec

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MSG_QUEUE_TYPE pb_ota_msg_queue;
static PB_OTA_CONTEXT_TYPE pb_ota_context;
//static bool pb_ota_need_send_dbi = true;  //not use in this version

/******************************************************************************
* Local Functions define
******************************************************************************/
static bool pb_ota_net_stat_check(uint8 dev, uint8 stat);

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_ota_network_connected
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_ota_network_connected(void)
{
    return pb_ota_net_stat_check(pb_ota_context.act_net_dev, PB_OTA_NET_CONNECTED);
}

/******************************************************************************
* Function    : pb_ota_net_get_stat
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_ota_net_get_stat(void)
{
    return pb_ota_context.net_info[pb_ota_context.act_net_dev].net_state;
}

/******************************************************************************
* Function    : pb_ota_net_get_act_dev
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_ota_net_get_act_dev(void)
{
    return pb_ota_context.act_net_dev;
}

/******************************************************************************
* Function    : pb_ota_need_set_reboot
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set reboot flag
******************************************************************************/
void pb_ota_need_set_reboot(bool sw)
{
    pb_ota_context.need_reboot = sw;
}

/******************************************************************************
* Function    : pb_ota_cell_location_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : handle location message and send location report
******************************************************************************/
static void pb_ota_cell_location_hdlr(PB_MSG_TYPE *pMsg)
{
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 needReport = *(pMsg->pMsgData);

    PB_PROT_RSP_LOC_PARAM locRsp;

    if (pb_ota_context.act_net_dev == PB_OTA_NET_DEV_GPRS)
    {
        if (pb_ota_context.csq_update_cnt >= PB_OTA_NET_CSQ_INTERVAL)
        {
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_WARN, "***Cell loc wait csq update***");
            os_scheduler_delay(DELAY_500_MS);
            pb_ota_send_msg_data_to_ota_mod(PB_MSG_OTA_CELL_LOCATION_REQ, needReport);
            return;
        }

        if (0)//pb_util_get_location((char*)locRsp.longitude, (char*)locRsp.latitude))
        {
            locRsp.fixType = 0x10;//GSM basestion location
            locRsp.timestamp = pb_util_get_timestamp();
            //pb_prot_proc_set_dev_location(&locRsp);
        }
        else
        {
            //pb_prot_proc_get_dev_location(&locRsp);
        }
    }
    else
    {
        //pb_prot_proc_get_dev_location(&locRsp);
    }

    if (needReport)
    {
        pb_prot_send_rsp_param_req(PB_PROT_RSP_LOC, (uint8*)&locRsp, sizeof(locRsp));
    }
}

/******************************************************************************
* Function    : pb_ota_gsm_info_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_gsm_info_hdlr(PB_MSG_TYPE *pMsg)
{
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 needReport = *(pMsg->pMsgData);

    PB_PROT_RSP_GSMINFO_PARAM gsmRsp;
    memset(&gsmRsp, 0, sizeof(gsmRsp));

    if (pb_ota_context.act_net_dev == PB_OTA_NET_DEV_GPRS)
    {
        if (pb_ota_context.csq_update_cnt >= PB_OTA_NET_CSQ_INTERVAL)
        {
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_WARN, "***GSM info wait csq update***");
            os_scheduler_delay(DELAY_500_MS);
            pb_ota_send_msg_data_to_ota_mod(PB_MSG_OTA_GSMINFO_REQ, needReport);
            return;
        }
        
        //pb_util_get_gsm_info(&gsmRsp);
        //pb_prot_proc_set_dev_gsm_info(&gsmRsp);
    }
    else
    {
        //pb_prot_proc_get_dev_gsm_info(&gsmRsp);
    }

    if (needReport)
    {
        pb_prot_send_rsp_param_req(PB_PROT_RSP_GSM, (uint8*)&gsmRsp, sizeof(gsmRsp));
    }
}

/******************************************************************************
* Function    : pb_ota_set_recv_copying
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set flag in case of write during reading
******************************************************************************/
void pb_ota_set_recv_copying(bool sw)
{
    pb_ota_context.b_recv_copying = sw;
}

/******************************************************************************
* Function    : pb_ota_get_recv_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_ota_get_recv_data(uint8 *data, uint16 maxLen)
{
    uint16 len;
    len = MIN_VALUE(pb_ota_context.recv_len, maxLen);
    
    memcpy(data, pb_ota_context.recv_buff, len);

    return len;
}

/******************************************************************************
* Function    : pb_ota_try_to_send_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_try_to_send_data(void)
{
    if (!pb_ota_net_stat_check(pb_ota_context.act_net_dev, PB_OTA_NET_CONNECTED))
    {
        return;
    }
    
    if (!pb_ota_context.b_sending && (pb_ota_network_send_que_size() > 0))
    {
        pb_ota_send_dev_msg_to_ota_mod(PB_OTA_NET_DEV_UNKNOW, PB_MSG_OTA_NET_SEND);
    }
}

/******************************************************************************
* Function    : pb_ota_send_data_append
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_ota_send_data_append(const uint8* data, uint16 len)
{
    bool ret = false;

    if (pb_ota_network_send_que_append(data, len))
    {
        pb_ota_try_to_send_data();
        ret = true;
    }
    
    return ret;
}

/******************************************************************************
* Function    : pb_ota_try_to_recv_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_try_to_recv_data(void)
{
    if (!pb_ota_net_stat_check(pb_ota_context.act_net_dev, PB_OTA_NET_CONNECTED))
    {
        return;
    }
    
    if (!pb_ota_context.b_recving && !pb_ota_context.b_recv_copying)
    {
        pb_ota_send_dev_msg_to_ota_mod(PB_OTA_NET_DEV_UNKNOW, PB_MSG_OTA_NET_RECV);
    }
}

#if 0   //not use
/******************************************************************************
* Function    : pb_ota_send_msgdata_to_ota_mod
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send message and data to ota module
******************************************************************************/
void pb_ota_send_msgdata_to_ota_mod(PB_MSG_TYPE *msg)
{
    pb_util_message_queue_send(pb_ota_msg_queue, ( void*)msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Send[%d]data to OTA", msg->msgID);
}
#endif

/******************************************************************************
* Function    : pb_ota_send_msg_to_ota_mod
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send message to ota module
******************************************************************************/
void pb_ota_send_msg_to_ota_mod(uint8 msgID)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = NULL;
    msg.msgID = msgID;
    
    os_msg_queue_send(pb_ota_msg_queue, ( void*)&msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Send[%d]to OTA", msgID);
}

/******************************************************************************
* Function    : pb_ota_send_msg_data_to_ota_mod
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_send_msg_data_to_ota_mod(uint8 msgID, uint8 data)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = (uint8*)os_malloc(sizeof(uint8));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBOTA, "msg malloc err");
        return;
    }

    msg.msgID = msgID;
    *(msg.pMsgData) = data;
    os_msg_queue_send(pb_ota_msg_queue, ( void*)&msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Send [%d][%d] to OTA", data, msgID);
}

/******************************************************************************
* Function    : pb_ota_send_dev_msg_to_ota_mod
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send msg to ota mod , msg data is net device type
******************************************************************************/
void pb_ota_send_dev_msg_to_ota_mod(uint8 devType, uint8 msgID)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = (uint8*)os_malloc(sizeof(uint8));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBOTA, "msg malloc err");
        return;
    }

    msg.msgID = msgID;
    *(msg.pMsgData) = devType;
    os_msg_queue_send(pb_ota_msg_queue, ( void*)&msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Send dev[%d][%d] to OTA", devType, msgID);
}

/******************************************************************************
* Function    : pb_ota_net_stat_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_net_stat_set(uint8 dev, uint8 stat)
{
    if (dev >= PB_OTA_NET_DEV_NUM)
    {
        return;
    }

    pb_ota_context.net_info[dev].net_state = stat;
}

/******************************************************************************
* Function    : pb_ota_net_stat_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_ota_net_stat_check(uint8 dev, uint8 stat)
{
    if (dev >= PB_OTA_NET_DEV_NUM)
    {
        return false;
    }
    
    if (pb_ota_context.net_info[dev].net_state == stat)
    {
        return true;
    }

    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net stat[%d] cur[%d], need[%d]", 
                                 dev, pb_ota_context.net_info[dev].net_state, stat);

    return false;
}

/******************************************************************************
* Function    : pb_ota_net_retry_cnt_clear
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_net_retry_cnt_clear(void)
{
    pb_ota_context.net_retry_cnt = 0;
}

/******************************************************************************
* Function    : pb_ota_net_restart
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : restart net try to other net dev
******************************************************************************/
static void pb_ota_net_restart(uint8 curDevType)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net[%d] err, try to restart", curDevType);

    uint8 restartDevType = curDevType;

    if (restartDevType >= PB_OTA_NET_DEV_NUM)
    {
        OS_DBG_ERR(DBG_MOD_PBOTA, "Unknow dev,set to ETH");

        restartDevType = PB_OTA_NET_DEV_ETH;
        pb_ota_context.net_retry_cnt = 0;
    }
    else
    {
        if (pb_ota_context.net_retry_cnt >= PB_OTA_NET_SWITCH_RETRY)
        {
            pb_ota_context.net_retry_cnt = 0;

            //try other dev net
            if (curDevType == PB_OTA_NET_DEV_ETH)
            {
                restartDevType = PB_OTA_NET_DEV_GPRS;
            }
            else
            {
                restartDevType = PB_OTA_NET_DEV_ETH;
            }
        }
        pb_ota_context.net_retry_cnt++;
    }

    pb_ota_context.act_net_dev = restartDevType;
    pb_ota_net_stat_set(restartDevType, PB_OTA_NET_DEV_INIT);
    pb_ota_send_dev_msg_to_ota_mod(restartDevType, PB_MSG_OTA_NET_DEV_RESET);
}

/******************************************************************************
* Function    : pb_ota_net_hw_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : net dev hardware reset
******************************************************************************/
static void pb_ota_net_hw_reset(PB_MSG_TYPE *pMsg)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net dev hw rst");

    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 devType = *(pMsg->pMsgData);

    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_DEV_INIT))
    {
        return;
    }

    pb_ota_network_hw_reset(devType);

    if (pb_ota_network_available(devType))
    {
        pb_ota_net_stat_set(devType, PB_OTA_NET_CONFIG);
        pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CONFIG);
    }
    else
    {
        pb_ota_net_restart(devType);
    }
}

/******************************************************************************
* Function    : pb_ota_net_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network config
******************************************************************************/
static void pb_ota_net_config(PB_MSG_TYPE *pMsg)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net config");
    
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 devType = *(pMsg->pMsgData);

    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_CONFIG))
    {
        return;
    }

    if (!pb_ota_network_config(devType))
    {
        pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
        pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);

        OS_DBG_ERR(DBG_MOD_PBOTA, "Net config err, close socket and reset net");
    }
    else
    {
        pb_ota_net_stat_set(devType, PB_OTA_NET_CONNECT);
        pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CONNECT);
    }
}

/******************************************************************************
* Function    : pb_ota_net_get_act_server
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_net_get_act_server(char *server, uint16 *port)
{
    PB_CFG_SER *ser = &(pb_cfg_proc_get_cmd()->ser);
    
    switch (pb_ota_context.act_server)
    {
        case PB_OTA_SERVER_MAIN:
        {
            strncpy(server, (char*)ser->mainServer, strlen((char*)ser->mainServer));
            *port = ser->mainPort;
            break;
        }
        case PB_OTA_SERVER_BACKUP:
        {
            strncpy(server, (char*)ser->bakServer, strlen((char*)ser->bakServer));
            *port = ser->bakPort;
            break;
        }
        default:
        {
            pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
            strncpy(server, (char*)ser->mainServer, strlen((char*)ser->mainServer));
            *port = ser->mainPort;
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_ota_net_switch_act_server
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_ota_net_switch_act_server(void)
{
    bool ret = false;
    //Report mode is 0 or 1
    if (pb_cfg_proc_get_cmd()->ser.mode != PB_SER_MODE_BACKUP_SERVER)
    {
        pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
        return true;
    }

    //Report mode is 2
    switch (pb_ota_context.act_server)
    {
        case PB_OTA_SERVER_MAIN:
        {
            pb_ota_context.act_server = PB_OTA_SERVER_BACKUP;
            break;
        }
        case PB_OTA_SERVER_BACKUP:
        {
            pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
            ret = true;
            break;
        }
        default:
        {
            pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
            break;
        }
    }

    return ret;
}


/******************************************************************************
* Function    : pb_ota_net_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network connect , create connection to server
******************************************************************************/
static void pb_ota_net_connect(PB_MSG_TYPE *pMsg)
{
    char serverDomain[PB_SER_DOMAINNAME_LEN+1];
    uint16 serverPort;
    
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net connect");
    
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 devType = *(pMsg->pMsgData);
    
    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_CONNECT))
    {
        return;
    }

retry:
    memset(serverDomain, 0, sizeof(serverDomain));
    serverPort = 0;
    pb_ota_net_get_act_server(serverDomain, &serverPort);
    if (!pb_ota_network_connect(devType, serverDomain, serverPort))
    {
        //switch active server to be coonnected
        if (pb_ota_net_switch_act_server())
        {
            pb_ota_context.server_connect_cnt++;
        }
        //check server connect count
        if (pb_ota_context.server_connect_cnt < PB_OTA_SERVER_SWITCH_RETRY)
        {
            pb_ota_network_disconnect(devType);
            goto retry;
        }
        else
        {
            pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
            pb_ota_context.server_connect_cnt = 0;
            pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
            pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);
            
            OS_DBG_ERR(DBG_MOD_PBOTA, "Net connect err, close socket and reset net");
        }
    }
    else
    {
        pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
        pb_ota_context.server_connect_cnt = 0;
        pb_ota_net_stat_set(devType, PB_OTA_NET_CONNECTED);
        pb_ota_net_retry_cnt_clear();

        #if 0//disable auto send in this version
        //After bootup, first time to connect network, send device basic info
        if (pb_ota_need_send_dbi)
        {
            pb_ota_need_send_dbi = false;
            pb_prot_proc_device_basic_info_process();
        }
        #endif//disable auto send in this version
    }
}

/******************************************************************************
* Function    : pb_ota_net_gprs_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : gprs close socket 
******************************************************************************/
static void pb_ota_net_close(PB_MSG_TYPE *pMsg)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net close");

    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    uint8 devType = *(pMsg->pMsgData);

    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_CLOSE))
    {
        return;
    }

    pb_ota_network_disconnect(devType);
    pb_ota_net_restart(devType);
}

/******************************************************************************
* Function    : pb_ota_net_gprs_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : gprs send data
******************************************************************************/
static void pb_ota_net_send(PB_MSG_TYPE *pMsg)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net send");

    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }

    uint8 devType = *(pMsg->pMsgData);
    if (devType == PB_OTA_NET_DEV_UNKNOW)
    {
        devType = pb_ota_context.act_net_dev;
    }

    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_CONNECTED))
    {
        pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
        pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);

        OS_DBG_ERR(DBG_MOD_PBOTA, "Net stat err, close socket and reset net");
        return;
    }

    pb_ota_context.b_sending = true;
    //try to send all the message
    while (pb_ota_network_send_que_size() != 0)
    {
        uint8 *data = NULL;
        uint16 len = 0;

        if (!pb_ota_network_que_head_data(&data, &len))
        {
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_WARN, "Net send que is empty");
            break;
        }
        #if (PB_OTA_DBG == 1)//debug to show recv string
        char sendHex[128+1];
        uint16 hexLen = MIN_VALUE(len, 120/2);
        os_trace_get_hex_str((uint8*)sendHex, sizeof(sendHex), data, hexLen);
        OS_INFO("Send:%s", sendHex);
        #else
        OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Send [%d]", len);
        #endif /*PB_OTA_DBG*/

        if (0 == pb_ota_network_send(devType, data, len))
        {
            pb_ota_context.send_retry_cnt++;
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_WARN, "SEND [%d]retry %d", devType, pb_ota_context.send_retry_cnt);
            #if 0
            if ((pb_ota_context.send_retry_cnt > PB_OTA_SEND_MAX_RETRY
                && (!pb_ota_network_check_net_stat(devType)))
                //something wrong in net device, maybe ram full, also need restart
                || pb_ota_context.send_retry_cnt > PB_OTA_SEND_RETRY_EXCEED)
            #else
            if ((pb_ota_context.send_retry_cnt >= PB_OTA_SEND_MAX_RETRY)
                || (!pb_ota_network_check_net_stat(devType)))
            #endif

            {
                pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
                pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);

                OS_DBG_ERR(DBG_MOD_PBOTA, "Net send err, close socket and reset net");
                break;
            }
        }
        else
        {
            pb_ota_context.send_retry_cnt = 0;
            pb_ota_network_send_que_remove_head();
            os_scheduler_delay(DELAY_100_MS);
        }
    }
    pb_ota_context.b_sending = false;

    if (pb_ota_context.need_reboot)
    {
        pb_ota_context.need_reboot = false;
        hal_board_reset();
    }
}

/******************************************************************************
* Function    : pb_ota_net_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_net_recv(PB_MSG_TYPE *pMsg)
{
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Net recv");

    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }

    uint8 devType = *(pMsg->pMsgData);
    if (devType == PB_OTA_NET_DEV_UNKNOW)
    {
        devType = pb_ota_context.act_net_dev;
    }

    if (!pb_ota_net_stat_check(devType, PB_OTA_NET_CONNECTED))
    {
        pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
        pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);

        OS_DBG_ERR(DBG_MOD_PBOTA, "Net stat err, close socket and reset net");
        return;
    }

    pb_ota_context.b_recving = true;
    pb_ota_context.recv_len = pb_ota_network_recv(devType, pb_ota_context.recv_buff, PB_OTA_RECV_BUFF_SIZE);

    if (pb_ota_context.recv_len > 0)
    {
        pb_ota_set_recv_copying(true);
        
        pb_prot_input_available_set(PB_PORT_SRC_OTA, true);
        pb_prot_send_msg_to_prot_mod(PB_MSG_PROT_ANALYSE_DATA);
    }

    if (pb_ota_context.recv_len >0)
    {
        #if (PB_OTA_DBG == 1)//debug to show recv string
        uint16 tmpStrLen = 0;
        tmpStrLen = MIN_VALUE(pb_ota_context.recv_len, PB_OTA_RECV_BUFF_SIZE);
        pb_ota_context.recv_buff[tmpStrLen] = '\0';
        OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Recv [%d][%s]", pb_ota_context.recv_len, pb_ota_context.recv_buff);

        char recvHex[128+1];
        uint16 hexLen = MIN_VALUE(pb_ota_context.recv_len, 120/2);
        os_trace_get_hex_str((uint8*)recvHex, sizeof(recvHex), pb_ota_context.recv_buff, hexLen);
        OS_INFO("RECV:%s", recvHex);
        #else
        OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Recv [%d]", pb_ota_context.recv_len);
        #endif /*PB_OTA_DBG*/
    }

    //get GPRS csq
    if (++pb_ota_context.csq_update_cnt >= PB_OTA_NET_CSQ_INTERVAL)
    {
        pb_ota_network_update_csq(pb_ota_context.act_net_dev);
        pb_ota_context.csq_update_cnt = 0;

        //check netstate
        if (!pb_ota_network_check_net_stat(devType))
        {
            pb_ota_net_stat_set(devType, PB_OTA_NET_CLOSE);
            pb_ota_send_dev_msg_to_ota_mod(devType, PB_MSG_OTA_NET_CLOSE);
            OS_DBG_ERR(DBG_MOD_PBOTA, "Net send err, close socket and reset net");
        }
    }    

    pb_ota_context.b_recving = false;
}

/******************************************************************************
* Function    : pb_ota_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ota_init(void)
{
    pb_ota_msg_queue = os_msg_queue_create(PB_OTA_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));

    pb_ota_network_context_init();
    pb_ota_network_hw_init(PB_OTA_NET_DEV_GPRS);
    pb_ota_network_hw_init(PB_OTA_NET_DEV_ETH);

    memset(&pb_ota_context, 0, sizeof(pb_ota_context));
    pb_ota_context.need_reboot = false;
    pb_ota_context.act_net_dev = PB_OTA_NET_DEV_ETH;
    pb_ota_context.act_server = PB_OTA_SERVER_MAIN;
    pb_ota_context.server_connect_cnt = 0;
    pb_ota_context.send_retry_cnt = 0;
    pb_ota_context.net_retry_cnt = 0;
    pb_ota_context.b_sending = false;
    pb_ota_context.b_recving = false;
    pb_ota_context.b_recv_copying = false;
    pb_ota_context.csq_update_cnt = 0;

    pb_ota_net_stat_set(pb_ota_context.act_net_dev, PB_OTA_NET_DEV_INIT);
    pb_ota_send_dev_msg_to_ota_mod(pb_ota_context.act_net_dev, PB_MSG_OTA_NET_DEV_RESET);
}

/******************************************************************************
* Function    : pb_prot_main
* 
* Author      : Chen Hao
* 
* Parameters  : command sub-type and args
* 
* Return      : 
* 
* Description : save command configuration and execute command
******************************************************************************/
void pb_ota_main(void *pvParameters)
{
    pb_ota_init();
    
    os_set_task_init(OS_TASK_ITEM_PB_OTA);
    os_wait_task_init_sync();
    
    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        //need some blocking function in case of wasting cpu      
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_ota_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Recv msg %d", p_pb_msg->msgID);
            switch (p_pb_msg->msgID)
            {
                case PB_MSG_OTA_NET_DEV_RESET:
                {
                    pb_ota_net_hw_reset(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_NET_CONFIG:
                {
                    pb_ota_net_config(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_NET_CONNECT:
                {
                    pb_ota_net_connect(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_NET_CLOSE:
                {
                    pb_ota_net_close(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_NET_SEND:
                {
                    pb_ota_net_send(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_NET_RECV:
                {
                    pb_ota_net_recv(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_CELL_LOCATION_REQ:
                {
                    pb_ota_cell_location_hdlr(p_pb_msg);
                    break;
                }
                case PB_MSG_OTA_GSMINFO_REQ:
                {
                    pb_ota_gsm_info_hdlr(p_pb_msg);
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_PBOTA, "Unknow msg %d", p_pb_msg->msgID);
                    break;
                }
            }

            if (p_pb_msg->pMsgData != NULL)
            {
                os_free(p_pb_msg->pMsgData);
            }
            p_pb_msg->msgID = PB_MESSAGE_NONE;
        }
    }
}

