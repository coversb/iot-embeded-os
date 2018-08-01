/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_prot_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   pb_prot main flow
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-18      1.00                    Chen Hao
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
#include "pb_io_monitor_main.h"
#include "pb_util.h"
#include "pb_cfg_proc.h"
#include "pb_fota.h"
#include "pb_crypto.h"
#include "pb_prot_cmd.h"
#include "pb_prot_main.h"
#include "pb_prot_parse.h"
#include "pb_prot_assemble.h"
#include "pb_prot_proc.h"
#include "pb_ota_main.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
uint8 g_pb_rsp_buff[PB_PROT_RSP_BUFF_SIZE];

static OS_MSG_QUEUE_TYPE pb_prot_msg_queue;
static PB_PROT_CONTEXT_TYPE pb_prot_context;

/******************************************************************************
* Local Functions
******************************************************************************/
void pb_prot_set_need_wait_data(bool sw)
{
    pb_prot_context.bNeedWaitData = sw;
}

/******************************************************************************
* Function    : pb_prot_send_msgdata_to_prot_mod
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send message and data to prot module
******************************************************************************/
void pb_prot_send_msgdata_to_prot_mod(PB_MSG_TYPE *msg)
{
    os_msg_queue_send(pb_prot_msg_queue, ( void*)msg, 0);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send[%d]data to PROT", msg->msgID);
}

/******************************************************************************
* Function    : pb_prot_send_msg_to_prot_mod
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send message to ota module
******************************************************************************/
void pb_prot_send_msg_to_prot_mod(uint8 msgID)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = NULL;
    msg.msgID = msgID;

    os_msg_queue_send(pb_prot_msg_queue, ( void*)&msg, 0);
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send[%d] to PROT", msgID);
}

/******************************************************************************
* Function    : pb_prot_send_rsp_req
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_send_rsp_req(PB_PROT_MSG_RSP_TYPE msgSubType)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = (uint8*)os_malloc(sizeof(PB_PROT_RSP_TYPE));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "msg malloc error");
        return;
    }
    PB_PROT_RSP_TYPE *rsp = (PB_PROT_RSP_TYPE*)msg.pMsgData;

    rsp->subType = msgSubType;
    rsp->param[0] = NULL;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send RSP req[%02X] to PROT", msgSubType);
    msg.msgID = PB_MSG_PROT_SEND_RSP;

    os_msg_queue_send(pb_prot_msg_queue, ( void*)&msg, 0);
}

/******************************************************************************
* Function    : pb_prot_send_rsp_param_req
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send RSP with param
******************************************************************************/
void pb_prot_send_rsp_param_req(PB_PROT_MSG_RSP_TYPE msgSubType, uint8 *param, uint16 paramSize)
{
    PB_MSG_TYPE msg;
    uint16 mallocSize = sizeof(PB_PROT_RSP_TYPE) + paramSize;

    msg.pMsgData = (uint8 *)os_malloc(mallocSize);

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "msg malloc error");
        return;
    }

    PB_PROT_RSP_TYPE *rsp = (PB_PROT_RSP_TYPE *)msg.pMsgData;
    rsp->subType = msgSubType;
    memcpy(rsp->param, param, paramSize);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send RSP req[%02X] to PROT", msgSubType);
    msg.msgID = PB_MSG_PROT_SEND_RSP;

    os_msg_queue_send(pb_prot_msg_queue, ( void*)&msg, 0);
}

/******************************************************************************
* Function    : pb_prot_send_sae_req
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_send_sae_req(uint8 alarmType, uint8 alarmLv)
{
    PB_PROT_RSP_SAE_PARAM saeParam;
    saeParam.alarmType = alarmType;
    saeParam.alarmLv = alarmLv;

    pb_prot_send_rsp_param_req(PB_PROT_RSP_SAE, (uint8*)&saeParam, sizeof(saeParam));
}

/******************************************************************************
* Function    : pb_order_send_uie
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_send_uie_req(uint8 type, uint8 *data)
{
    PB_PROT_RSP_UIE_PARAM uieParam;
    memset(&uieParam, 0, sizeof (uieParam));
    uieParam.type = type;
    memcpy(uieParam.data, data, MIN_VALUE(PB_UIE_DATA_LEN, strlen((char*)data)));

    pb_prot_send_rsp_param_req(PB_PROT_RSP_UIE, (uint8*)&uieParam, sizeof(uieParam));
}

/******************************************************************************
* Function    : pb_prot_send_coe_req
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_send_coe_req(uint8 type, uint32 operationID, uint32 consumerID, uint8 *info)
{
    PB_PROT_RSP_COE_PARAM coeParam;
    memset(&coeParam, 0, sizeof(coeParam));
    coeParam.type = type;
    coeParam.operationID = operationID;
    coeParam.consumerID = consumerID;
    memcpy(coeParam.info, info, MIN_VALUE(strlen((char*)info), PB_COE_INFO_LEN));

    OS_INFO("COE type:%d, operationID:%d, consumerID:%d, info:%s", 
                  coeParam.type,
                  coeParam.operationID, 
                  coeParam.consumerID,
                  coeParam.info);

    pb_prot_send_rsp_param_req(PB_PROT_RSP_COE, (uint8*)&coeParam, sizeof(coeParam));
}

/******************************************************************************
* Function    : pb_prot_send_dbg_info_req
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_send_dbg_info_req(uint8 *debugInfo, uint16 infoLen)
{
    PB_MSG_TYPE msg;
    uint16 mallocSize = sizeof(PB_PROT_RSP_TYPE) + infoLen;

    msg.pMsgData = (uint8 *)os_malloc(mallocSize);

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "msg malloc error");
        return;
    }

    PB_PROT_RSP_TYPE *rsp = (PB_PROT_RSP_TYPE *)msg.pMsgData;
    rsp->subType = PB_PROT_RSP_DBG;
    memcpy(rsp->param, debugInfo, infoLen);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send DBG INFO req to PROT");
    msg.msgID = PB_MSG_PROT_SEND_RSP;

    os_msg_queue_send(pb_prot_msg_queue, ( void*)&msg, 0);
}

/******************************************************************************
* Function    : pb_prot_input_available_set
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : set data available flag
******************************************************************************/
void pb_prot_input_available_set(uint8 id, bool available)
{
    if (id > PB_PORT_SRC_NUM)
    {
        return;
    }
    pb_prot_context.input[id] = available;
}

/******************************************************************************
* Function    : pb_prot_input_available_get
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get currnet available srcid to get the right data
******************************************************************************/
static PB_PROT_SRC_ID pb_prot_input_available_get(void)
{
    for (uint8 idx = 0; idx < PB_PORT_SRC_NUM; ++idx)
    {
        if (pb_prot_context.input[idx])
        {
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Get [%s] data", pbProtSrcName[idx]);

            return (PB_PROT_SRC_ID)idx;
        }
    }

    return PB_PORT_SRC_UNKNWON;
}

/******************************************************************************
* Function    : pb_prot_input_available
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check whether there is  available data
******************************************************************************/
static bool pb_prot_available(void)
{
    for (uint8 idx = 0; idx < PB_PORT_SRC_NUM; ++idx)
    {
        if (pb_prot_context.input[idx])
        {
            pb_prot_context.parseType = PB_PROT_PARSE_INPUT;
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%s] has new data", pbProtSrcName[idx]);

            return true;
        }
    }

    for (uint8 idx = 0; idx < PB_PORT_SRC_NUM; ++idx)
    {
        if (pb_prot_parse_rawbuff_check(idx))
        {
            pb_prot_context.rawbuff[idx] = true;
            pb_prot_context.parseType = PB_PROT_PARSE_RAWBUFF;

            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%s] has none-parsed data", pbProtSrcName[idx]);

            return true;
        }
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_uart_get
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get data from uart buffer
******************************************************************************/
static void pb_prot_uart_get(void)
{
    pb_prot_context.inputLen = MIN_VALUE(PB_DEBUG_COM.available(), PB_PROT_INPUT_BUFFSIZE);

    memset(pb_prot_context.inputBuff, 0, sizeof(pb_prot_context.inputBuff));
    PB_DEBUG_COM.readBytes(pb_prot_context.inputBuff, pb_prot_context.inputLen);
}

/******************************************************************************
* Function    : pb_prot_ota_get
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : copy data from OTA
******************************************************************************/
static void pb_prot_ota_get(void)
{
    pb_prot_context.inputLen = pb_ota_get_recv_data(pb_prot_context.inputBuff, PB_PROT_INPUT_BUFFSIZE);
    pb_ota_set_recv_copying(false);
}

/******************************************************************************
* Function    : pb_prot_rawdata_get
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get input data
******************************************************************************/
static bool pb_prot_input_get(PB_PROT_RAW_PACKET_TYPE *raw)
{
    PB_PROT_SRC_ID id = PB_PORT_SRC_UNKNWON;

    id = pb_prot_input_available_get();

    switch (id)
    {
        case PB_PROT_SRC_UART:
        {
            pb_prot_uart_get();
            break;
        }
        case PB_PORT_SRC_OTA:
        {
            pb_prot_ota_get();
            break;
        }
        case PB_PORT_SRC_BT:
        {
            break;
        }
        case PB_PORT_SRC_SMS:
        {
            break;
        }
        case PB_PORT_SRC_UNKNWON:
        {
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "No input");
            return false;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Bad SRC ID[%d]", id);
            return false;
        }
    }

    raw->srcID = id;
    raw->rawData = pb_prot_context.inputBuff;
    raw->rawLength = pb_prot_context.inputLen;

    pb_prot_input_available_set(id, false);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Get [%d] new [%s] data", raw->rawLength, pbProtSrcName[id]);

    return true;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_get
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_rawbuff_get(PB_PROT_RAW_PACKET_TYPE *raw)
{
    for (uint8 idx = 0; idx < PB_PORT_SRC_NUM; ++idx)
    {
        if (pb_prot_context.rawbuff[idx])
        {
            raw->rawData = NULL;
            raw->rawLength = 0;
            raw->srcID = (PB_PROT_SRC_ID)idx;

            pb_prot_context.rawbuff[idx] = false;
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Get remaining [%s] data", pbProtSrcName[idx]);
            return true;
        }
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_send_ack_to_ota
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send ack to server
******************************************************************************/
static bool pb_prot_send_ack_to_ota(uint8 *data, uint16 len)
{
    if (!pb_ota_send_data_append(data, len))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "OTA ACK append error [%d]", len);
        return false;
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "OTA ACK append[%d]", len);
    return true;
}

/******************************************************************************
* Function    : pb_prot_send_ack
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_send_ack(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool ret = false;

    if (parsedFrame->srcID == PB_PROT_SRC_UART)
    {
        OS_INFO("OK");
        ret = true;
        goto end;
    }

    PB_PROT_ACK_PACK_TYPE ackPack;
    memset(g_pb_rsp_buff, 0, sizeof(g_pb_rsp_buff));
    ackPack.data = g_pb_rsp_buff;
    ackPack.length = 0;
    ackPack.srcID = parsedFrame->srcID;

    //assemble ack
    ret = (bool)pb_prot_assemble_ack(parsedFrame, &ackPack);
    if (!ret) goto end;

    switch (ackPack.srcID)
    {
    case PB_PORT_SRC_OTA:
    {
        ret = pb_prot_send_ack_to_ota(ackPack.data, ackPack.length);
        break;
    }
    case PB_PORT_SRC_BT:
    {
        break;
    }
    case PB_PORT_SRC_SMS:
    {
        break;
    }
    default:
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Ack bad srcID[%d]", ackPack.srcID);
        ret = false;
        break;
    }
    }

end:
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Send ack err");
    }
    return ret;
}

/******************************************************************************
* Function    : pb_prot_send_hbp
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send heart beat package to server
******************************************************************************/
static void pb_prot_send_hbp(void)
{
    PB_PROT_HBP_PACK_TYPE hbpPack;
    memset(g_pb_rsp_buff, 0, sizeof(g_pb_rsp_buff));
    
    hbpPack.srcID = PB_PORT_SRC_UNKNWON;
    hbpPack.length = 0;
    hbpPack.data = g_pb_rsp_buff;

    pb_prot_assemble_hbp(&hbpPack);
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Assemble HBP %d", hbpPack.length);

    //Added a un-confirmed HBP
    if (pb_prot_check_event(PB_PROT_ACK_GEN))
    {
        pb_prot_proc_set_sack_cnt(true);
    }

    if (!pb_ota_send_data_append(hbpPack.data, hbpPack.length))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "HBP append [%d] err", hbpPack.length);
        return;
    }
}

/******************************************************************************
* Function    : pb_prot_send_rsp
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send rsp to server
******************************************************************************/
static void pb_prot_send_rsp(PB_PROT_RSP_TYPE *rsp)
{
    PB_PROT_RSP_PACK_TYPE rspPack;
    memset(g_pb_rsp_buff, 0, sizeof(g_pb_rsp_buff));

    rspPack.srcID = PB_PORT_SRC_UNKNWON;
    rspPack.data = g_pb_rsp_buff;
    rspPack.length = 0;
    rspPack.msgSubType = rsp->subType;
    rspPack.msgParam = rsp->param;

    if (0 == pb_prot_assemble_rsp(&rspPack))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "RSP assemble[%d] err", rspPack.length);
        return;
    }

    if (!pb_ota_send_data_append(rspPack.data, rspPack.length))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "RSP append[%d] err", rspPack.length);
        return;
    }
}

/******************************************************************************
* Function    : pb_prot_check_event
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : bitmask to check whether send RSP
******************************************************************************/
bool pb_prot_check_event(uint8 subType)
{
    uint16 eventMask = pb_cfg_proc_get_cmd()->cfg.eventMask;
    bool ret = true;

    switch (subType)
    {
        case PB_PROT_RSP_PNE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_PNE));
            break;
        }
        case PB_PROT_RSP_PFE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_PFE));
            break;
        }
        case PB_PROT_RSP_UIE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_UIE));
            break;
        }
        case PB_PROT_RSP_PSE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_PSE));
            break;
        }
        case PB_PROT_RSP_DSE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_DSE));
            break;
        }
        case PB_PROT_RSP_MUE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_MUE));
            break;
        }
        case PB_PROT_RSP_PCE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_PCE));
            break;
        }
        case PB_PROT_RSP_COE:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_COE));
            break;
        }
        case PB_PROT_ACK_GEN:
        {
            ret = (bool)(0 != BIT_CHECK(eventMask, PB_CFG_EVENT_GEN));
            break;
        }
        default:
            break;
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "EVENT[%04X], [%02X], %d", eventMask, subType, ret);
    return ret;
}

/******************************************************************************
* Function    : pb_prot_send_rsp_hdlr
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_send_rsp_hdlr(PB_MSG_TYPE *pMsg)
{
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    PB_PROT_RSP_TYPE *rsp = (PB_PROT_RSP_TYPE*)pMsg->pMsgData;

    if (pb_prot_check_event(rsp->subType))
    {
        pb_prot_send_rsp(rsp);
    }
}

/******************************************************************************
* Function    : pb_prot_analyse
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_analyse(void)
{
    bool ret = false;
    PB_PROT_CMD_PARSED_FRAME_TYPE cmdParsedFrame;
    PB_PROT_RAW_PACKET_TYPE rawPack;
    memset(&cmdParsedFrame, 0, sizeof(cmdParsedFrame));
    memset(&rawPack, 0, sizeof(rawPack));

    //get input data
    switch (pb_prot_context.parseType)
    {
        case PB_PROT_PARSE_INPUT:
        {
            ret = pb_prot_input_get(&rawPack);
            break;
        }
        case PB_PROT_PARSE_RAWBUFF:
        {
            ret = pb_prot_rawbuff_get(&rawPack);
            break;
        }
        default:
            break;
    }
    if (!ret) goto error;

    if (pb_prot_cmd_parse_ascii(&rawPack))
    {
        return false;
    }

    //parse input data
    ret = pb_prot_parse_raw_data(&rawPack, &cmdParsedFrame);
    if (!ret) goto error;

    //send ack to srcID
    if (cmdParsedFrame.msgType == PB_PROT_CMD)//only command need report ack
    {
        ret = pb_prot_send_ack(&cmdParsedFrame);
        if (!ret) goto error;
    }

    //execute command and save
    pb_prot_proc_cmd_exec(&cmdParsedFrame);

error:
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad cmd");
    }
    return ret;
}

/******************************************************************************
* Function    : pb_prot_input_available_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check remained input data in cache buff
******************************************************************************/
static void pb_prot_analyse_data(void)
{
    while (pb_prot_available())
    {
        pb_prot_analyse();
        if (pb_prot_context.bNeedWaitData)
        {
            break;
        }
        os_scheduler_delay(DELAY_100_MS);
    }
}

/******************************************************************************
* Function    : pb_prot_function_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : start protocol functions accroding to config
******************************************************************************/
static void pb_prot_function_init(bool pwrOn)
{
    //set encrypt key
    pb_crypto_set_key(pb_cfg_proc_get_cmd()->sec.comKey);

    if (pwrOn)
    {
        //send POWER ON RSP
        pb_prot_send_rsp_req(PB_PROT_RSP_PNE);
    }
}

/******************************************************************************
* Function    : pb_prot_main_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_init(void)
{
    pb_prot_msg_queue = os_msg_queue_create(PB_PROT_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));
    memset(&pb_prot_context, 0, sizeof(PB_PROT_CONTEXT_TYPE));

    pb_cfg_proc_init();
    pb_prot_proc_init();
    pb_prot_parse_init();
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
void pb_prot_main(void *pvParameters)
{
    pb_prot_init();

    os_set_task_init(OS_TASK_ITEM_PB_PROT);
    os_wait_task_init_sync();

    pb_prot_function_init(true);

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        //need some blocking function in case of wasting cpu
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_prot_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Recv msg %d", p_pb_msg->msgID);
            switch (p_pb_msg->msgID)
            {
                case PB_MSG_PROT_ANALYSE_DATA:
                {
                    pb_prot_analyse_data();
                    break;
                }
                case PB_MSG_PROT_SEND_HBP:
                {
                    pb_prot_send_hbp();
                    break;
                }
                case PB_MSG_PROT_SEND_RSP:
                {
                    pb_prot_send_rsp_hdlr(p_pb_msg);
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_PBPROT, "Bad msg %d", p_pb_msg->msgID);
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

