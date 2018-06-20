/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_firmware_manage.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   firmware info manage
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-14      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_FIRMWARE_MANAGE_H__
#define __PB_FIRMWARE_MANAGE_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define MAGICKEY 0xDEADBEEF
#define DEFAULT_VALUE 0xFFFFFFFF

#define FIRMWARE_BOOTFAIL 8
#define FIRMWARE_UPGRADE_RETRY 5

/******************************************************************************
* Enum
******************************************************************************/
typedef enum
{
    PB_IMAGE_BL = 0,
    PB_IMAGE_APP
}PB_IMAGE_TYPE;

typedef enum
{
    PB_FIRMWARE_A = 0,
    PB_FIRMWARE_B,
    PB_FIRMWARE_NUM,
    PB_FIRMWARE_UART = 2
}PB_FIRMWARE_TYPE;

typedef enum
{
    PB_FIRMWARE_NORMAL,
    PB_FIRMWARE_FOTA,
    PB_FIRMWARE_RECOVER
}PB_FIRMWARE_RUNNABLE_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 imageType;
    uint16 hwVersion;
    uint16 imageVersion;
}PB_IMAGE_INFO;

typedef struct //24B
{
    uint32 length;                  // total image length in bytes. normally it is 128B aligned. it is used to indicate the baker is used or not.
    uint32 crc;                         // CRC32 value of the whole image except last 4B
    uint32 upgradeTimestamp; // the UTC time when upgrading. if no, 0xFFFFFFFF
    uint16 runTimes;             // 0xFFFF: not used, 0: just uggraded, >0 verified, every boot increase by 1 till 0x7FFF
    uint16 version;               // image version number
    uint32 romBegin;           // for upgrader to check flash write boundary
    uint32 romEnd;             // for upgrader to check flash write boundary
}PB_IMAGE_CONTEXT;

typedef struct
{
    uint32 magicKey;                // 0xDEADBEEF
    uint32 eraseTimes;              // block erased times
    uint16 curImage;                // which image is used in runnable, 0: appa, 1: appb, 2: image download via uart
    uint16 runnableWorking;     // 0: working, other: not work. app itself need set it to zero when it is running. it is used for 7 strike.
    uint32 runnableBootTimes; // 0: inform bl img updated, >0 run times till 0x7FFFFFFF. set to zero when need ota, set to 1 when move, increase by 1 when confirm.
    PB_IMAGE_CONTEXT images[PB_FIRMWARE_NUM];
}PB_FIRMWARE_CONTEXT;

typedef struct
{
    PB_FIRMWARE_CONTEXT* (*firmwareContext)(void);
    bool (*imageInfo)(uint8 type, PB_IMAGE_INFO *info);
    void (*load)(void);
    void (*update)(void);
    void (*submit)(PB_IMAGE_CONTEXT *imageContext);
    void (*updateRunnable)(uint8 type);
}PB_FIRMWARE_MANAGE;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const PB_FIRMWARE_MANAGE fwManager;

#endif  /* __PB_FIRMWARE_MANAGE_H__ */
