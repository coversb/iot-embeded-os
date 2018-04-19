
#ifndef _SYSFLASH_H_
#define _SYSFLASH_H_

typedef unsigned int uint32_t;
/****************************************************

	说明：	ParkBox 2G的Flash分配表
	此文件与Bios共用，不要随意更改，若App需要使用，请使用预留的地址。

	最新更新时间 2017/9/22
****************************************************/
#define PRODUCT_IS__PORKBOX_

#define VERSION_OFFSET            ((uint32_t)0x1000) //version & beta version
#define DATE_OFFSET               ((uint32_t)0x1008)
#define TIME_OFFSET               ((uint32_t)0x1018)

/*内部Flash  0x80000 512KB*/

// bootloader 16K
#define BOOTLOADER_ADD_START      ((uint32_t)0x08000000)
#define BOOTLOADER_ADD_END        ((uint32_t)0x08004000)


#define DesKeyAddr	              ((uint32_t)0x08003800)
#define RSAKeyAddr	              ((uint32_t)(DesKeyAddr + 64)) 

// app img 160K
#define APP_ADD_START             ((uint32_t)0x08004000)
#define APP_ADD_END               ((uint32_t)0x0802c000)

// app buffer 160*2K + ota control 2K * 2[for ping pong buffer]
#define APPA_ADD_START            ((uint32_t)0x0802c000)
#define APPA_ADD_END              ((uint32_t)0x08054000)
#define APPB_ADD_START            ((uint32_t)0x08054000)
#define APPB_ADD_END              ((uint32_t)0x0807c000)
#define OTA_CONTROL_ADD_START     ((uint32_t)0x0807c000)
#define OTA_CONTROL_ADD_END       ((uint32_t)0x0807d000)

// parameter data 6K [not used now]
#define PARAMETER_DATA_ADD_START  ((uint32_t)0x0807d000)
#define PARAMETER_DATA_ADD_END    ((uint32_t)0x0807e800)

// permanent data 6K for app
#define PERMANENT_DATA_ADD_START  ((uint32_t)0x0807E800)
	// dev info
	// cmd config
	// reserved
#define PERMANENT_DATA_ADD_END    ((uint32_t)0x08080000)

#endif
