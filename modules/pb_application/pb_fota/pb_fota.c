/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_fota.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   firmware over the air functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "os_trace_log.h"
#include "pb_app_config.h"
#include "pb_fota.h"

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
* Function    : pb_fota_request_free_bank
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get current image flash address
******************************************************************************/
int32 pb_fota_request_free_bank(PB_IMAGE_CONTENT_TYPE *pIC)
{
    return getFreeImageBanker(pIC);
}

/******************************************************************************
* Function    : pb_fota_write_firmware_to_free_bank
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write firmdata to free bank
******************************************************************************/
int32 pb_fota_write_firmware_to_free_bank(PB_IMAGE_CONTENT_TYPE *pIC, uint32 addrOffset, uint8 *pdata, uint16 len)
{
    uint32 writeAddr = pIC->rom_add_start + addrOffset;
    if (writeAddr > pIC->rom_add_end)
    {
        OS_DBG_ERR(DBG_MOD_PBFOTA, "BAD FOTA WRITE ADDR[%08X]", writeAddr);
        return -1;
    }

    return PB_FLASH_HDLR.write(writeAddr, (uint32*)pdata, len);
}

/******************************************************************************
* Function    : pb_util_submit_new_image
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : submit new image to bl
******************************************************************************/
int32 pb_fota_submit_new_image(PB_IMAGE_CONTENT_TYPE IC)
{
    return submitNewImage(IC);
}

/******************************************************************************
* Function    : pb_prot_firmware_confirm
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
void pb_fota_firmware_confirm(void)
{
    int32 ret = confirmImage();
    OS_DBG_TRACE(DBG_MOD_PBFOTA, DBG_INFO, "Prot FIRMWARE COMFIRM[%d]", ret);

    switch (ret)
    {
        case PB_IMAGE_FOTA_OK:
        {
            //pb_ota_send_fota_msg(PB_PROT_RSP_FOTA_UPGRADE_OK, 0);
            break;
        }
        case PB_IMAGE_FOTA_ERR_AND_RECOVER:
        {
            //pb_ota_send_fota_msg(PB_PROT_RSP_FOTA_UPGRADE_ERR, 0);
            break;
        }
        case PB_IMAGE_NORMAL:
        case PB_IMAGE_COM_UPDATE:
        default:
            break;
    }
}

/******************************************************************************
* Function    : pb_fota_get_firmware_info
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_fota_get_firmware_info(uint32 *upTimestamp, uint32 *runTimes)
{
    getCurrentImageInfo(upTimestamp, runTimes);
}

/******************************************************************************
* Function    : pb_fota_get_firmware_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_fota_get_firmware_version(void)
{
    return PB_FIRMWARE_VERSION;
}

/******************************************************************************
* Function    : pb_fota_get_bl_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_fota_get_bl_version(void)
{
    return PB_BOOTLOADER_VERSION;
}

