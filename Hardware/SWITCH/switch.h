#ifndef __SWITCH_H
#define __SWITCH_H	 
#include "sys.h"

#define X_MIN  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_11)//行程开关X_MIN
#define X_MAX  GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_12)//行程开关X_MAX
#define Y_MIN  GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_9)//行程开关Y_MIN 
#define Y_MAX  GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_11)//行程开关Y_MAX
#define Z_MIN  GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_12)//行程开关Z_MIN
#define Z_MAX  GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_13)//行程开关Z_MAX
#define S_1	   GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_14)//行程开关S_1
#define S_2    GPIO_ReadInputDataBit(GPIOG,GPIO_Pin_15)//行程开关S_2 
									  
//////////////////////////////////////////////////////////////////////////////////   	 
void SWITCH_Init(void);//外部中断初始化	
u8 SWITCH_Scan(u8);  	//按键扫描函数	 					    
#endif
