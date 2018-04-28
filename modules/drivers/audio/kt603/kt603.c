/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          kt603.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   kt603 operation
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-27      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board_config.h"
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "kt603.h"
#include "pam8610.h"

/******************************************************************************
* Macros
******************************************************************************/
#define KT603_CMDBUF_LEN 8
#define KT603_RSPBUF_LEN 10
#define KT603_RESEND_MAX 3
#define KT603_RSP_WAITCNT 10

/*KT603 protocol*/
#define KT603_CMD_HEADER 0x7E
#define KT603_CMD_TAIL 0xEF
#define KT603_CMD_VER 0xFF
#define KT603_CMD_LEN 0x06
#define KT603_RSP_ACK 0x41

#define KT603_ACK_SW 0x01  // 0: disable ack; 1: enable ack;

/*KT603 CMD*/
#define KT603_REQ_DEV 0x3F
#define KT603_REQ_PLAY_STATUS 0x42
#define KT603_REQ_DIR_FILE_NUM 0x4E

#define KT603_CMD_VOLUME 0x06
#define KT603_CMD_PLAY 0x0D
#define KT603_CMD_PAUSE 0x0E
#define KT603_CMD_PLAY_DIR_FILE 0x0F
#define KT603_CMD_STOP 0x16
#define KT603_CMD_INTERRUPT_PLAY_DIR_FILE 0x25

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static DEV_TYPE_PA *PA = (DEV_TYPE_PA*)&devPA;
static HAL_USART_TYPE *KT603_COM = NULL;
static uint32 KT603_COM_BAUDRATE = 9600;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : kt603_checksum
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : calculate kt603 cmd checksum
******************************************************************************/
static uint16 kt603_checksum(uint8 *pdata, uint8 len)
{
    uint16 checksum = 0;
    for (uint8 idx = 0; idx < len; ++idx)
    {
        checksum += pdata[idx];
    }
    checksum = 0 - checksum;

    return checksum;
}

/******************************************************************************
* Function    : kt603_send_cmd
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : send kt603 command
******************************************************************************/
static void kt603_send_cmd(uint8 cmd, uint8 ack, uint16 dat)
{
    uint8 sendCmd[KT603_CMDBUF_LEN];
    uint16 checksum;

    sendCmd[0] = KT603_CMD_VER;  //ver
    sendCmd[1] = KT603_CMD_LEN;  //length
    sendCmd[2] = cmd;   //cmd type
    sendCmd[3] = ack;   //enable or disable ack
    sendCmd[4] = (uint8)(dat >> 8); //data H
    sendCmd[5] = (uint8)(dat);  //data L

    checksum = kt603_checksum(&sendCmd[0], KT603_CMD_LEN);
    sendCmd[6] = (uint8)(checksum >> 8);
    sendCmd[7] = (uint8)(checksum & 0xFF);

    //clear read buff first
    while (KT603_COM->available())
    {
        KT603_COM->read();
    }

    KT603_COM->write(KT603_CMD_HEADER);
    KT603_COM->writeBytes(sendCmd, KT603_CMD_LEN);
    KT603_COM->write(KT603_CMD_TAIL);

    #if (BOARD_KT603_DEBUG == 1)
    char test[32];
    snprintf(test, 32, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
               KT603_CMD_HEADER, sendCmd[0], sendCmd[1], sendCmd[2],
               sendCmd[3], sendCmd[4], sendCmd[5], sendCmd[6],
               sendCmd[7], KT603_CMD_TAIL);
    OS_INFO("CMD:%s", test);
    #endif /*BOARD_KT603_DEBUG*/
}

/******************************************************************************
* Function    : kt603_recv_rsp
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get kt603 respond
******************************************************************************/
static bool kt603_recv_rsp(uint8 *cmd, uint16 *dat)
{
    uint8 offset = 0;
    uint8 respond[KT603_RSPBUF_LEN + 1] = {0};
    uint16 timeout = 0;

wait:
    while (timeout < KT603_RSP_WAITCNT)// 10 * 50ms = 500 ms time out
    {
        timeout++;
        if (!KT603_COM->available())
        {
            os_scheduler_delay(DELAY_50_MS);//wait for respond
        }
        else
        {
            break;
        }
    }
    while (KT603_COM->available() > 0 && offset < KT603_RSPBUF_LEN)
    {
        respond[offset++] = KT603_COM->read();
    }

    if (offset < 10 && timeout < KT603_RSP_WAITCNT)
    {
        goto wait;
    }

    #if (BOARD_KT603_DEBUG == 1)
    char test[32];
    snprintf(test, 32, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
               respond[0], respond[1], respond[2], respond[3],
               respond[4], respond[5], respond[6], respond[7],
               respond[8], respond[9]);
    OS_INFO("RSP:%s", test);
    #endif /*BOARD_KT603_DEBUG*/

    if (respond[0] != KT603_CMD_HEADER
            || respond[1] != KT603_CMD_VER
            || respond[2] != KT603_CMD_LEN
            || respond[9] != KT603_CMD_TAIL)
    {
        return false;
    }
    uint16 checksumRsp = ((uint16)(respond[7] << 8)) | respond[8];
    uint16 checksumCal = kt603_checksum(&respond[1], 6);

    if (checksumRsp != checksumCal)
    {
        return false;
    }

    *cmd = respond[3];
    *dat = ((uint16)(respond[5] << 8)) | respond[6];

    return true;
}

/******************************************************************************
* Function    : kt603_play_file
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : play file in dir
******************************************************************************/
static bool kt603_play_file(uint8 dir, uint8 file)
{
    uint16 data = (dir << 8) | file;
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_PLAY_DIR_FILE, KT603_ACK_SW, data);

    #if (KT603_ACK_SW == 1)
    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "FILE PLAY[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }
    #endif /*KT603_ACK_SW*/

    return true;
}

/******************************************************************************
* Function    : kt603_interrupt_play_file
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : interrupt play item in ADVERTX
******************************************************************************/
static bool kt603_interrupt_play_file(uint8 dir, uint8 file)
{
    uint16 data = (dir << 8) | file;
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_INTERRUPT_PLAY_DIR_FILE, KT603_ACK_SW, data);

    #if (KT603_ACK_SW == 1)
    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "INT PLAY[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }
    #endif /*KT603_ACK_SW*/

    return true;
}

/******************************************************************************
* Function    : kt603_play
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool kt603_play(void)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_PLAY, KT603_ACK_SW, 0);

    #if (KT603_ACK_SW == 1)
    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "START PLAY[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }
    #endif /*KT603_ACK_SW*/

    return true;
}

/******************************************************************************
* Function    : kt603_stop
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool kt603_stop(void)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_STOP, KT603_ACK_SW, 0);

    #if (KT603_ACK_SW == 1)
    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "STOP PLAY[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }
    #endif /*KT603_ACK_SW*/

    return true;
}

/******************************************************************************
* Function    : kt603_pause
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool kt603_pause(void)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_PAUSE, KT603_ACK_SW, 0);

    #if (KT603_ACK_SW == 1)
    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "PAUSE[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }
    #endif /*KT603_ACK_SW*/

    return true;
}

/******************************************************************************
* Function    : kt603_play_status
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static uint8 kt603_play_status(void)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_REQ_PLAY_STATUS, KT603_ACK_SW, 0);

    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_REQ_PLAY_STATUS)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "kt603_playing[%d][%02X, %04X]", ret, cmd, dat);
            return KT603_UNKNOW;
        }
        goto retry;
    }
    
    return (dat & 0xFF);
}

/******************************************************************************
* Function    : kt603_dir_file_num
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static uint16 kt603_dir_file_num(uint8 dir)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;
    //this command must keep ack is 0
    kt603_send_cmd(KT603_REQ_DIR_FILE_NUM, 0, dir);

    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_REQ_DIR_FILE_NUM)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "NUM[%d][%02X, %04X]", ret, cmd, dat);
            return 0;
        }
        goto retry;
    }

    return dat;
}

/******************************************************************************
* Function    : kt603_set_mute
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void kt603_set_mute(bool sw)
{
    PA->sw(sw);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Set mute[%d]", sw);
}

/******************************************************************************
* Function    : kt603_set_volume
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool kt603_set_volume(uint8 lv)
{
    lv = MIN_VALUE(lv, KT603_MAX_VOL);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Set volume[%d]", lv);

    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_CMD_VOLUME, KT603_ACK_SW, lv);

    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_RSP_ACK)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "VOLUME[%d][%02X, %04X]", ret, cmd, dat);
            return false;
        }
        goto retry;
    }

    return true;
}

/******************************************************************************
* Function    : kt603_check_available_device
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check memery device
******************************************************************************/
static uint8 kt603_available_device(void)
{
    uint8 retryCnt = 0;
retry:
    retryCnt++;

    kt603_send_cmd(KT603_REQ_DEV, KT603_ACK_SW, 0);

    uint8 cmd;
    uint16 dat;
    bool ret;
    ret = kt603_recv_rsp(&cmd, &dat);

    if (!ret || cmd != KT603_REQ_DEV)
    {
        if (retryCnt >= KT603_RESEND_MAX)
        {
            OS_DBG_ERR(DBG_MOD_DEV, "DEV check[%d][%02X, %04X]", ret, cmd, dat);
            return KT603_NO_DEV;
        }
        goto retry;
    }

    return dat;
}

/******************************************************************************
* Function    : kt603_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool kt603_reset(void)
{
    PA->sw(false);

    hal_gpio_set(BOARD_KT603_RST, HAL_GPIO_LOW);
    os_scheduler_delay(DELAY_100_MS * 3);
    hal_gpio_set(BOARD_KT603_RST, HAL_GPIO_HIGH);

    KT603_COM->end();
    KT603_COM->begin(KT603_COM_BAUDRATE);

    kt603_set_volume(0);
    PA->sw(true);

    return true;
}

/******************************************************************************
* Function    : kt603_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool kt603_init(const HAL_USART_TYPE *com, const uint32 baudrate)
{
    if (com == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "KT603 com is invalid");
        return false;
    }
    //init PA
    PA->init();
    PA->sw(false);
    os_scheduler_delay(DELAY_500_MS);

    //init KT603 and usart
    KT603_COM = (HAL_USART_TYPE *)com;
    KT603_COM_BAUDRATE = baudrate;

    hal_rcc_enable(BOARD_KT603_IO_RCC);
    hal_gpio_set_mode(BOARD_KT603_RST, GPIO_Mode_Out_PP);

    kt603_reset();

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "KT603 INIT OK, BAUD[%d]", baudrate);
    return true;
}

const DEV_TYPE_KT603 devKT603 = 
{
    kt603_init,
    kt603_reset,
    kt603_set_volume,
    kt603_set_mute,
    kt603_available_device,
    kt603_dir_file_num,
    kt603_play_file,
    kt603_interrupt_play_file,
    kt603_play,
    kt603_stop,
    kt603_pause,
    kt603_play_status
};

