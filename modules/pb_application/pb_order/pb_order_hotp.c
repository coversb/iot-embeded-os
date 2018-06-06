/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_order_hotp.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   hotp algorithm
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-5         1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "os_trace_log.h"
#include "hmac_sha256.h"
#include "pb_app_config.h"
#include "pb_order_hotp.h"
#include "pb_cfg_proc.h"
#include "pb_prot_proc.h"
#include "pb_prot_main.h"
#include "pb_util.h"
#include "pb_order_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_ORDER_OFFLINE_PW_UPDATE_INTERVAL (55*60) // seconds
#define PB_ORDER_ENG_PASS_BUFF 8

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_ORDER_HOTP_CONTEXT pb_order_hotp_context;

/******************************************************************************
* Function    : pb_order_hotp_offline_password_enable
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_order_hotp_offline_password_enable(void)
{
    if (pb_cfg_proc_get_cmd()->sec.hotpMode == PB_SEC_HOTP_ON)
    {
        if (!pb_prot_check_event(PB_PROT_ACK_GEN))
        {
            return true;
        }
        else 
        if (pb_prot_proc_is_sack_timeout())
        {
            return true;
        }
    }

    return false;
}

/******************************************************************************
* Function    : pb_order_hotp_need_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_need_update(void)
{
    bool ret = false;
    uint32 curTime = pb_util_get_timestamp();

    if (curTime > pb_order_hotp_context.updateTime + PB_ORDER_OFFLINE_PW_UPDATE_INTERVAL)
    {
        ret = true;
    }
    else
    if (pb_order_hotp_context.updateTime > curTime + 5*60)
    {
        ret = true;
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_sha_key
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_sha_key(uint8 *key)
{
    memcpy(key, pb_cfg_proc_get_cmd()->sec.serviceKey, PB_SEC_KEY_LEN);
}

/******************************************************************************
* Function    : pb_order_hotp_sha_msg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_sha_msg(uint32 t, uint8 *msg)
{
    uint8 szTime[16];
    memset(szTime, 0, sizeof(szTime));
    sprintf((char*)szTime, "%d", t);
    
    memcpy(msg, pb_cfg_proc_get_uid(), PB_UID_LEN);
    memcpy(&msg[16], szTime, 16);
}

/******************************************************************************
* Function    : pb_order_hotp_get_max_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get max password by password len
******************************************************************************/
static uint32 pb_order_hotp_get_max_password(uint8 passLen)
{
    uint32 maxPass = 1;

    for (uint8 idx = 0; idx < passLen; ++idx)
    {
        maxPass *= 10;
    }

    return maxPass;
}

/******************************************************************************
* Function    : pb_order_hotp_password_valid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check validity
******************************************************************************/
static bool pb_order_hotp_password_valid(const uint32 pass, const uint32 maxPass)
{
    //first number can't be '0'
    if (pass < (maxPass / 10) )
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_order_hotp_password_duplicate
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : if pass is duplicate in pass pool, return true
******************************************************************************/
static bool pb_order_hotp_password_duplicate(uint32 *passPool, uint8 poolSize, uint32 pass)
{
    bool ret = false;
    for (uint8 idx = 0; idx < poolSize; ++idx)
    {
        if (passPool[idx] == pass)
        {
            ret = true;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_update_eng_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : generate engineering password
******************************************************************************/
void pb_order_hotp_update_eng_password(uint32 timestamp, uint32* outPassword)
{
    uint8 key[PB_SEC_KEY_LEN];
    uint8 msg[PB_SEC_KEY_LEN];
    uint8 shaData[SHA256_DIGEST_SIZE];
    uint32 uMaxPass = pb_order_hotp_get_max_password(PB_ORDER_ENG_PW_LEN);

    memset(key, 0, sizeof(key));
    memset(msg, 0, sizeof(msg));
    memset(shaData, 0, sizeof(shaData));

    pb_order_hotp_sha_key(key);
    pb_order_hotp_sha_msg(timestamp, msg);

    hmac_sha256(key, sizeof(key), msg, sizeof(msg), shaData, sizeof(shaData));

    uint8 idx = 0;
    uint8 size = 0;
    uint32 pass = 0;
    for (idx = 0, size = 0; idx < PB_ORDER_ENG_PASS_BUFF && size < PB_ORDER_ENG_PW_NUM; ++idx)
    {
        pass = (shaData[4 * idx] * 0x1000000 
                + shaData[4 * idx + 1] * 0x10000 
                + shaData[4 * idx + 2] * 0x100 
                + shaData[4 * idx + 3]) % uMaxPass;

        if (!pb_order_hotp_password_valid(pass, uMaxPass))
        {
            continue;
        }
        if (pb_order_hotp_password_duplicate(outPassword, size, pass))
        {
            continue;
        }

        outPassword[size] = pass;
        size++;
    }

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "%d eng pass generated", size);
}

/******************************************************************************
* Function    : pb_order_hotp_verify_eng_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_verify_eng_password(uint32 password)
{
    bool ret = false;
    for (uint8 idx = 0; idx < PB_ORDER_ENG_PW_NUM; ++idx)
    {
        if (password == pb_order_hotp_context.engPw[idx])
        {
            ret = true;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_verify_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_order_hotp_verify_password(uint32 password)
{
    uint8 passType = PB_ORDER_VERIFY_PW_UNKNOWN;

    //engineering password
    if (password >= (pb_order_hotp_get_max_password(PB_ORDER_ENG_PW_LEN) / 10))
    {
        if (pb_order_hotp_verify_eng_password(password))
        {
            passType = PB_ORDER_VERIFY_PW_ENG;
        }
    }
    //offline password
    else
    {
        ;
    }
    /*
    PB_ORDER_VERIFY_PW_UNKNOWN,
    PB_ORDER_VERIFY_PW_OFFLINE,
    PB_ORDER_VERIFY_PW_ENG,
    */
    return passType;
}

/******************************************************************************
* Function    : pb_order_hotp_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_hotp_update(void)
{
    if (!pb_order_hotp_need_update())
    {
        return;
    }
    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Offline pw need update");

    uint32 updateTime = pb_util_get_timestamp();
    //adjust to hourly timestamp
    updateTime += 300;
    updateTime /= 3600;
    updateTime *= 3600;

    pb_order_hotp_update_eng_password(updateTime, pb_order_hotp_context.engPw);

    pb_order_hotp_context.updateTime = updateTime;
}

/******************************************************************************
* Function    : pb_order_hotp_offline_password_print
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_hotp_offline_password_print(void)
{
    char szTime[32];
    
    pb_util_timestamp_to_datetime(szTime, sizeof(szTime), pb_order_hotp_context.updateTime);
    OS_INFO("OFFLINE PASSWORD @ %s", szTime);

    OS_INFO("ENG PASS:");
    for (uint8 idx = 0; idx < PB_ORDER_ENG_PW_NUM; ++idx)
    {
        printf("%d ", pb_order_hotp_context.engPw[idx]);
    }
    OS_INFO("");
}

/******************************************************************************
* Function    : pb_order_hotp_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_hotp_init(void)
{
    memset(&pb_order_hotp_context, 0, sizeof(pb_order_hotp_context));
    pb_order_hotp_update();
}

