/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_gui_main.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   lcd user interface
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_GUI_MAIN_H__
#define __PB_GUI_MAIN_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_util.h"

/******************************************************************************
* Macros
******************************************************************************/


/******************************************************************************
* Enums
******************************************************************************/
typedef enum
{
    PB_GUI_ACT_NONE = 0,
    PB_GUI_ACT_UPDATE,
    PB_GUI_ACT_MENU,
    PB_GUI_ACT_REVERSE,
    PB_GUI_ACT_VOLDOWN,
    PB_GUI_ACT_VOLUP,
    PB_GUI_ACT_UPGRADEMENU,
    PB_GUI_ACT_END
}PB_GUI_ACTIONS;

typedef enum
{
    PB_GUI_MENU_VERSION = 0,
    PB_GUI_MENU_INFO,
    PB_GUI_MENU_VOLUME,
    PB_GUI_MENU_END,
    PB_GUI_MENU_UPGRADE,
} PB_GUI_MENU_ID;

typedef enum
{
    PB_GUI_UP_START = 0,
    PB_GUI_UP_CONNECT_SERVER,
    PB_GUI_UP_START_DOWNLOAD,
    PB_GUI_UP_DOWNLOADING,
    PB_GUI_UP_VERIFY,
    PB_GUI_UP_OK,
    PB_GUI_UP_REBOOT
}PB_GUI_UP_STAGE;

/******************************************************************************
* Types
******************************************************************************/
typedef struct
{
    uint8 cursor;
    uint8 upStage;
    uint8 upProcess;
}PB_GUI_CONTEXT_TYPE;

/******************************************************************************
* Global Variables
******************************************************************************/

/******************************************************************************
* Global Functions
******************************************************************************/
extern void pb_gui_main(void *pvParameters);
extern void pb_gui_send_act_req(PB_GUI_ACTIONS action);
extern void pb_gui_set_upgrade_info(uint8 stage, uint8 process);

#endif /* __PB_GUIOCOL_MAIN_H__ */

