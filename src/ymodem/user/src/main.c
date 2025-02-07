/******************************************************************************
****版本：1.0.0
****平台：
****日期：2020-07-29
****作者：Qitas
****版权：
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern pFunction Jump_To_Application;
extern uint32_t JumpAddress;


/* Private function prototypes -----------------------------------------------*/
static void iap_init(void);

/* Private functions ---------------------------------------------------------*/

GPIO_InitTypeDef GPIO_InitStructure;
USART_InitTypeDef USART_InitStruct;
USART_InitTypeDef USART_InitStructure;

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void RCC_Configuration(void)
{
  RCC->APB2ENR |= RCC_APB2Periph_USART1 |RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO ;
  // RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1 |RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOB |RCC_APB2Periph_AFIO  , ENABLE);
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/

void GPIO_Configuration(void)
{
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;				     //LED1控制--PB5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;			 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         		 //USART1 TX
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;    		 //复用推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);		    		 //A端口

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	         	 //USART1 RX
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;   	 //复用开漏输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);		         	 //A端口

}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void USART_Config(USART_TypeDef* USARTx)
{
  USART_InitStructure.USART_BaudRate = 9600;						//速率115200bps
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//数据位8位
  USART_InitStructure.USART_StopBits = USART_StopBits_1;			//停止位1位
  USART_InitStructure.USART_Parity = USART_Parity_No;				//无校验位
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   //无硬件流控
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					//收发模式

  /* Configure USART1 */
  USART_Init(USARTx, &USART_InitStructure);							//配置串口参数函数


  /* Enable USART1 Receive and Transmit interrupts */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                    //使能接收中断
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断

  /* Enable the USART1 */
  USART_Cmd(USART1, ENABLE);
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
void key_init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC  , ENABLE);


  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;			 //口线翻转速度为50MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_5;		 // k1 k2 k3
  GPIO_Init(GPIOC, &GPIO_InitStructure);

}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/

void iap_init(void)
{
  RCC_Configuration();											  //系统时钟设置
  GPIO_Configuration();											  //端口初始化

  USART_Config(USART1);											  //串口1初始化
}

/*******************************************************************************
**函数信息 ：
**功能描述 ：
**输入参数 ：无
**输出参数 ：无
*******************************************************************************/
int main(void)
{
	SystemInit();
  FLASH_Unlock();			  /* Flash unlock */
	//key_init();


  /* Initialize Key Button mounted on STM3210X-EVAL board */
  //STM_EVAL_PBInit(Button_KEY, Mode_GPIO);
  /* Test if Key push-button on STM3210X-EVAL Board is pressed */
//  if (GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)  == 0x00)			//k1
	if(1)
  { /* If Key is pressed */

    /* Execute the IAP driver in order to re-program the Flash */
    iap_init();
    SerialPutString("\r\n======================================================================");
    SerialPutString("\r\n=              (C) COPYRIGHT 2009 STMicroelectronics                 =");
    SerialPutString("\r\n=                                                                    =");
    SerialPutString("\r\n=     In-Application Programming Application  (Version 3.1.0)        =");
    SerialPutString("\r\n=                                                                    =");
    SerialPutString("\r\n=                                   By MCD Application Team          =");
    SerialPutString("\r\n======================================================================");
    SerialPutString("\r\n\r\n");
    Main_Menu ();
  }
  /* Keep the user application running */
  else
  {
    /* Test if user code is programmed starting from address "ApplicationAddress" */
    if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    { /* Jump to user application */
      JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) ApplicationAddress);
      Jump_To_Application();
    }
  }

  while (1)
  {


	}
}


#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t* file, uint32_t line)
{
  while (1)
  {
  }
}
#endif


/*-------------------------(C) COPYRIGHT 2020 QITAS --------------------------*/
