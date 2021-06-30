#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

#include "stm32f4xx.h"
#include <stdio.h>
//#define SET_SPI1   0
#define SET_SPI3      1
#if SET_SPI1

/*SPI接口定义-开头****************************/
#define FLASH_SPI_1                           SPI1
#define FLASH_SPI_CLK_1                       RCC_APB2Periph_SPI1
#define FLASH_SPI_CLK_INIT_1                  RCC_APB2PeriphClockCmd

#define FLASH_SPI_SCK_PIN_1                   GPIO_Pin_5                 
#define FLASH_SPI_SCK_GPIO_PORT_1             GPIOA                       
#define FLASH_SPI_SCK_GPIO_CLK_1              RCC_AHB1Periph_GPIOA
#define FLASH_SPI_SCK_PINSOURCE_1             GPIO_PinSource5
#define FLASH_SPI_SCK_AF_1                    GPIO_AF_SPI1

#define FLASH_SPI_MISO_PIN_1                  GPIO_Pin_6                
#define FLASH_SPI_MISO_GPIO_PORT_1            GPIOA                   
#define FLASH_SPI_MISO_GPIO_CLK_1             RCC_AHB1Periph_GPIOA
#define FLASH_SPI_MISO_PINSOURCE_1            GPIO_PinSource6
#define FLASH_SPI_MISO_AF_1                   GPIO_AF_SPI1

#define FLASH_SPI_MOSI_PIN_1                  GPIO_Pin_7                
#define FLASH_SPI_MOSI_GPIO_PORT_1            GPIOA
#define FLASH_SPI_MOSI_GPIO_CLK_1             RCC_AHB1Periph_GPIOA
#define FLASH_SPI_MOSI_PINSOURCE_1            GPIO_PinSource7
#define FLASH_SPI_MOSI_AF_1                   GPIO_AF_SPI1

#define FLASH_CS_PIN_1                        GPIO_Pin_4               
#define FLASH_CS_GPIO_PORT_1                  GPIOA
#define FLASH_CS_GPIO_CLK_1                   RCC_AHB1Periph_GPIOA

#define SPI_FLASH_CS_LOW_1()      {FLASH_CS_GPIO_PORT_1->BSRRH=FLASH_CS_PIN_1;}
#define SPI_FLASH_CS_HIGH_1()     {FLASH_CS_GPIO_PORT_1->BSRRL=FLASH_CS_PIN_1;}
/*SPI接口定义-结尾****************************/




void SD_SPI_Init_1(void);
void SPI1_Init(void);			 //3?ê??ˉSPI1?ú
void SPI1_SetSpeed(u8 SpeedSet); //éè??SPI1?ù?è   
u8 SPI1_ReadWriteByte(u8 TxData);//SPI1×ü???áD′ò???×??ú

#elif  SET_SPI3 

/*SPI接口定义-开头****************************/
#define FLASH_SPI3                           SPI3
#define FLASH_SPI3_CLK                       RCC_APB1Periph_SPI3 //RCC_APB2Periph_SPI1
#define FLASH_SPI3_CLK_INIT                  RCC_APB1PeriphClockCmd

#define FLASH_SPI3_SCK_PIN                   GPIO_Pin_10                 
#define FLASH_SPI3_SCK_GPIO_PORT             GPIOC
#define FLASH_SPI3_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOC
#define FLASH_SPI3_SCK_PINSOURCE             GPIO_PinSource10
#define FLASH_SPI3_SCK_AF                    GPIO_AF_SPI3

#define FLASH_SPI3_MISO_PIN                  GPIO_Pin_11                
#define FLASH_SPI3_MISO_GPIO_PORT            GPIOC                   
#define FLASH_SPI3_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOC
#define FLASH_SPI3_MISO_PINSOURCE            GPIO_PinSource11
#define FLASH_SPI3_MISO_AF                   GPIO_AF_SPI3

#define FLASH_SPI3_MOSI_PIN                  GPIO_Pin_12                
#define FLASH_SPI3_MOSI_GPIO_PORT            GPIOC
#define FLASH_SPI3_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOC
#define FLASH_SPI3_MOSI_PINSOURCE            GPIO_PinSource12
#define FLASH_SPI3_MOSI_AF                   GPIO_AF_SPI3

#define FLASH_CS_PIN_3                        GPIO_Pin_3               
#define FLASH_CS_GPIO_PORT_3                  GPIOB
#define FLASH_CS_GPIO_CLK_3                   RCC_AHB1Periph_GPIOB

#define SPI3_FLASH_CS_LOW()      {FLASH_CS_GPIO_PORT_3->BSRRH=FLASH_CS_PIN_3;}
#define SPI3_FLASH_CS_HIGH()     {FLASH_CS_GPIO_PORT_3->BSRRL=FLASH_CS_PIN_3;}
/*SPI接口定义-结尾****************************/

void SD_SPI3_Init(void);
void SPI3_Init(void);			 //3?ê??ˉSPI1?ú
void SPI3_SetSpeed(u8 SpeedSet); //éè??SPI1?ù?è   
u8 SPI3_ReadWriteByte(u8 TxData);//SPI1×ü???áD′ò???×??ú




#endif
#endif /* __SPI_FLASH_H */

