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
*  change summary:   2018-06-01     1.00                    Chen Hao
*
******************************************************************************/
#ifndef __PB_ORDER_LIST_H__
#define __PB_ORDER_LIST_H__
/******************************************************************************
* Include Files
******************************************************************************/
#include "basetype.h"
#include "pb_app_config.h"
#include "pb_prot_type.h"

#if (PB_ORDER_CONTAINER_LIST == 1)
/******************************************************************************
* Macros
******************************************************************************/

/******************************************************************************
* Enums
******************************************************************************/

/******************************************************************************
* Types
******************************************************************************/
typedef struct pb_order_list_node
{
    struct pb_order_list_node *pNextOrder;
    PB_PROT_ORDER_TYPE order; 
}PB_ORDER_LIST_NODE;

typedef struct
{
    PB_ORDER_LIST_NODE *header;
    uint16 size;
}PB_ORDER_LIST;

typedef struct
{
    void (*init)(void);
    uint16 (*size)(void);
    PB_PROT_ORDER_TYPE* (*header)(void);
    bool (*add)(PB_PROT_ORDER_TYPE *pOrder);
    bool (*remove)(PB_PROT_ORDER_TYPE * pOrder);
    void (*update)(void);
    void (*clear)(void);
    void (*print)(void);
    uint8 (*verifyPassword)(uint32 timestamp, uint32 password, uint32 *orderID);
    uint16 (*validNumByTime)(uint32 timestamp);
}PB_ORDER_LIST_MANAGER;

/******************************************************************************
* Global Variables
******************************************************************************/
extern const PB_ORDER_LIST_MANAGER PB_ORDER;
#endif /*PB_ORDER_CONTAINER_LIST*/

#endif /* __PB_ORDER_LIST_H__ */

