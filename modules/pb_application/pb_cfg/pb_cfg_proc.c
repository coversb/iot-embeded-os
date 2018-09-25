/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_cfg_proc.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   manage configuration saved in flash 
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-17      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <string.h>
#include "os_trace_log.h"
#include "pb_cfg_proc.h"
#include "pb_cfg_default.h"
#include "pb_util.h"

/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_CFG_CMD g_pb_cfg_cmd;
static PB_CFG_DEV_INFO pb_cfg_devinfo;

/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_cfg_proc_load
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint32 pb_cfg_proc_load(uint32 addr, void *pdata, uint32 len)
{
    return PB_FLASH_HDLR.read(addr, (uint8*)pdata, len);
}

/******************************************************************************
* Function    : pb_cfg_porc_save
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static int32 pb_cfg_proc_save(uint32 addr, void *pdata, uint32 len)
{
    return PB_FLASH_HDLR.write(addr, (uint32*)pdata, len);
}

/******************************************************************************
* Function    : pb_cfg_proc_get_dev_type
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint8 pb_cfg_proc_get_dev_type(void)
{
    return (uint8)pb_util_hex_string_to_int((uint8*)pb_cfg_proc_get_sn(), 2);
}

/******************************************************************************
* Function    : pb_cfg_proc_get_hardware_version
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_cfg_proc_get_hardware_version(void)
{
    return (uint16)pb_util_hex_string_to_int((uint8*)pb_cfg_proc_get_sn(), 4);
}

/******************************************************************************
* Function    : pb_cfg_proc_get_sn
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
const uint8* pb_cfg_proc_get_sn(void)
{
    return pb_cfg_devinfo.sn;
}

/******************************************************************************
* Function    : pb_cfg_proc_set_sn
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_cfg_proc_set_sn(uint8 *sn)
{
    uint16 crc;

    memcpy(pb_cfg_devinfo.sn, sn, PB_SN_LEN);
    crc = pb_util_get_crc16((uint8*)&pb_cfg_devinfo, ((uint8*) & (pb_cfg_devinfo.crc) - (uint8*)(&pb_cfg_devinfo)));
    pb_cfg_devinfo.crc = crc;

    pb_cfg_proc_save(PB_CFG_DEVINFO_ADDR, &pb_cfg_devinfo, sizeof(pb_cfg_devinfo));
}

/******************************************************************************
* Function    : pb_cfg_proc_get_uid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
const uint8* pb_cfg_proc_get_uid(void)
{
    return pb_cfg_devinfo.uid;
}

/******************************************************************************
* Function    : pb_cfg_proc_set_uid
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_cfg_proc_set_uid(uint8 *uid)
{
    uint16 crc;

    memcpy(pb_cfg_devinfo.uid, uid, PB_UID_LEN);
    crc = pb_util_get_crc16((uint8*)&pb_cfg_devinfo, ((uint8*) & (pb_cfg_devinfo.crc) - (uint8*)(&pb_cfg_devinfo)));
    pb_cfg_devinfo.crc = crc;

    pb_cfg_proc_save(PB_CFG_DEVINFO_ADDR, &pb_cfg_devinfo, sizeof(pb_cfg_devinfo));
}

/******************************************************************************
* Function    : pb_cfg_proc_get_mac
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
const uint8* pb_cfg_proc_get_mac(void)
{
    return pb_cfg_devinfo.mac;
}

/******************************************************************************
* Function    : pb_cfg_proc_set_mac
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_cfg_proc_set_mac(uint8 *mac)
{
    uint16 crc;

    memcpy(pb_cfg_devinfo.mac, mac, PB_MAC_LEN);
    crc = pb_util_get_crc16((uint8*)&pb_cfg_devinfo, ((uint8*) & (pb_cfg_devinfo.crc) - (uint8*)(&pb_cfg_devinfo)));
    pb_cfg_devinfo.crc = crc;

    pb_cfg_proc_save(PB_CFG_DEVINFO_ADDR, &pb_cfg_devinfo, sizeof(pb_cfg_devinfo));
}

/******************************************************************************
* Function    : pb_cfg_proc_load_devinfo
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_cfg_proc_load_devinfo(void)
{
    uint16 crc;

    pb_cfg_proc_load(PB_CFG_DEVINFO_ADDR, &pb_cfg_devinfo, sizeof(pb_cfg_devinfo));
    crc = pb_util_get_crc16((uint8*)&pb_cfg_devinfo, ((uint8*) & (pb_cfg_devinfo.crc) - (uint8*)(&pb_cfg_devinfo)));

    if (crc != pb_cfg_devinfo.crc)
    {
        OS_DBG_ERR(DBG_MOD_PBCFG, "Devinfo crc[%04X] err, need[%04X]", crc, pb_cfg_devinfo.crc);

        memcpy(pb_cfg_devinfo.sn, PB_CFG_SN_DEFAULT, PB_SN_LEN);
        memcpy(pb_cfg_devinfo.uid, PB_CFG_UID_DEFALUT, PB_UID_LEN);
        memcpy(pb_cfg_devinfo.mac, PB_CFG_MAC_DEFALUT, PB_MAC_LEN);
        pb_cfg_devinfo.crc = pb_util_get_crc16((uint8*)&pb_cfg_devinfo, ((uint8*) & (pb_cfg_devinfo.crc) - (uint8*)(&pb_cfg_devinfo)));

        pb_cfg_proc_save(PB_CFG_DEVINFO_ADDR, &pb_cfg_devinfo, sizeof(pb_cfg_devinfo));

        OS_DBG_ERR(DBG_MOD_PBCFG, "Set default devinfo");
    }
    
    char snStr[PB_SN_LEN+1];
    memcpy(snStr, pb_cfg_devinfo.sn, PB_SN_LEN);
    snStr[PB_SN_LEN] = '\0';
    OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                            "Load devinfo, "
                            "UID[%02X %02X %02X %02X %02X %02X %02X %02X], "
                            "SN[%s], "
                            "MAC[%02X.%02X.%02X.%02X.%02X.%02X]",
                            pb_cfg_devinfo.uid[0], pb_cfg_devinfo.uid[1], pb_cfg_devinfo.uid[2], pb_cfg_devinfo.uid[3],
                            pb_cfg_devinfo.uid[4], pb_cfg_devinfo.uid[5], pb_cfg_devinfo.uid[6], pb_cfg_devinfo.uid[7],
                            snStr,
                            pb_cfg_devinfo.mac[0], pb_cfg_devinfo.mac[1], pb_cfg_devinfo.mac[2],
                            pb_cfg_devinfo.mac[3], pb_cfg_devinfo.mac[4], pb_cfg_devinfo.mac[5]);
}

/******************************************************************************
* Function    : pb_cfg_proc_cmd_crc_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static bool pb_cfg_proc_cmd_crc_check(const void *pHead, const void *pTail)
{
    uint16 crcLen = (uint8*)pTail - (uint8*)pHead;
    const uint16 cfgCRC = *((uint16*)pTail);
    uint16 calCRC;

    calCRC = pb_util_get_crc16((uint8*)pHead, crcLen);

    if (cfgCRC != calCRC)
    {
        OS_DBG_ERR(DBG_MOD_PBCFG, "CRC[%04X], need[%04X]", calCRC, cfgCRC);
        return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_cfg_proc_cmd_valid_check
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint8 pb_cfg_proc_cmd_valid_check(uint8 type)
{
    uint8 ret = PB_CFG_CMD_ERR;

    switch (type)
    {
        case PB_PROT_CMD_APC:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.apc), (void*) & (g_pb_cfg_cmd.apc.crc));
            break;
        }
        case PB_PROT_CMD_SER:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.ser), (void*) & (g_pb_cfg_cmd.ser.crc));
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.cfg), (void*) & (g_pb_cfg_cmd.cfg.crc));
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.tma), (void*) & (g_pb_cfg_cmd.tma.crc));
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.dog), (void*) & (g_pb_cfg_cmd.dog.crc));
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.aco), (void*) & (g_pb_cfg_cmd.aco.crc));
            break;
        }
        case PB_PROT_CMD_SEC:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.sec), (void*) & (g_pb_cfg_cmd.sec.crc));
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.omc), (void*) & (g_pb_cfg_cmd.omc.crc));
            break;
        }
        case PB_PROT_CMD_ACW:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.acw), (void*) & (g_pb_cfg_cmd.acw.crc));
            break;
        }
        case PB_PROT_CMD_OWC:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.owc), (void*) & (g_pb_cfg_cmd.owc.crc));
            break;
        }
        case PB_PROT_CMD_DOA:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.doa), (void*) & (g_pb_cfg_cmd.doa.crc));
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.sma), (void*) & (g_pb_cfg_cmd.sma.crc));
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            ret = pb_cfg_proc_cmd_crc_check((void*) & (g_pb_cfg_cmd.muo), (void*) & (g_pb_cfg_cmd.muo.crc));
            break;
        }
        default:
        {
            ret = PB_CFG_CMD_UNKNOW;
            break;
        }
    }

    return ret;
}

/******************************************************************************
* Function    : pb_cfg_proc_get_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
PB_CFG_CMD* pb_cfg_proc_get_cmd(void)
{
    return &g_pb_cfg_cmd;
}

/******************************************************************************
* Function    : pb_cfg_proc_save_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
bool pb_cfg_proc_save_cmd(uint8 type, void *arg, uint32 size)
{
    uint16 calCRC = 0xFFFF;

    switch (type)
    {
        case PB_PROT_CMD_APC:
        {
            PB_CFG_APC *pApc = (PB_CFG_APC*)arg;
            calCRC = pb_util_get_crc16((uint8*)pApc, ((uint8*) & (pApc->crc) - (uint8*)pApc));
            pApc->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.apc), pApc, size);
            break;
        }
        case PB_PROT_CMD_SER:
        {
            PB_CFG_SER *pSer = (PB_CFG_SER*)arg;
            calCRC = pb_util_get_crc16((uint8*)pSer, ((uint8*) & (pSer->crc) - (uint8*)pSer));
            pSer->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.ser), pSer, size);
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            PB_CFG_CFG *pCfg = (PB_CFG_CFG*)arg;
            calCRC = pb_util_get_crc16((uint8*)pCfg, ((uint8*) & (pCfg->crc) - (uint8*)pCfg));
            pCfg->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.cfg), pCfg, size);
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            PB_CFG_TMA *pTma = (PB_CFG_TMA*)arg;
            calCRC = pb_util_get_crc16((uint8*)pTma, ((uint8*) & (pTma->crc) - (uint8*)pTma));
            pTma->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.tma), pTma, size);
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            PB_CFG_DOG *pDog = (PB_CFG_DOG*)arg;
            calCRC = pb_util_get_crc16((uint8*)pDog, ((uint8*) & (pDog->crc) - (uint8*)pDog));
            pDog->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.dog), pDog, size);
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            PB_CFG_ACO *pAco = (PB_CFG_ACO*)arg;
            calCRC = pb_util_get_crc16((uint8*)pAco, ((uint8*) & (pAco->crc) - (uint8*)pAco));
            pAco->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.aco), pAco, size);
            break;
        }
        case PB_PROT_CMD_SEC:
        {
            PB_CFG_SEC *pSec = (PB_CFG_SEC*)arg;
            calCRC = pb_util_get_crc16((uint8*)pSec, ((uint8*) & (pSec->crc) - (uint8*)pSec));
            pSec->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.sec), pSec, size);
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            PB_CFG_OMC *pOmc = (PB_CFG_OMC*)arg;
            calCRC = pb_util_get_crc16((uint8*)pOmc, ((uint8*) & (pOmc->crc) - (uint8*)pOmc));
            pOmc->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.omc), pOmc, size);
            break;
        }
        case PB_PROT_CMD_ACW:
        {
            PB_CFG_ACW *pAcw = (PB_CFG_ACW*)arg;
            calCRC = pb_util_get_crc16((uint8*)pAcw, ((uint8*) & (pAcw->crc) - (uint8*)pAcw));
            pAcw->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.acw), pAcw, size);
            break;
        }
        case PB_PROT_CMD_OWC:
        {
            PB_CFG_OWC *pOwc = (PB_CFG_OWC*)arg;
            calCRC = pb_util_get_crc16((uint8*)pOwc, ((uint8*) & (pOwc->crc) - (uint8*)pOwc));
            pOwc->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.owc), pOwc, size);
            break;
        }
        case PB_PROT_CMD_DOA:
        {
            PB_CFG_DOA *pDoo = (PB_CFG_DOA*)arg;
            calCRC = pb_util_get_crc16((uint8*)pDoo, ((uint8*) & (pDoo->crc) - (uint8*)pDoo));
            pDoo->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.doa), pDoo, size);
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            PB_CFG_SMA *pSma = (PB_CFG_SMA*)arg;
            calCRC = pb_util_get_crc16((uint8*)pSma, ((uint8*) & (pSma->crc) - (uint8*)pSma));
            pSma->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.sma), pSma, size);
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            PB_CFG_MUO *pMuo = (PB_CFG_MUO*)arg;
            calCRC = pb_util_get_crc16((uint8*)pMuo, ((uint8*) & (pMuo->crc) - (uint8*)pMuo));
            pMuo->crc = calCRC;
            memcpy(&(g_pb_cfg_cmd.muo), pMuo, size);
            break;
        }
        default:
        {
            OS_DBG_ERR(DBG_MOD_PBCFG, "Bad cmd[%02X]", type);
            return false;
        }
    }
    OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, "CMD size %d, [%d]%d, [%04X]", 
                            sizeof(PB_CFG_CMD), type, size, calCRC);

    pb_cfg_proc_save(PB_CFG_CMD_ADDR, &g_pb_cfg_cmd, sizeof(PB_CFG_CMD));

    return true;
}

/******************************************************************************
* Function    : pb_cfg_proc_load_default_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_cfg_proc_load_default_cmd(uint8 type)
{
    switch (type)
    {
        case PB_PROT_CMD_APC:
        {
            memcpy(&(g_pb_cfg_cmd.apc), &PB_CFG_APC_DEFAULT, sizeof(PB_CFG_APC));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.apc), sizeof(PB_CFG_APC));
            break;
        }
        case PB_PROT_CMD_SER:
        {
            memcpy(&(g_pb_cfg_cmd.ser), &PB_CFG_SER_DEFAULT, sizeof(PB_CFG_SER));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.ser), sizeof(PB_CFG_SER));
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            memcpy(&(g_pb_cfg_cmd.cfg), &PB_CFG_CFG_DEFAULT, sizeof(PB_CFG_CFG));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.cfg), sizeof(PB_CFG_CFG));
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            memcpy(&(g_pb_cfg_cmd.tma), &PB_CFG_TMA_DEFAULT, sizeof(PB_CFG_TMA));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.tma), sizeof(PB_CFG_TMA));
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            memcpy(&(g_pb_cfg_cmd.dog), &PB_CFG_DOG_DEFAULT, sizeof(PB_CFG_DOG));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.dog), sizeof(PB_CFG_DOG));
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            memcpy(&(g_pb_cfg_cmd.aco), &PB_CFG_ACO_DEFAULT, sizeof(PB_CFG_ACO));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.aco), sizeof(PB_CFG_ACO));
            break;
        }
        case PB_PROT_CMD_SEC:
        {
            memcpy(&(g_pb_cfg_cmd.sec), &PB_CFG_SEC_DEFAULT, sizeof(PB_CFG_SEC));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.sec), sizeof(PB_CFG_SEC));
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            memcpy(&(g_pb_cfg_cmd.omc), &PB_CFG_OMC_DEFAULT, sizeof(PB_CFG_OMC));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.omc), sizeof(PB_CFG_OMC));
            break;
        }
        case PB_PROT_CMD_ACW:
        {
            memcpy(&(g_pb_cfg_cmd.acw), &PB_CFG_ACW_DEFAULT, sizeof(PB_CFG_ACW));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.acw), sizeof(PB_CFG_ACW));
            break;
        }
        case PB_PROT_CMD_OWC:
        {
            memcpy(&(g_pb_cfg_cmd.owc), &PB_CFG_OWC_DEFAULT, sizeof(PB_CFG_OWC));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.owc), sizeof(PB_CFG_OWC));
            break;
        }
        case PB_PROT_CMD_DOA:
        {
            memcpy(&(g_pb_cfg_cmd.doa), &PB_CFG_DOA_DEFAULT, sizeof(PB_CFG_DOA));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.doa), sizeof(PB_CFG_DOA));
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            memcpy(&(g_pb_cfg_cmd.sma), &PB_CFG_SMA_DEFAULT, sizeof(PB_CFG_SMA));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.sma), sizeof(PB_CFG_SMA));
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            memcpy(&(g_pb_cfg_cmd.muo), &PB_CFG_MUO_DEFAULT, sizeof(PB_CFG_MUO));
            pb_cfg_proc_save_cmd(type, &(g_pb_cfg_cmd.muo), sizeof(PB_CFG_MUO));
            break;
        }
        default:return false;
    }

    return true;
}

/******************************************************************************
* Function    : pb_cfg_proc_print_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_cfg_proc_print_cmd(uint8 type)
{
    switch (type)
    {
        case PB_PROT_CMD_APC:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "APC:apn[%s], usr[%s], pass[%s], DNS1[%d.%d.%d.%d], DNS2[%d.%d.%d.%d], lastDNS[%d.%d.%d.%d]",
                                    g_pb_cfg_cmd.apc.apn,
                                    g_pb_cfg_cmd.apc.usr,
                                    g_pb_cfg_cmd.apc.pass,
                                    g_pb_cfg_cmd.apc.mainDNS[0], g_pb_cfg_cmd.apc.mainDNS[1], g_pb_cfg_cmd.apc.mainDNS[2], g_pb_cfg_cmd.apc.mainDNS[3],
                                    g_pb_cfg_cmd.apc.bkDNS[0], g_pb_cfg_cmd.apc.bkDNS[1], g_pb_cfg_cmd.apc.bkDNS[2], g_pb_cfg_cmd.apc.bkDNS[3],
                                    g_pb_cfg_cmd.apc.lastGoodDNS[0], g_pb_cfg_cmd.apc.lastGoodDNS[1], g_pb_cfg_cmd.apc.lastGoodDNS[2], g_pb_cfg_cmd.apc.lastGoodDNS[3]);
            break;
        }
        case PB_PROT_CMD_SER:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "SER:mode[%d], main[%s:%d], bak[%s:%d], sms[%s], hbp[%d], random[%d]",
                                    g_pb_cfg_cmd.ser.mode,
                                    g_pb_cfg_cmd.ser.mainServer, g_pb_cfg_cmd.ser.mainPort,
                                    g_pb_cfg_cmd.ser.bakServer, g_pb_cfg_cmd.ser.bakPort,
                                    g_pb_cfg_cmd.ser.smsGateway,
                                    g_pb_cfg_cmd.ser.hbpInterval, g_pb_cfg_cmd.ser.randomTime);
            break;
        }
        case PB_PROT_CMD_CFG:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "CFG:event[%04X], inf[%d]",
                                    g_pb_cfg_cmd.cfg.eventMask,
                                    g_pb_cfg_cmd.cfg.infInterval);
            break;
        }
        case PB_PROT_CMD_TMA:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "TMA:mode[%d], time offset[%c %02d:%02d, %d]",
                                    g_pb_cfg_cmd.tma.mode,
                                    (g_pb_cfg_cmd.tma.sign == 0 ? '-' : '+'),
                                    g_pb_cfg_cmd.tma.hour,
                                    g_pb_cfg_cmd.tma.minute,
                                    g_pb_cfg_cmd.tma.daylightSaving);
            break;
        }
        case PB_PROT_CMD_DOG:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "DOG:mode[%d], report[%d], reboot interval[%d], reboot time[%02d:%02d], random[%d]",
                                    g_pb_cfg_cmd.dog.mode, g_pb_cfg_cmd.dog.report,
                                    g_pb_cfg_cmd.dog.interval, g_pb_cfg_cmd.dog.rstHour, g_pb_cfg_cmd.dog.rstMinute,
                                    g_pb_cfg_cmd.dog.randomDuration);
            break;
        }
        case PB_PROT_CMD_ACO:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "ACO:pwr[%d], work[%d], wind[%d], interval[%d], duration[%d], temperature[%d]",
                                    g_pb_cfg_cmd.aco.pwrMode,
                                    g_pb_cfg_cmd.aco.workMode,
                                    g_pb_cfg_cmd.aco.windLevel,
                                    g_pb_cfg_cmd.aco.interval,
                                    g_pb_cfg_cmd.aco.duration,
                                    g_pb_cfg_cmd.aco.temperature);
            break;
        }
        case PB_PROT_CMD_SEC:
        {
            char nk[PB_SEC_KEY_LEN * 2 + 1];
            char sk[PB_SEC_KEY_LEN * 2 + 1];
            char ck[PB_SEC_KEY_LEN * 2 + 1];
            os_trace_get_hex_str((uint8*)nk, sizeof(nk), g_pb_cfg_cmd.sec.normalKey, PB_SEC_KEY_LEN);
            os_trace_get_hex_str((uint8*)sk, sizeof(sk), g_pb_cfg_cmd.sec.serviceKey, PB_SEC_KEY_LEN);
            os_trace_get_hex_str((uint8*)ck, sizeof(ck), g_pb_cfg_cmd.sec.comKey, PB_SEC_KEY_LEN);
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, "SEC:Hotp mode[%d]", g_pb_cfg_cmd.sec.hotpMode);
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, "SEC:normal[%s] ", nk);
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, "SEC:service[%s] ", sk);
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, "SEC:comm[%s] ", ck);
            break;
        }
        case PB_PROT_CMD_OMC:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, 
                                    "OMC:idle[%08X], in-service[%08X], mode[%d, %02d:%02d-%02d:%02d], "
                                    "valid time idle[%08X], valid time in-service[%08X]", 
                                    g_pb_cfg_cmd.omc.idleOutput, g_pb_cfg_cmd.omc.inServiceOutput,
                                    g_pb_cfg_cmd.omc.mode, g_pb_cfg_cmd.omc.startHour, g_pb_cfg_cmd.omc.startMin,
                                    g_pb_cfg_cmd.omc.stopHour, g_pb_cfg_cmd.omc.stopMin,
                                    g_pb_cfg_cmd.omc.validIdleOutput, g_pb_cfg_cmd.omc.validInServiceOutput);
            break;
        }
        case PB_PROT_CMD_ACW:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, 
                                    "ACW:mode[%d], pwr on[%02X], pwr off[%02X], duration[%d], vaild time[%02d:%02d-%02d:%02d]",
                                    g_pb_cfg_cmd.acw.mode,
                                    g_pb_cfg_cmd.acw.pwrOnEventMask, g_pb_cfg_cmd.acw.pwrOffEventMask,
                                    g_pb_cfg_cmd.acw.duration,
                                    g_pb_cfg_cmd.acw.startHour, g_pb_cfg_cmd.acw.startMin,
                                    g_pb_cfg_cmd.acw.stopHour, g_pb_cfg_cmd.acw.stopMin);
            break;
        }
        case PB_PROT_CMD_OWC:
        {
            for (uint8 idx = 0; idx < PB_OWC_SIZE; ++idx)
            {
                OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO, 
                                        "OWC:item[%d], mode[%d], vaild time[%02d:%02d-%02d:%02d]",
                                        idx, g_pb_cfg_cmd.owc.item[idx].mode,
                                        g_pb_cfg_cmd.owc.item[idx].startHour, g_pb_cfg_cmd.owc.item[idx].startMin,
                                        g_pb_cfg_cmd.owc.item[idx].stopHour, g_pb_cfg_cmd.owc.item[idx].stopMin);
            }
            break;
        }

        case PB_PROT_CMD_DOA:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "DOA:mode[%d], trigger[%d], duration[%d], interval[%d]",
                                    g_pb_cfg_cmd.doa.mode,
                                    g_pb_cfg_cmd.doa.triggerType,
                                    g_pb_cfg_cmd.doa.duration,
                                    g_pb_cfg_cmd.doa.interval);
            break;
        }
        case PB_PROT_CMD_SMA:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "SMA:mode[%d], threshold[%d], duration[%d], interval[%d]",
                                    g_pb_cfg_cmd.sma.mode,
                                    g_pb_cfg_cmd.sma.threshold,
                                    g_pb_cfg_cmd.sma.duration,
                                    g_pb_cfg_cmd.sma.interval);
            break;
        }
        case PB_PROT_CMD_MUO:
        {
            OS_DBG_TRACE(DBG_MOD_PBCFG, DBG_INFO,
                                    "MUO:volume[%d], auto BGM[%d]",
                                    g_pb_cfg_cmd.muo.volume, 
                                    g_pb_cfg_cmd.muo.autoBGM);
            break;
        }
        default:break;
    }
}

/******************************************************************************
* Function    : pb_cfg_proc_load_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void pb_cfg_proc_load_cmd(void)
{
    pb_cfg_proc_load(PB_CFG_CMD_ADDR, &g_pb_cfg_cmd, sizeof(PB_CFG_CMD));

    //check validity of cmd
    for (uint16 idx = PB_PROT_CMD_BEGIN; idx < PB_PROT_CMD_END; ++idx)
    {
        if (PB_CFG_CMD_ERR == pb_cfg_proc_cmd_valid_check(idx))
        {
            OS_DBG_ERR(DBG_MOD_PBCFG, "Load[%02X] err, get default conf", idx);
            pb_cfg_proc_print_cmd(idx);
            pb_cfg_proc_load_default_cmd(idx);
        }

        pb_cfg_proc_print_cmd(idx);
    }    
}

/******************************************************************************
* Function    : pb_cfg_proc_reset_all_cmd
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
void pb_cfg_proc_reset_all_cmd(void)
{
    for (uint16 idx = PB_PROT_CMD_BEGIN; idx < PB_PROT_CMD_END; ++idx)
    {
        if (idx == PB_PROT_CMD_APC
            || idx == PB_PROT_CMD_SER
            || idx == PB_PROT_CMD_SEC
            || idx == PB_PROT_CMD_MUO)
        {
            //when reset all config, skip this 4 config
            continue;
        }
        pb_cfg_proc_load_default_cmd(idx);
        pb_cfg_proc_print_cmd(idx);
    }
}

/******************************************************************************
* Function    : pb_cfg_proc_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : init flash handler and load datas from flash
******************************************************************************/
void pb_cfg_proc_init(void)
{
    PB_FLASH_HDLR.init();

    pb_cfg_proc_load_devinfo();
    pb_cfg_proc_load_cmd();    
}

