/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          ir_remote_control.c
*  author:
*  version:             1.00
*  file description:   ir related driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-26      1.00
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

/******************************************************************************
* Macros
******************************************************************************/
#if 1
#define NEC_ACTIVE_INTERVAL 560
#define NEC_ZERO_INTERVAL 560
#define NEC_ONE_INTERVAL 1690
#else
#define NEC_ACTIVE_INTERVAL 600
#define NEC_ZERO_INTERVAL 470
#define NEC_ONE_INTERVAL 1620
#endif

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static HAL_TIM_PWM_TYPE *IR_PWM = NULL;
static IR_USDELAY USDELAY = NULL;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : ir_remote_control_transmit_bit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void ir_remote_control_transmit_bit(uint16 duration)
{
    IR_PWM->enable();
    USDELAY(duration);
    IR_PWM->generateEvent();
    IR_PWM->disable();
}

/******************************************************************************
* Function    : ir_remote_control_nec
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void ir_remote_control_nec(uint16 *data)
{
    uint16 bitmask;
    unsigned portBASE_TYPE uxPriority;
    uxPriority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, tskIDLE_PRIORITY + 5);

    ir_remote_control_transmit_bit(4340);
    USDELAY(4250);
    for (uint8 idx = 0; idx < IR_KEY_LENGTH_MAX; ++idx)
    {
        if (data[idx] == 0)
        {
            break;
        }

        bitmask = 0x8000;
        for(uint8 bitIdx = 0; bitIdx < 16; ++bitIdx)
        {
            ir_remote_control_transmit_bit(NEC_ACTIVE_INTERVAL);

            if (data[idx] & bitmask)
            {
                USDELAY(NEC_ONE_INTERVAL);
            }
            else
            {
                USDELAY(NEC_ZERO_INTERVAL);
            }
            bitmask = bitmask >> 1;
        }
    }
    
    ir_remote_control_transmit_bit(NEC_ACTIVE_INTERVAL);
    USDELAY(5050);

    ir_remote_control_transmit_bit(4340);
    USDELAY(4250);
    for (uint8 idx = 0; idx < IR_KEY_LENGTH_MAX; ++idx)
    {
        if (data[idx] == 0)
        {
            break;
        }
        
        bitmask = 0x8000;
        for(uint8 bitIdx = 0; bitIdx < 16; ++bitIdx)
        {
            ir_remote_control_transmit_bit(NEC_ACTIVE_INTERVAL);

            if (data[idx] & bitmask)
            {
                USDELAY(NEC_ONE_INTERVAL);
            }
            else
            {
                USDELAY(NEC_ZERO_INTERVAL);
            }
            bitmask = bitmask >> 1;
        }
    }
    ir_remote_control_transmit_bit(NEC_ACTIVE_INTERVAL);

    vTaskPrioritySet(NULL, uxPriority);
}

/******************************************************************************
* Function    : ir_remote_control_transmit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : need set high pority when transmit, make sure the timing is right
******************************************************************************/
static void ir_remote_control_transmit(IR_KEY_TYPE *key)
{
    if (IR_PWM == NULL || USDELAY == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "IR need init");
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, 
                            "IR transmit[%d]{%04X %04X %04X %04X}",
                            key->protocol, key->keyData[0], key->keyData[1], key->keyData[2], key->keyData[3]);


    switch (key->protocol)
    {
        case IR_NEC:
        {
            ir_remote_control_nec(key->keyData);
            break;
        }
        default:break;
    }
}

/******************************************************************************
* Function    : ir_remote_control_set_delay
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void ir_remote_control_set_delay(IR_USDELAY delay)
{
    if (delay == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "IR delay is invalid");
        return;
    }
    USDELAY = delay;
}

/******************************************************************************
* Function    : ir_remote_control_deinit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void ir_remote_control_deinit(void)
{
    IR_PWM->deinit();
    IR_PWM = NULL;
    USDELAY = NULL;
}

/******************************************************************************
* Function    : ir_remote_control_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool ir_remote_control_init(const HAL_TIM_PWM_TYPE *pwm)
{
    if (pwm == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "IR pwm is invalid");
        return false;
    }
    IR_PWM = (HAL_TIM_PWM_TYPE *)pwm;
    IR_PWM->init();
    return true;
}

const DEV_TYPE_IR_CONTROL devIRCtrl = 
{
    ir_remote_control_init,
    ir_remote_control_deinit,
    ir_remote_control_set_delay,
    ir_remote_control_transmit
};

