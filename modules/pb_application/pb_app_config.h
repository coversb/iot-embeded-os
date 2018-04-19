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
#include "hal_usart.h"

/******************************************************************************
* Macros
******************************************************************************/
/*Flash partition*/
#define PB_DEVINFO_ADDR (BOARD_FLASH_BASE + 0x7E800)
#define PB_CMDCFG_ADDR (BOARD_FLASH_BASE + 0x7F000)

/*Versions*/
#define PB_PROTOCOL_VERSION 0x0116  //1.22
#define PB_FIRMWARE_VERSION 0x2000  //2.00.00
#define PB_BOOTLOADER_VERSION 0x1040  //1.04.00

/******************************************************************************
* Device define
******************************************************************************/
/*Flash handler*/
#define PB_FLASH_HDLR hwFlash

/*Debug com*/
#define PB_DEBUG_COM OS_TRACE_COM

/*Password keyboard*/
#define PB_KEYBOARD hwSerial5

/*Tasks marco define*/
#define PB_IO_MONITOR_INTERVAL (DELAY_1_MS*10)

/*Application features*/
#define PB_PROT_DBG 0
#define PB_PROT_NETWORK_BYTE_ORDER  1 //big endian
#define PB_DATA_TIME_OFFSET (8 * 3600) //beijing +8
#define PB_TMA_AUTO_SET_RANGE 30 //seconds
#define PB_TMA_AUTO_SET_DEBOUNCE 3 //count

#endif /* __PB_APP_CONFIG_H__ */

