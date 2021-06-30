/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/


#include "laser.h"
#include "lcd_menu.h"

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:激光端口初始化
*/
void HeadType(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);



	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	u8 type = ReadByte_165();

	if((type == 248) || (type == 240))
	{
		PrinterMode = 1;			//laser print mode
	}
	else
	{
		PrinterMode = 0;			// 3d print mode
	}
}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u8 ReadByte_165(void)
{
	u8 i, dat = 0;
	CLK2H;

	GPIO_ResetBits(GPIOB, GPIO_Pin_1);
	delay_us(2);
	GPIO_SetBits(GPIOB, GPIO_Pin_1);

	for(i = 0; i < 8; i++)
	{
		dat = dat << 1;
		if(DAT == 1)
		{
			dat = dat + 1;
		}
		CLK2L;
		delay_us(1);
		CLK2H;
	}
	CLK2L;
	return ~dat;
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:激光系统初始化
*/
void laser_init(void)
{
	if(PrinterMode == 1)
	{
		LASER_FAN = TRUE;
		tim1_init();
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:激光控制使用定时器
*/
void tim1_init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = 99;
	TIM_TimeBaseStructure.TIM_Prescaler = 167;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM1);


	TIM_TimeBaseStructure.TIM_Period = 999;
	TIM_TimeBaseStructure.TIM_Prescaler = 168;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);


	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OCInitStructure.TIM_Pulse = 500;//CCR
	TIM_OCInitStructure.TIM_OCPolarity	 = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity  = TIM_OCNPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
	TIM_OC2Init(TIM1, &TIM_OCInitStructure);

	TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM1, ENABLE); //ARPE 使能

	TIM_Cmd(TIM1, ENABLE);  //使能TIM1
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
}



/*
END OF FILES
*/


