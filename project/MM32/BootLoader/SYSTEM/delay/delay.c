#include "delay.h"

#define USE_SYSTICK_DELAY 0	 //1为使用systick产生延时，0为普通延时

/*如果使用systick作为延时，一定要先初始化systick，即先调用函数delay_init(u8 pclk2)*/

#if USE_SYSTICK_DELAY
static __IO uint32_t TimingDelay;

extern u32 SystemCoreClock;
/********************************************************************************************************
**函数信息 ：delay_init()                 
**功能描述 ：systick延时函数初始化
**输入参数 ：无
**输出参数 ：无
********************************************************************************************************/
void delay_init()
{
    if (SysTick_Config(SystemCoreClock / 1000))
    { 
        /* Capture error */ 
        while (1);
    }
    /* Configure the SysTick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x0);//SysTick中断优先级设置
}

/********************************************************************************************************
**函数信息 ：TimingDelay_Decrement(void)               
**功能描述 ：以1ms的速度递减
**输入参数 ：pclk2，例如系统时钟为8MHz，参数应传入8
**输出参数 ：无
********************************************************************************************************/
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    { 
        TimingDelay--;
    }
}

/********************************************************************************************************
**函数信息 ：SysTick_Handler(void)                    
**功能描述 ：进入该中断函数后，Systick进行递减
**输入参数 ：无
**输出参数 ：无
********************************************************************************************************/
void SysTick_Handler(void)
{
    TimingDelay_Decrement();
}

/********************************************************************************************************
**函数信息 ：delay_ms(__IO uint32_t nTime)                     
**功能描述 ：程序应用调用延时，使用systick
**输入参数 ：nTime：延时
**输出参数 ：无
********************************************************************************************************/
void delay_ms(__IO uint32_t nTime)
{ 
    TimingDelay = nTime;
    
    while(TimingDelay != 0);
}
#else

/********************************************************************************************************
**函数信息 ：delay_us(__IO uint32_t nTime)                 
**功能描述 ：程序应用调用延时
**输入参数 ：nTime：延时
**输出参数 ：无
********************************************************************************************************/
void delay_us(__IO uint32_t nTime)
{		
    u16 i=0;   
    while(nTime--)   
    {   
        i=10;  //自己定义      
        while(i--);       
    }					 
}
/********************************************************************************************************
**函数信息 ：delay_ms(__IO uint32_t nTime)                     
**功能描述 ：程序应用调用延时
**输入参数 ：nTime：延时
**输出参数 ：无
********************************************************************************************************/
void delay_ms(__IO uint32_t nTime)
{	 		  	  
    u16 i=0;   
    while(nTime--)   
    {      
        i=750;  //自己定义    
        while(i--);        
    }	  	    
} 
#endif








































