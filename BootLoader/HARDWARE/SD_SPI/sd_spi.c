 /**
  ******************************************************************************
  * @file    bsp_spi_flash.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   spi flash 底层应用函数bsp 
  ******************************************************************************
  * @attention
  *
  * 实验平台:秉火STM32 F407 开发板
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
//#include "bsp_spi_flash.h"

#include "sd_spi.h"



 /**
  * @brief  SPI_FLASH初始化
  * @param  无
  * @retval 无
  */


#if  SET_SPI1
#if 1

#if 0
 void SPI1_Init(void)
 {	  
   GPIO_InitTypeDef  GPIO_InitStructure;
   SPI_InitTypeDef	SPI_InitStructure;
	 
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ê1?üGPIOBê±?ó
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);//ê1?üSPI1ê±?ó
  
   //GPIOFB3,4,53?ê??ˉéè??
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;//PB3~5?′ó?1|?üê?3?	 
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?′ó?1|?ü
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//í?íìê?3?
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//é?à-
   GPIO_Init(GPIOA, &GPIO_InitStructure);//3?ê??ˉ
	 
	 GPIO_PinAFConfig(GPIOA,GPIO_PinSource5,GPIO_AF_SPI1); //PB3?′ó??a SPI1
	 GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_SPI1); //PB4?′ó??a SPI1
	 GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_SPI1); //PB5?′ó??a SPI1
  
	 //?aà???????SPI?ú3?ê??ˉ
	 RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,ENABLE);//?′??SPI1
	 RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1,DISABLE);//í￡?1?′??SPI1
 
	 SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //éè??SPIμ￥?ò?ò?????òμ?êy?Y?￡ê?:SPIéè???a???????òè???1¤
	 SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		 //éè??SPI1¤×÷?￡ê?:éè???a?÷SPI
	 SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		 //éè??SPIμ?êy?Y′óD?:SPI·￠?í?óê?8?????á11
	 SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;	 //′?DDí?2?ê±?óμ????D×′ì??a??μ???
	 SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	 //′?DDí?2?ê±?óμ?μú?t??ì?±???￡¨é?éy?ò???μ￡?êy?Y±?2é?ù
	 SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		 //NSSD?o?óéó2?t￡¨NSS1ü??￡??1ê?èí?t￡¨ê1ó?SSI??￡?1üàí:?ú2?NSSD?o?óDSSI??????
	 SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;		 //?¨ò?2¨ì??ê?¤·??μμ??μ:2¨ì??ê?¤·??μ?μ?a256
	 SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;  //???¨êy?Y′?ê?′óMSB???1ê?LSB???aê?:êy?Y′?ê?′óMSB???aê?
	 SPI_InitStructure.SPI_CRCPolynomial = 7;	 //CRC?μ????μ??à??ê?
	 SPI_Init(SPI1, &SPI_InitStructure);  //?ù?YSPI_InitStruct?D???¨μ?2?êy3?ê??ˉíaéèSPIx??′??÷
  
	 SPI_Cmd(SPI1, ENABLE); //ê1?üSPIíaéè
 
	 SPI1_ReadWriteByte(0xff);//???ˉ′?ê? 	  
 }	

#endif
 void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
 {
   assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//?D??óDD§D?
	 SPI1->CR1&=0XFFC7;//??3-5??á?￡?ó?à′éè??2¨ì??ê
	 SPI1->CR1|=SPI_BaudRatePrescaler;	 //éè??SPI1?ù?è 
	 SPI_Cmd(SPI1,ENABLE); //ê1?üSPI1
 } 
 //SPI1 ?áD′ò???×??ú
 //TxData:òaD′è?μ?×??ú
 //·μ???μ:?áè?μ?μ?×??ú
 u8 SPI1_ReadWriteByte(u8 TxData)
 {					  
  
   while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET){}//μè′y·￠?í????  
	 
	 SPI_I2S_SendData(SPI1, TxData); //í¨1yíaéèSPIx·￠?íò???byte  êy?Y
		 
   while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){} //μè′y?óê?íêò???byte  
  
	 return SPI_I2S_ReceiveData(SPI1); //·μ??í¨1ySPIx×??ü?óê?μ?êy?Y	 
			 
 }
 
 
 #endif
 
 
 
void SD_SPI_Init_1(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* 使能 FLASH_SPI 及GPIO 时钟 */
  /*!< SPI_FLASH_SPI_CS_GPIO, SPI_FLASH_SPI_MOSI_GPIO, 
       SPI_FLASH_SPI_MISO_GPIO,SPI_FLASH_SPI_SCK_GPIO 时钟使能 */
  RCC_AHB1PeriphClockCmd (FLASH_SPI_SCK_GPIO_CLK_1 | FLASH_SPI_MISO_GPIO_CLK_1|FLASH_SPI_MOSI_GPIO_CLK_1|FLASH_CS_GPIO_CLK_1, ENABLE);

  /*!< SPI_FLASH_SPI 时钟使能 */
  FLASH_SPI_CLK_INIT_1(FLASH_SPI_CLK_1, ENABLE);
 
  //设置引脚复用
  GPIO_PinAFConfig(FLASH_SPI_SCK_GPIO_PORT_1,FLASH_SPI_SCK_PINSOURCE_1,FLASH_SPI_SCK_AF_1); 
	GPIO_PinAFConfig(FLASH_SPI_MISO_GPIO_PORT_1,FLASH_SPI_MISO_PINSOURCE_1,FLASH_SPI_MISO_AF_1); 
	GPIO_PinAFConfig(FLASH_SPI_MOSI_GPIO_PORT_1,FLASH_SPI_MOSI_PINSOURCE_1,FLASH_SPI_MOSI_AF_1); 
  
  /*!< 配置 SPI_FLASH_SPI 引脚: SCK */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_SCK_PIN_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
  
  GPIO_Init(FLASH_SPI_SCK_GPIO_PORT_1, &GPIO_InitStructure);
  
	/*!< 配置 SPI_FLASH_SPI 引脚: MISO */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MISO_PIN_1;
  GPIO_Init(FLASH_SPI_MISO_GPIO_PORT_1, &GPIO_InitStructure);
  
	/*!< 配置 SPI_FLASH_SPI 引脚: MOSI */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI_MOSI_PIN_1;
  GPIO_Init(FLASH_SPI_MOSI_GPIO_PORT_1, &GPIO_InitStructure);  

	/*!< 配置 SPI_FLASH_SPI 引脚: CS */
  GPIO_InitStructure.GPIO_Pin = FLASH_CS_PIN_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(FLASH_CS_GPIO_PORT_1, &GPIO_InitStructure);

  /* 停止信号 FLASH: CS引脚高电平*/
  SPI_FLASH_CS_HIGH_1();

  /* FLASH_SPI 模式配置 */
  // FLASH芯片 支持SPI模式0及模式3，据此设置CPOL CPHA
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;//SPI_NSS_Hard
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(FLASH_SPI_1, &SPI_InitStructure);

  /* 使能 FLASH_SPI_1  */
  SPI_Cmd(FLASH_SPI_1, ENABLE);

}

#elif SET_SPI3

#if 1

 void SPI3_SetSpeed(u8 SPI_BaudRatePrescaler)
 {
   assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//?D??óDD§D?
	 SPI3->CR1&=0XFFC7;//??3-5??á?￡?ó?à′éè??2¨ì??ê
	 SPI3->CR1|=SPI_BaudRatePrescaler;	 //éè??SPI1?ù?è 
	 SPI_Cmd(SPI3,ENABLE); //ê1?üSPI1
 } 
 //SPI1 ?áD′ò???×??ú
 //TxData:òaD′è?μ?×??ú
 //·μ???μ:?áè?μ?μ?×??ú
 u8 SPI3_ReadWriteByte(u8 TxData)
 {					  
  
   while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET){}//μè′y·￠?í????  
	 
	 SPI_I2S_SendData(SPI3, TxData); //í¨1yíaéèSPIx·￠?íò???byte  êy?Y
		 
   while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET){} //μè′y?óê?íêò???byte  
  
	 return SPI_I2S_ReceiveData(SPI3); //·μ??í¨1ySPIx×??ü?óê?μ?êy?Y	 
			 
 }
 
 
 #endif
 
 
 
void SD_SPI3_Init(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* 使能 FLASH_SPI 及GPIO 时钟 */
  /*!< SPI_FLASH_SPI_CS_GPIO, SPI_FLASH_SPI_MOSI_GPIO, 
       SPI_FLASH_SPI_MISO_GPIO,SPI_FLASH_SPI_SCK_GPIO 时钟使能 */
  RCC_AHB1PeriphClockCmd (FLASH_SPI3_SCK_GPIO_CLK | FLASH_SPI3_MISO_GPIO_CLK|FLASH_SPI3_MOSI_GPIO_CLK|FLASH_CS_GPIO_CLK_3, ENABLE);

  /*!< SPI_FLASH_SPI 时钟使能 */
  FLASH_SPI3_CLK_INIT(FLASH_SPI3_CLK, ENABLE);
 
  //设置引脚复用
  GPIO_PinAFConfig(FLASH_SPI3_SCK_GPIO_PORT,FLASH_SPI3_SCK_PINSOURCE,FLASH_SPI3_SCK_AF); 
	GPIO_PinAFConfig(FLASH_SPI3_MISO_GPIO_PORT,FLASH_SPI3_MISO_PINSOURCE,FLASH_SPI3_MISO_AF); 
	GPIO_PinAFConfig(FLASH_SPI3_MOSI_GPIO_PORT,FLASH_SPI3_MOSI_PINSOURCE,FLASH_SPI3_MOSI_AF); 
  
  /*!< 配置 SPI_FLASH_SPI 引脚: SCK */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI3_SCK_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;  
  
  GPIO_Init(FLASH_SPI3_SCK_GPIO_PORT, &GPIO_InitStructure);
  
	/*!< 配置 SPI_FLASH_SPI 引脚: MISO */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI3_MISO_PIN;
  GPIO_Init(FLASH_SPI3_MISO_GPIO_PORT, &GPIO_InitStructure);
  
	/*!< 配置 SPI_FLASH_SPI 引脚: MOSI */
  GPIO_InitStructure.GPIO_Pin = FLASH_SPI3_MOSI_PIN;
  GPIO_Init(FLASH_SPI3_MOSI_GPIO_PORT, &GPIO_InitStructure);  

	/*!< 配置 SPI_FLASH_SPI 引脚: CS */
  GPIO_InitStructure.GPIO_Pin = FLASH_CS_PIN_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(FLASH_CS_GPIO_PORT_3, &GPIO_InitStructure);

  /* 停止信号 FLASH: CS引脚高电平*/
  SPI3_FLASH_CS_HIGH();
  //SPI_Cmd(SPI3,DISABLE);

  /* FLASH_SPI 模式配置 */
  // FLASH芯片 支持SPI模式0及模式3，据此设置CPOL CPHA
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(FLASH_SPI3, &SPI_InitStructure);

  /* 使能 FLASH_SPI_1  */
  SPI_Cmd(FLASH_SPI3, ENABLE);

}


#endif
