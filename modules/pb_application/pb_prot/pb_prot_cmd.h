/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_cmd.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   ascii command parser
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-18      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_CMD_H__
#define __PB_PROT_CMD_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/
#define PB_PROT_DBGCMD_LOG_L "+pblog="
#define PB_PROT_DBGCMD_LOG_U "+PBLOG="

#define PB_PROT_CFGREQ_UID_L "+pbuid?"
#define PB_PROT_CFGREQ_UID_U "+PBUID?"
#define PB_PROT_CFGCMD_UID_L "+pbuid="
#define PB_PROT_CFGCMD_UID_U "+PBUID="

#define PB_PROT_CFGREQ_MAC_L "+pbmac?"
#define PB_PROT_CFGREQ_MAC_U "+PBMAC?"
#define PB_PROT_CFGCMD_MAC_L "+pbmac="
#define PB_PROT_CFGCMD_MAC_U "+PBMAC="

#define PB_PROT_CFGREQ_SN_L "+pbsn?"
#define PB_PROT_CFGREQ_SN_U "+PBSN?"
#define PB_PROT_CFGCMD_SN_L "+pbsn="
#define PB_PROT_CFGCMD_SN_U "+PBSN="

#define PB_PROT_CFGCMD_FCT_L "+pbfct="
#define PB_PROT_CFGCMD_FCT_U "+PBFCT="

#define PB_PROT_CFGCMD_AC_L "+pbac="
#define PB_PROT_CFGCMD_AC_U "+PBAC="

#define PB_PROT_CMD_ASCII_MIN_LEN 6

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_PROT_FCT_NONE = 0,
    PB_PROT_FCT_RED_BLINK = 1,
    PB_PROT_FCT_GREEN_BLINK = 2,
    PB_PROT_FCT_WHITE_BLINK = 3,
    PB_PROT_FCT_INPUT = 4,
    PB_PROT_FCT_OUTPUT = 5,
    PB_PROT_FCT_RELAY = 6,
    PB_PROT_FCT_WG = 7,
    PB_PROT_FCT_AUDIO_CHECK = 8,
    PB_PROT_FCT_AUDIO_VOLUME_SET = 9,
    PB_PROT_FCT_AUDIO_CTRL = 10,
    PB_PROT_FCT_DATETIME = 80,
    PB_PROT_FCT_HOTP = 81,
    PB_PROT_FCT_ENCRYPT = 82,
    PB_PROT_FCT_RESET_HOTP_KEY = 83,
    PB_PROT_FCT_RESET_COM_KEY = 84,
    PB_PROT_FCT_HOTP_SW = 85,
    PB_PROT_FCT_CLEAR_ORDER = 90,
    PB_PROT_FCT_SHOW_ORDER = 91,
    PB_PROT_FCT_STACK = 100,
    PB_PROT_FCT_HEAP = 101,
    PB_PROT_FCT_REBOOT = 200,
    PB_PROT_FCT_END
}PB_PROT_FCT_CMD_TYPE;

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern bool pb_prot_cmd_parse_ascii(PB_PROT_RAW_PACKET_TYPE *rawPack);

#endif /* __PB_PROT_CMD_H__ */

