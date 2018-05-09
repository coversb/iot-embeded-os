/******************************************************************************
*        
*     Open source
*        
*******************************************************************************
*  file name:          hal_spi.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   spi driver
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-5-2        1.00                    Chen Hao
*
******************************************************************************/
#ifndef __HAL_SPI_H__
#define __HAL_SPI_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "board_config.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    void (*init)(void);
    void (*deinit)(void);
    void (*select)(bool sw);
    uint8 (*read_u8)(void);
    void (*write_u8)(uint8 data);
    uint16 (*read_u16)(void);
    void (*write_u16)(uint16 data);
    void (*write)(const uint8 *pdata, const uint16 size);
}HAL_SPI_TYPE;

/******************************************************************************
* Extern variable
******************************************************************************/
extern const HAL_SPI_TYPE hwSPI2;

#endif /*__HAL_SPI_H__*/

