/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   parkbox protocol main flow
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-16      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_MAIN_H__
#define __PB_PROT_MAIN_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"
#include "pb_app_message.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_RSP_BUFF_SIZE (512 + 30)
#define PB_PROT_INPUT_BUFFSIZE (512 + 30)
#define PB_PROT_CIPHER_BUFFSIZE (512 + 30)
#define PB_PROT_PLAINTEXT_BUFFSIZE (512 + 30)

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_PROT_PARSE_INPUT = 0,
    PB_PROT_PARSE_RAWBUFF
}PB_PROT_PARSE_TYPE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    bool bNeedWaitData;
    uint8 parseType;
    //new input data
    volatile bool input[PB_PORT_SRC_NUM];
    uint16 inputLen;
    uint8 inputBuff[PB_PROT_INPUT_BUFFSIZE+1];
    //none-parsed rawbuff
    volatile bool rawbuff[PB_PORT_SRC_NUM];
}PB_PROT_CONTEXT_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const char *pbProtSrcName[];

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_prot_main(void *pvParameters);
extern void pb_prot_input_available_set(uint8 id, bool available);
extern void pb_prot_send_msgdata_to_prot_mod(PB_MSG_TYPE *msg);
extern void pb_prot_send_msg_to_prot_mod(uint8 msgID);
extern void pb_prot_send_rsp_req(PB_PROT_MSG_RSP_TYPE msgSubType);
extern void pb_prot_send_rsp_param_req(PB_PROT_MSG_RSP_TYPE msgSubType, uint8 *param, uint16 paramSize);
extern void pb_prot_set_need_wait_data(bool sw);
extern bool pb_prot_check_event(uint8 subType);
extern void pb_prot_send_dbg_info_req(uint8 *debugInfo, uint16 infoLen);
extern void pb_prot_send_sae_req(uint8 alarmType, uint8 alarmLv);
extern void pb_prot_send_uie_req(uint8 type, uint8 *data);

#endif /* __PB_PROT_MAIN_H__ */

