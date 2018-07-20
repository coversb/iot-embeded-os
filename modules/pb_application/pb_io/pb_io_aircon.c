/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_aircon.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   air conditioner control
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-31      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "os_trace_log.h"
#include "pb_util.h"
#include "pb_io_drv.h"
#include "pb_cfg_proc.h"
#include "pb_io_aircon.h"
#include "pb_prot_type.h"
#include "pb_order_main.h"
#include "pb_prot_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_IO_ARICON_SNED_CMD_DELAY (15 * SEC2MSEC)
#define PB_IO_ARICON_PWROFF_DELAY (5 * MIN2MSEC)

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_TMR_TYPE delaySendCmdTmr;
static OS_TMR_TYPE delayPwroffTmr;
static PB_IO_AIRCON_STAT pwrState = PB_IO_AIRCON_CLOSE;
static uint16 airconState = 0;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_io_aircon_get_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_io_aircon_get_state(void)
{
    return airconState;
}

/******************************************************************************
* Function    : pb_io_aircon_set_state
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : Bit | 15 ... 12 | 11 ... 6 | 5 4 | 3 2 | 1 0
*                           Reserved    Temp    Wind Mode PWR
******************************************************************************/
static void pb_io_aircon_set_state(uint8 sw, uint8 workMode, uint8 windLv, uint8 temp)
{
    airconState = 0;
    //PWR
    airconState |= sw & 0x0003;
    //work mode
    airconState |= (((workMode & 0x03) << 2) & 0x000C);
    //wind
    airconState |= (((windLv & 0x03) << 4) & 0x0030);
    //temperature
    airconState |= (((temp & 0x03F) << 6) & 0x0FC0);

    pb_prot_send_rsp_req(PB_PROT_RSP_INF);
    
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO,
                            "Set aircon pwr[%d], work[%d], wind[%d], temperature[%d]",
                            sw, workMode, windLv, temp); 
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "aircon[%04X]", AIRCON.state);
}

/******************************************************************************
* Function    : pb_io_aircon_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send command to airconditioner
******************************************************************************/
static void pb_io_aircon_set(uint8 cmd)
{
    PB_CFG_ACO *pAco = &(pb_cfg_proc_get_cmd()->aco);

    switch (cmd)
    {
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBIO, "bad aircon cmd %d, close it", cmd);
        }
        case PB_IO_AIRCON_OFF:
        {
            AC_REMOTE_CTRL.close();
            break;
        }
        case PB_IO_AIRCON_ON:
        {
            if (pAco->pwrMode == PB_IO_AIRCON_OFF)
            {
                AC_REMOTE_CTRL.close();
            }
            else
            {
                AC_REMOTE_CTRL.set((AC_MODE_TYPE)pAco->workMode, (AC_SPEED_TYPE)pAco->windLevel, pAco->temperature);
            }
            break;
        }
    }

    pb_io_aircon_set_state(cmd, pAco->workMode, pAco->windLevel, pAco->temperature);
}

/******************************************************************************
* Function    : pb_io_aircon_has_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_has_order(void)
{
    bool ret = false;
    
    // has no order
    if (0 == pb_order_number())
    {
        ret =  false;
    }
    else
    // already in service
    if (PB_ORDER_OPSTAT_SERVICE == pb_order_operation_state())
    {
        ret = true;
    }
    else
    {
        PB_CFG_ACW *pAcw = &(pb_cfg_proc_get_cmd()->acw);
        uint32 curTime = pb_util_get_timestamp();
        uint32 nearestStartTime = pb_order_nearest_start_time() + PB_ORDER_START_ADJUST;

        if ((curTime < nearestStartTime)
            && (nearestStartTime - curTime <= (pAcw->duration * MIN2SEC)))
        {
            ret = true;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_io_aircon_open_by_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need open airconditioner by order
******************************************************************************/
static bool pb_io_aircon_open_by_order(void)
{
    if (pb_io_aircon_has_order())
    {
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "aircon OPEN by order");
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_io_aircon_close_by_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need close airconditioner by order
******************************************************************************/
static bool pb_io_aircon_close_by_order(void)
{
    if (!pb_io_aircon_has_order())
    {
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "aircon CLOSE by order");
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_io_aircon_in_validtime
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_in_validtime(void)
{
    PB_CFG_ACW *pAcw = &(pb_cfg_proc_get_cmd()->acw);
    bool bInValidTime = false;
    uint8 hour;
    uint8 min;
    uint8 sec;
    pb_util_get_time(&hour, &min, &sec);
    
    uint16 startMinuteStamp = pAcw->startHour * 60 + pAcw->startMin;
    uint16 stopMinuteStamp = pAcw->stopHour * 60 + pAcw->stopMin;

    if (startMinuteStamp != stopMinuteStamp)
    {
        uint16 curMinuteStamp = hour * 60 + min;
        
        //in one day
        if (startMinuteStamp < stopMinuteStamp)
        {
            if (curMinuteStamp >= startMinuteStamp && curMinuteStamp < stopMinuteStamp)
            {
                bInValidTime = true;
            }
        }
        //go through 00:00
        else
        {
            if (curMinuteStamp >= startMinuteStamp || curMinuteStamp < stopMinuteStamp)
            {
                bInValidTime = true;
            }
        }
    }

    return bInValidTime;
}

/******************************************************************************
* Function    : pb_io_aircon_open_by_validtime
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_open_by_validtime(void)
{
    if (pb_io_aircon_in_validtime())
    {
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "aircon OPEN by valid time");
        return true;
    }
    
    return false;
}

/******************************************************************************
* Function    : pb_io_aircon_close_by_validtime
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_close_by_validtime(void)
{
    if (!pb_io_aircon_in_validtime())
    {
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "aircon CLOSE by valid time");
        return true;
    }
    
    return false;
}

const static PB_IO_AIRCON_PWRON_EVENT AIRCON_PWRON_EVENT[PB_ACW_PWRON_EVENT_END] = 
{
    {pb_io_aircon_open_by_order}, 
    {pb_io_aircon_open_by_validtime}
};

const static PB_IO_AIRCON_PWROFF_EVENT AIRCON_PWROFF_EVENT[PB_ACW_PWROFF_EVENT_END] = 
{
    {pb_io_aircon_close_by_order}, 
    {pb_io_aircon_close_by_validtime}
};

/******************************************************************************
* Function    : pb_io_aircon_get_pwr_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_need_open(void)
{
    bool ret = false;
    PB_CFG_ACW *pAcw = &(pb_cfg_proc_get_cmd()->acw);

    if (pAcw->mode == PB_ACW_PWR_OFF)
    {
        return false;
    }

    for (uint8 idx = PB_ACW_PWRON_EVENT_BEGIN; idx < PB_ACW_PWRON_EVENT_END; ++idx)
    {
        if (BIT_CHECK(pAcw->pwrOnEventMask, idx))
        {
            if (AIRCON_PWRON_EVENT[idx].needOpen())
            {
                ret = true;
            }
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_io_aircon_need_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_io_aircon_need_close(void)
{
    bool ret = true;
    PB_CFG_ACW *pAcw = &(pb_cfg_proc_get_cmd()->acw);

    if (pAcw->mode == PB_ACW_PWR_OFF)
    {
        return true;
    }

    for (uint8 idx = PB_ACW_PWROFF_EVENT_BEGIN; idx < PB_ACW_PWROFF_EVENT_END; ++idx)
    {
        if (BIT_CHECK(pAcw->pwrOffEventMask, idx))
        {
            if (!AIRCON_PWROFF_EVENT[idx].needClose())
            {
                ret = false;
            }
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_io_aircon_delay_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_delay_set(OS_TMR_TYPE tmr)
{
    pb_io_aircon_set(PB_IO_AIRCON_ON);
}

/******************************************************************************
* Function    : pb_io_aircon_pwroff
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_pwroff(OS_TMR_TYPE tmr)
{
    //open air conditioner power by output
    pb_io_drv_output_set(PB_OUT_AIR_CON1, HAL_GPIO_LOW);
    pb_io_drv_output_set(PB_OUT_AIR_CON2, HAL_GPIO_LOW);
    
    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "pwr off air conditioner");
}

/******************************************************************************
* Function    : pb_io_aircon_delay_pwroff
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_delay_pwroff(void)
{
    //close air control timer
    os_tmr_stop(delaySendCmdTmr);

    //close air conditioner by ir
    pb_io_aircon_set(PB_IO_AIRCON_OFF);

    //start delay power off timer
    os_tmr_restart(delayPwroffTmr);
}

/******************************************************************************
* Function    : pb_io_aircon_pwron
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_pwron(void)
{
    //stop power off delay timer
    os_tmr_stop(delayPwroffTmr);
    
    //open air conditioner power by output
    pb_io_drv_output_set(PB_OUT_AIR_CON1, HAL_GPIO_HIGH);
    pb_io_drv_output_set(PB_OUT_AIR_CON2, HAL_GPIO_HIGH);

    //start timer, and in the timer it will configurate by ir
    os_tmr_restart(delaySendCmdTmr);
}

/******************************************************************************
* Function    : pb_io_aircon_set_pwr_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_set_pwr_state(PB_IO_AIRCON_STAT state)
{
    pwrState = state;
}

/******************************************************************************
* Function    : pb_io_aircon_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_process(void)
{    
    switch (pwrState)
    {
        case PB_IO_AIRCON_OPEN:
        {
            if (pb_io_aircon_need_close())
            {
                pb_io_aircon_delay_pwroff();

                pwrState = PB_IO_AIRCON_CLOSE;
                OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Delay close air conditioner");
            }
            break;
        }
        case PB_IO_AIRCON_CLOSE:
        case PB_IO_AIRCON_NEED_RESTART:
        default:
        {
            if (pb_io_aircon_need_open())
            {
                pb_io_aircon_pwron();
                
                pwrState = PB_IO_AIRCON_OPEN;
                OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Open air conditioner");
            }
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_io_aircon_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_aircon_init(void)
{
    delaySendCmdTmr = os_tmr_create(PB_IO_ARICON_SNED_CMD_DELAY, pb_io_aircon_delay_set, false);
    delayPwroffTmr = os_tmr_create(PB_IO_ARICON_PWROFF_DELAY, pb_io_aircon_pwroff, false);

    pb_io_aircon_set(PB_IO_AIRCON_OFF);
}

const PB_IO_AIRCON AIRCON = 
{
    pb_io_aircon_get_state,
    pb_io_aircon_init,
    pb_io_aircon_process,
    pb_io_aircon_set,
    pb_io_aircon_set_pwr_state
};

