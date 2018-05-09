/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_spi.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   spi driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-2        1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rcc.h"
#include "hal_spi.h"
#include "hal_gpio.h"
#include "board_config.h"

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
* Function    : hal_spi_config
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : config usart function
******************************************************************************/
static void hal_spi_config(SPI_TypeDef * SPIx)
{
    SPI_I2S_DeInit(SPIx);
    SPI_InitTypeDef SPI_InitStruct; 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStruct.SPI_Direction= SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_CRCPolynomial = 7;

    SPI_Init(SPIx, &SPI_InitStruct);
}

#if ( BOARD_SPI2_ENABLE == 1 )
/******************************************************************************
* Function    : hal_spi2_read_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 hal_spi2_read_u8(void)
{
    while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);
    SPI2->DR = 0xFF;
    while((SPI2->SR & SPI_I2S_FLAG_RXNE) == 0);

    return SPI2->DR;
}

/******************************************************************************
* Function    : hal_spi2_write_u8
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_write_u8(uint8 data)
{
    while((SPI2->SR & SPI_I2S_FLAG_TXE) == 0);
    SPI2->DR = data;
    while((SPI2->SR & SPI_I2S_FLAG_RXNE) == 0);
    SPI2->DR;
}

/******************************************************************************
* Function    : hal_spi2_read_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 hal_spi2_read_u16(void)
{
    uint16 data = 0;

    data = hal_spi2_read_u8();
    data <<= 8;
    data |= hal_spi2_read_u8();

    return data;
}

/******************************************************************************
* Function    : hal_spi2_write_u16
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_write_u16(uint16 data)
{
    hal_spi2_write_u8((uint8)(data >> 8));
    hal_spi2_write_u8((uint8)(data & 0x00ff));
}

/******************************************************************************
* Function    : hal_spi2_write
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_write(const uint8 *pdata, const uint16 size)
{
    for(uint16 idx = 0; idx < size; ++idx) 
    {
        hal_spi2_write_u8(pdata[idx]);
    }
}

/******************************************************************************
* Function    : hal_spi2_select
* hal_gpio_set
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_select(bool sw)
{
    if (sw)
    {
        hal_gpio_set(BOARD_SPI2_CS_W5500, HAL_GPIO_LOW);
    }
    else
    {
        hal_gpio_set(BOARD_SPI2_CS_W5500, HAL_GPIO_HIGH);
    }
}

/******************************************************************************
* Function    : hal_spi2_deinit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_deinit(void)
{
    SPI_SSOutputCmd(SPI2, DISABLE);
    SPI_Cmd(SPI2, DISABLE);
}

/******************************************************************************
* Function    : hal_spi2_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_spi2_init(void)
{
    //spi 2 io init
    hal_rcc_enable(BOARD_SPI2_IO_RCC);
    hal_gpio_set_mode(BOARD_SPI2_CLK, GPIO_Mode_AF_PP);
    hal_gpio_set_mode(BOARD_SPI2_MISO, GPIO_Mode_AF_PP);
    hal_gpio_set_mode(BOARD_SPI2_MOSI, GPIO_Mode_AF_PP);
    hal_gpio_set_mode(BOARD_SPI2_CS_W5500, GPIO_Mode_Out_PP);
    //spi 2 init
    hal_rcc_enable(BOARD_SPI2_RCC);
    /*SPI2 config*/
    hal_spi_config(SPI2);
    /*enable spi2*/
    SPI_SSOutputCmd(SPI2, ENABLE);
    SPI_Cmd(SPI2, ENABLE);
}

const HAL_SPI_TYPE hwSPI2 = 
{
    hal_spi2_init,
    hal_spi2_deinit,
    hal_spi2_select,
    hal_spi2_read_u8,
    hal_spi2_write_u8,
    hal_spi2_read_u16,
    hal_spi2_write_u16,
    hal_spi2_write
};

#endif /*BOARD_SPI2_ENABLE*/

