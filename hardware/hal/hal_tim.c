/******************************************************************************
*        
*     Copyright (c) 2018 ParkBox Ltd.   
*        
*******************************************************************************
*  file name:          hal_tim.h
*  author:              Chen Hao
*  version:             1.00
*  file description:   hal tim
*******************************************************************************
*  revision history:    date               version                  author
*
*  change summary:   2018-4-24      1.00                    Chen Hao
*
******************************************************************************/
/******************************************************************************
* Include Files
******************************************************************************/
#include "hal_rcc.h"
#include "hal_gpio.h"
#include "hal_tim.h"
#include "hal_board.h"

#if (BOARD_TIM2_ENABLE == 1)
/******************************************************************************
* Variables (Extern, Global and Static)
******************************************************************************/
static TIM_CALLBACK tim2_callback = NULL;
static uint32 tim2_cnt = 0;

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == RESET)
    {
        return;
    }
    ++tim2_cnt;
    if (tim2_callback != NULL)
    {
        tim2_callback();
    }

    TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update);
}

/******************************************************************************
* Function    : hal_tim2_micros
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : tim2 start us
******************************************************************************/
static uint32 hal_tim2_micros(void)
{
    return (uint32_t)(tim2_cnt * BOARD_TIM2_COUNTER + TIM_GetCounter(TIM2));
}

/******************************************************************************
* Function    : hal_tim2_delay
*
* Author      : Chen Hao
*
* Parameters  :
*
* Return      :
*
* Description : us delay
******************************************************************************/
static void hal_tim2_delay(uint32 us)
{
    uint32 now, target, start;

    now = hal_tim2_micros();
    target = now + us;
    start = now;

    if (target > now)
    {
        do
        {
            now = hal_tim2_micros();
        } while (now  <= target);
    }
    else
    {
        do
        {
            now = hal_tim2_micros();
        } while (now  > start);
        do
        {
            now = hal_tim2_micros();
        } while (now  <= target);
    }
}

/******************************************************************************
* Function    : hal_tim2_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim2_init(void)
{
    tim2_cnt = 0;

    hal_rcc_enable(BOARD_TIM2_RCC);

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Period = BOARD_TIM2_COUNTER;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    /*irq priority config*/
    hal_board_nvic_set_irq(TIM2_IRQn, BOARD_IQR_PRIO_TIM2, BOARD_IQR_SUB_PRIO_TIM2, ENABLE);
}

/******************************************************************************
* Function    : hal_tim2_deinit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim2_deinit(void)
{
    TIM_Cmd(TIM2, DISABLE);
    hal_board_nvic_set_irq(TIM2_IRQn, BOARD_IQR_PRIO_TIM2, BOARD_IQR_SUB_PRIO_TIM2, DISABLE);
    TIM_DeInit(TIM2);
    
    tim2_callback = NULL;
    tim2_cnt = 0;
}

/******************************************************************************
* Function    : hal_tim2_set_callback
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim2_set_callback(TIM_CALLBACK callback)
{
    if (callback != NULL)
    {
        tim2_callback = callback;
    }
}

const HAL_TIM_TYPE tim2 = 
{
    hal_tim2_init,
    hal_tim2_deinit,
    hal_tim2_set_callback,
    hal_tim2_micros,
    hal_tim2_delay
};

#else /*BOARD_TIM2_ENABLE*/

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == RESET)
    {
        return;
    }

    TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update);
}
#endif /*BOARD_TIM2_ENABLE*/

#if (BOARD_TIM3_PWM_ENABLE == 1)
/******************************************************************************
* Function    : hal_tim3_pwm_init
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : tim3 generates 38K pwm signal
******************************************************************************/
static void hal_tim3_pwm_init(void)
{
    hal_rcc_enable(BOARD_TIM3_CH2_RCC);
    hal_gpio_set_mode(BOARD_TIM3_CH2, GPIO_Mode_AF_PP);

    hal_rcc_enable(BOARD_TIM3_RCC);
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;
    u16 CCR1_Val;

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 1894;       //����ʱ����0������999����Ϊ1000�Σ�Ϊһ����ʱ����
    TIM_TimeBaseStructure.TIM_Prescaler = 0;	    //����Ԥ��Ƶ����Ԥ��Ƶ����Ϊ72MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1 ;	//����ʱ�ӷ�Ƶϵ��������Ƶ(�����ò���)
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //���ϼ���ģʽ
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel1 */
    CCR1_Val = (1894 + 1) * 500 / 1000; /* PWM�źŵ�ƽ����ֵ */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;	    //����ΪPWMģʽ1
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;  //����ʱ������ֵС��CCR1_ValʱΪ�ߵ�ƽ
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = CCR1_Val;	  //���õ�ƽ����ֵ�����һ��ռ�ձȵ�PWM

    TIM_OC2Init(TIM3, &TIM_OCInitStructure);	  //ʹ��ͨ��2
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);  // ʹ��TIM3���ؼĴ���ARR

    /* TIM3 enable counter */
    TIM_Cmd(TIM3, DISABLE); //disable first
}

/******************************************************************************
* Function    : hal_tim3_pwm_deinit
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim3_pwm_deinit(void)
{
    TIM_Cmd(TIM3, DISABLE);
    TIM_DeInit(TIM3);
}

/******************************************************************************
* Function    : hal_tim3_pwm_enable
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim3_pwm_enable(void)
{
    TIM_Cmd(TIM3, ENABLE);
}

/******************************************************************************
* Function    : hal_tim3_pwm_disable
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim3_pwm_disable(void)
{
    TIM_Cmd(TIM3, DISABLE);
}

/******************************************************************************
* Function    : hal_tim3_pwm_generate_event
* 
* Author      : Chen Hao
* 
* Parameters  : 
* 
* Return      : 
* 
* Description : 
******************************************************************************/
static void hal_tim3_pwm_generate_event(void)
{
    TIM_GenerateEvent(TIM3, TIM_EventSource_Update);
}

const HAL_TIM_PWM_TYPE tim3Pwm = 
{
    hal_tim3_pwm_init,
    hal_tim3_pwm_deinit,
    hal_tim3_pwm_enable,
    hal_tim3_pwm_disable,
    hal_tim3_pwm_generate_event
};

#endif /*BOARD_TIM3_PWM_ENABLE*/

