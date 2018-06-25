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
#include "stm32f4xx.h"
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#define BOARD_FLASH_BASE FLASH_BASE
#define BOARD_FLASH_SIZE 0x200000
#define BOARD_BL_OFFSET 0x0000
#define BOARD_APP_OFFSET 0x20000

#define BOADR_FLASH_SEC0     ((uint32)0x08000000) /* Base address of Sector 0, 16 Kbytes   */
#define BOADR_FLASH_SEC1     ((uint32)0x08004000) /* Base address of Sector 1, 16 Kbytes   */
#define BOADR_FLASH_SEC2     ((uint32)0x08008000) /* Base address of Sector 2, 16 Kbytes   */
#define BOADR_FLASH_SEC3     ((uint32)0x0800C000) /* Base address of Sector 3, 16 Kbytes   */
#define BOADR_FLASH_SEC4     ((uint32)0x08010000) /* Base address of Sector 4, 64 Kbytes   */
#define BOADR_FLASH_SEC5     ((uint32)0x08020000) /* Base address of Sector 5, 128 Kbytes  */
#define BOADR_FLASH_SEC6     ((uint32)0x08040000) /* Base address of Sector 6, 128 Kbytes  */
#define BOADR_FLASH_SEC7     ((uint32)0x08060000) /* Base address of Sector 7, 128 Kbytes  */
#define BOADR_FLASH_SEC8     ((uint32)0x08080000) /* Base address of Sector 8, 128 Kbytes  */
#define BOADR_FLASH_SEC9     ((uint32)0x080A0000) /* Base address of Sector 9, 128 Kbytes  */
#define BOADR_FLASH_SEC10   ((uint32)0x080C0000) /* Base address of Sector 10, 128 Kbytes */
#define BOADR_FLASH_SEC11   ((uint32)0x080E0000) /* Base address of Sector 11, 128 Kbytes */
#define BOADR_FLASH_SEC12   ((uint32)0x08100000) /* Base address of Sector 12, 16 Kbytes  */
#define BOADR_FLASH_SEC13   ((uint32)0x08104000) /* Base address of Sector 13, 16 Kbytes  */
#define BOADR_FLASH_SEC14   ((uint32)0x08108000) /* Base address of Sector 14, 16 Kbytes  */
#define BOADR_FLASH_SEC15   ((uint32)0x0810C000) /* Base address of Sector 15, 16 Kbytes  */
#define BOADR_FLASH_SEC16   ((uint32)0x08110000) /* Base address of Sector 16, 64 Kbytes  */
#define BOADR_FLASH_SEC17   ((uint32)0x08120000) /* Base address of Sector 17, 128 Kbytes */
#define BOADR_FLASH_SEC18   ((uint32)0x08140000) /* Base address of Sector 18, 128 Kbytes */
#define BOADR_FLASH_SEC19   ((uint32)0x08160000) /* Base address of Sector 19, 128 Kbytes */
#define BOADR_FLASH_SEC20   ((uint32)0x08180000) /* Base address of Sector 20, 128 Kbytes */
#define BOADR_FLASH_SEC21   ((uint32)0x081A0000) /* Base address of Sector 21, 128 Kbytes */
#define BOADR_FLASH_SEC22   ((uint32)0x081C0000) /* Base address of Sector 22, 128 Kbytes */
#define BOADR_FLASH_SEC23   ((uint32)0x081E0000) /* Base address of Sector 23, 128 Kbytes */

/******************************************************************************
* Flash partition (2MB) 0x08000000 ~ 0x08200000
*
* Bootloader (64KB)        0x08000000 ~ 0x08010000 (S0 - S3)
* Reserved (64KB)          0x08010000 ~ 0x08020000 (S4)
* APP (384KB)              0x08020000 ~ 0x08080000 (S5 - S7)
* Reserved (512KB)         0x08080000 ~ 0x08100000 (S8 - S11)
* FIRMWARE_CFG (32KB)      0x08100000 ~ 0x08108000 (S12 - S13)
* DEVINFO (16KB)           0x08108000 ~ 0x0810C000 (S14)
* CMD_CONFIG (16KB)        0x0810C000 ~ 0x08110000 (S15)
* Reserved (192KB)         0x08110000 ~ 0x08140000 (S16 - S17)
* FIRMWARE_A (384KB)       0x08140000 ~ 0x081A0000 (S18 - S20)
* FIRMWARE_B (384KB)       0x081A0000 ~ 0x08200000 (S21 - S23)
******************************************************************************/
/*Bootloader*/
#define BL_BEGIN        ((uint32)0x08000000)
#define BL_END            ((uint32)0x08010000)

/*Application*/
#define APP_BEGIN      ((uint32)0x08020000)
#define APP_END          ((uint32)0x08080000)

/*Firmware buffer(FOTA)*/
#define FIRMWARE_A_BEGIN    ((uint32)0x08140000)
#define FIRMWARE_A_END       ((uint32)0x081A0000)
#define FIRMWARE_B_BEGIN    ((uint32)0x081A0000)
#define FIRMWARE_B_END       ((uint32)0x08200000)

/*Firmware config*/
#define FIRMWARE_CFG_BEGIN ((uint32)0x08100000)
#define FIRMWARE_CFG_END    ((uint32)0x08108000)
#define FIRMWARE_CFG_BLOCK_SIZZE ((uint32)0x4000)
#define FIRMWARE_IMAGE_INFO_OFFSET ((uint32)0x1000)
#define FWCFG_SECTOR1 (FIRMWARE_CFG_BEGIN)
#define FWCFG_SECTOR2 (FIRMWARE_CFG_BEGIN + FIRMWARE_CFG_BLOCK_SIZZE)

/*Device info*/
#define DEVINFO_BEGIN           ((uint32)0x08108000)
#define DEVINFO_END              ((uint32)0x0810C000)

/*Device config*/
#define CMDCFG_BEGIN            ((uint32)0x0810C000)
#define CMDCFG_END               ((uint32)0x08110000)

#endif /* __FLASH_CONFIG_H__ */

