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
#ifndef __PB_BL_DBG_H__
#define __PB_BL_DBG_H__

/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"

/******************************************************************************
* Macros
******************************************************************************/
#if defined(PB_BOOTLOADER_DBG)
#define BL_DBG_INFO(...) bl_trace_info(__VA_ARGS__)
#else
#define BL_DBG_INFO(...) 
#endif /*PB_BOOTLOADER_DBG*/

/******************************************************************************
* Global Functions
******************************************************************************/
#if defined(PB_BOOTLOADER_DBG)
extern void bl_trace_info(const char *fmt, ...);
#endif /*PB_BOOTLOADER_DBG*/

#endif  /* __PB_BL_DBG_H__ */
