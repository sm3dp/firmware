#ifndef __SM_PLUS_H__
#define __SM_PLUS_H__

#include "main.h"
#include "usart.h"
#include "exfuns.h"
#include "sys.h"
#include "delay.h"	
#include "24cxx.h"
#include "sdio_sdcard.h"
#include "malloc.h"
#include "ff.h"
#include "exfuns.h"
#include "sm_firmware.h" 
#include "Configuration.h"


#include "lcd.h"
#include "piclib.h"	
//#include "lcdmenu.h"
#include "lcd_menu.h"
#include "text.h"
#include "guix.h"

#include <stdio.h>
#include <stdlib.h>

#include "bsp_spi_flash.h"


#define PRINT_IDLE 	0
#define PRINT_ING 	1
#define PRINT_PAUSE 2
#define PRINT_PRE 	3

typedef struct
{
	u8 printsd;			//0:idle 1:printing 2:pause 3:prepare print
	u8 filament;		//bit0~bit2 filament1,2,3;0 normal 1 filament run out
	u8 print_prepare;	//print prepare progress
	u8 printfile[50];	
	u8 printper;		//progress bar
	u32 filesize;		//print file size
	u32 printtime;		//print use time
}PRINTINFO;
typedef struct
{
	u8 reprint_flg;					//续打标志
	u8 break_point_flg;				//断点续打标志
	u8 active_extruder;
	int temperature_extruder[2];	//挤出头目标温度
	int temperature_bed;			//热床目标温度
	u32	lseek;						//续打偏移地址
	float e_axis;					//E轴值
	float z_axis;					//Z轴值
	float x_axis;					//x轴值
	float y_axis;					//y轴值
	
}REPRINT;		//断电续打参数结构体
extern REPRINT reprint;
void RePrint(void);

extern PRINTINFO PrintInfo;
extern u32 ReadChar;
extern u32 CPUID[3];
extern u32 sm_version;
extern u32 wifi_version;

extern u8 INVERT_X_DIR;
extern u8 INVERT_Y_DIR;
extern u8 INVERT_Z_DIR;
extern u8 INVERT_E0_DIR;
extern u8 INVERT_E1_DIR;
extern u8 INVERT_E2_DIR;

extern u8 X_ENDSTOPS_INVERTING;
extern u8 Y_ENDSTOPS_INVERTING;
extern u8 Z_ENDSTOPS_INVERTING;
extern u8 U_ENDSTOPS_INVERTING;
extern u8 V_ENDSTOPS_INVERTING;
extern u8 W_ENDSTOPS_INVERTING;


extern u8 LeveingComplete;
extern FIL logtext;
extern float ParaVersion;
//add by zlb
extern u8 LevelMode;
extern u8 EXTRUDER_NUM;
extern float LevelZAxisOffset;
//////////

void update_icon(void);
void qlzdecompressed(u8* name);
void qlzcompress(u8* path);
void printinfo_update(void);
void printinfo_init(void);
void equation_plan(float *x,float *y,float *z);

void RePrint(void);
void PowerOff_Init(void);
void SavePrintInfo(void);



#define BLTOUCH_UD TIM1->CCR1


#define BL_UP	1473
#define BL_DOWN	647


//#define BLTOUCH_LIMIT PDin(11)
#define BLTOUCH_LIMIT PBin(0)


extern u8 old_block_buffer_head;
extern long e_steps_count;
extern long z_setps_count;


extern float show_extruder_temperature[2];
extern float show_bed_temperature;

extern u32 timer_now,timer_old;

extern u16 UpdateFlg;	//
extern u8 BeepFlg;
extern u8 BeepSwitch;
extern int show_extruder_tp;
void show_temperature(void);


void bl_touch_init(void);
void copyfiles(void);
void ls_spi_icon(void);
void copy_font(void);
void filament_lack(void);
void sd_check(void);



void copy_diy_icons(void);
void ls_spi_diy_icon(void);


//#endif	//#ifdef AUTO_LEVELING
#if 1
enum {
	RDCONFIG_OK = 0,
	NON_CONFIGFILE, //文件不存在
	FILE_MISMATCH,//文件不匹配
	PARAM_NONALL//配置文件参数不全
};

typedef struct {
	char 			*name;		//参数名
	unsigned char	para_Num;	//个数
	float  		    Float_Data[4];
} Para_T;

u16 ConfigurationWrite2EEPROM(void);
void save_configfile(void);

void BeepSet(u8 stat);

#endif


#endif



