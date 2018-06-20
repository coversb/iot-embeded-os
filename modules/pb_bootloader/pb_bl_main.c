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
#include "pb_bl_dbg.h"
#include "pb_bl_config.h"
#include "pb_bl_upgrade.h"
#include "pb_firmware_manage.h"

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
    uint16 hwVer = BOARD_HW_VERSION;
    uint16 fmVer = PB_BL_VERSION;

    PB_IMAGE_INFO imageInfo;
    if (!fwManager.imageInfo(PB_IMAGE_BL, &imageInfo))
    {
        BL_INFO("image info err %x, %x, %x", imageInfo.imageType, imageInfo.hwVersion, imageInfo.imageVersion);
    }
    else
    {
        hwVer = imageInfo.hwVersion;
        fmVer = imageInfo.imageVersion;
    }
    
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
* Description :init hardware
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

    fwManager.load();
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
* Description : jump to app
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

    PB_FIRMWARE_CONTEXT *pFwContext = fwManager.firmwareContext();
    BL_INFO("Run IMG[%d], BOOT[%d], FAIL[%d], ERASE[%d]\r\n", 
                 pFwContext->curImage, pFwContext->runnableBootTimes, pFwContext->runnableWorking,
                 pFwContext->eraseTimes);

    SysTick->CTRL &= 0xFFFFFFFD;
    /* Jump to user application */
    appJumper = (pFunction)appAddr;
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*)APP_BEGIN); //top of main stack

    appJumper();

    return true;
}

/******************************************************************************
* Function    : pb_bl_need_recover
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check if need recover app
******************************************************************************/
static bool pb_bl_need_recover(void)
{
    if (FIRMWARE_BOOTFAIL < fwManager.firmwareContext()->runnableWorking)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* Function    : pb_bl_try_to_reocver
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : try to recover firmware
******************************************************************************/
static bool pb_bl_try_to_reocver(void)
{
    if (pb_bl_recover())
    {
        fwManager.updateRunnable(PB_FIRMWARE_RECOVER);
        hal_board_reset();

        return true;
    }

    return false;
}

/******************************************************************************
* Function    : bootloader_main
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : bootloader main process
******************************************************************************/
void bootloader_main(void *pvParameters)
{
    //upgrade via uart
    if (pb_bl_upgrade_by_uart())
    {
        fwManager.submit(NULL);
        hal_board_reset();
    }
    //print bl info via usart
    pb_bl_print_info();

    //check fota state
    if (pb_bl_upgrade_by_fota())
    {
        fwManager.updateRunnable(PB_FIRMWARE_FOTA);
    }

    fwManager.updateRunnable(PB_FIRMWARE_NORMAL);

    //check recover image
    if (pb_bl_need_recover())
    {
        BL_INFO("FAIL[%d], RECOVER", fwManager.firmwareContext()->runnableWorking);
        pb_bl_try_to_reocver();
    }
    else
    {
        //jump to app
        if (!pb_bl_jump_app())
        {
            BL_INFO("BOOT FAIL, RECOVER");
            pb_bl_try_to_reocver();
        }

        BL_INFO("BAD APP BIN");
    }

    //should not goto here
    while (1)
    {
        hal_wdg_feed();
        if (pb_bl_upgrade_by_uart())
        {
            fwManager.submit(NULL);
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

