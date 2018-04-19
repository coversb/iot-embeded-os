/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_parse.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_PARSE_H__
#define __PB_PROT_PARSE_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_UART_RAW_SIZE 1024
#define PB_PROT_OTA_RAW_SIZE 1024
#define PB_PROT_BT_RAW_SIZE 1024
#define PB_PROT_SMS_RAW_SIZE 800    //160 * 5

/*
header                      3 bytes
device type                1 byte
protocol version          2 bytes
UID                           8 bytes
length                        2 bytes
head check code          2 bytes
send time                   4 bytes (encrypt)
serial number              2 bytes (encrypt)
message type             1 byte   (encrypt)
message sub-type       1 byte   (encrypt)
content                      N bytes  (encrypt)
check code                  2 bytes
tail                            2 bytes
*/
#define PB_PROT_PARSE_CMD_PLAINTEXT_SIZE 22
#define PB_PROT_PARSE_CMD_HEAD_CHECK_LEN 16

#define PB_PROT_PARSE_CMD_HEADER_POS 0
#define PB_PROT_PARSE_CMD_DEVICETYPE_POS 3  // 0 + 3
#define PB_PROT_PARSE_CMD_PROTVER_POS 4      // 3 + 1
#define PB_PROT_PARSE_CMD_UID_POS 6               // 4 + 2
#define PB_PROT_PARSE_CMD_LENGTH_POS 14       // 6 + 8
#define PB_PROT_PARSE_CMD_HEAD_CHECKCODE_POS 16 //14 + 2
#define PB_PROT_PARSE_CMD_CIPHER_POS 18        // 16 + 2
#define PB_PROT_PARSE_CMD_CHECKCODE_POS(n) ((n) + PB_PROT_PARSE_CMD_CIPHER_POS)
#define PB_PROT_PARSE_CMD_TAIL_POS(n) ((n) + PB_PROT_PARSE_CMD_CIPHER_POS + 2)

#define PB_PROT_PARSE_CMD_CIPHER_FIX_SIZE 8  
#define PB_PROT_PARSE_CMD_SENDTIME_POS 0
#define PB_PROT_PARSE_CMD_SN_POS 4                  // 0 + 4
#define PB_PROT_PARSE_CMD_MSGTYPE_POS 6        // 4 + 2
#define PB_PROT_PARSE_CMD_MSGSUBTYPE_POS 7 // 6 + 1
#define PB_PROT_PARSE_CMD_CONTENT_POS 8       // 7 + 1

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PROT_PARSE_ERR = 0,
    PROT_PARSE_OK,
    PROT_PARSE_NEED_WAIT
}PB_PROT_PARSE_RET_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint16 rawHead;
    uint16 rawTail;
    uint16 valuedataCnt;
    uint16 pbProtRawMaxSize;
    uint8 *pbProtRaw;
}PB_PROT_RAW_QUEUE_TYPE;

typedef struct
{
    PB_PROT_RAW_QUEUE_TYPE rawQueue[PB_PORT_SRC_NUM];
}PB_PROT_RAW_CONTEXT_TYPE;

typedef struct
{
    uint32 maxSize;
    uint8 *pBuff;
}PB_PROT_RAW_BUFF_TYPE;

typedef struct
{
    uint8 msgType;
    uint8 msgSubType;
    uint16 serialNumber;
    uint8 *content;
    uint16 contentLen;
}PB_PROT_PARSED_CONTENT_TYPE;

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_prot_parse_init(void);
extern bool pb_prot_parse_rawbuff_check(uint8 id);
extern bool pb_prot_parse_raw_data(PB_PROT_RAW_PACKET_TYPE *rawPack, 
                                                               PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame);
#endif /*__PB_PROT_PARSE_H__*/
