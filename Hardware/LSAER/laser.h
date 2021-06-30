/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#ifndef __LASER_H_
#define __LASER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stm32f4xx.h"

	
	
#define  PL2H GPIO_SetBits(GPIOB ,GPIO_Pin_1)
#define  PL2L GPIO_ResetBits(GPIOB ,GPIO_Pin_1)
#define  DAT  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)
#define  CLK2H GPIO_SetBits(GPIOC ,GPIO_Pin_6)
#define  CLK2L GPIO_ResetBits(GPIOC ,GPIO_Pin_6)

#define LASER_FAN	PDout(13)	//LASER FAN CONTRL 
	

	
	
void laser_init(void);
void HeadType(void);
u8 ReadByte_165(void);
void tim1_init(void);
#ifdef __cplusplus
}
#endif

#endif



/*
End of files
*/
