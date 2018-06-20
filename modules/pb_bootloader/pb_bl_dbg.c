/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_bl_dbg.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   second com debug
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-20      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "os_trace_log.h"
#include "pb_bl_config.h"
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
#if defined(PB_BOOTLOADER_DBG)
void bl_trace_info(const char *fmt, ...)
{
    char buff[OS_TRACE_LOG_SIZE + 1];
    uint32 len;

    va_list va;
    va_start(va, fmt);
    len = vsnprintf(buff, OS_TRACE_LOG_SIZE, fmt, va);
    va_end(va);
    buff[len] = '\0';

    PB_BL_DEBUG_COM.println(buff);
}
#endif /*PB_BOOTLOADER_DBG*/

