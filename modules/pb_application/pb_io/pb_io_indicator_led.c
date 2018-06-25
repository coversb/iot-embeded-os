/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_indicator_led.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   on-board led to indicate the sys status
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-30      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "pb_util.h"
#include "pb_io_indicator_led.h"
#include "pb_ota_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define BLINK_SLOW_DURATION 800 // ms
#define BLINK_FAST_DURATION 90 // ms

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint8 pb_io_indicator_status = 0;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_io_indicator_led_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_indicator_led_set(uint8 item)
{
    switch (item)
    {
        case PB_IO_INDICATOR_SYS_NORAML:
        {
            hal_gpio_set(BOARD_LED_SYS, HAL_GPIO_LOW);
            break;
        }
        case PB_IO_INDICATOR_NET_WORKING:
        {
            hal_gpio_set(BOARD_LED_NET, HAL_GPIO_LOW);
            break;
        }
        case PB_IO_INDICATOR_SCREEN_OK:
        {
            hal_gpio_set(BOARD_LED_SCREEN, HAL_GPIO_LOW);
            break;
        }
        default:break;
    }    
    
    BIT_SET(pb_io_indicator_status, item);
}

/******************************************************************************
* Function    : pb_io_indicator_led_clear
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_indicator_led_clear(uint8 item)
{
    switch (item)
    {
        case PB_IO_INDICATOR_SYS_NORAML:
        {
            hal_gpio_set(BOARD_LED_SYS, HAL_GPIO_HIGH);
            break;
        }
        case PB_IO_INDICATOR_NET_WORKING:
        {
            hal_gpio_set(BOARD_LED_NET, HAL_GPIO_HIGH);
            break;
        }
        case PB_IO_INDICATOR_SCREEN_OK:
        {
            hal_gpio_set(BOARD_LED_SCREEN, HAL_GPIO_HIGH);
            break;
        }
        default:break;
    }    
    
    BIT_CLEAR(pb_io_indicator_status, item);
}

/******************************************************************************
* Function    : pb_io_indicator_led_normal
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_indicator_sys(void)
{
    static bool lastState = false;
    
    if (BIT_CHECK(pb_io_indicator_status, PB_IO_INDICATOR_SYS_NORAML))
    {
        hal_gpio_set(BOARD_LED_SYS, (HAL_GPIO_VAL)lastState);
        lastState = (bool)!lastState;
    }
}

/******************************************************************************
* Function    : pb_io_indicator_network
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_indicator_network(void)
{
    static bool lastState = false;
    static uint8 longLastCnt = 0;
    static uint8 blinkCnt = 0;
    
    if (BIT_CHECK(pb_io_indicator_status, PB_IO_INDICATOR_NET_WORKING))
    {
        if (pb_ota_net_get_stat() == PB_OTA_NET_CONNECTED)
        {
            longLastCnt++;
            if (longLastCnt >= 16)
            {
                hal_gpio_set(BOARD_LED_NET, HAL_GPIO_LOW);

                blinkCnt++;
                if (blinkCnt <= 6)
                {
                    hal_gpio_set(BOARD_LED_NET, (HAL_GPIO_VAL)lastState);
                    lastState = (bool)!lastState;
                }
                else
                {
                    lastState = false;
                    longLastCnt = 0;
                    blinkCnt = 0;
                }
            }
            else
            {
                hal_gpio_set(BOARD_LED_NET, HAL_GPIO_HIGH);
            }
        }
        else
        {
            hal_gpio_set(BOARD_LED_NET, (HAL_GPIO_VAL)lastState);
            lastState = (bool)!lastState;
        }
    }
}

/******************************************************************************
* Function    : pb_io_indicator_led_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_indicator_led_process(void)
{
    static uint32 lastIndicateSysTime = 0;
    static uint32 lastIndicateNetTime = 0;

    if (os_get_tick_count() - lastIndicateSysTime >= BLINK_SLOW_DURATION)
    {
        pb_io_indicator_sys();
        lastIndicateSysTime = os_get_tick_count();
    }

    if (os_get_tick_count() - lastIndicateNetTime >= BLINK_FAST_DURATION)
    {
        pb_io_indicator_network();
        lastIndicateNetTime = os_get_tick_count();
    }
}

/******************************************************************************
* Function    : pb_io_indicator_led_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_indicator_led_init(void)
{
    hal_rcc_enable(BOARD_LED_RCC);

    hal_gpio_set_mode(BOARD_LED_SYS, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_LED_NET, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_LED_SCREEN, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_LED_RESERVED, HAL_GPIO_OUT_PP);

    hal_gpio_set(BOARD_LED_RESERVED, HAL_GPIO_LOW);
    pb_io_indicator_led_clear(PB_IO_INDICATOR_SYS_NORAML);
    pb_io_indicator_led_clear(PB_IO_INDICATOR_NET_WORKING);
    pb_io_indicator_led_clear(PB_IO_INDICATOR_SCREEN_OK);
}

const PB_IO_INDICATOR_LED SYSLED = 
{
    pb_io_indicator_led_init,
    pb_io_indicator_led_set,
    pb_io_indicator_led_clear,
    pb_io_indicator_led_process
};

