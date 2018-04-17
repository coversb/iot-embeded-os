/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_app_config.h 
*  author:              Chen Hao
*  version:             1.00
*  file description:   global defination
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_APP_CONFIG_H__
#define __PB_APP_CONFIG_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "board_config.h"
#include "os_config.h"
#include "os_middleware.h"
#include "hal_flash.h"

/******************************************************************************
* Macros
******************************************************************************/
/*Flash partition*/
#define PB_DEVINFO_ADDR (BOARD_FLASH_BASE + 0x7E800)
#define PB_CMDCFG_ADDR (BOARD_FLASH_BASE + 0x7F000)

/*Flash handler*/
#define PB_FLASH_HDLR hwFlash

/*Debug com*/
#define PB_DEBUG_COM OS_TRACE_COM

/*Versions*/
#define PB_PROTOCOL_VERSION 0x0116  //1.22
#define PB_FIRMWARE_VERSION 0x2000  //2.00.00
#define PB_BOOTLOADER_VERSION 0x1040  //1.04.00

/*TASK INFO DEFINE begin*/
//pb_prot_main task
#define PB_PROT_STACK (OS_STACK_1K*4)   //bytes
#define PB_PROT_NAME "pb_prot"
#define PB_PROT_PRIO (tskIDLE_PRIORITY+3)
#define PB_PROT_MSGQUE_SIZE 15

//pb_input_monitor_main task
#define PB_IO_MONITOR_STACK (OS_STACK_1K*2)   //bytes
#define PB_IO_MONITOR_NAME "pb_input_monitor"
#define PB_IO_MONITOR_PRIO (tskIDLE_PRIORITY+2)
#define PB_IO_MONITOR_POLL_INTERVAL (DELAY_1_MS*10)
#define PB_IO_MONITOR_EVENT_INTERVAL DELAY_1_S

//monitor task
#define PB_MONITOR_STACK (OS_STACK_1K*2)   //bytes
#define PB_MONITOR_NAME "pb_monitor"
#define PB_MONITOR_PRIO (tskIDLE_PRIORITY)
/*TASK INFO DEFINE end*/

#define PB_PROT_NETWORK_BYTE_ORDER  //big endian

#endif /* __PB_APP_CONFIG_H__ */

