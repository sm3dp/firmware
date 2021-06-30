#include "bsp.h"

u8 FunctionCode = 0;


u8 WWDG_CNT = 0X7F;
//tr   :T[6:0],
//wr   :W[6:0],
//fprer:
//Fwwdg=PCLK1/(4096*2^fprer).
void WWDG_Init(u8 tr, u8 wr, u32 fprer)
{

	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE); 

	WWDG_CNT = tr & WWDG_CNT; //.
	WWDG_SetPrescaler(fprer); //
	WWDG_SetWindowValue(wr); //
	//	WWDG_SetCounter(WWDG_CNT);//
	WWDG_Enable(WWDG_CNT);  //

	NVIC_InitStructure.NVIC_IRQChannel = WWDG_IRQn; //
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;					//
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //
	NVIC_Init(&NVIC_InitStructure);

	WWDG_ClearFlag();//
	WWDG_EnableIT();//
}




void WWDG_IRQHandler(void)
{
	WWDG_SetCounter(WWDG_CNT); //
	WWDG_ClearFlag();//
	WWDG_SetCounter(WWDG_CNT);	//
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, (uint8_t)'*');
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, (uint8_t)FunctionCode);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
	USART_SendData(USART1, (uint8_t)'*');
}


void gpio_sddet_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}



