/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#include "delay.h"
#include "sys.h"

#include "sm_firmware.h"
#include "bsp.h"

//////////////////////////////////////////////////////////////////////////////////
#if SYSTEM_SUPPORT_UCOS
	#include "includes.h"					//ucos use
#endif



/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
volatile unsigned long timer6_millis = 0;
static u8  fac_us = 0; 
static u16 fac_ms = 0; 

void delay_init()
{

	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//  HCLK/8
	fac_us = SystemCoreClock / 8000000;	//1/8
	fac_ms = (u16)fac_us * 1000; //
}



/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void delay_us(u32 nus)
{
	u32 temp;
	SysTick->LOAD = nus * fac_us; 
	SysTick->VAL = 0x00;      
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;        
	do
	{
		temp = SysTick->CTRL;
	} while(temp & 0x01 && !(temp & (1 << 16)));
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;     
	SysTick->VAL = 0X00;      
}

void delay_ms(u16 nms)
{
	u32 temp;
	SysTick->LOAD = (u32)nms * fac_ms; 
	SysTick->VAL = 0x00;          
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;        
	do
	{
		temp = SysTick->CTRL;
	} while(temp & 0x01 && !(temp & (1 << 16))); 
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;    
	SysTick->VAL = 0X00;     
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void delay_second(u8 ns)
{
	int i;
	while(ns--)
	{
		for(i = 0; i < 10; i++)
		{
			delay_ms(100);
		}
	}
}




