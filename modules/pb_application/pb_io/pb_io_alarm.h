/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_alarm.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   io alarm check
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-29      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_IO_ALARM_H__
#define __PB_IO_ALARM_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_IO_PWR_ALARM_DEBOUNCE 3 // seconds
#define PB_IO_PWR_ALARM_INTERVAL 60 // seconds
#define PB_IO_SMOKE_AUDIO_INTERVAL 5 // seconds

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_IO_ALARM_RELIEVED = 0,
    PB_IO_ALARM_TRIGGERED = 1
}PB_IO_ALARM_STATUS;

typedef enum
{
    PB_IO_ALARM_BEGIN = 0,
    PB_IO_ALARM_PWR_SUPPLY = PB_IO_ALARM_BEGIN,
    PB_IO_ALARM_SMOKE,
    PB_IO_ALARM_DOOR,
    PB_IO_ALARM_END
}PB_IO_ALARM_ITEM;

typedef enum
{
    PB_IO_ALARM_STATE_PWR = 0,
    PB_IO_ALARM_STATE_SMOKE,
    PB_IO_ALARM_STATE_DOOR
}PB_IO_ALARM_STATE_ITEM;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    const char *name;
    PB_IO_ALARM_STATE_ITEM item;
    PB_IO_ALARM_STATUS alarmStatus;
    uint32 lastRecordTime;
    bool (*needCheck)(void);
    uint32 (*getDebounceDuration)(void);
    bool (*isDetected)(void);
    void (*triggerHandler)(void);
    void (*triggeringHandler)(uint32 *lastReocrdTime, uint32 curTime);
    void (*relieveHandler)(void);
}PB_IO_ALARM;

/******************************************************************************
* Global Variables
******************************************************************************/
extern void pb_io_alarm_init(void);
extern void pb_io_alarm_check(void);

#endif /* __PB_IO_ALARM_H__ */

