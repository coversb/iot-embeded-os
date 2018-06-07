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
#ifndef __PB_ORDER_HOTP_H__
#define __PB_ORDER_HOTP_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_ORDER_OFFLINE_BUFFSIZE 30

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_ORDER_HOTP_UNKNOWN = 0,
    PB_ORDER_HOTP_ENG,
    PB_ORDER_HOTP_OFFLINE,
    PB_ORDER_HOTP_LASTOFFLINE
}PB_ORDER_HOTP_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint32 inputTime;
    uint32 startTime;
    uint32 password;
}PB_OFFLINE_ORDER;

typedef struct
{
    uint16 size;
    PB_OFFLINE_ORDER order[PB_ORDER_OFFLINE_BUFFSIZE];
}PB_ORDER_OFFLINE_BUFF;

typedef struct
{
    uint32 updateTime;
    uint16 lastOfflinePw[PB_ORDER_OFFLINE_PW_NUM];
    uint16 offlinePw[PB_ORDER_OFFLINE_PW_NUM];
    uint32 engPw[PB_ORDER_ENG_PW_NUM];
}PB_ORDER_HOTP_CONTEXT;

/******************************************************************************
* Extern functions
******************************************************************************/
extern void pb_order_hotp_init(void);
extern void pb_order_hotp_update(void);
extern void pb_order_hotp_try_to_send_buffer_order(void);
extern PB_ORDER_OFFLINE_BUFF *pb_order_hotp_offline_order_buff(void);
extern void pb_order_hotp_offline_password_print(void);
extern uint8 pb_order_hotp_verify_password(uint32 password);

#endif /*__PB_HOTP_H__*/
