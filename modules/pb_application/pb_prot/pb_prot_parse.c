/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_prot_parse.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   parse park box air protocol
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_cfg_proc.h"
#include "pb_prot_parse.h"
#include "pb_prot_main.h"
#include "pb_prot_proc.h"
#include "pb_crypto.h"
#include "pb_util.h"
#include "pb_order_main.h"

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static uint8 cipherBuff[PB_PROT_CIPHER_BUFFSIZE];
static uint8 plainTextBuff[PB_PROT_PLAINTEXT_BUFFSIZE];

static uint8 pbProtUartRawBuff[PB_PROT_UART_RAW_SIZE];
static uint8 pbProtOtaRawBuff[PB_PROT_OTA_RAW_SIZE];
/*
static uint8 pbProtBTRawBuff[PB_PROT_BT_RAW_SIZE];
static uint8 pbProtSMSRawBuff[PB_PROT_SMS_RAW_SIZE];
*/

const static PB_PROT_RAW_BUFF_TYPE pb_prot_raw_buff[PB_PORT_SRC_NUM] = 
{
    {PB_PROT_UART_RAW_SIZE, pbProtUartRawBuff},
    {PB_PROT_OTA_RAW_SIZE, pbProtOtaRawBuff},
    /* not used now
    {PB_PROT_BT_RAW_SIZE, pbProtBTRawBuff},
    {PB_PROT_SMS_RAW_SIZE, pbProtSMSRawBuff}
    */
};

static PB_PROT_RAW_CONTEXT_TYPE pbProtRawContext;

const char *pbProtSrcName[] =
{
    "UART",
    "OTA",
    "BT",
    "SMS"
};

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_prot_rawbuff_value_size
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get value data size in raw buff
******************************************************************************/
static uint16 pb_prot_rawbuff_value_size(uint8 id)
{
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    return rawQue->valuedataCnt;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_get_byte
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get one byte raw data with offset from header
******************************************************************************/
static uint8 pb_prot_rawbuff_get_byte(uint8 id, uint16 offset)
{
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    if (offset == 0)
    {
        return rawQue->pbProtRaw[rawQue->rawHead];
    }
    else
    {
        return rawQue->pbProtRaw[(rawQue->rawHead + offset) % rawQue->pbProtRawMaxSize];
    }
}

/******************************************************************************
* Function    : pb_prot_rawbuff_deque
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get assigned length from raw data with offset
******************************************************************************/
static uint16 pb_prot_rawbuff_get_buff(uint8 id, uint8 *buf, uint16 offset, uint16 len)
{
    uint16 first_len = 0;
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    if (rawQue->valuedataCnt < (offset + len))
    {
        return 0;
    }

    uint16 offsetHead = (rawQue->rawHead + offset) % (rawQue->pbProtRawMaxSize);
    first_len = (rawQue->pbProtRawMaxSize) - offsetHead;

    if (len <= first_len)
    {
        memcpy(buf, &(rawQue->pbProtRaw[offsetHead]), len);
    }
    else
    {
        memcpy(buf, &(rawQue->pbProtRaw[offsetHead]), first_len);
        memcpy(&buf[first_len], rawQue->pbProtRaw, len - first_len);
    }

    return len;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_deque_byte
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get one byte from the header
******************************************************************************/
static uint8 pb_prot_rawbuff_deque_byte(uint8 id)
{
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    uint8 byte = pb_prot_rawbuff_get_byte(id, 0);

    rawQue->rawHead = (rawQue->rawHead + 1) % rawQue->pbProtRawMaxSize;
    rawQue->valuedataCnt--;

    return byte;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_enque
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : write raw data to buff
******************************************************************************/
static uint16  pb_prot_rawbuff_enque(uint8 id, uint8 *data, uint16 len)
{
    uint16 first_len;
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    //Save GPS data to buffer
    if ((len + rawQue->valuedataCnt) > (rawQue->pbProtRawMaxSize))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "[%s] rawbuff full", pbProtSrcName[id]);
        return 0;
    }

    first_len = rawQue->pbProtRawMaxSize - rawQue->rawTail;

    if (len <= first_len)
    {
        memcpy(&(rawQue->pbProtRaw[rawQue->rawTail]), data, len);
    }
    else
    {
        memcpy(&(rawQue->pbProtRaw[rawQue->rawTail]), data, first_len);
        memcpy(rawQue->pbProtRaw, &data[first_len], len - first_len);
    }

    rawQue->rawTail = (rawQue->rawTail + len) % rawQue->pbProtRawMaxSize;
    rawQue->valuedataCnt += len;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%s] en [%d] rawbuff, total[%d]",
                            pbProtSrcName[id], len, rawQue->valuedataCnt);

    return len;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_clear_header
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : discard datas from raw buff header
******************************************************************************/
static uint16 pb_prot_rawbuff_clear_header(uint8 id, uint16 len)
{
    PB_PROT_RAW_QUEUE_TYPE *rawQue = &(pbProtRawContext.rawQueue[id]);

    if (rawQue->valuedataCnt < len)
    {
        return 0;
    }

    rawQue->rawHead = (rawQue->rawHead + len) % rawQue->pbProtRawMaxSize;
    rawQue->valuedataCnt -= len;

    return len;
}

/******************************************************************************
* Function    : pb_prot_parse_u8
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get u8 from buff
******************************************************************************/
static uint16 pb_prot_parse_u8(uint8 *buff, uint8 *u8)
{
    (*u8) = buff[0];

    return 1;
}

/******************************************************************************
* Function    : pb_prot_parse_u16
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get u16 from buff
******************************************************************************/
static uint16 pb_prot_parse_u16(uint8 *buff, uint16 *u16)
{
    u16_u8_union u16_u8;

    #if (PB_PROT_NETWORK_BYTE_ORDER == 1)
    u16_u8.c[1] = buff[0];
    u16_u8.c[0] = buff[1];
    #else
    u16_u8.c[0] = buff[0];
    u16_u8.c[1] = buff[1];
    #endif /*PB_PROT_NETWORK_BYTE_ORDER*/

    (*u16) = u16_u8.num;

    return 2;
}

/******************************************************************************
* Function    : pb_prot_parse_u32
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get u32 from buff
******************************************************************************/
static uint16 pb_prot_parse_u32(uint8 *buff, uint32 *u32)
{
    u32_u8_union u32_u8;

    #if (PB_PROT_NETWORK_BYTE_ORDER == 1)
    u32_u8.c[3] = buff[0];
    u32_u8.c[2] = buff[1];
    u32_u8.c[1] = buff[2];
    u32_u8.c[0] = buff[3];
    #else
    u32_u8.c[0] = buff[0];
    u32_u8.c[1] = buff[1];
    u32_u8.c[2] = buff[2];
    u32_u8.c[3] = buff[3];
    #endif /*PB_PROT_NETWORK_BYTE_ORDER*/

    (*u32) = u32_u8.num;

    return 4;
}

/******************************************************************************
* Function    : pb_prot_parse_u8array
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get u8 array from buff
******************************************************************************/
static uint16 pb_prot_parse_u8_array(uint8 *buff, uint8 *dst, uint16 len)
{
    memcpy(dst, buff, len);

    return len;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_header
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather header is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_header(uint8 id)
{
    return (bool)((pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_HEADER_POS) == PB_PROT_HEADFLAG_PLUSE)
                  && (pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_HEADER_POS + 1) == PB_PROT_HEADFLAG_P)
                  && (pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_HEADER_POS + 2) == PB_PROT_HEADFLAG_B));
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_header_crc
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_parse_check_cmd_header_crc(uint8 id)
{
    uint8 buff[2];
    buff[0] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_HEAD_CHECKCODE_POS);
    buff[1] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_HEAD_CHECKCODE_POS + 1);

    uint16 crc;
    pb_prot_parse_u16(buff, &crc);

    uint8 headBuff[PB_PROT_PARSE_CMD_HEAD_CHECK_LEN];
    if (PB_PROT_PARSE_CMD_HEAD_CHECK_LEN != pb_prot_rawbuff_get_buff(id, headBuff, PB_PROT_PARSE_CMD_HEADER_POS, PB_PROT_PARSE_CMD_HEAD_CHECK_LEN))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad header");
        return false;
    }

    uint16 calCrc = pb_util_get_crc16(headBuff, PB_PROT_PARSE_CMD_HEAD_CHECK_LEN);
    if (crc != calCrc)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Head CRC err[%04X], need[%04X]", crc, calCrc);
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_device_type
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather device type is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_device_type(uint8 id)
{
    uint8 devType = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_DEVICETYPE_POS);

    #if 0 // server is not available for device type, disabled it now
    //(bool)(pb_util_get_device_type() == devType)
    #endif
    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_protocol_version
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather protocol version is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_protocol_version(uint8 id)
{
    uint8 buff[2];
    buff[0] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_PROTVER_POS);
    buff[1] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_PROTVER_POS + 1);

    uint16 protVer;
    pb_prot_parse_u16(buff, &protVer);

    //return (bool)(pb_util_get_protocol_version() >= protVer);
    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_uid
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather uid is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_uid(uint8 id)
{
    for (int idx = 0; idx < PB_UID_LEN; ++idx)
    {
        if (pb_cfg_proc_get_uid()[idx] != pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_UID_POS + idx))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_sendime
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather send time is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_sendime(uint8 *buff)
{
    uint32 sendtime;
    pb_prot_parse_u32(buff, &sendtime);

    //check send time vaildity
    if (sendtime == 0)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad send time[%08X]", sendtime);
        return false;
    }

    pb_prot_proc_update_rtc(sendtime);
    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_sn
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather serial number is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_sn(uint8 *buff, uint16 *sn)
{
    pb_prot_parse_u16(buff, sn);

    //check serial number vaildity
    //PB_DEBUG_TRACE(DBG_TRACE_PARSE, "Serial number [%04X]",  (*sn));

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_message_type
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather message type is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_message_type(uint8 *buff, uint8 *msgType)
{
    pb_prot_parse_u8(buff, msgType);

    if ((*msgType) != PB_PROT_CMD
        && (*msgType) != PB_PROT_ACK)
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_submessage_type
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather sub-message type is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_submessage_type(uint8 *buff, uint8 *subMsgType)
{
    pb_prot_parse_u8(buff, subMsgType);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_content
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static uint8 pb_prot_parse_check_cmd_content(uint8 id, PB_PROT_PARSED_CONTENT_TYPE *parsedContent)
{
    uint8 buff[2];
    buff[0] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_LENGTH_POS);
    buff[1] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_LENGTH_POS + 1);

    uint16 cipherLen, cipherFlag;
    pb_prot_parse_u16(buff, &cipherLen);
    cipherFlag = cipherLen & 0xC000;
    cipherFlag = cipherFlag >> 14;
    cipherLen &= 0xFFF;

    if (cipherLen != pb_prot_rawbuff_get_buff(id, cipherBuff, PB_PROT_PARSE_CMD_CIPHER_POS, cipherLen))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Get content err, data is less than length");
        return PROT_PARSE_NEED_WAIT;
    }

    buff[0] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_CHECKCODE_POS(cipherLen));
    buff[1] = pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_CHECKCODE_POS(cipherLen) + 1);

    uint16 crc;
    pb_prot_parse_u16(buff, &crc);
    uint16 calCrc = pb_util_get_crc16(cipherBuff, cipherLen);
    if (crc != calCrc)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "CRC err[%04X], need[%04X]", crc, calCrc);
        return PROT_PARSE_ERR;
    }

    uint16 plainLen = 0;
    if (cipherFlag == 0x01)
    {
        plainLen = pb_decrypt(cipherBuff, cipherLen, pb_crypto_get_key(), plainTextBuff);
    }
    else
    {
        memcpy(plainTextBuff, cipherBuff, cipherLen);
        plainLen =  cipherLen;
    }
    if (plainLen == 0)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Decrypt err");
        return PROT_PARSE_ERR;
    }
    
    #if ( PB_PROT_DBG == 1 )
    char recvHex[128+1];
    uint16 hexLen = MIN_VALUE(plainLen, 120/2);
    os_trace_get_hex_str((uint8*)recvHex, sizeof(recvHex), plainTextBuff, hexLen);
    OS_INFO("RECV:%s", recvHex);
    #else
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Recv [%d]", plainLen);
    #endif /*PB_PROT_DBG*/

    //send time
    if (!pb_prot_parse_check_cmd_sendime(&plainTextBuff[PB_PROT_PARSE_CMD_SENDTIME_POS]))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad sendtime");
        return PROT_PARSE_ERR;
    }

    //serial number
    if (!pb_prot_parse_check_cmd_sn(&plainTextBuff[PB_PROT_PARSE_CMD_SN_POS], &(parsedContent->serialNumber)))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad sn");
        return PROT_PARSE_ERR;
    }

    //message type
    if (!pb_prot_parse_check_cmd_message_type(&plainTextBuff[PB_PROT_PARSE_CMD_MSGTYPE_POS], &(parsedContent->msgType)))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad msg type");
        return PROT_PARSE_ERR;
    }

    //message sub type
    if (!pb_prot_parse_check_cmd_submessage_type(&plainTextBuff[PB_PROT_PARSE_CMD_MSGSUBTYPE_POS], &(parsedContent->msgSubType)))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad sub-msg type");
        return PROT_PARSE_ERR;
    }

    //content
    parsedContent->content = &plainTextBuff[PB_PROT_PARSE_CMD_CONTENT_POS];
    parsedContent->contentLen = (cipherLen - PB_PROT_PARSE_CMD_CIPHER_FIX_SIZE);

    return PROT_PARSE_OK;
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_tail
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather tail is valid
******************************************************************************/
static bool pb_prot_parse_check_cmd_tail(uint8 id, uint16 pos)
{
    return (bool)((pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_TAIL_POS(pos)) == PB_PROT_TAILFLAG_R)
                  && (pb_prot_rawbuff_get_byte(id, PB_PROT_PARSE_CMD_TAIL_POS(pos) + 1) == PB_PROT_TAILFLAG_N));
}

/******************************************************************************
* Function    : pb_prot_parse_check_cmd_valid
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check wheather the command message is valid
******************************************************************************/
static uint8 pb_prot_parse_check_cmd_valid(uint8 id, PB_PROT_PARSED_CONTENT_TYPE *parsedContent)
{
    uint16 msgLen = 0;

    //header
    if (!pb_prot_parse_check_cmd_header(id))
    {
        return PROT_PARSE_ERR;
    }

    //check header crc
    if (!pb_prot_parse_check_cmd_header_crc(id))
    {
        return PROT_PARSE_ERR;
    }

    //device type
    if (!pb_prot_parse_check_cmd_device_type(id))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad dev type");
        return PROT_PARSE_ERR;
    }

    //protocol version
    if (!pb_prot_parse_check_cmd_protocol_version(id))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad prot ver");
        return PROT_PARSE_ERR;
    }

    //unique ID
    if (!pb_prot_parse_check_cmd_uid(id))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad UID");
        return PROT_PARSE_ERR;
    }

    //check crc
    uint8 ret = pb_prot_parse_check_cmd_content(id, parsedContent);
    if (ret != PROT_PARSE_OK)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Parse content err %d", ret);
        return ret;
    }

    msgLen = parsedContent->contentLen + PB_PROT_PARSE_CMD_CIPHER_FIX_SIZE;

    //check tail
    if (!pb_prot_parse_check_cmd_tail(id, msgLen))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad tail");
        return PROT_PARSE_ERR;
    }

    msgLen += PB_PROT_PARSE_CMD_PLAINTEXT_SIZE;
    if (msgLen != pb_prot_rawbuff_clear_header(id, msgLen))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Clear [%s] rawbuff err", pbProtSrcName[id]);
        return PROT_PARSE_ERR;
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "CMD[%d]:[%02X %02X]",
                            parsedContent->contentLen,
                            parsedContent->msgType,
                            parsedContent->msgSubType);

    return PROT_PARSE_OK;
}

/******************************************************************************
* Function    : pb_prot_parse_rawbuff
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : find valid command in rawbuff
******************************************************************************/
static bool pb_prot_parse_rawbuff(uint8 id, PB_PROT_PARSED_CONTENT_TYPE *parsedContent)
{
    uint8 ret = PROT_PARSE_ERR;
    while (pb_prot_rawbuff_value_size(id) >= PB_PROT_CMD_MIN_LENGTH)
    {
        pb_prot_set_need_wait_data(false);
        ret = pb_prot_parse_check_cmd_valid(id, parsedContent);
        if (ret == PROT_PARSE_OK)
        {
            OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%s]parsed content[%02X %02X], parse[%d]",
                                     pbProtSrcName[id],
                                     parsedContent->msgType,
                                     parsedContent->msgSubType,
                                     ret);
            return true;
        }
        else 
        if (ret == PROT_PARSE_NEED_WAIT)
        {
            if (pb_prot_rawbuff_value_size(id) < pbProtRawContext.rawQueue[id].pbProtRawMaxSize)
            {
                pb_prot_set_need_wait_data(true);
                return false;
            }
            else
            {
                OS_DBG_ERR(DBG_MOD_PBPROT, "cmd is incompleted and buffer[%d] is full, discard", id);
                pb_prot_rawbuff_deque_byte(id);
            }
        }
        else 
        if (ret == PROT_PARSE_ERR)
        {
            #if ( PB_PROT_DBG == 1 )
            uint8 discardByte = pb_prot_rawbuff_deque_byte(id);
            OS_DBG_ERR(DBG_MOD_PBPROT, "discard:%02X", discardByte);
            #else
            pb_prot_rawbuff_deque_byte(id);
            #endif /*PB_PROT_DBG*/
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "[%s] has no valid cmd", pbProtSrcName[id]);

    return false;
}

/******************************************************************************
* Function    : check_content_len
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check current pointer wheather overflow
******************************************************************************/
static bool check_content_len(uint8 *pdata, uint8 *pcontent, uint16 len)
{
    if (pdata - pcontent > len)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "pdata overflow[%d]:[%p - %p]", len, pdata, pcontent);
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_apc
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : access point configuration content parse
******************************************************************************/
static bool pb_prot_parse_cmd_apc(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    //Filed length
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint16 filedLen;
    pdata += pb_prot_parse_u16(pdata, &filedLen);

    uint8 apnLen = (filedLen & 0x003F);
    uint8 usrLen = ((filedLen >> 6) & 0x001F);
    uint8 passLen = ((filedLen >> 11) & 0x001F);

    if ((apnLen > PB_APN_LEN)
            || (usrLen > PB_APN_USR_LEN)
            || (passLen > PB_APN_PASS_LEN))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Field length err, [%d, %d, %d]", apnLen, usrLen, passLen);
        return false;
    }

    //APN
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.apc.apn, apnLen);
    frame->arg.apc.apn[apnLen] = '\0';
    if (strlen((char*)frame->arg.apc.apn) != apnLen)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "apn err");
        return false;
    }

    //APN usr name
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.apc.usr, usrLen);
    frame->arg.apc.usr[usrLen] = '\0';
    if (strlen((char*)frame->arg.apc.usr) != usrLen)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "usr err");
        return false;
    }

    //APN password
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.apc.pass, passLen);
    frame->arg.apc.pass[passLen] = '\0';
    if (strlen((char*)frame->arg.apc.pass) != passLen)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "pass err");
        return false;
    }

    //Main DNS server
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.apc.mainDNS, PB_APN_DNS_LEN);

    //Backup DNS server
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.apc.bkDNS, PB_APN_DNS_LEN);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "APN---apn[%d,%s], usr[%d,%s], pass[%d,%s], DNS1[%d.%d.%d.%d], DNS2[%d.%d.%d.%d]",
                            apnLen, frame->arg.apc.apn,
                            usrLen, frame->arg.apc.usr,
                            passLen, frame->arg.apc.pass,
                            frame->arg.apc.mainDNS[0], frame->arg.apc.mainDNS[1], frame->arg.apc.mainDNS[2], frame->arg.apc.mainDNS[3],
                            frame->arg.apc.bkDNS[0], frame->arg.apc.bkDNS[1], frame->arg.apc.bkDNS[2], frame->arg.apc.bkDNS[3]);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_ser
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : server configuration content parse
******************************************************************************/
static bool pb_prot_parse_cmd_ser(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    //Filed length
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 filedLen[3];
    pdata += pb_prot_parse_u8_array(pdata, filedLen, sizeof(filedLen));

    uint8 mainAddrLen = (filedLen[2] & 0x3F);
    uint8 bakAddrLen = (((filedLen[1] & 0x0F) << 2) + (filedLen[2] >> 6));
    uint8 smsLen = (((filedLen[0] & 0x01) << 4) + ((filedLen[1] >> 4) & 0x0F));

    if ((mainAddrLen > PB_SER_DOMAINNAME_LEN)
            || (bakAddrLen > PB_SER_DOMAINNAME_LEN)
            || (smsLen > PB_SER_SMS_GATEWAY_LEN))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "[%02X %02X %02X]filed len err, [%d, %d, %d]",
                             filedLen[0], filedLen[1], filedLen[2],
                             mainAddrLen, bakAddrLen, smsLen);
        return false;
    }

    //Report mode
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 mode;
    pdata += pb_prot_parse_u8(pdata, &mode);
    frame->arg.ser.mode = mode;

    //Main server domain name
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.ser.mainServer, mainAddrLen);
    frame->arg.ser.mainServer[mainAddrLen] = '\0';

    //Main server port
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint16 mainPort;
    pdata += pb_prot_parse_u16(pdata, &mainPort);
    frame->arg.ser.mainPort = mainPort;

    //Backup server domain
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.ser.bakServer, bakAddrLen);
    frame->arg.ser.bakServer[bakAddrLen] = '\0';

    //Backup server port
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint16 bakPort;
    pdata += pb_prot_parse_u16(pdata, &bakPort);
    frame->arg.ser.bakPort = bakPort;

    //SMS gateway
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.ser.smsGateway, smsLen);
    frame->arg.ser.smsGateway[smsLen] = '\0';

    //Heartbeat interval
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 hbpInterval;
    pdata += pb_prot_parse_u8(pdata, &hbpInterval);
    frame->arg.ser.hbpInterval = hbpInterval;

    //Max random time
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint16 randomTime;
    pdata += pb_prot_parse_u16(pdata, &randomTime);
    frame->arg.ser.randomTime = randomTime;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "SER---mode[%d], main[%d,%s:%d], bak[%d,%s:%d], sms[%d,%s], hbp[%d], random[%d]",
                            frame->arg.ser.mode,
                            mainAddrLen, frame->arg.ser.mainServer, frame->arg.ser.mainPort,
                            bakAddrLen, frame->arg.ser.bakServer, frame->arg.ser.bakPort,
                            smsLen, frame->arg.ser.smsGateway,
                            frame->arg.ser.hbpInterval, frame->arg.ser.randomTime);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_cfg
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : global configuration content parse
******************************************************************************/
static bool pb_prot_parse_cmd_cfg(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint16 tmp16;

    if (content->contentLen < PB_CFG_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Event mask
    pdata += pb_prot_parse_u16(pdata, &tmp16);
    frame->arg.cfg.eventMask = tmp16;
    //Info report interval
    pdata += pb_prot_parse_u16(pdata, &tmp16);
    frame->arg.cfg.infInterval = tmp16;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "CFG---event[%04X], interval[%d]",
                            frame->arg.cfg.eventMask, frame->arg.cfg.infInterval);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_tma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : time adjust content parse
******************************************************************************/
static bool pb_prot_parse_cmd_tma(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    if (content->contentLen < PB_TMA_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }

    //Time adjust
    uint16 adjust;
    pdata += pb_prot_parse_u16(pdata, &adjust);

    uint8 sign = (adjust & 0x01);
    uint8 hour = ((adjust >> 1) & 0x1F);
    uint8 minute = ((adjust >> 6) & 0x3F);
    uint8 daylightSaving = ((adjust >> 12) & 0x01);
    uint8 mode = ((adjust >> 13) & 0x07);

    if (sign > PB_TMA_SIGN_MAX)
    {
        return false;
    }
    frame->arg.tma.sign = sign;

    if (hour > PB_TMA_HOUR_MAX)
    {
        return false;
    }
    frame->arg.tma.hour = hour;

    if (hour > PB_TMA_MINUTE_MAX)
    {
        return false;
    }
    frame->arg.tma.minute = minute;

    if (hour > PB_TMA_DLS_MAX)
    {
        return false;
    }
    frame->arg.tma.daylightSaving = daylightSaving;
    frame->arg.tma.mode = mode;

    //UTC time
    uint32 timestamp;
    pdata += pb_prot_parse_u32(pdata, &timestamp);
    frame->arg.tma.timestamp = timestamp;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "TMA---mode[%d], adjust[%c %02d:%02d, %d], timestamp[%08X]",
                            frame->arg.tma.mode,
                            (frame->arg.tma.sign == 0 ? '-' : '+'),
                            frame->arg.tma.hour, frame->arg.tma.minute,
                            frame->arg.tma.daylightSaving,
                            frame->arg.tma.timestamp);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_dog
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : protocol watchdog content parse
******************************************************************************/
static bool pb_prot_parse_cmd_dog(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;
    uint16 tmp16;

    if (content->contentLen < PB_DOG_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Mode
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.dog.mode = tmp8 & 0x03;
    frame->arg.dog.report = (tmp8 >> 2) & 0x01;
    frame->arg.dog.interval = (tmp8 >> 3) & 0x1F;
    //Reboot time
    pdata += pb_prot_parse_u16(pdata, &tmp16);
    frame->arg.dog.rstHour = (tmp16 >> 6) & 0x1F;
    frame->arg.dog.rstMinute = tmp16 & 0x3F;
    //Max random time
    pdata += pb_prot_parse_u16(pdata, &tmp16);
    frame->arg.dog.randomDuration = tmp16;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                   "DOG---mode[%d], report[%d], interval[%d], time[%02d:%02d], random[%d]",
                   frame->arg.dog.mode, frame->arg.dog.report, frame->arg.dog.interval,
                   frame->arg.dog.rstHour, frame->arg.dog.rstMinute,
                   frame->arg.dog.randomDuration);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_aco
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : air conditioner operation content parse
******************************************************************************/
static bool pb_prot_parse_cmd_aco(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;

    if (content->contentLen < PB_ACO_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Mode
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.aco.pwrMode = tmp8 & 0x03;
    frame->arg.aco.workMode = (tmp8 >> 2) & 0x03;
    frame->arg.aco.windLevel = (tmp8 >> 4) & 0x03;
    //Interval
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.aco.interval = tmp8;
    //Duration
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.aco.duration = tmp8;
    //Temperature
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.aco.temperature = tmp8;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "ACO---pwr[%d], work[%d], wind[%d], interval[%d], duration[%d], temperature[%d]",
                            frame->arg.aco.pwrMode, frame->arg.aco.workMode, frame->arg.aco.windLevel,
                            frame->arg.aco.interval, frame->arg.aco.duration, frame->arg.aco.temperature);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_sec
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : security configuration content parse
******************************************************************************/
static bool pb_prot_parse_cmd_sec(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    if (content->contentLen < PB_SEC_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }

    bool isEncrypt = true;
    
    //Key type
    uint8 keyType;
    pdata += pb_prot_parse_u8(pdata, &keyType);
    if (0x0F == ((keyType >> 4) & 0x0F))
    {
        isEncrypt = false;
        frame->arg.sec.keyType = (keyType & 0x0F);
    }
    else
    {
        frame->arg.sec.keyType = keyType;
    }

    //Key data
    uint8 keyData[PB_SEC_KEY_DATA_LEN];
    uint8 keyHeader[] = PB_SEC_KEY_HEADER;
    uint8 keyTail[] = PB_SEC_KEY_TAIL;
    memset(keyData, 0, sizeof(keyData));
    pb_prot_parse_u8_array(pdata, keyData, PB_SEC_KEY_DATA_LEN);

    if (isEncrypt)
    {
        //decrypt for keyData
    }

    //check header and tail
    if (0 != memcmp(&keyData[PB_SEC_KEY_HEADER_OFFSET], keyHeader, 4)
        || 0 != memcmp(&keyData[PB_SEC_KEY_TAIL_OFFSET], keyTail, 4))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Bad key data");
        return false;
    }

    //check crc
    uint16 crc;
    pb_prot_parse_u16(&keyData[PB_SEC_KEY_CRC_OFFSET], &crc);
    uint16 calCrc = pb_util_get_crc16(keyData, PB_SEC_KEY_CRC_OFFSET);
    if (crc != calCrc)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "KEY CRC err[%04X], need[%04X]", crc, calCrc);
        return false;
    }

    //Key
    pb_prot_parse_u8_array(&keyData[PB_SEC_KEY_OFFSET], frame->arg.sec.key, PB_SEC_KEY_LEN);

    char keyStr[PB_SEC_KEY_LEN * 2 + 1];
    os_trace_get_hex_str((uint8*)keyStr, sizeof(keyStr), frame->arg.sec.key, PB_SEC_KEY_LEN);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "SEC---key[%d][%s]", frame->arg.sec.keyType, keyStr);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_omc
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_parse_cmd_omc(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    if (content->contentLen < PB_OMC_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }

    //Idle output
    pdata += pb_prot_parse_u32(pdata, &frame->arg.omc.idleOutput);

    //In-service output
    pdata += pb_prot_parse_u32(pdata, &frame->arg.omc.inServiceOutput);

    //Mode
    pdata += pb_prot_parse_u8(pdata, &frame->arg.omc.mode);

    //Valid time
    uint32 validTime;
    pdata += pb_prot_parse_u32(pdata, &validTime);

    frame->arg.omc.startHour = (validTime & 0x1F);
    frame->arg.omc.startMin = ((validTime >> 5)  & 0x3F);
    frame->arg.omc.stopHour = ((validTime >> 11) & 0x1F);
    frame->arg.omc.stopMin = ((validTime >> 16)  & 0x3F);

    //Valid time idle output
    pdata += pb_prot_parse_u32(pdata, &frame->arg.omc.validIdleOutput);

    //Valid time in-service output
    pdata += pb_prot_parse_u32(pdata, &frame->arg.omc.validInServiceOutput);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "OMC---idle[%08X], in-service[%08X], mode[%d, %02d:%02d-%02d:%02d], "
                            "valid time idle[%08X], valid time in-service[%08X]",
                            frame->arg.omc.idleOutput, frame->arg.omc.inServiceOutput,
                            frame->arg.omc.mode, frame->arg.omc.startHour, frame->arg.omc.startMin,
                            frame->arg.omc.stopHour, frame->arg.omc.stopMin,
                            frame->arg.omc.validIdleOutput, frame->arg.omc.validInServiceOutput);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_acw
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_prot_parse_cmd_acw(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    if (content->contentLen < PB_ACW_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }

    //mode
    pdata += pb_prot_parse_u8(pdata, &frame->arg.acw.mode);

    //power on event mask
    pdata += pb_prot_parse_u8(pdata, &frame->arg.acw.pwrOnEventMask);

    //power off event mask
    pdata += pb_prot_parse_u8(pdata, &frame->arg.acw.pwrOffEventMask);

    //duration
    pdata += pb_prot_parse_u8(pdata, &frame->arg.acw.duration);

    //Valid time
    uint32 validTime;
    pdata += pb_prot_parse_u32(pdata, &validTime);

    frame->arg.acw.startHour= (validTime & 0x1F);
    frame->arg.acw.startMin = ((validTime >> 5)  & 0x3F);
    frame->arg.acw.stopHour = ((validTime >> 11) & 0x1F);
    frame->arg.acw.stopMin = ((validTime >> 16)  & 0x3F);

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "ACW---mode[%d], pwr on[%02X], pwr off[%02X], duration[%d], vaild time[%02d:%02d-%02d:%02d]",
                            frame->arg.acw.mode,
                            frame->arg.acw.pwrOnEventMask, frame->arg.acw.pwrOffEventMask,
                            frame->arg.acw.duration,
                            frame->arg.acw.startHour, frame->arg.acw.startMin,
                            frame->arg.acw.stopHour, frame->arg.acw.stopMin);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_doa
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_parse_cmd_doa(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;

    if (content->contentLen < PB_DOA_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Mode
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.doa.mode = tmp8;
    //Trigger type
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.doa.triggerType = tmp8;
    //Duration
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.doa.duration = tmp8;
    //Send Interval
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.doa.interval = tmp8;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "DOA---mode[%d], trigger[%d], duration[%d], interval[%d]",
                            frame->arg.doa.mode, frame->arg.doa.triggerType,
                            frame->arg.doa.duration, frame->arg.doa.interval);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_sma
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : smoke alarm configuration content parse
******************************************************************************/
static bool pb_prot_parse_cmd_sma(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;

    if (content->contentLen < PB_SMA_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Mode
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.sma.mode = tmp8;
    //Threshold
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.sma.threshold = tmp8;
    //Duration
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.sma.duration = tmp8;
    //Send Interval
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.sma.interval = tmp8;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "SMA---mode[%d], threshold[%d], duration[%d], interval[%d]",
                            frame->arg.sma.mode, frame->arg.sma.threshold,
                            frame->arg.sma.duration, frame->arg.sma.interval);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_ouo
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : update order to local order list
******************************************************************************/
static bool pb_prot_parse_cmd_ouo(PB_PROT_PARSED_CONTENT_TYPE *content, PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 number, tmp8;
    uint8 act;
    PB_PROT_ORDER_TYPE order;

    pdata += pb_prot_parse_u8(pdata, &number);
    if (content->contentLen < PB_OUO_CONTENT_LEN(number))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error [%d] %d", number, content->contentLen);
        return false;
    }

    for (int idx = 0; idx < number; ++idx)
    {
        //Action type
        pdata += pb_prot_parse_u8(pdata, &act);
        //Order ID
        pdata += pb_prot_parse_u32(pdata, &(order.id));
        //Start datetime
        pdata += pb_prot_parse_u32(pdata, &(order.startTime));
        //Expire datetime
        pdata += pb_prot_parse_u32(pdata, &(order.expireTime));

        if (PB_OUO_ACT_ADD_SERVICE == act)
        {
            //Password
            uint32 passwd32;
            pdata += pb_prot_parse_u32(pdata, &passwd32);
            order.passwd = passwd32;
        }
        else
        {
            //Password
            uint16 passwd16;
            pdata += pb_prot_parse_u16(pdata, &passwd16);
            order.passwd = passwd16;
            //Person number
            pdata += pb_prot_parse_u8(pdata, &tmp8);
            order.personNum = tmp8;
            //Password valid count
            pdata += pb_prot_parse_u8(pdata, &tmp8);
            order.passwdValidCnt = tmp8;
        }

        OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                                "OUO[%d]---act[%d], id[%08X], start[%08X], expire[%08X], pass[%d], person[%d], pass cnt[%d]",
                                idx, act, order.id, order.startTime, order.expireTime,
                                order.passwd, order.personNum, order.passwdValidCnt);

        //pb_order_updating_set(true);
        if (act == PB_OUO_ACT_ADD_NORMAL
            || act == PB_OUO_ACT_ADD_SERVICE)
        {
            //add new order
            pb_order_booking(&order);
        }
        else 
        if (act == PB_OUO_ACT_REMOVE)
        {
            //delete order
            pb_order_cancel(&order);
        }
        else
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Bad action [%d]", act);
        }
        //pb_order_updating_set(false);
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_out
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : output operation content parse
******************************************************************************/
static bool pb_prot_parse_cmd_out(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;
    uint32 tmp32;

    if (content->contentLen < PB_OUT_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.out.pinIdx = tmp8;
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.out.pinState = tmp8;
    pdata += pb_prot_parse_u32(pdata, &tmp32);
    frame->arg.out.ctrlMask = tmp32;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "OUT---idx[%d], state[%d], ctrlMask[%08X]",
                            frame->arg.out.pinIdx, frame->arg.out.pinState,
                            frame->arg.out.ctrlMask);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_muo
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : multimedia operation content parse
******************************************************************************/
static bool pb_prot_parse_cmd_muo(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;

    if (content->contentLen < PB_MUO_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //type&act
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.muo.type = tmp8 & 0x0F;
    frame->arg.muo.act = (tmp8 >> 4) & 0x0F;
    //Volume
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.muo.volume = tmp8;
    //Media file name
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.muo.fileIdx = tmp8;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                   "MUO---type[%d], act[%d], volume[%d], file[%d]",
                   frame->arg.muo.type, frame->arg.muo.act,
                   frame->arg.muo.volume, frame->arg.muo.fileIdx);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_rto
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : real time operation content parse
******************************************************************************/
static bool pb_prot_parse_cmd_rto(PB_PROT_PARSED_CONTENT_TYPE *content,
                                  PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;
    uint8 tmp8;

    if (content->contentLen < PB_RTO_CONTENT_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
        return false;
    }
    //Command
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.rto.cmd = tmp8;
    //Sub command
    pdata += pb_prot_parse_u8(pdata, &tmp8);
    frame->arg.rto.subCmd = tmp8;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                             "RTO---cmd[%d], sub[%d]",
                             frame->arg.rto.cmd, frame->arg.rto.subCmd);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_fota
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : firmware over the air
******************************************************************************/
static bool pb_prot_parse_cmd_fota(PB_PROT_PARSED_CONTENT_TYPE *content,
                                   PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    uint8 *pdata = content->content;

    //Retry times
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 retry;
    pdata += pb_prot_parse_u8(pdata, &retry);
    frame->arg.fota.retry = retry;

    //Download timeout
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 timeout;
    pdata += pb_prot_parse_u8(pdata, &timeout);
    frame->arg.fota.timeout = timeout;

    //Download protocol
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 dwnProtocol;
    pdata += pb_prot_parse_u8(pdata, &dwnProtocol);
    frame->arg.fota.protocol = dwnProtocol;

    //Server URL length
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 urlLen;
    pdata += pb_prot_parse_u8(pdata, &urlLen);

    if (urlLen > PB_FOTA_URL_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "FOTA url len[%d] err", urlLen);
        return false;
    }

    //Server URL
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.fota.url, urlLen);
    frame->arg.fota.url[urlLen] = '\0';

    //Server port
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint16 port;
    pdata += pb_prot_parse_u16(pdata, &port);
    frame->arg.fota.port = port;

    //Server user name length
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 usrLen;
    pdata += pb_prot_parse_u8(pdata, &usrLen);

    if (usrLen > PB_FOTA_USR_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "FOTA usr name len[%d] err", usrLen);
        return false;
    }

    //Server user name
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.fota.usrName, usrLen);
    frame->arg.fota.usrName[usrLen] = '\0';

    //Server user pass length
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint8 passLen;
    pdata += pb_prot_parse_u8(pdata, &passLen);

    if (passLen > PB_FOTA_PASS_LEN)
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "FOTA usr pass len[%d] err", passLen);
        return false;
    }

    //Server user pass
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.fota.usrPass, usrLen);
    frame->arg.fota.usrPass[passLen] = '\0';

    //MD5
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    pdata += pb_prot_parse_u8_array(pdata, frame->arg.fota.md5, 16);

    //Key
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint32 key;
    pdata += pb_prot_parse_u32(pdata, &key);
    frame->arg.fota.key = key;

    //Download address
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint32 dwnAddr;
    pdata += pb_prot_parse_u32(pdata, &dwnAddr);
    frame->arg.fota.downAddr = dwnAddr;

    //App bootup address
    if (!check_content_len(pdata, content->content, content->contentLen))
    {
        return false;
    }
    uint32 bootAddr;
    pdata += pb_prot_parse_u32(pdata, &bootAddr);
    frame->arg.fota.bootAddr = bootAddr;

    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO,
                            "FOTA---retry[%d], timeout[%d], protocol[%d], url[%s:%d], auth[%s:%s], "
                            "md5[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X], "
                            "key[%08X], dwn[%08X], boot[%08X]",
                            frame->arg.fota.retry, frame->arg.fota.timeout, frame->arg.fota.protocol,
                            frame->arg.fota.url, frame->arg.fota.port,
                            frame->arg.fota.usrName, frame->arg.fota.usrPass,
                            frame->arg.fota.md5[0],  frame->arg.fota.md5[1],  frame->arg.fota.md5[2],  frame->arg.fota.md5[3],
                            frame->arg.fota.md5[4],  frame->arg.fota.md5[5],  frame->arg.fota.md5[6],  frame->arg.fota.md5[7],
                            frame->arg.fota.md5[8],  frame->arg.fota.md5[9],  frame->arg.fota.md5[10],  frame->arg.fota.md5[11],
                            frame->arg.fota.md5[12],  frame->arg.fota.md5[13],  frame->arg.fota.md5[14],  frame->arg.fota.md5[15],
                            frame->arg.fota.key, frame->arg.fota.downAddr, frame->arg.fota.bootAddr);

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_cmd_content
* 
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : protocol command content parse
******************************************************************************/
static bool pb_prot_parse_cmd_content(PB_PROT_PARSED_CONTENT_TYPE *content,
                                      PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Parse arg[%02X]", content->msgSubType);

    switch (content->msgSubType)
    {
        case PB_PROT_CMD_APC:
        {
            if (!pb_prot_parse_cmd_apc(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_SER:
        {
            if (!pb_prot_parse_cmd_ser(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            if (!pb_prot_parse_cmd_cfg(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            if (!pb_prot_parse_cmd_tma(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            if (!pb_prot_parse_cmd_dog(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            if (!pb_prot_parse_cmd_aco(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_SEC:
        {
            if (!pb_prot_parse_cmd_sec(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            if (!pb_prot_parse_cmd_omc(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_ACW:
        {
            if (!pb_prot_parse_cmd_acw(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_DOA:
        {
            if (!pb_prot_parse_cmd_doa(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            if (!pb_prot_parse_cmd_sma(content, frame))
            {
                return false;
            }
            break;
        }
        /*action operate command*/
        case PB_PROT_CMD_OUO:
        {
            if (!pb_prot_parse_cmd_ouo(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_OUT:
        {
            if (!pb_prot_parse_cmd_out(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            if (!pb_prot_parse_cmd_muo(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_RTO:
        {
            if (!pb_prot_parse_cmd_rto(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_CMD_FOTA:
        {
            if (!pb_prot_parse_cmd_fota(content, frame))
            {
                return false;
            }
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Bad sub-msg type[%02X]", content->msgSubType);
            return false;
        }
    }

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_ack_content
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_prot_parse_ack_content(PB_PROT_PARSED_CONTENT_TYPE *content,
                                      PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    if (content->msgSubType == PB_PROT_ACK_GEN)
    {
        uint8 *pdata = content->content;

        if (content->contentLen < PB_ACK_GEN_CONTENT_LEN)
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Length error %d", content->contentLen);
            return false;
        }
        //Serial number
        pdata += pb_prot_parse_u16(pdata, &(frame->arg.genAck.serialNumber));

        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_parse_data_content
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : protocol command content parse
******************************************************************************/
static bool pb_prot_parse_data_content(PB_PROT_PARSED_CONTENT_TYPE *content,
                                      PB_PROT_CMD_PARSED_FRAME_TYPE *frame)
{
    OS_DBG_TRACE(DBG_MOD_PBPROT, DBG_INFO, "Parse arg[%02X]", content->msgSubType);

    switch (content->msgType)
    {
        case PB_PROT_CMD:
        {
            if (!pb_prot_parse_cmd_content(content, frame))
            {
                return false;
            }
            break;
        }
        case PB_PROT_ACK:
        {
            if (!pb_prot_parse_ack_content(content, frame))
            {
                return false;
            }
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBPROT, "Bad msg type[%02X]", content->msgType);
            return false;
        }
    }

    frame->msgType = (PB_PROT_TYPE)content->msgType;
    frame->msgSubType = (PB_PROT_CMD_TYPE)content->msgSubType;
    frame->serialNumber = content->serialNumber;

    return true;
}

/******************************************************************************
* Function    : pb_prot_parse_raw_data
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
bool pb_prot_parse_raw_data(PB_PROT_RAW_PACKET_TYPE *rawPack,
                            PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame)
{
    PB_PROT_PARSED_CONTENT_TYPE parsedContent;

    //new input data or remaining data
    if (rawPack->rawData != NULL)
    {
        pb_prot_rawbuff_enque(rawPack->srcID, rawPack->rawData, rawPack->rawLength);
    }

    if (!pb_prot_parse_rawbuff(rawPack->srcID, &parsedContent))
    {
        return false;
    }

    if (!pb_prot_parse_data_content(&parsedContent, parsedFrame))
    {
        OS_DBG_ERR(DBG_MOD_PBPROT, "CMD content err");
        return false;
    }

    parsedFrame->srcID = rawPack->srcID;

    return true;
}

/******************************************************************************
* Function    : pb_prot_rawbuff_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : init the protocol input buff context
******************************************************************************/
void pb_prot_parse_init(void)
{
    memset(cipherBuff, 0, sizeof(cipherBuff));
    memset(plainTextBuff, 0, sizeof(plainTextBuff));

    memset(&pbProtRawContext, 0, sizeof(pbProtRawContext));

    for (uint8 idx = PB_PORT_SRC_BEGIN; idx < PB_PORT_SRC_NUM; ++idx)
    {
        memset(pb_prot_raw_buff[idx].pBuff, 0, pb_prot_raw_buff[idx].maxSize);
        pbProtRawContext.rawQueue[idx].pbProtRawMaxSize = pb_prot_raw_buff[idx].maxSize;
        pbProtRawContext.rawQueue[idx].pbProtRaw = pb_prot_raw_buff[idx].pBuff;
    }
}

/******************************************************************************
* Function    : pb_prot_parse_rawbuff_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
bool pb_prot_parse_rawbuff_check(uint8 id)
{
    switch (id)
    {
        case PB_PROT_SRC_UART:
        case PB_PORT_SRC_OTA:
        {
            return (bool)(pb_prot_rawbuff_value_size(id) >= PB_PROT_CMD_MIN_LENGTH);
        }
        case PB_PORT_SRC_BT:
        case PB_PORT_SRC_SMS:
        default:
        {
            break;
        }
    }

    return false;
}

