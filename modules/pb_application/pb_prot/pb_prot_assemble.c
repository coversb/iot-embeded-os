/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_assemble.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   assemble protocol message and ack
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
#include <string.h>
#include "hal_board.h"
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_cfg_proc.h"
#include "pb_util.h"
#include "pb_fota.h"
#include "pb_prot_proc.h"
#include "pb_prot_main.h"
#include "pb_prot_type.h"
#include "pb_prot_assemble.h"
#include "pb_crypto.h"
#include "pb_ota_network.h"
#include "pb_io_main.h"
#include "pb_order_hotp.h"
#include "pb_order_main.h"

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint8 encryptBuff[PB_PROT_CIPHER_BUFFSIZE];

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_prot_assemble_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble u8 to buff
******************************************************************************/
static uint16 pb_prot_assemble_u8(uint8 *buff, uint8 u8)
{
    buff[0] = u8;
    
    return 1;
}

/******************************************************************************
* Function    : pb_prot_assemble_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble u16 to buff
******************************************************************************/
static uint16 pb_prot_assemble_u16(uint8 *buff, uint16 u16)
{
    u16_u8_union u16_u8;
    
    u16_u8.num = u16;

    #if (PB_PROT_NETWORK_BYTE_ORDER == 1)
    buff[0] = u16_u8.c[1];
    buff[1] = u16_u8.c[0];
    #else
    buff[0] = u16_u8.c[0];
    buff[1] = u16_u8.c[1];
    #endif /*PB_PROT_NETWORK_BYTE_ORDER*/

    return 2;
}

/******************************************************************************
* Function    : pb_prot_assemble_u32
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble u32 to buff
******************************************************************************/
static uint16 pb_prot_assemble_u32(uint8 *buff, uint32 u32)
{
    u32_u8_union u32_u8;
    
    u32_u8.num = u32;

    #if (PB_PROT_NETWORK_BYTE_ORDER == 1)
    buff[0] = u32_u8.c[3];
    buff[1] = u32_u8.c[2];
    buff[2] = u32_u8.c[1];
    buff[3] = u32_u8.c[0];
    #else
    buff[0] = u32_u8.c[0];
    buff[1] = u32_u8.c[1];
    buff[2] = u32_u8.c[2];
    buff[3] = u32_u8.c[3];
    #endif /*PB_PROT_NETWORK_BYTE_ORDER*/

    return 4;
}

/******************************************************************************
* Function    : pb_prot_assemble_str
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble string to u8
******************************************************************************/
static uint16 pb_prot_assemble_str(uint8 *buff, uint8 *str)
{
    strcpy((char*)buff, (char*)str);
    
    return (uint16)strlen((char*)str);
}

/******************************************************************************
* Function    : pb_prot_assemble_u8_array
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_u8_array(uint8 *buff, uint8 *src, uint16 len)
{
    memcpy(buff, src, len);
    
    return len;
}

/******************************************************************************
* Function    : pb_prot_assemble_header
* 
* Author      : Chen Hao
* 
* Parameters  : message buff pointer
* 
* Return      : assemble length
* 
* Description : assemble header flag "+PB"
******************************************************************************/
static uint16 pb_prot_assemble_header_flag(uint8 *buff)
{
    pb_prot_assemble_u8(&buff[0], PB_PROT_HEADFLAG_PLUSE);
    pb_prot_assemble_u8(&buff[1], PB_PROT_HEADFLAG_P);
    pb_prot_assemble_u8(&buff[2], PB_PROT_HEADFLAG_B);
    return 3;
}

/******************************************************************************
* Function    : pb_prot_assemble_device_type
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble device type
******************************************************************************/
static uint16 pb_prot_assemble_device_type(uint8 *buff)
{
    return pb_prot_assemble_u8(buff, pb_cfg_proc_get_dev_type());
}

/******************************************************************************
* Function    : pb_prot_assemble_protocol_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble protocol version 
******************************************************************************/
static uint16 pb_prot_assemble_protocol_version(uint8 *buff)
{
    return pb_prot_assemble_u16(buff, (uint16)PB_PROTOCOL_VERSION);
}

/******************************************************************************
* Function    : pb_prot_assemble_uid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble uid
******************************************************************************/
static uint16 pb_prot_assemble_uid(uint8 *buff)
{
    const uint8 *uid = pb_cfg_proc_get_uid();
    for (uint8 idx = 0; idx < PB_UID_LEN; ++idx) 
    {
        buff[idx] = uid[idx];
    }
    return PB_UID_LEN;
}

/******************************************************************************
* Function    : pb_prot_assemble_length
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble length
******************************************************************************/
static uint16 pb_prot_assemble_length(uint8 *buff, uint16 len)
{
    return pb_prot_assemble_u16(buff, len);
}

/******************************************************************************
* Function    : pb_prot_assemble_header_crc
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble header crc
******************************************************************************/
static uint16 pb_prot_assemble_header_crc(uint8 *buff, uint16 len)
{
    return pb_prot_assemble_u16(buff, len);
}

/******************************************************************************
* Function    : pb_prot_assemble_timestamp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble timestamp
******************************************************************************/
static uint16 pb_prot_assemble_timestamp(uint8 *buff, uint32 timestamp)
{
    return pb_prot_assemble_u32(buff, timestamp);
}

/******************************************************************************
* Function    : pb_prot_assemble_counter_number
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble report counter number
******************************************************************************/
static uint16 pb_prot_assemble_counter_number(uint8 *buff)
{
    return pb_prot_assemble_u16(buff, pb_prot_proc_get_report_sn());
}

/******************************************************************************
* Function    : pb_prot_assemble_message_type
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble message type
******************************************************************************/
static uint16 pb_prot_assemble_message_type(uint8 *buff, uint8 msgType)
{
    return pb_prot_assemble_u8(buff, msgType);
}

/******************************************************************************
* Function    : pb_prot_assemble_message_sub_type
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble message sub type
******************************************************************************/
static uint16 pb_prot_assemble_message_sub_type(uint8 *buff, uint8 subMsgType)
{
    return pb_prot_assemble_u8(buff, subMsgType);
}

/******************************************************************************
* Function    : pb_prot_assemble_check_code
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble check code
******************************************************************************/
static uint16 pb_prot_assemble_check_code(uint8 *buff, uint16 checkCode)
{
    return pb_prot_assemble_u16(buff, checkCode);
}

/******************************************************************************
* Function    : pb_prot_assemble_tail
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble tail flag
******************************************************************************/
static uint16 pb_prot_assemble_tail(uint8 *buff)
{
    uint8* pbuff = buff;

    pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_TAILFLAG_R);
    pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_TAILFLAG_N);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_serial_number
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble serial number
******************************************************************************/
static uint16 pb_prot_assemble_serial_number(uint8 *buff, uint16 sn)
{
    return pb_prot_assemble_u16(buff, sn);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_inf
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble information message for inf report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_inf(uint8 *buff, uint8 subMsgType)
{
    uint8* pbuff = buff;

    /*Status*/
    switch (subMsgType)
    {
        case PB_PROT_RSP_PNE:
        {
            pbuff += pb_prot_assemble_u8(pbuff, hal_board_get_boot_type());
            break;
        }
        case PB_PROT_RSP_PFE:
        case PB_PROT_RSP_INF:
        case PB_PROT_RSP_DOG:
        case PB_PROT_RSP_OMC:
        default:
        {
            pbuff += pb_prot_assemble_u8(pbuff, 0xFF);
            break;
        }
    }

    /*CSQ RSSI*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_ota_network_get_csq_rssi());

    /*CSQ BER*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_ota_network_get_csq_ber());

    /*Input state*/
    pbuff += pb_prot_assemble_u32(pbuff, pb_io_input_mask());

    /*Output state*/
    pbuff += pb_prot_assemble_u32(pbuff, pb_io_output_mask());
    
    /*Air conditioner state*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_io_aircon_state());

    /*Smoke level*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_io_smoke_level());
    
    /*Indoor temperature*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_util_get_indoor_temperature());

    /*Indoor humidity*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_util_get_indoor_humidity());

    /*Indoor PM2.5*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_util_get_indoor_pm25());

    /*Backup battery voltage*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_util_get_bak_bat_voltage());

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_pse
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble information message for power event report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_pse(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 pwrEvent = *((uint8*)param);

    /*Power event type*/
    pbuff += pb_prot_assemble_u8(pbuff, pwrEvent);

    /*CSQ RSSI*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_ota_network_get_csq_rssi());

    /*CSQ BER*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_ota_network_get_csq_ber());

    /*Input state*/
    pbuff += pb_prot_assemble_u32(pbuff, pb_io_input_mask());

    /*Output state*/
    pbuff += pb_prot_assemble_u32(pbuff, pb_io_output_mask());
    
    /*Air conditioner state*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_io_aircon_state());

    /*Smoke level*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_io_smoke_level());
    
    /*Indoor temperature*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_util_get_indoor_temperature());

    /*Indoor humidity*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_util_get_indoor_humidity());

    /*Indoor PM2.5*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_util_get_indoor_pm25());

    /*Backup battery voltage*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_util_get_bak_bat_voltage());

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_uie
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble user input data report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_uie(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_UIE_PARAM* pUieParam = (PB_PROT_RSP_UIE_PARAM*)param;

    /*User input type*/
    pbuff += pb_prot_assemble_u8(pbuff, pUieParam->type);

    /*Input data length*/
    pbuff += pb_prot_assemble_u8(pbuff, strlen(((char*)(pUieParam->data))));

    /*Input data*/
    pbuff += pb_prot_assemble_str(pbuff, pUieParam->data);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_dse
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble door status report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_dse(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 operationType = *((uint8*)param);

    /*Door status*/
    pbuff += pb_prot_assemble_u8(pbuff, pb_io_door_status());

    /*Operation type*/
    pbuff += pb_prot_assemble_u8(pbuff, operationType);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_pce
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_pce(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_PCE_PARAM *pParam = (PB_PROT_RSP_PCE_PARAM*)param;

    /*Detection method*/
    pbuff += pb_prot_assemble_u8(pbuff, pParam->method);

    /*Detection position*/
    pbuff += pb_prot_assemble_u8(pbuff, pParam->position);

    /*Detection type*/
    pbuff += pb_prot_assemble_u8(pbuff, pParam->type);

    /*Detected number*/
    pbuff += pb_prot_assemble_u8(pbuff, pParam->curNum);

    /*Total person number*/
    pbuff += pb_prot_assemble_u16(pbuff, pParam->totalNum);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_coe
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_coe(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_COE_PARAM *pParam = (PB_PROT_RSP_COE_PARAM*)param;

    /*Operation type*/
    pbuff += pb_prot_assemble_u16(pbuff, pParam->type);

    /*Operation ID*/
    pbuff += pb_prot_assemble_u32(pbuff, pParam->operationID);

    /*Consumer ID*/
    pbuff += pb_prot_assemble_u32(pbuff, pParam->consumerID);

    /*Operation info length*/
    pbuff += pb_prot_assemble_u8(pbuff, strlen((char*)pParam->info));

    /*Operation info*/
    pbuff += pb_prot_assemble_str(pbuff, pParam->info);
    
    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_sae
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble information message for sensor alarm event report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_sae(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_SAE_PARAM *pSaeParam = (PB_PROT_RSP_SAE_PARAM *)param;

    /*Sensor alarm type*/
    pbuff += pb_prot_assemble_u8(pbuff, pSaeParam->alarmType);

    /*Sensor level*/
    pbuff += pb_prot_assemble_u8(pbuff, pSaeParam->alarmLv);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_mue
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_mue(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 eventType = *((uint8*)param);

    /*Module*/
    pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_MUE_MODULE_SOUND);

    /*Event type*/
    pbuff += pb_prot_assemble_u8(pbuff, eventType);

    /*Service status*/
    pbuff += pb_prot_assemble_u8(pbuff, (uint8)pb_order_operation_state());

    /*Local order number*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_order_number());

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_rto
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble information message for real-time operation report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_rto(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 cmdType = *((uint8*)param);

    /*Command type*/
    pbuff += pb_prot_assemble_u8(pbuff, cmdType);

    /*Firmware version*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_fota_get_firmware_version());

    /*Bootloader version*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_cfg_proc_get_hardware_version());

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_opo
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_opo(uint8 *buff)
{
    uint8* pbuff = buff;

    PB_ORDER_OFFLINE_BUFF *pOfflineOrder = pb_order_hotp_offline_order_buff();

    /*Order number*/
    pbuff += pb_prot_assemble_u8(pbuff, pOfflineOrder->size);

    for (uint8 idx = 0; idx < pOfflineOrder->size; ++idx)
    {
        /*Input datetime*/
        pbuff += pb_prot_assemble_u32(pbuff, pOfflineOrder->order[idx].inputTime);

        /*Start datetime*/
        pbuff += pb_prot_assemble_u32(pbuff, pOfflineOrder->order[idx].startTime);

        /*Password datetime*/
        pbuff += pb_prot_assemble_u32(pbuff, pOfflineOrder->order[idx].password);
    }
    //set number to 0
    pOfflineOrder->size = 0;
   
    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_cfg_item
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_cfg_item(uint8 *buff, uint8 idx)
{
    uint8* pbuff = buff;

    switch (idx)
    {
        case PB_PROT_CMD_APC:
        {
            PB_CFG_APC *cfgApc = &(pb_cfg_proc_get_cmd()->apc);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Filed length*/
            uint16 filedLen = 0;
            uint8 apnLen = strlen((char*)cfgApc->apn);
            uint8 apnUsrLen = strlen((char*)cfgApc->usr);
            uint8 apnPassLen = strlen((char*)cfgApc->pass);
            filedLen |= (apnLen & 0x003F);
            filedLen |= ((apnUsrLen << 6) & 0x07C0);
            filedLen |= ((apnPassLen << 11) & 0xF800);            
            pbuff += pb_prot_assemble_u16(pbuff, filedLen);
            /*APN*/
            pbuff += pb_prot_assemble_str(pbuff, cfgApc->apn);
            /*APN user name*/
            pbuff += pb_prot_assemble_str(pbuff, cfgApc->usr);
            /*APN password*/
            pbuff += pb_prot_assemble_str(pbuff, cfgApc->pass);
            /*Main DNS server*/
            pbuff += pb_prot_assemble_u8_array(pbuff, cfgApc->mainDNS, PB_APN_DNS_LEN);
            /*Backup DNS server*/
            pbuff += pb_prot_assemble_u8_array(pbuff, cfgApc->bkDNS, PB_APN_DNS_LEN);
            break;
        }
        case PB_PROT_CMD_SER:
        {
            PB_CFG_SER *cfgSer = &(pb_cfg_proc_get_cmd()->ser);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Field length*/
            uint8 filedLen[3];
            uint8 mainAddrLen = strlen((char*)cfgSer->mainServer);
            uint8 bakAddrLen = strlen((char*)cfgSer->bakServer);
            uint8 smsLen = strlen((char*)cfgSer->smsGateway);
            filedLen[2] = mainAddrLen;
            filedLen[2] |= ((bakAddrLen & 0x03) << 6);
            filedLen[1] = ((bakAddrLen & 0x3C) >> 2);
            filedLen[1] |= ((smsLen & 0x0F) << 4);
            filedLen[0] = ((smsLen & 0x10) >> 4);
            pbuff += pb_prot_assemble_u8_array(pbuff, filedLen, 3);
            /*Report mode*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSer->mode);
            /*Main server domain name*/
            pbuff += pb_prot_assemble_str(pbuff, cfgSer->mainServer);
            /*Main server port*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgSer->mainPort);
            /*Backup server domain*/
            pbuff += pb_prot_assemble_str(pbuff, cfgSer->bakServer);
            /*Backup server port*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgSer->bakPort);
            /*SMS gateway*/
            pbuff += pb_prot_assemble_str(pbuff, cfgSer->smsGateway);
            /*Heartbeat interval*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSer->hbpInterval);
            /*Max random time*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgSer->randomTime);
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            PB_CFG_CFG *cfgCfg = &(pb_cfg_proc_get_cmd()->cfg);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Event mask*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgCfg->eventMask);
            /*Info report interval*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgCfg->infInterval);
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            PB_CFG_TMA *cfgTma = &(pb_cfg_proc_get_cmd()->tma);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Time adjust mask*/
            uint16 adjust = 0;
            adjust |= (cfgTma->sign & 0x0001);
            adjust |= ((cfgTma->hour  << 1) & 0x003E);
            adjust |= ((cfgTma->minute << 6) & 0x0FC0);
            adjust |= ((cfgTma->daylightSaving << 12) & 0x1000);
            adjust |= ((cfgTma->mode << 13) & 0xE000);
            pbuff += pb_prot_assemble_u16(pbuff, adjust);
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            PB_CFG_DOG *cfgDog = &(pb_cfg_proc_get_cmd()->dog);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Mode*/
            uint8 mode = 0;
            mode |= (cfgDog->mode & 0x03);
            mode |= ((cfgDog->report << 2) & 0x04);
            mode |= ((cfgDog->interval << 3) & 0xF8);
            pbuff += pb_prot_assemble_u8(pbuff, mode);
            /*Reboot Time*/
            uint16 rebootTime = 0;
            rebootTime |= (cfgDog->rstMinute & 0x003F);
            rebootTime |= ((cfgDog->rstHour << 6) & 0x07C0);
            #if 0 //useless, comment it
            rebootTime |= ((0 << 11) & 0xF800);
            #endif
            pbuff += pb_prot_assemble_u16(pbuff, rebootTime);

            /*Max random time*/
            pbuff += pb_prot_assemble_u16(pbuff, cfgDog->randomDuration);
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            PB_CFG_ACO *cfgAco = &(pb_cfg_proc_get_cmd()->aco);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Mode*/
            uint8 mode = 0;
            mode |= (cfgAco->pwrMode & 0x03);
            mode |= ((cfgAco->workMode << 2) & 0x0C);
            mode |= ((cfgAco->windLevel << 4) & 0x30);
            #if 0 //useless, comment it
            mode |= ((0 << 6) & 0xC0);
            #endif
            pbuff += pb_prot_assemble_u8(pbuff, mode);
            /*Interval*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgAco->interval);
            /*Duration*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgAco->duration);
            /*Temperature*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgAco->temperature);
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            PB_CFG_OMC *cfgOmc = &(pb_cfg_proc_get_cmd()->omc);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Idle output*/
            pbuff += pb_prot_assemble_u32(pbuff, cfgOmc->idleOutput);
            /*In-service output*/
            pbuff += pb_prot_assemble_u32(pbuff, cfgOmc->inServiceOutput);
            /*Mode*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgOmc->mode);
            /*Valid time*/
            uint32 validTime = 0;
            validTime |= (cfgOmc->startHour & 0x0000001F);
            validTime |= ((cfgOmc->startMin << 5) & 0x000007E0);
            validTime |= ((cfgOmc->stopHour << 11) & 0x0000F800);
            validTime |= ((cfgOmc->stopMin << 16) & 0x001F0000);
            #if 0 //useless, comment it
            validTime |= ((0 << 21) & 0xFFE00000);
            #endif
            pbuff += pb_prot_assemble_u32(pbuff, validTime);
            /*Valid time idle output*/
            pbuff += pb_prot_assemble_u32(pbuff, cfgOmc->validIdleOutput);
            /*Valid time in-service output*/
            pbuff += pb_prot_assemble_u32(pbuff, cfgOmc->validInServiceOutput);
            break;
        }
        case PB_PROT_CMD_DOA:
        {
            PB_CFG_DOA *cfgDoa = &(pb_cfg_proc_get_cmd()->doa);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Mode*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgDoa->mode);
            /*Trigger type*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgDoa->triggerType);
            /*Duration*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgDoa->duration);
            /*Send Interval*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgDoa->interval);
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            PB_CFG_SMA *cfgSma = &(pb_cfg_proc_get_cmd()->sma);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Mode*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSma->mode);
            /*Threshold*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSma->threshold);
            /*Duration*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSma->duration);
            /*Send Interval*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgSma->interval);
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            PB_CFG_MUO *cfgMuo = &(pb_cfg_proc_get_cmd()->muo);

            /*Message type*/
            pbuff += pb_prot_assemble_u8(pbuff, PB_PROT_CMD);
            /*Message sub-type*/
            pbuff += pb_prot_assemble_u8(pbuff, idx);
            /*Mode*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgMuo->autoBGM);
            /*Volume*/
            pbuff += pb_prot_assemble_u8(pbuff, cfgMuo->volume);
            break;
        }
        default:break;
    }

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_cfg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_cfg(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 subCmdType = *((uint8*)param);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "CFG RSP subcmd[%d]", subCmdType);

    if (subCmdType == 0)
    {
        for (uint8 idx = PB_PROT_CMD_BEGIN; idx < PB_PROT_CMD_END; ++idx)
        {
            if (idx == PB_PROT_CMD_SEC)
            {
                continue;//skip key cfg
            }
            pbuff += pb_prot_assemble_rsp_cfg_item(pbuff, idx);
        }
    }

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_loc
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble information message for location report
******************************************************************************/
static uint16 pb_prot_assemble_rsp_loc(uint8 *buff)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_LOC_PARAM *pLoc = pb_prot_proc_get_dev_location();
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "LON[%s], LAT[%s]",
                                pLoc->longitude, pLoc->latitude);
    
    /*Fix type*/
    pbuff += pb_prot_assemble_u8(pbuff, pLoc->fixType);

    /*Longitude*/
    pbuff += pb_prot_assemble_str(pbuff, pLoc->longitude);

    /*Latitude*/
    pbuff += pb_prot_assemble_str(pbuff, pLoc->latitude);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_gsm_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_gsm_info(uint8 *buff)
{
    uint8* pbuff = buff;
    PB_PROT_RSP_GSMINFO_PARAM *pGsm = pb_prot_proc_get_dev_gsm_info();
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "GSM[%s], IMEI[%s], IMSI[%s], ICCID[%s]",
                            pGsm->gsmModule, pGsm->imei,
                            pGsm->imsi, pGsm->iccid);
    
    /*GSM module*/
    pbuff += pb_prot_assemble_u8_array(pbuff, pGsm->gsmModule, PB_GSM_MODULE_LEN);

    /*IMEI*/
    pbuff += pb_prot_assemble_u8_array(pbuff, pGsm->imei, PB_GSM_IMEI_LEN);

    /*IMSI*/
    pbuff += pb_prot_assemble_u8_array(pbuff, pGsm->imsi, PB_GSM_IMSI_LEN);
    
    /*ICCID*/
    pbuff += pb_prot_assemble_u8_array(pbuff, pGsm->iccid, PB_GSM_ICCID_LEN);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_dbi
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : assemble device basic information
******************************************************************************/
static uint16 pb_prot_assemble_rsp_dbi(uint8 *buff)
{
    uint8* pbuff = buff;

    uint32 imageUpTimestamp = 0;
    uint32 imageRunTimes = 0;
    uint32 longitude = 0;
    uint32 latitude = 0;
    PB_PROT_RSP_GSMINFO_PARAM *gsm;
    PB_PROT_RSP_LOC_PARAM *loc;
    
    gsm = pb_prot_proc_get_dev_gsm_info();
    loc = pb_prot_proc_get_dev_location();
    pb_fota_get_firmware_info(&imageUpTimestamp, &imageRunTimes);

    longitude = pb_util_decimal_string_to_int((char*)loc->longitude, 10000000);
    latitude = pb_util_decimal_string_to_int((char*)loc->latitude, 10000000);
    
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, 
                                "DBI upTime[%u], runTimes[%u], "
                                "GSM[%s], IMEI[%s], IMSI[%s], ICCID[%s], "
                                "FIX[%02X], FIX_Time[%u], LONG[%u], LAT[%u]",
                                imageUpTimestamp, imageRunTimes,
                                gsm->gsmModule, gsm->imei, gsm->imsi, gsm->iccid,
                                loc->fixType, loc->timestamp, longitude, latitude);

    /*Hardware type*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_cfg_proc_get_hardware_version());

    /*BL version*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_fota_get_bl_version());

    /*Firmware version*/
    pbuff += pb_prot_assemble_u16(pbuff, pb_fota_get_firmware_version());

    /*Firmware upgrade timestamp*/
    pbuff += pb_prot_assemble_u32(pbuff, imageUpTimestamp);

    /*Firmware run times*/
    pbuff += pb_prot_assemble_u32(pbuff, imageRunTimes);

    /*SN*/
    pbuff += pb_prot_assemble_u8_array(pbuff, (uint8*)pb_cfg_proc_get_sn(), PB_SN_LEN);

    /*MAC*/
    pbuff += pb_prot_assemble_u8_array(pbuff, (uint8*)pb_cfg_proc_get_mac(), PB_MAC_LEN);

    /*GSM module*/
    pbuff += pb_prot_assemble_u8_array(pbuff, gsm->gsmModule, PB_GSM_MODULE_LEN);

    /*IMEI*/
    pbuff += pb_prot_assemble_u8_array(pbuff, gsm->imei, PB_GSM_IMEI_LEN);

    /*IMSI*/
    pbuff += pb_prot_assemble_u8_array(pbuff, gsm->imsi, PB_GSM_IMSI_LEN);
    
    /*ICCID*/
    pbuff += pb_prot_assemble_u8_array(pbuff, gsm->iccid, PB_GSM_ICCID_LEN);

    /*FIX type*/
    pbuff += pb_prot_assemble_u8(pbuff, loc->fixType);

    /*Locate timestamp*/
    pbuff += pb_prot_assemble_u32(pbuff, loc->timestamp);

    /*Longitude*/
    pbuff += pb_prot_assemble_u32(pbuff, longitude);
    
    /*Latitude*/
    pbuff += pb_prot_assemble_u32(pbuff, latitude);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_dbg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_dbg(uint8 *buff, void *param)
{
    uint8* pbuff = buff;
    uint8 *pDbgInfo = (uint8 *)param;
    uint16 dbgInfoLen = MIN_VALUE(PB_DBG_MAXSIZE, strlen((char*)pDbgInfo));
    //uint8 totalNum = CEIL_VALUE(dbgInfoLen, PB_PROT_RSP_DBG_MAXSIZE);
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Len[%d], Assembled len[%d], INFO[%s]", 
                             strlen((char*)pDbgInfo), dbgInfoLen, pDbgInfo);

    /*Total number*/
    pbuff += pb_prot_assemble_u8(pbuff, 1);

    /*Currnet report index*/
    pbuff += pb_prot_assemble_u8(pbuff, 1);

    /*Information length*/
    pbuff += pb_prot_assemble_u16(pbuff, dbgInfoLen);

    /*Info*/
    pbuff += pb_prot_assemble_u8_array(pbuff, pDbgInfo, dbgInfoLen);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp_fota
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_rsp_fota(uint8 *buff, void *param)
{
    uint8* pbuff = buff;

    PB_PROT_RSP_FOTA_PARAM *fotaParam = (PB_PROT_RSP_FOTA_PARAM*)param;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "FOTA[%02X], retry[%d]",
                                fotaParam->status, fotaParam->cnt);
    /*Status code*/
    pbuff += pb_prot_assemble_u8(pbuff, fotaParam->status);

    /*Retry times*/
    pbuff += pb_prot_assemble_u8(pbuff, fotaParam->cnt);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_header
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_prot_assemble_header(uint8 *buff)
{
    uint8* pbuff = buff;

    /*Header*/
    pbuff += pb_prot_assemble_header_flag(pbuff);

    /*Device type*/
    pbuff += pb_prot_assemble_device_type(pbuff);

    /*Protocol version*/
    pbuff += pb_prot_assemble_protocol_version(pbuff);

    /*UID*/
    pbuff += pb_prot_assemble_uid(pbuff);

    return (pbuff - buff);
}

/******************************************************************************
* Function    : pb_prot_assemble_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : assembled report package length
* 
* Description : assemble RSP package
******************************************************************************/
uint16 pb_prot_assemble_rsp(PB_PROT_RSP_PACK_TYPE *rspPack)
{
    uint8 *pbuff = NULL;
    uint8 *plen = NULL;
    uint8 *pheadCRC = NULL;

    if (rspPack->msgSubType < PB_PROT_RSP_BEGIN
        || rspPack->msgSubType > PB_PROT_MSG_RSP_END)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad msg subtype[%d]", rspPack->msgSubType);
        return 0;
    }

    pbuff = rspPack->data;

    /*Header*/
    pbuff += pb_prot_assemble_header(pbuff);

    /*Length*/
    plen = pbuff;
    pbuff += pb_prot_assemble_length(pbuff, 0);

    /*Head CRC*/
    pheadCRC = pbuff;
    pbuff += pb_prot_assemble_header_crc(pbuff, 0);    

    memset(encryptBuff, 0, sizeof(encryptBuff));
    uint8 *pCipher = encryptBuff;

    /*Send time*/
    pCipher += pb_prot_assemble_timestamp(pCipher, pb_util_get_timestamp());

    /*Counter number*/
    pCipher += pb_prot_assemble_counter_number(pCipher);

    /*Message type*/
    pCipher += pb_prot_assemble_message_type(pCipher, PB_PROT_RSP);

    /*Message sub-type*/
    pCipher += pb_prot_assemble_message_sub_type(pCipher, rspPack->msgSubType);

    switch (rspPack->msgSubType)
    {
        case PB_PROT_RSP_PNE:
        case PB_PROT_RSP_PFE:
        case PB_PROT_RSP_INF:
        case PB_PROT_RSP_DOG:
        case PB_PROT_RSP_OMC:
        {
            pCipher += pb_prot_assemble_rsp_inf(pCipher, rspPack->msgSubType);
            break;
        }
        case PB_PROT_RSP_PSE:
        {
            pCipher += pb_prot_assemble_rsp_pse(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_UIE:
        {
            pCipher += pb_prot_assemble_rsp_uie(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_DSE:
        {
            pCipher += pb_prot_assemble_rsp_dse(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_PCE:
        {
            pCipher += pb_prot_assemble_rsp_pce(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_COE:
        {
            pCipher += pb_prot_assemble_rsp_coe(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_SAE:
        {
            pCipher += pb_prot_assemble_rsp_sae(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_MUE:
        {
            pCipher += pb_prot_assemble_rsp_mue(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_RTO:
        {
            pCipher += pb_prot_assemble_rsp_rto(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_OPO:
        {
            pCipher += pb_prot_assemble_rsp_opo(pCipher);
            break;
        }
        case PB_PROT_RSP_CFG:
        {
            pCipher += pb_prot_assemble_rsp_cfg(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_LOC:
        {
            pCipher += pb_prot_assemble_rsp_loc(pCipher);
            break;
        }
        case PB_PROT_RSP_GSM:
        {
            pCipher += pb_prot_assemble_rsp_gsm_info(pCipher);
            break;
        }
        case PB_PROT_RSP_DBI:
        {
            pCipher += pb_prot_assemble_rsp_dbi(pCipher);
            break;
        }
        case PB_PROT_RSP_DBG:
        {
            pCipher += pb_prot_assemble_rsp_dbg(pCipher, rspPack->msgParam);
            break;
        }
        case PB_PROT_RSP_FOTA:
        {
            pCipher += pb_prot_assemble_rsp_fota(pCipher, rspPack->msgParam);
            break;
        }
        default:return 0;
    }

    //get encrypt data
    uint16 cipherLen = pb_encrypt(encryptBuff, pCipher - encryptBuff, pb_crypto_get_key(), pbuff);
    #if (PB_PROT_AES == 1)
    pb_prot_assemble_length(plen, cipherLen|0x4000);
    #else
    pb_prot_assemble_length(plen, cipherLen);
    #endif /*PB_PROT_AES*/
    
    //get header CRC
    uint16 headCRC = pb_util_get_crc16(rspPack->data, PB_PROT_ASSEMBLE_HEAD_CHECK_LEN);
    pb_prot_assemble_length(pheadCRC, headCRC);
    
    /*Check code*/
    uint16 checkCode = pb_util_get_crc16(pbuff, cipherLen);
    pbuff += cipherLen;
    pbuff += pb_prot_assemble_check_code(pbuff, checkCode);
    
    /*Tail*/
    pbuff += pb_prot_assemble_tail(pbuff);

    rspPack->length = pbuff - rspPack->data;

    return rspPack->length;
}

/******************************************************************************
* Function    : pb_prot_assemble_ack
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : assembled ack package length
* 
* Description : assemble ACK package
******************************************************************************/
uint16 pb_prot_assemble_ack(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame,
                                                       PB_PROT_ACK_PACK_TYPE *ackPack)
{
    uint8 *pbuff = NULL;
    uint8 *plen = NULL;
    uint8 *pheadCRC = NULL;

    pbuff = ackPack->data;

    /*Header*/
    pbuff += pb_prot_assemble_header(pbuff);

    /*Length*/
    plen = pbuff;
    pbuff += pb_prot_assemble_length(pbuff, 0);
    
    /*Head CRC*/
    pheadCRC = pbuff;
    pbuff += pb_prot_assemble_header_crc(pbuff, 0);    

    memset(encryptBuff, 0, sizeof(encryptBuff));
    uint8 *pCipher = encryptBuff;

    /*Send time*/
    pCipher += pb_prot_assemble_timestamp(pCipher, pb_util_get_timestamp());

    /*Counter number*/
    pCipher += pb_prot_assemble_counter_number(pCipher);

    /*Message type*/
    pCipher += pb_prot_assemble_message_type(pCipher, PB_PROT_ACK);

    /*Message sub-type*/
    pCipher += pb_prot_assemble_message_sub_type(pCipher, parsedFrame->msgSubType);

    /*Serial number*/
    pCipher += pb_prot_assemble_serial_number(pCipher, parsedFrame->serialNumber);

    /*RTO command ack*/
    if (parsedFrame->msgSubType == PB_PROT_CMD_RTO)
    {
        pCipher += pb_prot_assemble_u8(pCipher, parsedFrame->arg.rto.cmd);
    }

    //get encrypt data
    uint16 cipherLen = pb_encrypt(encryptBuff, pCipher - encryptBuff, pb_crypto_get_key(), pbuff);
    #if (PB_PROT_AES == 1)
    pb_prot_assemble_length(plen, cipherLen|0x4000);
    #else
    pb_prot_assemble_length(plen, cipherLen);
    #endif /*PB_PROT_AES*/

    //get header CRC
    uint16 headCRC = pb_util_get_crc16(ackPack->data, PB_PROT_ASSEMBLE_HEAD_CHECK_LEN);
    pb_prot_assemble_length(pheadCRC, headCRC);
    
    /*Check code*/
    uint16 checkCode =  pb_util_get_crc16(pbuff, cipherLen);
    pbuff += cipherLen;
    pbuff += pb_prot_assemble_check_code(pbuff, checkCode);
    
    /*Tail*/
    pbuff += pb_prot_assemble_tail(pbuff);

    ackPack->length = pbuff - ackPack->data;

    return ackPack->length;
}

/******************************************************************************
* Function    : pb_prot_assemble_hbp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : assembled heartbeat package length
* 
* Description : assemble HBP package
******************************************************************************/
uint16 pb_prot_assemble_hbp(PB_PROT_HBP_PACK_TYPE *hbpPack)
{
    uint8 *pbuff = NULL;
    uint8 *plen = NULL;
    uint8 *pheadCRC = NULL;

    pbuff = hbpPack->data;

    /*Header*/
    pbuff += pb_prot_assemble_header(pbuff);

    /*Length*/
    plen = pbuff;
    pbuff += pb_prot_assemble_length(pbuff, 0);

    /*Head CRC*/
    pheadCRC = pbuff;
    pbuff += pb_prot_assemble_header_crc(pbuff, 0);    

    memset(encryptBuff, 0, sizeof(encryptBuff));
    uint8 *pCipher = encryptBuff;

    /*Send time*/
    pCipher += pb_prot_assemble_timestamp(pCipher, pb_util_get_timestamp());

    /*Counter number*/
    pCipher += pb_prot_assemble_counter_number(pCipher);

    /*Message type*/
    pCipher += pb_prot_assemble_message_type(pCipher, PB_PROT_HBP);

    /*Message sub-type*/
    pCipher += pb_prot_assemble_message_sub_type(pCipher, PB_PROT_MSG_HBP_SUBTYPE);

    //get encrypt data
    uint16 cipherLen = pb_encrypt(encryptBuff, pCipher - encryptBuff, pb_crypto_get_key(), pbuff);
    #if (PB_PROT_AES == 1)
    pb_prot_assemble_length(plen, cipherLen|0x4000);
    #else
    pb_prot_assemble_length(plen, cipherLen);
    #endif /*PB_PROT_AES*/
    
    //get header CRC
    uint16 headCRC = pb_util_get_crc16(hbpPack->data, PB_PROT_ASSEMBLE_HEAD_CHECK_LEN);
    pb_prot_assemble_length(pheadCRC, headCRC);

    /*Check code*/
    uint16 checkCode =  pb_util_get_crc16(pbuff, cipherLen);
    pbuff += cipherLen;
    pbuff += pb_prot_assemble_check_code(pbuff, checkCode);
    
    /*Tail*/
    pbuff += pb_prot_assemble_tail(pbuff);

    hbpPack->length = pbuff - hbpPack->data;

    return hbpPack->length;
}

