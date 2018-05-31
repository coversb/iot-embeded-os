/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   inuput / output functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-25      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "hal_bkp.h"
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_util.h"
#include "pb_io_main.h"
#include "pb_io_drv.h"
#include "pb_io_alarm.h"
#include "pb_prot_main.h"
#include "pb_io_indicator_led.h"
#include "pb_cfg_proc.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_IO_CLOSE_DOOR_DELAY (6 * SEC2MSEC)
#define PB_IO_CLOSE_DEVBOX_DELAY (300 * SEC2MSEC)

/******************************************************************************
* Local Functions define
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
typedef struct
{
    OS_TMR_TYPE close_door_tmr;
    OS_TMR_TYPE close_devbox_tmr;
    bool needMonitorDoor;

}PB_IO_CONTEXT_TYPE;

static PB_IO_CONTEXT_TYPE pb_io_context;
/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_io_pwr_suply
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get power supply status
******************************************************************************/
uint8 pb_io_pwr_suply(void)
{
    if (HAL_GPIO_LOW == pb_io_drv_input_val(PB_IN_PWR_SUPPLY))
    {
        return PB_IO_PWR_OFF;
    }

    return PB_IO_PWR_ON;
}

/******************************************************************************
* Function    : pb_io_smoke_level
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get smoke sensor level
******************************************************************************/
uint8 pb_io_smoke_level(void)
{
    if (HAL_GPIO_LOW == pb_io_drv_input_val(PB_IN_SMOKE_SENSOR1)
            || HAL_GPIO_LOW == pb_io_drv_input_val(PB_IN_SMOKE_SENSOR1))
    {
        return 100;
    }

    return 0;
}

/******************************************************************************
* Function    : pb_io_door_status
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_io_door_status(void)
{
    if (HAL_GPIO_HIGH == pb_io_drv_input_val(PB_IN_DOOR_STATUS))
    {
        return PB_IO_DOOR_OPEN;
    }
    else
    {
        return PB_IO_DOOR_CLOSE;
    }
}

/******************************************************************************
* Function    : pb_io_door_monitor
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : monitor door status
******************************************************************************/
static void pb_io_door_monitor(void)
{
    static uint8 lastDoorStatus = PB_IO_DOOR_UNKNOWN;
    uint8 curDoorStatus = pb_io_door_status();

    if (true == pb_io_context.needMonitorDoor
        && lastDoorStatus != curDoorStatus)
    {
        uint8 operationType = PB_PROT_DSE_MONITOR;

        //Monitor door status to send real status
        pb_prot_send_rsp_param_req(PB_PROT_RSP_DSE, (uint8*)&operationType, sizeof(operationType));
        lastDoorStatus = curDoorStatus;
        pb_io_context.needMonitorDoor = false;
    }
}

/******************************************************************************
* Function    : pb_io_door_lock_sw
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_door_lock_sw(uint8 sw, uint8 type)
{
    pb_io_drv_gpo_set(PB_GPO_DOOR_LOCK, sw);
    pb_prot_send_rsp_param_req(PB_PROT_RSP_DSE, (uint8*)&type, sizeof(type));

    pb_io_context.needMonitorDoor = true;

    if (sw == PB_IO_DOOR_OPEN)
    {
        switch (type)
        {
            case PB_PROT_DSE_PASSWD:
            case PB_PROT_DSE_SERVER:
            case PB_PROT_DSE_EMERGNCY:
            {
                // restart door close delay timer
                os_tmr_restart(pb_io_context.close_door_tmr);
                break;
            }
            default:break;
        }
    }
}

/******************************************************************************
* Function    : pb_io_door_delay_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_door_delay_close(OS_TMR_TYPE tmr)
{
    pb_io_door_lock_sw(PB_IO_DOOR_CLOSE, PB_PROT_DSE_UNKNOW);
    
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Door close");
}

/******************************************************************************
* Function    : pb_io_dev_lock_sw
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_dev_lock_sw(uint8 sw)
{
    pb_io_drv_gpo_set(PB_GPO_DEV_LOCK, !sw);

    if (!sw)//close relay to open the lock
    {
        //restart close lock delay timer
        os_tmr_restart(pb_io_context.close_devbox_tmr);
    }
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Device box %d", !sw);
}

/******************************************************************************
* Function    : pb_io_get_output_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get current output mode
******************************************************************************/
static uint8 pb_io_get_output_mode(void)
{
    PB_CFG_OMC *pOmc = &(pb_cfg_proc_get_cmd()->omc);
    uint16 startMinuteStamp = pOmc->startHour * 60 + pOmc->startMin;
    uint16 stopMinuteStamp = pOmc->stopHour * 60 + pOmc->stopMin;
    uint8 hour;
    uint8 min;
    uint8 sec;
    pb_util_get_time(&hour, &min, &sec);
    uint8 mode = PB_IO_OUTPUT_OUT_OF_VALIDTIME;

    if ((pOmc->mode != 0) && (startMinuteStamp != stopMinuteStamp))
    {
        uint16 curMinuteStamp = hour * 60 + min;
        
        //in one day
        if (startMinuteStamp < stopMinuteStamp)
        {
            if (curMinuteStamp >= startMinuteStamp && curMinuteStamp <= stopMinuteStamp)
            {
                mode = PB_IO_OUTPUT_IN_VALIDTIME;
            }
        }
        //go through 00:00
        else
        {
            if (curMinuteStamp >= startMinuteStamp || curMinuteStamp <= stopMinuteStamp)
            {
                mode = PB_IO_OUTPUT_IN_VALIDTIME;
            }
        }        

        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "mode[%d], valid time[%02d:%02d --- %02d:%02d], cur [%02d:%02d]",
                                mode,
                                pOmc->startHour, pOmc->startMin,
                                pOmc->stopHour, pOmc->stopMin,
                                hour, min);    
    }

    return mode;
}

/******************************************************************************
* Function    : pb_io_get_output
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get current need output status
******************************************************************************/
static uint32 pb_io_get_output(void)
{
    PB_CFG_OMC *pOmc = &(pb_cfg_proc_get_cmd()->omc);
    uint32 output = 0;

    bool inService = false;
    switch (pb_io_get_output_mode())
    {
        case PB_IO_OUTPUT_OUT_OF_VALIDTIME:
        {
            if (inService)
            {
                output = pOmc->inServiceOutput;
            }
            else
            {
                output = pOmc->idleOutput;
            }
            break;
        }
        case PB_IO_OUTPUT_IN_VALIDTIME:
        {
            if (inService)
            {
                output = pOmc->validInServiceOutput;
            }
            else
            {
                output = pOmc->validIdleOutput;
            }

            break;
        }        
    }

    return output;
}

/******************************************************************************
* Function    : pb_io_send_omc_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_send_omc_rsp(void)
{
    if (pb_cfg_proc_get_cmd()->omc.mode == 2)
    {
        pb_prot_send_rsp_req(PB_PROT_RSP_OMC);
    }
}

/******************************************************************************
* Function    : pb_io_check_output
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_check_output(void)
{
    static uint32 lastOutput = 0;
    uint32 output = pb_io_get_output();

    if (lastOutput != output)
    {
        pb_io_output_set(output);
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "output [%08X]--->[%08X]", lastOutput, output);
        lastOutput = output;
    }
}

/******************************************************************************
* Function    : pb_io_devbox_delay_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_devbox_delay_close(OS_TMR_TYPE tmr)
{
    pb_io_dev_lock_sw(PB_IO_DEVBOX_CLOSE);
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Device box close");
}

/******************************************************************************
* Function    : pb_io_input_mask
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get currnet input pin state (0x00000000 - 0xFFFFFFFF)
******************************************************************************/
uint32 pb_io_input_mask(void)
{
    uint32 mask = 0;

    for (uint8 pin = PB_IN_BEGIN; pin < PB_IN_END; ++pin)
    {
        if (HAL_GPIO_LOW == pb_io_drv_input_val(pin))
        {
            BIT_CLEAR(mask, pin);
        }
        else
        {
            BIT_SET(mask, pin);
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "INPUT[%08X]", mask);
    return mask;
}

/******************************************************************************
* Function    : pb_io_output_mask
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get currnet output pin state (0x00000000 - 0xFFFFFFFF)
******************************************************************************/
uint32 pb_io_output_mask(void)
{
    uint32 mask = 0;

    for (uint8 pin = PB_OUT_BEGIN; pin < PB_OUT_END; ++pin)
    {
        if (HAL_GPIO_LOW == pb_io_drv_output_val(pin))
        {
            BIT_CLEAR(mask, pin);
        }
        else
        {
            BIT_SET(mask, pin);
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "OUTPUT[%08X]", mask);
    return mask;
}

/******************************************************************************
* Function    : pb_io_output_set_filter
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_output_set_filter(uint8 pin)
{
    if (pin == PB_OUT_RESERVED29
        || pin == PB_OUT_RESERVED30)
    {
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_io_output_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : output batch set
******************************************************************************/
void pb_io_output_set(uint32 mask)
{
    for (uint8 pin = PB_OUT_BEGIN; pin < PB_OUT_END; ++pin)
    {
        if (pb_io_output_set_filter(pin))
        {
            continue;
        }
        
        if (BIT_CHECK(mask, pin))
        {
            pb_io_drv_output_set(pin, HAL_GPIO_HIGH);
        }
        else
        {
            pb_io_drv_output_set(pin, HAL_GPIO_LOW);
        }
    }

    pb_io_send_omc_rsp();
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "OUTPUT[%08X]", mask);
}

/******************************************************************************
* Function    : pb_io_main_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : io main task init
******************************************************************************/
static void pb_io_main_init(void)
{
    pb_io_drv_init();
    pb_io_alarm_init();

    // close the door
    pb_io_drv_gpo_set(PB_GPO_DOOR_LOCK, PB_IO_DOOR_CLOSE);
    // close the dev box
    pb_io_drv_gpo_set(PB_GPO_DEV_LOCK, !PB_IO_DEVBOX_CLOSE);
    // set output

    //init door timer to delay close door
    pb_io_context.close_door_tmr = os_tmr_create(PB_IO_CLOSE_DOOR_DELAY, pb_io_door_delay_close, false);
    //init devicebox timer to delay close door
    pb_io_context.close_devbox_tmr = os_tmr_create(PB_IO_CLOSE_DEVBOX_DELAY, pb_io_devbox_delay_close, false);
    pb_io_context.needMonitorDoor = false;
}

/******************************************************************************
* Function    : pb_io_main
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : 
******************************************************************************/
void pb_io_main(void *param)
{
    pb_io_main_init();

    os_set_task_init(OS_TASK_ITEM_PB_IO);
    os_wait_task_init_sync();

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        //monitor door status
        pb_io_door_monitor();
        //check alarm
        pb_io_alarm_check();

        //check output status by config
        pb_io_check_output();

        #if 0
        //check air conditioner switch 
        pb_io_air_conditioner_sw_check();
        #endif
        os_scheduler_delay(DELAY_1_S);
    }
}

