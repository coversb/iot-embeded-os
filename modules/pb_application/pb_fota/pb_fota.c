/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_fota.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   firmware over the air functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "os_trace_log.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_app_message.h"
#include "pb_util.h"
#include "pb_fota.h"
#include "pb_prot_main.h"
#include "pb_ota_main.h"
#include "ftp.h"
#include "md5.h"
#include "pb_io_indicator_led.h"
#include "pb_gui_main.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_FOTA_UPGRADE_START_TIMEOUT 20 // seconds
#define PB_FOTA_BLOCK_SIZE 2048 // bytes

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MSG_QUEUE_TYPE pb_fota_msg_queue;
static PB_FOTA_CONTEXT pb_fota_context;
static bool b_fota_upgrading;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_fota_send_fota_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_fota_send_fota_rsp(uint8 status, uint8 cnt)
{
    PB_PROT_RSP_FOTA_PARAM param;
    param.status = status;
    param.cnt = cnt;

    pb_prot_send_rsp_param_req(PB_PROT_RSP_FOTA, (uint8*)&param, sizeof(param));
}

/******************************************************************************
* Function    : pb_fota_erase_free_bank
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_fota_erase_free_bank(PB_IMAGE_CONTEXT *imageContext)
{
    return PB_FLASH_HDLR.erasePages(imageContext->romBegin, imageContext->romEnd - imageContext->romBegin);
}

/******************************************************************************
* Function    : pb_fota_write_firmware_to_free_bank
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write firmdata to free bank
******************************************************************************/
static int32 pb_fota_write_firmware_to_free_bank(PB_IMAGE_CONTEXT *imageContext, uint32 addrOffset, uint8 *pdata, uint16 len)
{
    uint32 writeAddr = imageContext->romBegin + addrOffset;
    if (writeAddr > imageContext->romEnd)
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "BAD FOTA WRITE ADDR[%08X]", writeAddr);
        return -1;
    }

    return PB_FLASH_HDLR.forceWrite(writeAddr, (uint32*)pdata, len);
}

/******************************************************************************
* Function    : pb_prot_firmware_confirm
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_fota_firmware_confirm(void)
{
    uint32 ret = fwManager.confirm();
    bool needReportVer = false;
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Prot FIRMWARE COMFIRM[%d]", ret);

    switch (ret)
    {
        case PB_IMAGE_FOTA_OK:
        {
            pb_fota_send_fota_rsp(PB_FOTA_UPGRADE_OK, 0);
            needReportVer = true;
            break;
        }
        case PB_IMAGE_FOTA_ERR_AND_RECOVER:
        {
            pb_fota_send_fota_rsp(PB_FOTA_UPGRADE_ERR, 0);
            needReportVer = true;
            break;
        }
        case PB_IMAGE_COM_UPDATE:
        {
            needReportVer = true;
            break;
        }
        case PB_IMAGE_NORMAL:
        default:
            break;
    }
    
    if (needReportVer)
    {
        uint8 cmdType = 0;
        pb_prot_send_rsp_param_req(PB_PROT_RSP_RTO, (uint8*)&cmdType, sizeof(cmdType));
    }
}

/******************************************************************************
* Function    : pb_fota_get_firmware_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_fota_get_firmware_info(uint32 *upTimestamp, uint32 *runTimes)
{
    fwManager.imageContext(upTimestamp, runTimes);
}

/******************************************************************************
* Function    : pb_fota_get_firmware_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_fota_get_firmware_version(void)
{
    PB_IMAGE_INFO imageInfo = {0};
    fwManager.imageInfo(PB_IMAGE_APP, &imageInfo);

    return imageInfo.imageVersion;
}

/******************************************************************************
* Function    : pb_fota_get_bl_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_fota_get_bl_version(void)
{
    PB_IMAGE_INFO imageInfo = {0};
    fwManager.imageInfo(PB_IMAGE_BL, &imageInfo);

    return imageInfo.imageVersion;
}

/******************************************************************************
* Function    : pb_fota_send_msg_to_fota_mod
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_fota_send_msg_to_fota_mod(uint8 msgID)
{
    PB_MSG_TYPE msg;
    msg.pMsgData = NULL;
    msg.msgID = msgID;

    os_msg_queue_send(pb_fota_msg_queue, ( void*)&msg, 0);
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Send[%d] to FOTA", msgID);
}

/******************************************************************************
* Function    : pb_fota_set_param
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_fota_set_param(PB_PROT_CMD_FOTA_ARG *param)
{
    pb_fota_context.retry = param->retry;
    pb_fota_context.timeout = param->timeout;
    pb_fota_context.protocol = param->protocol;
    memcpy(pb_fota_context.url, param->url, PB_FOTA_URL_LEN + 1);
    pb_fota_context.port = param->port;
    memcpy(pb_fota_context.usrName, param->usrName, PB_FOTA_USR_LEN + 1);
    memcpy(pb_fota_context.usrPass, param->usrPass, PB_FOTA_PASS_LEN + 1);
    memcpy(pb_fota_context.md5, param->md5, PB_FOTA_MD5_LEN);
    pb_fota_context.key = param->key;
    pb_fota_context.downAddr = param->downAddr;
    pb_fota_context.bootAddr = param->bootAddr;

    pb_fota_context.curRetryCnt = 0;
    pb_fota_context.totalSize = 0;
}

/******************************************************************************
* Function    : pb_fota_upgrading
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_fota_upgrading(void)
{
    return b_fota_upgrading;
}

/******************************************************************************
* Function    : pb_fota_req_hdlr
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_fota_req_hdlr(void)
{
    pb_fota_send_fota_rsp(PB_FOTA_START, 0);
    pb_fota_send_msg_to_fota_mod(PB_MSG_FOTA_START_UPGRADE_REQ);
}

/******************************************************************************
* Function    : pb_fota_parse_url
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get addr, path, from url
******************************************************************************/
static bool pb_fota_parse_url(char *src, uint16 maxLen, char *addr, char *path, char *fname)
{
    char *p = src;
    char *pPath = NULL;
    char *pFname = NULL;
    uint16 len;

    //find file name in url
    if (NULL != (pFname = (strrchr(p, '/'))))//find last '/'
    {
        len = strlen(pFname) - 1;
        if (len > maxLen)
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "Bad fname[%d]", len);
            return false;
        }
        memcpy(fname, (pFname+1), len);
        fname[len] = '\0';
        OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "fname[%s]", fname);
    }
    else
    {
        return false;
    }

    // find ip path in url
    if (NULL != (pPath = (strchr(p, '/'))))//find first '/'
    {
        len = pFname - pPath + 1;
        if (len > maxLen)
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "Bad path[%d]", len);
            return false;
        }
        memcpy(path, pPath, len);
        path[len] = '\0';
        OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "path[%s]", path);
    }
    else
    {
        return false;
    }

    //find address in url
    len = pPath - p;
    if (len > maxLen)
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "Bad addr[%d]", len);
        return false;
    }
    memcpy(addr, p, len);
    addr[len] = '\0';
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "addr[%s]", addr);

    return true;
}

/******************************************************************************
* Function    : pb_fota_save
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write download data to flash
******************************************************************************/
static bool pb_fota_save(FTP_CLIENT *ftp, PB_FOTA_CONTEXT *param, PB_IMAGE_CONTEXT *imageContext)
{
    bool ret = false;
    uint32 totalLen = 0;
    uint32 dataLen = 0;

    if (!pb_fota_erase_free_bank(imageContext))
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "FLASH ERASE ERR");
        return ret;
    }

    uint8 data[PB_FOTA_BLOCK_SIZE];
    do
    {
        memset(data, 0, PB_FOTA_BLOCK_SIZE);
        if (totalLen < param->totalSize)
        {
            dataLen = ftp->getFileData(data, PB_FOTA_BLOCK_SIZE);
            if (dataLen == 0 && totalLen != param->totalSize)
            {
                break;
            }
            
            //write to flash
            if (pb_fota_write_firmware_to_free_bank(imageContext, totalLen, data, dataLen) < 0)
            {
                OS_DBG_ERR(DBG_MOD_PBFOTA, "FLASH WRITE ERR");
                break;
            }

            /*!!!when trace log text length is longer than buff size, datas will be error!!!*/
            //PB_DEBUG_TRACE(DBG_TRACE_DRV, "File data[%d][%s]", dataLen, data);
            totalLen += dataLen;

            //display up process in screen 10% ~ 90%
            pb_gui_set_upgrade_info(PB_GUI_UP_DOWNLOADING, (uint8)(10 + (totalLen * 80.0f / param->totalSize)));
        }
        else
        {
            OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Save FOTA bin OK[%d]", totalLen);
            ret = true;
            break;
        }
    }
    while (1);

    return ret;
}

/******************************************************************************
* Function    : pb_fota_ftp_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_fota_ftp_process(PB_FOTA_CONTEXT *param, PB_IMAGE_CONTEXT *imageContext)
{
    uint8 ret = PB_FOTA_DOWNLOAD_ERR;
    FTP_CLIENT *ftp = (FTP_CLIENT*)pb_ota_network_get_ftp_client(pb_ota_net_get_act_dev());
    if (NULL == ftp)
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "get ftp client err");
        goto err;
    }

    char addr[PB_FOTA_URL_LEN+1];
    char path[PB_FOTA_URL_LEN+1];
    char fname[PB_FOTA_URL_LEN+1];

    //parse url to get server addr / path / file name
    if (!pb_fota_parse_url((char*)param->url, PB_FOTA_URL_LEN, addr, path, fname))
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "Invalid url param");
        goto err;
    }
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "FOTA[%s][%s][%s]", addr, path, fname);

    do 
    {
        param->curRetryCnt++;

        //connect to ftp server
        if (!ftp->connect(addr, param->port, (char*)param->usrName, (char*)param->usrPass))
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp connect err");
            continue;
        }
        //display up process in screen
        pb_gui_set_upgrade_info(PB_GUI_UP_CONNECT_SERVER, 5);

        //set ftp server path
        if (!ftp->setPath(path))
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp set path err");
            goto ftpClose;
        }

        //set ftp work mode
        if (!ftp->setMode())
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp set mode err");
            goto ftpClose;
        }

        //display up process in screen
        pb_gui_set_upgrade_info(PB_GUI_UP_START_DOWNLOAD, 10);
        //get frimware bin total size
        if (0 == (param->totalSize = ftp->getFileSize(fname, param->timeout*60*1000)))
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp get file size err");
            goto ftpClose;
        }

        //save firmware bin file to flash
        if (!pb_fota_save(ftp, param, imageContext))
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp save firmware err");
            goto ftpClose;
        }
        ret = PB_FOTA_DOWNLOAD_OK;

    ftpClose:
        if (!ftp->disconnect())
        {
            OS_DBG_ERR(DBG_MOD_PBFOTA, "ftp disconnect err");
        }

        if (ret == PB_FOTA_DOWNLOAD_OK)
        {
            break;
        }
    }
    while (param->curRetryCnt < param->retry);

err:
    return ret;
}

/******************************************************************************
* Function    : pb_fota_version_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_fota_version_check(uint32 addr)
{
    bool ret = false;
    PB_IMAGE_INFO *pInfo = NULL;

    pInfo = (PB_IMAGE_INFO*)(addr|FIRMWARE_IMAGE_INFO_OFFSET);
    
    if (pInfo->hwVersion == BOARD_HW_VERSION
        && pInfo->imageType == PB_IMAGE_APP)
    {
        ret = true;
    }
    else
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "Err ver, hw[%X]need[%X], type[%X]need[%X]", 
                            pInfo->hwVersion, BOARD_HW_VERSION,
                            pInfo->imageType, PB_IMAGE_APP);
    }

    return ret;
}

/******************************************************************************
* Function    : pb_fota_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_fota_process(PB_FOTA_CONTEXT *param)
{
    //get image management content
    PB_IMAGE_CONTEXT newImage;
    memset(&newImage, 0, sizeof(newImage));

    fwManager.request(&newImage);
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "REQ FIRM ADDR[%08X]", newImage.romBegin);

    uint8 ret = (uint8)PB_FOTA_DOWNLOAD_ERR;

    //firmware downloading
    switch (param->protocol)
    {
        case PB_FOTA_FTP:
        {
            ret = pb_fota_ftp_process(param, &newImage);
            break;
        }
        default:
        {
            ret = (uint8)PB_FOTA_UNKNOW_PROT;
            break;
        }
    }
    if (ret != PB_FOTA_DOWNLOAD_OK)
    {
        goto end;
    }
    //display up process in screen
    pb_gui_set_upgrade_info(PB_GUI_UP_VERIFY, 90);

    //firmware MD5 check
    uint8 md5Data[PB_FOTA_MD5_LEN];
    md5((uint8*)newImage.romBegin, param->totalSize, md5Data);
    if (memcmp(md5Data, param->md5, 16) != 0)
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "Err MD5:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                            md5Data[0], md5Data[1], md5Data[2], md5Data[3], 
                            md5Data[4], md5Data[5], md5Data[6], md5Data[7], 
                            md5Data[8], md5Data[9], md5Data[10], md5Data[11], 
                            md5Data[12], md5Data[13], md5Data[14], md5Data[15]);

        ret =  PB_FOTA_DOWNLOAD_MD5_ERR;
        goto end;
    }

    if (!pb_fota_version_check(newImage.romBegin))
    {
        ret = PB_FOTA_DOWNLOAD_VER_ERR;
        goto end;
    }

    //update Image content
    newImage.length = param->totalSize;
    newImage.crc = 0;
    newImage.upgradeTimestamp = pb_util_get_timestamp();
    newImage.runTimes = 0;
    newImage.version += 1;

    fwManager.submit(&newImage);    
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "NEW IMAGE SUBMIT");
    //display up process in screen
    pb_gui_set_upgrade_info(PB_GUI_UP_OK, 100);

end:
    return ret;
}

/******************************************************************************
* Function    : pb_fota_upgrade
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_fota_upgrade(void)
{
    //wait FOTA start RSP sent out
    uint32 startTime = pb_util_get_timestamp();
    while (pb_ota_network_send_que_size() != 0)
    {
        if (pb_util_get_timestamp() - startTime >= PB_FOTA_UPGRADE_START_TIMEOUT)
        {
            pb_fota_send_fota_rsp(PB_FOTA_DOWNLOAD_ERR, 0);
            OS_DBG_ERR(DBG_MOD_PBFOTA, "Wait fota start rsp send timeout");
            return;
        }
        os_scheduler_delay(DELAY_500_MS);
        OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Wait send queue message clear");
    }

    //start upgrading
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Start FOTA...");
    b_fota_upgrading = true;
    
    //turn on display upgrading info in OLED
    pb_gui_send_act_req(PB_GUI_ACT_UPGRADEMENU);
    pb_gui_set_upgrade_info(PB_GUI_UP_START, 0);

    uint8 fotaRet;
    fotaRet = pb_fota_process(&pb_fota_context);
    
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "FOTA BIN DOWNLOAD[%02X]", fotaRet);
    pb_fota_send_fota_rsp(fotaRet, pb_fota_context.curRetryCnt);

    b_fota_upgrading = false;

    if (fotaRet == PB_FOTA_DOWNLOAD_OK)
    {
        pb_fota_send_fota_rsp(PB_FOTA_UPGRADE_START, pb_fota_context.curRetryCnt);
        pb_prot_send_rsp_req(PB_PROT_RSP_PFE);

        pb_gui_set_upgrade_info(PB_GUI_UP_REBOOT, 100);
        pb_ota_need_set_reboot(true);
    }
    else
    {
        //turn off display upgrading info in OLED
        pb_gui_send_act_req(PB_GUI_ACT_UPGRADEMENU);
    }
}

/******************************************************************************
* Function    : pb_fota_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : fota context init
******************************************************************************/
static void pb_fota_init(void)
{
    memset(&pb_fota_context, 0, sizeof(pb_fota_context));
    b_fota_upgrading = false;
    
    pb_fota_msg_queue = os_msg_queue_create(PB_FOTA_MSGQUE_SIZE, sizeof(PB_MSG_TYPE));
}

/******************************************************************************
* Function    : pb_fota_main
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_fota_main(void *parameters)
{
    pb_fota_init();
    
    os_set_task_init(OS_TASK_ITEM_PB_FOTA);
    os_wait_task_init_sync();

    //confirm image
    pb_fota_firmware_confirm();
    SYSLED.set(PB_IO_INDICATOR_SYS_NORAML);

    PB_MSG_TYPE pb_msg;
    PB_MSG_TYPE *p_pb_msg = &pb_msg;
    memset(p_pb_msg, 0, sizeof(PB_MSG_TYPE));

    while (1)
    {
        //need some blocking function in case of wasting cpu      
        if (OS_MSG_RECV_FAILED != os_msg_queue_recv(pb_fota_msg_queue, p_pb_msg, OS_MSG_BLOCK_WAIT))
        {
            OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Recv msg %d", p_pb_msg->msgID);
            switch (p_pb_msg->msgID)
            {
                case PB_MSG_FOTA_FIRMWARE_UPGRADE_REQ:
                {
                    pb_fota_req_hdlr();
                    break;
                }
                case PB_MSG_FOTA_START_UPGRADE_REQ:
                {
                    pb_fota_upgrade();
                    break;
                }
                default:
                {
                    OS_DBG_ERR(DBG_MOD_PBFOTA, "Unknow msg %d", p_pb_msg->msgID);
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

