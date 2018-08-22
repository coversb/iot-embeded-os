/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ble_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   ble management
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-8-16      1.00                    Chen Hao
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
#include "pb_ble_main.h"
#include "hm11.h"

#if (PB_BLE_ENABLE == 1)
/******************************************************************************
* Macro
******************************************************************************/
#define BLE devHM11

#define BLE_PROC_CONFIG_INTERVAL (DELAY_500_MS)
#define BLE_PROC_WORK_INTERVAL (DELAY_1_S*10)

#define BLE_ADV_CONTAINER_SIZE 16

//HT Sensor
#define HT_SENSOR_UUIDH_OFFSET (10)
#define HT_SENSOR_UUIDL_OFFSET (9)
#define HT_SENSOR_FRAME_OFFSET (11)
#define HT_SENSOR_PRODUCT_OFFSET (12)
#define HT_SENSOR_BAT_OFFSET (13)
#define HT_SENSOR_TH_OFFSET (14)    //temperature high byte
#define HT_SENSOR_HH_OFFSET (16)    //humidity high byte

#define HT_SENSOR_UUIDH 0xFF
#define HT_SENSOR_UUIDL 0xE1
#define HT_SENSOR_FRAME 0xA1
#define HT_SENSOR_PRODUCT 0x01

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_BLE_CONTEXT_TYPE pb_ble_context;
static PB_BLE_HT_DATA bleHtSensor;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_ble_module_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_module_init(void)
{
    BLE.init(&BLE_COM, BLE_COM_BAUDRATE);
}

/******************************************************************************
* Function    : pb_ble_main_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_main_init(void)
{
    memset(&pb_ble_context, 0, sizeof(pb_ble_context));
    // work as master
    pb_ble_context.bleMode = HM11_ROLE_MASTER;

    pb_ble_module_init();
    #if 1 // for debug
    pb_ble_context.bleState = BLE_STATE_WORKING;
    pb_ble_context.bleProcInterval = BLE_PROC_WORK_INTERVAL;
    #else
    pb_ble_context.bleState = BLE_STATE_CONFIG;
    pb_ble_context.bleProcInterval = BLE_PROC_CONFIG_INTERVAL;
    #endif
}

/******************************************************************************
* Function    : pb_ble_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_reset(void)
{
    BLE.reset();
    pb_ble_context.bleState = BLE_STATE_CONFIG;
    pb_ble_context.bleProcInterval = BLE_PROC_CONFIG_INTERVAL;
}

/******************************************************************************
* Function    : pb_ble_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_config(void)
{
    //config for BLE module
    if (!BLE.config(BLE_NAME))
    {
        OS_DBG_ERR(DBG_MOD_PBBLE, "ble config err");
        pb_ble_context.bleState = BLE_STATE_RESET;
        pb_ble_context.bleProcInterval = BLE_PROC_CONFIG_INTERVAL;
        return;
    }

    //check ble role
    if (pb_ble_context.bleMode != HM11_ROLE_SLAVE
        && pb_ble_context.bleMode != HM11_ROLE_MASTER)
    {
        OS_DBG_ERR(DBG_MOD_PBBLE, "unknown mode[%d], set master", 
                            pb_ble_context.bleMode, HM11_ROLE_MASTER);
        pb_ble_context.bleMode = HM11_ROLE_MASTER;
    }

    if (BLE.role(pb_ble_context.bleMode))
    {
        pb_ble_context.bleState = BLE_STATE_WORKING;
        pb_ble_context.bleProcInterval = BLE_PROC_WORK_INTERVAL;
    }
    else
    {
        pb_ble_context.bleState = BLE_STATE_RESET;
        pb_ble_context.bleProcInterval = BLE_PROC_CONFIG_INTERVAL;
        OS_DBG_ERR(DBG_MOD_PBBLE, "Enter master err, reset");
    }
}

/******************************************************************************
* Function    : pb_ble_ht_duplicate
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_ble_ht_duplicate(PB_BLE_HT_DATA *existHt, uint8 existNum, PB_BLE_HT_DATA *newHt)
{
    for (uint8 idx = 0; idx < existNum; ++idx)
    {
        if (0 == memcmp(existHt[idx].mac, newHt->mac, 6))
        {
            if (newHt->rssi >= existHt[idx].rssi)
            {
                memcpy(&existHt[idx], newHt, sizeof(PB_BLE_HT_DATA));
                OS_DBG_TRACE(DBG_MOD_PBBLE, DBG_INFO, "repeat ht, replace it");
            }
            return true;
        }
    }

    return false;
}

/******************************************************************************
* Function    : pb_ble_convert_ht
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_ble_convert_ht(HM11_SCAN_DATA *in, uint8 inNum, PB_BLE_HT_DATA *out, uint8 maxOut)
{
    uint8 num = 0;

    inNum = MIN_VALUE(inNum, maxOut);

    for (uint8 idx = 0; idx < inNum; ++idx)
    {
        #if 0   //hex dump debug
        char sendHex[128+1];
        uint16 hexLen = MIN_VALUE(HM11_SCAN_DATA_LEN, 120/2);
        os_trace_get_hex_str((uint8*)sendHex, sizeof(sendHex), in[idx].data, hexLen);
        OS_INFO("Send:%s", sendHex);
        #endif

        if (HT_SENSOR_UUIDH == in[idx].data[HT_SENSOR_UUIDH_OFFSET]
            && HT_SENSOR_UUIDL == in[idx].data[HT_SENSOR_UUIDL_OFFSET]
            && HT_SENSOR_FRAME == in[idx].data[HT_SENSOR_FRAME_OFFSET]
            && HT_SENSOR_PRODUCT == in[idx].data[HT_SENSOR_PRODUCT_OFFSET])
        {
            PB_BLE_HT_DATA ht;
            //MAC
            ht.mac[0] = in[idx].mac[0];
            ht.mac[1] = in[idx].mac[1];
            ht.mac[2] = in[idx].mac[2];
            ht.mac[3] = in[idx].mac[3];
            ht.mac[4] = in[idx].mac[4];
            ht.mac[5] = in[idx].mac[5];
            //RSSI
            ht.rssi = in[idx].rssi;
            //Battery
            ht.battery = in[idx].data[HT_SENSOR_BAT_OFFSET];
            //Temperature
            uint16 t = in[idx].data[HT_SENSOR_TH_OFFSET] << 8 | in[idx].data[HT_SENSOR_TH_OFFSET + 1];
            ht.temperature = t / 256.0f;
            //Humidity
            uint16 h = in[idx].data[HT_SENSOR_HH_OFFSET] << 8 | in[idx].data[HT_SENSOR_HH_OFFSET + 1];
            ht.humidity = h / 256.0f;

            if (pb_ble_ht_duplicate(out, num, &ht))
            {
                continue;
            }

            memcpy(&out[num], &ht, sizeof(PB_BLE_HT_DATA));
            num++;  
        }
    }

    return num;
}

/******************************************************************************
* Function    : pb_ble_update_ht
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_update_ht(PB_BLE_HT_DATA *htSensor, uint8 num)
{
    PB_BLE_HT_DATA *pHt = htSensor;
    for (uint8 idx = 0; idx < num; ++idx)
    {
        OS_DBG_TRACE(DBG_MOD_PBBLE, DBG_INFO, 
                                "MAC[%02X%02X%02X%02X%02X%02X], RSSI[%d], B[%d%%], H[%.2f], T[%.2f]",
                                htSensor[idx].mac[0], htSensor[idx].mac[1], htSensor[idx].mac[2], 
                                htSensor[idx].mac[3], htSensor[idx].mac[4], htSensor[idx].mac[5], 
                                htSensor[idx].rssi, htSensor[idx].battery,
                                htSensor[idx].humidity, htSensor[idx].temperature);
    }
    //filter 
    if (num == 1)
    {
        pHt = &htSensor[0];
    }
    else
    {
        pHt = &htSensor[0];
    }

    OS_DBG_TRACE(DBG_MOD_PBBLE, DBG_INFO, 
                            "update ht[%02X%02X%02X%02X%02X%02X][H %.2f, T %.2f]--->[H %.2f, T %.2f]",
                            bleHtSensor.mac[0], bleHtSensor.mac[1], bleHtSensor.mac[2], 
                            bleHtSensor.mac[3], bleHtSensor.mac[4], bleHtSensor.mac[5], 
                            bleHtSensor.humidity, bleHtSensor.temperature,
                            pHt->humidity, pHt->temperature); 
    memcpy(&bleHtSensor, pHt, sizeof(PB_BLE_HT_DATA));
}

/******************************************************************************
* Function    : pb_ble_parse_ht_adv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : parse humidity and temperature from adv data
******************************************************************************/
static void pb_ble_parse_ht_adv(void)
{
    HM11_SCAN_DATA scanData[BLE_ADV_CONTAINER_SIZE];
    uint8 num = 0;
    memset(scanData, 0, sizeof(scanData));

    num = BLE.scan(scanData, BLE_ADV_CONTAINER_SIZE, HM11_SCAN_HT);
    OS_DBG_TRACE(DBG_MOD_PBBLE, DBG_INFO, "scan ht[%d]", num); 
    if (0 == num)
    {
        return;
    }

    PB_BLE_HT_DATA htSensor[BLE_ADV_CONTAINER_SIZE];
    num = pb_ble_convert_ht(scanData, num, htSensor, BLE_ADV_CONTAINER_SIZE);
    OS_DBG_TRACE(DBG_MOD_PBBLE, DBG_INFO, "convert ht[%d]", num);

    if (0 == num)
    {
        return;
    }

    pb_ble_update_ht(htSensor, num);
}

/******************************************************************************
* Function    : pb_ble_working
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_ble_working(void)
{
    switch (pb_ble_context.bleMode)
    {
        case HM11_ROLE_MASTER:
        {
            pb_ble_parse_ht_adv();
            break;
        }
        case HM11_ROLE_SLAVE:
        {
            break;
        }
        default:
        {
            pb_ble_context.bleMode = HM11_ROLE_MASTER;
            pb_ble_context.bleState = BLE_STATE_RESET;
            pb_ble_context.bleProcInterval = BLE_PROC_CONFIG_INTERVAL;
            OS_DBG_ERR(DBG_MOD_PBBLE, "unknown mode[%d], reset", pb_ble_context.bleMode);
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_ble_get_humidity
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_ble_get_humidity(void)
{
    return (uint8)bleHtSensor.humidity;
}

/******************************************************************************
* Function    : pb_ble_get_temperature
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_ble_get_temperature(void)
{
    return (uint8)bleHtSensor.temperature;
}

/******************************************************************************
* Function    : pb_ble_tramsmit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_ble_tramsmit(char *data)
{
    BLE.transmit(data);
}

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
void pb_ble_main(void *pvParameters)
{
    pb_ble_main_init();

    os_set_task_init(OS_TASK_ITEM_PB_BLE);
    os_wait_task_init_sync();

    while (1)
    {
        switch (pb_ble_context.bleState)
        {
            case BLE_STATE_CONFIG:
            {
                pb_ble_config();
                break;
            }
            case BLE_STATE_WORKING:
            {
                pb_ble_working();
                break;
            }
            default:
            case BLE_STATE_INIT:
            case BLE_STATE_RESET:
            {
                pb_ble_reset();
                break;
            }
        }

        os_scheduler_delay(pb_ble_context.bleProcInterval);
    }    
}

#endif /*PB_BLE_ENABLE*/

