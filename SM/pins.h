#ifndef PINS_H
#define PINS_H

//All these generations of Gen7 supply thermistor power
//via PS_ON, so ignore bad thermistor readings
//#define BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
#if 0
#define X_DIR_PIN     PDout(11) 
#define X_STEP_PIN    PDout(12)     
#define X_ENABLE_PIN  PDout(13) 
#define X_MIN_PIN     PDin(6) 
//#define X_MAX_PIN     PGin(15)    
//#define X_MS1_PIN 	   PAout(14)
//#define X_MS2_PIN 	   PAout(13)
//#define X_MS3_PIN 	   PAout(9)

#define Y_DIR_PIN      PGout(2)
#define Y_STEP_PIN     PGout(3) 
#define Y_ENABLE_PIN   PGout(4)
#define Y_MIN_PIN      PGin(9) 
//#define Y_MAX_PIN      PGin(14)  
//#define Y_MS1_PIN      PGout(8) 
//#define Y_MS2_PIN      PGout(7) 
//#define Y_MS3_PIN      PGout(6) 

#define Z_DIR_PIN      PGout(5) 
#define Z_STEP_PIN     PGout(6) 
#define Z_ENABLE_PIN   PGout(7)
#define Z_MIN_PIN      PGin(10) 
//#define Z_MAX_PIN      PGin(13) 
//#define Z_MS1_PIN      PCout(4)
//#define Z_MS2_PIN      PCout(3)
//#define Z_MS3_PIN      PCout(2)

#define E0_DIR_PIN     PGout(8) 
#define E0_STEP_PIN    PCout(6)         
#define E0_ENABLE_PIN  PCout(7)     
//#define E0_MS1_PIN     PFout(9)
//#define E0_MS2_PIN 	   PFout(8)
//#define E0_MS3_PIN 	   PFout(7)

#define UART_DTR_PIN     PAin(8) 

#define E1_DIR_PIN	   PAout(13) 
#define E1_STEP_PIN    PAout(14)	
#define E1_ENABLE_PIN   PAout(15) 
//#define E1_MS1_PIN      PEout(5) 
//#define E1_MS2_PIN      PEout(4) 
//#define E1_MS3_PIN      PEout(3)

#define  HEATER_0_PIN   TIM8->CCR2	  //E0_PWM T8_2 
//#define HEATER_1_PIN   TIM5->CCR3	  //E1_PWM T5_3
#define  FAN_PIN        TIM5->CCR1	  //BED_FAN T5_1
#define  HEATER_BED_PIN TIM8->CCR3	  //BED_PWM T8_3

#define  E0_FAN       PAout(1) // TIM5->CCR2  	//E0_FAN T5_2 
 
#define TEMP_0_PIN	   (Get_Adc(ADC_Channel_4)>>2)   						// AD3_4	E0_TEMP
#define TEMP_1_PIN	 		(Get_Adc(ADC_Channel_6)>>2)   						// AD3_6	E1_TEMP
#define TEMP_BED_PIN   (Get_Adc(ADC_Channel_5)>>2)   						// AD3_5  BED_TEMP
#endif


#define X_DIR_PIN     PEout(10) //原板 	E15
#define X_STEP_PIN    PEout(11) //原板 	E14 
#define X_ENABLE_PIN  PEout(12)	//原板 	E13
#define X_MIN_PIN     PAin(8)	//原板 	D8

#define X_DIR(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 10) : (GPIOE->BSRRH = 1 << 10))
#define X_STEP(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 11) : (GPIOE->BSRRH = 1 << 11))
#define X_EN(dat)			((dat > 0) ? (GPIOE->BSRRL = 1 << 12) : (GPIOE->BSRRH = 1 << 12))


#define Y_DIR_PIN      PEout(7)	//原板 	E9
#define Y_STEP_PIN     PEout(8)	//原板 	B10 
#define Y_ENABLE_PIN   PEout(9)	//原板 	E10
#define Y_MIN_PIN      PCin(9)	//原板 	D9

#define Y_DIR(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 7) : (GPIOE->BSRRH = 1 << 7))
#define Y_STEP(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 8) : (GPIOE->BSRRH = 1 << 8))
#define Y_EN(dat)			((dat > 0) ? (GPIOE->BSRRL = 1 << 9) : (GPIOE->BSRRH = 1 << 9))


#define Z_DIR_PIN      PCout(1) //原板 	C0
#define Z_STEP_PIN     PCout(2) //原板 	C6
#define Z_ENABLE_PIN   PCout(3) //原板 	C15
#define Z_MIN_PIN      PDin(11)	//原板 	D10
#define Z_MAX_PIN	   PDin(10)	//原板 	D11

//Z_EN(x)		(0:1)		Z_EN_H:ZEN_L;
#define Z_DIR(dat)		((dat > 0) ? (GPIOC->BSRRL = 1 << 1) : (GPIOC->BSRRH = 1 << 1))
#define Z_STEP(dat)		((dat > 0) ? (GPIOC->BSRRL = 1 << 2) : (GPIOC->BSRRH = 1 << 2))
#define Z_EN(dat)			((dat > 0) ? (GPIOC->BSRRL = 1 << 3) : (GPIOC->BSRRH = 1 << 3))


#define E0_DIR_PIN     PCout(13)//原板 	C13 
#define E0_STEP_PIN    PCout(14)//原板 	D12        
#define E0_ENABLE_PIN  PCout(15)//原板 	C14
#define E0_MIN_PIN     PDin(11)	//E1  STOP？

#define E0_DIR(dat)		((dat > 0) ? (GPIOC->BSRRL = 1 << 13) : (GPIOC->BSRRH = 1 << 13))
#define E0_STEP(dat)	((dat > 0) ? (GPIOC->BSRRL = 1 << 14) : (GPIOC->BSRRH = 1 << 14))
#define E0_EN(dat)		((dat > 0) ? (GPIOC->BSRRL = 1 << 15) : (GPIOC->BSRRH = 1 << 15))



#define E1_DIR_PIN	   PEout(4) //原板 	E6
#define E1_STEP_PIN    PEout(5)	//原板 	E5
#define E1_ENABLE_PIN  PEout(6)	//原板 	E4
#define E1_MIN_PIN     PAin(8)	//E2  STOP？

#define E1_DIR(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 4) : (GPIOE->BSRRH = 1 << 4))
#define E1_STEP(dat)	((dat > 0) ? (GPIOE->BSRRL = 1 << 5) : (GPIOE->BSRRH = 1 << 5))
#define E1_EN(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 6) : (GPIOE->BSRRH = 1 << 6))


#define E2_DIR_PIN	   PEout(1) //原板 	E2
#define E2_STEP_PIN    PEout(2)	//原板 	B8
#define E2_ENABLE_PIN  PEout(3) //原板 	E3
#define E2_MIN_PIN     PDin(0)	//E3  STOP？

#define E2_DIR(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 1) : (GPIOE->BSRRH = 1 << 1))
#define E2_STEP(dat)	((dat > 0) ? (GPIOE->BSRRL = 1 << 2) : (GPIOE->BSRRH = 1 << 2))
#define E2_EN(dat)		((dat > 0) ? (GPIOE->BSRRL = 1 << 3) : (GPIOE->BSRRH = 1 << 3))

#define STOP_U			PDin(10)//原板  	D11  Z轴最大限位
#define STOP_V			PDin(9) //原板 	A8	 断料检测
#define STOP_W			PDin(8) //原板 	D0	 门状态


#define DOOR_STAT		PDin(8)			//门状态 A3->D8


#define  HEATER_0_PIN   TIM3->CCR3	  // TIM3_CH2->TIM4_CH4		PC7->D15  	
#define  HEATER_1_PIN   TIM4->CCR3	  // TIM3_CH2->TIM4_CH3		PC7->D14
//#define  HEATER_1_PIN 	TIM3->CCR3

#define  HEATER_2_PIN   TIM3->CCR2	  // TIM3_CH2	PC7  

#define  BL_TOUCH_PIN   TIM8->CCR3    //B1

//#define  FAN_PIN        TIM3->CCR3	  // TIM3_CH3	PB0
#define  HEATER_BED_PIN TIM4->CCR4	  // TIM3_CH3  	PB0         TIM4_CH3->TIM3_CH3      PD14->PC8

#define  E0_FAN        PDout(12)	//E0_FAN T5_2   TIM5->CCR2// PAout(1) ->TIM4_CH2 PD13 	
 
 
//#define TEMP_0_PIN	   (Get_Adc(ADC_Channel_3)>>2)   						// AD3_4	E0_TEMP  A0->A3 ADC1_IN0   -> ADC1_IN3
////#define TEMP_1_PIN	   (Get_Adc(ADC_Channel_1)>>2)   						// AD3_6	E1_TEMP	A6-A1
//#define TEMP_BED_PIN   (Get_Adc(ADC_Channel_2)>>2)  						//C2->A2 ADC1_IN12-> ADC1_IN2



#define LASER	PDout(12)

#define SD_CD PAin(15)	//SD卡插入检测	C8->A15

#define POWER_DOWN  PEin(15)	//断电脉冲输入引脚


#endif
