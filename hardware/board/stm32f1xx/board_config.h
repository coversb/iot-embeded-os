/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          board_config.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   bsp header files and macro to enable/disable featrures fot stm32f103
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-12      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "stm32f10x_conf.h"
#include "misc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_iwdg.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_spi.h"
#include "flash_config.h"

/******************************************************************************
* Enums
******************************************************************************/
enum
{
    BOARD_RCC_AHB = 0x00,
    BOARD_RCC_AHB1 = 0x01,
    BOARD_RCC_AHB2 = 0x02,
    BOARD_RCC_AHB3 = 0x03,
    BOARD_RCC_APB1 = 0x10,
    BOARD_RCC_APB2 = 0x20
};

/******************************************************************************
* Macros control
******************************************************************************/
#define BOARD_HW_VERSION 0x0100 //V1.00

#if defined(PB_BOOTLOADER)
#define BOARD_USART1_ENABLE 1   //USART1 for debug
#define BOARD_USART1_RX_BUFFSIZE 2048
#if defined(PB_BOOTLOADER_DBG)
#define BOARD_USART5_ENABLE 1
#endif /*PB_BOOTLOADER_DBG*/

#elif defined(PB_APPLICATION)
#define BOARD_BKP_ENABLE 1
#define BOARD_RTC_ENABLE 1
#define BOARD_USART1_ENABLE 1   //USART1 for debug
#define BOARD_USART1_RX_BUFFSIZE 256

#define BOARD_USART2_ENABLE 1   //USART2 for KT603 (Audio)
#define BOARD_USART3_ENABLE 1   //USART3 for M26 (GPRS)
#define BOARD_USART5_ENABLE 1   //key board or QR module
#define BOARD_SW_I2C1_ENABLE 1
#define BOARD_SPI2_ENABLE 1
#define BOARD_TIM2_ENABLE 1
#define BOARD_TIM3_PWM_ENABLE 1

#endif

/******************************************************************************
* Macros
******************************************************************************/
#define BOARD_NVIC_PRIO_GROUP NVIC_PriorityGroup_1

//peripheral
/*internal wathdog*/
#define BOARD_IWDG_ENABLE 1 //internal watchdog
/*external watchdog*/
#define BOARD_EWDG_ENABLE 1 //external watchdog
#define BOARD_EXT_WDG_IO_RCC RCC_APB2Periph_GPIOB, BOARD_RCC_APB2
#define BOARD_EXT_WDG_PIN GPIOB, GPIO_Pin_1

/*BKP*/
#define BOARD_BKP_RCC RCC_APB1Periph_PWR|RCC_APB1Periph_BKP, BOARD_RCC_APB1
#define BOARD_BKP_RTC_ADDR BKP_DR1
#define BOARD_BKP_DEV_CRC_ADDR BKP_DR2
#define BOARD_BKP_DEV_ADDR BKP_DR3

/*RTC*/
#define BOARD_RTC_BKP_SETFLAG  0x5A5A
#define BOARD_RTC_DEFAULT  1525104000   //2018-05-01 00:00:00

/*USART1*/
#define BOARD_IQR_PRIO_USART1 1
#define BOARD_IQR_SUB_PRIO_USART1 2
#define BOARD_USART1_RCC RCC_APB2Periph_USART1, BOARD_RCC_APB2
#define BOARD_USART1_IO_RCC RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, BOARD_RCC_APB2
#define BOARD_USART1_TX GPIOA, GPIO_Pin_9
#define BOARD_USART1_RX GPIOA, GPIO_Pin_10

/*USART2*/
#define BOARD_IQR_PRIO_USART2 1
#define BOARD_IQR_SUB_PRIO_USART2 2
#define BOARD_USART2_RCC RCC_APB1Periph_USART2, BOARD_RCC_APB1
#define BOARD_USART2_IO_RCC RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, BOARD_RCC_APB2
#define BOARD_USART2_TX GPIOA, GPIO_Pin_2
#define BOARD_USART2_RX GPIOA, GPIO_Pin_3
#define BOARD_USART2_RX_BUFFSIZE 32
//PAM8610
#define BOARD_PA_IO_RCC RCC_APB2Periph_GPIOE, BOARD_RCC_APB2
#define BOARD_PA_PWR GPIOE, GPIO_Pin_0
//KT603C
#define BOARD_KT603_IO_RCC RCC_APB2Periph_GPIOA, BOARD_RCC_APB2
#define BOARD_KT603_RST GPIOA, GPIO_Pin_1
#define BOARD_KT603_DEBUG 0

/*USART3*/
#define BOARD_IQR_PRIO_USART3 1
#define BOARD_IQR_SUB_PRIO_USART3 1
#define BOARD_USART3_RCC RCC_APB1Periph_USART3, BOARD_RCC_APB1
#define BOARD_USART3_IO_RCC RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, BOARD_RCC_APB2
#define BOARD_USART3_TX GPIOB, GPIO_Pin_10
#define BOARD_USART3_RX GPIOB, GPIO_Pin_11
#define BOARD_USART3_RX_BUFFSIZE 2048
//M26
#define BOARD_M26_IO_RCC RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC, BOARD_RCC_APB2
#define BOARD_M26_PWR GPIOC, GPIO_Pin_6
#define BOARD_M26_PWRKEY GPIOA, GPIO_Pin_8

/*USART5*/
#define BOARD_IQR_PRIO_UART5 1
#define BOARD_IQR_SUB_PRIO_UART5 1
#define BOARD_UART5_RCC RCC_APB1Periph_UART5, BOARD_RCC_APB1
#define BOARD_UART5_IO_RCC RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, BOARD_RCC_APB2
#define BOARD_UART5_TX GPIOC, GPIO_Pin_12
#define BOARD_UART5_RX GPIOD, GPIO_Pin_2
#define BOARD_UART5_RX_BUFFSIZE 128

/*Software I2C 1*/
#define BOARD_SW_I2C1_RCC RCC_APB2Periph_GPIOB, BOARD_RCC_APB2
#define BOARD_SW_I2C1_SCL GPIOB, GPIO_Pin_6
#define BOARD_SW_I2C1_SDA GPIOB, GPIO_Pin_7

/*SPI2*/
#define BOARD_SPI2_RCC RCC_APB1Periph_SPI2, BOARD_RCC_APB1
#define BOARD_SPI2_IO_RCC RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOG|RCC_APB2Periph_AFIO, BOARD_RCC_APB2
#define BOARD_SPI2_CLK GPIOB, GPIO_Pin_13
#define BOARD_SPI2_MISO GPIOB, GPIO_Pin_14
#define BOARD_SPI2_MOSI GPIOB, GPIO_Pin_15
#define BOARD_SPI2_CS_W5500 GPIOG, GPIO_Pin_15
//w5500
#define BOARD_W5500_IO_RCC RCC_APB2Periph_GPIOG, BOARD_RCC_APB2
#define BOARD_W5500_RST GPIOG, GPIO_Pin_14

/*TIM*/
#define BOARD_TIM2_RCC RCC_APB1Periph_TIM2, BOARD_RCC_APB1
#define BOARD_TIM2_COUNTER 48
#define BOARD_TIM2_PRESCALER 71
#define BOARD_IQR_PRIO_TIM2 1
#define BOARD_IQR_SUB_PRIO_TIM2 2

#define BOARD_TIM3_RCC RCC_APB1Periph_TIM3, BOARD_RCC_APB1
#define BOARD_TIM3_PEROID 1894
#define BOARD_TIM3_CH2_RCC RCC_APB2Periph_GPIOA, BOARD_RCC_APB2
#define BOARD_TIM3_CH2 GPIOA, GPIO_Pin_7

/*RGB LED*/
#define BOARD_RGBLED_RCC RCC_APB2Periph_GPIOF, BOARD_RCC_APB2
#define BOARD_RGBLED_R GPIOF, GPIO_Pin_11
#define BOARD_RGBLED_G GPIOF, GPIO_Pin_12
#define BOARD_RGBLED_B GPIOF, GPIO_Pin_13

/*On-board key*/
#define BOARD_KEY_RCC RCC_APB2Periph_GPIOG, BOARD_RCC_APB2
#define BOARD_KEY_REVERSE GPIOG, GPIO_Pin_5 //KEY1
#define BOARD_KEY_MENU GPIOG, GPIO_Pin_4 //KEY2
#define BOARD_KEY_VOLUME_UP GPIOG, GPIO_Pin_3   //KEY3
#define BOARD_KEY_VOLUME_DOWN GPIOG, GPIO_Pin_2 //KEY4

/*on-board LED*/
#define BOARD_LED_RCC RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOG, BOARD_RCC_APB2
#define BOARD_LED_SYS GPIOD, GPIO_Pin_7
#define BOARD_LED_NET GPIOG, GPIO_Pin_9
#define BOARD_LED_SCREEN GPIOG, GPIO_Pin_10
#define BOARD_LED_RESERVED GPIOG, GPIO_Pin_11

/*board input and output pin define*/
#define BOARD_PIN_RESERVED NULL, 0
/*input pin*/
#define BOARD_IN_RCC RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|RCC_APB2Periph_GPIOF|RCC_APB2Periph_GPIOG, BOARD_RCC_APB2

#define BOARD_IN_RESERVED0 GPIOD, GPIO_Pin_8
#define BOARD_IN_PWR_SUPPLY GPIOD, GPIO_Pin_9
#define BOARD_IN_SMOKE_SENSOR1 GPIOD, GPIO_Pin_10
#define BOARD_IN_SMOKE_SENSOR2 GPIOD, GPIO_Pin_11
#define BOARD_IN_RESERVED4 GPIOE, GPIO_Pin_12
#define BOARD_IN_RESERVED5 GPIOE, GPIO_Pin_13
#define BOARD_IN_IR_DETECTOR1 GPIOE, GPIO_Pin_14
#define BOARD_IN_IR_DETECTOR2 GPIOE, GPIO_Pin_15
#define BOARD_IN_RESERVED8 GPIOE, GPIO_Pin_8
#define BOARD_IN_RESERVED9 GPIOE, GPIO_Pin_9
#define BOARD_IN_RESERVED10 GPIOE, GPIO_Pin_10
#define BOARD_IN_DEVLOCK_STATUS GPIOE, GPIO_Pin_11
#define BOARD_IN_RESERVED12 GPIOF, GPIO_Pin_15
#define BOARD_IN_RESERVED13 GPIOG, GPIO_Pin_0
#define BOARD_IN_RESERVED14 GPIOG, GPIO_Pin_1
#define BOARD_IN_RESERVED15 GPIOE, GPIO_Pin_7
#define BOARD_IN_EMERGENCY_BTN GPIOD, GPIO_Pin_1
#define BOARD_IN_DOOR_STATUS GPIOD, GPIO_Pin_3
#define BOARD_IN_RESERVED18 GPIOD, GPIO_Pin_5
#define BOARD_IN_RESERVED19 GPIOD, GPIO_Pin_4

/*output pin*/
#define BOARD_OUT_RCC RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOE|RCC_APB2Periph_GPIOF, BOARD_RCC_APB2

#define BOARD_OUT_RESERVED0 GPIOB, GPIO_Pin_0
#define BOARD_OUT_RESERVED1 GPIOC, GPIO_Pin_5
#define BOARD_OUT_RESERVED2 GPIOC, GPIO_Pin_4
#define BOARD_OUT_RESERVED3 BOARD_PIN_RESERVED // used by ir transmitter
#define BOARD_OUT_EXHAUST GPIOA, GPIO_Pin_6
#define BOARD_OUT_AIR_CON1 GPIOA, GPIO_Pin_5
#define BOARD_OUT_AIR_CON2 GPIOA, GPIO_Pin_4
#define BOARD_OUT_GROUND_PLUG1 GPIOC, GPIO_Pin_3
#define BOARD_OUT_GROUND_PLUG2 GPIOC, GPIO_Pin_2
#define BOARD_OUT_INDOOR_LIGHT GPIOC, GPIO_Pin_1
#define BOARD_OUT_INDOOR_TV GPIOC, GPIO_Pin_0
#define BOARD_OUT_FRESH_AIR GPIOF, GPIO_Pin_10
#define BOARD_OUT_VENDING_MACHINE GPIOF, GPIO_Pin_9
#define BOARD_OUT_EMERGENCY_LIGHT GPIOF, GPIO_Pin_8
#define BOARD_OUT_RESERVED14 GPIOF, GPIO_Pin_7
#define BOARD_OUT_RESERVED15 GPIOF, GPIO_Pin_6
#define BOARD_OUT_RESERVED16 GPIOF, GPIO_Pin_5
#define BOARD_OUT_RESERVED17 GPIOF, GPIO_Pin_4
#define BOARD_OUT_RESERVED18 GPIOF, GPIO_Pin_3
#define BOARD_OUT_RESERVED19 GPIOF, GPIO_Pin_2
#define BOARD_OUT_RESERVED20 GPIOF, GPIO_Pin_1
#define BOARD_OUT_RESERVED21 GPIOF, GPIO_Pin_0
#define BOARD_OUT_RESERVED22 GPIOE, GPIO_Pin_6
#define BOARD_OUT_RESERVED23 GPIOE, GPIO_Pin_5
#define BOARD_OUT_RESERVED24 GPIOE, GPIO_Pin_4
#define BOARD_OUT_RESERVED25 GPIOE, GPIO_Pin_3
#define BOARD_OUT_RESERVED26 GPIOE, GPIO_Pin_2
#define BOARD_OUT_RESERVED27 GPIOE, GPIO_Pin_1
#define BOARD_OUT_RESERVED28 GPIOF, GPIO_Pin_14
#define BOARD_OUT_RESERVED29 BOARD_PIN_RESERVED // used by RGB-R
#define BOARD_OUT_RESERVED30 BOARD_PIN_RESERVED // used by RGB-G
#define BOARD_OUT_RESERVED31 BOARD_PIN_RESERVED // used by RGB-B

/*gpoutput pin*/
#define BOARD_GPO_RCC RCC_APB2Periph_GPIOB, BOARD_RCC_APB2

#define BOARD_GPO_DEV_LOCK GPIOB, GPIO_Pin_9
#define BOARD_GPO_DOOR_LOCK GPIOB, GPIO_Pin_8
#define BOARD_GPO_RESERVED2 GPIOB, GPIO_Pin_5

#endif /* __BOARD_CONFIG_H__ */

