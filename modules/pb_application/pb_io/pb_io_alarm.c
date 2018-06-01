/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_alarm.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   io alarm manager
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-29      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "hal_bkp.h"
#include "os_trace_log.h"
#include "pb_util.h"
#include "pb_io_alarm.h"
#include "pb_io_drv.h"
#include "pb_io_main.h"
#include "pb_prot_main.h"
#include "pb_cfg_proc.h"
#include "pb_multimedia.h"
#include "pb_io_aircon.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint32 lastSmaAudioTime = 0;

/******************************************************************************
* Local Functions
******************************************************************************/
/*power supply alarm*/
/******************************************************************************
* Function    : pb_io_pwr_alarm_need_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need check power alarm
******************************************************************************/
static bool pb_io_pwr_alarm_need_check(void)
{
    return true;
}

/******************************************************************************
* Function    : pb_io_pwr_alarm_get_debounce_duration
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get power alarm debounce
******************************************************************************/
static uint32 pb_io_pwr_alarm_get_debounce_duration(void)
{
    return PB_IO_PWR_ALARM_DEBOUNCE;
}

/******************************************************************************
* Function    : pb_io_pwr_alarm_is_detected
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check is power alarm detected
******************************************************************************/
static bool pb_io_pwr_alarm_is_detected(void)
{
    return (bool)(pb_io_pwr_suply() == PB_IO_PWR_OFF);
}

/******************************************************************************
* Function    : pb_io_pwr_alarm_trigger_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_pwr_alarm_trigger_handler(void)
{
    //turn on the emengrcy light
    pb_io_drv_output_set(PB_OUT_EMERGENCY_LIGHT, HAL_GPIO_HIGH);
    //send alarm RSP
    uint8 powerType = PB_PWR_ALARM_MAIN_LOST;
    pb_prot_send_rsp_param_req(PB_PROT_RSP_PSE, (uint8*)&powerType, sizeof(powerType));
}

/******************************************************************************
* Function    : pb_io_pwr_alarm_triggering_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_pwr_alarm_triggering_handler(uint32 *lastRecordTime, uint32 curTime)
{
    //check emergency light status
    if (pb_io_drv_output_val(PB_OUT_EMERGENCY_LIGHT) != HAL_GPIO_HIGH)
    {
        //turn on the emengrcy light
        pb_io_drv_output_set(PB_OUT_EMERGENCY_LIGHT, HAL_GPIO_HIGH);
    }
    //send alarm RSP in each 60 seconds
    if ((curTime - *lastRecordTime) >= PB_IO_PWR_ALARM_INTERVAL)
    {
        *lastRecordTime = curTime;
        //send alarm RSP
        uint8 powerType = PB_PWR_ALARM_MAIN_LOST;
        pb_prot_send_rsp_param_req(PB_PROT_RSP_PSE, (uint8*)&powerType, sizeof(powerType));
    }
}

/******************************************************************************
* Function    : pb_io_pwr_alarm_relieve_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_pwr_alarm_relieve_handler(void)
{
    //reset the output
    pb_io_output_restore();
    //reset the air conditioner
    AIRCON.setState(PB_IO_AIRCON_NEED_RESTART);
    
    //send alarm relieved RSP
    uint8 powerType = PB_PWR_ALARM_MAIN_RECOVER;
    pb_prot_send_rsp_param_req(PB_PROT_RSP_PSE, (uint8*)&powerType, sizeof(powerType));
}

/*smoke supply alarm*/
/******************************************************************************
* Function    : pb_io_smoke_alarm_need_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need check smoke alarm
******************************************************************************/
static bool pb_io_smoke_alarm_need_check(void)
{
    if (pb_cfg_proc_get_cmd()->sma.mode == 0)
    {
        return false;
    }
    return true;
}

/******************************************************************************
* Function    : pb_io_smoke_alarm_get_debounce_duration
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get power alarm debounce
******************************************************************************/
static uint32 pb_io_smoke_alarm_get_debounce_duration(void)
{
    return pb_cfg_proc_get_cmd()->sma.duration;
}

/******************************************************************************
* Function    : pb_io_smoke_alarm_is_detected
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check is smoke alarm detected
******************************************************************************/
static bool pb_io_smoke_alarm_is_detected(void)
{
    return (bool)(pb_io_smoke_level() >= pb_cfg_proc_get_cmd()->sma.threshold);
}

/******************************************************************************
* Function    : pb_io_smoke_alarm_trigger_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_smoke_alarm_trigger_handler(void)
{
    //open door
    pb_io_door_lock_sw(PB_IO_DOOR_OPEN, PB_PROT_DSE_SMA);

    //play alarm audio
    pb_multimedia_send_audio_msg(PB_MM_PLAY_SMOKE_ALARM, 0);
    lastSmaAudioTime = pb_util_get_timestamp();

    //send alarm RSP
    pb_prot_send_sae_req(PB_SENSOR_ALARM_SMOKE, pb_io_smoke_level());
}

/******************************************************************************
* Function    : pb_io_smoke_alarm_triggering_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_smoke_alarm_triggering_handler(uint32 *lastRecordTime, uint32 curTime)
{
    //repeat alarm audio and open door
    if (curTime - lastSmaAudioTime > PB_IO_SMOKE_AUDIO_INTERVAL)
    {
        //open door
        pb_io_door_lock_sw(PB_IO_DOOR_OPEN, PB_PROT_DSE_SMA);

        //play alarm audio
        pb_multimedia_send_audio_msg(PB_MM_PLAY_SMOKE_ALARM, 0);
        lastSmaAudioTime = pb_util_get_timestamp();
    }
    
    //in mode 2 need send alarm RSP in interval
    if (pb_cfg_proc_get_cmd()->sma.mode == 2 && pb_cfg_proc_get_cmd()->sma.interval != 0)
    {
        if ((curTime - *lastRecordTime) >= pb_cfg_proc_get_cmd()->sma.interval)
        {
            *lastRecordTime = curTime;
            
            //send alarm RSP
            pb_prot_send_sae_req(PB_SENSOR_ALARM_SMOKE, pb_io_smoke_level());
        }
    }
}

/******************************************************************************
* Function    : pb_io_smoke_alarm_relieve_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_smoke_alarm_relieve_handler(void)
{
    //close door
    pb_io_door_lock_sw(PB_IO_DOOR_CLOSE, PB_PROT_DSE_SMA);

    //send alarm RSP
    pb_prot_send_sae_req(PB_SENSOR_ALARM_SMOKE, pb_io_smoke_level());
}

/*door alarm*/
/******************************************************************************
* Function    : pb_io_door_alarm_need_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need check door alarm
******************************************************************************/
static bool pb_io_door_alarm_need_check(void)
{
    if (pb_cfg_proc_get_cmd()->doa.mode == 0)
    {
        return false;
    }
    return true;
}

/******************************************************************************
* Function    : pb_io_door_alarm_get_debounce_duration
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get door alarm debounce
******************************************************************************/
static uint32 pb_io_door_alarm_get_debounce_duration(void)
{
    return pb_cfg_proc_get_cmd()->doa.duration * MIN2SEC;
}

/******************************************************************************
* Function    : pb_io_door_alarm_is_detected
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check is smoke alarm detected
******************************************************************************/
static bool pb_io_door_alarm_is_detected(void)
{
    return (bool)(pb_io_door_status() == pb_cfg_proc_get_cmd()->doa.triggerType);
}

/******************************************************************************
* Function    : pb_io_door_alarm_trigger_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_door_alarm_trigger_handler(void)
{
    //send alarm RSP
    pb_prot_send_sae_req(PB_SENSOR_ALARM_DOOR, pb_io_door_status());
}

/******************************************************************************
* Function    : pb_io_door_alarm_triggering_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_door_alarm_triggering_handler(uint32 *lastRecordTime, uint32 curTime)
{  
    //in mode 2 need send alarm RSP in interval
    if (pb_cfg_proc_get_cmd()->doa.mode == 2 && pb_cfg_proc_get_cmd()->doa.interval != 0)
    {
        if ((curTime - *lastRecordTime) >= pb_cfg_proc_get_cmd()->doa.interval)
        {
            *lastRecordTime = curTime;
            
            //send alarm RSP
            pb_prot_send_sae_req(PB_SENSOR_ALARM_DOOR, pb_io_door_status());
        }
    }
}

/******************************************************************************
* Function    : pb_io_door_alarm_relieve_handler
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_door_alarm_relieve_handler(void)
{
    //send alarm RSP
    pb_prot_send_sae_req(PB_SENSOR_ALARM_DOOR, pb_io_door_status());
}

/*alarm item define*/
PB_IO_ALARM pb_io_alarm_item[PB_IO_ALARM_END] = 
{
    {
        "PWR", PB_IO_ALARM_STATE_PWR, PB_IO_ALARM_RELIEVED, 0, 
        pb_io_pwr_alarm_need_check,
        pb_io_pwr_alarm_get_debounce_duration,
        pb_io_pwr_alarm_is_detected,
        pb_io_pwr_alarm_trigger_handler,
        pb_io_pwr_alarm_triggering_handler,
        pb_io_pwr_alarm_relieve_handler
    },
    {
        "SMA", PB_IO_ALARM_STATE_SMOKE, PB_IO_ALARM_RELIEVED, 0, 
        pb_io_smoke_alarm_need_check,
        pb_io_smoke_alarm_get_debounce_duration,
        pb_io_smoke_alarm_is_detected,
        pb_io_smoke_alarm_trigger_handler,
        pb_io_smoke_alarm_triggering_handler,
        pb_io_smoke_alarm_relieve_handler
    },
    {
        "DOA", PB_IO_ALARM_STATE_DOOR, PB_IO_ALARM_RELIEVED, 0, 
        pb_io_door_alarm_need_check,
        pb_io_door_alarm_get_debounce_duration,
        pb_io_door_alarm_is_detected,
        pb_io_door_alarm_trigger_handler,
        pb_io_door_alarm_triggering_handler,
        pb_io_door_alarm_relieve_handler
    }
};

/******************************************************************************
* Function    : pb_io_get_alarm_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get alarm state from bpk register
******************************************************************************/
static uint16 pb_io_get_alarm_state(void)
{
    uint16 stat = hal_bkp_read(BOARD_BKP_DEV_ADDR);
    uint16 crc = hal_bkp_read(BOARD_BKP_DEV_CRC_ADDR);

    uint16 calCrc = pb_util_get_crc16((uint8*)&stat, sizeof(stat));

    if (calCrc != crc)
    {
        OS_DBG_ERR(DBG_MOD_PBIO, "alarm stat crc[%04X] err, need[%04X]", calCrc, crc);
        return 0xFFFF;
    }

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "Get devstat[%04X], crc[%04X]", stat, crc);

    return stat;
}

/******************************************************************************
* Function    : pb_io_set_alarm_state
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : save alarm state to bpk register
******************************************************************************/
static void pb_io_set_alarm_state(PB_IO_ALARM_STATE_ITEM item, PB_IO_ALARM_STATUS val)
{
    uint16 stat = hal_bkp_read(BOARD_BKP_DEV_ADDR);

    if (val == PB_IO_ALARM_RELIEVED)
    {
        BIT_CLEAR(stat, item);
    }
    else
    {
        BIT_SET(stat, item);
    }

    uint16 crc = pb_util_get_crc16((uint8*)&stat, sizeof(stat));

    hal_bkp_write(BOARD_BKP_DEV_ADDR, stat);
    hal_bkp_write(BOARD_BKP_DEV_CRC_ADDR, crc);

    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "set[%d:%d], devstat[%04X], crc[%04X]", item, val, stat, crc);
}

/******************************************************************************
* Function    : pb_io_alarm_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check alarm
******************************************************************************/
void pb_io_alarm_check(void)
{
    uint32 curTime = 0;
    
    for (uint8 idx = PB_IO_ALARM_BEGIN; idx < PB_IO_ALARM_END; ++idx)
    {
        if (!pb_io_alarm_item[idx].needCheck())
        {
            continue;
        }

        curTime = pb_util_get_timestamp();
        if (pb_io_alarm_item[idx].isDetected())
        {
            OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "%s alarm detected", pb_io_alarm_item[idx].name);

            //haven't triggered alarm, check debounce
            if (PB_IO_ALARM_RELIEVED == pb_io_alarm_item[idx].alarmStatus)
            {
                if (curTime - pb_io_alarm_item[idx].lastRecordTime >= pb_io_alarm_item[idx].getDebounceDuration())
                {
                    pb_io_alarm_item[idx].lastRecordTime = curTime;
                    pb_io_alarm_item[idx].alarmStatus = PB_IO_ALARM_TRIGGERED;
                    pb_io_set_alarm_state(pb_io_alarm_item[idx].item, PB_IO_ALARM_TRIGGERED);
                    pb_io_alarm_item[idx].triggerHandler();
                    
                    OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "%s alarm triggered", pb_io_alarm_item[idx].name);
                }
            }
            else
            {
                pb_io_alarm_item[idx].triggeringHandler(&(pb_io_alarm_item[idx].lastRecordTime), curTime);
            }
        }
        else
        {
            if (PB_IO_ALARM_TRIGGERED == pb_io_alarm_item[idx].alarmStatus)
            {
                pb_io_alarm_item[idx].alarmStatus = PB_IO_ALARM_RELIEVED;
                pb_io_set_alarm_state(pb_io_alarm_item[idx].item, PB_IO_ALARM_RELIEVED);
                pb_io_alarm_item[idx].relieveHandler();

                OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "%s alarm relieved", pb_io_alarm_item[idx].name);
            }
            pb_io_alarm_item[idx].lastRecordTime = curTime;
        }
    }
}

/******************************************************************************
* Function    : pb_io_alarm_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_alarm_init(void)
{
    uint16 alarmState = pb_io_get_alarm_state();
    for (uint8 idx = PB_IO_ALARM_BEGIN; idx < PB_IO_ALARM_END; ++idx)
    {
        if ((bool)BIT_CHECK(alarmState, idx))
        {
            pb_io_alarm_item[idx].alarmStatus = PB_IO_ALARM_TRIGGERED;
        }
        else
        {
            pb_io_alarm_item[idx].alarmStatus = PB_IO_ALARM_RELIEVED;
        }
        pb_io_alarm_item[idx].lastRecordTime = pb_util_get_timestamp();
        OS_DBG_TRACE(DBG_MOD_PBIO, DBG_INFO, "load %s[%d][%u]", 
                                pb_io_alarm_item[idx].name, 
                                pb_io_alarm_item[idx].alarmStatus,
                                pb_io_alarm_item[idx].lastRecordTime);
    }
}

