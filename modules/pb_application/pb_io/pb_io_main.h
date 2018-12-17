/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   inuput / output functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-25      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_IO_MAIN_H__
#define __PB_IO_MAIN_H__
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
    PB_IO_PWR_OFF = 0,
    PB_IO_PWR_ON = 1
} PB_IO_PWR_TYPE;

typedef enum
{
    PB_IO_DOOR_CLOSE = 0,
    PB_IO_DOOR_OPEN = 1,
    PB_IO_DOOR_UNKNOWN = 0xFF
} PB_IO_DOOR_SW_TYPE;

typedef enum
{
    PB_IO_IR_IDLE = 0,
    PB_IO_IR_TRIGGERED = 1
}PB_IO_IR_DETECTION_TYPE;

typedef enum
{
    PB_IO_DEVBOX_CLOSE = 0,
    PB_IO_DEVBOX_OPEN = 1
} PB_IO_DEVBOX_SW_TYPE;

typedef enum
{
    PB_IO_OUTPUT_OUT_OF_VALIDTIME = 0,
    PB_IO_OUTPUT_IN_VALIDTIME
}PB_IO_OUTPUT_MODE;

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_io_main(void *param);

extern void pb_io_output_restore(void);
extern void pb_io_output_set(uint32 mask);
extern uint32 pb_io_output_mask(void);
extern uint32 pb_io_input_mask(void);
extern void pb_io_door_lock_sw(uint8 sw, uint8 type);
extern void pb_io_dev_lock_sw(uint8 sw);

extern void pb_io_tv_reboot_sw(uint8 sw);
extern void pb_io_exhaust_mode(uint8 mode);

extern void pb_io_aircon_update(void);
extern uint16 pb_io_aircon_state(void);
extern uint8 pb_io_pwr_suply(void);
extern uint8 pb_io_smoke_level(void);
extern uint8 pb_io_door_status(void);
extern uint8 pb_io_ir_status(void);

#endif /* __PB_IO_MAIN_H__ */

