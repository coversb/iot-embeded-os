/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          m26.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   quectel m26 operator
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-20      1.00                    Chen Hao
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
#include "m26.h"

/******************************************************************************
* Macros
******************************************************************************/
#define M26_RSP_MAX_LEN 128
#define M26_AT_CMD_MAX_LEN 128
#define M26_FTP_MAX_SIZE 512000  //500KB

#define M26_ATCMD_TIMEOUT (3*DELAY_1_S)
#define M26_RSP_MIN_LEN 4

#define M26_CONF_TIMEOUT (5*DELAY_1_S)   //5 seconds
#define M26_SIM_BUSY_TIMEOUT (10*DELAY_1_S)   // 10 seconds
#define M26_GSM_ATTACH_TIMEOUT (30*DELAY_1_S)   // 30 seconds
#define M26_GPRS_ATTACH_TIMEOUT (30*DELAY_1_S)   // 30 seconds
#define M26_SOCK_OPEN_TIMEOUT (10*DELAY_1_S) // 10 seconds
#define M26_RECV_TIMEOUT (DELAY_1_S)    // 1 second
#define M26_SEND_WAIT_RDY_TIMEOUT (DELAY_1_S) // 1 second
#define M26_SEND_TIMEOUT (DELAY_1_S*3)    // 3 second
#define M26_SEND_ACK_TIMEOUT (DELAY_1_S*10)    // 10 second

#define M26_GSM_INFO_DEFAULT "UNKNOWN"

//config at command
#define M26_AT_CMD_SIM_STAT "AT+CPIN?"
#define M26_AT_CMD_CREG "AT+CREG?"
#define M26_AT_CMD_APN "AT+QIREGAPP="
#define M26_AT_CMD_GET_APN "AT+QIREGAPP?"
#define M26_AT_CMD_GPRS_ATT "AT+QIFGCNT=0"
#define M26_AT_CMD_GPRS_STAT "AT+CGATT?"
#define M26_AT_CMD_SOCK_STAT "AT+QISTATE"
#define M26_AT_CMD_SOCK_OPEN "AT+QIOPEN=\"TCP\","
#define M26_AT_CMD_RECV "AT+QIRD=0,1,0,542" //stay same with PB_OTA_RECV_BUFF_SIZE
#define M26_AT_CMD_CELL_LOC "AT+QCELLLOC=1"

//respond
#define M26_RSP_OK "OK"
#define M26_RSP_ERROR "ERROR"
#define M26_RSP_CREG_OK "+CREG: 0,1"
#define M26_RSP_CREG_ROAM "+CREG: 0,5"
#define M26_RSP_CSQ "+CSQ: "
#define M26_RSP_SIM_NOTINSERT "+CME ERROR: 10"
#define M26_RSP_SIM_BUSY "+CME ERROR: 14"
#define M26_RSP_SIM_RDY "+CPIN: READY"
#define M26_RSP_GPRS_ATTACH "+CGATT: 1"
#define M26_RSP_SOCK_OPENED "STATE: CONNECT OK"
#define M26_RSP_SOCK_CLOSE "STATE: IP CLOSE"
#define M26_RSP_RDYSEND ">"
#define M26_RSP_RDYRECV "+QIRD:"
#define M26_RSP_CELL_LOC "+QCELLLOC: "
#define M26_RSP_MODULE "Revision: "

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static HAL_USART_TYPE *M26_COM = NULL;
static uint32 M26_COM_BAUDRATE = 115200;
static OS_MUTEX_TYPE M26_MUTEX = NULL;
static int32 hBinFile = -1;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : m26_check_timeout
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check is at command timeout
******************************************************************************/
static bool m26_check_timeout(uint32 start, uint32 timeout)
{
    if (os_get_tick_count() - start >= timeout)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* Function    : m26_at_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send at command and get respond
******************************************************************************/
static uint16 m26_at_cmd(const char *at, char *rsp, uint16 rspMaxLen)
{
    uint32 sendTime = 0;
    uint16 offset = 0;
    os_mutex_lock(&M26_MUTEX);

    if (at != NULL)
    {
        while (M26_COM->available())
        {
            M26_COM->read();
        }

        M26_COM->println((char*)at);
        os_scheduler_delay(DELAY_100_MS);
        sendTime = os_get_tick_count();
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "AT[%s]", at);
    }

wait:
    while (!M26_COM->available())
    {
        os_scheduler_delay(DELAY_50_MS);
        if (m26_check_timeout(sendTime, M26_ATCMD_TIMEOUT))
        {
            offset = 0;
            goto err;
        }
    }

    while (M26_COM->available() > 0 && (offset + 1) < rspMaxLen)
    {
        rsp[offset++] = M26_COM->read();
    }

    if (offset < M26_RSP_MIN_LEN)//filter
    {
        offset = 0;
        goto wait;
    }

err:
    os_mutex_unlock(&M26_MUTEX);

    rsp[offset] = '\0';
    return offset;
}

/******************************************************************************
* Function    : m26_at_cmd_without_lock
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send at command and get respond
******************************************************************************/
static uint16 m26_at_cmd_without_lock(const char *at, char *rsp, uint16 rspMaxLen)
{
    uint32 sendTime = 0;
    uint16 offset = 0;
    uint8 readByte;

    if (at != NULL)
    {
        while (M26_COM->available())
        {
            M26_COM->read();
        }

        M26_COM->println((char*)at);
        os_scheduler_delay(DELAY_100_MS);
        sendTime = os_get_tick_count();
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "AT[%s]", at);
    }

wait:
    while (!M26_COM->available())
    {
        os_scheduler_delay(DELAY_50_MS);
        if (m26_check_timeout(sendTime, M26_ATCMD_TIMEOUT))
        {
            offset = 0;
            goto err;
        }
    }

    while (M26_COM->available() > 0 && (offset + 1) < rspMaxLen)
    {
        readByte = M26_COM->read();
        if (readByte == '\n')
        {
            rsp[offset++] = '\0';
            break;
        }
        
        rsp[offset++] = readByte;
    }

    if (strlen((char*)rsp) < 4)//filter: \r\n OK 
    {
        offset = 0;
        memset(rsp, 0, rspMaxLen);
        goto wait;
    }

err:
    return offset;
}

/******************************************************************************
* Function    : m26_at_cmd_check_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send at command and check respond
******************************************************************************/
static bool m26_at_cmd_check_rsp(const char *at, const char *rsp)
{
    bool ret = false;
    char buff[M26_RSP_MAX_LEN + 1] = {0};

    if (0 != m26_at_cmd(at, buff, M26_RSP_MAX_LEN))
    {
        if (NULL != strstr((char*)buff, rsp))
        {
            ret = true;
        }
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "RSP[%s]", buff);

    return ret;    
}

/******************************************************************************
* Function    : m26_at_cmd_wait_rsp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_at_cmd_wait_rsp(const char *at, const char *rsp, uint32 timeout)
{
    uint32 startTime = os_get_tick_count();

    while (false == m26_at_cmd_check_rsp(at, rsp))
    {
        if (m26_check_timeout(startTime, timeout))
        {
            return false;
        }
        os_scheduler_delay(DELAY_500_MS);
    }

    return true;
}

/******************************************************************************
* Function    : m26_get_rsp_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static char* m26_get_rsp_data(char *pdata, uint16 maxLen)
{
    uint16 begin = 0;
    uint16 offset = 0;

    //filter '\r' '\n' in the begin
    while (pdata[offset] == '\r' || pdata[offset] == '\n')
    {
        if (offset < maxLen)
        {
            offset++;
        }
        else
        {
            return NULL;
        }
    }
    begin = offset;

    while (pdata[offset] != '\r' && pdata[offset] != '\n')
    {
        if (offset < maxLen)
        {
            offset++;
        }
        else
        {
            return NULL;
        }
    }
    pdata[offset] = '\0';

    return &pdata[begin];
}

/******************************************************************************
* Function    : m26_get_ver
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_get_ver(char *pdata, uint16 len)
{
    char buff[32+1];
    uint16 dataLen = 0;
    char *pVer = NULL;

    dataLen = m26_at_cmd("AT+GMR", buff, 32);
    
    if (NULL == (pVer = strstr(buff, M26_RSP_MODULE)))
    {
        return false;
    }
    pVer = m26_get_rsp_data(buff, dataLen);
    pVer += strlen(M26_RSP_MODULE);
    dataLen = strlen(pVer);
    
    if (dataLen > 0 && dataLen <= len)
    {
        strncpy(pdata, pVer, dataLen);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
* Function    : pb_dev_m26_get_module_imei
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_get_imei(char *pdata, uint16 len)
{
    char buff[32+1];
    uint16 dataLen = 0;
    char* pImei = NULL;

    m26_at_cmd("AT+GSN", buff, 32);

    dataLen = strlen(buff);
    pImei = m26_get_rsp_data(buff, dataLen);
    dataLen = strlen(pImei);
    
    if (dataLen > 0 && dataLen <= len)
    {
        strncpy(pdata, pImei, dataLen);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
* Function    : m26_get_imsi
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_get_imsi(char *pdata, uint16 len)
{
    char buff[32+1];
    uint16 dataLen = 0;
    char* pImsi = NULL;

    m26_at_cmd("AT+CIMI", buff, 32);

    dataLen = strlen(buff);
    pImsi = m26_get_rsp_data(buff, dataLen);
    dataLen = strlen(pImsi);
    
    if (dataLen > 0 && dataLen <= len)
    {
        strncpy(pdata, pImsi, dataLen);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
* Function    : m26_get_iccid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_get_iccid(char *pdata, uint16 len)
{
    char buff[32+1];
    uint16 dataLen = 0;
    char* pIccid = NULL;

    m26_at_cmd("AT+QCCID", buff, 32);

    dataLen = strlen(buff);
    pIccid = m26_get_rsp_data(buff, dataLen);
    dataLen = strlen(pIccid);
    
    if (dataLen > 0 && dataLen <= len)
    {
        strncpy(pdata, pIccid, dataLen);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************
* Function    : pb_dev_m26_gsm_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void m26_gsm_info(char *ver, const uint8 verLen, char *imei, const uint8 imeiLen, char *imsi, const uint8 imsiLen, char *iccid, const uint8 iccidLen)
{
    char strTmp[32 + 1];

    //get GSM module version
    memset(strTmp, 0, sizeof(strTmp));
    if (!m26_get_ver(strTmp, verLen))
    {
        memcpy(ver, M26_GSM_INFO_DEFAULT, strlen(M26_GSM_INFO_DEFAULT));
    }
    else
    {
        memcpy(ver, strTmp, strlen(strTmp));
    }

    //get GSM module IMEI
    memset(strTmp, 0, sizeof(strTmp));
    if (!m26_get_imei(strTmp, imeiLen))
    {
        memcpy(imei, M26_GSM_INFO_DEFAULT, strlen(M26_GSM_INFO_DEFAULT));
    }
    else
    {
        memcpy(imei, strTmp, strlen(strTmp));
    }

    //get SIM IMSI
    memset(strTmp, 0, sizeof(strTmp));
    if (!m26_get_imsi(strTmp, imsiLen))
    {
        memcpy(imsi, M26_GSM_INFO_DEFAULT, strlen(M26_GSM_INFO_DEFAULT));
    }
    else
    {
        memcpy(imsi, strTmp, strlen(strTmp));
    }

    //get SIM ICCID
    memset(strTmp, 0, sizeof(strTmp));
    if (!m26_get_iccid(strTmp, iccidLen))
    {
        memcpy(iccid, M26_GSM_INFO_DEFAULT, strlen(M26_GSM_INFO_DEFAULT));
    }
    else
    {
        memcpy(iccid, strTmp, strlen(strTmp));
    }
}

/******************************************************************************
* Function    : m26_get_csq
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : update GPRS csq val
******************************************************************************/
static void m26_get_csq(uint8 *rssi, uint8 *ber)
{
    char rsp[M26_RSP_MAX_LEN+1];
    char *pBer = NULL;
    char *pRssi = NULL;
    uint8 len = 0;

    memset(rsp, 0, sizeof(rsp));
    m26_at_cmd("AT+CSQ", rsp, M26_RSP_MAX_LEN);

    if (NULL != (pBer = (strrchr(rsp, ','))))//find last ','
    {
        *ber = atoi(pBer+1);        

        if (NULL != (pRssi = (strstr(rsp, M26_RSP_CSQ))))
        {
            pRssi += strlen(M26_RSP_CSQ);
            len = pBer - pRssi;

            char tmp[3];
            if (len <= 2)
            {
                memcpy(tmp, pRssi, len);
                *rssi = atoi(tmp);
            }
            else
            {
                *rssi =0xFF;
            }
        }
        else
        {
            *rssi = 0xFF;
        }
    }
    else
    {
        *ber = 0xFF;
    }
}

/******************************************************************************
* Function    : m26_net_check_stat
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_net_check_stat(void)
{
    //check sim card state first 
    if (m26_at_cmd_check_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_NOTINSERT))
    {
        OS_INFO("SIM CARD NOT INSERTED");
        return false;
    }

    char buff[M26_RSP_MAX_LEN+1] = {0};

    #if 0
    //discard cache data
    while (m26_com.available())
    {
        m26_com.read();
    }
    #endif

    m26_at_cmd(M26_AT_CMD_SOCK_STAT, buff, M26_RSP_MAX_LEN);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Netstat[%s]", buff);

    if (NULL != strstr(buff, M26_RSP_SOCK_OPENED))
    {
        return true;
    }
    else
    {
        OS_DBG_ERR(DBG_MOD_DEV, "Net err %s", buff);
        return false;
    }
}

/******************************************************************************
* Function    : m26_wait_send_rdy
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : wait '>' to check send rdy
******************************************************************************/
static bool m26_wait_send_rdy(uint32 sendTime)
{
    int8 nread = 0;
    uint8 total_read = 0;
    uint8 respond[M26_RSP_MAX_LEN + 1];

    do
    {
        if (m26_check_timeout(sendTime, M26_SEND_WAIT_RDY_TIMEOUT))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Send timeout");
            return false;
        }

        memset(respond, '\0', sizeof(respond));
        nread = M26_COM->readBytes(respond, M26_RSP_MAX_LEN);

        if (nread <= 0)
        {
            os_scheduler_delay(DELAY_1_MS*10);//wait for respond
            continue;
        }
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "%s", respond);

        total_read += nread;

        if (NULL != strstr((char*)respond, M26_RSP_RDYSEND))
        {
            return true;
        }
        if (NULL != strstr((char*)respond, M26_RSP_ERROR))
        {
            return false;
        }

    }while (total_read < 64);

    return false;
}

/******************************************************************************
* Function    : m26_check_send_ok
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_check_send_ok(uint32 sendTime)
{
    char buff[M26_RSP_MAX_LEN + 1] = {0};
    uint16 offset = 0;
    bool ret = false;

wait:
    while (!M26_COM->available())
    {
        os_scheduler_delay(DELAY_50_MS);
        if (m26_check_timeout(sendTime, M26_SEND_TIMEOUT))
        {
            offset = 0;
            goto err;
        }
    }

    while (M26_COM->available() > 0 && (offset + 1) < M26_RSP_MAX_LEN)
    {
        buff[offset++] = M26_COM->read();
    }

    if (offset < M26_RSP_MIN_LEN)//filter
    {
        offset = 0;
        goto wait;
    }
    if (NULL != strstr((char*)buff, "SEND OK"))
    {
        ret = true;
    }

err:
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "RSP[%s]", buff);
    return ret;
}

/******************************************************************************
* Function    : m26_wait_sack
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_wait_sack(uint32 sendTime)
{
    char buff[M26_RSP_MAX_LEN + 1] = {0};
    bool ret = false;

    do
    {
        if (m26_check_timeout(sendTime, M26_SEND_ACK_TIMEOUT))
        {
            goto err;
        }

        memset(buff, 0, sizeof(buff));
        m26_at_cmd_without_lock("AT+QISACK", buff, M26_RSP_MAX_LEN);
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "%s", buff);

        //+QISACK: 660, 622, 38
        // sent, acked, nAcked
        char *pNAck = NULL;
        uint32 nAck = 0;
        
        if (NULL != strstr(buff, "+QISACK: "))
        {
            if (NULL != (pNAck = (strrchr(buff, ','))))//find last ','
            {
                pNAck+= 2; //skip ' ,'
                nAck = atoi(pNAck);
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "nAck %d", nAck);

                if (nAck == 0)
                {
                    ret = true;
                    break;
                }
            }
        }
    }while (1);
    
err:
    return ret;
}

/******************************************************************************
* Function    : m26_net_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 m26_net_send(const uint8 *pdata, const uint16 size)
{
    uint32 sendTime = 0;
    uint16 realSend = 0;
    char sendCmd[32];
    snprintf(sendCmd, 32, "AT+QISEND=%d", size);

    os_mutex_lock(&M26_MUTEX);

    while (M26_COM->available())
    {
        M26_COM->read();
    }

    M26_COM->println(sendCmd);
    os_scheduler_delay(DELAY_50_MS);
    sendTime = os_get_tick_count();
    
    if (m26_wait_send_rdy(sendTime))
    {
        M26_COM->writeBytes((uint8*)pdata, size);

        if (m26_check_send_ok(sendTime))
        {
            sendTime = os_get_tick_count();
            if (m26_wait_sack(sendTime))
            {
                realSend = size;
            }
        }
    }
    os_mutex_unlock(&M26_MUTEX);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Send %d", realSend);
    return realSend;
}

/******************************************************************************
* Function    : m26_wait_recv_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get recv data length
******************************************************************************/
static uint16 m26_wait_recv_data(uint32 sendTime)
{
    uint8 total_read = 0;
    uint8 readByte;
    uint8 respond[M26_RSP_MAX_LEN + 1];
    char *pFind = NULL;
    
    memset(respond, 0, sizeof(respond));
    do
    {
        if (m26_check_timeout(sendTime, M26_RECV_TIMEOUT))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Recv timeout");
            return 0;
        }

        if (M26_COM->available() == 0)
        {
            os_scheduler_delay(DELAY_1_MS*10);//wait for respond
            continue;
        }

        readByte = M26_COM->read();
        if ((total_read < strlen(M26_RSP_RDYRECV))
            && (readByte == '\r' || readByte == '\n'))
        {
            continue;
        }
        
        respond[total_read++] = readByte;
        if (NULL != strstr ((char*)respond, "+QIRDI: "))
        {
            memset(respond, 0, sizeof(respond));
            total_read = 0;
            continue;
        }

        if ((NULL != strstr((char*)respond, M26_RSP_RDYRECV))
            && (NULL != (pFind = strstr((char*)respond, "TCP,")))
            && (NULL != strstr((char*)respond, "\n")))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "--------TCP\r\n%s--------\r\n", respond);
            uint8 findOffset = 0;
            char recvLen[4+1] = {0};

            pFind += 4;//skip "TCP,"
            while ((findOffset < 4) && (*pFind != '\n'))
            {
                recvLen[findOffset] = *pFind++;
                findOffset++;
            }
            recvLen[findOffset] = '\0';

            return atoi(recvLen);
        }
        if (NULL != strstr((char*)respond, M26_RSP_ERROR))
        {
            return 0;
        }

    }while (total_read < 128);

    return 0;
}

/******************************************************************************
* Function    : m26_net_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv data
******************************************************************************/
static uint16 m26_net_recv(uint8 *pdata, const uint16 max)
{
    uint32 sendTime = 0;
    uint16 recvLen;
    uint16 readCnt = 0;

    os_mutex_lock(&M26_MUTEX);

    M26_COM->println(M26_AT_CMD_RECV);
    os_scheduler_delay(DELAY_50_MS);

    sendTime = os_get_tick_count();
    
    recvLen = m26_wait_recv_data(sendTime);
    recvLen = MIN_VALUE(recvLen, max);

    if (recvLen == 0)
    {
        while (M26_COM->available())
        {
            M26_COM->read();
        }
        goto err;
    }   

    while (M26_COM->available() != 0 && readCnt < recvLen)
    {
        pdata[readCnt] = M26_COM->read();
        readCnt++;
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Recv[%d]", recvLen);
err:
    os_mutex_unlock(&M26_MUTEX);

    return readCnt;
}

/******************************************************************************
* Function    : m26_net_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : close socket and de-attach pdp and gprs prepare to reset m26
******************************************************************************/
static bool m26_net_disconnect(void)
{
    //Close socket
    m26_at_cmd_check_rsp("AT+QICLOSE", "CLOSE OK");
    
    //pdp deattach
    m26_at_cmd_check_rsp("AT+CGACT=0,1", "NO CARRIER");

    //GPRS de-act
    m26_at_cmd_check_rsp("AT+CGATT=0", M26_RSP_OK);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net disconnected");

    return true;
}

/******************************************************************************
* Function    : m26_net_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : connet to server
******************************************************************************/
static bool m26_net_connect(const char *domain, const uint16 port)
{
    bool ret = false;
    char cmd[M26_AT_CMD_MAX_LEN+1] = {0};

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net connecting[%s:%d]", domain, port);

    snprintf(cmd, M26_AT_CMD_MAX_LEN, M26_AT_CMD_SOCK_OPEN"\"%s\",\"%d\"", domain, port);
    
    //open socket and connect to server
    if (!m26_at_cmd_wait_rsp(cmd, M26_RSP_OK, M26_SOCK_OPEN_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "Sock open err");
        goto err;
    }

    //check connect state
    if (!m26_at_cmd_wait_rsp(M26_AT_CMD_SOCK_STAT, M26_RSP_SOCK_OPENED, M26_SOCK_OPEN_TIMEOUT))
    {
        if (m26_at_cmd_check_rsp(M26_AT_CMD_SOCK_STAT, M26_RSP_SOCK_CLOSE))
        {
            OS_DBG_ERR(DBG_MOD_DEV, "Sock closed");
            goto err;
        }

        OS_DBG_ERR(DBG_MOD_DEV, "Sock not open");
        goto err;
    }    

    ret = true;
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net connected");
err:
    return ret;
}

/******************************************************************************
* Function    : m26_check_creg
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_check_creg(uint32 timeout)
{
    uint32 startTime = os_get_tick_count();
    char buff[M26_RSP_MAX_LEN+1] = {0};

    while (1)
    {
        if (m26_check_timeout(startTime, timeout))
        {
            return false;
        }
        os_scheduler_delay(DELAY_500_MS);

        if (0 != m26_at_cmd(M26_AT_CMD_CREG, buff, M26_RSP_MAX_LEN))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "RSP[%s]", buff);
            if (NULL != strstr(buff, M26_RSP_CREG_OK) 
                || NULL != strstr(buff, M26_RSP_CREG_ROAM))
            {
                return true;
            }
        }
    }
}

/******************************************************************************
* Function    : m26_set_apn
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_set_apn(const char *apn, const char *user, const char *pass)
{
    char cmd[M26_AT_CMD_MAX_LEN] = {0};

    snprintf(cmd, M26_AT_CMD_MAX_LEN, M26_AT_CMD_APN"\"%s\",\"%s\",\"%s\"", apn, user, pass);
    
    return m26_at_cmd_check_rsp(cmd, M26_RSP_OK);
}

/******************************************************************************
* Function    : m26_check_gprs
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_check_gprs(uint32 timeout)
{
    uint32 startTime = os_get_tick_count();

    while (false == m26_at_cmd_check_rsp(M26_AT_CMD_GPRS_STAT, M26_RSP_GPRS_ATTACH))
    {
        //debug info
        m26_at_cmd_check_rsp("AT+CPIN?", M26_RSP_OK);
        m26_at_cmd_check_rsp("AT+COPS?", M26_RSP_OK);
        m26_at_cmd_check_rsp("AT+CREG?", M26_RSP_OK);
        m26_at_cmd_check_rsp("AT+CGREG?", M26_RSP_OK);
        m26_at_cmd_check_rsp("AT+CSQ", M26_RSP_OK);
        m26_at_cmd_check_rsp("AT+QISTATE", M26_RSP_OK);
        
        if (m26_check_timeout(startTime, timeout))
        {
            return false;
        }
        os_scheduler_delay(DELAY_500_MS);
    }

    return true;
}

/******************************************************************************
* Function    : m26_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool m26_config(const char *apn, const char *user, const char *pass)
{
    bool ret = false;
/*
    m26_at_cmd_check_rsp("ATE0", M26_RSP_OK);
    m26_at_cmd_check_rsp("AT+CFUN=1", M26_RSP_OK);

    //SIM card stat check
    if (m26_at_cmd_check_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_NOTINSERT))
    {
        OS_INFO("SIM CARD NOT INSERTED");
        goto err;
    }
*/
    //SIM card stat check
    if (!m26_at_cmd_wait_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_RDY, M26_SIM_BUSY_TIMEOUT))
    {
        m26_at_cmd_wait_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_BUSY, M26_SIM_BUSY_TIMEOUT);

        if (!m26_at_cmd_check_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_RDY))
        {
            OS_DBG_ERR(DBG_MOD_DEV, "SIM BUSY");
            goto err;
        }
    }

    //check GSM network register
    if (!m26_check_creg(M26_GSM_ATTACH_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "GSM TIMEOUT");
        goto err;
    }

    //set APN when it's not null
    if (apn != NULL && strlen(apn) != 0)
    {
        if (!m26_set_apn(apn, user, pass))
        {
            OS_DBG_ERR(DBG_MOD_DEV, "APN Err");
            goto err;
        }
        m26_at_cmd_check_rsp(M26_AT_CMD_GET_APN, M26_RSP_OK);
    }

    //show ip addr
    //pb_dev_m26_at_cmd_check_resp("AT+QILOCIP=0", PB_DEV_AT_RSP_OK);

    //set active conetxt
    if (!m26_at_cmd_wait_rsp(M26_AT_CMD_GPRS_ATT, M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "GPRS ATTACH Err");
        goto err;
    }
    //GPRS attach, this AT command is not used
    //pb_dev_m26_at_cmd_check_resp("AT+CGATT=1", PB_DEV_AT_RSP_OK);

    //check gprs status
    if (!m26_check_gprs(M26_GPRS_ATTACH_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "GPRS status err");
        goto err;
    }

    //cache recv data
    if (!m26_at_cmd_wait_rsp("AT+QINDI=1", M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "QINDI err");
        goto err;
    }

    //SEND showback off
    if (!m26_at_cmd_wait_rsp("AT+QISDE=0", M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "QISDE err");
        goto err;
    }

    //tcp connect by domain name
    if (!m26_at_cmd_wait_rsp("AT+QIDNSIP=1", M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "QIDNSIP err");
        goto err;
    }

    ret = true;
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "M26 config OK");
err:

    return ret;
}

/******************************************************************************
* Function    : m26_sim_available
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check sim card is inserted
******************************************************************************/
static bool m26_sim_available(void)
{
    m26_at_cmd_check_rsp("ATE0", M26_RSP_OK);
    m26_at_cmd_check_rsp("AT+CFUN=1", M26_RSP_OK);

    //SIM card stat check
    if (m26_at_cmd_check_rsp(M26_AT_CMD_SIM_STAT, M26_RSP_SIM_NOTINSERT))
    {
        OS_INFO("SIM CARD NOT INSERTED");
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : m26_hw_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : m26 module re-open and usart re-init
******************************************************************************/
static bool m26_hw_reset(void)
{
    //m26 power off and on
    hal_gpio_set(BOARD_M26_PWR, HAL_GPIO_LOW);
    os_scheduler_delay(DELAY_1_S);
    hal_gpio_set(BOARD_M26_PWR, HAL_GPIO_HIGH);
    
    //m26 powerkey reset
    hal_gpio_set(BOARD_M26_PWRKEY, HAL_GPIO_HIGH);
    os_scheduler_delay(DELAY_100_MS);
    hal_gpio_set(BOARD_M26_PWRKEY, HAL_GPIO_LOW);
    os_scheduler_delay(DELAY_500_MS);
    hal_gpio_set(BOARD_M26_PWRKEY, HAL_GPIO_HIGH);

    M26_COM->end();
    M26_COM->begin(M26_COM_BAUDRATE);
    //wait com and m26 stable
    os_scheduler_delay(DELAY_1_S*5);
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "M26 RST OK");
    return true;
}

/******************************************************************************
* Function    : m26_hw_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set up com usart and baudrate, and gpio init
******************************************************************************/
static bool m26_hw_init(HAL_USART_TYPE *com, const uint32 baudrate)
{
    if (com == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "M26 com is invalid");
        return false;
    }
    M26_COM = com;
    M26_COM_BAUDRATE = baudrate;
    os_mutex_lock_init(&M26_MUTEX);

    hal_rcc_enable(BOARD_M26_IO_RCC);

    hal_gpio_set_mode(BOARD_M26_PWR, HAL_GPIO_OUT_PP);
    hal_gpio_set_mode(BOARD_M26_PWRKEY, HAL_GPIO_OUT_PP);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "M26 INIT OK, BAUD[%d]", baudrate);
    return true;
}

/*M26 FTP begin*/
/******************************************************************************
* Function    : m26_ftp_open_file
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : open downloaded firmware, get file handler
******************************************************************************/
static int32 m26_ftp_open_file(void)
{
    int32 hFile = -1;
    char rsp[M26_RSP_MAX_LEN+1];
    uint8 retryCnt = 0;

    do
    {
        retryCnt++;
        memset(rsp, 0, sizeof(rsp));

        //open bin file in m26 RAM
        m26_at_cmd("AT+QFOPEN=\"RAM:dwl\",2", rsp, M26_RSP_MAX_LEN);
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "QFOPEN[%s]", rsp);
        
        char *pFind = NULL;
        if (NULL != (pFind = strstr(rsp, "+QFOPEN:")))
        {
            pFind += strlen("+QFOPEN:");    //skip rsp string to get file hdlr
            hFile = atoi(pFind);
            break;
        }

    } while (retryCnt < 3);
    
    return hFile;
}

/******************************************************************************
* Function    : m26_ftp_close_file
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : close file by handler
******************************************************************************/
static bool m26_ftp_close_file(int32 hFile)
{
    bool ret = false;
    char cmd[M26_AT_CMD_MAX_LEN+1];

    //set ftp server path
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFCLOSE=%d", hFile);
    if (!m26_at_cmd_wait_rsp(cmd, M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "close file err");
        goto err;
    }
    ret = true;

err:
    hBinFile = -1;
    return ret;
}

/******************************************************************************
* Function    : m26_ftp_request_file_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send at command to request file data
******************************************************************************/
static uint32 m26_ftp_request_file_data(int32 hFile, const uint32 size)
{
    uint32 startTime = 0;
    uint32 totalRead = 0;
    char cmd[M26_AT_CMD_MAX_LEN+1];
    char rsp[M26_RSP_MAX_LEN + 1];
    uint8 readByte;
    char *pFind = NULL;

    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFREAD=%d, %d", hFile, size);
    M26_COM->println(cmd);
    os_scheduler_delay(DELAY_50_MS);

    startTime = os_get_tick_count();
    memset(rsp, 0, sizeof(rsp));
    do
    {
        if (m26_check_timeout(startTime, M26_CONF_TIMEOUT))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Wait QFREAD timeout");
            return 0;
        }

        if (M26_COM->available() == 0)
        {
            os_scheduler_delay(DELAY_1_MS*10);//wait for respond
            continue;
        }

        readByte = M26_COM->read();
        if ((totalRead < strlen("CONNECT "))
            && (readByte == '\r' || readByte == '\n'))
        {
            continue;
        }
        rsp[totalRead++] = readByte;

        if (NULL != (pFind = strstr((char*)rsp, "CONNECT "))
            && (NULL != strstr((char*)rsp, "\n")))
        {
            uint8 findOffset = 0;
            char recvLen[4+1] = {0};

            pFind += strlen("CONNECT ");//skip "CONNECT "
            while ((findOffset < 4) && (*pFind != '\n'))
            {
                recvLen[findOffset] = *pFind++;
                findOffset++;
            }
            recvLen[findOffset] = '\0';
            
            return atoi(recvLen);
        }
        if (NULL != strstr((char*)rsp, M26_RSP_ERROR))
        {
            return 0;
        }

    }while (totalRead < 128);

    return 0;
}

/******************************************************************************
* Function    : m26_ftp_get_file_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get firmware data from file
******************************************************************************/
static uint32 m26_ftp_get_file_data(uint8 *pdata, const uint32 maxSize)
{
    if (hBinFile == -1)
    {
        hBinFile = m26_ftp_open_file();
    }

    uint32 dataLen = 0;
    uint32 readLen = 0;
    uint32 startTime =0;
    
    dataLen = m26_ftp_request_file_data(hBinFile, maxSize);
    dataLen = MIN_VALUE(dataLen, maxSize);

    if (dataLen == 0)
    {
        while (M26_COM->available())
        {
            M26_COM->read();
        }
        OS_DBG_ERR(DBG_MOD_DEV, "QFREAD none");
        goto end;
    }

    startTime = os_get_tick_count();
    while (readLen < dataLen)
    {
        if (m26_check_timeout(startTime, DELAY_500_MS))
        {
            break;
        }
        if (M26_COM->available() == 0)
        {
            os_scheduler_delay(DELAY_1_MS*5);
            continue;
        }
        startTime = os_get_tick_count();
        pdata[readLen] = M26_COM->read();
        readLen++;
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "QFREAD[%d]", readLen);

end:
    if (0 == readLen)
    {
        m26_ftp_close_file(hBinFile);
    }
    
    return readLen;
}

/******************************************************************************
* Function    : m26_ftp_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : connect to ftp server
******************************************************************************/
static bool m26_ftp_connect(const char* server, const uint16 port, const char *user, const char *password)
{
    bool ret = false;
    char cmd[M26_AT_CMD_MAX_LEN+1];

    //set ftp user
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFTPUSER=\"%s\"", user);
     if (!m26_at_cmd_wait_rsp(cmd, M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set user[%s] err", user);
        goto err;
    }

    //set ftp password
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFTPPASS=\"%s\"", password);
     if (!m26_at_cmd_wait_rsp(cmd, M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set password[%s] err", password);
        goto err;
    }

    //open ftp server
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFTPOPEN=\"%s\",%d", server, port);
     if (!m26_at_cmd_wait_rsp(cmd, M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp open [%s:%d] err", server, port);
        goto err;
    }
     
    //check ftp server open state
    //if (!m26_at_cmd_wait_rsp(NULL, "+QFTPOPEN:0", M26_SOCK_OPEN_TIMEOUT))
    if (!m26_at_cmd_wait_rsp(NULL, ":0", M26_SOCK_OPEN_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "Sock open err");
        goto err;
    }
    ret = true;

err:
    return ret;
}

/******************************************************************************
* Function    : m26_ftp_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : close ftp
******************************************************************************/
static bool m26_ftp_disconnect(void)
{
    bool ret = false;
    //close ftp socket
    if (!m26_at_cmd_wait_rsp("AT+QFTPCLOSE", M26_RSP_OK, M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp close err");
        goto err;
    }

    //check ftp server open state
    //if (!m26_at_cmd_wait_rsp(NULL, "+QFTPCLOSE:0", M26_SOCK_OPEN_TIMEOUT))
    if (!m26_at_cmd_wait_rsp(NULL, ":0", M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp close err");
        goto err;
    }
    ret = true;

err:
    if (hBinFile != -1)
    {
        m26_ftp_close_file(hBinFile);
    }
    
    return ret;
}

/******************************************************************************
* Function    : m26_ftp_set_path
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set ftp server path
******************************************************************************/
static bool m26_ftp_set_path(const char *path)
{
    bool ret = false;
    char cmd[M26_AT_CMD_MAX_LEN+1];

    //set ftp server path
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFTPPATH=\"%s\"", path);
    if (!m26_at_cmd_wait_rsp(cmd, "+QFTPPATH:0", M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set path err");
        goto err;
    }
    ret = true;

err:
    return ret;
}

/******************************************************************************
* Function    : m26_ftp_set_mode
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set passive mode / binary transfer / local path
******************************************************************************/
static bool m26_ftp_set_mode(void)
{
    bool ret = false;

    //set ftp passive mode
    if (!m26_at_cmd_wait_rsp("AT+QFTPCFG=1,1", "+QFTPCFG:0", M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set pasv err");
        goto err;
    }

    //set ftp transfer type as binary
    if (!m26_at_cmd_wait_rsp("AT+QFTPCFG=2,0", "+QFTPCFG:0", M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set binary err");
        goto err;
    }

    //set ftp local path
    if (!m26_at_cmd_wait_rsp("AT+QFTPCFG=4,\"/RAM/dwl\"", "+QFTPCFG:0", M26_CONF_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "ftp set local path err");
        goto err;
    }
    ret = true;

err:
    return ret;
}

/******************************************************************************
* Function    : m26_ftp_get_file_size
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get file size when finish downloading
******************************************************************************/
static uint32 m26_ftp_get_file_size(const char *fname, const uint32 timeout)
{
    uint32 totalSize = 0;
    char cmd[M26_AT_CMD_MAX_LEN+1];
    char rsp[M26_RSP_MAX_LEN+1];
    char *pFind = NULL;
    uint32 startTime =0;

    //start to download file
    snprintf(cmd, M26_AT_CMD_MAX_LEN, "AT+QFTPGET=\"%s\",%d", fname, M26_FTP_MAX_SIZE);
    m26_at_cmd(cmd, rsp, M26_RSP_MAX_LEN);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "FTP GET[%s]", rsp);

    if (NULL != strstr(rsp, M26_RSP_ERROR))
    {
        goto err;
    }
    else
    if (NULL != strstr(rsp, "+QFTPGET:-"))
    {
        goto err;
    }
    #if 0
    else
    if (NULL != strstr(rsp, M26_RSP_OK))
    {
    }
    #endif

    //wait download finished
    memset(rsp, 0, sizeof(rsp));
    startTime = os_get_tick_count();
    do
    {
        m26_at_cmd(NULL, rsp, M26_RSP_MAX_LEN);
        if (NULL == (pFind = strstr(rsp, "+QFTPGET:")))
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "FTP download start %u, pass %u", 
                                    startTime / 1000, (os_get_tick_count() - startTime) / 1000);
            if (m26_check_timeout(startTime, timeout))
            {
                OS_DBG_ERR(DBG_MOD_DEV, "FTP download timeout");
                break;
            }
            os_scheduler_delay(DELAY_1_S);
        }
        else
        {
            int32 dwnSize = -1;
            pFind += strlen("+QFTPGET:");    //skip rsp string to get file hdlr
            dwnSize = atoi(pFind);
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "FTP download size[%d]", dwnSize);
            totalSize = MAX_VALUE(dwnSize, 0);
            break;
        }
    }while (1);

err:
    return totalSize;
}
/*M26 FTP end*/

const FTP_CLIENT ftpM26 = {
    m26_ftp_connect,
    m26_ftp_disconnect,
    m26_ftp_set_path,
    m26_ftp_set_mode,
    m26_ftp_get_file_size,
    m26_ftp_get_file_data
};

const DEV_TYPE_M26 devM26 = 
{
    m26_hw_init,
    m26_hw_reset,
    m26_sim_available,
    m26_config,
    m26_net_connect,
    m26_net_disconnect,
    m26_net_recv,
    m26_net_send,
    m26_net_check_stat,
    m26_get_csq,
    m26_gsm_info,
    NULL,
    &ftpM26
};

