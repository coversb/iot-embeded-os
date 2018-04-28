/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_multimedia.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   multimedia operations
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-28      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_MULTIMEDIA_H__
#define __PB_MULTIMEDIA_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_MM_MUTE_OFF = 0,
    PB_MM_MUTE_ON,
    PB_MM_SET_VOL,
    PB_MM_VOL_UP,
    PB_MM_VOL_DOWN, // 4
    PB_MM_STOP,
    PB_MM_PAUSE,
    PB_MM_PLAY,
    PB_MM_PLAY_WELCOME, //8
    PB_MM_PLAY_ORDER_OVER,
    PB_MM_PLAY_SMOKE_ALARM,
    PB_MM_PLAY_BGM // 11
}PB_MULTIMEDIA_ACT_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 cmd;
    uint8 param;
} PB_MULTIMEDIA_MSG_PARA_TYPE;

typedef struct
{
    uint16 fileNum;
    uint16 lastPalyingIdx;
    bool playing;
    uint32 lastStartTime;
    uint32 lastCheckTime;
}AUDIO_BGM_MANAGE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_multimedia_main(void *param);
extern bool pb_multimedia_audio_available(void);
extern uint8 pb_multimedia_get_audio_volume(void);
extern void pb_multimedia_send_monitor_req(void);
extern void pb_multimedia_send_audio_msg(uint8 cmd, uint8 param);

#endif /* __PB_MULTIMEDIA_H__ */

