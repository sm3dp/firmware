/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#ifndef __APP_TIMER_H_
#define __APP_TIMER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f4xx.h"

typedef struct
{
	uint16_t mmsec;
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t date;
	uint8_t mounth;
	uint8_t year;
}USR_TIMER;

extern USR_TIMER usr_timer;

extern volatile uint8_t TargetTemperatureChangeFlag;  //zlb add
extern volatile uint8_t TargetBedChangeFlag;//zlb add

extern __IO uint16_t TIM2_CCRV1;
void App_TIM2_Config(void);
void TIM6_Init(u16 arr,u16 psc);
void App_TIM7_Init(void);
void App_TIM5_Init(void);

#ifdef __cplusplus
}
#endif

#endif

/*
End of files
*/
