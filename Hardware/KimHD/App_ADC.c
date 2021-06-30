/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#include "App_ADC.h"

__IO uint16_t ADCConvertedValue[4] = {0, 0, 0, 0};

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void ADC1_DMA_Config(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	ADC_DeInit();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);


	DMA_DeInit(DMA2_Stream0);
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCConvertedValue;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 4;

	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);

	/*DMA2_Stream0 ENABLE*/
	DMA_Cmd(DMA2_Stream0, ENABLE);

	/*ADC Common Init*/
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	/*ADC1 Init*/
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 4;								//one time get adc channel
	ADC_Init(ADC1, &ADC_InitStructure);

	/*Enable ADC1 DMA*/
	ADC_DMACmd(ADC1, ENABLE);

	/*ADC1 regular channel18 (vbat) configuration*/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_144Cycles);		//PA.0 B2T
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_480Cycles);		//PA.1 E2T
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_144Cycles);		//PA.2 B1T
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_480Cycles);		//PA.3 E1T

	/*Enable VBAT Channel*/
	ADC_VBATCmd(ENABLE);

	/*Enable dma Request ater last transfer (single_adc_mdoe)*/
	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

	/*Enable ADC1*/
	ADC_Cmd(ADC1, ENABLE);

	/*Start ADC1 Software Conversion*/
	ADC_SoftwareStartConv(ADC1);

}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:获得ADC值 ch:通道值 0~3
*/

uint16_t Get_Adc_DMA(u8 ch)
{
	switch(ch)
	{
		case ADC_Channel_0:
			return(ADCConvertedValue[0]);
		case ADC_Channel_1:
			return(ADCConvertedValue[1]);
		case ADC_Channel_2:
			return(ADCConvertedValue[2]);
		case ADC_Channel_3:
			return(ADCConvertedValue[3]);
		default:
			break;
	}
	return(0);
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
uint16_t Get_Adc_Average_DMA(u8 ch, u8 times)
{
	u32 temp_val = 0;
	u8 t;
	for(t = 0; t < times; t++)
	{
		temp_val += Get_Adc_DMA(ch);
	}
	return temp_val / times;
}



/*
End of files
*/
