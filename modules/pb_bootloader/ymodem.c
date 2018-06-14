/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          ymodem.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   ymodem transmit
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "os_middleware.h"
#include "os_trace_log.h"
#include "pb_bl_config.h"
#include "ymodem.h"
#include "hal_wdg.h"

/******************************************************************************
* Macros
******************************************************************************/
#if defined(PB_BOOTLOADER_DBG)
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define YMODEM_INFO(...) ymodem_trace_info(__VA_ARGS__)
#else
#define YMODEM_INFO(...) 
#endif /*PB_BOOTLOADER_DBG*/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
#if defined(PB_BOOTLOADER_DBG)
static void ymodem_trace_info(const char *fmt, ...)
{
    char buff[OS_TRACE_LOG_SIZE + 1];
    uint32 len;

    va_list va;
    va_start(va, fmt);
    len = vsnprintf(buff, OS_TRACE_LOG_SIZE, fmt, va);
    va_end(va);
    buff[len] = '\0';

    PB_BL_DEBUG_COM.println(buff);
}
#endif /*PB_BOOTLOADER_DBG*/

static uint8 char2int(uint8 character)
{
    uint8 i = 0;

    if (character >= '0' && character <= '9')
    {
        i = character - '0';
    }

    return i;
}

static uint32 dec2int(uint8 *string, uint8 len)
{
    uint8 i = 0;
    uint32 decSum = 0;

    for (i = 0; i < len; ++i)
    {
        decSum = decSum * 10 + char2int(string[i]);
    }

    return decSum;
}

/******************************************************************************
* Function    : calCRC16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 calCRC16(uint16 crc_in, uint8 byte)
{
    uint32 crc = crc_in;
    uint32 in = byte | 0x100;

    do
    {
        crc <<= 1;
        in <<= 1;
        if (in & 0x100)
        {
            ++crc;
        }
        if (crc & 0x10000)
        {
            crc ^= 0x1021;
        }
    }
    while (!(in & 0x10000));

    return crc & 0xffffu;
}

/******************************************************************************
* Function    : getCRC16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 getCRC16(const uint8* p_data, uint32 size)
{
    uint32 crc = 0;
    const uint8* dataEnd = p_data + size;

    while(p_data < dataEnd)
        crc = calCRC16(crc, *p_data++);

    crc = calCRC16(crc, 0);
    crc = calCRC16(crc, 0);

    return crc & 0xffffu;
}

/******************************************************************************
* Function    : terminate
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void terminate(void)
{
    PB_UPGRADE_COM.write(CA);
    PB_UPGRADE_COM.write(CA);
}

/******************************************************************************
* Function    : uartTimeout
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool uartTimeout(uint32 start, uint32 timeout)
{
    if (os_get_tick_count() - start >= timeout)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* Function    : uartRecv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 uartRecv(uint8 *pData, uint16 needLen)
{
    uint32 sendTime = os_get_tick_count();
    uint16 offset = 0;

    while (!PB_UPGRADE_COM.available())
    {
        os_scheduler_delay(DELAY_50_MS);
        if (uartTimeout(sendTime, DOWNLOAD_TIMEOUT))
        {
            YMODEM_INFO("recv timeout");
            offset = 0;
            goto err;
        }
    }

wait:
    while (PB_UPGRADE_COM.available() > 0 && (offset + 1) <= needLen)
    {
        pData[offset++] = PB_UPGRADE_COM.read();
    }

    if (offset < needLen)//filter
    {
        goto wait;
    }

err:
    return offset;
}

/******************************************************************************
* Function    : uartSend
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void uartSend(uint8 byte)
{
    PB_UPGRADE_COM.write(byte);
}

/******************************************************************************
* Function    : ymodem_packet_type
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 ymodem_packet_type(void)
{
    uint8 ret = YMODEM_RECV_NONE;
    uint8 readByte = 0;
    
    if (0 == uartRecv(&readByte, 1))
    {
        return YMODEM_RECV_NONE;
    }

    switch (readByte)
    {
        case SOH:
        case STX:
        {
            ret = readByte;
            break;
        }
        case EOT:
        {
            ret = YMODEM_RECV_FINISH;
            break;
        }
        case CA:
        {
            if (0 != uartRecv(&readByte, 1) && (CA == readByte))
            {
                ret = YMODEM_RECV_TRANS_ABORT;
            }
            else
            {
                ret = YMODEM_RECV_ERR;
            }
            break;
        }
        case ABORT1:
        case ABORT2:
        {
            ret = YMODEM_RECV_USER_ABORT;
            break;
        }
        default:
        {
            ret = YMODEM_RECV_ERR;
            break;
        }
    }
    
    return ret;
}

/******************************************************************************
* Function    : ymodem_packet_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 ymodem_packet_data(const uint8 type, uint8 *pdata, uint16 *size)
{
    uint16 needSize = 0;
    uint16 readSize = 0;
    uint16 crc = 0;

    switch (type)
    {
        case SOH:
        {
            *size = PACKET_SIZE;
            break;
        }
        case STX:
        default:
        {
            *size = PACKET_1K_SIZE;
            break;
        }
    }

    *pdata = type;
    
    needSize = *size + PACKET_OVERHEAD_SIZE;

    YMODEM_INFO("need recv %d", needSize);
    readSize = uartRecv(&pdata[PACKET_NUMBER_INDEX], needSize);
    
    if (readSize != needSize)
    {
        YMODEM_INFO("need %d, real %d", needSize, readSize);
        readSize = 0;
        goto err;
    }

    if (pdata[PACKET_NUMBER_INDEX] != ((pdata[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE))
    {
        YMODEM_INFO("idx %d, %d", pdata[PACKET_NUMBER_INDEX], ((pdata[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE));
        readSize = 0;
        goto err;
    }

    crc = pdata[*size + PACKET_DATA_INDEX] << 8;
    crc += pdata[*size + PACKET_DATA_INDEX + 1];
    if (getCRC16(&pdata[PACKET_DATA_INDEX], *size) != crc)
    {
        YMODEM_INFO("crc need %02X, %02X", crc, getCRC16(&pdata[PACKET_DATA_INDEX], *size));
        readSize = 0;
        goto err;
    }

err:
    return readSize;
}

/******************************************************************************
* Function    : ymodem_get_file_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool ymodem_get_file_info(uint8 *pdata, uint32 *fSize)
{
    uint8 szSize[FILE_SIZE_LENGTH+1];
    uint8 offset = 0;
    
    pdata += PACKET_DATA_INDEX;
    while ((*pdata) != ' ' && (*pdata) != '\0')
    {
        pdata++;
    }

    //skip ' '
    pdata++;
    while ((*pdata) != '\0' && offset < FILE_SIZE_LENGTH)
    {
        szSize[offset++] = *pdata++;
    }
    szSize[offset] = '\0';

    *fSize = dec2int(szSize, offset);
    YMODEM_INFO("FSIZE %d", *fSize);
    
    return true;
}

/******************************************************************************
* Function    : ymodem_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
YMODEM_STATUS ymodem_process(void)
{
    YMODEM_STATUS ret = YMODEM_INIT;
    
    bool bTransmitting = true;
    bool fileTransmitting = false;
    uint8 fileReqCnt = 0;
    uint8 errCnt = 0;

    uint8 pakcetType = 0;
    uint8 packetIdx = 0;
    uint8 packetData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];
    uint16 packetSize = 0;

    uint32 romAddr = 0;
    uint32 ramSrc = 0;

    YMODEM_INFO("YMODEM Start");
    //send begin
    uartSend(CRC16);

    while (bTransmitting)
    {
        //feed watchdog
        hal_wdg_feed();
        
        pakcetType = ymodem_packet_type();
        YMODEM_INFO("TYPE %d", pakcetType);
        switch (pakcetType)
        {
            //Data packet
            case SOH:
            case STX:
            {
                //get data in packet
                if (0 == ymodem_packet_data(pakcetType, packetData, &packetSize))
                {
                    YMODEM_INFO("Recv none");
                    continue;
                }
                //check packet index
                if (packetIdx != packetData[PACKET_NUMBER_INDEX])
                {
                    YMODEM_INFO("Idx err %d, %d", packetIdx, packetData[PACKET_NUMBER_INDEX]);
                    uartSend(NAK);
                    break;
                }

                //first packet
                if (!fileTransmitting)
                {
                    //first packet is empty, end transmition
                    if (packetData[PACKET_DATA_INDEX] == 0)
                    {
                        uartSend(ACK);
                        ret = YMODEM_NONE;
                        bTransmitting = false;
                    }
                    else
                    {
                        uint32 fSize = 0;
                        ymodem_get_file_info(packetData, &fSize);

                        if (fSize > (APP_END - APP_BEGIN))
                        {
                            terminate();
                            ret = YMODEM_LIMIT;
                            bTransmitting = false;
                        }
                        else
                        {
                            fileTransmitting = true;
                            romAddr = APP_BEGIN;

                            uartSend(ACK);
                            if (!PB_FLASH_HDLR.erasePages(APP_BEGIN, APP_END - APP_BEGIN))
                            {
                                YMODEM_INFO("erase err");
                            }
                            else
                            {
                                uartSend(CRC16);
                            }
                        }
                    }
                }
                else
                {
                    ramSrc = (uint32)&packetData[PACKET_DATA_INDEX];
                    YMODEM_INFO("write %X to %x", ramSrc, romAddr);
                    if (PB_FLASH_HDLR.forceWrite(romAddr, (uint32*)ramSrc, packetSize) == 0)
                    {
                        romAddr += packetSize;
                        uartSend(ACK);
                        YMODEM_INFO("romAddr %X, %d", romAddr, packetSize);
                    }
                    else
                    {
                        YMODEM_INFO("write %X to %x error", ramSrc, romAddr);
                        terminate();
                        ret = YMODEM_FLASHERR;
                        bTransmitting = false;
                    }
                }
                packetIdx++;
                
                break;
            }
            case YMODEM_RECV_FINISH:
            {
                uartSend(ACK);
                ret = YMODEM_OK;
                bTransmitting = false;
                //submitImage
                break;
            }
            case YMODEM_RECV_TRANS_ABORT:
            {
                uartSend(ACK);
                ret = YMODEM_ABORT;
                bTransmitting = false;
                break;
            }
            case YMODEM_RECV_USER_ABORT:
            {
                terminate();
                ret = YMODEM_ABORT;
                bTransmitting = false;
                break;
            }
            case YMODEM_RECV_NONE:
            case YMODEM_RECV_ERR:
            default:
            {
                if (fileTransmitting)
                {
                    if (++errCnt >= YMODEM_ERR_MAX)
                    {
                        terminate();
                        ret = YMODEM_ERROR;
                        bTransmitting = false;
                    }
                }
                else
                {
                    if (++fileReqCnt >= YMODEM_FILEREQ_MAX)
                    {
                        ret = YMODEM_TIMEOUT;
                        bTransmitting = false;
                    }
                    else
                    {
                        //send begin
                        uartSend(CRC16);
                    }
                }
                break;
            }
        }
    }

    return ret;
}

