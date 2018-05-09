/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          w5500.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   wiznet w5500 operation
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-2         1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "board_config.h"
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "w5500.h"
#include "dhcp.h"
#include "dns.h"
#include "pb_cfg_proc.h"
#include "pb_util.h"

/******************************************************************************
* Macros
******************************************************************************/
#define LOCAL_PORT_POOL_SIZE 8000
#define COM_LOCAL_PORT 10000
#define DNS_LOCAL_PORT 40000

#define DHCP_TIMEOUT (20*DELAY_1_S) // 20 seconds
#define DHCP_WAIT DELAY_100_MS

#define W5500_RSP_TIMEOUT (5*DELAY_1_S) // 5 seconds
#define W5500_WAIT DELAY_100_MS

#define SOCKET_DHCP SOCKET0
#define SOCKET_DNS SOCKET0
#define SOCKET_COM SOCKET1

#define SOCKET_REG(s) ((uint8)(s * 0x20 + 0x08))
#define SOCKET_RXREG(s) ((uint8)(s * 0x20 + 0x18))
#define SOCKET_TXREG(s) ((uint8)(s * 0x20 + 0x10))

//#define __ETH_DEBUG__
/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static HAL_SPI_TYPE *W5500_SPI = NULL;

//DNS server
static const uint8 DEF_DNS_SERVER[4] = {114, 114, 114, 114};

//dhcp client context
static DHCP_CLIENT w5500_dhcp_client;
static DNS_CLIENT w5500_dns_client;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : w5500_read_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read byte from reg
******************************************************************************/
static uint8 w5500_read_u8(uint16 regAddr)
{
    uint8 byte = 0;

    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM1 | RWB_READ | COMMON_R);
    byte = W5500_SPI->read_u8();

    W5500_SPI->select(false);

    return byte;
}

/******************************************************************************
* Function    : w5500_write_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write unsigned char to w5500 reg
******************************************************************************/
static void w5500_write_u8(uint16 regAddr, uint8 data)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM1 | RWB_WRITE | COMMON_R);
    W5500_SPI->write_u8(data);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_write_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write unsigned short to w5500 reg
******************************************************************************/
static void w5500_write_u16(uint16 regAddr, uint16 data)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM2 | RWB_WRITE | COMMON_R);
    W5500_SPI->write_u16(data);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_write_bytes
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write n bytes to w5500 reg
******************************************************************************/
static void w5500_write_bytes(uint16 regAddr, const uint8 *pdata, const uint16 size)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(VDM | RWB_WRITE | COMMON_R);
    W5500_SPI->write(pdata, size);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_read_sock_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read byte from socket reg
******************************************************************************/
static uint8 w5500_read_sock_u8(SOCKET s, uint16 regAddr)
{
    uint8 byte = 0;

    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM1 | RWB_READ | SOCKET_REG(s));
    byte = W5500_SPI->read_u8();

    W5500_SPI->select(false);

    return byte;
}

/******************************************************************************
* Function    : w5500_write_sock_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write byte to socket reg
******************************************************************************/
static void w5500_write_sock_u8(SOCKET s, uint16 regAddr, uint8 data)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM1 | RWB_WRITE | SOCKET_REG(s));
    W5500_SPI->write_u8(data);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_read_sock_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read u16 from socket reg
******************************************************************************/
static uint16 w5500_read_sock_u16(SOCKET s, uint16 regAddr)
{
    uint16 data;

    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM2 | RWB_READ | SOCKET_REG(s));
    data = W5500_SPI->read_u16();

    W5500_SPI->select(false);

    return data;
}

/******************************************************************************
* Function    : w5500_write_sock_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write u16 to socket reg
******************************************************************************/
static void w5500_write_sock_u16(SOCKET s, uint16 regAddr, uint16 data)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM2 | RWB_WRITE | SOCKET_REG(s));
    W5500_SPI->write_u16(data);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_write_sock_4bytes
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write 4 bytes to socket reg
******************************************************************************/
static void w5500_write_sock_4bytes(SOCKET s, uint16 regAddr, const uint8 *pdata)
{
    W5500_SPI->select(true);

    W5500_SPI->write_u16(regAddr);
    W5500_SPI->write_u8(FDM4 | RWB_WRITE | SOCKET_REG(s));
    W5500_SPI->write(pdata, 4);

    W5500_SPI->select(false);
}

/******************************************************************************
* Function    : w5500_check_timeout
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_check_timeout(uint32 start, uint32 timeout)
{
    if (os_get_tick_count() - start >= timeout)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* Function    : w5500_check_socket_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_check_socket_config(SOCKET s, uint32 startTime, uint32 timeout)
{
    // when socket configurion action is done, flag will be clear
    while (0 != w5500_read_sock_u8(s, Sn_CR))
    {
        os_scheduler_delay(W5500_WAIT);
        if (w5500_check_timeout(startTime, timeout))
        {            
            return false;
        }
    }

    return true;
}

/******************************************************************************
* Function    : pb_dev_w5500_read_sock_buffer
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : read datas from socket
******************************************************************************/
static uint16 w5500_read_sock_buffer(SOCKET s, uint8 *pbuf, const uint16 mxsize)
{
    uint16 rx_size = 0;
    uint16 available_rx_size = 0;
    uint16 offset = 0;
    uint16 record_offset = 0;
    uint16 idx = 0;
    uint8 read_byte = 0;
    unsigned int count = 0;
 
    available_rx_size = w5500_read_sock_u16(s, Sn_RX_RSR);
    if (available_rx_size == 0)
    {
        return 0;
    }

    rx_size = MIN_VALUE(available_rx_size, W5500_RX_BUFF_SIZE);
    rx_size = MIN_VALUE(rx_size, mxsize);

    offset = w5500_read_sock_u16(s, Sn_RX_RD);
    record_offset = offset;

    //calculate real phy address
    offset &= (W5500_RX_BUFF_SIZE - 1);

    W5500_SPI->select(true);

    W5500_SPI->write_u16(offset);//write phy address
    W5500_SPI->write_u8(VDM | RWB_READ | SOCKET_RXREG(s));

    if ((offset + rx_size) < W5500_RX_BUFF_SIZE) 
    {
        for (idx = 0; idx < rx_size; ++idx)
        {
            read_byte = W5500_SPI->read_u8();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
        }
    } 
    else 
    {
        offset = W5500_RX_BUFF_SIZE - offset;
        for (idx = 0; idx < offset; ++idx) 
        {
            read_byte = W5500_SPI->read_u8();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
        }
        W5500_SPI->select(false);

        W5500_SPI->select(true);
        W5500_SPI->write_u16(0x00);
        W5500_SPI->write_u8(VDM | RWB_READ | SOCKET_RXREG(s));
        for (; idx < rx_size; idx++) 
        {
            read_byte = W5500_SPI->read_u8();
            if (++count <= mxsize) 
            {
                *pbuf = read_byte;
                ++pbuf;
            }
         }
    }

    W5500_SPI->select(false);

    record_offset += rx_size; //更新实际物理地址,即下次读取接收到的数据的起始地址
    w5500_write_sock_u16(s, Sn_RX_RD, record_offset);
    w5500_write_sock_u8(s, Sn_CR, RECV); //发送启动接收命令

    return count < mxsize ? rx_size : mxsize;
}

/******************************************************************************
* Function    : w5500_write_sock_buffer
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write datas to socket
******************************************************************************/
static uint16 w5500_write_sock_buffer(SOCKET s, const uint8 *pbuf, const uint16 size)
{
    uint16 cal_offset = 0;
    uint16 record_offset = 0;
    uint16 idx = 0;
    uint16 realSend = 0;

    cal_offset = w5500_read_sock_u16(s, Sn_TX_WR);
    record_offset = cal_offset;

    cal_offset &= (W5500_TX_BUFF_SIZE - 1); //计算实际的物理地址

    W5500_SPI->select(true);

    W5500_SPI->write_u16(cal_offset);
    W5500_SPI->write_u8(VDM | RWB_WRITE | SOCKET_TXREG(s));

    //如果最大地址未超过W5500发送缓冲区寄存器的最大地址
    if ((cal_offset + size) < W5500_TX_BUFF_SIZE)
    {
        for (idx = 0; idx < size; idx++) 
        {
            W5500_SPI->write_u8(*pbuf);
            ++pbuf;
        }
    }
    else 
    {
        cal_offset = W5500_TX_BUFF_SIZE - cal_offset;
        for (idx = 0; idx < cal_offset; idx++) 
        {
            W5500_SPI->write_u8(*pbuf);
            ++pbuf;
        }
        W5500_SPI->select(false);

        W5500_SPI->select(true);
        W5500_SPI->write_u16(0x00);
        W5500_SPI->write_u8(VDM | RWB_WRITE | SOCKET_TXREG(s));

        for (; idx < size; ++idx) 
        {
            W5500_SPI->write_u8(*pbuf);
            ++pbuf;
        }
    }

    W5500_SPI->select(false);

    record_offset += size;
    //更新实际物理地址,即下次写待发送数据到发送数据缓冲区的起始地址
    w5500_write_sock_u16(s, Sn_TX_WR, record_offset);
    w5500_write_sock_u8(s, Sn_CR, SEND); //发送启动发送命令

    uint32 startTime = os_get_tick_count();
    if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "sock send err");
        return realSend;
    }

    //check send state
    uint8 sendStat;
    startTime = os_get_tick_count();
    while (1)
    {
        sendStat = w5500_read_sock_u8(s, Sn_IR);
        if (sendStat & IR_SEND_OK)
        {
            w5500_write_sock_u8(s, Sn_IR, IR_SEND_OK);
            realSend = size;
            break;
        }
        else
        if (sendStat & IR_TIMEOUT)
        {
            w5500_write_sock_u8(s, Sn_IR, IR_TIMEOUT);
            OS_DBG_ERR(DBG_MOD_DEV, "sock send timeout");
            break;
        }

        if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
        {
            OS_DBG_ERR(DBG_MOD_DEV, "sock send err");
            break;
        }
    }
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Write buff[%02X]", sendStat);

    return realSend;
}

/******************************************************************************
* Function    : w5500_random_port
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get a random number as port accroding to the range
******************************************************************************/
static uint16 w5500_random_port(uint16 min)
{
    uint16 port = (uint16)(pb_util_random_num(0xFFFF) % (LOCAL_PORT_POOL_SIZE + 1) + min);
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Get local port %d", port);
    return port;
}

/******************************************************************************
* Function    : w5500_cable_link
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check cable link check with 5s timeout
******************************************************************************/
static bool w5500_cable_link(void)
{
    if (0 == (w5500_read_u8(PHYCFGR) & LINK))
    {
        OS_INFO("NO ETH CABLE LINK");
        return false;
    }
    return true;
}

/******************************************************************************
* Function    : w5500_socket
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : create socket
******************************************************************************/
static bool w5500_socket(SOCKET s, uint8 protType, uint16 localPort)
{
    bool ret = true;
    uint32 startTime = 0;

    //w5500_write_sock_u16(s, Sn_MSSR, W5500_RX_BUFF_SIZE);
    w5500_write_sock_u16(s, Sn_PORT, localPort);
    w5500_write_sock_u8(s, Sn_MR, protType);
    w5500_write_sock_u8(s, Sn_CR, OPEN);

    startTime = os_get_tick_count();
    if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "socket[%02X] create timeout", protType);
    }

    return ret;
}

/******************************************************************************
* Function    : w5500_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket connect to server
******************************************************************************/
static bool w5500_connect(SOCKET s, const uint8 *ip, const uint16 port)
{
    bool ret = true;
    uint32 startTime = 0;

    w5500_write_sock_4bytes(s, Sn_DIPR, ip);    //set server IP
    w5500_write_sock_u16(s, Sn_DPORTR, port);   //set server port
    w5500_write_sock_u8(s, Sn_CR, CONNECT); // connect

    startTime = os_get_tick_count();
    if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "connect timeout");
        goto err;
    }

    startTime = os_get_tick_count();
    while(w5500_read_sock_u8(s, Sn_SR) != SOCK_ESTABLISHED)
    {
        uint8 stat = w5500_read_sock_u8(s, Sn_IR);
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Connect[%02X]", stat);
        if (stat & IR_TIMEOUT || stat & IR_DISCON)
        {
            w5500_write_sock_u8(s, Sn_IR, IR_TIMEOUT);
            w5500_write_sock_u8(s, Sn_IR, IR_DISCON);

            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "socket connect timeout");
            break;
        }

        os_scheduler_delay(W5500_WAIT);
        if (w5500_check_timeout(startTime, W5500_RSP_TIMEOUT))
        {
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "connect timeout");
            break;
        }
    }

err:
    return ret;
}

/******************************************************************************
* Function    : w5500_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket disconnect to server
******************************************************************************/
static bool w5500_disconnect(SOCKET s)
{
    bool ret = true;
    uint32 startTime = 0;

    w5500_write_sock_u8(s, Sn_CR, DISCON);

    startTime = os_get_tick_count();
    if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "disconnect timeout");
        goto err;
    }

    startTime = os_get_tick_count();
    while(w5500_read_sock_u8(s, Sn_SR) != SOCK_CLOSED)
    {
        if (w5500_read_sock_u8(s, Sn_IR) & IR_TIMEOUT)
        {
            w5500_write_sock_u8(s, Sn_IR, IR_TIMEOUT);
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "sock disconnect timeout");
            break;
        }

        os_scheduler_delay(W5500_WAIT);
        if (w5500_check_timeout(startTime, W5500_RSP_TIMEOUT))
        {
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "disconnect timeout");
            break;
        }        
    }

err:
    return ret;
}

/******************************************************************************
* Function    : w5500_close
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tcp socket close
******************************************************************************/
static bool w5500_close(SOCKET s)
{
    bool ret = true;
    uint32 startTime = 0;

    //close socket
    w5500_write_sock_u8(s, Sn_CR, CLOSE);

    startTime = os_get_tick_count();
    if (!w5500_check_socket_config(s, startTime, W5500_RSP_TIMEOUT))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "close timeout");
        goto err;
    }

    //clear all interrupt of the socket
    w5500_write_sock_u8(s, Sn_IR, 0xFF);

    startTime = 0;
    while(w5500_read_sock_u8(s, Sn_SR) != SOCK_CLOSED)
    {
        if (w5500_read_sock_u8(s, Sn_IR) & IR_TIMEOUT)
        {
            w5500_write_sock_u8(s, Sn_IR, IR_TIMEOUT);
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "sock close timeout");
            break;
        }

        os_scheduler_delay(W5500_WAIT);
        if (w5500_check_timeout(startTime, W5500_RSP_TIMEOUT))
        {
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "sock close timeout");
            break;
        }        
    }

err:
    return ret;
}

/******************************************************************************
* Function    : w5500_socket_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send datas to socket
******************************************************************************/
static uint16 w5500_socket_send(SOCKET s, const uint8 *pbuf, uint16 size)
{
    uint16 realSend = 0;
    
    if (w5500_read_sock_u8(s, Sn_SR)  != SOCK_ESTABLISHED)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "sock stat error");
        goto err;
    }

    if (0 == (realSend = w5500_write_sock_buffer(s, pbuf, size)))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "sock send err");
        goto err;
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "socket send[%d] ok", realSend);

err:
    return realSend;
}

/******************************************************************************
* Function    : w5500_socket_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv datas from socket
******************************************************************************/
static uint16 w5500_socket_recv(SOCKET s, uint8 *pbuf, uint16 maxLen)
{
    return w5500_read_sock_buffer(s, pbuf, maxLen);
}

/******************************************************************************
* Function    : w5500_socket_sendto
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 w5500_socket_sendto(SOCKET s, uint8 *pbuf, uint16 len, uint8 *addr, uint16 port)
{
    uint16 realSend = 0;

    if (addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "Sock sendto invalid addr");
        goto err;
    }

    //set server IP
    w5500_write_sock_4bytes(s, Sn_DIPR, addr);
    //set server port
    w5500_write_sock_u16(s, Sn_DPORTR, port);

    if (0 == (realSend = w5500_write_sock_buffer(s, pbuf, len)))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "Sock sendto err");
        goto err;
    }
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Sendto[%d.%d.%d.%d:%d][%d]", 
                            addr[0], addr[1], addr[2], addr[3], port, realSend);
err:
    return realSend;
}

/******************************************************************************
* Function    : w5500_socket_recvfrom
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 w5500_socket_recvfrom(SOCKET s, uint8 *pbuf, uint16 maxLen, uint8 *addr, uint16 *port)
{
    uint8 head[8];
    uint16 recvLen;
    uint16 realRead;

    realRead = w5500_read_sock_buffer(s, head, 8);

    if (realRead != 0)
    {
        addr[0] = head[0];
        addr[1] = head[1];
        addr[2] = head[2];
        addr[3] = head[3];
        *port = (head[4] << 8) + head[5];
        recvLen = (head[6] << 8) + head[7];

        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Recvfrom[%d][%d.%d.%d.%d:%d][%d]",
                                realRead, addr[0], addr[1], addr[2], addr[3], *port, recvLen);

        recvLen = MIN_VALUE(recvLen, maxLen);
        realRead = w5500_read_sock_buffer(s, pbuf, recvLen);
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Recvfrom[%d]", realRead);
    return realRead;
}

/******************************************************************************
* Function    : w5500_socket_set_buffsize
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : set buffer for comminute socket
******************************************************************************/
static void w5500_socket_set_buffsize(SOCKET s)
{
    //send/recv buff 4KB
    w5500_write_sock_u8(s, Sn_RXBUF_SIZE, W5500_RX_CFG);
    w5500_write_sock_u8(s, Sn_TXBUF_SIZE, W5500_TX_CFG);

    w5500_write_sock_u16(s, Sn_RX_RD, 0);
    w5500_write_sock_u16(s, Sn_TX_WR, 0);
}

/******************************************************************************
* Function    : w5500_net_default_loca_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
#if (1 == ETH_DEBUG )
static void w5500_net_default_loca_config(void)
{
    OS_DBG_ERR(DBG_MOD_DEV, "Set default IP");
    //IP addr
    uint8 DEF_IP_ADDR[4] = {192, 16, 8, 222};
    //Net mask
    uint8 DEF_NETMASK[4] = {255, 255, 255, 0};
    //Gateway
    uint8 DEF_GATEWAY[4] = {192, 16, 8, 254};

    //mac
    w5500_write_bytes(SHAR, pb_cfg_proc_get_mac(), PB_MAC_LEN);
    //gate way
    w5500_write_bytes(GAR, DEF_GATEWAY, sizeof(DEF_GATEWAY));
    //network mask
    w5500_write_bytes(SUBR, DEF_NETMASK, sizeof(DEF_NETMASK));
    //ip addr
    w5500_write_bytes(SIPR, DEF_IP_ADDR, sizeof(DEF_IP_ADDR));
}
#endif /*ETH_DEBUG*/

/******************************************************************************
* Function    : w5500_net_update_local_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : update ip addr by DHCP
******************************************************************************/
static void w5500_net_update_local_config(DHCP_CLIENT *dhcp)
{
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP IP update, IP[%d.%d.%d.%d], GW[%d.%d.%d.%d], MASK[%d.%d.%d.%d], DNS[%d.%d.%d.%d]",
                             dhcp->ip[0], dhcp->ip[1], dhcp->ip[2], dhcp->ip[3],
                             dhcp->gateway[0], dhcp->gateway[1], dhcp->gateway[2], dhcp->gateway[3], 
                             dhcp->mask[0], dhcp->mask[1], dhcp->mask[2], dhcp->mask[3],
                             dhcp->dns[0], dhcp->dns[1], dhcp->dns[2], dhcp->dns[3]);

    //gateway
    w5500_write_bytes(GAR, dhcp->gateway, sizeof(dhcp->gateway));
    //network mask
    w5500_write_bytes(SUBR, dhcp->mask, sizeof(dhcp->mask));
    //ip addr
    w5500_write_bytes(SIPR, dhcp->ip, sizeof(dhcp->ip));
}

/******************************************************************************
* Function    : w5500_dhcp_client_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init for dhcp client to get local ip addr
******************************************************************************/
static void w5500_dhcp_init(void)
{
    uint8 nullIP[4] = {0};

    //mac
    w5500_write_bytes(SHAR, pb_cfg_proc_get_mac(), PB_MAC_LEN);
    //gate way
    w5500_write_bytes(GAR, nullIP, sizeof(nullIP));
    //network mask
    w5500_write_bytes(SUBR, nullIP, sizeof(nullIP));
    //ip addr
    w5500_write_bytes(SIPR, nullIP, sizeof(nullIP));

    dhcp_client_init(&w5500_dhcp_client);
    
    memcpy(w5500_dhcp_client.mac, pb_cfg_proc_get_mac(), sizeof(w5500_dhcp_client.mac));
    w5500_dhcp_client.sock = SOCKET_DHCP;
    w5500_dhcp_client.sendto = w5500_socket_sendto;
    w5500_dhcp_client.recvfrom = w5500_socket_recvfrom;

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP client init OK");
}

/******************************************************************************
* Function    : w5500_dhcp_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_dhcp_process(void)
{
    bool ret = false;
    bool dhcp_process = true;
    uint8 stat;
    uint32 startTime = 0;

    startTime = os_get_tick_count();
    while(dhcp_process)
    {
        stat = dhcp_client_process(&w5500_dhcp_client);
        switch(stat)
        {
            case DHCP_RET_UPDATE:
            {
                w5500_net_update_local_config(&w5500_dhcp_client);
                dhcp_process = false;
                ret = true;
                break;
            }
            case DHCP_RET_CONFLICT:
            {
                OS_DBG_ERR(DBG_MOD_DEV, "DHCP IP conflict");
                dhcp_process = false;
                ret = false;
                break;
            }
            case DHCP_RET_LEASED_IP:
            {
                OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP IP LEASED");
                dhcp_process = false;
                ret = true;
                break;
            }
            case DHCP_RET_NONE:
            default:
            {
                break;
            }
        }

        os_scheduler_delay(DHCP_WAIT);
        if (w5500_check_timeout(startTime, DHCP_TIMEOUT))
        {
            ret = false;
            OS_DBG_ERR(DBG_MOD_DEV, "DHCP timeout");
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : w5500_dhcp
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check dhcp state and update ip address
******************************************************************************/
static bool w5500_dhcp(void)
{
    bool ret = false;

    w5500_socket_set_buffsize(SOCKET_DHCP);

    ret = w5500_socket(SOCKET_DHCP, SOCK_DGRAM, DHCP_CLIENT_PORT);
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "DHCP client sock create err");
        goto err;
    }

    ret = w5500_dhcp_process();
    
err:
    if (!w5500_close(SOCKET_DHCP))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "DHCP client sock close err");
    }

    return ret;
}

/******************************************************************************
* Function    : pb_dev_w5500_dhcp_check_lease_time
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_dhcp_check_lease_time(void)
{
    //When lease time pass half, we need renew ip
    uint32 curTime = os_get_second_count();
    if ((w5500_dhcp_client.leaseTime != INFINITE_LEASETIME) 
        && ((curTime - w5500_dhcp_client.lastTime) > (w5500_dhcp_client.leaseTime/2))) 
    {
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Need lease IP by DHCP[%d]", (curTime - w5500_dhcp_client.lastTime));
        if (!w5500_dhcp())
        {
            OS_DBG_ERR(DBG_MOD_DEV, "DHCP lease IP err");
            return false;
        }
        else
        {
            OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP[%d] lease time[%d]", 
                                        w5500_dhcp_client.state, w5500_dhcp_client.leaseTime);
        }
    }
    return true;
}

/******************************************************************************
* Function    : w5500_dns_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void w5500_dns_init(void)
{
    dns_client_init(&w5500_dns_client);
    
    w5500_dns_client.sock = SOCKET_DNS;
    w5500_dns_client.sendto = w5500_socket_sendto;
    w5500_dns_client.recvfrom = w5500_socket_recvfrom;

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DNS client init OK");
}

/******************************************************************************
* Function    : w5500_dns_server_select
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void w5500_dns_server_select(uint8 *dnsSer, uint8 *ip)
{
    PB_CFG_APC *cfgApc = &(pb_cfg_proc_get_cmd()->apc);
    switch (*dnsSer)
    {
        case ETH_DNS_DHCP_DNS:
        {
            *dnsSer = ETH_DNS_LAST_GOOD;
            memcpy(ip, w5500_dhcp_client.dns, sizeof(w5500_dhcp_client.dns));
            break;
        }
        case ETH_DNS_LAST_GOOD:
        {
            *dnsSer = ETH_DNS_MAIN;
            memcpy(ip, cfgApc->lastGoodDNS, sizeof(cfgApc->lastGoodDNS));
            break;
        }
        case ETH_DNS_MAIN:
        {
            *dnsSer = ETH_DNS_BACKUP;
            memcpy(ip, cfgApc->mainDNS, sizeof(cfgApc->mainDNS));
            break;
        }
        case ETH_DNS_BACKUP:
        {
            *dnsSer = ETH_DNS_DEFAULT;
            memcpy(ip, cfgApc->bkDNS, sizeof(cfgApc->bkDNS));
            break;
        }
        case ETH_DNS_DEFAULT:
        {
            *dnsSer = ETH_DNS_FAILED;
            memcpy(ip, DEF_DNS_SERVER, sizeof(DEF_DNS_SERVER));
            break;
        }
        default:
        {
            *dnsSer = ETH_DNS_DEFAULT;
            memcpy(ip, DEF_DNS_SERVER, sizeof(DEF_DNS_SERVER));
            break;
        }
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "USE DNS[%d], %d.%d.%d.%d",
                            *dnsSer,
                            ip[0], ip[1], ip[2], ip[3]);
}

/******************************************************************************
* Function    : pb_dev_w5500_net_dns_server_save
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void w5500_dns_server_save(uint8 *ip)
{
    PB_CFG_APC *cfgApc = &(pb_cfg_proc_get_cmd()->apc);

    if (0 != memcmp(ip, cfgApc->lastGoodDNS, sizeof(cfgApc->lastGoodDNS)))
    {
        memcpy(cfgApc->lastGoodDNS, ip, sizeof(cfgApc->lastGoodDNS));
        pb_cfg_proc_save_cmd(PB_PROT_CMD_APC, cfgApc, sizeof(PB_CFG_APC));

        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Save %d.%d.%d.%d as last good DNS server", 
                                ip[0], ip[1], ip[2], ip[3]);
    }
}

/******************************************************************************
* Function    : w5500_dns_process
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_dns_process(char *domain, uint8 *ip)
{
    bool ret = true;
    uint8 dnsSer = ETH_DNS_DHCP_DNS;

dnsRetry:
    w5500_dns_server_select(&dnsSer, w5500_dns_client.dnsIP);
    w5500_dns_client.lastTime = 0;
    w5500_dns_client.retryCnt = 0;
    
    if (DNS_ERR == dns_process(&w5500_dns_client, domain, ip))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "DNS process failed");
        if (dnsSer == ETH_DNS_FAILED)
        {
            ret = false;
        }
        else
        {
            goto dnsRetry;
        }
    }

    if (ret)
    {
        //check last good dns, if changed, save it
        w5500_dns_server_save(w5500_dns_client.dnsIP);
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DNS get IP %d.%d.%d.%d",
                                    ip[0], ip[1], ip[2], ip[3]);
    }

    return ret;
}

/******************************************************************************
* Function    : w5500_dns
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : DNS to check IP
******************************************************************************/
static bool w5500_dns(char *domain, uint8 *ip)
{
    bool ret = false;
    uint16 localPort = DNS_LOCAL_PORT;

    w5500_dns_init();

    w5500_socket_set_buffsize(SOCKET_DNS);

    localPort = w5500_random_port(DNS_LOCAL_PORT);
    ret = w5500_socket(SOCKET_DNS, SOCK_DGRAM, localPort);
    if (!ret)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "DNS client sock create err");
        goto err;
    }

    ret = w5500_dns_process(domain, ip);
    
err:
    if (!w5500_close(SOCKET_DNS))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "DNS client sock close err");
    }

    return ret;
}

/******************************************************************************
* Function    : w5500_net_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : net config
******************************************************************************/
static bool w5500_net_config(void)
{
    //software reset
    w5500_write_u8(MR, RST);
    os_scheduler_delay(DELAY_100_MS);

    //w5500 global config
    w5500_write_u16(RTR, W5500_SOCKET_RETRY_TIMEOUT);

    //re-try times
    w5500_write_u8(RCR, W5500_SOCKET_MAX_RETRY);
    //open socket 0 ir
    w5500_write_u8(SIMR, S0_IMR);
    w5500_write_u8(SIMR, S1_IMR);
    w5500_write_u8(SIMR, S2_IMR);
    w5500_write_u8(SIMR, S3_IMR);

    w5500_socket_set_buffsize(SOCKET_COM);

    #if ( 1 == ETH_DEBUG )
    //set default net config
    w5500_net_default_loca_config();
    #else
    //get dynamic ip addr by DHCP
    w5500_dhcp_init();
    if (!w5500_dhcp())
    {
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP get IP failed");
        return false;
    }
    else
    {
        OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "DHCP[%d] lease time[%d]", 
                                 w5500_dhcp_client.state, w5500_dhcp_client.leaseTime);
    }
    #endif /*ETH_DEBUG*/
		
    return true;
}

/******************************************************************************
* Function    : w5500_net_connect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : connet to server
******************************************************************************/
static bool w5500_net_connect(const char *domainName, uint16 port)
{
    bool ret = true;
    uint8 serIP[4] = {0, 0, 0, 0};
    uint16 localPort = 0;
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net connecting[%s:%d]", domainName, port);
    
    if (!w5500_dns((char*)domainName, serIP))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "Domain DNS err, use default server IP");
        goto err;
    }

    localPort = w5500_random_port(COM_LOCAL_PORT);
    if (!w5500_socket(SOCKET_COM, SOCK_STREAM, localPort))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "Sock create err");
        goto err;
    }

    if (!w5500_connect(SOCKET_COM, serIP, port))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "Net connect err");
        goto err;
    }

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net connect ok");

err:
    return ret;
}

/******************************************************************************
* Function    : w5500_net_disconnect
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : close socket and prepare to reset w5500
******************************************************************************/
static bool w5500_net_disconnect(void)
{
    bool ret = true;
    //disconnect 
    if (!w5500_disconnect(SOCKET_COM))
    {
        ret = false;
        OS_DBG_ERR(DBG_MOD_DEV, "net disconnect err");
        goto err;
    }
    //close
    if (!w5500_close(SOCKET_COM))
    {
        OS_DBG_ERR(DBG_MOD_DEV, "socket close err");
    }
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "net disconnect OK");
    
err:
    return ret;
}

/******************************************************************************
* Function    : w5500_net_send
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : send data to socket
******************************************************************************/
static uint16 w5500_net_send(const uint8 *pdata, uint16 len)
{
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Send[%d]", len);

    return w5500_socket_send(SOCKET_COM, pdata, len);
}

/******************************************************************************
* Function    : w5500_net_recv
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : recv data from socket
******************************************************************************/
static uint16 w5500_net_recv(uint8 *pdata, uint16 maxLen)
{
    return w5500_socket_recv(SOCKET_COM, pdata, maxLen);
}

/******************************************************************************
* Function    : w5500_net_check_stat
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check w550 current network stat
******************************************************************************/
static bool w5500_net_check_stat(void)
{
    //check cable fitst
    if (!w5500_cable_link())
    {
        return false;
    }
    
    uint8 netStat;
    netStat = w5500_read_sock_u8(SOCKET_COM, Sn_SR);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "Net stat[%02X]", netStat);

    if (netStat != SOCK_ESTABLISHED)
    {
        return false;
    }

    //check lease time
    if (!w5500_dhcp_check_lease_time())
    {
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : w5500_hw_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_hw_reset(void)
{
    //w5500 hw reset
    hal_gpio_set(BOARD_W5500_RST, HAL_GPIO_LOW);
    os_scheduler_delay(DELAY_100_MS);
    hal_gpio_set(BOARD_W5500_RST, HAL_GPIO_HIGH);

    W5500_SPI->deinit();
    W5500_SPI->init();
    //wait spi and w5500 stable
    os_scheduler_delay(DELAY_1_S*3);
    
    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "W5500 RST OK, VER[%d]", w5500_read_u8(VERR));
    return true;
}

/******************************************************************************
* Function    : w5500_hw_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool w5500_hw_init(HAL_SPI_TYPE *spi)
{
    if (spi == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "W5500 spi is invalid");
        return false;
    }
    W5500_SPI = spi;

    hal_rcc_enable(BOARD_W5500_IO_RCC);
    hal_gpio_set_mode(BOARD_W5500_RST, GPIO_Mode_Out_PP);

    OS_DBG_TRACE(DBG_MOD_DEV, DBG_INFO, "W5500 INIT OK");
    return true;
}

const DEV_TYPE_W5500 devW5500 = 
{
    w5500_hw_init,
    w5500_hw_reset,
    w5500_cable_link,
    w5500_net_config,
    w5500_net_connect,
    w5500_net_disconnect,
    w5500_net_recv,
    w5500_net_send,
    w5500_net_check_stat
};

