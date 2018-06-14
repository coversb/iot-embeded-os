/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_bl_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   parkbox bootloader main
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include "hal_board.h"
#include "hal_wdg.h"
#include "os_trace_log.h"
#include "pb_bl_config.h"
#include "pb_bl_upgrade.h"

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
typedef void (*pFunction)(void);

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_bl_print_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : print bootloader info
******************************************************************************/
static void pb_bl_print_info(void)
{
    uint16 hwVer = PB_HW_VERSION;
    uint16 fmVer = PB_BL_VERSION;
    
    BL_INFO("");
    BL_INFO("================================================");
    BL_INFO("=          (C) COPYRIGHT 2018 ParkBox          =");
    BL_INFO("=                                              =");
    BL_INFO("=                 HW_Ver:%02d.%02d                 =", (hwVer >> 8), (hwVer & 0x00FF));
    BL_INFO("=                                              =");
    BL_INFO("=    Bootloader with IAP (version %02d.%02d.%02d)    =", (fmVer >> 12), ((fmVer >> 4) & 0xFF), (fmVer & 0x000F));
    BL_INFO("=                     @%s-%s    =", __DATE__, __TIME__);
    BL_INFO("=                          By Embedded Team    =");
    BL_INFO("================================================");
    BL_INFO("");
}

/******************************************************************************
* Function    : hardware_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void hardware_init()
{
    hal_board_init();
    PB_DEBUG_COM.begin(115200);

    #if defined(PB_BOOTLOADER_DBG)
    PB_BL_DEBUG_COM.begin(115200);
    #endif /*PB_BOOTLOADER_DBG*/
    
    hal_wdg_init();
    PB_FLASH_HDLR.init();
}

/******************************************************************************
* Function    : pb_bl_jump_app
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_bl_jump_app(void)
{
    uint32 ramAddr, appAddr;
    register pFunction appJumper;

    ramAddr = *(__IO uint32*)APP_BEGIN;
    appAddr = *(__IO uint32*)(APP_BEGIN + 4);
    
    if ((ramAddr & 0xFFFF0000) != 0x20000000)
    {
        BL_INFO("Bad ram addr %X", ramAddr);
        return false;
    }
    if ((appAddr & 0xFFF00000) != 0x08000000)
    {
        BL_INFO("Bad app addr %X", appAddr);
        return false;
    }

    SysTick->CTRL &= 0xFFFFFFFD;
    /* Jump to user application */
    appJumper = (pFunction)appAddr;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*)APP_BEGIN); //top of main stack
    
    appJumper();
    
    return true;
}

void bootloader_main(void *pvParameters)
{
    //upgrade by uart
    if (pb_bl_upgrade_by_uart())
    {
        hal_board_reset();
    }

    pb_bl_print_info();    

    pb_bl_jump_app();

    BL_INFO("BAD APP BIN");
    //should not goto here
    while (1)
    {
        hal_wdg_feed();
        if (pb_bl_upgrade_by_uart())
        {
            hal_board_reset();
        }
    }
}

/******************************************************************************
* Function    : main
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
int main(void)
{
    hardware_init();

    os_task_create(bootloader_main, NULL, "BL", (OS_STACK_1K*2), (tskIDLE_PRIORITY+1), NULL);
    os_task_scheduler();

    return 0;
}

