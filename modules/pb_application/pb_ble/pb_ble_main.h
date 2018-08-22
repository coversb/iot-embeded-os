/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ble_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   ble management
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-8-16      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_BLE_MAIN_H__
#define __PB_BLE_MAIN_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_util.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    BLE_STATE_INIT = 0,
    BLE_STATE_CONFIG,
    BLE_STATE_WORKING,
    BLE_STATE_RESET
}PB_BLE_STATE_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 mac[6];
    int8 rssi;
    uint8 battery;
    float humidity;
    float temperature;
}PB_BLE_HT_DATA;

typedef struct
{
    uint8 bleState;
    uint16 bleProcInterval;
    uint8 bleMode;
}PB_BLE_CONTEXT_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_ble_main(void *pvParameters);
extern void pb_ble_tramsmit(char *data);
extern uint8 pb_ble_get_temperature(void);
extern uint8 pb_ble_get_humidity(void);

#endif /* __PB_BLE_MAIN_H__ */

