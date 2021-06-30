#include "switch.h"
#include "delay.h"
#include "usart.h"
//#include "lcd.h"
//////////////////////////////////////////////////////////////////////////////////
//外部中断0服务程序
void SWITCH_Init(void)
{

	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//初始化XMIN-->GPIOC.11,XMIN-->GPIOC.12,
	//      YMIN-->GPIOG.9,	YMIN-->GPIOG.11,
	//      ZMIN-->GPIOG.12,ZMIN-->GPIOG.13,
	//      S_1-->GPIOG.14,	S_2-->GPIOG.15,
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOG, ENABLE); //使能PORTA,PORTE时钟

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11 | GPIO_Pin_12; //PE2~4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIOC11,12

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
	GPIO_Init(GPIOG, &GPIO_InitStructure);//初始化GPIOG9,11-15


	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);	//使能复用功能时钟


	//GPIOG.14	  中断线以及中断初始化配置  下降沿触发	//S_1
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource14);
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_Line = EXTI_Line14;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	//GPIOG.15	  中断线以及中断初始化配置  下降沿触发	//S_2
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource15);
	EXTI_InitStructure.EXTI_Line = EXTI_Line15;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	  	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//使能开关S_1,S_2所在的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;	//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;					//子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);  	  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	EXTI_ClearITPendingBit(EXTI_Line14);
	EXTI_ClearITPendingBit(EXTI_Line15);
}
u8 SWITCH_Scan(u8 mode)
{
	return 0;// 无按键按下
}


//外部中断15_10服务程序
void EXTI15_10_IRQHandler(void)
{
	if(EXTI_GetITStatus(EXTI_Line14) != RESET)
	{
		delay_ms(10);//消抖
		if(S_1 == 0)	 //开关S_1
		{
		}
		EXTI_ClearITPendingBit(EXTI_Line14);  //清除LINE4上的中断标志位
	}
	else  if(EXTI_GetITStatus(EXTI_Line15) != RESET)
	{
		delay_ms(10);//消抖
		if(S_2 == 0)	 //开关S_2
		{
		}
		EXTI_ClearITPendingBit(EXTI_Line15);  //清除LINE4上的中断标志位
	}

}
