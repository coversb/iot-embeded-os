/******************************************************************************
*
*     Copyright (c) 2018 ParkBox Ltd.
*
*******************************************************************************
*  file name:          pb_order_list.c
*  author:              Chen Hao
*  version:             1.00
*  file description:   mamage order by list
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-01-19             1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_trace_log.h"
#include "pb_order_list.h"
#include "pb_util.h"
#include "pb_order_main.h"

#if (PB_ORDER_CONTAINER_LIST == 1)
/******************************************************************************
* Macros
******************************************************************************/
#define PB_ORDER_MAX_SIZE 500

/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static PB_ORDER_LIST pb_order_list;

static bool pb_order_list_remove(PB_PROT_ORDER_TYPE *pOrder);
/******************************************************************************
* Local Functions
******************************************************************************/
/******************************************************************************
* Function    : pb_order_list_expire_check
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : compare start time and current time
******************************************************************************/
static bool pb_order_list_expire(PB_PROT_ORDER_TYPE *pOrder)
{
    uint32 curTimestamp = pb_util_get_timestamp();

    if (pOrder->expireTime < curTimestamp)
    {
        OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "expire[%u], cur[%u]", pOrder->expireTime, curTimestamp);
        return true;
    }

    return false;
}

/******************************************************************************
* Function    : pb_order_list_check_duplicate
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_list_duplicate(PB_PROT_ORDER_TYPE *pOrder)
{
    PB_ORDER_LIST_NODE *p = pb_order_list.header->pNextOrder;

    while (NULL != p)
    {
        if (pOrder->id == p->order.id)
        {
            OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_WARN, "Find duplicate ORDER[%u]", pOrder->id);

            if (0 != memcmp(pOrder, &(p->order), sizeof(PB_PROT_ORDER_TYPE)))
            {
                /*if find list has same order UID but content is different, 
                delete it first and then add it. Make sure the order in list sort by start time*/
                pb_order_list_remove(pOrder);
                OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_WARN, "Find same ORDER[%u], need delete it first", pOrder->id);
                return false;
            }
            else
            {
                return true;
            }
        }

        p = p->pNextOrder;
    }

    return false;
}

/******************************************************************************
* Function    : pb_order_list_get_order_num_by_time
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
uint16 pb_order_list_get_order_num_by_time(uint32 timestamp)
{
    uint16 num = 0;
    
    if (pb_order_list.size == 0)
    {
        return 0;
    }

    PB_ORDER_LIST_NODE *p = pb_order_list.header->pNextOrder;

    while (NULL != p)
    {
        if (timestamp < p->order.startTime)
        {
            break;
        }

        if ((timestamp >= p->order.startTime)
            && (timestamp <= p->order.expireTime))
        {
            num++;
        }

        p = p->pNextOrder;
    }

    return num;
}

/******************************************************************************
* Function    : pb_order_list_is_canceled_pass
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : not implement in order list
******************************************************************************/
static bool pb_order_list_is_canceled_pass(uint32 timestamp, uint32 password)
{
    return false;
}

/******************************************************************************
* Function    : pb_order_list_verify_password
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : check password in order list
******************************************************************************/
static uint8 pb_order_list_verify_password(uint32 timestamp, uint32 password, uint32 *orderID)
{
    if (pb_order_list_is_canceled_pass(timestamp, password))
    {
        return PB_ORDER_VERIFY_PW_CANCELED;
    }

    if (0 == pb_order_list.size)
    {
        return PB_ORDER_VERIFY_PW_UNKNOWN;
    }

    PB_ORDER_LIST_NODE *p = pb_order_list.header->pNextOrder;

    while (NULL != p)
    {
        if (timestamp < p->order.startTime)
        {
            break;
        }
        
        if ((password == p->order.passwd)
            && (timestamp >= p->order.startTime)
            && (timestamp <= p->order.expireTime))
        {
            OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Valid ORDER");

            *orderID = p->order.id;
            if (password >= 10000000)
            {
                return PB_ORDER_VERIFY_PW_ENG;
            }
            else
            {
                return PB_ORDER_VERIFY_PW_VALID;
            }
        }

        p = p->pNextOrder;
    }

    return PB_ORDER_VERIFY_PW_UNKNOWN;
}

/******************************************************************************
* Function    : pb_order_list_print
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description :
******************************************************************************/
static void pb_order_list_print(void)
{
    char startDatetime[24 + 1];
    char expireDatetime[24 + 1];
    PB_ORDER_LIST_NODE *p = pb_order_list.header->pNextOrder;

    OS_INFO("ORDER NUM %d",  pb_order_list.size);
    while (NULL != p)
    {
        pb_util_timestamp_to_datetime(startDatetime, 24, p->order.startTime);
        pb_util_timestamp_to_datetime(expireDatetime, 24, p->order.expireTime);
        OS_INFO("ORDER[%u], time[%s to %s], pass[%d], person[%d], valid[%d]",
                     p->order.id, startDatetime, expireDatetime,
                     p->order.passwd, p->order.personNum, p->order.passwdValidCnt);

        p = p->pNextOrder;
    }
}

/******************************************************************************
* Function    : pb_order_list_clear
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : for test
******************************************************************************/
static void pb_order_list_clear(void)
{
    PB_ORDER_LIST_NODE *p = pb_order_list.header;
    PB_ORDER_LIST_NODE *pSwap;

    while (p != NULL && NULL != p->pNextOrder)
    {
        pSwap = p->pNextOrder;
        p->pNextOrder = pSwap->pNextOrder;
        os_free(pSwap);

        pb_order_list.size--;
    }

    OS_INFO("CLEAR ORDER");
}

/******************************************************************************
* Function    : pb_order_list_update
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : order list update by order expire time
******************************************************************************/
static void pb_order_list_update(void)
{
    if (0 == pb_order_list.size)
    {
        return;
    }

    PB_ORDER_LIST_NODE *p = pb_order_list.header;
    PB_ORDER_LIST_NODE *pSwap;

    while (p != NULL && NULL != p->pNextOrder)
    {
        if (pb_order_list_expire(&(p->pNextOrder->order)))
        {
            OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Delete expire order[%u]", p->pNextOrder->order.id);

            pSwap = p->pNextOrder;
            p->pNextOrder = pSwap->pNextOrder;
            os_free(pSwap);

            pb_order_list.size--;
        }
        else
        {
            p = p->pNextOrder;
        }
    }
}

/******************************************************************************
* Function    : pb_order_list_remove
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static bool pb_order_list_remove(PB_PROT_ORDER_TYPE *pOrder)
{
    if (NULL == pOrder)
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "bad order");
        return false;
    }

    if (0 == pb_order_list.size)
    {
        return false;
    }

    PB_ORDER_LIST_NODE *p = pb_order_list.header;
    PB_ORDER_LIST_NODE *pSwap;

    while (p != NULL && NULL != p->pNextOrder)
    {
        if (pOrder->id == p->pNextOrder->order.id)
        {
            OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Delete order[%u]", p->pNextOrder->order.id);

            pSwap = p->pNextOrder;
            p->pNextOrder = pSwap->pNextOrder;
            os_free(pSwap);

            pb_order_list.size--;
            break;
        }
        else
        {
            p = p->pNextOrder;
        }
    }

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Order total[%d]", pb_order_list.size);

    return true;
}

/******************************************************************************
* Function    : pb_order_list_add
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : add order to list
******************************************************************************/
static bool pb_order_list_add(PB_PROT_ORDER_TYPE *pOrder)
{
    if (pOrder == NULL || pb_order_list_expire(pOrder))
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "bad order");
        return false;
    }
    
    if (pb_order_list.size >= PB_ORDER_MAX_SIZE)
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "order list full %d", pb_order_list.size);
        return false;
    }

    if (pb_order_list_duplicate(pOrder))
    {
        return false;
    }

    PB_ORDER_LIST_NODE *pNew = (PB_ORDER_LIST_NODE *)os_malloc(sizeof(PB_ORDER_LIST_NODE));

    if (pNew == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "add order malloc err");
        return false;
    }

    PB_ORDER_LIST_NODE *p = pb_order_list.header;
    PB_ORDER_LIST_NODE *pSwap;

    memcpy(&(pNew->order), pOrder, sizeof(PB_PROT_ORDER_TYPE));
    pb_order_list.size++;

    while ((p->pNextOrder != NULL) && (p->pNextOrder->order.startTime < pOrder->startTime))
    {
        OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "START %u < %u", p->pNextOrder->order.startTime, pOrder->startTime);
        p = p->pNextOrder;
    }

    pSwap = p->pNextOrder;
    p->pNextOrder = pNew;
    pNew->pNextOrder = pSwap;

    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Order insert:ID[%u], time[%u-%u], pass[%d], person[%d], valid[%d]",
                            pOrder->id, pOrder->startTime, pOrder->expireTime,
                            pOrder->passwd, pOrder->personNum, pOrder->passwdValidCnt);
    OS_DBG_TRACE(DBG_MOD_PBORDER, DBG_INFO, "Order total[%d]", pb_order_list.size);

    return true;
}

/******************************************************************************
* Function    : pb_order_list_header
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : get first element of order list
******************************************************************************/
static PB_PROT_ORDER_TYPE* pb_order_list_header(void)
{
    return &(pb_order_list.header->pNextOrder->order);
}

/******************************************************************************
* Function    : pb_order_list_get_order_num
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static uint16 pb_order_list_size(void)
{
    return pb_order_list.size;
}

/******************************************************************************
* Function    : pb_order_list_create
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : create order list header
******************************************************************************/
static void pb_order_list_init(void)
{
    pb_order_list.header = (PB_ORDER_LIST_NODE*)os_malloc(sizeof(PB_ORDER_LIST_NODE));
    
    if ( pb_order_list.header == NULL)
    {
        OS_DBG_ERR(DBG_MOD_PBORDER, "order list init err");
        return;
    }
    pb_order_list.header->pNextOrder = NULL;
    pb_order_list.size = 0;
}

const PB_ORDER_LIST_MANAGER PB_ORDER = 
{
    pb_order_list_init,
    pb_order_list_size,
    pb_order_list_header,
    pb_order_list_add,
    pb_order_list_remove,
    pb_order_list_update,
    pb_order_list_clear,
    pb_order_list_print,
    pb_order_list_verify_password,
    pb_order_list_get_order_num_by_time
};
#endif /*PB_ORDER_CONTAINER_LIST*/

