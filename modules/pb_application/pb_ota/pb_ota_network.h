/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_ota_network.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   over the air network functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_OTA_NETWORK_H__
#define __PB_OTA_NETWORK_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "os_datastruct.h"
#include "pb_prot_main.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_OTA_NET_DEV_UNKNOW = 0,
    PB_OTA_NET_DEV_GPRS,
    PB_OTA_NET_DEV_ETH,
    PB_OTA_NET_DEV_NUM
}PB_OTA_NET_DEV_TYPE;

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_ota_network_context_init(void);
extern uint16 pb_ota_network_send_que_size(void);
extern bool pb_ota_network_send_que_append(const uint8* data, uint16 len);
extern void pb_ota_network_send_que_remove_head(void);
extern bool pb_ota_network_que_head_data(uint8 **pdata, uint16 *len);

extern void pb_ota_network_hw_init(uint8 devType);
extern void pb_ota_network_hw_reset(uint8 devType);
extern bool pb_ota_network_available(uint8 devType);
extern bool pb_ota_network_config(uint8 devType);
extern bool pb_ota_network_check_net_stat(uint8 devType);
extern bool pb_ota_network_modal_info(uint8 devType);
extern bool pb_ota_network_connect(uint8 devType, const char *domainName, uint16 port);
extern void pb_ota_network_disconnect(uint8 devType);
extern uint16 pb_ota_network_send(uint8 devType, uint8 *data, uint16 size);
extern uint16 pb_ota_network_recv(uint8 devType, uint8 *data, uint16 maxLen);
extern void *pb_ota_network_get_ftp_client(uint8 devType);

extern void pb_ota_network_update_csq(uint8 devType);
extern uint8 pb_ota_network_get_csq_rssi(void);
extern void pb_ota_network_set_csq_rssi(uint8 rssi);
extern uint8 pb_ota_network_get_csq_ber(void);
extern void pb_ota_network_set_csq_ber(uint8 ber);
    
#endif /* __PB_OTA_NETWORK_H__ */

