/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_fota.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   fota functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_FOTA_H__
#define __PB_FOTA_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"
#include "pb_firmware_manage.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/
/*FOTA context definition*/
typedef struct
{
    uint8 retry;
    uint8 timeout;
    uint8 protocol;
    uint8 url[PB_FOTA_URL_LEN + 1];
    uint16 port;
    uint8 usrName[PB_FOTA_USR_LEN + 1];
    uint8 usrPass[PB_FOTA_PASS_LEN + 1];
    uint8 md5[PB_FOTA_MD5_LEN];
    uint32 key;
    uint32 downAddr;
    uint32 bootAddr;
    
    uint8 curRetryCnt;
    uint32 totalSize;
}PB_FOTA_CONTEXT;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_fota_main(void *parameters);
extern void pb_fota_send_msg_to_fota_mod(uint8 msgID);
extern void pb_fota_set_param(PB_PROT_CMD_FOTA_ARG *param);
extern bool pb_fota_upgrading(void);

extern void pb_fota_get_firmware_info(uint32 *upTimestamp, uint32 *runTimes);
extern uint16 pb_fota_get_firmware_version(void);
extern uint16 pb_fota_get_bl_version(void);

#endif /* __PB_FOTA_H__ */

