/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          hm11.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   hm-11 ble module
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-8-17      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HM11_H__
#define __HM11_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define HM11_SCAN_MAC_LEN 6
#define HM11_SCAN_DATA_LEN 64

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    HM11_BAUDRATE_9600 = 9600,
    HM11_BAUDRATE_19200 = 19200,
    HM11_BAUDRATE_38400 = 38400,
    HM11_BAUDRATE_57600 = 57600,
    HM11_BAUDRATE_115200 = 115200
}HM11_BAUDRATE_TYPE;

typedef enum
{
    HM11_BAUDRATE_CMD_BEGIN = 0,
    HM11_BAUDRATE_CMD_9600 = HM11_BAUDRATE_CMD_BEGIN,
    HM11_BAUDRATE_CMD_19200 = 1,
    HM11_BAUDRATE_CMD_38400 = 2,
    HM11_BAUDRATE_CMD_57600 = 3,
    HM11_BAUDRATE_CMD_115200 = 4,
    HM11_BAUDRATE_CMD_END,
    HM11_BAUDRATE_CMD_UNKNOWN = 0xFF
}HM11_BAUDRATE_CMD;

typedef enum
{
    HM11_ROLE_SLAVE = 0,
    HM11_ROLE_MASTER = 1
}HM11_ROLE_TYPE;

typedef enum
{
    HM11_SCAN_HT = 0
}HM11_SCAN_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 mac[HM11_SCAN_MAC_LEN];
    int8 rssi;
    uint8 len;
    uint8 data[HM11_SCAN_DATA_LEN];
}HM11_SCAN_DATA;

typedef struct
{
    bool (*init)(const HAL_USART_TYPE *com, const uint32 baudrate);
    bool (*reset)(void);
    bool (*config)(char *name);
    void (*transmit)(char *data);
    bool (*role)(uint8 mode);
    uint8 (*scan)(HM11_SCAN_DATA *pData, uint8 maxSize, uint8 type);
}DEV_TYPE_HM11;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const DEV_TYPE_HM11 devHM11;

#endif /* __KT603_H__ */

