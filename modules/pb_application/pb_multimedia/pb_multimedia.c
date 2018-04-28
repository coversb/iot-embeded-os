/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_multimedia.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   multimedia operations
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-28      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_app_message.h"
#include "pb_multimedia.h"
#include "pb_cfg_proc.h"
#include "kt603.h"
#include "pb_util.h"

/******************************************************************************
* Macros
******************************************************************************/
#define AUDIO devKT603

//audio file index
#define PB_MM_FILE_WELCOME 1
#define PB_MM_FILE_ORDER_OVER 2
#define PB_MM_FILE_SMOKE_ALARM 3
//audio dir index
#define PB_MM_DIR_INT 1
#define PB_MM_DIR_BGM 2
#define PB_MM_DIR_SYSAUDIO 8

#define PB_MM_BGM_CHECK_INTERVAL 10 // seconds
/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MSG_QUEUE_TYPE pb_multimedia_msg_queue;
static AUDIO_BGM_MANAGE bgmManager;

/******************************************************************************
* Local Functions define
******************************************************************************/
/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_multimedia_audio_available
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_multimedia_audio_available(void)
{
    return (bool)(KT603_NO_DEV != AUDIO.available());
}

/******************************************************************************
* Function    : pb_multimedia_get_audio_volume
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_multimedia_get_audio_volume(void)
{
    return pb_cfg_proc_get_cmd()->muo.volume;
}

/******************************************************************************
* Function    : pb_multimedia_set_audio_volume
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_multimedia_set_audio_volume(uint8 lv)
{
    lv = MIN_VALUE(lv, AUDIO_MAX_VOL);
    
    PB_CFG_MUO *cfgMuo = &(pb_cfg_proc_get_cmd()->muo);
    if (lv != cfgMuo->volume)
    {
        cfgMuo->volume = lv;
        pb_cfg_proc_save_cmd(PB_PROT_CMD_MUO, cfgMuo, sizeof(PB_CFG_MUO));
    }

    AUDIO.volume(lv);
}

/******************************************************************************
* Function    : pb_multimedia_volume_up
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_multimedia_volume_up(void)
{
    uint8 lv = pb_multimedia_get_audio_volume();

    if (lv < AUDIO_MAX_VOL)
    {
        lv += 1;
        pb_multimedia_set_audio_volume(lv);
    }
}

/******************************************************************************
* Function    : pb_multimedia_volume_down
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_multimedia_volume_down(void)
{
    uint8 lv = pb_multimedia_get_audio_volume();

    if (lv > AUDIO_MIN_VOL)
    {
        lv -= 1;
        pb_multimedia_set_audio_volume(lv);
    }
}

/******************************************************************************
* Function    : pb_multimedia_play_bgm
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_multimedia_play_bgm(void)
{
    static bool needGetFileNum = true;

    if (needGetFileNum)
    {
        bgmManager.fileNum = AUDIO.fileNumber(PB_MM_DIR_BGM);
        if (bgmManager.fileNum != 0)
        {
            needGetFileNum = false;
        }
        OS_DBG_TRACE(DBG_MOD_PBMM, DBG_INFO, "BGM file num[%d]", bgmManager.fileNum);
    }

    uint16 playIdx = 0;
re_select:
    playIdx = pb_util_random_num(bgmManager.fileNum);
    if (playIdx == bgmManager.lastPalyingIdx)
    {
        goto re_select;
    }
    bgmManager.lastPalyingIdx = playIdx;
    bgmManager.lastStartTime = pb_util_get_timestamp();
    bgmManager.lastCheckTime = pb_util_get_timestamp();
    bgmManager.playing = true;

    OS_DBG_TRACE(DBG_MOD_PBMM, DBG_INFO, "BGM file idx[%d]", playIdx);
    AUDIO.playFile(PB_MM_DIR_BGM, playIdx);
}

/******************************************************************************
* Function    : pb_multimedia_audio_ctrl_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : handle audio control request
******************************************************************************/
static void pb_multimedia_audio_ctrl_hdlr(PB_MSG_TYPE *pMsg)
{
    if (!os_msg_data_vaild(pMsg->pMsgData))
    {
        return;
    }
    
    PB_MULTIMEDIA_MSG_PARA_TYPE *pAudioMsg = (PB_MULTIMEDIA_MSG_PARA_TYPE*)pMsg->pMsgData;
    OS_DBG_TRACE(DBG_MOD_PBMM, DBG_INFO, "Audio control[%d,%d]", pAudioMsg->cmd, pAudioMsg->param);

    switch (pAudioMsg->cmd)
    {
        case PB_MM_MUTE_ON:
        {
            AUDIO.mute(true);
            break;
        }
        case PB_MM_MUTE_OFF:
        {
            AUDIO.mute(false);
            break;
        }
        case PB_MM_SET_VOL:
        {
            pb_multimedia_set_audio_volume(pAudioMsg->param);
            break;
        }
        case PB_MM_VOL_UP:
        {
            pb_multimedia_volume_up();
            break;
        }
        case PB_MM_VOL_DOWN:
        {
            pb_multimedia_volume_down();
            break;
        }
        case PB_MM_STOP:
        {
            AUDIO.stop();
            bgmManager.playing = false;
            break;
        }
        case PB_MM_PAUSE:
        {
            AUDIO.pause();
            break;
        }
        case PB_MM_PLAY:
        {
            AUDIO.play();
            break;
        }
        case PB_MM_PLAY_WELCOME:
        {
            AUDIO.interruptPlayFile(PB_MM_DIR_INT, PB_MM_FILE_WELCOME);
            break;
        }
        case PB_MM_PLAY_ORDER_OVER:
        {
            AUDIO.interruptPlayFile(PB_MM_DIR_INT, PB_MM_FILE_ORDER_OVER);
            break;
        }
        case PB_MM_PLAY_SMOKE_ALARM:
        {
            AUDIO.interruptPlayFile(PB_MM_DIR_INT, PB_MM_FILE_SMOKE_ALARM);
            break;
        }
        case PB_MM_PLAY_BGM:
        {
            pb_multimedia_play_bgm();
            break;
        }
        default:
        {
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_multimedia_monitor_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_multimedia_monitor_hdlr(void)
{
    if (bgmManager.playing)
    {
        uint32 now = pb_util_get_timestamp();

        if (now - bgmManager.lastCheckTime > PB_MM_BGM_CHECK_INTERVAL)
        {
            bgmManager.lastCheckTime = now;
            if (KT603_STOP == AUDIO.playStatus())
            {
                pb_multimedia_play_bgm();
            }
        }
    }
}

/******************************************************************************
* Function    : pb_multimedia_send_audio_msg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_multimedia_send_audio_msg(uint8 cmd, uint8 param)
{
    PB_MULTIMEDIA_MSG_PARA_TYPE audioMsg;
    audioMsg.cmd = cmd;
    audioMsg.param = param;

    PB_MSG_TYPE msg;
    msg.pMsgData = (uint8*)os_malloc(sizeof(audioMsg));

    if (msg.pMsgData == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBMM, "msg malloc err");
        return;
    }

    OS_DBG_TRACE(DBG_MOD_PBMM, DBG_INFO, "Send AUDIO req[%d,%d] to MM", cmd, param);
    msg.msgID = PB_MSG_MM_AUDIO_CTRL_REQ;
    memcpy(msg.pMsgData, &audioMsg, sizeof(audioMsg));

    os_msg_queue_send(pb_multimedia_msg_queue, ( void*)&msg, 0);
}

/******************************************************************************
* Function    : pb_multimedia_send_monitor_req
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_multimedia_send_monitor_req(void)
{
    if (bgmManager.playing)
    {
        return;
    }
    
    PB_MSG_TYPE msg;

    msg.pMsgData = NULL;
    msg.msgID = PB_MSG_MM_AUDIO_MONITOR_REQ;
    os_msg_queue_send(pb_multimedia_msg_queue, ( void*)&msg, 0);
}

/******************************************************************************
* Function    : pb_multimedia_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : multimedia task init
******************************************************************************/
static void pb_multimedia_init(void)
{
    AUDIO.init(&AUDIO_COM, AUDIO_COM_BAUDRATE);

    pb_multimedia_msg_queue = os_msg_queue_create(PB_MM_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));
    memset(&bgmManager, 0, sizeof(bgmManager));
}

/******************************************************************************
* Function    : pb_multimedia_main
*
* Author      : Chen Hao
*
* Parameters  : 
*
* Return      :
*
* Description : control multimedia releated operations
******************************************************************************/
void pb_multimedia_main(void *param)
{
    pb_multimedia_init();

    os_set_task_init(OS_TASK_ITEM_PB_MULTIMEDIA);
    os_wait_task_init_sync();

    //set audio volume
    pb_multimedia_set_audio_volume(pb_multimedia_get_audio_volume());

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        //need some blocking function in case of wasting cpu
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_multimedia_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            OS_DBG_TRACE(DBG_MOD_PBMM, DBG_INFO, "Recv msg %d", p_pb_msg->msgID);
            switch (p_pb_msg->msgID)
            {
                case PB_MSG_MM_AUDIO_CTRL_REQ:
                {
                    pb_multimedia_audio_ctrl_hdlr(p_pb_msg);
                    break;
                }
                case PB_MSG_MM_AUDIO_MONITOR_REQ:
                {
                    pb_multimedia_monitor_hdlr();
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_PBMM, "Unknow msg %d", p_pb_msg->msgID);
                    break;
                }
            }

            if (p_pb_msg->pMsgData != NULL)
            {
                os_free(p_pb_msg->pMsgData);
            }
            p_pb_msg->msgID = PB_MESSAGE_NONE;
        }
    }
}

