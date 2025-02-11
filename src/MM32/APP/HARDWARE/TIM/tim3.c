#include "tim3.h" 
#include "led.h"

extern uint8_t Flag_once;

uint16_t ucTim1Flag;

/********************************************************************************************************
**函数信息 ：void TIM2_IRQHandler(void)                  
**功能描述 ：定时器2中断函数
**输入参数 ：无
**输出参数 ：无
********************************************************************************************************/
void TIM3_IRQHandler(void)
{
    /*必须手动清除中断标志位*/
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    
    /*全局标志*/
    ucTim1Flag++;
	
	  if(ucTim1Flag >2)  //300us 没有接收到数据则本次数据接收完成
		{
			 ucTim1Flag = 0;
			 TIM_Cmd(TIM3, DISABLE);
       Flag_Receive = 1;			
			 Flag_once = 0;
		}
	   
//    LED4_TOGGLE() ;
}

/********************************************************************************************************
**函数信息 ：void Tim2_UPCount_test1(u16 Period,u16 Prescaler)                     
**功能描述 ：配置定时器2向上计数模式
**输入参数 ：Period 16位计数器重载值
Prescaler 时钟预分频值
**输出参数 ：无
********************************************************************************************************/
void Tim3_UPCount_test(u16 Prescaler,u16 Period)
{
    TIM_TimeBaseInitTypeDef TIM_StructInit;
    NVIC_InitTypeDef NVIC_StructInit;
    
    /*使能TIM1时钟,默认时钟源为PCLK1(PCLK1未分频时不倍频,否则由PCLK1倍频输出),可选其它时钟源*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    
    TIM_StructInit.TIM_Period=Period;                                                                      //ARR寄存器值
    TIM_StructInit.TIM_Prescaler=Prescaler;                                                                //预分频值
    /*数字滤波器采样频率,不影响定时器时钟*/
    TIM_StructInit.TIM_ClockDivision=TIM_CKD_DIV1;                                                         //采样分频值
    TIM_StructInit.TIM_CounterMode=TIM_CounterMode_Up;                                                     //计数模式
    TIM_StructInit.TIM_RepetitionCounter=0;
    
    TIM_TimeBaseInit(TIM3, &TIM_StructInit);
    
    /* 配置定时器3中断通道及优先级 */
    NVIC_StructInit.NVIC_IRQChannel=TIM3_IRQn;
    NVIC_StructInit.NVIC_IRQChannelPriority=1;
    NVIC_StructInit.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_StructInit);
    
    /*更新定时器时会产生更新时间,清除标志位*/
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    /*允许定时器2更新中断*/
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM3->CNT = 0;
//    TIM_Cmd(TIM3, ENABLE);	
		
		
}


