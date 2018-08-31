/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_aircon.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   air conditioner control
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-31      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_IO_AIRCON_H__
#define __PB_IO_AIRCON_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_IO_AIRCON_CLOSE = 0,
    PB_IO_AIRCON_OPEN,
    PB_IO_AIRCON_NEED_RESTART
} PB_IO_AIRCON_STAT;

typedef enum
{
    PB_IO_AIRCON_OFF = 0,
    PB_IO_AIRCON_ON
}PB_IO_AIRCON_MODE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint16 (*state)(void);
    void (*init)(void);
    void (*process)(void);
    void (*set)(uint8 cmd);
    void (*setState)(PB_IO_AIRCON_STAT state);
}PB_IO_AIRCON;

typedef struct
{
    bool (*needOpen)(void);
}PB_IO_AIRCON_PWRON_EVENT;

typedef struct
{
    bool (*needClose)(void);
}PB_IO_AIRCON_PWROFF_EVENT;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const PB_IO_AIRCON AIRCON;

#endif /* __PB_IO_AIRCON_H__ */

