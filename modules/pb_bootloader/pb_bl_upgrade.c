/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_bl_upgrade.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   bl upgrade functions
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
#include "os_trace_log.h"
#include "hal_wdg.h"
#include "pb_bl_upgrade.h"
#include "ymodem.h"
#include "pb_firmware_manage.h"
#include "pb_bl_dbg.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_bl_upgrade_by_uart
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_bl_upgrade_by_uart(void)
{
    if (YMODEM_OK == ymodem_process())
    {
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_bl_upgrade_by_fota
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_bl_upgrade_by_fota(void)
{
    uint8 retryCnt = 0;
    uint32 imageBeginAddr = 0;
    uint32 imageSize = 0;
    PB_FIRMWARE_CONTEXT *pFwContext = fwManager.firmwareContext();

    //commit image by app
    if (pFwContext->runnableBootTimes == 0)
    {
        imageBeginAddr = pFwContext->images[pFwContext->curImage].romBegin;
        imageSize = pFwContext->images[pFwContext->curImage].length;

    retry:
        BL_INFO("FOTA...");

        if (++retryCnt > FIRMWARE_UPGRADE_RETRY)
        {
            goto end;
        }

        //feed watchdog
        hal_wdg_feed();
        if (!PB_FLASH_HDLR.erasePages(APP_BEGIN, APP_END - APP_BEGIN))
        {
            BL_INFO("app erase err");
            goto retry;
        }

        //feed watchdog
        hal_wdg_feed();
        if (PB_FLASH_HDLR.forceWrite(APP_BEGIN, (uint32*)imageBeginAddr, imageSize) != 0)
        {
            BL_INFO("app write err");
            goto retry;
        }
        return true;
    }
    
end:
    return false;
}

/******************************************************************************
* Function    : pb_bl_recover
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_bl_recover(void)
{
    uint8 curImage = 0;
    uint8 retryCnt = 0;
    uint32 imageBeginAddr = 0;
    uint32 imageSize = 0;
    PB_FIRMWARE_CONTEXT *pFwContext = fwManager.firmwareContext();

    //current is upgraded by fota, disable it
    if (pFwContext->curImage < PB_FIRMWARE_NUM)
    {
        curImage = pFwContext->curImage;

        pFwContext->images[curImage].length = 0xFFFFFFFF;
        pFwContext->images[curImage].crc = 0xFFFFFFFF;
        pFwContext->images[curImage].upgradeTimestamp = 0xFFFFFFFF;
        pFwContext->images[curImage].runTimes = 0xFFFF;
        pFwContext->images[curImage].version = 0xFFFF;
    }
    
    /*find available firmware in image buffer*/
    //both invalid
    if (DEFAULT_VALUE == pFwContext->images[PB_FIRMWARE_A].length
        && DEFAULT_VALUE == pFwContext->images[PB_FIRMWARE_B].length)
    {
        BL_INFO("NO VALID IMAGE");
        goto end;
    }
    //both valid, chose higher version
    if (DEFAULT_VALUE != pFwContext->images[PB_FIRMWARE_A].length
        && DEFAULT_VALUE != pFwContext->images[PB_FIRMWARE_B].length)
    {
        if (pFwContext->images[PB_FIRMWARE_A].version > pFwContext->images[PB_FIRMWARE_B].version)
        {
            pFwContext->curImage = PB_FIRMWARE_A;
        }
        else
        {
            pFwContext->curImage = PB_FIRMWARE_B;
        }
    }
    //have one valid, find it
    else
    {
        if (DEFAULT_VALUE != pFwContext->images[PB_FIRMWARE_A].length)
        {
            pFwContext->curImage = PB_FIRMWARE_A;
        }
        else
        if (DEFAULT_VALUE != pFwContext->images[PB_FIRMWARE_B].length)
        {
            pFwContext->curImage = PB_FIRMWARE_B;
        }
    }

    imageBeginAddr = pFwContext->images[pFwContext->curImage].romBegin;
    imageSize = pFwContext->images[pFwContext->curImage].length;

retry:
    BL_INFO("RECOVER...");

    if (++retryCnt > FIRMWARE_UPGRADE_RETRY)
    {
        goto end;
    }

    //feed watchdog
    hal_wdg_feed();
    if (!PB_FLASH_HDLR.erasePages(APP_BEGIN, APP_END - APP_BEGIN))
    {
        BL_INFO("app erase err");
        goto retry;
    }

    //feed watchdog
    hal_wdg_feed();
    if (PB_FLASH_HDLR.forceWrite(APP_BEGIN, (uint32*)imageBeginAddr, imageSize) != 0)
    {
        BL_INFO("app write err");
        goto retry;
    }
    return true;
    
end:
    return false;
}

