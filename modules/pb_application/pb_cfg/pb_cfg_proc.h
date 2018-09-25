/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_cfg_proc.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   define all configuration types need save in board
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-16      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_CFG_PROC_H__
#define __PB_CFG_PROC_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "pb_prot_type.h"

/******************************************************************************
* Macros
******************************************************************************/
/*device info(SN,UID,MAC) config address*/
#define PB_CFG_DEVINFO_ADDR PB_DEVINFO_ADDR

/*command config read address*/
#define PB_CFG_CMD_GET_ADDR(HEADER_ADDR, CMD) ((HEADER_ADDR) + sizeof(PB_CFG_##CMD)) 
#define PB_CFG_CMD_ADDR PB_CMDCFG_ADDR
#define PB_CFG_APC_ADDR PB_CFG_CMD_ADDR
#define PB_CFG_SER_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_APC_ADDR, APC)
#define PB_CFG_CFG_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_SER_ADDR, SER)
#define PB_CFG_TMA_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_CFG_ADDR, CFG)
#define PB_CFG_DOG_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_TMA_ADDR, TMA)
#define PB_CFG_ACO_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_DOG_ADDR, DOG)
#define PB_CFG_SEC_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_ACO_ADDR, ACO)
#define PB_CFG_OMC_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_SEC_ADDR, SEC)
#define PB_CFG_ACW_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_OMC_ADDR, OMC)
#define PB_CFG_DOA_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_ACW_ADDR, ACW)
#define PB_CFG_SMA_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_DOA_ADDR, DOA)
#define PB_CFG_MUO_ADDR PB_CFG_CMD_GET_ADDR(PB_CFG_SMA_ADDR, SMA)

#define PB_UID_LEN 8
#define PB_MAC_LEN 6
#define PB_SN_LEN 16

#define PB_CFG_APC_PENDING 50
#define PB_CFG_SER_PENDING 50
#define PB_CFG_CFG_PENDING 50
#define PB_CFG_TMA_PENDING 50
#define PB_CFG_DOG_PENDING 50
#define PB_CFG_ACO_PENDING 50
#define PB_CFG_INS_PENDING 50
#define PB_CFG_SEC_PENDING 50
#define PB_CFG_OMC_PENDING (50-1)
#define PB_CFG_ACW_PENDING 50
#define PB_CFG_OWC_PENDING 50
#define PB_CFG_DOA_PENDING (50-1)
#define PB_CFG_SMA_PENDING 50
#define PB_CFG_MUO_PENDING 50 

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_CFG_CMD_ERR,
    PB_CFG_CMD_OK,
    PB_CFG_CMD_UNKNOW
}PB_CFG_CMD_VALID;

/******************************************************************************
* device info configuration data struct
******************************************************************************/
typedef struct
{
    uint8 uid[PB_UID_LEN];
    uint8 mac[PB_MAC_LEN];
    uint8 sn[PB_SN_LEN];
    uint16 crc;
}PB_CFG_DEV_INFO;

/******************************************************************************
* Command configuration data struct
******************************************************************************/
//APN
typedef struct
{
    uint8 apn[PB_APN_LEN + 1];
    uint8 usr[PB_APN_USR_LEN + 1];
    uint8 pass[PB_APN_PASS_LEN + 1];
    uint8 mainDNS[PB_APN_DNS_LEN];
    uint8 bkDNS[PB_APN_DNS_LEN];
    uint8 lastGoodDNS[PB_APN_DNS_LEN];

    uint32 align;
    uint8 pending[PB_CFG_APC_PENDING];
    uint16 crc;
}PB_CFG_APC;

//Server configuration
typedef struct
{
    uint8 mode;
    uint8 mainServer[PB_SER_DOMAINNAME_LEN + 1];
    uint16 mainPort;
    uint8 bakServer[PB_SER_DOMAINNAME_LEN + 1];
    uint16 bakPort;
    uint8 smsGateway[PB_SER_SMS_GATEWAY_LEN + 1];
    uint8 hbpInterval;
    uint16 randomTime;

    uint32 align;
    uint8 pending[PB_CFG_SER_PENDING];
    uint16 crc;
}PB_CFG_SER;

//Global Configuration
typedef struct
{
    uint16 eventMask;
    uint16 infInterval;

    uint32 align;
    uint8 pending[PB_CFG_CFG_PENDING];
    uint16 crc;
}PB_CFG_CFG;

//Time Adjust
typedef struct
{
    uint8 mode;
    uint8 sign;
    uint8 hour;
    uint8 minute;
    uint8 daylightSaving;

    uint32 align;
    uint8 pending[PB_CFG_TMA_PENDING];
    uint16 crc;
}PB_CFG_TMA;

//Protocol Watchdog
typedef struct
{
    uint8 mode;
    uint8 report;
    uint8 interval;
    uint8 rstHour;
    uint8 rstMinute;
    uint16 randomDuration;

    uint32 align;
    uint8 pending[PB_CFG_DOG_PENDING];
    uint16 crc;
}PB_CFG_DOG;

//Air Conditioner Operation
typedef struct
{
    uint8 pwrMode;
    uint8 workMode;
    uint8 windLevel;
    uint8 interval;
    uint8 duration;
    uint8 temperature;

    uint32 align;
    uint8 pending[PB_CFG_ACO_PENDING];
    uint16 crc;
}PB_CFG_ACO;

//Security Configuration
typedef struct
{
    uint8 serviceKey[PB_SEC_KEY_LEN];
    uint8 normalKey[PB_SEC_KEY_LEN];
    uint8 comKey[PB_SEC_KEY_LEN];
    uint8 hotpMode;

    uint32 align;
    uint8 pending[PB_CFG_SEC_PENDING];
    uint16 crc;
}PB_CFG_SEC;

//Output Mode Configuration
typedef struct
{
    uint32 idleOutput;
    uint32 inServiceOutput;
    uint8 mode;
    uint8 startHour;
    uint8 startMin;
    uint8 stopHour;
    uint8 stopMin;
    uint32 validIdleOutput;
    uint32 validInServiceOutput;

    uint32 align;
    uint8 pending[PB_CFG_OMC_PENDING];
    uint8 tvRebootSw;
    uint8 exhaustForceMode;
    uint16 crc;
}PB_CFG_OMC;

//Air conditioner working config
typedef struct
{
    uint8 mode;
    uint8 pwrOnEventMask;
    uint8 pwrOffEventMask;
    uint8 duration;
    uint8 startHour;
    uint8 startMin;
    uint8 stopHour;
    uint8 stopMin;
    
    uint32 align;
    uint8 pending[PB_CFG_ACW_PENDING];
    uint16 crc;
}PB_CFG_ACW;

//Output working config
typedef struct
{
    uint8 mode;
    uint8 startHour;
    uint8 startMin;
    uint8 stopHour;
    uint8 stopMin;
}PB_CFG_OWC_ITEM;

typedef struct
{
    PB_CFG_OWC_ITEM item[PB_OWC_SIZE];
    uint32 align;
    uint8 pending[PB_CFG_OWC_PENDING];
    uint16 crc;
}PB_CFG_OWC;

//Door Alarm
typedef struct
{
    uint8 mode;
    uint8 triggerType;
    uint8 duration;
    uint8 interval;
    
    uint32 align;
    uint8 pending[PB_CFG_DOA_PENDING];
    uint8 reverse;
    uint16 crc;
}PB_CFG_DOA;

//Smoke Alarm
typedef struct
{
    uint8 mode;
    uint8 threshold;
    uint8 duration;
    uint8 interval;
    
    uint32 align;
    uint8 pending[PB_CFG_SMA_PENDING];
    uint16 crc;
}PB_CFG_SMA;

//Mutilmedia config
typedef struct
{
    uint8 volume;
    uint8 autoBGM;
    
    uint32 align;
    uint8 pending[PB_CFG_MUO_PENDING];
    uint16 crc;
}PB_CFG_MUO;

typedef struct
{
    PB_CFG_APC apc;
    PB_CFG_SER ser;
    PB_CFG_CFG cfg;
    PB_CFG_TMA tma;
    PB_CFG_DOG dog;
    PB_CFG_ACO aco;
    PB_CFG_SEC sec;
    PB_CFG_DOA doa;
    PB_CFG_SMA sma;
    PB_CFG_MUO muo;
    PB_CFG_OMC omc;
    PB_CFG_ACW acw;
    PB_CFG_OWC owc;
}PB_CFG_CMD;

/******************************************************************************
* External variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern uint8 pb_cfg_proc_get_dev_type(void);
extern uint16 pb_cfg_proc_get_hardware_version(void);
extern const uint8* pb_cfg_proc_get_sn(void);
extern void pb_cfg_proc_set_sn(uint8 *sn);
extern const uint8* pb_cfg_proc_get_uid(void);
extern void pb_cfg_proc_set_uid(uint8 *uid);
extern const uint8* pb_cfg_proc_get_mac(void);
extern void pb_cfg_proc_set_mac(uint8 *mac);

extern PB_CFG_CMD* pb_cfg_proc_get_cmd(void);
extern bool pb_cfg_proc_save_cmd(uint8 type, void *arg, uint32 size);
extern void pb_cfg_proc_reset_all_cmd(void);
extern void pb_cfg_proc_init(void);

#endif /* __PB_CFG_PROC_H__ */

