// shared with app
#include "string.h"

#include "stm32f10x_flash.h"
#include "AppFlashCfg.h"
#include "AppUpgrade.h"

#define DEBUGAPPUPGRADE 0

// this module is a epprom simulator. it use 2KB*2 flash to simulate 64B structure. it decrease erase chance by 64 times.
/*static*/ Upgrade_ControlBlockTypeDef lvOTAControlBlock;

/*static*/ int lfLoadOTAControlBlock(void)
{
    Upgrade_ControlBlockTypeDef *pTemp;
    u32 addressTemp, addressRead, *pUTemp, i;
    FLASH_Status FLASHStatus;
#if DEBUGAPPUPGRADE
    char cTemp[64];
#endif

    // first round find at least one CB in the flash
    addressTemp = OTA_CONTROL_ADD_START;
    do {
        pTemp = (Upgrade_ControlBlockTypeDef *)addressTemp;
        if (pTemp->magic_key == MAGICKEY_CBR)
            break;
        addressTemp += sizeof(Upgrade_ControlBlockTypeDef);
    } while (addressTemp < OTA_CONTROL_ADD_END);

    // find the last record CB
    if (addressTemp < OTA_CONTROL_ADD_END)
    {   // second round find the first one not used after the used one
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "\r\nCB addressTemp = 0x%08x\r\n", addressTemp);
        Printf_String(cTemp);
#endif
        do {
            pTemp = (Upgrade_ControlBlockTypeDef *)addressTemp;
            if (pTemp->magic_key == 0xFFFFFFFF)
                break;
            addressTemp += sizeof(Upgrade_ControlBlockTypeDef);
        } while (addressTemp < OTA_CONTROL_ADD_END);
        addressRead = addressTemp - sizeof(Upgrade_ControlBlockTypeDef);
        memcpy((char*)&lvOTAControlBlock, (char*)addressRead, sizeof(Upgrade_ControlBlockTypeDef));
    }
    else
    {   // not initilized
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   addressTemp = 0x%08x\r\n", addressTemp);
        Printf_String(cTemp);
#endif
        // init data
        lvOTAControlBlock.magic_key = MAGICKEY_CBR;                  // which image is used in runnable, 0: appa, 1: appb, 2: image download via uart
        lvOTAControlBlock.erase_times = 1;                           // block erased times
        lvOTAControlBlock.current_image = 2;                         // which image is used in runnable, 0: appa, 1: appb, 2: image download via uart
        lvOTAControlBlock.runnable_working = 1;                      // 0: working, other: not work. app itself need set it to zero when it is running
        lvOTAControlBlock.runnable_boottimes = 1;                    // 0: just upgraded, >0 run times till 0xFFFFFFFF
        lvOTAControlBlock.images[0].run_times = 0xFFFF;              // 0xFFFF: not used, 0: just uggraded, >0 verified, every boot increase by 1 till 0x7FFF.
        lvOTAControlBlock.images[0].length = 0xFFFFFFFF;             // total image length in bytes. normally it is 2KB aligned
        lvOTAControlBlock.images[0].crc = 0xFFFFFFFF;                // CRC32 value of the whole image except last 4B
        lvOTAControlBlock.images[0].upgrade_time = 0xFFFFFFFF;       // the UTC time when upgrading. if no, 0xFFFFFFFF
        lvOTAControlBlock.images[0].version = 0xFFFF;            // image version number
        lvOTAControlBlock.images[0].rom_add_start = APPA_ADD_START;  // for upgrader to check flash write boundary
        lvOTAControlBlock.images[0].rom_add_end = APPA_ADD_END;      // for upgrader to check flash write boundary
        lvOTAControlBlock.images[1].run_times = 0xFFFF;
        lvOTAControlBlock.images[1].length = 0xFFFFFFFF;
        lvOTAControlBlock.images[1].crc = 0xFFFFFFFF;
        lvOTAControlBlock.images[1].upgrade_time = 0xFFFFFFFF;
        lvOTAControlBlock.images[1].version = 0xFFFF;
        lvOTAControlBlock.images[1].rom_add_start = APPB_ADD_START;
        lvOTAControlBlock.images[1].rom_add_end = APPB_ADD_END;
        addressRead = OTA_CONTROL_ADD_START;
        // store in flash
        FLASH_Unlock();
        FLASHStatus = FLASH_ErasePage(OTA_CONTROL_ADD_START);
        if(FLASHStatus != FLASH_COMPLETE) goto EXIT_FINDCB;
        FLASHStatus = FLASH_ErasePage(OTA_CONTROL_ADD_START + 0X800);
        if(FLASHStatus != FLASH_COMPLETE) goto EXIT_FINDCB;
        for (i = 0, addressTemp = OTA_CONTROL_ADD_START, pUTemp = (u32*)&lvOTAControlBlock; i < sizeof(lvOTAControlBlock) / 4; i++)
        {
//            sprintf(cTemp, "   flash write %02i %08x = 0x%08x\r\n", i, addressTemp, *pUTemp);
//            Printf_String(cTemp);

            FLASHStatus = FLASH_ProgramWord(addressTemp, *pUTemp);
            if(FLASHStatus != FLASH_COMPLETE)
            {
#if DEBUGAPPUPGRADE
                sprintf(cTemp, "   FLASHStatus %08x\r\n", FLASHStatus);
                Printf_String(cTemp);
#endif
                goto EXIT_FINDCB;
            }

            if(*(u32*)addressTemp != *pUTemp)
            {
#if DEBUGAPPUPGRADE
                sprintf(cTemp, "   value %08x\r\n", *(u32*)addressTemp);
                Printf_String(cTemp);
#endif
                goto EXIT_FINDCB;
            }
            addressTemp += 4;
            pUTemp++;
        }
        FLASH_Lock();

    }

#if DEBUGAPPUPGRADE
    sprintf(cTemp, "CB @ %08x\r\n", addressRead);
    Printf_String(cTemp);
    sprintf(cTemp, "   erase_times %d\r\n", lvOTAControlBlock.erase_times);
    Printf_String(cTemp);
    sprintf(cTemp, "   current_image %08x\r\n", lvOTAControlBlock.current_image);
    Printf_String(cTemp);
    sprintf(cTemp, "   runnable_working %08x\r\n", lvOTAControlBlock.runnable_working);
    Printf_String(cTemp);
    sprintf(cTemp, "   runnable_boottimes %08x\r\n", lvOTAControlBlock.runnable_boottimes);
    Printf_String(cTemp);
    sprintf(cTemp, "   [0].run_times %08x\r\n", lvOTAControlBlock.images[0].run_times);
    Printf_String(cTemp);
    sprintf(cTemp, "   [0].version %08x\r\n", lvOTAControlBlock.images[0].version);
    Printf_String(cTemp);
    sprintf(cTemp, "   [0].rom_add_start %08x\r\n", lvOTAControlBlock.images[0].rom_add_start);
    Printf_String(cTemp);
    sprintf(cTemp, "   [1].run_times %08x\r\n", lvOTAControlBlock.images[1].run_times);
    Printf_String(cTemp);
    sprintf(cTemp, "   [1].version %08x\r\n", lvOTAControlBlock.images[1].version);
    Printf_String(cTemp);
    sprintf(cTemp, "   [1].rom_add_start %08x\r\n", lvOTAControlBlock.images[1].rom_add_start);
    Printf_String(cTemp);
#endif
    return 0;

EXIT_FINDCB:
#if DEBUGAPPUPGRADE
    sprintf(cTemp, "   find CB error %s %i - %i\r\n", __FILE__, __LINE__, i);
    Printf_String(cTemp);
#endif
    FLASH_Lock();
    return -1;
}

/*static*/ int lfStoreOTAControlBlock(void)
{
    Upgrade_ControlBlockTypeDef *pTemp;
    u32 addressTemp, offsetTemp, *pUTemp, i;
    u32 addressWrite;
    FLASH_Status FLASHStatus;
#if DEBUGAPPUPGRADE
    char cTemp[64];
#endif

    // find the first used record
    addressTemp = OTA_CONTROL_ADD_START;
    offsetTemp = 0;
    do {
        pTemp = (Upgrade_ControlBlockTypeDef *)addressTemp;
        if (pTemp->magic_key == MAGICKEY_CBR)
            break;
        offsetTemp += sizeof(Upgrade_ControlBlockTypeDef);
        addressTemp += sizeof(Upgrade_ControlBlockTypeDef);
    } while (addressTemp < OTA_CONTROL_ADD_END);

    // find the free one after used
    do {
        pTemp = (Upgrade_ControlBlockTypeDef *)addressTemp;
        if (pTemp->magic_key == 0xFFFFFFFF)
            break;
        offsetTemp += sizeof(Upgrade_ControlBlockTypeDef);
        addressTemp += sizeof(Upgrade_ControlBlockTypeDef);
    } while (addressTemp < OTA_CONTROL_ADD_END);

    // find the last record CB
    if (addressTemp < OTA_CONTROL_ADD_END)
    {
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   store addressTemp = 0x%08x\r\n", addressTemp);
        Printf_String(cTemp);
#endif
    }
    else
    {
        addressTemp = OTA_CONTROL_ADD_START;
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   store addressTemp = 0x%08x\r\n", addressTemp);
        Printf_String(cTemp);
#endif
    }
    addressWrite = addressTemp;

    // store in flash
    FLASH_Unlock();
    if ((addressWrite == OTA_CONTROL_ADD_START) || (addressWrite == OTA_CONTROL_ADD_START + 0x800))
        lvOTAControlBlock.erase_times += 1;
    for (i = 0, pUTemp = (u32*)&lvOTAControlBlock; i < sizeof(lvOTAControlBlock) / 4; i++)
    {
//        sprintf(cTemp, "   flash write %02i %08x = 0x%08x\r\n", i, addressTemp, *pUTemp);
//        Printf_String(cTemp);
        FLASHStatus = FLASH_ProgramWord(addressTemp, *pUTemp);
        if(FLASHStatus != FLASH_COMPLETE)
        {
#if DEBUGAPPUPGRADE
            sprintf(cTemp, "   FLASHStatus %08x\r\n", FLASHStatus);
            Printf_String(cTemp);
#endif
            goto EXIT_WRITECB;
        }

        if(*(u32*)addressTemp != *pUTemp)
        {
#if DEBUGAPPUPGRADE
            sprintf(cTemp, "   value %08x\r\n", *(u32*)addressTemp);
            Printf_String(cTemp);
#endif
            goto EXIT_WRITECB;
        }
        addressTemp += 4;
        pUTemp++;
    }

    // erase the other sector for writing
    if (addressWrite == OTA_CONTROL_ADD_START)
    {
        FLASHStatus = FLASH_ErasePage(OTA_CONTROL_ADD_START + 0x800);
        if(FLASHStatus != FLASH_COMPLETE) goto EXIT_WRITECB;
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   store erase  %08x\r\n", OTA_CONTROL_ADD_START + 0x800);
        Printf_String(cTemp);
#endif
    }
    else if (addressWrite == OTA_CONTROL_ADD_START + 0x800)
    {
        FLASHStatus = FLASH_ErasePage(OTA_CONTROL_ADD_START);
        if(FLASHStatus != FLASH_COMPLETE) goto EXIT_WRITECB;
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   store erase  %08x\r\n", OTA_CONTROL_ADD_START);
        Printf_String(cTemp);
#endif
    }

    FLASH_Lock();
    return 0;

EXIT_WRITECB:
#if DEBUGAPPUPGRADE
    sprintf(cTemp, "   store CB error %s %i - %i\r\n", __FILE__, __LINE__, i);
    Printf_String(cTemp);
#endif
    FLASH_Lock();
    return -1;
}

// !!!app upgrade procedure!!!
//	1. getFreeImageBanker => where can store the new image
//  2. modify Image_ContentTypeDefand call submitNewImage => inform OTA module the new image is ready
//      2.1 IC.crc
//      2.2 IC.upgrade_time
//      2.3 IC.version
//      2.4 IC.length
//  3. reboot => during next boot bl will copy new image to rannable banker

// this function can be removed
int OTAManagerInit(void)
{
#if DEBUGAPPUPGRADE
    char cTemp[128];
#endif

    if (lfLoadOTAControlBlock())
    {
#if DEBUGAPPUPGRADE
        sprintf(cTemp, "   OTAManagerInit fail\r\n");
        Printf_String(cTemp);
#endif
        return -1;
    }
    else
    {
        return 0;
    }
}


// this is the basic mechanism for image upgrading
// get the free slot [a/b] to store next new image
int getFreeImageBanker(Image_ContentTypeDef *pIC)
{
    u32 aRun = 0, bRun = 0;
    if (lvOTAControlBlock.current_image > 1)
    {   // runnable image is update via uart
        if ((lvOTAControlBlock.images[0].run_times != 0) && (lvOTAControlBlock.images[0].run_times != 0xFFFF))
            aRun += 1;
        if ((lvOTAControlBlock.images[1].run_times != 0) && (lvOTAControlBlock.images[1].run_times != 0xFFFF))
            bRun += 1;
        if (lvOTAControlBlock.images[0].version < lvOTAControlBlock.images[1].version)
            bRun += 1;
        if (lvOTAControlBlock.images[0].version > lvOTAControlBlock.images[1].version)
            aRun += 1;
    }
    else if (lvOTAControlBlock.current_image == 0)
    {   // overwrite older version
        aRun += 1;
    }
    else if (lvOTAControlBlock.current_image == 1)
    {
        bRun += 1;
    }

    if (aRun < bRun)
        *pIC = lvOTAControlBlock.images[0];
    else
        *pIC = lvOTAControlBlock.images[1];

    return 0;
}


// new image stored in slot
int submitNewImage(const Image_ContentTypeDef IC)
{
    if (IC.rom_add_start == lvOTAControlBlock.images[0].rom_add_start)
    {   //a updated
        lvOTAControlBlock.current_image = 0;
        lvOTAControlBlock.images[0].run_times = 0;
        lvOTAControlBlock.images[0].crc = IC.crc;
        lvOTAControlBlock.images[0].upgrade_time = IC.upgrade_time;
        lvOTAControlBlock.images[0].version = IC.version;
        lvOTAControlBlock.images[0].length = IC.length;

    }
    else if (IC.rom_add_start == lvOTAControlBlock.images[1].rom_add_start)
    {   //b updated
        lvOTAControlBlock.current_image = 1;
        lvOTAControlBlock.images[1].run_times = 0;
        lvOTAControlBlock.images[1].crc = IC.crc;
        lvOTAControlBlock.images[1].upgrade_time = IC.upgrade_time;
        lvOTAControlBlock.images[1].version = IC.version;
        lvOTAControlBlock.images[1].length = IC.length;
    }
    lvOTAControlBlock.runnable_working = 1;
    lvOTAControlBlock.runnable_boottimes = 0;
    return lfStoreOTAControlBlock();
}


// app imform it can boot and work
//   0: non-first time run
//   1: new image first time run
//   2£»recover image first time run
//   3: usb upgraded image first time run
//  -1: error
int confirmImage()
{
    int temp = 0;

    if (lfLoadOTAControlBlock() == 0)
    {
        lvOTAControlBlock.runnable_working = 0; // reset 7 striker
        if (lvOTAControlBlock.current_image > 1)
        {   // runnable image is update via uart
            if (lvOTAControlBlock.runnable_boottimes == 1)
            {   // first time run
                temp = 3;
            }
        }
        else if (lvOTAControlBlock.current_image == 0)
        {
            if (lvOTAControlBlock.images[0].run_times < 0x7FFF)
                lvOTAControlBlock.images[0].run_times += 1;
            if (lvOTAControlBlock.runnable_boottimes == 1)
            {   // first time run after upgrade
                if (lvOTAControlBlock.images[0].run_times == 1)
                    temp = 1;
                else
                    temp = 2;
            }
        }
        else if (lvOTAControlBlock.current_image == 1)
        {
            if (lvOTAControlBlock.images[1].run_times < 0x7FFF)
                lvOTAControlBlock.images[1].run_times += 1;
            if (lvOTAControlBlock.runnable_boottimes == 1)
            {   // first time run after upgrade
                if (lvOTAControlBlock.images[1].run_times == 1)
                    temp = 1;
                else
                    temp = 2;
            }
        }
        if (lvOTAControlBlock.runnable_boottimes < 0x7FFFFFFF)
            lvOTAControlBlock.runnable_boottimes += 1;

        lfStoreOTAControlBlock();
        return temp;
    }

    return -1;
}

void getCurrentImageInfo(unsigned int *upgradeTimestamps, unsigned int *runTimes)
{
    if (lvOTAControlBlock.current_image > 1)
    {
        *upgradeTimestamps = 0;
        *runTimes = 0;
        return;
    }
    
    *upgradeTimestamps = lvOTAControlBlock.images[lvOTAControlBlock.current_image].upgrade_time;
    *runTimes = lvOTAControlBlock.runnable_boottimes;
}

