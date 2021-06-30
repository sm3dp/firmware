#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"





extern volatile unsigned long  timer6_millis;
#define millis() timer6_millis 	 
void delay_init(void);
void TIM6_Init(u16 arr,u16 psc)	;
void delay_ms(u16 nms);
void delay_us(u32 nus);
void delay_second(u8 ns);

#endif





























