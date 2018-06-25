/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_wdg.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   hardware watchdog
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-13      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rtc.h"
#include "hal_bkp.h"
#include "os_trace_log.h"
#include <time.h>

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
* Function    : hal_rtc_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void hal_rtc_set(uint32 timestamp)
{
    #if ( BOARD_RTC_ENABLE == 1 )
    PWR_BackupAccessCmd(ENABLE);

    #if defined(BOARD_STM32F1XX)
    RTC_WaitForLastTask();
    RTC_SetCounter(timestamp);
    RTC_WaitForLastTask();
    #elif defined(BOARD_STM32F4XX)
    time_t t;
    struct tm *lt;
    t = timestamp;
    lt = localtime(&t);
    
    RTC_DateTypeDef RTC_DateTypeInitStructure;
    RTC_TimeTypeDef RTC_TimeTypeInitStructure;

    RTC_DateTypeInitStructure.RTC_Year = lt->tm_year + 1900 - 2000;
    RTC_DateTypeInitStructure.RTC_Month = lt->tm_mon + 1;
    RTC_DateTypeInitStructure.RTC_Date = lt->tm_mday;
    //set a default value, incase of reg calc err
    RTC_DateTypeInitStructure.RTC_WeekDay = 0;

    RTC_TimeTypeInitStructure.RTC_Hours = lt->tm_hour;
    RTC_TimeTypeInitStructure.RTC_Minutes = lt->tm_min;
    RTC_TimeTypeInitStructure.RTC_Seconds = lt->tm_sec;
    //set a default value, incase of reg calc err
    RTC_TimeTypeInitStructure.RTC_H12 = 0;
    RTC_SetDate(RTC_Format_BIN, &RTC_DateTypeInitStructure);
    RTC_SetTime(RTC_Format_BIN, &RTC_TimeTypeInitStructure);
    #else
    #error hal_rtc_set
    #endif

    PWR_BackupAccessCmd(DISABLE);
    #endif /*BOARD_RTC_ENABLE*/
}

/******************************************************************************
* Function    : hal_rtc_get
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint32 hal_rtc_get(void)
{
    uint32 time = 0;

    #if ( BOARD_RTC_ENABLE == 1 )
    #if defined(BOARD_STM32F1XX)
    RTC_WaitForLastTask();
    time = RTC_GetCounter();
    RTC_WaitForLastTask();
    #elif defined(BOARD_STM32F4XX)
    uint16 y;
    uint8 m, d, hour, min, sec;

    RTC_DateTypeDef RTC_DateTypeInitStructure;
    RTC_TimeTypeDef RTC_TimeTypeInitStructure;

    RTC_GetTime(RTC_Format_BIN, &RTC_TimeTypeInitStructure);
    RTC_GetDate(RTC_Format_BIN, &RTC_DateTypeInitStructure);

    y = RTC_DateTypeInitStructure.RTC_Year + 2000;
    m = RTC_DateTypeInitStructure.RTC_Month;
    d = RTC_DateTypeInitStructure.RTC_Date;

    hour = RTC_TimeTypeInitStructure.RTC_Hours;
    min = RTC_TimeTypeInitStructure.RTC_Minutes;
    sec = RTC_TimeTypeInitStructure.RTC_Seconds;

    struct tm lt;
    lt.tm_year = y - 1900;
    lt.tm_mon = m - 1;
    lt.tm_mday = d;
    lt.tm_hour = hour;
    lt.tm_min = min;
    lt.tm_sec = sec;

    time = mktime(&lt);
    #else
    #error hal_rtc_get
    #endif
    #endif /*BOARD_RTC_ENABLE*/

    return time;
}

/******************************************************************************
* Function    : hal_rtc_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void hal_rtc_init(void)
{
    #if ( BOARD_RTC_ENABLE == 1 )
    if (hal_bkp_read(BOARD_BKP_RTC_ADDR) != BOARD_RTC_BKP_SETFLAG)
    {
        OS_DBG_ERR(DBG_MOD_HAL, "RTC is not configured");
        
        uint32 lseWait = 0;

        /*Enable RTC and backup register access*/
        PWR_BackupAccessCmd(ENABLE);

        //Enable LSE
        RCC_LSEConfig(RCC_LSE_ON);
        //Wait till LSE is ready
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
        {
            for (uint8 i = 0; i < 255; ++ i) {}//some delay
            
            ++lseWait;
            if (lseWait >= 0xFFFF)
            {
                OS_DBG_ERR(DBG_MOD_HAL, "LSE ERR");
                return;
            }
        }

        //Select LSE as RTC Clock Source
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        //Enable RTC Clock */
        RCC_RTCCLKCmd(ENABLE);
        RTC_WaitForSynchro();

        #if defined(BOARD_STM32F1XX)
        //Set RTC prescaler: set RTC period to 1sec
        RTC_WaitForLastTask();
        RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
        #elif defined(BOARD_STM32F4XX)
        RTC_InitTypeDef RTC_InitStructure;
        RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
        RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
        RTC_InitStructure.RTC_SynchPrediv = 0xFF;
        RTC_Init(&RTC_InitStructure);
        #else
        #error hal_rtc_init
        #endif

        OS_DBG_ERR(DBG_MOD_HAL, "RTC set to default");
        hal_rtc_set(BOARD_RTC_DEFAULT);

        hal_bkp_write(BOARD_BKP_RTC_ADDR, BOARD_RTC_BKP_SETFLAG);
    }
    else
    {
        RTC_WaitForSynchro();
    }    
    #endif /*BOARD_RTC_ENABLE*/
}

