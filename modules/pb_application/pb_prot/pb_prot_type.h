/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_prot_type.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   define all the protocol data type
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-16      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_PROT_TYPE_H__
#define __PB_PROT_TYPE_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Protocol header and tail flags
******************************************************************************/
#define PB_PROT_HEADFLAG_PLUSE 0x2B
#define PB_PROT_HEADFLAG_P 0x50
#define PB_PROT_HEADFLAG_B 0x42
#define PB_PROT_TAILFLAG_R 0x0D //'\r'
#define PB_PROT_TAILFLAG_N 0x0A //'\n'

#define PB_PROT_CMD_MIN_LENGTH 30

/******************************************************************************
* Protocol enums
******************************************************************************/
//protocol data sources, order by priority
typedef enum
{
    PB_PORT_SRC_BEGIN = 0,
    PB_PROT_SRC_UART = PB_PORT_SRC_BEGIN,
    PB_PORT_SRC_OTA,
    PB_PORT_SRC_NUM,
    PB_PORT_SRC_BT,
    PB_PORT_SRC_SMS,
    PB_PORT_SRC_UNKNWON
}PB_PROT_SRC_ID;

typedef enum
{
    PB_PROT_RESERVED = 0,               //0x00 reserved
    PB_PROT_BEGIN = 1,
    PB_PROT_CMD = PB_PROT_BEGIN,    //0x01 command
    PB_PROT_ACK,                                //0x02 acknowledgement
    PB_PROT_RSP,                                //0x03 report
    PB_PROT_HBP,                                //0x04 heart-beat package
    PB_PROT_END
}PB_PROT_TYPE;

typedef enum
{
    PB_PROT_CMD_RESERVED = 0x00,                      //0x00 reserved
    PB_PROT_CMD_BEGIN = 0x01,
    /*configuration command*/
    PB_PROT_CMD_APC = PB_PROT_CMD_BEGIN,        //0x01 Access Point Configuration
    PB_PROT_CMD_SER = 0x02,                                //0x02 Server Configuration
    PB_PROT_CMD_CFG = 0x03,                                //0x03 Global Configuration
    PB_PROT_CMD_TMA = 0x04,                                //0x04 Time Adjust
    PB_PROT_CMD_DOG = 0x05,                                //0x05 Protocol Watchdog
    PB_PROT_CMD_ACO = 0x06,                                //0x06 Air Conditioner Operation
    PB_PROT_CMD_SEC = 0x07,                                 //0x07 Security Configuration
    PB_PROT_CMD_OMC = 0x08,                                //0x08 Output Mode Configuration
    PB_PROT_CMD_ACW = 0x09,                                //0x09 Air Conditioner Working Configuration
    PB_PROT_CMD_OWC = 0x0A,                                //0x0A Output Working Configuration
    /*alarm configuration command*/
    PB_PROT_CMD_DOA = 0x31,                                //0x31 Door Alarm
    PB_PROT_CMD_SMA = 0x32,                                 //0x32 Smoke Alarm
    /*action operate command*/
    PB_PROT_CMD_OUO = 0x81,                                //0x81 Order Update Operation
    PB_PROT_CMD_OUT = 0x82,                                //0x82 Output Operation
    PB_PROT_CMD_MUO = 0x83,                                //0x83 Mutilmedia Operation
    PB_PROT_CMD_RTO = 0x84,                                //0x84 Real Time Operation
    /*firmware over the air*/
    PB_PROT_CMD_FOTA = 0xF1,                               //0xF1 Firmware over the air
    PB_PROT_CMD_END
}PB_PROT_CMD_TYPE;

typedef enum
{
    /*KEEP SAME WITH PB_PROT_CMD_TYPE*/    
    PB_PROT_ACK_GEN = 0xFF, //0xFF General acknowledgement
}PB_PROT_ACK_TYPE;

typedef enum
{
    PB_PROT_RSP_RESERVED = 0x00,    //0x00 reserved
    PB_PROT_RSP_BEGIN = 0x01,
    /*normal event report*/
    PB_PROT_RSP_PNE = 0x01, //0x01 power on event
    PB_PROT_RSP_PFE = 0x02, //0x02 power off event
    PB_PROT_RSP_INF = 0x03, //0x03 information
    PB_PROT_RSP_DOG = 0x05, //0x05 watchdog report
    PB_PROT_RSP_OMC = 0x08, //0x08 Output Mode Configuration
    PB_PROT_RSP_PSE = 0x11, //0x11 power supply event
    PB_PROT_RSP_UIE = 0x21, // 0x21 user input event
    PB_PROT_RSP_DSE = 0x22, // 0x22 door status event
    PB_PROT_RSP_PCE = 0x23, //0x23 person counter event
    PB_PROT_RSP_COE = 0x24, //0x24 consumer operation event
    PB_PROT_RSP_SAE = 0x31, //0x31 sensor alarm event
    PB_PROT_RSP_MUE = 0x83, //0x83 Mutilmedia event
    PB_PROT_RSP_RTO = 0x84, //0x84 real time operation
    PB_PROT_RSP_OPO = 0x85, //0x85 offline password order
    PB_PROT_RSP_CFG = 0x90, //0x90 device cfg report
    PB_PROT_RSP_LOC = 0x91, //0x91 location report
    PB_PROT_RSP_GSM = 0x92, //0x92 GSM info report
    PB_PROT_RSP_DBI = 0x93, //0x93 Device basic info report
    /*debug info*/
    PB_PROT_RSP_DBG = 0xE1, //0xE1 Debug info report
    /*firmware over the air*/
    PB_PROT_RSP_FOTA = 0xF1,    //0xF1 Firmware over the air
    PB_PROT_MSG_RSP_END
}PB_PROT_MSG_RSP_TYPE;

#define PB_PROT_MSG_HBP_SUBTYPE 0xFF

/******************************************************************************
* Protocol command data struct
******************************************************************************/
/******************************************************************************
* Access Point Configuration (APC[0x01 0x01])
******************************************************************************/
#define PB_APN_LEN 40
#define PB_APN_USR_LEN 30
#define PB_APN_PASS_LEN 30
#define PB_APN_DNS_LEN 4

typedef struct
{
    uint8 apn[PB_APN_LEN + 1];
    uint8 usr[PB_APN_USR_LEN + 1];
    uint8 pass[PB_APN_PASS_LEN + 1];
    uint8 mainDNS[PB_APN_DNS_LEN];
    uint8 bkDNS[PB_APN_DNS_LEN];
}PB_PROT_CMD_APC_ARG;

/******************************************************************************
* Server Configuration (SER[0x01 0x02])
******************************************************************************/
#define PB_SER_DOMAINNAME_LEN 60
#define PB_SER_SMS_GATEWAY_LEN 20

typedef enum
{
    PB_SER_MODE_DISABLE = 0,
    PB_SER_MODE_SINGLE_SERVER = 1,
    PB_SER_MODE_BACKUP_SERVER = 2
}PB_PROT_SER_MODE_TYPE;

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
}PB_PROT_CMD_SER_ARG;

/******************************************************************************
* Global Configuration (CFG[0x01 0x03])
******************************************************************************/
#define PB_CFG_CONTENT_LEN 4

typedef enum
{
    PB_CFG_EVENT_PNE = 0,
    PB_CFG_EVENT_PFE,
    PB_CFG_EVENT_UIE,
    PB_CFG_EVENT_PSE,
    PB_CFG_EVENT_DSE,
    PB_CFG_EVENT_MUE,
    PB_CFG_EVENT_GEN,
    PB_CFG_EVENT_PCE,
    PB_CFG_EVENT_COE
}PB_PROT_CFG_EVENT_MASK;

typedef struct
{
    uint16 eventMask;
    uint16 infInterval;
}PB_PROT_CMD_CFG_ARG;

/******************************************************************************
* Time Adjust (TMA[0x01 0x04])
******************************************************************************/
#define PB_TMA_CONTENT_LEN 6
#define PB_TMA_SIGN_MAX 1
#define PB_TMA_HOUR_MAX 23
#define PB_TMA_MINUTE_MAX 59
#define PB_TMA_DLS_MAX 1

typedef struct
{
    uint8 mode;
    uint8 sign;
    uint8 hour;
    uint8 minute;
    uint8 daylightSaving;  
    uint32 timestamp;
}PB_PROT_CMD_TMA_ARG;

/******************************************************************************
* Protocol Watchdog (DOG[0x01 0x05])
******************************************************************************/
#define PB_DOG_CONTENT_LEN 5

typedef struct
{
    uint8 mode;
    uint8 report;
    uint8 interval;
    uint8 rstHour;
    uint8 rstMinute;
    uint16 randomDuration;
}PB_PROT_CMD_DOG_ARG;

/******************************************************************************
* Air Conditioner Operation (ACO[0x01 0x06])
******************************************************************************/
#define PB_ACO_CONTENT_LEN 4

typedef enum
{
    PB_ACO_MODE_OFF = 0,
    PB_ACO_MODE_ON,
    PB_ACO_MODE_AUTO
}PB_PROT_ACO_MODE_TYPE;

typedef struct
{
    uint8 pwrMode;
    uint8 workMode;
    uint8 windLevel;
    uint8 interval;
    uint8 duration;
    uint8 temperature;    
}PB_PROT_CMD_ACO_ARG;

/******************************************************************************
* Security Configuration (SEC[0x01 0x07])
******************************************************************************/
#define PB_SEC_CONTENT_LEN 49
#define PB_SEC_KEY_LEN 32

#define PB_SEC_KEY_DATA_LEN 48
#define PB_SEC_KEY_HEADER_OFFSET 0
#define PB_SEC_KEY_OFFSET 4
#define PB_SEC_KEY_TAIL_OFFSET 36
#define PB_SEC_KEY_CRC_OFFSET 40
#define PB_SEC_KEY_HEADER {0xDE, 0xAD, 0xBE, 0xEF}
#define PB_SEC_KEY_TAIL {0xBE, 0xEF, 0xDE, 0xAD}

typedef enum
{
    PB_SEC_HOTP_OFF = 0,
    PB_SEC_HOTP_ON
}PB_PROT_SEC_HOTP_MODE;

typedef enum
{
    PB_SEC_KEY_OFFLINE_NORMAL = 1,
    PB_SEC_KEY_OFFLINE_SERVICE = 2,
    PB_SEC_KEY_AES = 3
}PB_PROT_SEC_KEY_TYPE;

typedef struct
{
    uint8 keyType;
    uint8 key[PB_SEC_KEY_LEN];
}PB_PROT_CMD_SEC_ARG;

/******************************************************************************
* Output Mode Configuration (OMC[0x01 0x08])
******************************************************************************/
#define PB_OMC_CONTENT_LEN 21

typedef enum
{
    PB_OMC_EXHAUST_FORCE_MODE_OFF = 0,
    PB_OMC_EXHAUST_FORCE_MODE1,
    PB_OMC_EXHAUST_FORCE_MODE2
}PB_OMC_EXHAUST_TYPE;

typedef enum
{
    PB_OMC_MODE_NORMAL = 0,
    PB_OMC_MODE_SPECIAL_TIME,
    PB_OMC_MODE_SPECIAL_TIME_WITH_RSP
}PB_OMC_MODE_TYPE;

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
}PB_PROT_CMD_OMC_ARG;

/******************************************************************************
* Air conditioner working config (ACW[0x01 0x09])
******************************************************************************/
#define PB_ACW_CONTENT_LEN 8

typedef enum
{
    PB_ACW_PWR_OFF = 0,
    PB_ACW_PWR_ON
}PB_ACW_PWR_MODE;

typedef enum
{
    PB_ACW_PWRON_EVENT_BEGIN = 0,
    PB_ACW_PWRON_ORDER_BEGIN = PB_ACW_PWRON_EVENT_BEGIN,
    PB_ACW_PWRON_IN_VALIDTIME,
    PB_ACW_PWRON_EVENT_END
}PB_ACW_PWRON_EVENT;

typedef enum
{
    PB_ACW_PWROFF_EVENT_BEGIN = 0,
    PB_ACW_PWROFF_ORDER_END = PB_ACW_PWROFF_EVENT_BEGIN,
    PB_ACW_PWROFF_OUT_VALIDTIME,
    PB_ACW_PWROFF_EVENT_END
}PB_ACW_PWROFF_EVENT;

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
}PB_PROT_CMD_ACW_ARG;

/******************************************************************************
* Output working config (OWC[0x01 0x0A])
******************************************************************************/
#define PB_OWC_CONTENT_LEN 6

typedef enum
{
    PB_OWC_OFF = 0,
    PB_OWC_ON
}PB_OWC_MODE;

typedef enum
{
    PB_OWC_OUTDOOR_TV1 = 0,
    PB_OWC_OUTDOOR_TV2,
    PB_OWC_OUTDOOR_LIGHTBOX,
    PB_OWC_VENDING_MACHINE,
    PB_OWC_RESERVED4,
    PB_OWC_RESERVED5,
    PB_OWC_RESERVED6,
    PB_OWC_RESERVED7,
    PB_OWC_SIZE
}PB_OWC_ITEM_TYPE;

typedef struct
{
    uint8 item;
    uint8 mode;
    uint8 startHour;
    uint8 startMin;
    uint8 stopHour;
    uint8 stopMin;
}PB_PROT_CMD_OWC_ARG;

/******************************************************************************
* Door Alarm (DOA[0x01 0x31])
******************************************************************************/
#define PB_DOA_CONTENT_LEN 4

typedef enum
{
    PB_DOA_OFF = 0,
    PB_DOA_ON,
    PB_DOA_REPEAT_REPORT,
    PB_DOA_REVERSE = 0xF0
}PB_DOA_MODE;

typedef struct
{
    uint8 mode;
    uint8 triggerType;
    uint8 duration;
    uint8 interval;
}PB_PROT_CMD_DOA_ARG;

/******************************************************************************
* Smoke Alarm (SMA[0x01 0x32])
******************************************************************************/
#define PB_SMA_CONTENT_LEN 4

typedef struct
{
    uint8 mode;
    uint8 threshold;
    uint8 duration;
    uint8 interval;
}PB_PROT_CMD_SMA_ARG;

/******************************************************************************
* Order Update Operation (OUO[0x01 0x81])
******************************************************************************/
#define PB_OUO_CONTENT_LEN(n) (17 * (n) + 1)

typedef enum
{
    PB_OUO_ACT_ADD_NORMAL = 0,
    PB_OUO_ACT_REMOVE,
    PB_OUO_ACT_ADD_SERVICE
}PB_PROT_OUO_ACT_TYPE;

typedef struct
{
    uint32 id;
    uint32 startTime;
    uint32 expireTime;
    uint32 passwd;
    uint8 personNum;
    uint8 passwdValidCnt;
}PB_PROT_ORDER_TYPE;

/******************************************************************************
* Output Operation (OUT[0x01 0x82])
******************************************************************************/
#define PB_OUT_CONTENT_LEN 6

typedef struct
{
    uint8 pinIdx;
    uint8 pinState;
    uint32 ctrlMask;
}PB_PROT_CMD_OUT_ARG;

/******************************************************************************
* Mutilmedia Operation (MUO[0x01 0x83])
******************************************************************************/
#define PB_MUO_CONTENT_LEN 2

typedef enum
{
    PB_MUO_SOUND = 1
}PB_PROT_MUO_TYPE;

typedef enum
{
    PB_MUO_ACT_STOP = 0,
    PB_MUO_ACT_PLAY = 1,
    PB_MUO_ACT_VOL_DOWN = 2,
    PB_MUO_ACT_VOL_UP = 3,
    PB_MUO_ACT_SET_VOL = 10,
    PB_MUO_ACT_AUTO_BGM_OFF = 11,
    PB_MUO_ACT_AUTO_BGM_ON = 12
}PB_PROT_MUO_ACT_TYPE;

typedef enum
{
    PB_MUO_FILE_WELCOME = 0,
    PB_MUO_FILE_OVER,
    PB_MUO_FILE_SMOKE_ALARM,
    PB_MUO_FILE_BGM,
    PB_MUO_FILE_ALERT_BAD_PEOPLE,
    PB_MUO_FILE_WARNING_TAKE_UNPAID,
    PB_MUO_FILE_WARNING_BAD_PEOPLE,
    PB_MUO_FILE_WARNING_UNPAID
}PB_PROT_MUO_FILE_TYPE;

typedef struct
{
    uint8 type;
    uint8 act;
    uint8 volume;
    uint8 fileIdx;
}PB_PROT_CMD_MUO_ARG;

/******************************************************************************
* Real Time Operation (RTO[0x01 0x84])
******************************************************************************/
#define PB_RTO_CONTENT_LEN 2

typedef enum
{
    PB_RTO_INF = 0,
    PB_RTO_VER,
    PB_RTO_REBOOT,
    PB_RTO_RESET,
    PB_RTO_DOOR_SW,
    PB_RTO_LOCATION,
    PB_RTO_GSMINFO,
    PB_RTO_DEVICEBOX_SW,
    PB_RTO_DEV_INFO_REQ,
    PB_RTO_DEV_CFG_REQ,
    PB_RTO_DEV_PC_CAL, //10 person counter calibration
    PB_RTO_DOOW_SW_WITHOUT_AUDIO,
    PB_RTO_TV_REBOOT = 100,
    PB_RTO_EXHAUST_FORCE_MODE,  // 101
    PB_RTO_HOTP_SW = 240,
}PB_PROT_RTO_TYPE;

typedef struct
{
    uint8 cmd;
    uint8 subCmd;
}PB_PROT_CMD_RTO_ARG;

/******************************************************************************
* Firmware over the air (FOTA[0x01 0xF1])
******************************************************************************/
#define PB_FOTA_URL_LEN 100
#define PB_FOTA_USR_LEN 40
#define PB_FOTA_PASS_LEN 40
#define PB_FOTA_MD5_LEN 16

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
}PB_PROT_CMD_FOTA_ARG;

typedef enum
{
    PB_FOTA_FTP = 0
}PB_PROT_CMD_FOTA_PROT;

//FOTA RSP[0x03 0xF1]
typedef enum
{
    PB_FOTA_START = 0x10,
    PB_FOTA_MEMLACK = 0x11,
    PB_FOTA_BATLOW = 0x12,
    PB_FOTA_UNKNOW_PROT = 0x13,
    PB_FOTA_DOWNLOAD_OK = 0x20,
    PB_FOTA_DOWNLOAD_ERR = 0x21,
    PB_FOTA_DOWNLOAD_MD5_ERR = 0x22,
    PB_FOTA_DOWNLOAD_VER_ERR = 0x23,
    PB_FOTA_UPGRADE_START = 0x30,
    PB_FOTA_UPGRADE_OK = 0x31,
    PB_FOTA_UPGRADE_ERR = 0x32
}PB_PROT_RSP_FOTA_STATUS_CODE;

typedef struct 
{
    uint8 status;
    uint8 cnt;
}PB_PROT_RSP_FOTA_PARAM;

/******************************************************************************
* Report enums
******************************************************************************/
//Power supply RSP[0x03 0x11]
typedef enum
{
    PB_PWR_ALARM_MAIN_LOST = 0,
    PB_PWR_ALARM_MAIN_RECOVER = 1
}PB_PROT_POWER_ALARM_TYPE;

//User input RSP[0x03 0x21]
#define PB_UIE_DATA_LEN 40

typedef enum
{
    PB_PROT_UIE_KEYBOARD = 0,
    PB_PROT_UIE_HOTP_PASS,
    PB_PROT_UIE_ENG_PASS,
    PB_PROT_UIE_CANCELED_PASS,
    PB_PROT_UIE_UNKNOWN
}PB_PROT_UIE_TYPE;

typedef struct
{
    uint8 type;
    uint8 data[PB_UIE_DATA_LEN+1];
}PB_PROT_RSP_UIE_PARAM;

//Door status RSP[0x03 0x22]
typedef enum
{
    PB_PROT_DSE_UNKNOW = 0,
    PB_PROT_DSE_PASSWD,   //password
    PB_PROT_DSE_SERVER,   //server
    PB_PROT_DSE_EMERGNCY, //emergncy button
    PB_PROT_DSE_SMA,  //smoke alarm
    PB_PROT_DSE_MONITOR,
    PB_PROT_DSE_CLEANER
} PB_PROT_DSE_TYPE;

//Person count event[0x03 0x23]
typedef struct
{
    uint8 method;
    uint8 position;
    uint8 type;
    uint8 curNum;
    uint16 totalNum;
}PB_PROT_RSP_PCE_PARAM;

//Consumer operation event[0x03 0x24]
#define PB_COE_INFO_LEN 255

typedef enum
{
    PB_COE_CLOSE_DOOR = 0,
    PB_COE_OPEN_DOOR
}PB_PROT_COE_TYPE;

typedef enum
{
    PB_COE_INFO_UNKNOWN = 0,
    PB_COE_INFO_ENG_PWD,
    PB_COE_INFO_OFFLINE_PWD,
    PB_COE_INFO_PWD,
    PB_COE_INFO_SERVER_OPEN,
    PB_COE_INFO_ENG_SERVER_OPEN,
    PB_COE_INFO_EMERGENCY_BTN,
    PB_COE_INFO_END
}PB_PROT_COE_INFO_TYPE;

typedef struct
{
    uint8 type;
    uint32 operationID;
    uint32 consumerID;
    uint8 info[PB_COE_INFO_LEN];
}PB_PROT_RSP_COE_PARAM;

//Sensor alamr RSP[0x03 0x31]
typedef enum
{
    PB_SENSOR_ALARM_DOOR = 0,
    PB_SENSOR_ALARM_SMOKE = 1,
    PB_SENSOR_ALARM_PCIR = 2
}PB_PROT_SENSOR_ALARM_TYPE;

typedef struct
{
    uint8 alarmType;
    uint8 alarmLv;
}PB_PROT_RSP_SAE_PARAM;

//Multi-media event RSP[0x03 0x83]
typedef enum
{
    PB_PROT_MUE_MODULE_SOUND = 1
}PB_PROT_MUE_MODULE_TYPE;

//Device location[0x03 0x91]
#define PB_LOC_LONG_LEN 11
#define PB_LOC_LAT_LEN 10

typedef enum
{
     PB_PROT_RSP_LOC_LAST_FIX = 0x11
}PB_PROT_RSP_LOC_FIX_TYPE;

typedef struct
{
    uint16 timestamp;
    uint8 fixType;
    uint8 longitude[PB_LOC_LONG_LEN+1];
    uint8 latitude[PB_LOC_LAT_LEN+1];
}PB_PROT_RSP_LOC_PARAM;

//GSM info RSP[0x03 0x92]
#define PB_GSM_MODULE_LEN 20
#define PB_GSM_IMEI_LEN 15
#define PB_GSM_IMSI_LEN 15
#define PB_GSM_ICCID_LEN 20

typedef struct
{
    uint8 gsmModule[PB_GSM_MODULE_LEN+1];
    uint8 imei[PB_GSM_IMEI_LEN+1];
    uint8 imsi[PB_GSM_IMSI_LEN+1];
    uint8 iccid[PB_GSM_ICCID_LEN+1];
}PB_PROT_RSP_GSMINFO_PARAM;

//Debug info RSP[0x03 0xE1]
#define PB_DBG_MAXSIZE (512 - 4)

//Genernal Server ACK
#define PB_ACK_GEN_CONTENT_LEN 2

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint16 serialNumber;
}PB_PROT_ACK_GEN_ARG;

typedef union
{
    PB_PROT_CMD_APC_ARG apc;
    PB_PROT_CMD_SER_ARG ser;
    PB_PROT_CMD_CFG_ARG cfg;
    PB_PROT_CMD_TMA_ARG tma;
    PB_PROT_CMD_DOG_ARG dog;
    PB_PROT_CMD_ACO_ARG aco;
    PB_PROT_CMD_SEC_ARG sec;
    PB_PROT_CMD_OMC_ARG omc;
    PB_PROT_CMD_ACW_ARG acw;
    PB_PROT_CMD_OWC_ARG owc;
    PB_PROT_CMD_DOA_ARG doa;
    PB_PROT_CMD_SMA_ARG sma;
    PB_PROT_CMD_OUT_ARG out;
    PB_PROT_CMD_MUO_ARG muo;
    PB_PROT_CMD_RTO_ARG rto;
    PB_PROT_CMD_FOTA_ARG fota;
    PB_PROT_ACK_GEN_ARG genAck;
}PB_PROT_CMD_ARG_UNION;

typedef struct
{
    PB_PROT_SRC_ID srcID;
    PB_PROT_TYPE msgType;
    PB_PROT_CMD_TYPE msgSubType;
    PB_PROT_CMD_ARG_UNION arg;
    uint16 serialNumber;
}PB_PROT_CMD_PARSED_FRAME_TYPE;

typedef struct 
{
    PB_PROT_SRC_ID srcID;
    uint8 *rawData;
    uint16 rawLength;
}PB_PROT_RAW_PACKET_TYPE;

typedef struct
{
    PB_PROT_SRC_ID srcID;
    uint16 length;
    uint8 *data;
}PB_PROT_ACK_PACK_TYPE;

typedef PB_PROT_ACK_PACK_TYPE PB_PROT_HBP_PACK_TYPE;

typedef struct
{
    PB_PROT_SRC_ID srcID;
    uint8 msgSubType;
    void *msgParam;
    uint16 length;
    uint8 *data;
}PB_PROT_RSP_PACK_TYPE;

typedef struct
{
    uint8 subType;
    uint8 param[1];
}PB_PROT_RSP_TYPE;

#endif /* __PB_PROT_TYPE_H__ */
