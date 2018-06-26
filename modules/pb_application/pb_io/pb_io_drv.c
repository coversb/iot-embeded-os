/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_io_drv.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   inuput / output manage
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-29      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "pb_io_drv.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
const static PB_IO_PIN_TYPE PB_INPUT_TABLE[PB_IN_END] = 
{
    {BOARD_IN_RESERVED0},    {BOARD_IN_PWR_SUPPLY},    {BOARD_IN_SMOKE_SENSOR1},    {BOARD_IN_SMOKE_SENSOR2},
    {BOARD_IN_RESERVED4},    {BOARD_IN_RESERVED5},     {BOARD_IN_IR_DETECTOR1},     {BOARD_IN_IR_DETECTOR2},
    {BOARD_IN_RESERVED8},    {BOARD_IN_RESERVED9},     {BOARD_IN_RESERVED10},       {BOARD_IN_DEVLOCK_STATUS},
    {BOARD_IN_RESERVED12},   {BOARD_IN_RESERVED13},    {BOARD_IN_RESERVED14},       {BOARD_IN_RESERVED15},
    {BOARD_IN_EMERGENCY_BTN},{BOARD_IN_DOOR_STATUS},   {BOARD_IN_RESERVED18},       {BOARD_IN_RESERVED19}
};

const static PB_IO_PIN_TYPE PB_OUTPUT_TABLE[PB_OUT_END] = 
{
    {BOARD_OUT_RESERVED0},          {BOARD_OUT_RESERVED1},          {BOARD_OUT_RESERVED2},     {BOARD_OUT_RESERVED3},
    {BOARD_OUT_EXHAUST},            {BOARD_OUT_AIR_CON1},           {BOARD_OUT_AIR_CON2},      {BOARD_OUT_GROUND_PLUG1},
    {BOARD_OUT_GROUND_PLUG2},       {BOARD_OUT_INDOOR_LIGHT},       {BOARD_OUT_INDOOR_TV},     {BOARD_OUT_FRESH_AIR},
    {BOARD_OUT_VENDING_MACHINE},    {BOARD_OUT_EMERGENCY_LIGHT},    {BOARD_OUT_RESERVED14},    {BOARD_OUT_RESERVED15},
    {BOARD_OUT_RESERVED16},         {BOARD_OUT_RESERVED17},         {BOARD_OUT_RESERVED18},    {BOARD_OUT_RESERVED19},
    {BOARD_OUT_RESERVED20},         {BOARD_OUT_RESERVED21},         {BOARD_OUT_RESERVED22},    {BOARD_OUT_RESERVED23},
    {BOARD_OUT_RESERVED24},         {BOARD_OUT_RESERVED25},         {BOARD_OUT_RESERVED26},    {BOARD_OUT_RESERVED27},
    {BOARD_OUT_RESERVED28},         {BOARD_OUT_RESERVED29},         {BOARD_OUT_RESERVED30},    {BOARD_OUT_RESERVED31}
};

const static PB_IO_PIN_TYPE PB_GPO_TABLE[PB_GPO_END] = 
{
    {BOARD_GPO_DEV_LOCK},    {BOARD_GPO_DOOR_LOCK},    {BOARD_GPO_RESERVED2}
};

/******************************************************************************
* Local Functions
******************************************************************************/

/******************************************************************************
* Function    : pb_io_drv_input_val
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
HAL_GPIO_VAL pb_io_drv_input_val(uint8 pin)
{
    if (pin >= PB_IN_END || PB_INPUT_TABLE[pin].group == NULL)
    {
        return HAL_GPIO_LOW;
    }

    return hal_gpio_val(PB_INPUT_TABLE[pin].group, PB_INPUT_TABLE[pin].pin);
}

/******************************************************************************
* Function    : pb_io_drv_input_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init input pin
******************************************************************************/
static void pb_io_drv_input_init(void)
{
    hal_rcc_enable(BOARD_IN_RCC);

    for (uint8 idx = PB_IN_BEGIN; idx < PB_IN_END; ++idx)
    {
        if (PB_INPUT_TABLE[idx].group == NULL)
        {
            continue;
        }
        hal_gpio_set_mode(PB_INPUT_TABLE[idx].group, PB_INPUT_TABLE[idx].pin, HAL_GPIO_IN_FLOATING);
    }
}

/******************************************************************************
* Function    : pb_io_drv_output_val
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get output value
******************************************************************************/
HAL_GPIO_VAL pb_io_drv_output_val(uint8 pin)
{
    if (pin >= PB_OUT_END || PB_OUTPUT_TABLE[pin].group == NULL)
    {
        return HAL_GPIO_LOW;
    }

    return hal_gpio_val(PB_OUTPUT_TABLE[pin].group, PB_OUTPUT_TABLE[pin].pin);
}

/******************************************************************************
* Function    : pb_io_drv_output_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_drv_output_set(uint8 pin, uint8 val)
{
    if (pin >= PB_OUT_END)
    {
        return;
    }

    if (PB_OUTPUT_TABLE[pin].group != NULL)
    {
        hal_gpio_set(PB_OUTPUT_TABLE[pin].group, PB_OUTPUT_TABLE[pin].pin, (HAL_GPIO_VAL)val);
    }
}

/******************************************************************************
* Function    : pb_io_drv_output_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init output pin
******************************************************************************/
static void pb_io_drv_output_init(void)
{
    hal_rcc_enable(BOARD_OUT_RCC);

    for (uint8 idx = PB_OUT_BEGIN; idx < PB_OUT_END; ++idx)
    {
        if (PB_OUTPUT_TABLE[idx].group == NULL)
        {
            continue;
        }
        hal_gpio_set_mode(PB_OUTPUT_TABLE[idx].group, PB_OUTPUT_TABLE[idx].pin, HAL_GPIO_OUT_PP);
    }
}

/******************************************************************************
* Function    : pb_io_drv_gpo_val
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
HAL_GPIO_VAL pb_io_drv_gpo_val(uint8 pin)
{
    if (pin >= PB_GPO_END || PB_GPO_TABLE[pin].group == NULL)
    {
        return HAL_GPIO_LOW;
    }

    return hal_gpio_val(PB_GPO_TABLE[pin].group, PB_GPO_TABLE[pin].pin);
}

/******************************************************************************
* Function    : pb_io_drv_gpo_set
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_drv_gpo_set(uint8 pin, uint8 val)
{
    if (pin >= PB_GPO_END)
    {
        return;
    }

    if (PB_GPO_TABLE[pin].group != NULL)
    {
        hal_gpio_set(PB_GPO_TABLE[pin].group, PB_GPO_TABLE[pin].pin, (HAL_GPIO_VAL)val);
    }
}

/******************************************************************************
* Function    : pb_io_drv_gpo_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_io_drv_gpo_init(void)
{
    hal_rcc_enable(BOARD_GPO_RCC);

    for (uint8 idx = PB_GPO_BEGIN; idx < PB_GPO_END; ++idx)
    {
        if (PB_GPO_TABLE[idx].group == NULL)
        {
            continue;
        }
        hal_gpio_set_mode(PB_GPO_TABLE[idx].group, PB_GPO_TABLE[idx].pin, HAL_GPIO_OUT_PP);
    }
}

/******************************************************************************
* Function    : pb_io_drv_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_io_drv_init(void)
{
    pb_io_drv_gpo_init();
    pb_io_drv_output_init();
    pb_io_drv_input_init();
}

