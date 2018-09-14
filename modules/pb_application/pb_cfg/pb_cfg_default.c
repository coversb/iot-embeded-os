/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_cfg_default.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   assign all the default data
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-17      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "pb_cfg_default.h"

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
/*device info default value*/
const uint8 PB_CFG_SN_DEFAULT[16] = "0101086002100000";
//default UID 0000000000600000
const uint8 PB_CFG_UID_DEFALUT[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00};
//default MAC addr 08.60.02.10.00.00
const uint8 PB_CFG_MAC_DEFALUT[6] = {0x08, 0x60, 0x02, 0x10, 0x00, 0x00};

/*command configurations default value*/
//APN
const PB_CFG_APC PB_CFG_APC_DEFAULT = 
{
    "",             //apn
    "",             //apn usr name
    "",             //apn password
    {114, 114, 114, 114},//main DNS server
    {114, 114, 114, 114},//backup DNS server
    {114, 114, 114, 114},//last good DNS server
    0,              //align
    {0},           //pending
    0               //crc
};

//Server configuration
const PB_CFG_SER PB_CFG_SER_DEFAULT = 
{
    1,                          //report mode
    "iot.gongyuanhezi.cn",   //main server domain
    65003,                    //main server port
    "iot.parkbox.cn",                          //backup server domain
    65003,                          //backup server port
    "",                         //sms gateway
    5,                          //heartbeat interval
    0,                          //random time
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Global Configuration
const PB_CFG_CFG PB_CFG_CFG_DEFAULT =
{
    0x00FF,                 //event mask
    60,                        //inf msg interval
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Time Adjust
const PB_CFG_TMA PB_CFG_TMA_DEFAULT = 
{
    1,                          //auto adjust time mode
    1,                          //offset flag
    8,                          //hour offset
    0,                          //minute offset
    0,                          //daylight saving
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Protocol Watchdog
const PB_CFG_DOG PB_CFG_DOG_DEFAULT = 
{
    0,                          //mode
    0,                          //report dog msg
    0,                          //reboot interval
    0,                          //reboot hour
    0,                          //reboot minute
    0,                          //random time before reboot 
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Air Conditioner Operation
const PB_CFG_ACO PB_CFG_ACO_DEFAULT =
{
    1,                          //pwr mode
    2,                          //work mode
    3,                          //wind level
    0,                          //open interval
    0,                          //open duration
    20,                          //temperature
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Security Configuration
const PB_CFG_SEC PB_CFG_SEC_DEFAULT = 
{
    "xobkraP$#@!5678*&^%1234Parkbox",   //eng pass key 
    "Parkbox*&^%1234$#@!5678xobkraP",   //offline pass key
    "XOBKRAP-PARKBOX@EmbeddedTeamSHA",  //com key
    1,                          //hotp mode
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Output Mode Configuration
const PB_CFG_OMC PB_CFG_OMC_DEFAULT = 
{
    0x00001010,             //Idle output
    0xFFFFFFEF,             //In-service output
    2,                            //Mode
    17,                            //Valid time start hour
    0,                            //Valid time start minute
    22,                            //Valid time stop hour
    0,                            //Valid time stop minute
    0x00003210,             //Valid time idle output
    0xFFFFFFEF,             //Valid time in-service output
    0,                            //align
    {0},                         //pending
    0,                            //tv reboot check
    0                             //crc
};

//Air conditioner working Configuration
const PB_CFG_ACW PB_CFG_ACW_DEFAULT = 
{
    1,                          //mode
    0x01,                     //pwrOnEventMask
    0x01,                     //pwrOffEventMask
    5,                          //duration
    0,                          //Valid time start hour
    0,                           //Valid time start minute
    0,                           //Valid time stop hour
    0,                           //Valid time stop minute
    0,                            //align
    {0},                         //pending
    0                             //crc
};

//Door Alarm
const PB_CFG_DOA PB_CFG_DOA_DEFAULT = 
{
    1,                          //mode
    0,                          //trigger type
    30,                        //debounce duration
    60,                        //alarm msg send interval
    0,                          //align
    {0},                       //pending
    0,                          //door status reverse
    0                           //crc
};

//Smoke Alarm
const PB_CFG_SMA PB_CFG_SMA_DEFAULT = 
{
    2,                          //mode
    50,                        //threshold
    60,                        //debounce duration
    60,                        //alarm msg send interval
    0,                          //align
    {0},                       //pending
    0                           //crc
};

//Multi-media control
const PB_CFG_MUO PB_CFG_MUO_DEFAULT = 
{
    20,                        //volume
    11,                          //auto BGM  11:off 12:on
    0,                          //align
    {0},                       //pending
    0                           //crc
};

/******************************************************************************
* Local Functions
******************************************************************************/

