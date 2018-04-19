#ifndef _APPUPGRADER

#define _APPUPGRADER

// there are three apps in the flash. runnable@APP_ADD_START, appa@APPA_ADD_START and appb@APPB_ADD_START. each of them are less than 0x20000[128KB]
// after the images there one sector for ota control block struct

// image format, 128B aligned
//    bin -> pad to +128B = Nx2KB
//		get 32B @0x1000, pad 92B secret -> image check
//    merge above two together and pad the all CRC32
//    current app image is 2KB aligned!!!!!

#define OTACONTROLITEMSIZE    64
#define MAGICKEY_CBR          0xDEADBEEF

typedef struct //24B
{
    unsigned int length;                  // total image length in bytes. normally it is 128B aligned. it is used to indicate the baker is used or not.
    unsigned int crc;                     // CRC32 value of the whole image except last 4B
    unsigned int upgrade_time;            // the UTC time when upgrading. if no, 0xFFFFFFFF
    unsigned short run_times;             // 0xFFFF: not used, 0: just uggraded, >0 verified, every boot increase by 1 till 0x7FFF
    unsigned short version;               // image version number
    unsigned int rom_add_start;           // for upgrader to check flash write boundary
    unsigned int rom_add_end;             // for upgrader to check flash write boundary
} Image_ContentTypeDef;

// this used to save the life cycle of flash by 64 times
// 2048B * 2 = 64B * 32 * 2
typedef struct //64B
{
    unsigned int magic_key;               // 0xDEADBEEF
    unsigned int erase_times;             // block erased times
    unsigned short current_image;         // which image is used in runnable, 0: appa, 1: appb, 2: image download via uart
    unsigned short runnable_working;      // 0: working, other: not work. app itself need set it to zero when it is running. it is used for 7 strike.
    unsigned int runnable_boottimes;      // 0: inform bl img updated, >0 run times till 0x7FFFFFFF. set to zero when need ota, set to 1 when move, increase by 1 when confirm.
    Image_ContentTypeDef images[2];
} Upgrade_ControlBlockTypeDef;

// init the module
int OTAManagerInit(void);
// get the free slot [a/b] to store next new image
int getFreeImageBanker(Image_ContentTypeDef *pIC);
// new image stored in slot
int submitNewImage(const Image_ContentTypeDef IC);
// after init app must call it to imform bl if app works well. 0: new image, >0: normal
int confirmImage(void);
// get current image info
void getCurrentImageInfo(unsigned int *upgradeTimestamps, unsigned int *runTimes);

#endif
