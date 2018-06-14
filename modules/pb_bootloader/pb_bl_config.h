/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_bl_config.h 
*  author:              Chen Hao
*  version:             1.00
*  file description:   bootloader config
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-12      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_BL_CONFIG_H__
#define __PB_BL_CONFIG_H__
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
/*Versions*/
#define PB_HW_VERSION 0x0101 //01.01
#define PB_BL_VERSION 0x2000  //2.00.00

/******************************************************************************
* Device define
******************************************************************************/
/*Flash handler*/
#define PB_FLASH_HDLR hwFlash

#if defined(PB_BOOTLOADER_DBG)
#define PB_BL_DEBUG_COM hwSerial5
#endif /*PB_BOOTLOADER_DBG*/
/*Debug com*/
#define PB_DEBUG_COM OS_TRACE_COM
#define PB_UPGRADE_COM OS_TRACE_COM

/*OLED I2C*/
//#define OLED_I2C swI2C1

#endif /* __PB_BL_CONFIG_H__ */

