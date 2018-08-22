/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          board_config.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   bsp header files and macro to enable/disable featrures for stm32f427
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-6-21      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "stm32f4xx_conf.h"
#include "misc.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_iwdg.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rtc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_spi.h"
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
#define BOARD_HW_VERSION 0x0200 //V2.00

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
#define BOARD_UART7_ENABLE 1     //BLE module
#define BOARD_SW_I2C1_ENABLE 1
#define BOARD_SPI2_ENABLE 1
#define BOARD_TIM1_PWM_ENABLE 1
#define BOARD_TIM2_ENABLE 1
#define BOARD_TIM3_PWM_ENABLE 0

#endif

/******************************************************************************
* Macros
******************************************************************************/
#define BOARD_NVIC_PRIO_GROUP NVIC_PriorityGroup_1

#define BORAD_FLASH_VOLTAGE VoltageRange_3

//peripheral
/*internal wathdog*/
#define BOARD_IWDG_ENABLE 1 //internal watchdog
/*external watchdog*/
#define BOARD_EWDG_ENABLE 0 //external watchdog. this hw version has no external wdg
//#define BOARD_EXT_WDG_IO_RCC RCC_APB2Periph_GPIOB, BOARD_RCC_APB2
//#define BOARD_EXT_WDG_PIN GPIOB, GPIO_Pin_1

/*BKP*/
#define BOARD_BKP_RCC RCC_APB1Periph_PWR, BOARD_RCC_APB1
#define BOARD_BKP_RTC_ADDR RTC_BKP_DR0
#define BOARD_BKP_DEV_CRC_ADDR RTC_BKP_DR1
#define BOARD_BKP_DEV_ADDR RTC_BKP_DR2

/*RTC*/
#define BOARD_RTC_BKP_SETFLAG  0x5A5A
#define BOARD_RTC_DEFAULT  1525104000   //2018-05-01 00:00:00

/*USART1*/
#define BOARD_IQR_PRIO_USART1 1
#define BOARD_IQR_SUB_PRIO_USART1 2
#define BOARD_USART1_RCC RCC_APB2Periph_USART1, BOARD_RCC_APB2
#define BOARD_USART1_IO_RCC RCC_AHB1Periph_GPIOA, BOARD_RCC_AHB1
#define BOARD_USART1_TX_AF GPIOA, GPIO_PinSource9, GPIO_AF_USART1
#define BOARD_USART1_RX_AF GPIOA, GPIO_PinSource10, GPIO_AF_USART1
#define BOARD_USART1_TX GPIOA, GPIO_Pin_9
#define BOARD_USART1_RX GPIOA, GPIO_Pin_10

/*USART2*/
#define BOARD_IQR_PRIO_USART2 1
#define BOARD_IQR_SUB_PRIO_USART2 2
#define BOARD_USART2_RCC RCC_APB1Periph_USART2, BOARD_RCC_APB1
#define BOARD_USART2_IO_RCC RCC_AHB1Periph_GPIOA, BOARD_RCC_AHB1
#define BOARD_USART2_TX_AF GPIOA, GPIO_PinSource2, GPIO_AF_USART2
#define BOARD_USART2_RX_AF GPIOA, GPIO_PinSource3, GPIO_AF_USART2
#define BOARD_USART2_TX GPIOA, GPIO_Pin_2
#define BOARD_USART2_RX GPIOA, GPIO_Pin_3
#define BOARD_USART2_RX_BUFFSIZE 32
//PAM8610
#define BOARD_PA_IO_RCC RCC_AHB1Periph_GPIOA, BOARD_RCC_AHB1
#define BOARD_PA_PWR GPIOA, GPIO_Pin_6
//KT603C
#define BOARD_KT603_IO_RCC RCC_AHB1Periph_GPIOA, BOARD_RCC_AHB1
#define BOARD_KT603_RST GPIOA, GPIO_Pin_1
#define BOARD_KT603_DEBUG 0

/*USART3*/
#define BOARD_IQR_PRIO_USART3 1
#define BOARD_IQR_SUB_PRIO_USART3 1
#define BOARD_USART3_RCC RCC_APB1Periph_USART3, BOARD_RCC_APB1
#define BOARD_USART3_IO_RCC RCC_AHB1Periph_GPIOB, BOARD_RCC_AHB1
#define BOARD_USART3_TX_AF GPIOB, GPIO_PinSource10, GPIO_AF_USART3
#define BOARD_USART3_RX_AF GPIOB, GPIO_PinSource11, GPIO_AF_USART3
#define BOARD_USART3_TX GPIOB, GPIO_Pin_10
#define BOARD_USART3_RX GPIOB, GPIO_Pin_11
#define BOARD_USART3_RX_BUFFSIZE 2048
//M26
#define BOARD_M26_IO_RCC RCC_AHB1Periph_GPIOH, BOARD_RCC_AHB1
#define BOARD_M26_PWR GPIOH, GPIO_Pin_10
#define BOARD_M26_PWRKEY GPIOH, GPIO_Pin_9

/*USART5*/
#define BOARD_IQR_PRIO_UART5 1
#define BOARD_IQR_SUB_PRIO_UART5 1
#define BOARD_UART5_RCC RCC_APB1Periph_UART5, BOARD_RCC_APB1
#define BOARD_UART5_IO_RCC RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD, BOARD_RCC_AHB1
#define BOARD_UART5_TX_AF GPIOC, GPIO_PinSource12, GPIO_AF_UART5
#define BOARD_UART5_RX_AF GPIOD, GPIO_PinSource2, GPIO_AF_UART5
#define BOARD_UART5_TX GPIOC, GPIO_Pin_12
#define BOARD_UART5_RX GPIOD, GPIO_Pin_2
#define BOARD_UART5_RX_BUFFSIZE 128

/*USART7*/
#define BOARD_IQR_PRIO_UART7 1
#define BOARD_IQR_SUB_PRIO_UART7 1
#define BOARD_UART7_RCC RCC_APB1Periph_UART7, BOARD_RCC_APB1
#define BOARD_UART7_IO_RCC RCC_AHB1Periph_GPIOF, BOARD_RCC_AHB1
#define BOARD_UART7_TX_AF GPIOF, GPIO_PinSource7, GPIO_AF_UART7
#define BOARD_UART7_RX_AF GPIOF, GPIO_PinSource6, GPIO_AF_UART7
#define BOARD_UART7_TX GPIOF, GPIO_Pin_7
#define BOARD_UART7_RX GPIOF, GPIO_Pin_6
#define BOARD_UART7_RX_BUFFSIZE 2048
//HM-11
#define BOARD_HM11_IO_RCC RCC_AHB1Periph_GPIOF, BOARD_RCC_AHB1
#define BOARD_HM11_RST BOARD_GPO_HM11_RST

/*Software I2C 1*/
#define BOARD_SW_I2C1_RCC RCC_AHB1Periph_GPIOB, BOARD_RCC_AHB1
#define BOARD_SW_I2C1_SCL GPIOB, GPIO_Pin_6
#define BOARD_SW_I2C1_SDA GPIOB, GPIO_Pin_7
//SH1106
#define BOARD_SH1106_IO_RCC RCC_AHB1Periph_GPIOG, BOARD_RCC_AHB1
#define BOARD_SH1106_PWR GPIOG, GPIO_Pin_15

/*SPI2*/
#define BOARD_SPI2_RCC RCC_APB1Periph_SPI2, BOARD_RCC_APB1
#define BOARD_SPI2_IO_RCC RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOD, BOARD_RCC_AHB1
#define BOARD_SPI2_CLK_AF GPIOB, GPIO_PinSource13, GPIO_AF_SPI2
#define BOARD_SPI2_MISO_AF GPIOB, GPIO_PinSource14, GPIO_AF_SPI2
#define BOARD_SPI2_MOSI_AF GPIOB, GPIO_PinSource15, GPIO_AF_SPI2
#define BOARD_SPI2_CLK GPIOB, GPIO_Pin_13
#define BOARD_SPI2_MISO GPIOB, GPIO_Pin_14
#define BOARD_SPI2_MOSI GPIOB, GPIO_Pin_15
#define BOARD_SPI2_CS_W5500 GPIOD, GPIO_Pin_9
//w5500
#define BOARD_W5500_IO_RCC RCC_AHB1Periph_GPIOD, BOARD_RCC_AHB1
#define BOARD_W5500_RST GPIOD, GPIO_Pin_8

/*TIM*/
#define BOARD_TIM1_RCC RCC_APB2Periph_TIM1, BOARD_RCC_APB2
#define BOARD_TIM1_PEROID 4735
#define BOARD_TIM1_CH1_RCC RCC_AHB1Periph_GPIOE, BOARD_RCC_AHB1
#define BOARD_TIM1_CH1_AF GPIOE, GPIO_PinSource9, GPIO_AF_TIM1
#define BOARD_TIM1_CH1 GPIOE, GPIO_Pin_9

#define BOARD_TIM2_RCC RCC_APB1Periph_TIM2, BOARD_RCC_APB1
#define BOARD_TIM2_COUNTER 48
#define BOARD_TIM2_PRESCALER 89
#define BOARD_IQR_PRIO_TIM2 1
#define BOARD_IQR_SUB_PRIO_TIM2 2

#define BOARD_TIM3_RCC RCC_APB1Periph_TIM3, BOARD_RCC_APB1
#define BOARD_TIM3_PEROID 1894
#define BOARD_TIM3_CH2_RCC RCC_APB2Periph_GPIOA, BOARD_RCC_APB2
#define BOARD_TIM3_CH2 GPIOA, GPIO_Pin_7

/*RGB LED*/
#define BOARD_RGBLED_RCC RCC_AHB1Periph_GPIOE, BOARD_RCC_AHB1
#define BOARD_RGBLED_R GPIOE, GPIO_Pin_12
#define BOARD_RGBLED_G GPIOE, GPIO_Pin_11
#define BOARD_RGBLED_B GPIOE, GPIO_Pin_10

/*On-board key*/
#define BOARD_KEY_RCC RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOG|RCC_AHB1Periph_GPIOI, BOARD_RCC_AHB1
#define BOARD_KEY_REVERSE GPIOG, GPIO_Pin_11 //KEY1
#define BOARD_KEY_MENU GPIOB, GPIO_Pin_9   //KEY3
#define BOARD_KEY_VOLUME_UP GPIOB, GPIO_Pin_8 //KEY2
#define BOARD_KEY_VOLUME_DOWN GPIOI, GPIO_Pin_4 //KEY4

/*on-board LED*/
#define BOARD_LED_RCC RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOG, BOARD_RCC_AHB1
#define BOARD_LED_SYS GPIOD, GPIO_Pin_6
#define BOARD_LED_NET GPIOD, GPIO_Pin_7
#define BOARD_LED_SCREEN GPIOG, GPIO_Pin_9
#define BOARD_LED_RESERVED GPIOG, GPIO_Pin_10

/*board input and output pin define*/
#define BOARD_PIN_RESERVED NULL, 0
/*input pin*/
#define BOARD_IN_RCC RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOH|RCC_AHB1Periph_GPIOI, BOARD_RCC_AHB1

#define BOARD_IN_RESERVED0 BOARD_PIN_RESERVED
#define BOARD_IN_PWR_SUPPLY GPIOI, GPIO_Pin_0   //INPUT9
#define BOARD_IN_SMOKE_SENSOR1 GPIOA, GPIO_Pin_15   //INPUT5
#define BOARD_IN_SMOKE_SENSOR2 GPIOI, GPIO_Pin_3    //INPUT6
#define BOARD_IN_RESERVED4 BOARD_PIN_RESERVED
#define BOARD_IN_RESERVED5 BOARD_PIN_RESERVED
#define BOARD_IN_IR_DETECTOR1 GPIOI, GPIO_Pin_2    //INPUT7
#define BOARD_IN_IR_DETECTOR2 GPIOI, GPIO_Pin_1    //INPUT8
#define BOARD_IN_RESERVED8 BOARD_PIN_RESERVED
#define BOARD_IN_RESERVED9 GPIOH, GPIO_Pin_15    //INPUT10
#define BOARD_IN_RESERVED10 GPIOH, GPIO_Pin_14  //INPUT11
#define BOARD_IN_DEVLOCK_STATUS GPIOH, GPIO_Pin_13  //INPUT12
#define BOARD_IN_RESERVED12 GPIOA, GPIO_Pin_12  //INPUT13
#define BOARD_IN_RESERVED13 GPIOA, GPIO_Pin_11   //INPUT14
#define BOARD_IN_RESERVED14 GPIOC, GPIO_Pin_9   //INPUT15
#define BOARD_IN_RESERVED15 GPIOC, GPIO_Pin_8   //INPUT16
#define BOARD_IN_EMERGENCY_BTN GPIOD, GPIO_Pin_4
#define BOARD_IN_DOOR_STATUS GPIOD, GPIO_Pin_5
#define BOARD_IN_RESERVED18 GPIOD, GPIO_Pin_3
#define BOARD_IN_RESERVED19 GPIOD, GPIO_Pin_1

/*output pin*/
#define BOARD_LATCH_RCC RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOF, BOARD_RCC_AHB1

#define BOARD_LATCH_OE01 GPIOF, GPIO_Pin_11
#define BOARD_LATCH_OE02 GPIOC, GPIO_Pin_5
#define BOARD_LATCH_OE03 GPIOB, GPIO_Pin_1
#define BOARD_LATCH_LE01 GPIOA, GPIO_Pin_7
#define BOARD_LATCH_LE02 GPIOC, GPIO_Pin_4
#define BOARD_LATCH_LE03 GPIOB, GPIO_Pin_0

#define BOARD_OUT_RCC RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOF|RCC_AHB1Periph_GPIOH|RCC_AHB1Periph_GPIOI, BOARD_RCC_AHB1

#define BOARD_OUT_RESERVED0 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED1 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED2 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED3 BOARD_PIN_RESERVED
#define BOARD_OUT_EXHAUST GPIOI, GPIO_Pin_5
#define BOARD_OUT_AIR_CON1 GPIOI, GPIO_Pin_6
#define BOARD_OUT_AIR_CON2 GPIOI, GPIO_Pin_7
#define BOARD_OUT_GROUND_PLUG1 GPIOI, GPIO_Pin_8
#define BOARD_OUT_GROUND_PLUG2 GPIOC, GPIO_Pin_13
#define BOARD_OUT_INDOOR_LIGHT GPIOI, GPIO_Pin_9
#define BOARD_OUT_INDOOR_TV GPIOI, GPIO_Pin_10
#define BOARD_OUT_FRESH_AIR GPIOI, GPIO_Pin_11
#define BOARD_OUT_VENDING_MACHINE GPIOF, GPIO_Pin_0
#define BOARD_OUT_EMERGENCY_LIGHT GPIOF, GPIO_Pin_1
#define BOARD_OUT_RESERVED14 GPIOF, GPIO_Pin_2
#define BOARD_OUT_RESERVED15 GPIOF, GPIO_Pin_3
#define BOARD_OUT_RESERVED16 GPIOF, GPIO_Pin_4
#define BOARD_OUT_RESERVED17 GPIOF, GPIO_Pin_5
#define BOARD_OUT_RESERVED18 GPIOF, GPIO_Pin_8
#define BOARD_OUT_RESERVED19 GPIOF, GPIO_Pin_9
#define BOARD_OUT_RESERVED20 GPIOF, GPIO_Pin_10
#define BOARD_OUT_RESERVED21 GPIOC, GPIO_Pin_0
#define BOARD_OUT_RESERVED22 GPIOC, GPIO_Pin_1
#define BOARD_OUT_RESERVED23 GPIOC, GPIO_Pin_2
#define BOARD_OUT_RESERVED24 GPIOC, GPIO_Pin_3
#define BOARD_OUT_RESERVED25 GPIOA, GPIO_Pin_0
#define BOARD_OUT_RESERVED26 GPIOH, GPIO_Pin_2
#define BOARD_OUT_RESERVED27 GPIOH, GPIO_Pin_3
#define BOARD_OUT_RESERVED28 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED29 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED30 BOARD_PIN_RESERVED
#define BOARD_OUT_RESERVED31 BOARD_PIN_RESERVED

/*gpoutput pin*/
#define BOARD_GPO_RCC RCC_AHB1Periph_GPIOE|RCC_AHB1Periph_GPIOF|RCC_AHB1Periph_GPIOG|RCC_AHB1Periph_GPIOH, BOARD_RCC_AHB1

#define BOARD_GPO_DEV_LOCK GPIOE, GPIO_Pin_15
#define BOARD_GPO_DOOR_LOCK GPIOH, GPIO_Pin_6
#define BOARD_GPO_RESERVED2 GPIOE, GPIO_Pin_14
#define BOARD_GPO_RESERVED3 GPIOE, GPIO_Pin_13
#define BOARD_GPO_RESERVED4 GPIOE, GPIO_Pin_12
#define BOARD_GPO_RESERVED5 GPIOE, GPIO_Pin_11
#define BOARD_GPO_RESERVED6 GPIOE, GPIO_Pin_10
#define BOARD_GPO_RESERVED7 GPIOE, GPIO_Pin_9
#define BOARD_GPO_RESERVED8 GPIOE, GPIO_Pin_8
#define BOARD_GPO_RESERVED9 GPIOE, GPIO_Pin_7
#define BOARD_GPO_RESERVED10 GPIOG, GPIO_Pin_1
#define BOARD_GPO_RESERVED11 GPIOG, GPIO_Pin_0
#define BOARD_GPO_RESERVED12 GPIOF, GPIO_Pin_15
#define BOARD_GPO_RESERVED13 GPIOF, GPIO_Pin_14
#define BOARD_GPO_RESERVED14 GPIOF, GPIO_Pin_13
#define BOARD_GPO_HM11_RST GPIOF, GPIO_Pin_12

#endif /* __BOARD_CONFIG_H__ */

