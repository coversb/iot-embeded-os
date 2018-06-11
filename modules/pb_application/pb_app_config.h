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
#include "hal_sw_i2c.h"
#include "hal_tim.h"
#include "ac_control.h"
#include "kt603.h"

/******************************************************************************
* Macros
******************************************************************************/
/*Flash partition*/
#define PB_DEVINFO_ADDR (BOARD_FLASH_BASE + 0x7E800)
#define PB_CMDCFG_ADDR (BOARD_FLASH_BASE + 0x7F000)

/*Versions*/
#define PB_PROTOCOL_VERSION 0x0117  //1.23
#define PB_FIRMWARE_VERSION 0x2000  //2.00.00
#define PB_BOOTLOADER_VERSION 0x1040  //1.04.00

/******************************************************************************
* Device define
******************************************************************************/
/*Flash handler*/
#define PB_FLASH_HDLR hwFlash

/*Debug com*/
#define PB_DEBUG_COM OS_TRACE_COM

/*2G Modem com*/
#define MODEM_2G_COM hwSerial3
#define MODEM_2G_COM_BAUDRATE 115200

/*ETH Module spi*/
#define ETH_COM hwSPI2

/*OLED I2C*/
#define OLED_I2C swI2C1 

/*Password keyboard*/
#define PB_KEYBOARD hwSerial5

/*Tasks marco define*/
#define PB_IO_MONITOR_INTERVAL (DELAY_1_MS*10)

/*Application features*/
#define PB_DATA_TIME_OFFSET (8 * 3600) //beijing +8

//rgb box task
#define RGBBOX_PWM tim2

//ac control
#define AC_REMOTE_CTRL acCtrl

//unit test
#define PB_UNIT_TEST 0

//autdio 
#define AUDIO_COM hwSerial2
#define AUDIO_COM_BAUDRATE 9600
#define AUDIO_MIN_VOL KT603_MIN_VOL
#define AUDIO_MAX_VOL KT603_MAX_VOL

//pb prot
#define PB_PROT_DBG 0
#define PB_PROT_NETWORK_BYTE_ORDER  1 //big endian
#define PB_PROT_AES 1

#define PB_TMA_AUTO_SET_RANGE 30 //seconds
#define PB_TMA_AUTO_SET_DEBOUNCE 3 //count

//pb order
#define PB_ORDER_START_ADJUST 300   // seconds
#define PB_ORDER_OFFLINE_PW_NUM 30
#define PB_ORDER_ENG_PW_NUM 5
#define PB_ORDER_PW_LEN 4
#define PB_ORDER_ENG_PW_LEN 8

//pb ota
#define PB_OTA_DBG 0
#define PB_OTA_NET_SWITCH_RETRY 2
#define PB_OTA_SERVER_SWITCH_RETRY 3

#define PB_OTA_SEND_MAX_RETRY 10
#define PB_OTA_SEND_RETRY_EXCEED 50

#endif /* __PB_APP_CONFIG_H__ */

