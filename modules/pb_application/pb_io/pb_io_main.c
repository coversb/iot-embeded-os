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
#include "pb_io_aircon.h"
#include "pb_order_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_IO_CLOSE_DOOR_DELAY (6 * SEC2MSEC)
#define PB_IO_CLOSE_DEVBOX_DELAY (5*MIN2MSEC)   // 5 mins
#define PB_IO_REBOOT_TV_DELAY (3*MIN2MSEC)  // 3 mins

#define PB_IO_EXHAUST_FORCE_MODE1_OPEN_DURATION (10*MIN2SEC) // 10 mins
#define PB_IO_EXHAUST_FORCE_MODE2_OPEN_DURATION (5*MIN2SEC) // 5mins
#define PB_IO_EXHAUST_FORCE_MODE2_CLOSE_DURATION (15*MIN2SEC) // 15mins

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
    OS_TMR_TYPE reboot_tv_tmr;
}PB_IO_CONTEXT_TYPE;

static PB_IO_CONTEXT_TYPE pb_io_context;
/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_io_aircon_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get airconditioner state
******************************************************************************/
uint16 pb_io_aircon_state(void)
{
    return AIRCON.state();
}

/******************************************************************************
* Function    : pb_io_aircon_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : update airconditioner state
******************************************************************************/
void pb_io_aircon_update(void)
{
    PB_CFG_ACO *pAco = &(pb_cfg_proc_get_cmd()->aco);

    switch (pAco->pwrMode)
    {
        case PB_ACO_MODE_AUTO:
        {
            break;
        }
        case PB_ACO_MODE_ON:
        {
            AIRCON.set(PB_IO_AIRCON_ON);
            break;
        }
        case PB_ACO_MODE_OFF:
        default:
        {
            AIRCON.set(PB_IO_AIRCON_OFF);
            break;
        }
    }
}

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
            || HAL_GPIO_LOW == pb_io_drv_input_val(PB_IN_SMOKE_SENSOR2))
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
    uint8 openState = HAL_GPIO_HIGH;
    if (pb_cfg_proc_get_cmd()->doa.reverse)
    {
        openState = HAL_GPIO_LOW;
    }
    
    if (openState == pb_io_drv_input_val(PB_IN_DOOR_STATUS))
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

    if (lastDoorStatus != curDoorStatus)
    {
        uint8 operationType = PB_PROT_DSE_MONITOR;

        //Monitor door status to send real status
        pb_prot_send_rsp_param_req(PB_PROT_RSP_DSE, (uint8*)&operationType, sizeof(operationType));
        lastDoorStatus = curDoorStatus;
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
    bool inService = (bool)(PB_ORDER_OPSTAT_CLOSE != pb_order_operation_state());
    
    switch (pb_io_get_output_mode())
    {
        default:
        {
            //go through to PB_IO_OUTPUT_OUT_OF_VALIDTIME
            OS_DBG_ERR(DBG_MOD_PBIO, "OUTPUT MODE ERROR!");
        }
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
    if (pb_cfg_proc_get_cmd()->omc.mode == PB_OMC_MODE_SPECIAL_TIME_WITH_RSP)
    {
        pb_prot_send_rsp_req(PB_PROT_RSP_OMC);
    }
}

/******************************************************************************
* Function    : pb_io_tv_delay_reboot
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_tv_delay_reboot(OS_TMR_TYPE tmr)
{
    pb_io_drv_output_set(PB_OUT_INDOOR_TV, HAL_GPIO_HIGH);
    pb_io_send_omc_rsp();
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Reboot tv open");
}

/******************************************************************************
* Function    : pb_io_check_tv_output
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check tv pwr supply to reboot tv, make sure tv app works fine
******************************************************************************/
static void pb_io_check_tv_output(uint32 lastOutput, uint32 output)
{
    static bool lastInService = false;
    
    if (!pb_cfg_proc_get_cmd()->omc.tvRebootSw)
    {
        return;
    }
    bool inService = (bool)(PB_ORDER_OPSTAT_CLOSE != pb_order_operation_state());

    // in-service to close
    if ((true == lastInService) && (false == inService))
    {
        // tv from open to open
        if ((0 != BIT_CHECK(lastOutput, PB_OUT_INDOOR_TV)) 
            && (0 != BIT_CHECK(output, PB_OUT_INDOOR_TV)))
        {
            pb_io_drv_output_set(PB_OUT_INDOOR_TV, HAL_GPIO_LOW);
            os_tmr_restart(pb_io_context.reboot_tv_tmr);
            pb_io_send_omc_rsp();
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "tv reboot, [%d--->%d]", lastInService, inService);
        }
    }
    lastInService = inService;
}

/******************************************************************************
* Function    : pb_io_tv_reboot_sw
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_tv_reboot_sw(uint8 sw)
{
    PB_CFG_OMC *cfgOmc = &(pb_cfg_proc_get_cmd()->omc);

    if (sw != cfgOmc->tvRebootSw)
    {
        cfgOmc->tvRebootSw = sw;
        pb_cfg_proc_save_cmd(PB_PROT_CMD_OMC, cfgOmc, sizeof(PB_CFG_OMC));
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Save TV reboot[%d]", cfgOmc->tvRebootSw);
    }
}

/******************************************************************************
* Function    : pb_io_exhaust_force_mode1
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_exhaust_force_mode1(bool *isOpen, uint32 *curTimestamp, uint32 *lastTimestamp)
{
    bool isSwitch = false;
    
    if (*isOpen)
    {
        if (*curTimestamp - *lastTimestamp >= PB_IO_EXHAUST_FORCE_MODE1_OPEN_DURATION)
        {
            // need close
            pb_io_drv_output_set(PB_OUT_EXHAUST, HAL_GPIO_LOW);
            *isOpen = false;
            isSwitch = true;
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "exhaust force1 close");
        }
    }
    else
    {
        uint8 h;
        uint8 m;
        uint8 s;
        pb_util_timestamp_to_time(*curTimestamp, &h, &m, &s);
        if (m >= 50) //HH:50 ~ HH:00
        {
            // need open
            pb_io_drv_output_set(PB_OUT_EXHAUST, HAL_GPIO_HIGH);
            *isOpen = true;
            isSwitch = true;
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "exhaust force1 open");
        }
    }

    if (isSwitch)
    {
        *lastTimestamp = *curTimestamp;
        pb_io_send_omc_rsp();
    }
}

/******************************************************************************
* Function    : pb_io_exhaust_force_mode2
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_exhaust_force_mode2(bool *isOpen, uint32 *curTimestamp, uint32 *lastTimestamp)
{
    bool isSwitch = false;

    if (*isOpen)
    {
        if (*curTimestamp - *lastTimestamp >= PB_IO_EXHAUST_FORCE_MODE2_OPEN_DURATION)
        {
            // need close
            pb_io_drv_output_set(PB_OUT_EXHAUST, HAL_GPIO_LOW);
            *isOpen = false;
            isSwitch = true;
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "exhaust force2 close");
        }
    }
    else
    {
        if (*curTimestamp - *lastTimestamp >= PB_IO_EXHAUST_FORCE_MODE2_CLOSE_DURATION)
        {
            // need open
            pb_io_drv_output_set(PB_OUT_EXHAUST, HAL_GPIO_HIGH);
            *isOpen = true;
            isSwitch = true;
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "exhaust force2 open");
        }
    }

    if (isSwitch)
    {
        *lastTimestamp = *curTimestamp;
        pb_io_send_omc_rsp();
    }
}

/******************************************************************************
* Function    : pb_io_exhaust_force_mode_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_exhaust_force_mode_check(uint32 curOutput, uint32 lastOutput)
{
    static uint32 lastUpdateTimestamp = 0;
    static bool isExhaustOpen = false;
    
    bool inService = (bool)(PB_ORDER_OPSTAT_CLOSE != pb_order_operation_state());
    uint8 exhaustForceMode = pb_cfg_proc_get_cmd()->omc.exhaustForceMode;
    
    if (PB_OMC_EXHAUST_FORCE_MODE_OFF == exhaustForceMode)
    {
        return;
    }

    uint32 curTimestamp = pb_util_get_timestamp();

    // record state when the output switch
    if (lastOutput != curOutput)
    {
        if (0 != BIT_CHECK(curOutput, PB_OUT_EXHAUST))
        {
            isExhaustOpen = true;
        }
        else
        {
            isExhaustOpen = false;
        }
        lastUpdateTimestamp = curTimestamp;
    }

    if (false == inService
        || (0 != BIT_CHECK(pb_cfg_proc_get_cmd()->omc.inServiceOutput, PB_OUT_EXHAUST))
        || (0 != BIT_CHECK(pb_cfg_proc_get_cmd()->omc.validInServiceOutput, PB_OUT_EXHAUST)))
    {
        return;
    }

    switch (exhaustForceMode)
    {
        case PB_OMC_EXHAUST_FORCE_MODE1:
        {
            pb_io_exhaust_force_mode1(&isExhaustOpen, &curTimestamp, &lastUpdateTimestamp);
            break;
        }
        case PB_OMC_EXHAUST_FORCE_MODE2:
        {
            pb_io_exhaust_force_mode2(&isExhaustOpen, &curTimestamp, &lastUpdateTimestamp);
            break;
        }
        default:break;
    }
}

/******************************************************************************
* Function    : pb_io_exhaust_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_exhaust_mode(uint8 mode)
{
    PB_CFG_OMC *cfgOmc = &(pb_cfg_proc_get_cmd()->omc);

    if (mode != cfgOmc->exhaustForceMode)
    {
        cfgOmc->exhaustForceMode = mode;
        pb_cfg_proc_save_cmd(PB_PROT_CMD_OMC, cfgOmc, sizeof(PB_CFG_OMC));
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Save exhaust force mode[%d]", cfgOmc->exhaustForceMode);
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
        pb_io_check_tv_output(lastOutput, output);
    }
    pb_io_exhaust_force_mode_check(output, lastOutput);

    // update here, in case make the output set order above wrong
    if (lastOutput != output)
    {
        lastOutput = output;
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "output [%08X]--->[%08X]", lastOutput, output);
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

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Get input[%08X]", mask);
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

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Get output[%08X]", mask);
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
    if (pin == PB_OUT_AIR_CON1
        || pin == PB_OUT_AIR_CON2)
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
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Set output[%08X]", mask);
}

/******************************************************************************
* Function    : pb_io_output_restore
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_output_restore(void)
{
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Restore output");
    pb_io_output_set(pb_io_get_output());
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
    AIRCON.init();

    // close the door
    pb_io_drv_gpo_set(PB_GPO_DOOR_LOCK, PB_IO_DOOR_CLOSE);
    // close the dev box
    pb_io_drv_gpo_set(PB_GPO_DEV_LOCK, !PB_IO_DEVBOX_CLOSE);
    // set output

    //init door timer to delay close door
    pb_io_context.close_door_tmr = os_tmr_create(PB_IO_CLOSE_DOOR_DELAY, pb_io_door_delay_close, false);
    //init devicebox timer to delay close door
    pb_io_context.close_devbox_tmr = os_tmr_create(PB_IO_CLOSE_DEVBOX_DELAY, pb_io_devbox_delay_close, false);
    //init tv reboot timer to delay open tv
    pb_io_context.reboot_tv_tmr = os_tmr_create(PB_IO_REBOOT_TV_DELAY, pb_io_tv_delay_reboot, false);
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
        //check air conditioner switch 
        AIRCON.process();
        
        os_scheduler_delay(DELAY_1_S);
    }
}

