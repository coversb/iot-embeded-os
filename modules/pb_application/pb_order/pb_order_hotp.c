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
#include "pb_ota_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_ORDER_OFFLINE_PW_UPDATE_INTERVAL (55*60) // seconds
#define PB_ORDER_ENG_PASS_BUFF 8

#define PB_ORDER_OFFLINE_HBYTE_BUFF 10
#define PB_ORDER_OFFLINE_PASS_BUFF 80

#define PB_ORDER_ENG_DURATION (600) // 10 minutes
#define PB_ORDER_OFFLINE_DURATION (7200) // 120 minutes

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
const static uint8 offlinePwSelectIndex[] = 
{
    3,  5,  7,  11,  13,  17,  19,  23,
    29, 31, 1,  2,   4,   6,   8,   9,
    10, 12, 14, 15,  16,  18,  20,  21,
    22, 24, 25, 26,  27,  28,  30
};

static bool keyChanged = false;
static PB_ORDER_HOTP_CONTEXT pb_order_hotp_context;
//save offline order, when the network re-connect, send to server
static PB_ORDER_OFFLINE_BUFF pb_offline_order_buff;

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
    else
    if (keyChanged)
    {
        keyChanged= false;
        ret = true;
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_sha_eng_key
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_sha_eng_key(uint8 *key)
{
    memcpy(key, pb_cfg_proc_get_cmd()->sec.serviceKey, PB_SEC_KEY_LEN);
}

/******************************************************************************
* Function    : pb_order_hotp_sha_offline_key
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_sha_offline_key(uint8 *key)
{
    memcpy(key, pb_cfg_proc_get_cmd()->sec.normalKey, PB_SEC_KEY_LEN);
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
* Function    : pb_order_hotp_password_duplicate32
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : if pass is duplicate in pass pool, return true
******************************************************************************/
static bool pb_order_hotp_password_duplicate32(uint32 *passPool, uint8 poolSize, uint32 pass)
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
* Function    : pb_order_hotp_password_duplicate16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_password_duplicate16(uint16 *passPool, uint8 poolSize, uint16 pass)
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
* Function    : pb_order_hotp_password_duplicate8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_password_duplicate8(uint8 *passPool, uint8 poolSize, uint8 pass)
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
* Description : generate engineer password
******************************************************************************/
static void pb_order_hotp_update_eng_password(uint32 timestamp, uint32* outPassword)
{
    uint8 key[PB_SEC_KEY_LEN];
    uint8 msg[PB_SEC_KEY_LEN];
    uint8 shaData[SHA256_DIGEST_SIZE];
    uint32 uMaxPass = pb_order_hotp_get_max_password(PB_ORDER_ENG_PW_LEN);

    memset(key, 0, sizeof(key));
    memset(msg, 0, sizeof(msg));
    memset(shaData, 0, sizeof(shaData));

    pb_order_hotp_sha_eng_key(key);
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
        if (pb_order_hotp_password_duplicate32(outPassword, size, pass))
        {
            continue;
        }

        outPassword[size] = pass;
        size++;
    }

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "%d eng pass generated", size);
}

/******************************************************************************
* Function    : pb_order_hotp_offline_hbyte_buff
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get high byte pools
******************************************************************************/
static uint8 pb_order_hotp_offline_hbyte_buff(uint8 *src, uint8 srcSize, uint8 *outBytes)
{
    uint8 hByteSize = 0;
    uint8 hByte = 0;
    
    for (uint8 idx = 0; idx < sizeof(offlinePwSelectIndex); ++idx)
    {
        hByte = src[offlinePwSelectIndex[idx]];
        if (pb_order_hotp_password_duplicate8(outBytes, hByteSize, hByte))
        {
            continue;
        }

        outBytes[hByteSize] = hByte;
        hByteSize++;
        
        if (hByteSize == PB_ORDER_OFFLINE_HBYTE_BUFF)
        {
            break;
        }
    }

    return hByteSize;
}

/******************************************************************************
* Function    : pb_order_hotp_offline_pass_buff
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_order_hotp_offline_pass_buff(uint8 *hBytes, uint8 *shaData, uint32 uMaxPass, uint16 *outPass)
{
    uint8 size = 0;
    uint16 pass = 0;
    for (uint8 hByteIdx = 0; hByteIdx < PB_ORDER_OFFLINE_HBYTE_BUFF; ++hByteIdx)
    {
        for (uint8 shaDataIdx = 0; shaDataIdx < SHA256_DIGEST_SIZE; ++shaDataIdx)
        {
            pass = (hBytes[hByteIdx] * 0x100 + shaData[shaDataIdx]) % uMaxPass;
            if (!pb_order_hotp_password_valid(pass, uMaxPass))
            {
                continue;
            }
            if (pb_order_hotp_password_duplicate16(outPass, size, pass))
            {
                continue;
            }
            outPass[size] = pass;
            size++;

            if (size == PB_ORDER_OFFLINE_PASS_BUFF)
            {
                goto end;
            }
        }
    }
end:

    return size;
}

/******************************************************************************
* Function    : pb_order_hotp_offline_duplicate_eng
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_offline_duplicate_eng(uint16 pass)
{
    bool ret = false;
    
    for (uint8 idx = 0; idx < PB_ORDER_ENG_PW_NUM; ++idx)
    {
        if ((pass == pb_order_hotp_context.engPw[idx] / 10000)
            || (pass == pb_order_hotp_context.engPw[idx] % 10000))
        {
            ret = true;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_update_offline_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : generate offline password
******************************************************************************/
static void pb_order_hotp_update_offline_password(uint32 timestamp, uint16* outPassword)
{
    uint8 key[PB_SEC_KEY_LEN];
    uint8 msg[PB_SEC_KEY_LEN];
    uint8 shaData[SHA256_DIGEST_SIZE];
    uint32 uMaxPass = pb_order_hotp_get_max_password(PB_ORDER_PW_LEN);
    uint8 hBytes[PB_ORDER_OFFLINE_HBYTE_BUFF];
    uint16 offlinePassBuff[PB_ORDER_OFFLINE_PASS_BUFF];
    uint32 offlinePassNum = 0;

    memset(key, 0, sizeof(key));
    memset(msg, 0, sizeof(msg));
    memset(shaData, 0, sizeof(shaData));
    memset(hBytes, 0, sizeof(hBytes));
    memset(offlinePassBuff, 0, sizeof(offlinePassBuff));

    pb_order_hotp_sha_offline_key(key);
    pb_order_hotp_sha_msg(timestamp, msg);

    hmac_sha256(key, sizeof(key), msg, sizeof(msg), shaData, sizeof(shaData));
    pb_order_hotp_offline_hbyte_buff(shaData, sizeof(shaData), hBytes);
    offlinePassNum = pb_order_hotp_offline_pass_buff(hBytes, shaData, uMaxPass, offlinePassBuff);

    uint8 offlineSize = 0;
    uint16 offlinePass = 0;
    for (uint8 idx = 0; idx < offlinePassNum; ++idx)
    {
        offlinePass = offlinePassBuff[idx];
        if (pb_order_hotp_offline_duplicate_eng(offlinePass))
        {
            continue;
        }
        outPassword[offlineSize] = offlinePass;
        offlineSize++;

        if (offlineSize == PB_ORDER_OFFLINE_PW_NUM)
        {
            break;
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "%d offline pass generated", offlineSize);
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
* Function    : pb_order_hotp_verify_offline_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_verify_offline_password(uint16 password)
{
    bool ret = false;
    for (uint8 idx = 0; idx < PB_ORDER_OFFLINE_PW_NUM; ++idx)
    {
        if (password == pb_order_hotp_context.offlinePw[idx])
        {
            ret = true;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_verify_offline_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_hotp_verify_last_offline_password(uint16 password)
{
    bool ret = false;
    for (uint8 idx = 0; idx < PB_ORDER_OFFLINE_PW_NUM; ++idx)
    {
        if (password == pb_order_hotp_context.lastOfflinePw[idx])
        {
            ret = true;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_order_hotp_buffer_offline_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_buffer_offline_order(PB_PROT_ORDER_TYPE *order)
{
    if (pb_offline_order_buff.size == PB_ORDER_OFFLINE_BUFFSIZE)
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "Offline order buff full %d", pb_offline_order_buff.size);
        return;
    }

    pb_offline_order_buff.order[pb_offline_order_buff.size].inputTime = order->id;
    pb_offline_order_buff.order[pb_offline_order_buff.size].startTime = order->startTime;
    pb_offline_order_buff.order[pb_offline_order_buff.size].password = order->passwd;
    pb_offline_order_buff.size++;
}

/******************************************************************************
* Function    : pb_order_hotp_offline_order_buff
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
PB_ORDER_OFFLINE_BUFF *pb_order_hotp_offline_order_buff(void)
{
    return &pb_offline_order_buff;
}

/******************************************************************************
* Function    : pb_order_hotp_try_to_send_buffer_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_hotp_try_to_send_buffer_order(void)
{
    if (pb_offline_order_buff.size == 0)
    {
        return;
    }

    if (pb_ota_network_connected()
        && pb_ota_network_send_que_size() == 0)
    {
        pb_prot_send_rsp_req(PB_PROT_RSP_OPO);
        
        OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Send %d offline order", pb_offline_order_buff.size);
    }
}

/******************************************************************************
* Function    : pb_order_hotp_add_offline_order
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_add_offline_order(uint8 type, uint32 pass)
{
    PB_PROT_ORDER_TYPE order;
    uint32 curTime = pb_util_get_timestamp();
    
    switch (type)
    {
        case PB_ORDER_HOTP_ENG:
        {
            order.startTime = curTime;
            order.expireTime = curTime + PB_ORDER_ENG_DURATION;
            break;
        }
        case PB_ORDER_HOTP_OFFLINE:
        {
            order.startTime = pb_order_hotp_context.updateTime;
            order.expireTime = order.startTime + PB_ORDER_OFFLINE_DURATION;
            break;
        }
        case PB_ORDER_HOTP_LASTOFFLINE:
        {
            order.startTime = pb_order_hotp_context.updateTime - 3600;
            order.expireTime = order.startTime + PB_ORDER_OFFLINE_DURATION;
            break;
        }
        default:return;
    }

    order.id = curTime;
    order.passwd = pass;
    order.personNum = 0;
    order.passwdValidCnt = 0;

    pb_order_hotp_buffer_offline_order(&order);
    pb_order_booking(&order);
    
    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Added offline order");
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
uint8 pb_order_hotp_verify_password(uint32 password, uint32 *orderID)
{
    uint8 passType = PB_ORDER_VERIFY_PW_UNKNOWN;
    uint8 orderType = PB_ORDER_HOTP_UNKNOWN;

    //engineering password
    if (password >= (pb_order_hotp_get_max_password(PB_ORDER_ENG_PW_LEN) / 10))
    {
        if (pb_order_hotp_verify_eng_password(password))
        {
            passType = PB_ORDER_VERIFY_PW_ENG;
            orderType = PB_ORDER_HOTP_ENG;
        }
    }
    //offline password
    else
    {
        if (pb_order_hotp_offline_password_enable())
        {
            if (pb_order_hotp_verify_offline_password(password))
            {
                passType = PB_ORDER_VERIFY_PW_OFFLINE;
                orderType = PB_ORDER_HOTP_OFFLINE;
            }
            else
            if (pb_order_hotp_verify_last_offline_password(password))
            {
                passType = PB_ORDER_VERIFY_PW_OFFLINE;
                orderType = PB_ORDER_HOTP_LASTOFFLINE;
            }
        }
    }

    pb_order_hotp_add_offline_order(orderType, password);

    if (passType != PB_ORDER_VERIFY_PW_UNKNOWN)
    {
        *orderID = password;
    }

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

    //generate eng password
    pb_order_hotp_update_eng_password(updateTime, pb_order_hotp_context.engPw);

    //save last offline password
    memcpy(pb_order_hotp_context.lastOfflinePw, pb_order_hotp_context.offlinePw, sizeof(pb_order_hotp_context.lastOfflinePw));

    //generate offline passoword
    pb_order_hotp_update_offline_password(updateTime, pb_order_hotp_context.offlinePw);

    pb_order_hotp_context.updateTime = updateTime;
}

/******************************************************************************
* Function    : pb_order_hotp_key_change
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_order_hotp_key_change(void)
{
    keyChanged = true;
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
    OS_INFO("\nOFFLINE PASS %s", pb_order_hotp_offline_password_enable() ? "ENABLED" : "DISABLED");

    OS_INFO("CURRENT PASS:");
    for (uint8 idx = 0; idx < PB_ORDER_OFFLINE_PW_NUM; ++idx)
    {
        printf("%d ", pb_order_hotp_context.offlinePw[idx]);
        if (idx % 10 == 9)
        {
            printf("\n");
        }
    }
    OS_INFO("LAST HOUR PASS:");
    for (uint8 idx = 0; idx < PB_ORDER_OFFLINE_PW_NUM; ++idx)
    {
        printf("%d ", pb_order_hotp_context.lastOfflinePw[idx]);
        if (idx % 10 == 9)
        {
            printf("\n");
        }
    }
    OS_INFO("");
}

/******************************************************************************
* Function    : pb_order_hotp_init_last_offline_password
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_order_hotp_init_last_offline_password(void)
{
    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "last hour pass init");

    uint32 updateTime = pb_util_get_timestamp();
    //adjust to hourly timestamp
    updateTime += 300;
    updateTime /= 3600;
    updateTime *= 3600;

    updateTime -= 3600;

    //generate eng password
    pb_order_hotp_update_eng_password(updateTime, pb_order_hotp_context.engPw);

    //save last offline password
    memcpy(pb_order_hotp_context.lastOfflinePw, pb_order_hotp_context.offlinePw, sizeof(pb_order_hotp_context.lastOfflinePw));

    //generate offline passoword
    pb_order_hotp_update_offline_password(updateTime, pb_order_hotp_context.offlinePw);
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
    memset(&pb_offline_order_buff, 0, sizeof(pb_offline_order_buff));
    memset(&pb_order_hotp_context, 0, sizeof(pb_order_hotp_context));
    pb_order_hotp_init_last_offline_password();
    pb_order_hotp_update();
}

