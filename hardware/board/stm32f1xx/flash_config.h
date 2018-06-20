/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          flash_config.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   flash partition
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-12      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "stm32f10x.h"
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define BOARD_FLASH_BASE FLASH_BASE
#define BOARD_FLASH_SIZE 0x80000
#define BOARD_FLASH_SECTOR_SIZE 0x800 //bytes
#define BOARD_BL_OFFSET 0x0000
#define BOARD_APP_OFFSET 0x4000

/******************************************************************************
* Flash partition (512KB) 0x08000000 ~ 0x08080000
*
* Bootloader (16KB) 0x08000000 ~ 0x08004000
* APP (160KB) 0x08004000 ~ 0x0802C000
* FIRMWARE_A (160KB) 0x0802C000 ~ 0x08054000
* FIRMWARE_B (160KB) 0x08054000 ~ 0x0807C000
* FIRMWARE_CFG (4KB) 0x0807C000 ~ 0x0807D000
* RESERVED (6KB) 0x0807D000 ~ 0x0807E800
* DEVINFO (2KB) 0x0807E800 ~ 0x0807F000
* CMD_CONFIG (4KB) 0x0807F000 ~ 0x08080000
*
******************************************************************************/
/*Bootloader*/
#define BL_BEGIN        ((uint32)0x08000000)
#define BL_END            ((uint32)0x08004000)

/*Application*/
#define APP_BEGIN      ((uint32)0x08004000)
#define APP_END          ((uint32)0x0802C000)

/*Firmware buffer(FOTA)*/
#define FIRMWARE_A_BEGIN    ((uint32)0x0802C000)
#define FIRMWARE_A_END       ((uint32)0x08054000)
#define FIRMWARE_B_BEGIN    ((uint32)0x08054000)
#define FIRMWARE_B_END       ((uint32)0x0807C000)

/*Firmware config*/
#define FIRMWARE_CFG_BEGIN ((uint32)0x0807C000)
#define FIRMWARE_CFG_END    ((uint32)0x0807D000)
#define FIRMWARE_CFG_BLOCK_SIZZE ((uint32)0x800)
#define FIRMWARE_IMAGE_INFO_OFFSET ((uint32)0x1000)
#define FWCFG_SECTOR1 (FIRMWARE_CFG_BEGIN)
#define FWCFG_SECTOR2 (FIRMWARE_CFG_BEGIN + FIRMWARE_CFG_BLOCK_SIZZE)

/*Device info*/
#define DEVINFO_BEGIN           ((uint32)0x0807E800)
#define DEVINFO_END              ((uint32)0x0807F000)

/*Device config*/
#define CMDCFG_BEGIN            ((uint32)0x0807F000)
#define CMDCFG_END               ((uint32)0x08080000)

#endif /* __FLASH_CONFIG_H__ */

