/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ota_network.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   over the air network functions
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

#include "os_middleware.h"
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_ota_network.h"
#include "pb_cfg_proc.h"
#include "pb_prot_proc.h"
#include "m26.h"
#include "w5500.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_OTA_NETWORK_SENDQUE_MAXSIZE 30
#define PB_OTA_NETWORK_SENDDATA_MAXSIZE PB_PROT_RSP_BUFF_SIZE//(512 + 28)

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_DS_LIST_QUEUE_TYPE pb_ota_net_sendque;
static uint8 pb_ota_csq_rssi;
static uint8 pb_ota_csq_ber;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_ota_network_context_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_network_context_init(void)
{
    memset(&pb_ota_net_sendque, 0, sizeof(pb_ota_net_sendque));
    pb_ota_csq_rssi = 0xFF;
    pb_ota_csq_ber = 0xFF;
}

/******************************************************************************
* Function    : pb_ota_network_send_que_size
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get send que size
******************************************************************************/
uint16 pb_ota_network_send_que_size(void)
{
    return os_ds_list_que_size(&pb_ota_net_sendque);
}

/******************************************************************************
* Function    : pb_ota_network_send_que_append
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : append data to que tail
******************************************************************************/
bool pb_ota_network_send_que_append(const uint8* data, uint16 len)
{
    return os_ds_list_que_append(&pb_ota_net_sendque, PB_OTA_NETWORK_SENDQUE_MAXSIZE, data, PB_OTA_NETWORK_SENDDATA_MAXSIZE, len);
}

/******************************************************************************
* Function    : pb_ota_network_send_que_remove_head
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : remove header data
******************************************************************************/
void pb_ota_network_send_que_remove_head(void)
{
    os_ds_list_que_remove_head(&pb_ota_net_sendque);
}

/******************************************************************************
* Function    : pb_ota_network_que_head_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get header data
******************************************************************************/
bool pb_ota_network_que_head_data(uint8 **pdata, uint16 *len)
{
    return os_ds_list_que_head_data(&pb_ota_net_sendque, pdata, len);
}

/******************************************************************************
* Function    : pb_ota_network_hw_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init network hardware
******************************************************************************/
void pb_ota_network_hw_init(uint8 devType)
{
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            devM26.init((HAL_USART_TYPE*)&MODEM_2G_COM, MODEM_2G_COM_BAUDRATE);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            devW5500.init((HAL_SPI_TYPE*)&ETH_COM);
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_ota_network_hw_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network hardware reset
******************************************************************************/
void pb_ota_network_hw_reset(uint8 devType)
{
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            devM26.reset();
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            devW5500.reset();
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_ota_network_available
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_ota_network_available(uint8 devType)
{
    bool ret = false;
    
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            ret = devM26.available();
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            ret = devW5500.available();
            break;
        }
        default:
        {
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_ota_network_check_net_stat
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check net state
******************************************************************************/
bool pb_ota_network_check_net_stat(uint8 devType)
{
    static uint8 badNetCnt = 0;
    bool ret = false;
    
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            ret = devM26.isConnected();
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            ret = devW5500.isConnected();
            break;
        }
        default:
        {
            break;
        }
    }

    //Added a debounce to filter the wrong state
    if (ret == false)
    {
        badNetCnt++;
        if (badNetCnt < 2)
        {
            ret = true;
        }
        OS_DBG_ERR(DBG_MOD_PBOTA, "Bad net filter cnt[%d], ret[%d]", badNetCnt, ret);
    }
    else
    {
        badNetCnt = 0;
    }

    return ret;
}

/******************************************************************************
* Function    : pb_ota_network_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network configuration
******************************************************************************/
bool pb_ota_network_config(uint8 devType)
{
    bool ret = false;
    
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            PB_CFG_APC *pApc = &(pb_cfg_proc_get_cmd()->apc);
            ret = devM26.config((char*)pApc->apn, (char*)pApc->usr, (char*)pApc->pass);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            ret = devW5500.config();
            break;
        }
        default:
        {
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_ota_network_modal_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get netowrk modal info
******************************************************************************/
bool pb_ota_network_modal_info(uint8 devType)
{
    bool ret = false;
    
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            PB_PROT_RSP_GSMINFO_PARAM gsmInfo;
            memset(&gsmInfo, 0, sizeof(gsmInfo));
            devM26.getModulInfo((char*)gsmInfo.gsmModule, PB_GSM_MODULE_LEN,
                                           (char*)gsmInfo.imei, PB_GSM_IMEI_LEN,
                                           (char*)gsmInfo.imsi, PB_GSM_IMSI_LEN,
                                           (char*)gsmInfo.iccid, PB_GSM_ICCID_LEN);
            pb_prot_proc_set_dev_gsm_info(&gsmInfo);
            ret = true;
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "Eth not support modal info");
            break;
        }
        default:
        {
            break;
        }
    }

    return ret;}

/******************************************************************************
* Function    : pb_ota_network_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network connect to server
******************************************************************************/
bool pb_ota_network_connect(uint8 devType, const char *domainName, uint16 port)
{
    bool ret = false;

    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            ret = devM26.connect(domainName, port);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            ret = devW5500.connect(domainName, port);
            break;
        }
        default:
        {
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_ota_network_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network disconnect from server
******************************************************************************/
void pb_ota_network_disconnect(uint8 devType)
{
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            devM26.disconnect();
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            devW5500.disconnect();
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_ota_network_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : network send data to server
******************************************************************************/
uint16 pb_ota_network_send(uint8 devType, uint8 *data, uint16 size)
{
    uint16 realSend = 0;

    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            realSend = devM26.send(data, size);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            realSend = devW5500.send(data, size);
            break;
        }
        default:
        {
            break;
        }
    }

    return realSend;
}

/******************************************************************************
* Function    : pb_ota_network_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv data from server
******************************************************************************/
uint16 pb_ota_network_recv(uint8 devType, uint8 *data, uint16 maxLen)
{
    uint16 realRead = 0;

    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            realRead = devM26.recv(data, maxLen);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            realRead = devW5500.recv(data, maxLen);
            break;
        }
        default:
        {
            break;
        }
    }

    return realRead;
}

/******************************************************************************
* Function    : pb_ota_network_get_ftp_client
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void *pb_ota_network_get_ftp_client(uint8 devType)
{
    void *ftpClient = NULL;
    
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            ftpClient = (void*)devM26.ftp;
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            ftpClient = (void*)devW5500.ftp;
            break;
        }
        default:
        {
            break;
        }
    }

    return ftpClient;
}

/******************************************************************************
* Function    : pb_ota_network_update_csq
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_network_update_csq(uint8 devType)
{
    switch (devType)
    {
        case PB_OTA_NET_DEV_GPRS:
        {
            devM26.getCsq(&pb_ota_csq_rssi, &pb_ota_csq_ber);
            break;
        }
        case PB_OTA_NET_DEV_ETH:
        {
            pb_ota_csq_rssi = 0xFF;
            pb_ota_csq_ber = 0xFF;
            break;
        }
        default:
        {
            break;
        }
    }
    
    OS_DBG_TRACE(DBG_MOD_PBOTA, DBG_INFO, "RSSI[%d], BER[%d]", pb_ota_csq_rssi, pb_ota_csq_ber);
}

/******************************************************************************
* Function    : pb_util_get_csq_rssi
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get current GSM rssi (0 -31 | 99)
******************************************************************************/
uint8 pb_ota_network_get_csq_rssi(void)
{
    return pb_ota_csq_rssi;
}

/******************************************************************************
* Function    : pb_ota_network_set_csq_rssi
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_network_set_csq_rssi(uint8 rssi)
{
    pb_ota_csq_rssi = rssi;
}

/******************************************************************************
* Function    : pb_util_get_csq_ber
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get current GSM ber (0 - 7 | 99)
******************************************************************************/
uint8 pb_ota_network_get_csq_ber(void)
{
    return pb_ota_csq_ber;
}

/******************************************************************************
* Function    : pb_ota_network_set_csq_ber
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ota_network_set_csq_ber(uint8 ber)
{
    pb_ota_csq_ber = ber;
}

