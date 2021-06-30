#ifndef __24CXX_H
#define __24CXX_H
#include "iic.h" 
#include "sys.h"


#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	    8191
#define AT24C128	16383
#define AT24C256	32767  
//use 24c64
#define EE_TYPE AT24C64

//define eeprom use space
#define EEPROM_OFFSET_CFG 	100		//printer value save start address

#define E_G_CODE_EAXIS		800		//E axis value
#define POINTER_FILE		804		//file pointer when power off 
#define HEIGHT_MODE			808		//model print hight
#define PRINT_HOUR			812		//model print hour
#define PRINT_MIN			813		//model print minute
#define PRINT_DATE			814		//model print time day
#define E_MULIT_SPEED		815		//print mulit speed
#define E_G_CODE_FEED		820		//print speed	
#define E_ACTIVE_EXTRUDER   830		//active extruder
#define E_BMP				832		//model image exit flag
#define E_BMP_SIZE			833		//pic size
#define E_X_POS				840		//x position
#define E_Y_POS				844		//y position
#define E_LANG				900		//language select

#define E_SDPRINT			1000	//sd card print flag
#define E_FILENAME			1001	//save sd card print file name
#define E_BEDTEMPERATURE	1100	//target bed temperature
#define E_EXTRUDERTEMPERATURE 1104	//target extruder temperature
#define E_G_CODE_LINE		1108	//G-code line

#define E_BEEP				1198	//beep enable/disable			


#define E_G_CODE_VALUE		1140	//G-code cmd data
#ifdef MESH_BED_LEVELING
#define MBL_OFFSET			1200	//auto level offset value
#define MBL_VALUE			1204	//auto level mesh bed leveling about 9 points
#endif
#define TEST_ADDR			1300	//test data

u8 AT24CXX_ReadOneByte(u16 ReadAddr);							
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);		
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);					
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);	
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   	

u8 AT24CXX_Check(void);  //¼ì²éÆ÷¼þ
#endif
