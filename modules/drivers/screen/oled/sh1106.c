/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          sh1106.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   sh1106 oled driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                     Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "board_config.h"
#include "os_middleware.h"
#include "os_trace_log.h"
#include "hal_sw_i2c.h"
#include "sh1106.h"
#include "font.h"

/******************************************************************************
* Macros
******************************************************************************/
#define SH1106_COM_MAX_ERR 255  // times
#define SH1106_RESET_DELAY 60   // seconds

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static HAL_SW_I2C_TYPE *SH1106_I2C = NULL;

static bool bComAvailable = true;
static uint32 lastErrTime = 0;

/******************************************************************************
* Local Functions
******************************************************************************/
static void sh1106_reset(void);

/******************************************************************************
* Function    : sh1106_set_error
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_set_error(bool sw)
{
    static uint16 comErrCnt = 0;

    if (sw)
    {
        if (++comErrCnt >= SH1106_COM_MAX_ERR)
        {
            bComAvailable = false;
            lastErrTime = os_get_second_count();
            OS_INFO("BAD SCREEN");
        }
    }
    else
    {
        comErrCnt = 0;
    }
}

/******************************************************************************
* Function    : sh1106_available
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool sh1106_available(void)
{    
    if (!bComAvailable)
    {
        if (os_get_second_count() - lastErrTime >= SH1106_RESET_DELAY)
        {
            sh1106_set_error(false);
            bComAvailable = true;
            sh1106_reset();
        }
    }
        
    return bComAvailable;
}

/******************************************************************************
* Function    : sh1106_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_config(uint8 cmd)
{
    if (!sh1106_available())
    {
        return;
    }
    
    if (!SH1106_I2C->wirteByte(0x78, 0x00, cmd))
    {
        sh1106_set_error(true);
        OS_DBG_ERR(DBG_MOD_DEV, "SH1106 conf err[%d]", cmd);
    }
    else
    {
        sh1106_set_error(false);
    }
}

/******************************************************************************
* Function    : sh1106_write_data
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_write_data(uint8 data)
{
    if (!sh1106_available())
    {
        return;
    }

    if (!SH1106_I2C->wirteByte(0x78, 0x40, data))
    {
        sh1106_set_error(true);
        OS_DBG_ERR(DBG_MOD_DEV, "SH1106 write data err[%d]", data);
    }
    else
    {
        sh1106_set_error(false);
    }
}

/******************************************************************************
* Function    : sh1106_set_pos
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_set_pos(uint8 column,uint8 row)
{
    column *= 8;
    sh1106_config(0xb0 + row);
    sh1106_config(((column & 0xf0) >> 4) | 0x10);
    sh1106_config((column & 0x0f) | 0x02);
}

/******************************************************************************
* Function    : sh1106_show_bitmap
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_show_bitmap(uint8 column,uint8 row, uint8 *map)
{
    sh1106_set_pos(column, row * 2);
    for(uint8 idx = 0; idx < 8; ++idx)
    {
        sh1106_write_data(map[idx]);
    }

    sh1106_set_pos(column, row * 2 + 1);
    for(uint8 idx = 0; idx < 8; ++idx)
    {
        sh1106_write_data(map[8 + idx]);
    }
}

/******************************************************************************
* Function    : sh1106_show
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_show(uint8 column, uint8 row, char *pdata)
{
    for (uint8 idx = 0; pdata[idx] != 0 && idx < 16 - column; idx++)
    {
        if (pdata[idx] >= ' ' && pdata[idx] <= '~')
        {
            sh1106_show_bitmap(column + idx, row, (uint8*)&FONT_CODE[pdata[idx] - ' '][0]);
        }
        else
        {
            sh1106_show_bitmap(column + idx, row, (uint8*)&FONT_CODE[0][0]);
        }            
    }
}

/******************************************************************************
* Function    : sh1106_clear
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_clear(void)
{
    for(uint8 m = 0; m < 8; m++)
    {
        sh1106_config(0xb0 + m);    //page0-page1
        sh1106_config(0x02);    //low column start address
        sh1106_config(0x10);    //high column start address
        for(uint8 n = 0; n < 128; n++)
        {
            sh1106_write_data(0);
        }
    }
}

/******************************************************************************
* Function    : sh1106_reverse
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_reverse(void)
{
    static bool reverse = false;

    reverse = (bool)!reverse;

    if (reverse)
    {
        // display in reverse direction
        sh1106_config(0xA0);
        sh1106_config(0xC0);
    }
    else
    {
        // display in normal direction
        sh1106_config(0xA1);
        sh1106_config(0xC8);
    }
}

/******************************************************************************
* Function    : sh1106_reset
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void sh1106_reset(void)
{
    sh1106_config(0xAE);    /*display off*/
    sh1106_config(0x02);    /*set lower column address*/
    sh1106_config(0x10);    /*set higher column address*/
    sh1106_config(0x40);    /*set display start line*/
    sh1106_config(0xB0);    /*set page address*/
    sh1106_config(0x81);    /*contract control*/
    sh1106_config(0xCF);    /*128*/
    sh1106_config(0xA6);    /*normal / reverse*/
    sh1106_config(0xA8);    /*multiplex ratio*/
    sh1106_config(0x3F);    /*duty = 1/64*/
    sh1106_config(0xC8);    /*Com scan direction*/
    sh1106_config(0xD3);    /*set display offset*/
    sh1106_config(0x00);
    sh1106_config(0xD5);    /*set osc division*/
    sh1106_config(0x80);
    sh1106_config(0xD9);    /*set pre-charge period*/
    sh1106_config(0Xf1);
    sh1106_config(0xDA);    /*set COM pins*/
    sh1106_config(0x12);
    sh1106_config(0xdb);    /*set vcomh*/
    sh1106_config(0x40);
    sh1106_config(0x8d);    //ÉèÖÃµçºÉ±Ã
    sh1106_config(0x14);		//¿ªÆôµçºÉ±Ã
    sh1106_config(0xAF);    //OLED»½ÐÑ

    // display in normal direction
    sh1106_config(0xA1);
    sh1106_config(0xC8);

    OS_INFO("RESET SCREEN");
}

/******************************************************************************
* Function    : sh1106_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool sh1106_init(HAL_SW_I2C_TYPE *i2c)
{
    if (i2c == NULL)
    {
        OS_DBG_ERR(DBG_MOD_DEV, "SH1106 I2C is invalid");
        return false;
    }
    SH1106_I2C = i2c;
    SH1106_I2C->init();

    sh1106_reset();
    sh1106_clear();
    return true;
}

const DEV_TYPE_SH1106 devSH1106 = 
{
    sh1106_init,
    sh1106_reset,
    sh1106_reverse,
    sh1106_clear,
    sh1106_show
};

