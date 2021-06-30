/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#include "App_Timer.h"
#include "App_Language.h"
#include "sm_firmware.h"
#include "bsp.h"
#include "delay.h"
#include "laser.h"

#include "gcodeplayer.h"
#include "laser.h"
#include "lcd_menu.h"
#include "sm_plus.h"
TIM_TimeBaseInitTypeDef  TIM_TimeBaseInitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_OCInitTypeDef	TIM_OCInitStructure;

__IO uint16_t TIM2_CCRV1 = 8000;        //USED TO X,Y,Z,E with printing
__IO uint16_t TIM2_CCRV2 = 1000;				//2K Interrupt
__IO uint16_t TIM2_CCRV3 = 5000;				//400 Hz Interrupt
__IO uint16_t TIM2_CCRV4 = 2000;				//1K Interrupt
USR_TIMER usr_timer;


//zlb add
uint16_t TargetTemperature_millis = 0;
volatile uint8_t TargetTemperatureChangeFlag;
uint8_t TargetBed_millis = 0;
volatile uint8_t TargetBedChangeFlag;
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
#define TIM2_PRE 38
void App_TIM2_Config(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);
	/*Time Base config*/
	#ifdef __TIM2_CCRV_MOTO
	TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;
	#else
	TIM_TimeBaseInitStructure.TIM_Period = 5000;
	#endif

	TIM_TimeBaseInitStructure.TIM_Prescaler = TIM2_PRE;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	/*Time prescaler config*/
	TIM_PrescalerConfig(TIM2, TIM2_PRE, TIM_PSCReloadMode_Immediate);

	#ifdef __TIM2_CCRV_MOTO
	/*tim output compare config*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM2_CCRV1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);

	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM2_CCRV2;
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Disable);

	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM2_CCRV3;
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable);

	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = TIM2_CCRV4;
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Disable);

	#endif

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	/*Enable All interrupt*/
	#ifdef __TIM2_CCRV_MOTO
	TIM2->DIER |= TIM_IT_CC1;//|TIM_IT_CC2|TIM_IT_CC3|TIM_IT_CC4;
	TIM2->CR1 |= TIM_CR1_CEN;
	#else
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 
	TIM_Cmd(TIM2, ENABLE); 
	#endif
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
extern void App_PrintingRead_SD(void);
extern void App_MotoRunning(void);
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
	{
		TIM2->SR = (uint16_t)~TIM_IT_CC1;
		TIM2->DIER &= (uint16_t)~TIM_IT_CC1;
		/*Add User Events*/
		#ifdef __TIM2_CCRV_MOTO
		App_MotoRunning();
		#endif
		TIM2->DIER |= TIM_IT_CC1;
		TIM2->SR = (uint16_t)~TIM_IT_CC1;
		TIM2->CCR1 = TIM2->CNT + TIM2_CCRV1;
	}

	if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
	{
		TIM2->SR = (uint16_t)~TIM_IT_CC2;
		TIM2->CCR2 = TIM2->CCR2 + TIM2_CCRV2;
		/*Add User Events*/


	}
	if(TIM_GetITStatus(TIM2, TIM_IT_CC3) != RESET)
	{
		TIM2->SR = (uint16_t)~TIM_IT_CC3;
		TIM2->CCR3 = TIM2->CNT + TIM2_CCRV3;
		/*Add User Events*/
	}
	if(TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET)
	{
		TIM2->SR = (uint16_t)~TIM_IT_CC4;
		TIM2->CCR4 = TIM2->CNT + TIM2_CCRV4;
		/*Add User Events*/

	}
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		TIM2->SR = (uint16_t)~TIM_IT_Update;
		/*Add User Events*/
		#ifndef __TIM2_CCRV_MOTO
		App_MotoRunning();
		#endif

	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void App_TIM5_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitStructure.TIM_Period = 9;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 8399;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM5, ENABLE);
}


/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:Tim6 system clock 42MHz;
*/
void TIM6_Init(u16 arr, u16 psc)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE); 

	TIM_TimeBaseInitStructure.TIM_Period = arr;
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM6, ENABLE);

}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u8 PwmCount = 0;
extern volatile float current_temperature[EXTRUDERS];
u16 x_pos, y_pos;

void TIM6_DAC_IRQHandler(void)   //TIM4??
{
	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update); 
		FunctionCode = IRQ_2;
		WWDG_SetCounter(WWDG_CNT);
		timer6_millis++;
		PwmCount++;
		if(PrinterMode == 0)
		{
			if(PwmCount < fanSpeed)
			{

				GPIOD->BSRRL = GPIO_Pin_13;
			}
			else
			{
				GPIOD->BSRRH = GPIO_Pin_13;

			}
			if(PwmCount < fanSpeed1)
			{

				GPIOD->BSRRL = GPIO_Pin_12;
			}
			else
			{
				GPIOD->BSRRH = GPIO_Pin_12;
			}
			if(PwmCount < fanSpeed2)
			{
				GPIOD->BSRRL = GPIO_Pin_14;
			}
			else
			{
				GPIOD->BSRRH = GPIO_Pin_14;
			}
		}
		if(UpdateFlg && ((timer6_millis % 1000) == 0))
		{
			x_pos = 8 * UpdateFlg;
			x_pos %= 320;
			y_pos = 8 * UpdateFlg;
			y_pos /= 320;
			y_pos *= 16;
			gui_show_ptchar(x_pos, y_pos, x_pos + 8, y_pos + 16, 0, BLACK, 16, '.', 0);
			UpdateFlg++;
		}
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:Use to System Timer Conter
*/
void App_TIM7_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); //????

	TIM_TimeBaseStructure.TIM_Period = 2000; 
	TIM_TimeBaseStructure.TIM_Prescaler = 41; 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM7, ENABLE);
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
extern CardReader card;
extern int feedmultiply;
uint16_t TimeAdj = 999;
u32 timer7_millis = 0;
u8 BeepCount = 0;


void TIM7_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
		timer7_millis++;
		//beep on
		if(BeepFlg)
		{
			if(timer7_millis > 500)
			{
				timer7_millis = 0;
				GPIO_ToggleBits(GPIOD, GPIO_Pin_3);	
				if(BeepCount > 20)	
				{
					BeepFlg = FALSE;
					BeepCount = 0;
				}
				else
					BeepCount++;
			}

		}
		
		if(TargetTemperatureChangeFlag == 0)
		{
			TargetTemperature_millis++;
			if(TargetTemperature_millis >= 2000)
			{
				TargetTemperatureChangeFlag = 1;
				TargetTemperature_millis = 0;

			}
		}

		if(TargetBedChangeFlag == 0)
		{
			TargetBed_millis++;
			if(TargetBed_millis >= 60)
			{
				TargetBedChangeFlag = 1;
				TargetBed_millis = 0;

			}
		}

		/*Add User Events*/
		if(card.sdprinting)							//Printing Timer
		{
			usr_timer.mmsec++;
			if(usr_timer.mmsec > TimeAdj)
			{
				usr_timer.mmsec = 0;
				usr_timer.sec++;
			}
			if(usr_timer.sec > 59)
			{
				usr_timer.sec = 0;
				usr_timer.min++;
			}
			if(usr_timer.min > 59)
			{
				usr_timer.min = 0;
				usr_timer.hour++;
			}
			if(usr_timer.hour > 24)
			{
				usr_timer.hour = 0;
				usr_timer.date++;
			}
			if(usr_timer.date > 30)
			{
				usr_timer.date = 0;
				usr_timer.mounth++;
			}
			if(usr_timer.mounth > 12)
			{
				usr_timer.mounth = 0;
				usr_timer.year++;
			}
		}
	}
}

/*
End of files
*/
