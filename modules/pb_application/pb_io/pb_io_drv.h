/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_drv.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   inuput / output manage
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-29      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_IO_DRV_H__
#define __PB_IO_DRV_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "hal_gpio.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_IN_BEGIN = 0,
    PB_IN_RESERVED0 = PB_IN_BEGIN,    // 0
    PB_IN_PWR_SUPPLY,                 // 1
    PB_IN_SMOKE_SENSOR1,              // 2
    PB_IN_SMOKE_SENSOR2,              // 3
    PB_IN_RESERVED4,                  // 4
    PB_IN_RESERVED5,                  // 5
    PB_IN_IR_DETECTOR1,               // 6
    PB_IN_IR_DETECTOR2,               // 7
    PB_IN_RESERVED8,                  // 8
    PB_IN_RESERVED9,                  // 9
    PB_IN_RESERVED10,                 // 10
    PB_IN_RESERVED11,                 // 11
    PB_IN_RESERVED12,                 // 12
    PB_IN_RESERVED13,                 // 13
    PB_IN_RESERVED14,                 // 14
    PB_IN_RESERVED15,                 // 15
    PB_IN_EMERGENCY_BTN,
    PB_IN_DOOR_STATUS,
    PB_IN_RESERVED18,
    PB_IN_RESERVED19,
    PB_IN_END
}PB_IO_INPUT_ENUM;

typedef enum
{
    PB_OUT_BEGIN = 0,
    PB_OUT_RESERVED0 = PB_OUT_BEGIN,       // 0
    PB_OUT_RESERVED1,                      // 1
    PB_OUT_RESERVED2,                      // 2
    PB_OUT_RESERVED3,                      // 3
    PB_OUT_EXHAUST,                        // 4
    PB_OUT_AIR_CON1,                       // 5
    PB_OUT_AIR_CON2,                       // 6
    PB_OUT_GROUND_PLUG1,                   // 7
    PB_OUT_GROUND_PLUG2,                   // 8
    PB_OUT_INDOOR_LIGHT,                   // 9
    PB_OUT_INDOOR_TV,                      // 10
    PB_OUT_FRESH_AIR,                      // 11
    PB_OUT_VENDING_MACHINE,                // 12
    PB_OUT_EMERGENCY_LIGHT,                // 13
    PB_OUT_ADV_MACHINE1,                     // 14
    PB_OUT_ADV_MACHINE2,                     // 15
    PB_OUT_RESERVED16,                     // 16
    PB_OUT_RESERVED17,                     // 17
    PB_OUT_RESERVED18,                     // 18
    PB_OUT_RESERVED19,                     // 19
    PB_OUT_RESERVED20,                     // 20
    PB_OUT_RESERVED21,                     // 21
    PB_OUT_RESERVED22,                     // 22
    PB_OUT_RESERVED23,                     // 23
    PB_OUT_RESERVED24,                     // 24
    PB_OUT_RESERVED25,                     // 25
    PB_OUT_RESERVED26,                     // 26
    PB_OUT_RESERVED27,                     // 27
    PB_OUT_RESERVED28,                     // 28
    PB_OUT_RESERVED29,                     // 29
    PB_OUT_RESERVED30,                     // 30
    PB_OUT_RESERVED31,                     // 31
    PB_OUT_END,
}PB_IO_OUTPUT_ENUM;

#if defined(BOARD_STM32F4XX)
typedef enum
{
    PB_LATCH_OE_BEGIN = 0,
    PB_LATCH_OE1 = PB_LATCH_OE_BEGIN,
    PB_LATCH_OE2,
    PB_LATCH_OE3,
    PB_LATCH_OE_END
}PB_IO_LATCH_OE_ENUM;

typedef enum
{
    PB_LATCH_LE_BEGIN = 0,
    PB_LATCH_LE1 = PB_LATCH_LE_BEGIN,
    PB_LATCH_LE2,
    PB_LATCH_LE3,
    PB_LATCH_LE_END,
    PB_LATCH_LE_NONE = 0xFF
}PB_IO_LATCH_LE_ENUM;
#endif /*BOARD_STM32F4XX*/

typedef enum
{
    PB_GPO_BEGIN = 0,
    PB_GPO_DEV_LOCK = PB_GPO_BEGIN,
    PB_GPO_DOOR_LOCK,
    PB_GPO_RESERVED2,
    PB_GPO_END
}PB_IO_GPO_NUM;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    GPIO_TypeDef *group;
    uint16 pin;
}PB_IO_PIN_TYPE;

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_io_drv_init(void);
extern void pb_io_drv_gpo_set(uint8 pin, uint8 val);
extern HAL_GPIO_VAL pb_io_drv_gpo_val(uint8 pin);
extern void pb_io_drv_output_set(uint8 pin, uint8 val);
extern HAL_GPIO_VAL pb_io_drv_output_val(uint8 pin);
extern HAL_GPIO_VAL pb_io_drv_input_val(uint8 pin);

#endif /* __PB_IO_DRV_H__ */

