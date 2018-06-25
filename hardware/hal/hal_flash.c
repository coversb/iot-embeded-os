/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_flash.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   flash driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-17      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_flash.h"
#include "os_middleware.h"
#include "os_trace_log.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static OS_MUTEX_TYPE hal_flash_mutex = NULL;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : hal_flash_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_flash_init(void)
{
    os_mutex_lock_init(&hal_flash_mutex);
}

/******************************************************************************
* Function    : hal_flash_deinit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_flash_deinit(void)
{
    os_mutex_lock_init(&hal_flash_mutex);
}

#if defined(BOARD_STM32F4XX)
/******************************************************************************
* Function    : hal_flash_get_sector
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 hal_flash_get_sector(uint32 addr)
{
    uint32 sector = 0xFFFF;

    if ((addr < BOADR_FLASH_SEC1) && (addr >= BOADR_FLASH_SEC0)) { sector = FLASH_Sector_0; }
    else if ((addr < BOADR_FLASH_SEC2) && (addr >= BOADR_FLASH_SEC1)) { sector = FLASH_Sector_1; }
    else if ((addr < BOADR_FLASH_SEC3) && (addr >= BOADR_FLASH_SEC2)) { sector = FLASH_Sector_2; }
    else if ((addr < BOADR_FLASH_SEC4) && (addr >= BOADR_FLASH_SEC3)) { sector = FLASH_Sector_3; }
    else if ((addr < BOADR_FLASH_SEC5) && (addr >= BOADR_FLASH_SEC4)) { sector = FLASH_Sector_4; }
    else if ((addr < BOADR_FLASH_SEC6) && (addr >= BOADR_FLASH_SEC5)) { sector = FLASH_Sector_5; }
    else if ((addr < BOADR_FLASH_SEC7) && (addr >= BOADR_FLASH_SEC6)) { sector = FLASH_Sector_6; }
    else if ((addr < BOADR_FLASH_SEC8) && (addr >= BOADR_FLASH_SEC7)) { sector = FLASH_Sector_7; }
    else if ((addr < BOADR_FLASH_SEC9) && (addr >= BOADR_FLASH_SEC8)) { sector = FLASH_Sector_8; }
    else if ((addr < BOADR_FLASH_SEC10) && (addr >= BOADR_FLASH_SEC9)) { sector = FLASH_Sector_9; }
    else if ((addr < BOADR_FLASH_SEC11) && (addr >= BOADR_FLASH_SEC10)) { sector = FLASH_Sector_10; }
    else if ((addr < BOADR_FLASH_SEC12) && (addr >= BOADR_FLASH_SEC11)) { sector = FLASH_Sector_11; }
    else if ((addr < BOADR_FLASH_SEC13) && (addr >= BOADR_FLASH_SEC12)) { sector = FLASH_Sector_12; }
    else if ((addr < BOADR_FLASH_SEC14) && (addr >= BOADR_FLASH_SEC13)) { sector = FLASH_Sector_13; }
    else if ((addr < BOADR_FLASH_SEC15) && (addr >= BOADR_FLASH_SEC14)) { sector = FLASH_Sector_14; }
    else if ((addr < BOADR_FLASH_SEC16) && (addr >= BOADR_FLASH_SEC15)) { sector = FLASH_Sector_15; }
    else if ((addr < BOADR_FLASH_SEC17) && (addr >= BOADR_FLASH_SEC16)) { sector = FLASH_Sector_16; }
    else if ((addr < BOADR_FLASH_SEC18) && (addr >= BOADR_FLASH_SEC17)) { sector = FLASH_Sector_17; }
    else if ((addr < BOADR_FLASH_SEC19) && (addr >= BOADR_FLASH_SEC18)) { sector = FLASH_Sector_18; }
    else if ((addr < BOADR_FLASH_SEC20) && (addr >= BOADR_FLASH_SEC19)) { sector = FLASH_Sector_19; }
    else if ((addr < BOADR_FLASH_SEC21) && (addr >= BOADR_FLASH_SEC20)) { sector = FLASH_Sector_20; }
    else if ((addr < BOADR_FLASH_SEC22) && (addr >= BOADR_FLASH_SEC21)) { sector = FLASH_Sector_21; }
    else if ((addr < BOADR_FLASH_SEC23) && (addr >= BOADR_FLASH_SEC22)) { sector = FLASH_Sector_22; }
    else if ((addr < BOARD_FLASH_BASE + BOARD_FLASH_SIZE) && (addr >= BOADR_FLASH_SEC23)) { sector = FLASH_Sector_23; } 
    
    return sector;
}

/******************************************************************************
* Function    : hal_flash_erase_sectors
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hal_flash_erase_sectors(uint32 addr, uint32 len)
{
    FLASH_Status stat = FLASH_COMPLETE;
    bool ret = true;
    uint32 curSector = 0;
    uint32 endSector = 0;
    
    os_mutex_lock(&hal_flash_mutex);
    
    FLASH_Unlock();
    FLASH_DataCacheCmd(DISABLE);

    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR
                             | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    
    curSector = hal_flash_get_sector(addr);
    endSector = hal_flash_get_sector(addr + len - 1);
    
    while (curSector <= endSector)
    {
        stat = FLASH_EraseSector(curSector, BORAD_FLASH_VOLTAGE);
        if ( stat != FLASH_COMPLETE)
        {
            OS_DBG_ERR(DBG_MOD_HAL, "flash erase err");
            ret = false;
            goto flash_programe_EXIT;
        }
        
        if (curSector == FLASH_Sector_11)
        {
            curSector += 40;
        }
        else
        {
            curSector += 8;
        }
    }
    
flash_programe_EXIT:    
    FLASH_DataCacheCmd(ENABLE);
    FLASH_Lock();

    os_mutex_unlock(&hal_flash_mutex);

    return ret;
}
#endif /*BOARD_STM32F4XX*/

/******************************************************************************
* Function    : hal_flash_erase_page
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : erase one page of flash
******************************************************************************/
static bool hal_flash_erase_page(uint32 addr)
{
    FLASH_Status stat = FLASH_COMPLETE;
    
    os_mutex_lock(&hal_flash_mutex);
    
    FLASH_Unlock();

    #if defined(BOARD_STM32F1XX)
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    stat = FLASH_ErasePage(addr);
    #elif defined(BOARD_STM32F4XX)
    FLASH_DataCacheCmd(DISABLE);
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR
                             | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    uint32 sector = hal_flash_get_sector(addr);
    if (sector != 0xFFFF)
    {
        stat = FLASH_EraseSector(sector, BORAD_FLASH_VOLTAGE);
    }
    else
    {
        OS_DBG_ERR(DBG_MOD_HAL, "bad flash addr %X", addr);
    }
    FLASH_DataCacheCmd(ENABLE);
    #else
    #error hal_flash_erase_page
    #endif
    
    FLASH_Lock();

    os_mutex_unlock(&hal_flash_mutex);

    if (stat != FLASH_COMPLETE)
    {
        OS_DBG_ERR(DBG_MOD_HAL, "flash erase err");
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : hal_flash_erase_pages
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool hal_flash_erase_pages(uint32 addr, uint32 len)
{
    if ((addr < BOARD_FLASH_BASE)
        || (addr > BOARD_FLASH_BASE + BOARD_FLASH_SIZE)
        || (addr + len > BOARD_FLASH_BASE + BOARD_FLASH_SIZE))
    {
        return false;
    }

    #if defined(BOARD_STM32F1XX)
    uint32 offsetAddr = 0;
    uint32 pagePos = 0;
    uint32 pageNum = 0;    

    offsetAddr = addr - BOARD_FLASH_BASE;
    pagePos = offsetAddr / BOARD_FLASH_SECTOR_SIZE;
    pageNum = len / BOARD_FLASH_SECTOR_SIZE + (((len % BOARD_FLASH_SECTOR_SIZE) > 0) ? 1 : 0);

    for (uint32 pageIdx = pagePos; pageIdx < pagePos + pageNum; ++pageIdx)
    {
        if (!hal_flash_erase_page(BOARD_FLASH_BASE + pageIdx * BOARD_FLASH_SECTOR_SIZE))
        {
            return false;
        }
    }
    #elif defined(BOARD_STM32F4XX)
    if (!hal_flash_erase_sectors(addr, len))
    {
        return false;
    }
    #else
    #error hal_flash_erase_pages
    #endif

    return true;
}

/******************************************************************************
* Function    : hal_flash_read
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 hal_flash_read(uint32 addr, uint8 *buff, uint32 len)
{
    int DataNum = 0;

    os_mutex_lock(&hal_flash_mutex);
    
    while (DataNum < len) 
    {
        *(buff + DataNum) = *(volatile uint8*) addr++;
        DataNum++;
    }

    os_mutex_unlock(&hal_flash_mutex);
    
    return DataNum;
}

/******************************************************************************
* Function    : hal_flash_write
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : write data to flash
******************************************************************************/
static int32 hal_flash_write(uint32 addr, uint32 *buff, uint32 len)
{
    uint16 i_write = 0;
    uint32 write_addr = addr;
    uint16 WordLength = (len + 3) / 4;

    hal_flash_erase_page(addr);

    os_mutex_lock(&hal_flash_mutex);
    
    FLASH_Unlock();
    #if defined(BOARD_STM32F4XX)
    FLASH_DataCacheCmd(DISABLE);
    #endif
    
    for(i_write = 0; i_write < WordLength; i_write++)
    {
        FLASH_ProgramWord(write_addr, *buff);
        if((*(u32*)write_addr) != (*buff))
        {
            OS_DBG_ERR(DBG_MOD_HAL, "flash write err");
            goto flash_programe_EXIT;
        }
        write_addr += 4;
        buff++;
    }
flash_programe_EXIT:
    #if defined(BOARD_STM32F4XX)
    FLASH_DataCacheCmd(ENABLE);
    #endif
    FLASH_Lock();

    os_mutex_unlock(&hal_flash_mutex);

    if(i_write == WordLength)
    {
        return 0;
    }
    
    return (-i_write);
}

/******************************************************************************
* Function    : hal_flash_write_force
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static int32 hal_flash_write_force(uint32 addr, uint32 *buff, uint32 len)
{
    uint16 i_write = 0;
    uint32 write_addr = addr;
    uint16 WordLength = (len + 3) / 4;

    os_mutex_lock(&hal_flash_mutex);
    
    FLASH_Unlock();
    #if defined(BOARD_STM32F4XX)
    FLASH_DataCacheCmd(DISABLE);
    #endif

    for(i_write = 0; i_write < WordLength; i_write++)
    {
        FLASH_ProgramWord(write_addr, *buff);
        if((*(u32*)write_addr) != (*buff))
        {
            OS_DBG_ERR(DBG_MOD_HAL, "flash write err");
            goto flash_programe_EXIT;
        }
        write_addr += 4;
        buff++;
    }
flash_programe_EXIT:
    #if defined(BOARD_STM32F4XX)
    FLASH_DataCacheCmd(ENABLE);
    #endif
    FLASH_Lock();

    os_mutex_unlock(&hal_flash_mutex);

    if(i_write == WordLength)
    {
        return 0;
    }
    
    return (-i_write);
}

const HAL_FLASH_TYPE hwFlash = 
{
    hal_flash_init,
    hal_flash_deinit,
    hal_flash_erase_page,
    hal_flash_erase_pages,
    hal_flash_read,
    hal_flash_write,
    hal_flash_write_force
};

