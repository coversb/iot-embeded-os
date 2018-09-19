/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_util.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   utility functions
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-19      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_UTIL_H__
#define __PB_UTIL_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "pb_app_message.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern uint16 pb_util_get_crc16(const uint8 *pdata, uint16 len);

extern uint8 pb_util_char_to_int(uint8 character);
extern uint32 pb_util_hex_string_to_int(uint8 *string, uint8 len);
extern uint32 pb_util_hex_array_to_int(uint8 *array, uint8 len);
extern int32 pb_util_decimal_string_to_int(char* dec, uint32 multiplier);

extern uint32 pb_util_random_num(uint32 div);

extern uint32 pb_util_get_timestamp(void);
extern void pb_util_set_timestamp(uint32 timestamp);
extern void pb_util_timestamp_to_datetime(char *pdata, uint16 len, uint32 timestamp);
extern void pb_util_get_datetime(char *pdata, uint16 len);
extern void pb_util_get_time(uint8 *hour, uint8 *minute, uint8 *sec);
extern void pb_util_timestamp_to_time(uint32 timestamp, uint8 *hour, uint8 *minute, uint8 *sec);

extern uint8 pb_util_get_indoor_temperature(void);
extern uint8 pb_util_get_indoor_humidity(void);
extern uint16 pb_util_get_indoor_pm25(void);
extern uint16 pb_util_get_bak_bat_voltage(void);

extern bool pb_util_check_is_ip(const char *str, uint16 len);
#endif /* __PB_UTIL_H__ */

