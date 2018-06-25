/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          rgb_led_task.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   manage rgb led and make all the action in a os message que
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-24      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "board_config.h"
#include "hal_rcc.h"
#include "hal_board.h"
#include "hal_gpio.h"
#include "hal_tim.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "os_task_define.h"
#include "rgb_led_task.h"

/******************************************************************************
* Macros
******************************************************************************/
#define RGB_OFF                0,   0,   0
#define RGB_WHITE             86, 100,  63
#define RGB_RED              100,   8,   0
#define RGB_GREEN             29, 100,   0

#define RGBLED_TASK_MSG_QUE_SIZE 5
#define RGBLED_OFF 0xDEADBEEF
#define RGBLED_ACTION_CHECK_INTERVAL 100    //ms

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MSG_QUEUE_TYPE rgbled_msg_que;
static RGBLED_ACTION_TYPE rgbled_action;

static uint32 rgbPwmCnt = RGBLED_OFF;
static RGBLED_VAL_TYPE rgbled_val;

/******************************************************************************
* Local Functions define
******************************************************************************/
/******************************************************************************
* Function    : rgbled_set
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : set RGB value
******************************************************************************/
static void rgbled_set(RGBLED_VAL_TYPE val)
{
    rgbled_val = val;
    rgbPwmCnt = 0;
}

/******************************************************************************
* Function    : rgbled_off
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : turn off the light box
******************************************************************************/
static void rgbled_off(void)
{
    rgbled_val.r = 0;
    rgbled_val.g = 0;
    rgbled_val.b = 0;
    rgbled_val.time = 0;
    rgbPwmCnt = RGBLED_OFF;
}

/******************************************************************************
* Function    : rgbled_process
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : control rgb light by pwm in tim4
******************************************************************************/
void rgbled_process(void)
{
    if (rgbPwmCnt == RGBLED_OFF)
    {
        return;
    }

    //Red light
    if (rgbled_val.r > rgbPwmCnt)
    {
        hal_gpio_set(BOARD_RGBLED_R, HAL_GPIO_HIGH);
    }
    else
    {
        hal_gpio_set(BOARD_RGBLED_R, HAL_GPIO_LOW);
    }

    //Green light
    if (rgbled_val.g > rgbPwmCnt)
    {
        hal_gpio_set(BOARD_RGBLED_G, HAL_GPIO_HIGH);
    }
    else
    {
        hal_gpio_set(BOARD_RGBLED_G, HAL_GPIO_LOW);
    }

    //Blue light
    if (rgbled_val.b > rgbPwmCnt)
    {
        hal_gpio_set(BOARD_RGBLED_B, HAL_GPIO_HIGH);
    }
    else
    {
        hal_gpio_set(BOARD_RGBLED_B, HAL_GPIO_LOW);
    }

    rgbPwmCnt = (rgbPwmCnt + 1) % 100;
}

/******************************************************************************
* Function    : rgbled_set_action
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void rgbled_set_action(RGBLED_VAL_TYPE *pAction, uint8 r, uint8 g, uint8 b, uint8 t)
{
    pAction->r = r;
    pAction->g = g;
    pAction->b = b;
    pAction->time = t;
}

/******************************************************************************
* Function    : rgbled_set_white_blink_action
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void rgbled_set_white_blink_action(void)
{
    memset(&rgbled_action, 0x00, sizeof(rgbled_action));
    rgbled_action.current = 0;
    rgbled_action.total = 7;
    rgbled_action.sub_current = 0;
    rgbled_set_action(&rgbled_action.actions[0], RGB_WHITE, 3);
    rgbled_set_action(&rgbled_action.actions[1], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[2], RGB_WHITE, 3);
    rgbled_set_action(&rgbled_action.actions[3], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[4], RGB_WHITE, 3);
    rgbled_set_action(&rgbled_action.actions[5], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[6], RGB_WHITE, 20);
    rgbled_set_action(&rgbled_action.actions[7], RGB_OFF, 3);
}

/******************************************************************************
* Function    : rgbled_set_red_blink_action
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void rgbled_set_red_blink_action(void)
{
    memset(&rgbled_action, 0x00, sizeof(rgbled_action));
    rgbled_action.current = 0;
    rgbled_action.total = 7;
    rgbled_action.sub_current = 0;
    rgbled_set_action(&rgbled_action.actions[0], RGB_RED, 3);
    rgbled_set_action(&rgbled_action.actions[1], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[2], RGB_RED, 3);
    rgbled_set_action(&rgbled_action.actions[3], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[4], RGB_RED, 3);
    rgbled_set_action(&rgbled_action.actions[5], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[6], RGB_RED, 20);
    rgbled_set_action(&rgbled_action.actions[7], RGB_OFF, 3);
}

/******************************************************************************
* Function    : rgbled_set_green_blink_action
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void rgbled_set_green_blink_action(void)
{
    memset(&rgbled_action, 0x00, sizeof(rgbled_action));
    rgbled_action.current = 0;
    rgbled_action.total = 7;
    rgbled_action.sub_current = 0;
    rgbled_set_action(&rgbled_action.actions[0], RGB_GREEN, 3);
    rgbled_set_action(&rgbled_action.actions[1], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[2], RGB_GREEN, 3);
    rgbled_set_action(&rgbled_action.actions[3], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[4], RGB_GREEN, 3);
    rgbled_set_action(&rgbled_action.actions[5], RGB_OFF, 3);
    rgbled_set_action(&rgbled_action.actions[6], RGB_GREEN, 20);
    rgbled_set_action(&rgbled_action.actions[7], RGB_OFF, 3);
}

/******************************************************************************
* Function    : rgbled_action_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool rgbled_action_process(void)
{
    bool ret = true;
    if (rgbled_action.total)
    {
        if (rgbled_action.sub_current == 0)
        {
            rgbled_set(rgbled_action.actions[rgbled_action.current]);
            rgbled_action.sub_current++;
        }
        else 
        if (rgbled_action.sub_current < rgbled_action.actions[rgbled_action.current].time)
        {
            rgbled_action.sub_current++;
        }
        else
        {
            rgbled_action.sub_current = 0;
            if (++(rgbled_action.current) > rgbled_action.total)
            {
                rgbled_action.total = 0;
                rgbled_off();
                ret = false;
            }
        }
    }
    return ret;
}

/******************************************************************************
* Function    : rgbled_task_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void rgbled_task_init(void)
{
    rgbled_msg_que = os_msg_queue_create(RGBLED_TASK_MSG_QUE_SIZE, sizeof(uint32));
    memset(&rgbled_action, 0x00, sizeof(rgbled_action));

    tim2.setCallback(rgbled_process);

    /*RCC config*/
    hal_rcc_enable(BOARD_RGBLED_RCC);

    /*GPIO config*/
    hal_gpio_set_mode(BOARD_RGBLED_R, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_RGBLED_G, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_RGBLED_B, HAL_GPIO_OUT_PP);

    hal_gpio_set(BOARD_RGBLED_R, HAL_GPIO_LOW);
    hal_gpio_set(BOARD_RGBLED_G, HAL_GPIO_LOW);
    hal_gpio_set(BOARD_RGBLED_B, HAL_GPIO_LOW);

    rgbled_off();
}

/******************************************************************************
* Function    : rgbled_task
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void rgbled_task(void *param)
{
    rgbled_task_init();

    os_set_task_init(OS_TASK_ITEM_RGBLED);
    os_wait_task_init_sync();

    uint32 actionID = 0;

    while (1)
    {
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(rgbled_msg_que, &actionID, OS_MSG_BLOCK_WAIT))
        {
            switch (actionID)
            {
                case RGBLED_BLINK_WHITE:
                {
                    rgbled_set_white_blink_action();
                    break;
                }
                case RGBLED_BLINK_RED:
                {
                    rgbled_set_red_blink_action();
                    break;
                }
                case RGBLED_BLINK_GREEN:
                {
                    rgbled_set_green_blink_action();
                    break;
                }
                case RGBLED_CHECK:
                {
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_DEV, "Wrong action %d error!", actionID);
                    break;
                }
            }

            if (rgbled_action_process())
            {
                os_scheduler_delay(RGBLED_ACTION_CHECK_INTERVAL);
                rgbled_send_act_req(RGBLED_CHECK);
            }
        }
    }
}

/******************************************************************************
* Function    : rgbled_send_act_req
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void rgbled_send_act_req(RGBLED_ACTION_ID req)
{
    os_msg_queue_send(rgbled_msg_que, ( void*)&req, 0);
}

