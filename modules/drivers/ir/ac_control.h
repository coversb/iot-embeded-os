/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          ac_control.h
*  author:              Chen hao
*  version:             1.00
*  file description:   air conditioner IR control
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-26      1.00                    Chen hao
*
******************************************************************************/
#ifndef __AC_CONTROL_H__
#define __AC_CONTROL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "ir_remote_control.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    AC_SPEED_HIGH = 0,
    AC_SPEED_MIDDLE,
    AC_SPEED_LOW,
    AC_SPEED_AUTO,
    AC_SPEED_NUM
}AC_SPEED_TYPE;

typedef enum
{
    AC_MODE_CLOSE = 0,
    AC_MODE_COLD,
    AC_MODE_HOT,
    AC_MODE_WIND,
    AC_MODE_DRY,
    AC_MODE_AUTO,
    AC_MODE_NUM
}AC_MODE_TYPE;

/******************************************************************************
* Types
******************************************************************************/

typedef struct
{
    bool (*init)(const DEV_TYPE_IR_CONTROL *ir);
    void (*close)(void);
    void (*set)(AC_MODE_TYPE mode, AC_SPEED_TYPE speed, uint8 temperature);
}DEV_TYPE_AC_CONTROL;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const DEV_TYPE_AC_CONTROL acCtrl;

#endif /* __AC_CONTROL_H__ */

