/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_prot_proc.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   process park box air protocol
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "pb_prot_proc.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "os_middleware.h"
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_util.h"
#include "pb_cfg_proc.h"
#include "pb_fota.h"
#include "pb_cfg_default.h"
#include "pb_prot_main.h"
#include "pb_multimedia.h"
#include "pb_ota_main.h"
#include "pb_io_drv.h"
#include "pb_io_main.h"
#include "pb_order_main.h"
#include "pb_crypto.h"
#include "pb_order_hotp.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_DBI_RSP_DELAY (10 * SEC2MSEC)
#define PB_PROT_SACK_CNTOUT 2

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint16 pbProtReportCnt;
//protocol watch dog
static bool bDogNeedUpdate = false;
//server config
static bool bSerNeedUpdate = false;
//global config
static bool bCfgNeedUpdate = false;
//sack check count
static uint8 unAckHBPCnt = 0;

static OS_TMR_TYPE device_basic_rsp_tmr = NULL;
static PB_PROT_RSP_GSMINFO_PARAM pb_gsm_info;
static PB_PROT_RSP_LOC_PARAM pb_location;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_prot_proc_get_report_sn
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_prot_proc_get_report_sn(void)
{
    return pbProtReportCnt++;
}

/******************************************************************************
* Function    : pb_prot_proc_set_dog_update
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_proc_set_dog_update(void)
{
    bDogNeedUpdate = true;
}

/******************************************************************************
* Function    : pb_prot_proc_watchdog_check_time
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_proc_watchdog_check_time(uint16 randomSec)
{
    uint8 hour;
    uint8 min;
    uint8 sec;
    pb_util_get_time(&hour, &min, &sec);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "DOG need[%02d:%02d:%02d], cur[%02d:%02d:%02d]",
                            pb_cfg_proc_get_cmd()->dog.rstHour, pb_cfg_proc_get_cmd()->dog.rstMinute, 
                            randomSec,
                            hour, min, sec);

    if ((hour == pb_cfg_proc_get_cmd()->dog.rstHour) && (min >= pb_cfg_proc_get_cmd()->dog.rstMinute)
        && (sec >= randomSec))
    {
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_proc_watchdog_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_proc_watchdog_check(void)
{
    static uint16 randomSec = 0;
    static uint32 recordTime = 0;
    static bool bFirstCheck = true;
    static bool bRebooting = false;

    if (pb_cfg_proc_get_cmd()->dog.mode == 0
        || pb_cfg_proc_get_cmd()->dog.interval == 0
        || bRebooting)
    {
        return;
    }

    uint32 curTime = pb_util_get_timestamp();
    if (bFirstCheck || bDogNeedUpdate)
    {
        recordTime = curTime;
        randomSec = pb_util_random_num(pb_cfg_proc_get_cmd()->dog.randomDuration);
        bFirstCheck = false;
        bDogNeedUpdate = false;
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "DOG update");
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "DOG interval[%d], lasting[%d], random[%d]",
                            (DAY2SEC * pb_cfg_proc_get_cmd()->dog.interval),
                            curTime - recordTime,
                            randomSec);
    //check interval
    if (curTime - recordTime >=  (DAY2SEC * pb_cfg_proc_get_cmd()->dog.interval))
    {
        //check watchdog reboot time
        if (pb_prot_proc_watchdog_check_time(randomSec))
        {
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "DOG need reboot");
            //send dog reboot RSP
            if (pb_cfg_proc_get_cmd()->dog.report)
            {
                pb_prot_send_rsp_req(PB_PROT_RSP_DOG);
            }
            //send power off RSP
            pb_prot_send_rsp_req(PB_PROT_RSP_PFE);
            pb_ota_need_set_reboot(true);
            bRebooting = true;
        }
    }
}

/******************************************************************************
* Function    : pb_prot_proc_set_ser_update
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_proc_set_ser_update(void)
{
    bSerNeedUpdate = true;
}

/******************************************************************************
* Function    : pb_prot_proc_hbp_send_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_proc_hbp_send_check(void)
{
    static uint32 recordTime = 0;
    static bool bFirstCheck = true;

    if (pb_cfg_proc_get_cmd()->ser.hbpInterval == 0)
    {
        return;
    }

    uint32 curTime = pb_util_get_timestamp();
    if (bFirstCheck || bSerNeedUpdate)
    {
        recordTime = curTime;
        bFirstCheck = false;
        bSerNeedUpdate = false;
        pb_prot_send_msg_to_prot_mod(PB_MSG_PROT_SEND_HBP);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "HBP update interval[%d] mins", pb_cfg_proc_get_cmd()->ser.hbpInterval);
    }

    //check interval
    if (curTime - recordTime >=  (pb_cfg_proc_get_cmd()->ser.hbpInterval * MIN2SEC))
    {
        recordTime = curTime;
        pb_prot_send_msg_to_prot_mod(PB_MSG_PROT_SEND_HBP);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send HBP");
    }
}

/******************************************************************************
* Function    : pb_prot_proc_set_cfg_update
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_proc_set_cfg_update(void)
{
    bCfgNeedUpdate = true;
}

/******************************************************************************
* Function    : pb_prot_proc_inf_send_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_proc_inf_send_check(void)
{
    static uint32 recordTime = 0;
    static bool bFirstCheck = true;

    if (pb_cfg_proc_get_cmd()->cfg.infInterval == 0)
    {
        return;
    }

    uint32 curTime = pb_util_get_timestamp();
    if (bFirstCheck || bCfgNeedUpdate)
    {
        recordTime = curTime;
        bFirstCheck = false;
        bCfgNeedUpdate = false;
        pb_prot_send_rsp_req(PB_PROT_RSP_INF);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "INF update interval[%d]secs", pb_cfg_proc_get_cmd()->cfg.infInterval);
    }

    //check interval
    if (curTime - recordTime >=  pb_cfg_proc_get_cmd()->cfg.infInterval)
    {
        recordTime = curTime;
        pb_prot_send_rsp_req(PB_PROT_RSP_INF);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Send INF");
    }
}

/******************************************************************************
* Function    : pb_prot_proc_update_rtc
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_update_rtc(uint32 timestamp)
{
    static uint8 needUpdateCnt = 0;
    static uint32 lastTimestamp = 0;
    uint32 localTimestamp = 0;
    
    if (pb_cfg_proc_get_cmd()->tma.mode == 0)
    {
        return;
    }

    if (timestamp <= lastTimestamp)
    {
        return;
    }
    
    localTimestamp = pb_util_get_timestamp();

    if ((int32)(timestamp - localTimestamp) >= PB_TMA_AUTO_SET_RANGE
        || (int32)(timestamp - localTimestamp) <= -PB_TMA_AUTO_SET_RANGE)
    {
        needUpdateCnt++;
    }

    if (needUpdateCnt >= PB_TMA_AUTO_SET_DEBOUNCE)
    {
        needUpdateCnt = 0;
        pb_util_set_timestamp(timestamp);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_ERR, "UPDATE RTC %u ---> %u", localTimestamp, timestamp);
    }

    lastTimestamp = timestamp;
}

/******************************************************************************
* Function    : pb_prot_proc_send_dbi
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_proc_send_dbi(OS_TMR_TYPE tmr)
{
    pb_prot_send_rsp_req(PB_PROT_RSP_DBI);
}

/******************************************************************************
* Function    : pb_prot_proc_device_basic_info_process
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_prot_proc_device_basic_info_process(void)
{
    //pb_ota_send_msg_data_to_ota_mod(PB_MSG_OTA_CELL_LOCATION_REQ, false);

    if (device_basic_rsp_tmr == NULL)
    {
        device_basic_rsp_tmr = os_tmr_create(PB_PROT_DBI_RSP_DELAY, pb_prot_proc_send_dbi, false);
    }
    os_tmr_restart(device_basic_rsp_tmr);
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_apc
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save apc configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_apc(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_APC_ARG *argApc = &(parsedFrame->arg.apc);
    PB_CFG_APC *cfgApc = &(pb_cfg_proc_get_cmd()->apc);
        
    //APN
    if (0 != strncmp((char *)argApc->apn, (char *)cfgApc->apn, PB_APN_LEN))
    {
        memcpy(cfgApc->apn, argApc->apn, PB_APN_LEN);
        cfgApc->apn[PB_APN_LEN] = '\0';

        needSave = true;
    }

    //APN user name
    if (0 != strncmp((char *)argApc->usr, (char *)cfgApc->usr, PB_APN_USR_LEN))
    {
        memcpy(cfgApc->usr, argApc->usr, PB_APN_USR_LEN);
        cfgApc->usr[PB_APN_USR_LEN] = '\0';

        needSave = true;
    }

    //APN password
    if (0 != strncmp((char *)argApc->pass, (char *)cfgApc->pass, PB_APN_PASS_LEN))
    {
        memcpy(cfgApc->pass, argApc->pass, PB_APN_PASS_LEN);
        cfgApc->pass[PB_APN_PASS_LEN] = '\0';

        needSave = true;
    }

    //Main DNS server
    if (0 != memcmp(argApc->mainDNS, cfgApc->mainDNS, PB_APN_DNS_LEN))
    {
        memcpy(cfgApc->mainDNS, argApc->mainDNS, PB_APN_DNS_LEN);

        needSave = true;
    }

    //Backup DNS server
    if (0 != memcmp(argApc->bkDNS, cfgApc->bkDNS, PB_APN_DNS_LEN))
    {
        memcpy(cfgApc->bkDNS, argApc->bkDNS, PB_APN_DNS_LEN);

        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgApc, sizeof(PB_CFG_APC));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save APC[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_ser
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save ser configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_ser(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_SER_ARG *argSer = &(parsedFrame->arg.ser);
    PB_CFG_SER *cfgSer = &(pb_cfg_proc_get_cmd()->ser);

    //Report mode
    if (argSer->mode != cfgSer->mode)
    {
        cfgSer->mode = argSer->mode;
        needSave = true;
    }

    //Main server domain name
    if ((0 != strlen((char *)argSer->mainServer))
            && (0 != strncmp((char *)argSer->mainServer, (char *)cfgSer->mainServer, PB_SER_DOMAINNAME_LEN)))
    {
        memcpy(cfgSer->mainServer, argSer->mainServer, PB_SER_DOMAINNAME_LEN);
        cfgSer->mainServer[PB_SER_DOMAINNAME_LEN] = '\0';

        needSave = true;
    }

    //Main server port
    if (argSer->mainPort != cfgSer->mainPort)
    {
        cfgSer->mainPort = argSer->mainPort;
        needSave = true;
    }

    //Backup server domain
    if ((0 != strlen((char *)argSer->bakServer))
            && (0 != strncmp((char *)argSer->bakServer, (char *)cfgSer->bakServer, PB_SER_DOMAINNAME_LEN)))
    {
        memcpy(cfgSer->bakServer, argSer->bakServer, PB_SER_DOMAINNAME_LEN);
        cfgSer->bakServer[PB_SER_DOMAINNAME_LEN] = '\0';

        needSave = true;
    }

    //Backup server port
    if (argSer->bakPort != cfgSer->bakPort)
    {
        cfgSer->bakPort = argSer->bakPort;
        needSave = true;
    }

    //SMS gateway
    if ((0 != strlen((char *)argSer->smsGateway))
        && (0 != strncmp((char *)argSer->smsGateway, (char *)cfgSer->smsGateway, PB_SER_SMS_GATEWAY_LEN)))
    {
        memcpy(cfgSer->smsGateway, argSer->smsGateway, PB_SER_SMS_GATEWAY_LEN);
        cfgSer->smsGateway[PB_SER_SMS_GATEWAY_LEN] = '\0';

        needSave = true;
    }

    //Heartbeat interval
    if (argSer->hbpInterval != cfgSer->hbpInterval)
    {
        cfgSer->hbpInterval = argSer->hbpInterval;
        needSave = true;
    }

    //Max random time
    if (argSer->randomTime != cfgSer->randomTime)
    {
        cfgSer->randomTime = argSer->randomTime;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgSer, sizeof(PB_CFG_SER));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save SER[%02X]", parsedFrame->msgSubType);

        pb_prot_proc_set_ser_update();
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_cfg
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save cfg configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_cfg(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_CFG_ARG *argCfg = &(parsedFrame->arg.cfg);
    PB_CFG_CFG *cfgCfg = &(pb_cfg_proc_get_cmd()->cfg);

    //Event mask
    if (argCfg->eventMask != cfgCfg->eventMask)
    {
        cfgCfg->eventMask = argCfg->eventMask;
        needSave = true;
    }

    //Info report interval
    if (argCfg->infInterval != cfgCfg->infInterval)
    {
        cfgCfg->infInterval = argCfg->infInterval;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgCfg, sizeof(PB_CFG_CFG));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save CFG[%02X]", parsedFrame->msgSubType);

        pb_prot_proc_set_cfg_update();

        //When disable GEN ACK, set unAckHBPCnt to 0
        if (!pb_prot_check_event(PB_PROT_ACK_GEN))
        {
            pb_prot_proc_set_sack_cnt(false);
        }
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_tma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save tma configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_tma(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_TMA_ARG *argTma = &(parsedFrame->arg.tma);
    PB_CFG_TMA *cfgTma = &(pb_cfg_proc_get_cmd()->tma);

    //Auto adjust mode
    if (argTma->mode != cfgTma->mode)
    {
        cfgTma->mode = argTma->mode;
        needSave = true;
    }

    //Time adjust sign
    if (argTma->sign != cfgTma->sign)
    {
        cfgTma->sign = argTma->sign;
        needSave = true;
    }

    //Time adjust hour
    if (argTma->hour != cfgTma->hour)
    {
        cfgTma->hour = argTma->hour;
        needSave = true;
    }

    //Time adjust hour
    if (argTma->minute != cfgTma->minute)
    {
        cfgTma->minute = argTma->minute;
        needSave = true;
    }

    //Time adjust daylightSaving
    if (argTma->daylightSaving != cfgTma->daylightSaving)
    {
        cfgTma->daylightSaving = argTma->daylightSaving;
        needSave = true;
    }

    //update RTC
    if (0 != argTma->timestamp)
    {
        pb_util_set_timestamp(argTma->timestamp);
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgTma, sizeof(PB_CFG_TMA));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save TMA[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_dog
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save dog configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_dog(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_DOG_ARG *argDog = &(parsedFrame->arg.dog);
    PB_CFG_DOG *cfgDog = &(pb_cfg_proc_get_cmd()->dog);

    //Work mode
    if (argDog->mode != cfgDog->mode)
    {
        cfgDog->mode = argDog->mode;
        needSave = true;
    }

    //Report before reboot
    if (argDog->report != cfgDog->report)
    {
        cfgDog->report = argDog->report;
        needSave = true;
    }

    //Report interval
    if (argDog->interval != cfgDog->interval)
    {
        cfgDog->interval = argDog->interval;
        needSave = true;
    }

    //Reboot time hour
    if (argDog->rstHour != cfgDog->rstHour)
    {
        cfgDog->rstHour = argDog->rstHour;
        needSave = true;
    }

    //Reboot time minute
    if (argDog->rstMinute != cfgDog->rstMinute)
    {
        cfgDog->rstMinute = argDog->rstMinute;
        needSave = true;
    }

    //Max random time
    if (argDog->randomDuration != cfgDog->randomDuration)
    {
        cfgDog->randomDuration = argDog->randomDuration;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgDog, sizeof(PB_CFG_DOG));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save DOG[%02X]", parsedFrame->msgSubType);

        pb_prot_proc_set_dog_update();
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_aco
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save aco configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_aco(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_ACO_ARG *argAco = &(parsedFrame->arg.aco);
    PB_CFG_ACO *cfgAco = &(pb_cfg_proc_get_cmd()->aco);

    //Mode power
    if (argAco->pwrMode != cfgAco->pwrMode)
    {
        cfgAco->pwrMode = argAco->pwrMode;
        needSave = true;
    }

    //Mode work
    if (argAco->workMode != cfgAco->workMode)
    {
        cfgAco->workMode = argAco->workMode;
        needSave = true;
    }

    //Mode wind level
    if (argAco->windLevel != cfgAco->windLevel)
    {
        cfgAco->windLevel = argAco->windLevel;
        needSave = true;
    }

    //Interval
    if (argAco->interval != cfgAco->interval)
    {
        cfgAco->interval = argAco->interval;
        needSave = true;
    }

    //Duration
    if (argAco->duration != cfgAco->duration)
    {
        cfgAco->duration = argAco->duration;
        needSave = true;
    }

    //Temperature
    if (argAco->temperature != cfgAco->temperature)
    {
        cfgAco->temperature = argAco->temperature;
        needSave = true;
    }

    //pb_io_air_conditioner_control(true);

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgAco, sizeof(PB_CFG_ACO));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save ACO[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_sec
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save sec configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_sec(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    bool needUpdateComKey = false;
    bool needUpdatePwKey = false;
    PB_PROT_CMD_SEC_ARG *argSec = &(parsedFrame->arg.sec);
    PB_CFG_SEC *cfgSec = &(pb_cfg_proc_get_cmd()->sec);

    switch (argSec->keyType)
    {
        case PB_SEC_KEY_OFFLINE_NORMAL:
        {
            if (0 != memcmp(argSec->key, cfgSec->normalKey, PB_SEC_KEY_LEN))
            {
                memcpy(cfgSec->normalKey, argSec->key, PB_SEC_KEY_LEN);
                needSave = true;
                needUpdatePwKey = true;
            }
            break;
        }
        case PB_SEC_KEY_OFFLINE_SERVICE:
        {
            if (0 != memcmp(argSec->key, cfgSec->serviceKey, PB_SEC_KEY_LEN))
            {
                memcpy(cfgSec->serviceKey, argSec->key, PB_SEC_KEY_LEN);
                needSave = true;
                needUpdatePwKey = true;
            }
            break;
        }
        case PB_SEC_KEY_AES:
        {
            if (0 != memcmp(argSec->key, cfgSec->comKey, PB_SEC_KEY_LEN))
            {
                memcpy(cfgSec->comKey, argSec->key, PB_SEC_KEY_LEN);
                needSave = true;
                needUpdateComKey = true;
            }
            break;
        }
        default:break;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(PB_PROT_CMD_SEC, cfgSec, sizeof(PB_CFG_SEC));

        if (needUpdatePwKey)
        {
            //update hotp key
            //pb_hotp_set_update(true);
        }
        else
        if (needUpdateComKey)
        {
            pb_crypto_set_key(cfgSec->comKey);
        }

        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save SEC[%d][%02X]", argSec->keyType, parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_save_hotp_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_prot_proc_save_hotp_mode(uint8 hotpMode)
{
    PB_CFG_SEC *cfgSec = &(pb_cfg_proc_get_cmd()->sec);

    if (hotpMode != cfgSec->hotpMode)
    {
        cfgSec->hotpMode = hotpMode;
        pb_cfg_proc_save_cmd(PB_PROT_CMD_SEC, cfgSec, sizeof(PB_CFG_SEC));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save HOTP Mode[%d]", cfgSec->hotpMode);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_omc
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_prot_proc_cmd_exec_omc(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_OMC_ARG *argOmc = &(parsedFrame->arg.omc);
    PB_CFG_OMC *cfgOmc = &(pb_cfg_proc_get_cmd()->omc);

    //Idle output
    if (argOmc->idleOutput != cfgOmc->idleOutput)
    {
        cfgOmc->idleOutput = argOmc->idleOutput;
        needSave = true;
    }
    
    //In-service output
    if (argOmc->inServiceOutput != cfgOmc->inServiceOutput)
    {
        cfgOmc->inServiceOutput = argOmc->inServiceOutput;
        needSave = true;
    }

    //Mode
    if (argOmc->mode != cfgOmc->mode)
    {
        cfgOmc->mode = argOmc->mode;
        needSave = true;
    }

    //Valid time
    if (argOmc->startHour != cfgOmc->startHour)
    {
        cfgOmc->startHour = argOmc->startHour;
        needSave = true;
    }
    if (argOmc->startMin != cfgOmc->startMin)
    {
        cfgOmc->startMin = argOmc->startMin;
        needSave = true;
    }
    if (argOmc->stopHour != cfgOmc->stopHour)
    {
        cfgOmc->stopHour = argOmc->stopHour;
        needSave = true;
    }
    if (argOmc->stopMin != cfgOmc->stopMin)
    {
        cfgOmc->stopMin = argOmc->stopMin;
        needSave = true;
    }

    //Valid time idle output
    if (argOmc->validIdleOutput != cfgOmc->validIdleOutput)
    {
        cfgOmc->validIdleOutput = argOmc->validIdleOutput;
        needSave = true;
    }

    //Valid time in-service output
    if (argOmc->validInServiceOutput != cfgOmc->validInServiceOutput)
    {
        cfgOmc->validInServiceOutput = argOmc->validInServiceOutput;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgOmc, sizeof(PB_CFG_OMC));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save OMC[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_acw
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_prot_proc_cmd_exec_acw(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_ACW_ARG *argAcw = &(parsedFrame->arg.acw);
    PB_CFG_ACW *cfgAcw = &(pb_cfg_proc_get_cmd()->acw);

    //mode
    if (argAcw->mode != cfgAcw->mode)
    {
        cfgAcw->mode = argAcw->mode;
        needSave = true;
    }

    //pwrOnEventMask
    if (argAcw->pwrOnEventMask != cfgAcw->pwrOnEventMask)
    {
        cfgAcw->pwrOnEventMask = argAcw->pwrOnEventMask;
        needSave = true;
    }
    
    //pwrOffEventMask
    if (argAcw->pwrOffEventMask != cfgAcw->pwrOffEventMask)
    {
        cfgAcw->pwrOffEventMask = argAcw->pwrOffEventMask;
        needSave = true;
    }

    //Duration
    if (argAcw->duration != cfgAcw->duration)
    {
        cfgAcw->duration= argAcw->duration;
        needSave = true;
    }

    //Valid time
    if (argAcw->startHour != cfgAcw->startHour)
    {
        cfgAcw->startHour = argAcw->startHour;
        needSave = true;
    }
    if (argAcw->startMin != cfgAcw->startMin)
    {
        cfgAcw->startMin = argAcw->startMin;
        needSave = true;
    }
    if (argAcw->stopHour != cfgAcw->stopHour)
    {
        cfgAcw->stopHour = argAcw->stopHour;
        needSave = true;
    }
    if (argAcw->stopMin != cfgAcw->stopMin)
    {
        cfgAcw->stopMin = argAcw->stopMin;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgAcw, sizeof(PB_CFG_ACW));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save ACW[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_doa
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_prot_proc_cmd_exec_doa(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_DOA_ARG *argDoa = &(parsedFrame->arg.doa);
    PB_CFG_DOA *cfgDoa = &(pb_cfg_proc_get_cmd()->doa);

    //Mode
    if (argDoa->mode != cfgDoa->mode)
    {
        cfgDoa->mode = argDoa->mode;
        needSave = true;
    }

    //Trigger type
    if (argDoa->triggerType != cfgDoa->triggerType)
    {
        cfgDoa->triggerType = argDoa->triggerType;
        needSave = true;
    }

    //Duration
    if (argDoa->duration != cfgDoa->duration)
    {
        cfgDoa->duration = argDoa->duration;
        needSave = true;
    }

    //Send interval
    if (argDoa->interval != cfgDoa->interval)
    {
        cfgDoa->interval = argDoa->interval;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgDoa, sizeof(PB_CFG_DOA));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save DOA[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_sma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check and save sma configuration
******************************************************************************/
static void pb_prot_proc_cmd_exec_sma(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    bool needSave = false;
    PB_PROT_CMD_SMA_ARG *argSma = &(parsedFrame->arg.sma);
    PB_CFG_SMA *cfgSma = &(pb_cfg_proc_get_cmd()->sma);

    //Mode
    if (argSma->mode != cfgSma->mode)
    {
        cfgSma->mode = argSma->mode;
        needSave = true;
    }

    //Threshold
    if (argSma->threshold != cfgSma->threshold)
    {
        cfgSma->threshold = argSma->threshold;
        needSave = true;
    }

    //Duration
    if (argSma->duration != cfgSma->duration)
    {
        cfgSma->duration = argSma->duration;
        needSave = true;
    }

    //Send interval
    if (argSma->interval != cfgSma->interval)
    {
        cfgSma->interval = argSma->interval;
        needSave = true;
    }

    if (needSave)
    {
        //save command
        pb_cfg_proc_save_cmd(parsedFrame->msgSubType, cfgSma, sizeof(PB_CFG_SMA));
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Save SMA[%02X]", parsedFrame->msgSubType);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_sma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : exec out command
******************************************************************************/
static void pb_prot_proc_cmd_exec_out(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    PB_PROT_CMD_OUT_ARG *argOut = &(parsedFrame->arg.out);

    if (argOut->pinIdx != 0)
    {
        if (argOut->pinState == 0)
        {
            pb_io_drv_output_set(argOut->pinIdx - 1, HAL_GPIO_LOW);
        }
        else
        {
            pb_io_drv_output_set(argOut->pinIdx - 1, HAL_GPIO_HIGH);
        }

        if (pb_cfg_proc_get_cmd()->omc.mode == PB_OMC_MODE_SPECIAL_TIME_WITH_RSP)
        {
            pb_prot_send_rsp_req(PB_PROT_RSP_OMC);
        }
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "OUT%d[%d]", argOut->pinIdx, argOut->pinState);
    }
    else
    {
        pb_io_output_set(argOut->ctrlMask);
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "OUT[%08X]", argOut->ctrlMask);
    }
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_sma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : exec muo command
******************************************************************************/
static void pb_prot_proc_cmd_exec_muo(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    PB_PROT_CMD_MUO_ARG *argMuo = &(parsedFrame->arg.muo);
    PB_CFG_MUO *cfgMuo = &(pb_cfg_proc_get_cmd()->muo);

    if (argMuo->type != PB_MUO_SOUND)
    {
        //ONLY SUPPORT SOUND NOW
        return;
    }

    switch (argMuo->act)
    {
        case PB_MUO_ACT_STOP:
        {
            pb_multimedia_send_audio_msg(PB_MM_STOP, 0);
            break;
        }
        case PB_MUO_ACT_PLAY:
        {
            uint8 audioCmd = 0xFF;
            switch (argMuo->fileIdx)
            {
                case PB_MUO_FILE_WELCOME:
                {
                    audioCmd = PB_MM_PLAY_WELCOME;
                    break;
                }
                case PB_MUO_FILE_OVER:
                {
                    audioCmd = PB_MM_PLAY_ORDER_OVER;
                    break;
                }
                case PB_MUO_FILE_SMOKE_ALARM:
                {
                    audioCmd = PB_MM_PLAY_SMOKE_ALARM;
                    break;
                }
                case PB_MUO_FILE_BGM:
                {
                    audioCmd = PB_MM_PLAY_BGM;
                    break;
                }
                default: break;
            }
            
            if (audioCmd != 0xFF)
            {
                pb_multimedia_send_audio_msg(audioCmd, 0);
            }
            break;
        }
        case PB_MUO_ACT_VOL_DOWN:
        {
            pb_multimedia_send_audio_msg(PB_MM_VOL_DOWN, 0);
            break;
        }
        case PB_MUO_ACT_VOL_UP:
        {
            pb_multimedia_send_audio_msg(PB_MM_VOL_UP, 0);
            break;
        }
        case PB_MUO_ACT_SET_VOL:
        {
            pb_multimedia_send_audio_msg(PB_MM_SET_VOL, argMuo->volume);
            break;
        }
        case PB_MUO_ACT_AUTO_BGM_OFF:
        case PB_MUO_ACT_AUTO_BGM_ON:
        {
            //save auto bgm mode and exec
            if (argMuo->act != cfgMuo->autoBGM)
            {
                cfgMuo->autoBGM = argMuo->act;
                pb_cfg_proc_save_cmd(PB_PROT_CMD_MUO, cfgMuo, sizeof(PB_CFG_MUO));
                OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Set auto BGM [%d]", argMuo->act);

                if (cfgMuo->autoBGM == PB_MUO_ACT_AUTO_BGM_ON)
                {
                    pb_order_control_bgm();
                }
                else
                {
                    pb_multimedia_send_audio_msg(PB_MM_STOP, 0);
                }
            }
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Unknown muo act type %d", argMuo->act);
            break;
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "execute multimedia cmd");
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_sma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : exec rto command
******************************************************************************/
static void pb_prot_proc_cmd_exec_rto(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    PB_PROT_CMD_RTO_ARG *argRto = &(parsedFrame->arg.rto);

    switch (argRto->cmd)
    {
        case PB_RTO_INF:
        {
            pb_prot_send_rsp_req(PB_PROT_RSP_INF);
            break;
        }
        case PB_RTO_VER:
        {
            uint8 cmdType = argRto->cmd;
            pb_prot_send_rsp_param_req(PB_PROT_RSP_RTO, (uint8*)&cmdType, sizeof(cmdType));
            break;
        }
        case PB_RTO_REBOOT:
        {
            pb_prot_send_rsp_req(PB_PROT_RSP_PFE);
            pb_ota_need_set_reboot(true);
            break;
        }
        case PB_RTO_RESET:
        {
            pb_cfg_proc_reset_all_cmd();
            uint8 cmdType = argRto->cmd;
            pb_prot_send_rsp_param_req(PB_PROT_RSP_RTO, (uint8*)&cmdType, sizeof(cmdType));
            break;
        }
        case PB_RTO_DOOR_SW:
        {
            pb_order_send_verify_req(PB_ORDER_VERIFY_SERVER, argRto->subCmd);
            break;
        }
        case PB_RTO_LOCATION:
        {
            //pb_ota_send_msg_data_to_ota_mod(PB_MSG_OTA_CELL_LOCATION_REQ, true);
            break;
        }
        case PB_RTO_GSMINFO:
        {
            pb_prot_send_rsp_req(PB_PROT_RSP_GSM);
            break;
        }
        case PB_RTO_DEVICEBOX_SW:
        {
            pb_io_dev_lock_sw(argRto->subCmd);
            break;
        }
        case PB_RTO_DEV_INFO_REQ:
        {
            pb_prot_proc_device_basic_info_process();
            break;
        }
        case PB_RTO_DEV_CFG_REQ:
        {
            uint8 subCmdType = argRto->subCmd;
            pb_prot_send_rsp_param_req(PB_PROT_RSP_CFG, (uint8*)&subCmdType, sizeof(subCmdType));
            break;
        }
        case PB_RTO_DEV_PC_CAL:
        {
            //pb_dev_pc_calibrate(argRto->subCmd);
            break;
        }
        case PB_RTO_HOTP_SW:
        {
            pb_prot_proc_save_hotp_mode(argRto->subCmd);
            break;            
        }
        default:break;
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "execute real-time operation cmd[%02X], subCmd[%02X]",
                            argRto->cmd, argRto->subCmd);
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec_fota
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : exec fota command
******************************************************************************/
static void pb_prot_proc_cmd_exec_fota(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "execute firmware ota cmd");

    pb_fota_set_param(&(parsedFrame->arg.fota));
    pb_fota_send_msg_to_fota_mod(PB_MSG_FOTA_FIRMWARE_UPGRADE_REQ);
}

/******************************************************************************
* Function    : pb_prot_sack_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_prot_sack_check(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    if (pb_prot_check_event(PB_PROT_ACK_GEN))
    {
        pb_prot_proc_set_sack_cnt(false);
    }

    PB_PROT_ACK_GEN_ARG *argGenAck = &(parsedFrame->arg.genAck);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Proc gen ack[%02X %02X] sn[%d]", 
                             parsedFrame->msgType, parsedFrame->msgSubType, argGenAck->serialNumber);
}

/******************************************************************************
* Function    : pb_prot_proc_cmd_exec
*
* Author      : Chen Hao
*
* Parameters  : command sub-type and args
*
* Return      :
*
* Description : save command configuration and execute command
******************************************************************************/
void pb_prot_proc_cmd_exec(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Proc cmd[%02X]", parsedFrame->msgSubType);

    if (parsedFrame->msgType == PB_PROT_CMD)
    {
        switch(parsedFrame->msgSubType)
        {
            case PB_PROT_CMD_APC:
            {
                pb_prot_proc_cmd_exec_apc(parsedFrame);
                break;
            }
            case PB_PROT_CMD_SER:
            {
                pb_prot_proc_cmd_exec_ser(parsedFrame);
                break;
            }
            case PB_PROT_CMD_CFG:
            {
                pb_prot_proc_cmd_exec_cfg(parsedFrame);
                break;
            }
            case PB_PROT_CMD_TMA:
            {
                pb_prot_proc_cmd_exec_tma(parsedFrame);
                break;
            }
            case PB_PROT_CMD_DOG:
            {
                pb_prot_proc_cmd_exec_dog(parsedFrame);
                break;
            }
            case PB_PROT_CMD_ACO:
            {
                pb_prot_proc_cmd_exec_aco(parsedFrame);
                break;
            }
            case PB_PROT_CMD_SEC:
            {
                pb_prot_proc_cmd_exec_sec(parsedFrame);
                break;
            }
            case PB_PROT_CMD_OMC:
            {
                pb_prot_proc_cmd_exec_omc(parsedFrame);
                break;
            }
            case PB_PROT_CMD_ACW:
            {
                pb_prot_proc_cmd_exec_acw(parsedFrame);
                break;
            }
            case PB_PROT_CMD_DOA:
            {
                pb_prot_proc_cmd_exec_doa(parsedFrame);
                break;
            }
            case PB_PROT_CMD_SMA:
            {
                pb_prot_proc_cmd_exec_sma(parsedFrame);
                break;
            }
            //command do not need save configuration
            case PB_PROT_CMD_OUO:
            {
                break;
            }
            case PB_PROT_CMD_OUT:
            {
                pb_prot_proc_cmd_exec_out(parsedFrame);
                break;
            }
            case PB_PROT_CMD_MUO:
            {
                pb_prot_proc_cmd_exec_muo(parsedFrame);
                break;
            }
            case PB_PROT_CMD_RTO:
            {
                pb_prot_proc_cmd_exec_rto(parsedFrame);
                break;
            }
            case PB_PROT_CMD_FOTA:
            {
                pb_prot_proc_cmd_exec_fota(parsedFrame);
                break;
            }
            default:break;
        }
    }
    else
    if (parsedFrame->msgType == PB_PROT_ACK)
    {
        if (parsedFrame->msgSubType == PB_PROT_ACK_GEN)
        {
            pb_prot_sack_check(parsedFrame);
        }
    }
}

/******************************************************************************
* Function    : pb_prot_proc_reset_hotp_key
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_reset_hotp_key(void)
{
    PB_CFG_SEC *cfgSec = &(pb_cfg_proc_get_cmd()->sec);

    memcpy(cfgSec->normalKey, PB_CFG_SEC_DEFAULT.normalKey, PB_SEC_KEY_LEN);
    memcpy(cfgSec->serviceKey, PB_CFG_SEC_DEFAULT.serviceKey, PB_SEC_KEY_LEN);
    pb_cfg_proc_save_cmd(PB_PROT_CMD_SEC, cfgSec, sizeof(PB_CFG_SEC));

    pb_order_hotp_key_change();
}

/******************************************************************************
* Function    : pb_prot_proc_reset_com_key
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_reset_com_key(void)
{
    PB_CFG_SEC *cfgSec = &(pb_cfg_proc_get_cmd()->sec);

    memcpy(cfgSec->comKey, PB_CFG_SEC_DEFAULT.comKey, PB_SEC_KEY_LEN);
    pb_cfg_proc_save_cmd(PB_PROT_CMD_SEC, cfgSec, sizeof(PB_CFG_SEC));

    pb_crypto_set_key(cfgSec->comKey);
}

/******************************************************************************
* Function    : pb_prot_proc_set_sack_cnt
* 
* Author      : Chen Hao
* 
* Parameters  : TRUE: Add 1 stand for detect one un-confirm HBP;
*                     FALSE: Minus 1 stand for detect one confirm HBP;
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_set_sack_cnt(bool np)
{
    if (np)
    {
        unAckHBPCnt++;
        if (unAckHBPCnt == 255)
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "un-confirm HBP comes to %d", unAckHBPCnt);
            unAckHBPCnt = 254;
        }
    }
    else
    {
        unAckHBPCnt = 0;
        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Clear un-confirm HBP cnt");
    }
    
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%d]un-confirm HBP %d", np, unAckHBPCnt);
}

/******************************************************************************
* Function    : pb_prot_proc_is_sack_timeout
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_prot_proc_is_sack_timeout(void)
{
    return (bool)(unAckHBPCnt >= PB_PROT_SACK_CNTOUT);
}

/******************************************************************************
* Function    : pb_prot_proc_set_dev_gsm_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_set_dev_gsm_info(PB_PROT_RSP_GSMINFO_PARAM *gsm)
{
    memcpy(&pb_gsm_info, gsm, sizeof(PB_PROT_RSP_GSMINFO_PARAM));
}

/******************************************************************************
* Function    : pb_prot_proc_get_dev_gsm_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
PB_PROT_RSP_GSMINFO_PARAM* pb_prot_proc_get_dev_gsm_info(void)
{
    return &pb_gsm_info;
}

/******************************************************************************
* Function    : pb_prot_proc_set_dev_location
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_prot_proc_set_dev_location(PB_PROT_RSP_LOC_PARAM *loc)
{
    memcpy(&pb_location, loc, sizeof(PB_PROT_RSP_LOC_PARAM));
    pb_location.fixType = PB_PROT_RSP_LOC_LAST_FIX;
}

/******************************************************************************
* Function    : pb_prot_proc_get_dev_location
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
PB_PROT_RSP_LOC_PARAM* pb_prot_proc_get_dev_location(void)
{
    return &pb_location;
}

/******************************************************************************
* Function    : pb_prot_proc_load_device_info
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : load device info from flash
******************************************************************************/
void pb_prot_proc_init(void)
{
    //set init gsm info
    PB_PROT_RSP_GSMINFO_PARAM gsm;
    memset(&gsm, 0, sizeof(PB_PROT_RSP_GSMINFO_PARAM));
    memcpy(gsm.gsmModule, "UNKNOWN", 7);
    memcpy(gsm.imei, "UNKNOWN", 7);
    memcpy(gsm.imsi, "UNKNOWN", 7);
    memcpy(gsm.iccid, "UNKNOWN", 7);
    pb_prot_proc_set_dev_gsm_info(&gsm);

    //set init loc info
    PB_PROT_RSP_LOC_PARAM loc;
    loc.fixType = 0;
    #if 0 //test 31.169081,121.408401
    memcpy(loc.longitude, "0121.408401", sizeof(loc.longitude));
    memcpy(loc.latitude, "031.169081", sizeof(loc.latitude));
    #else
    memcpy(loc.longitude, "0000.000000", sizeof(loc.longitude));
    memcpy(loc.latitude, "000.000000", sizeof(loc.latitude));
    #endif
    loc.timestamp = 0;
    pb_prot_proc_set_dev_location(&loc);
}

