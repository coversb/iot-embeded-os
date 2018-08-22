/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          kt603.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   kt603 operation
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-27      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board_config.h"
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "hm11.h"

/******************************************************************************
* Macros
******************************************************************************/
#define HM11_DEF_BAUDRATE 9600

#define HM11_ATCMD_TIMEOUT (DELAY_1_S)
#define HM11_ATCMD_WAIT_TIMEOUT (DELAY_1_S*1) // 10s for debug  //(DELAY_1_S*2)
#define HM11_RSP_MAX_LEN 128

#define HM11_RSP_SCAN_BUFF_SIZE 16
#define HM11_SCAN_PACKET_SIZE 64
#define HM11_SCAN_MAX_NUM 32

//AT+DISA? RSP format
#define HM11_SCAN_MAC_OFFSET (0)
#define HM11_SCAN_RSSI_OFFSET (7)
#define HM11_SCAN_LEN_OFFSET (8)
#define HM11_SCAN_DATA_OFFSET (9)

//RSP
#define HM11_RSP_OK "OK"
#define HM11_RSP_DISCONNECT "OK+LOST"
#define HM11_RSP_SET "OK+Set:"
#define HM11_RSP_SET0 "OK+Set:0"
#define HM11_RSP_SET1 "OK+Set:1"

#define HM11_RSP_SCAN_BEGIN "OK+DISAS"
#define HM11_RSP_SCAN_END "OK+DISCE"
#define HM11_RSP_SCAN_ITEM "OK+DISA:"

//CMD
#define HM11_CHECK_CMD "AT"
#define HM11_BAUD_CMD "AT+BAUD"
#define HM11_MODE_CMD "AT+IMME1" //don't work auto
#define HM11_NAME_CMD "AT+NAME"
#define HM11_SLAVE_CMD "AT+ROLE0"
#define HM11_MASTER_CMD "AT+ROLE1"

#define HM11_SCAN_CMD "AT+DISA?"

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static HAL_USART_TYPE *HM11_COM = NULL;
static OS_MUTEX_TYPE HM11_MUTEX = NULL;
static uint32 HM11_COM_BAUDRATE = 9600;

static uint32 HM11_BAUDRATE_TABLE[HM11_BAUDRATE_CMD_END] = 
{
    HM11_BAUDRATE_9600,
    HM11_BAUDRATE_19200,
    HM11_BAUDRATE_38400,
    HM11_BAUDRATE_57600,
    HM11_BAUDRATE_115200
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : hm11_check_timeout
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_check_timeout(uint32 start, uint32 timeout)
{
    if (os_get_tick_count() - start >= timeout)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* Function    : hm11_at_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 hm11_at_cmd(const char *at, char *rsp, uint16 rspMaxLen)
{
    uint32 sendTime = 0;
    uint16 offset = 0;
    os_mutex_lock(&HM11_MUTEX);

    if (at != NULL)
    {
        while (HM11_COM->available())
        {
            HM11_COM->read();
        }

        HM11_COM->print((char*)at);
        os_scheduler_delay(DELAY_100_MS);
        sendTime = os_get_tick_count();
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "AT[%s]", at);
    }

//wait:
    while (!HM11_COM->available())
    {
        os_scheduler_delay(DELAY_50_MS);
        if (hm11_check_timeout(sendTime, HM11_ATCMD_TIMEOUT))
        {
            offset = 0;
            goto err;
        }
    }

    while (HM11_COM->available() > 0 && (offset + 1) < rspMaxLen)
    {
        rsp[offset++] = HM11_COM->read();
    }
/*
    if (offset < HM11_RSP_MIN_LEN)//filter
    {
        offset = 0;
        goto wait;
    }
*/
err:
    os_mutex_unlock(&HM11_MUTEX);

    rsp[offset] = '\0';
    return offset;
}

/******************************************************************************
* Function    : hm11_at_cmd_check_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send at command and check respond
******************************************************************************/
static bool hm11_at_cmd_check_rsp(const char *at, const char *rsp)
{
    bool ret = false;
    char buff[HM11_RSP_MAX_LEN + 1] = {0};

    if (0 != hm11_at_cmd(at, buff, HM11_RSP_MAX_LEN))
    {
        if (NULL != strstr((char*)buff, rsp))
        {
            ret = true;
        }
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "RSP[%s]", buff);

    return ret;    
}

/******************************************************************************
* Function    : hm11_at_cmd_wait_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_at_cmd_wait_rsp(const char *at, const char *rsp, uint32 timeout)
{
    uint32 startTime = os_get_tick_count();

    while (false == hm11_at_cmd_check_rsp(at, rsp))
    {
        if (hm11_check_timeout(startTime, timeout))
        {
            return false;
        }
        os_scheduler_delay(DELAY_500_MS);
    }

    return true;
}

/******************************************************************************
* Function    : hm11_transmit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : transparent transmission to hm11 module
******************************************************************************/
static void hm11_transmit(char *data)
{
    os_mutex_lock(&HM11_MUTEX);
    HM11_COM->print(data);
    os_mutex_unlock(&HM11_MUTEX);
}

/******************************************************************************
* Function    : hm11_role
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_role(uint8 mode)
{
    bool ret = false;
    
    switch (mode)
    {
        case HM11_ROLE_SLAVE:
        {
            ret = hm11_at_cmd_wait_rsp(HM11_MASTER_CMD, HM11_RSP_SET1, HM11_ATCMD_WAIT_TIMEOUT);
            break;
        }
        case HM11_ROLE_MASTER:
        {
            ret = hm11_at_cmd_wait_rsp(HM11_SLAVE_CMD, HM11_RSP_SET0, HM11_ATCMD_WAIT_TIMEOUT);
            break;
        }
        default:break;
    }

    return ret;
}

/******************************************************************************
* Function    : hm11_scan_adv_item_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_scan_adv_item_data(uint8 idx, uint32 startTime, uint8* pdata)
{
    bool ret = false;
    //get scan data
    uint16 offset = idx * HM11_SCAN_PACKET_SIZE;

    while (1)
    {
        while (!HM11_COM->available())
        {
            os_scheduler_delay(DELAY_50_MS);
            if (hm11_check_timeout(startTime, (DELAY_1_S*10)))
            {
                OS_DBG_ERR(DBG_MOD_DEV, "Scan item timeout");
                goto end;
            }
        }

        if ((offset + 1) >= (idx + 1) * HM11_SCAN_PACKET_SIZE)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "Scan item exceed");
            goto end;
        }

        if (HM11_COM->available() > 0)
        {
            pdata[offset++] = HM11_COM->read();
            if (offset > HM11_SCAN_DATA_OFFSET
                && pdata[offset - 2] == 0x0D
                && pdata[offset - 1] == 0x0A)
            {
                ret = true;
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan item[%d] finish", idx);
                break;
            }
        }
    }

end:
    return ret;
}

/******************************************************************************
* Function    : hm11_scan_adv_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 hm11_scan_adv_data(uint8 *data, uint8 maxNum)
{
    uint32 sendTime = 0;
    uint8 scanState[HM11_RSP_SCAN_BUFF_SIZE];
    uint16 scanStateOffset = 0;

    uint8 num = 0;
    
    os_mutex_lock(&HM11_MUTEX);

    // clear serial buff
    while (HM11_COM->available())
    {
        HM11_COM->read();
    }

    HM11_COM->print((char*)HM11_SCAN_CMD);
    os_scheduler_delay(DELAY_100_MS);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "AT[%s]", HM11_SCAN_CMD);

    sendTime = os_get_tick_count();
    while (1)
    {
        while (!HM11_COM->available())
        {
            os_scheduler_delay(DELAY_50_MS);
            if (hm11_check_timeout(sendTime, (DELAY_1_S*10)))
            {
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan timeout");
                goto end;
            }
        }

        if ((scanStateOffset + 1) >= HM11_RSP_SCAN_BUFF_SIZE)
        {
            goto end;
        }

        if (HM11_COM->available() > 0)
        {
            scanState[scanStateOffset++] = HM11_COM->read();
            scanState[scanStateOffset] = '\0';

            if (NULL != strstr((char*)scanState, HM11_RSP_SCAN_BEGIN))
            {
                scanStateOffset = 0;
                num = 0;
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan begin");
            }
            else
            if (NULL != strstr((char*)scanState, HM11_RSP_SCAN_END))
            {
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan end");
                break;
            }
            else
            if (NULL != strstr((char*)scanState, HM11_RSP_SCAN_ITEM))
            {
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan item[%d]", num);
                scanStateOffset = 0;
                if (hm11_scan_adv_item_data(num, sendTime, data))
                {
                    num++;
                    if (num >= maxNum)
                    {
                        OS_DBG_ERR(DBG_MOD_DEV, "Scan item max num %d, cur %d", maxNum, num);
                        break;
                    }
                }
            }
        }
    }
        
end:
    os_mutex_unlock(&HM11_MUTEX);
    return num;
}

/******************************************************************************
* Function    : hm11_is_ht_sensor
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check mac to see if the ht sensor data
******************************************************************************/
static bool hm11_is_ht_sensor(uint8 *mac)
{
    if (mac[5] == 0xAC && mac[4] == 0x23 && mac[3] == 0x3F)
    {
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : hm11_scan_ht_sensor
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : scan ht sensor adv data
******************************************************************************/
static uint8 hm11_scan_ht_sensor(HM11_SCAN_DATA *pData, uint8 maxSize)
{
    uint8 scanData[HM11_SCAN_MAX_NUM][HM11_SCAN_PACKET_SIZE];
    uint8 scanNum = 0;
    uint8 validNum = 0;

    memset(scanData, 0, sizeof(scanData));
    scanNum = hm11_scan_adv_data((uint8*)scanData, HM11_SCAN_MAX_NUM);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Scan[%d], max[%d]", scanNum, maxSize);
    if (0 == scanNum)
    {
        goto end;
    }

    for (uint8 idx = 0; idx < scanNum; ++idx)
    {
        if (!hm11_is_ht_sensor(scanData[idx]))
        {
            #if 1
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO,
                                    "unknown MAC[%02X%02X%02X%02X%02X%02X], RSSI[%d], LEN[%d]",
                                    scanData[idx][HM11_SCAN_MAC_OFFSET + 5], scanData[idx][HM11_SCAN_MAC_OFFSET + 4],
                                    scanData[idx][HM11_SCAN_MAC_OFFSET + 3], scanData[idx][HM11_SCAN_MAC_OFFSET + 2],
                                    scanData[idx][HM11_SCAN_MAC_OFFSET + 1], scanData[idx][HM11_SCAN_MAC_OFFSET],
                                    scanData[idx][HM11_SCAN_RSSI_OFFSET],
                                    scanData[idx][HM11_SCAN_LEN_OFFSET]);
            #else
            OS_INFO("unknown MAC[%02X%02X%02X%02X%02X%02X], RSSI[%d], LEN[%d]",
                        scanData[idx][HM11_SCAN_MAC_OFFSET + 5], scanData[idx][HM11_SCAN_MAC_OFFSET + 4],
                        scanData[idx][HM11_SCAN_MAC_OFFSET + 3], scanData[idx][HM11_SCAN_MAC_OFFSET + 2],
                        scanData[idx][HM11_SCAN_MAC_OFFSET + 1], scanData[idx][HM11_SCAN_MAC_OFFSET],
                        scanData[idx][HM11_SCAN_RSSI_OFFSET],
                        scanData[idx][HM11_SCAN_LEN_OFFSET]);
            #endif
            continue;
        }
        
        //MAC
        pData[validNum].mac[0] = scanData[idx][HM11_SCAN_MAC_OFFSET + 5];
        pData[validNum].mac[1] = scanData[idx][HM11_SCAN_MAC_OFFSET + 4];
        pData[validNum].mac[2] = scanData[idx][HM11_SCAN_MAC_OFFSET + 3];
        pData[validNum].mac[3] = scanData[idx][HM11_SCAN_MAC_OFFSET + 2];
        pData[validNum].mac[4] = scanData[idx][HM11_SCAN_MAC_OFFSET + 1];
        pData[validNum].mac[5] = scanData[idx][HM11_SCAN_MAC_OFFSET];
        //RSSI
        pData[validNum].rssi = scanData[idx][HM11_SCAN_RSSI_OFFSET];
        //Rest data len
        pData[validNum].len = scanData[idx][HM11_SCAN_LEN_OFFSET];
        //Rest data
        memcpy(pData[validNum].data, &(scanData[idx][HM11_SCAN_DATA_OFFSET]), pData[validNum].len);

        #if 0   //debug
        OS_INFO("[%d]MAC[%02X%02X%02X%02X%02X%02X], RSSI[%d], LEN[%d]",
                      validNum,
                      pData[validNum].mac[0], pData[validNum].mac[1], pData[validNum].mac[2], 
                      pData[validNum].mac[3], pData[validNum].mac[4], pData[validNum].mac[5], 
                      pData[validNum].rssi,
                      pData[validNum].len);
        #endif
        validNum++;
        if (validNum >= maxSize)
        {
            break;
        }
    }

end:
    return validNum;
}

/******************************************************************************
* Function    : hm11_scan
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : scan ble slave adv data
******************************************************************************/
static uint8 hm11_scan(HM11_SCAN_DATA *pData, uint8 maxSize, uint8 type)
{
    uint8 num = 0;

    switch (type)
    {
        case HM11_SCAN_HT:
        {
            num = hm11_scan_ht_sensor(pData, maxSize);
            break;
        }
        default:break;
    }
    
    return num;
}

/******************************************************************************
* Function    : hm11_get_baudrate_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 hm11_get_baudrate_cmd(uint32 baud)
{
    uint8 cmd = HM11_BAUDRATE_CMD_UNKNOWN;
    for (uint8 idx = HM11_BAUDRATE_CMD_BEGIN; idx < HM11_BAUDRATE_CMD_END; ++idx)
    {
        if (HM11_BAUDRATE_TABLE[idx] == baud) 
        {
            cmd = idx;
            break;
        }
    }

    return cmd;
}

/******************************************************************************
* Function    : hm11_set_baudrate
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_set_baudrate(uint32 baud)
{
    bool ret = false;
    uint8 cmd = hm11_get_baudrate_cmd(baud);

    if (cmd == HM11_BAUDRATE_CMD_UNKNOWN)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "un-supported baudrate %d", baud);
    }
    else
    {
        char baudCmd[16] = {0};
        char baudRsp[16] = {0};

        snprintf(baudCmd, 15, "%s%d", HM11_BAUD_CMD, cmd);
        snprintf(baudRsp, 15, "%s%d", HM11_RSP_SET, cmd);

        ret = hm11_at_cmd_wait_rsp(baudCmd, baudRsp, HM11_ATCMD_WAIT_TIMEOUT);
    }
    
    return ret;
}

/******************************************************************************
* Function    : hm11_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_reset(void)
{
    uint8 retryCnt = 0;
    bool ret = false;

retry:
    hal_gpio_set(BOARD_HM11_RST, HAL_GPIO_LOW);
    os_scheduler_delay(DELAY_100_MS * 3);
    hal_gpio_set(BOARD_HM11_RST, HAL_GPIO_HIGH);

    HM11_COM->end();
    HM11_COM->begin(HM11_COM_BAUDRATE);

    ret = hm11_at_cmd_wait_rsp(HM11_CHECK_CMD, HM11_RSP_OK, HM11_ATCMD_WAIT_TIMEOUT);
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "try to set baudrate[%d]to[%d]", 
                            HM11_DEF_BAUDRATE, HM11_COM_BAUDRATE);

        HM11_COM->end();
        HM11_COM->begin(HM11_DEF_BAUDRATE);

        if (hm11_set_baudrate(HM11_COM_BAUDRATE))
        {
            if (++retryCnt <= 2)
            {
                goto retry;
            }
        }
    }

    return ret;
}

/******************************************************************************
* Function    : hm11_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_config(char *name)
{
    bool ret = false;
    char cmd[HM11_RSP_MAX_LEN] = {0};
    char rsp[HM11_RSP_MAX_LEN] = {0};

    //set work mode
    ret = hm11_at_cmd_wait_rsp(HM11_MODE_CMD, HM11_RSP_SET1, HM11_ATCMD_WAIT_TIMEOUT);
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "mode set err"); 
        goto err;
    }
    
    //set name
    snprintf(cmd, HM11_RSP_MAX_LEN, "%s%s", HM11_NAME_CMD, name);
    snprintf(rsp, HM11_RSP_MAX_LEN, "%s%s", HM11_RSP_SET, name);
    ret = hm11_at_cmd_wait_rsp(cmd, rsp, HM11_ATCMD_WAIT_TIMEOUT);
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "name set err"); 
        goto err;
    }

err:
    return ret;
}

/******************************************************************************
* Function    : hm11_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hm11_init(const HAL_USART_TYPE *com, const uint32 baudrate)
{
    if (com == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "HM11 com is invalid");
        return false;
    }

    //init HM11 and usart
    HM11_COM = (HAL_USART_TYPE *)com;
    HM11_COM_BAUDRATE = baudrate;
    os_mutex_lock_init(&HM11_MUTEX);

    hal_rcc_enable(BOARD_HM11_IO_RCC);
    hal_gpio_set_mode(BOARD_HM11_RST, HAL_GPIO_OUT_PP);

    hm11_reset();

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "HM11 INIT OK, BAUD[%d]", baudrate);
    return true;
}

const DEV_TYPE_HM11 devHM11 = 
{
    hm11_init,
    hm11_reset,
    hm11_config,
    hm11_transmit,
    hm11_role,
    hm11_scan
};

