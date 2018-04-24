/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          rgb_led_task.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   manage rgb led and make all the action in a os message que
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-24      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __RGB_LED_TASK_H__
#define __RGB_LED_TASK_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define RGBLED_MAX_ACTION 16

/******************************************************************************
* Enum
******************************************************************************/
typedef enum
{
    RGBLED_BLINK_WHITE = 0,
    RGBLED_BLINK_RED,
    RGBLED_BLINK_GREEN,
    RGBLED_CHECK
}RGBLED_ACTION_ID;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 r;
    uint8 g;
    uint8 b;
    uint8 time; // unit 100ms
}RGBLED_VAL_TYPE;

typedef struct
{
	uint8 current;
	uint8 total;
	uint8 sub_current;
	uint8 dummy;
	RGBLED_VAL_TYPE actions[RGBLED_MAX_ACTION];
}RGBLED_ACTION_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
// need add this function in a pwm
extern void rgbled_process(void);
extern void rgbled_task(void *param);
extern void rgbled_send_act_req(RGBLED_ACTION_ID req);

#endif /* __RGB_LED_TASK_H__ */

