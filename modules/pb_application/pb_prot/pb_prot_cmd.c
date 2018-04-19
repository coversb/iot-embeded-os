/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_prot_cmd.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   debug command parser
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-18      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os_middleware.h"
#include "os_trace_log.h"
#include "os_task_define.h"
#include "pb_app_config.h"
#include "pb_cfg_proc.h"
#include "pb_prot_cmd.h"
#include "pb_util.h"
#include "hal_board.h"
#include "pb_prot_proc.h"

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
* Function    : pb_prot_cmd_find_param
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : find debug command after '='
******************************************************************************/
static uint8 pb_prot_cmd_find_param(char* pdata, char* param, uint8 paramMaxLen)
{
    char *pFind = NULL;
    uint8 findOffset = 0;

    if (NULL != (pFind = strstr(pdata, "=")))
    {
        pFind++;//skip '='
        while ((findOffset < paramMaxLen) && (*pFind != '\0') && (*pFind != '\r') && (*pFind != '\n'))
        {
            param[findOffset] = *pFind++;
            findOffset++;
        }

        return findOffset;
    }

    return 0;
}

/******************************************************************************
* Function    : pb_prot_cmd_find_hex_param
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_prot_cmd_find_hex_param(char *pPara, uint32 *mod, uint32 *lv)
{
    char *p = NULL;
    uint8 findOffset = 0;
    int32 modLen = 0;

    if (NULL != (p = strstr(pPara, ",")))
    {
        modLen = p - pPara;
        if (modLen <= 0)
        {
            return false;
        }
        *mod = pb_util_hex_string_to_int((uint8*)pPara, modLen);
        p++;//skip ','

        uint8 param[5] = {0};
        uint8 maxLen = strlen(p);
        while ((findOffset < maxLen) && (*p != '\0') && (*p != '\r') && (*p != '\n'))
        {
            param[findOffset] = *p++;
            findOffset++;
        }
        (*lv) = atoi((char*)param);
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_cmd_find_single_param
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get param
******************************************************************************/
static bool pb_prot_cmd_find_single_param(char *pPara, uint8 *num)
{
    char *p = NULL;
    uint8 findOffset = 0;

    if (NULL != (p = strstr(pPara, ",")))
    {
        p++;//skip ','

        uint8 param[5] = {0};
        uint8 maxLen = strlen(p);
        while ((findOffset < maxLen) && (*p != '\0') && (*p != '\r') && (*p != '\n'))
        {
            param[findOffset] = *p++;
            findOffset++;
        }
        (*num) = atoi((char*)param);
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_cmd_find_double_param
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : get 2 params
******************************************************************************/
static bool pb_prot_cmd_find_double_param(char *pPara, uint8 *num, uint8 *subNum)
{
    char *p = NULL;
    uint8 findOffset = 0;

    if (NULL != (p = strstr(pPara, ",")))
    {
        p++;//skip ','

        char param1[5] = {0};
        char param2[5] = {0};
        uint8 maxLen = strlen(p);
        while ((findOffset < maxLen) && (*p != ',') && (*p != '\0') && (*p != '\r') && (*p != '\n'))
        {
            param1[findOffset] = *p++;
            findOffset++;
        }
        if (NULL != (p = strstr(p, ",")))
        {
            p++;//skip ','
            maxLen = strlen(p);
            findOffset = 0;
            while ((findOffset < maxLen) && (*p != '\0') && (*p != '\r') && (*p != '\n'))
            {
                param2[findOffset] = *p++;
                findOffset++;
            }

            *num = atoi(param1);
            *subNum = atoi(param2);
            return true;
        }
    }

    return false;
}

/******************************************************************************
* Function    : pb_prot_cmd_fct
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : fct command
******************************************************************************/
static void pb_prot_cmd_fct(uint8 cmd, char *para)
{
    switch (cmd)
    {
        /********************************
        * RGB Light Blink
        *********************************/
        case PB_PROT_FCT_RED_BLINK:
        {
            OS_INFO("RED BLINK");
            //pb_io_send_rgbbox_msg(PB_IO_RGBBOX_BLINK_RED);
            break;
        }
        case PB_PROT_FCT_GREEN_BLINK:
        {
            OS_INFO("GREEN BLINK");
            //pb_io_send_rgbbox_msg(PB_IO_RGBBOX_BLINK_GREEN);
            break;
        }
        case PB_PROT_FCT_WHITE_BLINK:
        {
            OS_INFO("WHITE BLINK");
            //pb_io_send_rgbbox_msg(PB_IO_RGBBOX_BLINK_WHITE);
            break;
        }
        /********************************
        * Input val check
        *********************************/
        case PB_PROT_FCT_INPUT:
        {
            uint8 pin = 0;
            if (pb_prot_cmd_find_single_param(para, &pin))
            {
                OS_INFO("INPUT%d:%d", pin, 0);//pb_dev_input_val(pin));
            }
            break;
        }
        /********************************
        * Output val set
        *********************************/
        case PB_PROT_FCT_OUTPUT:
        {
            uint8 pin;
            uint8 val;
            if (pb_prot_cmd_find_double_param(para, &pin, &val))
            {
                if (val == 0)
                {
                    //pb_dev_output_set(pin, PB_GPIO_LOW);
                }
                else
                if (val == 1)
                {
                    //pb_dev_output_set(pin, PB_GPIO_HIGH);
                }
                OS_INFO("OUTPUT%d[%d]", pin, val);
            }
            break;
        }
        /********************************
        * Board RELAY set
        *********************************/
        case PB_PROT_FCT_RELAY:
        {
            uint8 relay;
            uint8 sw;
            if (pb_prot_cmd_find_double_param(para, &relay, &sw))
            {
                if (sw == 0)
                {
                    //pb_dev_io_sys_relay(relay, false);
                }
                else if (sw == 1)
                {
                    //pb_dev_io_sys_relay(relay, true);
                }
                OS_INFO("RELAY%d[%d]", relay, sw);
            }
            break;
        }
        /********************************
        * Board INPUT val
        *********************************/
        case PB_PROT_FCT_WG:
        {
            uint8 pin = 0;
            if (pb_prot_cmd_find_single_param(para, &pin))
            {
                OS_INFO("GW_%c:%d", 'A' + pin - 1, 0);//pb_dev_wg_val(pin));
            }
            break;
        }
        /********************************
        * Audio control
        *********************************/
        case PB_PROT_FCT_AUDIO_CHECK:
        {
            bool ret = false;//pb_dev_kt603_available();
            OS_INFO("Audio is %s", (ret == true) ? "available" : "unavailable");
            break;
        }
        case PB_PROT_FCT_AUDIO_VOLUME_SET:
        {
            uint8 volume;
            if (pb_prot_cmd_find_single_param(para, &volume))
            {
                //pb_io_send_audio_msg(PB_DEV_KT603_CTRL_VOLUME, volume);
                OS_INFO("Audio volume[%d]", volume);
            }
            break;
        }
        case PB_PROT_FCT_AUDIO_CTRL:
        {
            uint8 cmd;
            if (pb_prot_cmd_find_single_param(para, &cmd))
            {
                if (cmd != 0)
                {
                    //pb_io_send_audio_msg(cmd, 0);
                    OS_INFO("Audio cmd[%d]", cmd);
                }
            }
            break;
        }
        /********************************
        * Datetime check
        *********************************/
        case PB_PROT_FCT_DATETIME:
        {
            char datetime[16+1] = {0};
            pb_util_get_datetime(datetime, sizeof(datetime));
            OS_INFO("Date time:%s", datetime);
            break;
        }
        /********************************
        * OFFLINE & ENGINEER mode PWD
        *********************************/
        case PB_PROT_FCT_HOTP:
        {
            //pb_util_print_hotp();
            break;
        }
        /********************************
        * Encrypt version check
        *********************************/
        case PB_PROT_FCT_ENCRYPT:
        {
            //pb_util_get_encrypt_version();
            break;
        }
        /********************************
        * HOTP key reset
        *********************************/
        case PB_PROT_FCT_RESET_HOTP_KEY:
        {
            pb_prot_proc_reset_hotp_key();
            OS_INFO("HOTP KEY RESET");
            break;
        }
        /********************************
        * com key reset
        *********************************/
        case PB_PROT_FCT_RESET_COM_KEY:
        {
            pb_prot_proc_reset_com_key();
            OS_INFO("COM KEY RESET");
            break;
        }
        /********************************
        * open/close HOTP for temp
        *********************************/
        case PB_PROT_FCT_HOTP_SW:
        {
            uint8 sw;
            if (pb_prot_cmd_find_single_param(para, &sw))//sw not save in flash
            {
                pb_cfg_proc_get_cmd()->sec.hotpMode = sw;
                OS_INFO("HOTP %d for DEBUG", sw);
            }
            break;
        }
        /********************************
        * Clear all order
        *********************************/
        case PB_PROT_FCT_CLEAR_ORDER:
        {
            //pb_order_local_order_clear();
            break;
        }
        /********************************
        * Show all order
        *********************************/
        case PB_PROT_FCT_SHOW_ORDER:
        {
            //pb_order_local_order_show();
            break;
        }
        /********************************
        * Stack usage
        *********************************/
        case PB_PROT_FCT_STACK:
        {
            os_task_print_free_stack();
            break;
        }
        case PB_PROT_FCT_HEAP:
        {
            os_task_print_free_heap();
            break;
        }
        /********************************
        * Reboot
        *********************************/
        case PB_PROT_FCT_REBOOT:
        {
            OS_INFO("SW REBOOT");
            hal_board_reset();
            break;
        }
        default:break;
    }
}

/******************************************************************************
* Function    : pb_prot_ac
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : control the air conductor
******************************************************************************/
static void pb_prot_cmd_ac(uint32 mode, char *para)
{
    switch (mode)
    {
        case 0:
        {
            OS_INFO("AC CLOSE");
            //pb_dev_ac_close();
            break;
        }
        default:
        {
            uint8 wind;
            uint8 temp;
            if (pb_prot_cmd_find_double_param(para, &wind, &temp))
            {
                OS_INFO("AC mode[%d], wind[%d], temperature[%d]", mode, wind, temp);
                //pb_dev_ac_key((PB_DEV_AC_AIRSPEED_TYPE)((cmd / 1000) % 10), (uint8)((cmd / 10) % 100), (PB_DEV_AC_MODE_TYPE)(cmd % 10));
            }
            break;
        }
    }
}

/******************************************************************************
* Function    : pb_prot_cmd_parse_ascii
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check debug commmand in ascii format
******************************************************************************/
bool pb_prot_cmd_parse_ascii(PB_PROT_RAW_PACKET_TYPE *rawPack)
{
    const uint8 *pdata = NULL;
    
    if ((rawPack->srcID != PB_PROT_SRC_UART)
            || (strlen((char*)rawPack->rawData) < PB_PROT_CMD_ASCII_MIN_LEN))
    {
        return false;
    }

    char *pRawData = (char*)rawPack->rawData;

    //+PBLOG=
    if ((0 == strncmp(pRawData, PB_PROT_DBGCMD_LOG_L, strlen(PB_PROT_DBGCMD_LOG_L)))
            || (0 == strncmp(pRawData, PB_PROT_DBGCMD_LOG_U, strlen(PB_PROT_DBGCMD_LOG_U))))
    {
        char logType[9] = {0};
        uint32 logMod = 0;
        uint32 logLv = 0;
        if (0 != pb_prot_cmd_find_param(pRawData, logType, sizeof(logType)))
        {
            if (pb_prot_cmd_find_hex_param(logType, &logMod, &logLv))
            {
                os_trace_log_set_mod(logMod, logLv);
                OS_INFO("DEBUG LOG %X, LEVEL %d", logMod, logLv);
            }
        }

        return true;
    }
    //+PBUID?
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGREQ_UID_L, strlen(PB_PROT_CFGREQ_UID_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGREQ_UID_U, strlen(PB_PROT_CFGREQ_UID_U))))
    {
        pdata = pb_cfg_proc_get_uid();
        OS_INFO("+PBUID:%02X%02X%02X%02X%02X%02X%02X%02X",
                      pdata[0], pdata[1], pdata[2], pdata[3],
                      pdata[4], pdata[5], pdata[6], pdata[7]);

        return true;
    }
    //+PBUID=
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGCMD_UID_L, strlen(PB_PROT_CFGCMD_UID_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGCMD_UID_U, strlen(PB_PROT_CFGCMD_UID_U))))
    {   
        uint8 uidStr[PB_UID_LEN * 2 + 1] = {0};
        uint8 uid[PB_UID_LEN + 1] = {0};
        uint8 paramLen = 0;
        if (0 != (paramLen = pb_prot_cmd_find_param(pRawData, (char*)uidStr, sizeof(uidStr))))
        {
            if (paramLen == PB_UID_LEN * 2)
            {
                uid[0] = pb_util_hex_string_to_int(&uidStr[0], 2);
                uid[1] = pb_util_hex_string_to_int(&uidStr[2], 2);
                uid[2] = pb_util_hex_string_to_int(&uidStr[4], 2);
                uid[3] = pb_util_hex_string_to_int(&uidStr[6], 2);
                uid[4] = pb_util_hex_string_to_int(&uidStr[8], 2);
                uid[5] = pb_util_hex_string_to_int(&uidStr[10], 2);
                uid[6] = pb_util_hex_string_to_int(&uidStr[12], 2);
                uid[7] = pb_util_hex_string_to_int(&uidStr[14], 2);
                pb_cfg_proc_set_uid(uid);
            }
        }

        pdata = pb_cfg_proc_get_uid();
        OS_INFO("+PBUID:%02X%02X%02X%02X%02X%02X%02X%02X",
                      pdata[0], pdata[1], pdata[2], pdata[3],
                      pdata[4], pdata[5], pdata[6], pdata[7]);

        return true;
    }
    //+PBMAC?
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGREQ_MAC_L, strlen(PB_PROT_CFGREQ_MAC_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGREQ_MAC_U, strlen(PB_PROT_CFGREQ_MAC_U))))
    {
        pdata = pb_cfg_proc_get_mac();
        OS_INFO("+PBMAC:%02X.%02X.%02X.%02X.%02X.%02X",
                      pdata[0], pdata[1], pdata[2],
                      pdata[3], pdata[4], pdata[5]);

        return true;
    }
    //+PBMAC=
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGCMD_MAC_L, strlen(PB_PROT_CFGCMD_MAC_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGCMD_MAC_U, strlen(PB_PROT_CFGCMD_MAC_U))))
    {   
        uint8 macStr[PB_MAC_LEN * 2 + 1];
        uint8 mac[PB_MAC_LEN];
        uint8 paramLen = 0;
        if (0 != (paramLen = pb_prot_cmd_find_param(pRawData, (char*)macStr, sizeof(macStr))))
        {
            if (paramLen == PB_MAC_LEN * 2)
            {
                mac[0] = pb_util_hex_string_to_int(&macStr[0], 2);
                mac[1] = pb_util_hex_string_to_int(&macStr[2], 2);
                mac[2] = pb_util_hex_string_to_int(&macStr[4], 2);
                mac[3] = pb_util_hex_string_to_int(&macStr[6], 2);
                mac[4] = pb_util_hex_string_to_int(&macStr[8], 2);
                mac[5] = pb_util_hex_string_to_int(&macStr[10], 2);
                pb_cfg_proc_set_mac(mac);
            }
        }

        pdata = pb_cfg_proc_get_mac();
        OS_INFO("+PBMAC:%02X.%02X.%02X.%02X.%02X.%02X",
                      pdata[0], pdata[1], pdata[2],
                      pdata[3], pdata[4], pdata[5]);

        return true;
    }
    //+PBSN?
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGREQ_SN_L, strlen(PB_PROT_CFGREQ_SN_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGREQ_SN_U, strlen(PB_PROT_CFGREQ_SN_U))))
    {
        char snStr[PB_SN_LEN + 1];
        memcpy(snStr, pb_cfg_proc_get_sn(), PB_SN_LEN);
        snStr[PB_SN_LEN] = '\0';

        OS_INFO("+PBSN:%s", snStr);

        return true;
    }
    //+PBSN=
    else
    if ((0 == strncmp(pRawData, PB_PROT_CFGCMD_SN_L, strlen(PB_PROT_CFGCMD_SN_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGCMD_SN_U, strlen(PB_PROT_CFGCMD_SN_U))))
    {
        uint8 sn[PB_SN_LEN + 1];
        uint8 paramLen = 0;
        if (0 != (paramLen = pb_prot_cmd_find_param(pRawData, (char*)sn, sizeof(sn))))
        {
            if (paramLen == PB_SN_LEN)
            {
                pb_cfg_proc_set_sn(sn);
            }
        }

        char snStr[PB_SN_LEN + 1];
        memcpy(snStr, pb_cfg_proc_get_sn(), PB_SN_LEN);
        snStr[PB_SN_LEN] = '\0';

        OS_INFO("+PBSN:%s", snStr);

        return true;
    }
    //+PBFCT=
    else 
    if ((0 == strncmp(pRawData, PB_PROT_CFGCMD_FCT_L, strlen(PB_PROT_CFGCMD_FCT_L)))
        || (0 == strncmp(pRawData, PB_PROT_CFGCMD_FCT_U, strlen(PB_PROT_CFGCMD_FCT_U))))
    {
        char fct[10] = {0};
        if (0 != pb_prot_cmd_find_param(pRawData, fct, sizeof(fct)))
        {
            pb_prot_cmd_fct(atoi(fct), fct);
            return true;
        }
    }
    //+PBAC=
    else
    if ((0 == strncmp(pRawData, PB_PROT_CFGCMD_AC_L, strlen(PB_PROT_CFGCMD_AC_L)))
             || (0 == strncmp(pRawData, PB_PROT_CFGCMD_AC_U, strlen(PB_PROT_CFGCMD_AC_U))))
    {
        char ac[10] = {0};
        if (0 != pb_prot_cmd_find_param(pRawData, ac, sizeof(ac)))
        {
            pb_prot_cmd_ac(atoi(ac), ac);
            return true;
        }
    }

    return false;
}

