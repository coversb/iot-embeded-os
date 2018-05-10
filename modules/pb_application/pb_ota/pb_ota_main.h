/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ota_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   over the air communication
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_OTA_MAIN_H__
#define __PB_OTA_MAIN_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_util.h"
#include "pb_ota_network.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_OTA_RECV_BUFF_SIZE PB_PROT_INPUT_BUFFSIZE //(512 + 30)   //stay same with PB_PROT_INPUT_BUFFSIZE

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_OTA_NET_DEV_INIT = 0,
    PB_OTA_NET_CONFIG,
    PB_OTA_NET_CONNECT,
    PB_OTA_NET_CONNECTED,
    PB_OTA_NET_CLOSE
}PB_OTA_NET_STATE;

typedef enum
{
    PB_OTA_SERVER_MAIN = 0,
    PB_OTA_SERVER_BACKUP,
}PB_OTA_SERVER_SELECT;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    int8 sock;
    uint8 net_state;
}PB_OTA_NET_INFO;

typedef struct
{
    bool need_reboot;
    uint8 act_net_dev;
    uint8 act_server;
    uint8 server_connect_cnt;
    uint8 csq_update_cnt;
    PB_OTA_NET_INFO net_info[PB_OTA_NET_DEV_NUM];
    uint8 send_retry_cnt;
    uint8 net_retry_cnt;
    bool b_sending;
    bool b_recving;
    bool b_recv_copying;
    uint16 recv_len;
    uint8 recv_buff[PB_OTA_RECV_BUFF_SIZE + 1];
}PB_OTA_CONTEXT_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_ota_main(void *pvParameters);
extern void pb_ota_send_msg_to_ota_mod(uint8 msgID);
extern void pb_ota_send_msg_data_to_ota_mod(uint8 msgID, uint8 data);
//extern void pb_ota_send_msgdata_to_ota_mod(PB_MSG_TYPE *msg);
extern void pb_ota_send_dev_msg_to_ota_mod(uint8 devType, uint8 msgID);
extern bool pb_ota_send_data_append(const uint8* data, uint16 len);
extern void pb_ota_set_recv_copying(bool sw);
extern uint16 pb_ota_get_recv_data(uint8 *data, uint16 maxLen);
extern uint8 pb_ota_net_get_act_dev(void);
extern uint8 pb_ota_net_get_stat(void);
extern void pb_ota_try_to_send_data(void);
extern void pb_ota_try_to_recv_data(void);
extern bool pb_ota_network_connected(void);
extern void pb_ota_need_set_reboot(bool sw);

#endif /* __PB_OTA_MAIN_H__ */

