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
#ifndef __KT603_H__
#define __KT603_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define KT603_MIN_VOL (0)
#define KT603_MAX_VOL (30)

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    KT603_NO_DEV = 0,
    KT603_UDISK = 1,
    KT603_TF = 2,
    KT603_PC = 4,
    KT603_FLASH = 8
}KT603_AVAILABLE_DEV;

typedef enum
{
    KT603_STOP = 0,
    KT603_PLAYING,
    KT603_PAUSE,
    KT603_UNKNOW
}KT603_PLAY_STATUS;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    bool (*init)(const HAL_USART_TYPE *com, const uint32 baudrate);
    bool (*reset)(void);
    bool (*volume)(uint8 lv);
    void (*mute)(bool sw);
    uint8 (*available)(void);
    uint16 (*fileNumber)(uint8 dir);
    bool (*playFile)(uint8 dir, uint8 file);
    bool (*interruptPlayFile)(uint8 dir, uint8 file);
    bool (*play)(void);
    bool (*stop)(void);
    bool (*pause)(void);
    uint8 (*playStatus)(void);
}DEV_TYPE_KT603;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const DEV_TYPE_KT603 devKT603;

#endif /* __KT603_H__ */

