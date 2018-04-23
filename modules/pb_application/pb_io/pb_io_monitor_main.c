/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_monitor_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   polling input source
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_io_monitor_main.h"
#include "pb_util.h"
#include "pb_prot_main.h"
#include "pb_gui_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_NPW_LEN 4
#define PB_SPW_LEN 8
#define PB_PWB_LEN 32
#define PB_PW_TIMEOUT 10//seconds
#define PB_KEY_CANCEL 0x1B
#define PB_KEY_QUERY 0x0D

/******************************************************************************
* Local Functions define
******************************************************************************/
static void pb_io_monitor_action(uint8 type);

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_IO_MONITOR_ITEM pb_io_monitor_items[PB_IO_MONITOR_END] = 
{
    /*PIN define,           trigger type,    default state,    action*/
    {BOARD_EMERGENCY_BTN,   HAL_GPIO_LOW,    HAL_GPIO_HIGH,    pb_io_monitor_action},
    {BOARD_KEY_REVERSE,	    HAL_GPIO_LOW,    HAL_GPIO_HIGH,    pb_io_monitor_action},
    {BOARD_KEY_MENU,        HAL_GPIO_LOW,    HAL_GPIO_HIGH,    pb_io_monitor_action},
    {BOARD_KEY_VOLUME_UP,   HAL_GPIO_LOW,    HAL_GPIO_HIGH,    pb_io_monitor_action},
    {BOARD_KEY_VOLUME_DOWN, HAL_GPIO_LOW,    HAL_GPIO_HIGH,    pb_io_monitor_action}
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_io_monitor_debug_com
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : monitor debug com input
******************************************************************************/
static void pb_io_monitor_debug_com(void)
{
    if (PB_DEBUG_COM.available() > 0)
    {
        pb_prot_input_available_set(PB_PROT_SRC_UART, true);
        pb_prot_send_msg_to_prot_mod(PB_MSG_PROT_ANALYSE_DATA);
    }
}

/******************************************************************************
* Function    : pb_io_monitor_keyboard
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : monitor password keyboard
******************************************************************************/
static void pb_io_monitor_keyboard(void)
{
    static uint8 keyOffset = 0;
    static uint8 keyBuff[PB_PWB_LEN];
    static uint32 lastTime = 0;
    
    uint8 byte;
    uint32 curTime;
    uint8 queryKey[PB_PWB_LEN];

    curTime = pb_util_get_timestamp();

    while (PB_KEYBOARD.available() > 0)
    {
        if (curTime - lastTime > PB_PW_TIMEOUT)
        {
            keyOffset = 0;
            memset(keyBuff, 0, sizeof(keyBuff));
        }
        lastTime = curTime;

        byte = PB_KEYBOARD.read();
        switch (byte)
        {
            case PB_KEY_CANCEL:
            {
                keyOffset = 0;
                memset(keyBuff, 0, sizeof(keyBuff));
                break;
            }

            case PB_KEY_QUERY:
            {
                if (keyOffset == PB_SPW_LEN)
                {
                    memset(queryKey, 0, sizeof(queryKey));
                    memcpy(queryKey, keyBuff, keyOffset);
                    OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "Check spw:%d", atoi((char*)queryKey));
                    /*
                    pb_order_check_password(atoi(queryKey));
                    */
                }
                keyOffset = 0;
                memset(keyBuff, 0, sizeof(keyBuff));

                break;
            }
            default:
            {
                keyBuff[keyOffset++] = byte;
                if ((keyOffset % PB_NPW_LEN == 0) && (keyOffset > 0))
                {
                    memset(queryKey, 0, sizeof(queryKey));
                    memcpy(queryKey, &keyBuff[keyOffset - PB_NPW_LEN], PB_NPW_LEN);
                    OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "Check npw:%d", atoi((char*)queryKey));
                    /*
                    pb_order_check_password(atoi(queryKey));
                    */
                    if (keyOffset > PB_SPW_LEN)
                    {
                        keyOffset = 0;
                        memset(keyBuff, 0, sizeof(keyBuff));
                    }
                }
                break;
            }
        }

        OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "KEY:%s", keyBuff);
    }
}

/******************************************************************************
* Function    : pb_io_monitor_action
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_monitor_action(uint8 type)
{
    switch (type)
    {
        case PB_IO_MONITOR_EMERGENCY:
        {
            OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "EMERGENCY btn triggered");
            break;
        }
        case PB_IO_MONITOR_REVERSE:
        {
            OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "REVERSE btn triggered");
            pb_gui_send_act_req(PB_GUI_ACT_REVERSE);
            break;
        }
        case PB_IO_MONITOR_MENU:
        {
            OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "MENU btn triggered");
            pb_gui_send_act_req(PB_GUI_ACT_MENU);
            break;
        }
        case PB_IO_MONITOR_VOLUME_UP:
        {
            OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "VOLUME_UP btn triggered");
            pb_gui_send_act_req(PB_GUI_ACT_VOLUP);
            break;
        }
        case PB_IO_MONITOR_VOLUME_DOWN:
        {
            OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "VOLUME_DOWN btn triggered");
            pb_gui_send_act_req(PB_GUI_ACT_VOLDOWN);
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBIO_MONITOR, "Unknown action");
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_io_monitor_pins
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : monitor on-board key
******************************************************************************/
static void pb_io_monitor_pins(void)
{
    for (uint8 idx = PB_IO_MONITOR_BEGIN; idx < PB_IO_MONITOR_END; ++idx)
    {
        HAL_GPIO_VAL curStat = hal_gpio_val(pb_io_monitor_items[idx].GPIOx, pb_io_monitor_items[idx].GPIO_Pin);

        if (pb_io_monitor_items[idx].lastStat != curStat)
        {
            if (curStat == pb_io_monitor_items[idx].triggerType)
            {
                if (pb_io_monitor_items[idx].function != NULL)
                {
                    pb_io_monitor_items[idx].function(idx);
                }
                OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "%d btn triggered", idx);
            }
            else
            {
                OS_DBG_TRACE(DBG_MOD_PBIO_MONITOR, DBG_INFO, "%d btn released", idx);
            }
            pb_io_monitor_items[idx].lastStat = curStat;
        }
    }
}

/******************************************************************************
* Function    : pb_io_monitor_main_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : io mnitor task init
******************************************************************************/
static void pb_io_monitor_init(void)
{
    //init password keyboard
    PB_KEYBOARD.begin(9600);
    //init onboard key
    hal_rcc_enable(BOARD_KEY_RCC);
    hal_gpio_set_mode(BOARD_KEY_REVERSE, GPIO_Mode_IN_FLOATING);
    hal_gpio_set_mode(BOARD_KEY_MENU, GPIO_Mode_IN_FLOATING);
    hal_gpio_set_mode(BOARD_KEY_VOLUME_UP, GPIO_Mode_IN_FLOATING);
    hal_gpio_set_mode(BOARD_KEY_VOLUME_DOWN, GPIO_Mode_IN_FLOATING);
    //init emergency button
    hal_rcc_enable(BOARD_EMERGENCY_BTN_RCC);
    hal_gpio_set_mode(BOARD_EMERGENCY_BTN, GPIO_Mode_IN_FLOATING);
}

/******************************************************************************
* Function    : pb_io_monitor_main
*
* Author      : Chen Hao
*
* Parameters  : command sub-type and args
*
* Return      :
*
* Description : control gpio releated operations
******************************************************************************/
void pb_io_monitor_main(void *param)
{
    pb_io_monitor_init();

    os_set_task_init(OS_TASK_ITEM_PB_IOMONITOR);
    os_wait_task_init_sync();

    while (1)
    {
        pb_io_monitor_pins();
        pb_io_monitor_keyboard();
        pb_io_monitor_debug_com();

        os_scheduler_delay(PB_IO_MONITOR_INTERVAL);
    }
}

