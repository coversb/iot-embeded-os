/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_app_main.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   parkbox app main
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-11      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_board.h"
#include "hal_wdg.h"
#include "hal_bkp.h"
#include "hal_rtc.h"
#include "os_trace_log.h"
#include "os_task_define.h"
#include "pb_app_config.h"

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
* Function    : hardware_init
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void hardware_init()
{
    hal_board_init();
    PB_DEBUG_COM.begin(115200);

    uint16 fmVer = 0x2000;
    OS_INFO("ParkBox V%d.%02d.%02d", (fmVer >> 12), ((fmVer >> 4) & 0xFF), (fmVer & 0x000F));
    OS_INFO("@%s-%s", __DATE__, __TIME__);

    hal_wdg_init();
    hal_bkp_init();
    hal_rtc_init();
    RGBBOX_PWM.init();
    AC_REMOTE_CTRL.init(&devIRCtrl);
}

/******************************************************************************
* Function    : main
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
int main(void)
{
    //os_trace_log_set_mod(0xFF, 3);
    hardware_init();

    os_task_create_all();
    os_task_scheduler();

    return 0;
}

