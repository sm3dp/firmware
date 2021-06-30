/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#ifndef __APP_ADC_H_
#define __APP_ADC_H_

#ifdef __cplusplus
extern"C"{
#endif

#include "stm32f4xx.h"

#define TEMP_0_PIN	   (Get_Adc_DMA(ADC_Channel_3)>>2)   						// AD3_4	E0_TEMP  A0->A3 ADC1_IN0   -> ADC1_IN3
#define TEMP_1_PIN	   (Get_Adc_DMA(ADC_Channel_1)>>2)   						// AD3_6	E1_TEMP	A6->A1
#define TEMP_2_PIN	   (Get_Adc_DMA(ADC_Channel_0)>>2)							
#define TEMP_BED_PIN   (Get_Adc_DMA(ADC_Channel_2)>>2)  						//C2->A2 ADC1_IN12-> ADC1_IN2


uint16_t Get_Adc_DMA(u8 ch);
void ADC1_DMA_Config(void);
uint16_t Get_Adc_Average_DMA(u8 ch,u8 times);



#ifdef __cplusplus
}
#endif

#endif


/*
End of files
*/
