/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_firmware_manage.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   firmware info manage
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-14      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include "os_middleware.h"
#include "os_trace_log.h"
#include "pb_firmware_manage.h"
#if defined(PB_BOOTLOADER)
#include "pb_bl_config.h"
#elif defined(PB_APPLICATION)
#include "pb_app_config.h"
#endif

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Image info defined
******************************************************************************/
#if defined(PB_BOOTLOADER)
const PB_IMAGE_INFO IMAGE_INFO __attribute__((at(BL_BEGIN|FIRMWARE_IMAGE_INFO_OFFSET))) =
{
    PB_IMAGE_BL,
    BOARD_HW_VERSION,
    PB_BL_VERSION
};
#elif defined(PB_APPLICATION)
const PB_IMAGE_INFO IMAGE_INFO __attribute__((at(APP_BEGIN|FIRMWARE_IMAGE_INFO_OFFSET))) =
{
    PB_IMAGE_APP,
    BOARD_HW_VERSION,
    PB_FIRMWARE_VERSION
};
#endif

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_FIRMWARE_CONTEXT pbFirmwareContext;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_firmware_init_context
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init context in flash
******************************************************************************/
static void pb_firmware_init_context(void)
{
    memset(&pbFirmwareContext, 0, sizeof(PB_FIRMWARE_CONTEXT));
    //need init block
    pbFirmwareContext.magicKey = MAGICKEY;
    pbFirmwareContext.eraseTimes = 1;
    pbFirmwareContext.curImage = PB_FIRMWARE_UART;
    pbFirmwareContext.runnableWorking = 1;
    pbFirmwareContext.runnableBootTimes = 1;
    //firmware A
    pbFirmwareContext.images[PB_FIRMWARE_A].length = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_A].crc = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_A].upgradeTimestamp = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_A].runTimes = 0xFFFF;
    pbFirmwareContext.images[PB_FIRMWARE_A].version = 0xFFFF;
    pbFirmwareContext.images[PB_FIRMWARE_A].romBegin = FIRMWARE_A_BEGIN;
    pbFirmwareContext.images[PB_FIRMWARE_A].romEnd = FIRMWARE_A_END;
    //firmware B
    pbFirmwareContext.images[PB_FIRMWARE_B].length = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_B].crc = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_B].upgradeTimestamp = DEFAULT_VALUE;
    pbFirmwareContext.images[PB_FIRMWARE_B].runTimes = 0xFFFF;
    pbFirmwareContext.images[PB_FIRMWARE_B].version = 0xFFFF;
    pbFirmwareContext.images[PB_FIRMWARE_B].romBegin = FIRMWARE_B_BEGIN;
    pbFirmwareContext.images[PB_FIRMWARE_B].romEnd = FIRMWARE_B_END;

    if (!PB_FLASH_HDLR.erasePages(FIRMWARE_CFG_BEGIN, FIRMWARE_CFG_END - FIRMWARE_CFG_BEGIN))
    {
        BL_INFO("FW cfg erase err");
        goto end;
    }
    if (PB_FLASH_HDLR.forceWrite(FIRMWARE_CFG_BEGIN, (uint32*)&pbFirmwareContext, sizeof(PB_FIRMWARE_CONTEXT)) != 0)
    {
        BL_INFO("FW cfg init write err");
        goto end;
    }
    BL_INFO("FW cfg init ok");

end:
    return;
}

/******************************************************************************
* Function    : pb_firmware_has_context
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : check has valued context in flash
******************************************************************************/
static bool pb_firmware_has_context(uint32 *addr)
{
    PB_FIRMWARE_CONTEXT *pFw = NULL;

    (*addr) = FIRMWARE_CFG_BEGIN;
    //find first used block
    do
    {
        pFw = (PB_FIRMWARE_CONTEXT*)(*addr);
        if (pFw->magicKey == MAGICKEY)
        {
            break;
        }
        (*addr) += sizeof(PB_FIRMWARE_CONTEXT);
    } while ((*addr) < FIRMWARE_CFG_END);

    return (bool)((*addr) < FIRMWARE_CFG_END);
}

/******************************************************************************
* Function    : pb_firmware_context_load
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : load firm context from flash
******************************************************************************/
static void pb_firmware_context_load(void)
{
    PB_FIRMWARE_CONTEXT *pFw = NULL;
    uint32 fwContextAddr = 0;

    if (pb_firmware_has_context(&fwContextAddr))
    {
        //find first not used block
        do
        {
            pFw = (PB_FIRMWARE_CONTEXT*)fwContextAddr;
            if (pFw->magicKey == DEFAULT_VALUE)
            {
                break;
            }
            fwContextAddr += sizeof(PB_FIRMWARE_CONTEXT);
        } while (fwContextAddr < FIRMWARE_CFG_END);

        fwContextAddr = fwContextAddr - sizeof(PB_FIRMWARE_CONTEXT);
        memcpy(&pbFirmwareContext, (void*)fwContextAddr, sizeof(PB_FIRMWARE_CONTEXT));
    }
    else
    {
        pb_firmware_init_context();
    }

    return;
}

/******************************************************************************
* Function    : pb_firmware_context_update
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : update firmware context to flash
******************************************************************************/
static void pb_firmware_context_update(void)
{
    PB_FIRMWARE_CONTEXT *pFw = NULL;
    uint32 fwContextAddr = 0;

    //find first used
    pb_firmware_has_context(&fwContextAddr);

    //find first free block after used
    do
    {
        pFw = (PB_FIRMWARE_CONTEXT*)fwContextAddr;
        if (pFw->magicKey == DEFAULT_VALUE)
        {
            break;
        }
        fwContextAddr += sizeof(PB_FIRMWARE_CONTEXT);
    } while (fwContextAddr < FIRMWARE_CFG_END);

    if (fwContextAddr >= FIRMWARE_CFG_END)
    {
        fwContextAddr = FIRMWARE_CFG_BEGIN;
    }

    if ((fwContextAddr == FWCFG_SECTOR1)
        || (fwContextAddr == FWCFG_SECTOR2))
    {
        pbFirmwareContext.eraseTimes += 1;
    }

    if (PB_FLASH_HDLR.forceWrite(fwContextAddr, (uint32*)&pbFirmwareContext, sizeof(PB_FIRMWARE_CONTEXT)) != 0)
    {
        BL_INFO("FW cfg update err");
        goto end;
    }

    //check firmware config cfg status
    switch (fwContextAddr)
    {
        case FWCFG_SECTOR1:
        {
            if (!PB_FLASH_HDLR.erasePages(FWCFG_SECTOR2, FIRMWARE_CFG_BLOCK_SIZZE))
            {
                BL_INFO("fwCfgSector2 erase err");
                goto end;
            }
            break;
        }
        case FWCFG_SECTOR2:
        {
            if (!PB_FLASH_HDLR.erasePages(FWCFG_SECTOR1, FIRMWARE_CFG_BLOCK_SIZZE))
            {
                BL_INFO("fwCfgSector1 erase err");
                goto end;
            }
            break;
        }
        default:break;
    }

end:
    return;
}

/******************************************************************************
* Function    : pb_firmware_manage_get_context
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get firmware context pointer
******************************************************************************/
static PB_FIRMWARE_CONTEXT* pb_firmware_manage_get_context(void)
{
    return &pbFirmwareContext;
}

/******************************************************************************
* Function    : pb_firmware_manage_get_image_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get image info from bin
******************************************************************************/
static bool pb_firmware_manage_get_image_info(uint8 type, PB_IMAGE_INFO *info)
{
    bool ret = false;
    PB_IMAGE_INFO *pInfo = NULL;
    
    switch (type)
    {
        case PB_IMAGE_BL:
        {
            pInfo = (PB_IMAGE_INFO*)(BL_BEGIN|FIRMWARE_IMAGE_INFO_OFFSET);
            break;
        }
        case PB_IMAGE_APP:
        {
            pInfo = (PB_IMAGE_INFO*)(APP_BEGIN|FIRMWARE_IMAGE_INFO_OFFSET);
            break;
        }
        default:
        {
            goto end;
        }
    }

    if (pInfo->imageType != type)
    {
        goto end;
    }
    info->imageType = pInfo->imageType;
    info->imageVersion = pInfo->imageVersion;
    info->hwVersion = pInfo->hwVersion;
    
    ret = true;
end:
    return ret;
}

/******************************************************************************
* Function    : pb_firmware_submit_image
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : submit image to save context in manager
******************************************************************************/
static void pb_firmware_submit_image(PB_IMAGE_CONTEXT *imageContext)
{
    //submit app upgrade via uart
    if (imageContext == NULL)
    {
        pbFirmwareContext.curImage = PB_FIRMWARE_UART;
        pbFirmwareContext.runnableWorking = 1;
        pbFirmwareContext.runnableBootTimes = 1;
    }
    else
    {
    
    }
    
    //write info to flash
    pb_firmware_context_update();
}

/******************************************************************************
* Function    : pb_firmware_update_runnable
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_firmware_update_runnable(uint8 type)
{
    switch (type)
    {
        case PB_FIRMWARE_NORMAL:
        {
            pbFirmwareContext.runnableWorking += 1;
            break;
        }
        case PB_FIRMWARE_FOTA:
        {
            pbFirmwareContext.runnableBootTimes = 1;
            break;
        }
        case PB_FIRMWARE_RECOVER:
        {
            pbFirmwareContext.runnableBootTimes = 1;
            pbFirmwareContext.runnableWorking = 1;
            break;
        }
        default:return;
    }
    
    //write info to flash
    pb_firmware_context_update();    
}

const PB_FIRMWARE_MANAGE fwManager =
{
    pb_firmware_manage_get_context,
    pb_firmware_manage_get_image_info,
    pb_firmware_context_load,
    pb_firmware_context_update,
    pb_firmware_submit_image,
    pb_firmware_update_runnable
};

