/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          ac_control.c
*  author:              Chen hao
*  version:             1.00
*  file description:   air conditioner IR control
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-26      1.00                    Chen hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "board_config.h"
#include "hal_tim.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "ir_remote_control.h"
#include "ac_control.h"

/******************************************************************************
* Macros
******************************************************************************/
#define AC_TEMPERATURE_MAX 30
#define AC_TEMPERATURE_MIN 17

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static DEV_TYPE_IR_CONTROL *IR = NULL;
/******************************************************************************
* Local Functions
******************************************************************************/
/*
 media air conductor ir code definition
  0xb24d, BYTE1 ~BYTE1, BYTE2 ~BYTE2
   BYTE1 = air speed + 0xf
				high: 3
				middle: 5
				low: 9
				auto for 除湿、自动: 1
				auto for 制冷、制热: b
   BYTE2 = temperature + mode
      temperature
         30：b
         29：a
         28：8
         27：9
         26：d
         25：c
         24：4
         23：5
         22：7
         21：6
         20：2
         19：3
         18：1
         17：0
      mode
         制冷：0
         制热：c
         自动：8
         除湿：4
*/
static void ac_set(AC_MODE_TYPE mode, AC_SPEED_TYPE speed, uint8 temperature)
{
    uint8 spdByte;
    uint8 tempByte;
    uint8 modeByte;
    uint8 temperatureModeByte;
    IR_KEY_TYPE key;

    key.protocol = IR_NEC;
    //address
    key.keyData[0] = 0xb24d;

    switch(speed)
    {
        case AC_SPEED_HIGH: spdByte = 3; break;
        case AC_SPEED_MIDDLE: spdByte = 5; break;
        case AC_SPEED_LOW: spdByte = 9; break;
        case AC_SPEED_AUTO:
        {
            if (mode == AC_MODE_AUTO || mode == AC_MODE_DRY)
            {
                spdByte = 1;
            }
            else
            {
                spdByte = 0xb;
            }
            break;
        }
        default: spdByte = 5; break;
    }
    
    spdByte = spdByte << 4 | 0xf;
    if (mode == AC_MODE_WIND)
    {
        spdByte = 0xbf;
    }
    key.keyData[1] = (spdByte << 8) | ((~spdByte) & 0xFF);

    temperature = MIN_VALUE(temperature, AC_TEMPERATURE_MAX);
    temperature = MAX_VALUE(temperature, AC_TEMPERATURE_MIN);
    
    switch(temperature)
    {
        case 30: tempByte = 0xb; break;
        case 29: tempByte = 0xa; break;
        case 28: tempByte = 0x8; break;
        case 27: tempByte = 0x9; break;
        case 26: tempByte = 0xd; break;
        case 25: tempByte = 0xc; break;
        case 24: tempByte = 0x4; break;
        case 23: tempByte = 0x5; break;
        case 22: tempByte = 0x7; break;
        case 21: tempByte = 0x6; break;
        case 20: tempByte = 0x2; break;
        case 19: tempByte = 0x3; break;
        case 18: tempByte = 0x1; break;
        case 17: tempByte = 0x0; break;
        default: tempByte = 0x2; break; // 20
    }
    switch(mode)
    {
        case AC_MODE_COLD: modeByte = 0x0; break;
        case AC_MODE_HOT: modeByte = 0xc; break;
        case AC_MODE_AUTO: modeByte = 0x8; break;
        case AC_MODE_DRY: modeByte = 0x4; break;
        case AC_MODE_WIND: tempByte = 0xe; modeByte = 0x4; break;
        default: modeByte = 0x8; break;    // auto mode
    }
    temperatureModeByte = tempByte << 4 | modeByte;
    key.keyData[2] = (temperatureModeByte << 8) | ((~temperatureModeByte) & 0xff);
    key.keyData[3] = 0;

    IR->transmit(&key);
}

/******************************************************************************
* Function    : ac_control_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void ac_control_close(void)
{
    IR_KEY_TYPE key;

    key.protocol = IR_NEC;
    key.keyData[0] = 0xb24d;
    key.keyData[1] = 0x7b84;
    key.keyData[2] = 0xe01f;
    key.keyData[3] = 0;
    IR->transmit(&key);
}

/******************************************************************************
* Function    : ac_control_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool ac_control_init(const DEV_TYPE_IR_CONTROL *ir)
{
    if (ir == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "IR is invalid");
        return false;
    }
    IR = (DEV_TYPE_IR_CONTROL *)ir;
    #if defined(BOARD_STM32F1XX)
    IR->init(&tim3Pwm);
    #elif defined(BOARD_STM32F4XX)
    IR->init(&tim1Pwm);
    #else
    #error ac_control_init
    #endif
    IR->setDelay(tim2.delay);
    return true;
}

const DEV_TYPE_AC_CONTROL acCtrl = 
{
    ac_control_init,
    ac_control_close,
    ac_set
};

