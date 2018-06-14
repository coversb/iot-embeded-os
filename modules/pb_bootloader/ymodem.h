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
#ifndef __YMODEM_H__
#define __YMODEM_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PACKET_HEADER_SIZE      ((uint32)3)
#define PACKET_START_INDEX      ((uint32)1)
#define PACKET_NUMBER_INDEX     ((uint32)2)
#define PACKET_CNUMBER_INDEX    ((uint32)3)
#define PACKET_DATA_INDEX       ((uint32)4)
#define PACKET_TRAILER_SIZE     ((uint32)2)
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
#define PACKET_SIZE             ((uint32)128)
#define PACKET_1K_SIZE          ((uint32)1024)

/* /-------- Packet in IAP memory ------------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  | 
 * |------------------------------------------------------------------------|
 * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/
 * the first byte is left unused for memory alignment reasons                 */

#define FILE_NAME_LENGTH    ((uint32)64)
#define FILE_SIZE_LENGTH    ((uint32)16)

#define SOH    ((uint8)0x01)  /* start of 128-byte data packet */
#define STX    ((uint8)0x02)  /* start of 1024-byte data packet */
#define EOT    ((uint8)0x04)  /* end of transmission */
#define ACK    ((uint8)0x06)  /* acknowledge */
#define NAK    ((uint8)0x15)  /* negative acknowledge */
#define CA    ((uint8)0x18) /* two of these in succession aborts transfer */
#define CRC16    ((uint8)0x43)  /* 'C' == 0x43, request 16-bit CRC */
#define NEGATIVE_BYTE    ((uint8)0xFF)

#define ABORT1    ((uint8)0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2    ((uint8)0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT    ((uint32)0x10000)
#define DOWNLOAD_TIMEOUT DELAY_500_MS /* 500 ms retry delay */

#define YMODEM_FILEREQ_MAX 3
#define YMODEM_ERR_MAX 5

/******************************************************************************
* Enum
******************************************************************************/
typedef enum
{
    YMODEM_RECV_NONE = 0xF0,    // be different from YMODEM packet type
    YMODEM_RECV_ERR,
    YMODEM_RECV_FINISH,
    YMODEM_RECV_TRANS_ABORT,
    YMODEM_RECV_USER_ABORT
}YMODEM_RECV_TYPE;

typedef enum
{
    YMODEM_INIT = 0,
    YMODEM_OK,
    YMODEM_ERROR,
    YMODEM_NONE,
    YMODEM_ABORT,
    YMODEM_TIMEOUT,
    YMODEM_FLASHERR,
    YMODEM_LIMIT,
} YMODEM_STATUS;

/******************************************************************************
* Global Functions
******************************************************************************/
extern YMODEM_STATUS ymodem_process(void);

#endif  /* __YMODEM_H__ */
