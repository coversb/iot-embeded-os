/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          pb_function_polling.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   polling function process
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-23      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "os_middleware.h"
#include "os_task_define.h" 
#include "os_trace_log.h"
#include "hal_wdg.h"
#include "pb_app_config.h"
#include "pb_function_polling.h"
#include "pb_util.h"
#include "pb_prot_main.h"
#include "pb_gui_main.h"
#include "pb_ota_main.h"
#include "pb_prot_proc.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Local Functions define
******************************************************************************/
/******************************************************************************
* Function    : pb_io_monitor_main
*
* Author      : Chen Hao
*
* Parameters  : command sub-type and args
*
* Return      :
*
* Description : control gpio releated operations
******************************************************************************/
void pb_function_polling_main(void *param)
{
    uint32 recordTime = 0;

    os_set_task_init(OS_TASK_ITEM_PB_FUNCPOLLING);
    os_wait_task_init_sync();

    while (1)
    {
        hal_wdg_feed();

        /*protocol function monitor in 1 second*/
        uint32 curTime = pb_util_get_timestamp();
        // 1 second
        if (curTime - recordTime >= 1)
        {
            recordTime = curTime;
            #if 0
            pb_kt603_tick_msg();
            //check smoke alarm
            pb_io_smoke_alarm_check();
            //check door alarm
            pb_io_door_alarm_check();
            //check power alarm
            pb_io_power_supply_check();
            #endif
            // update OLED info
            pb_gui_send_act_req(PB_GUI_ACT_UPDATE);

            #if 0
            if (pb_ota_doing_fota())
            {
                continue;
            }
            //check output status by config
            pb_io_output_mode_check();
            //check air conditioner switch 
            pb_io_air_conditioner_sw_check();
            #endif
            //pooling in 1s, to check send que
            pb_ota_try_to_send_data();
            pb_ota_try_to_recv_data();
            //check protocol watch dog
            pb_prot_proc_watchdog_check();
            //check hbp send
            pb_prot_proc_hbp_send_check();
            //check inf send
            pb_prot_proc_inf_send_check();
            //check pc sensor
            //pb_dev_pc_monitor();
            //update temperature and humidity
            //pb_io_update_temperature_and_humidity();
        }

        os_scheduler_delay(DELAY_500_MS);
    }
}
