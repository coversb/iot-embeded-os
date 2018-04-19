/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_assemble.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   assemble protocol message and ack
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_ASSEMBLE_H__
#define __PB_PROT_ASSEMBLE_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_ASSEMBLE_HEAD_CHECK_LEN 16  //keep same with PB_PROT_PARSE_CMD_HEAD_CHECK_LEN

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern uint16 pb_prot_assemble_ack(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame, 
                                                                PB_PROT_ACK_PACK_TYPE *ackPack);
extern uint16 pb_prot_assemble_hbp(PB_PROT_HBP_PACK_TYPE *hbpPack);
extern uint16 pb_prot_assemble_rsp(PB_PROT_RSP_PACK_TYPE *rspPack);

#endif /*__PB_PROTOCOL_ASSEMBLE_H__*/
