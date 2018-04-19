/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_proc.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   define all the protocol process data type
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_PROC_H__
#define __PB_PROT_PROC_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_PROT_PROC_CMD_ERR,
    PB_PROT_PROC_CMD_OK,
    PB_PROT_PROC_CMD_UNKNOW
}PB_PROT_PROC_CMD_VALID;

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_prot_proc_cmd_exec(PB_PROT_CMD_PARSED_FRAME_TYPE *parsedFrame);
extern bool pb_prot_proc_save_audio_volume(uint8 volume);
extern void pb_prot_proc_save_last_good_dns(void);
extern void pb_prot_proc_watchdog_check(void);
extern void pb_prot_proc_hbp_send_check(void);
extern void pb_prot_proc_inf_send_check(void);
extern void pb_prot_proc_update_rtc(uint32 timestamp);

extern uint16 pb_prot_proc_get_report_sn(void);

extern void pb_prot_proc_reset_hotp_key(void);
extern void pb_prot_proc_reset_com_key(void);

extern void pb_prot_proc_device_basic_info_process(void);
extern void pb_prot_proc_set_dev_gsm_info(PB_PROT_RSP_GSMINFO_PARAM *gsm);
extern void pb_prot_proc_get_dev_gsm_info(PB_PROT_RSP_GSMINFO_PARAM *gsm);
extern void pb_prot_proc_set_dev_location(PB_PROT_RSP_LOC_PARAM *loc);
extern void pb_prot_proc_get_dev_location(PB_PROT_RSP_LOC_PARAM *loc);

extern void pb_prot_proc_set_sack_cnt(bool np);
extern bool pb_prot_proc_is_sack_timeout(void);

extern void pb_prot_proc_init(void);

#endif /* __PB_PROT_PROC_H__ */

