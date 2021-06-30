/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/

#include "sm_plus.h"
#include "bsp.h"
#include "App_Language.h"
#include "App_Timer.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int test_Num;
FIL logtext;


PRINTINFO PrintInfo;		//print information
u32 ReadChar = 0;			//read sd card print file size
u32 CPUID[3];				//CPU ID
u32 sm_version = 100;		//mother board version
u32 wifi_version = 100;		//wifi module version

u16 UpdateFlg = 0;	//update font and pic flag
u8 BeepFlg = 0;	//beep run flag 0 stop  1 run
u8 BeepSwitch = 1;	//beep switch   0 off 1 on

static bool extruder_temp_stable_flag[2] = {FALSE, FALSE}; //挤出头温度稳定
static bool bed_temp_stable_flag = FALSE;                //热床温度稳定

//u8 INVERT_X_DIR=false;
u8 INVERT_X_DIR = FALSE;
u8 INVERT_Y_DIR = FALSE;
u8 INVERT_Z_DIR = FALSE;
u8 INVERT_E0_DIR = FALSE;
u8 INVERT_E1_DIR = FALSE;
u8 INVERT_E2_DIR = FALSE;

u8 X_ENDSTOPS_INVERTING = 1;
u8 Y_ENDSTOPS_INVERTING = 1;
u8 Z_ENDSTOPS_INVERTING = 1;
u8 U_ENDSTOPS_INVERTING = 1;
u8 V_ENDSTOPS_INVERTING = 0;
u8 W_ENDSTOPS_INVERTING = 1;

float ParaVersion = 0.01;	//configuration file version

long e_steps_count = 0;		//e axis step count; for filament in out 
long z_setps_count = 0;
extern volatile unsigned char block_buffer_head;           // Index of the next block to be pushed
extern volatile unsigned char block_buffer_tail;           // Index of the block to process now
extern block_t *current_block;

//===================ADD BY KIM=============================
extern uint32_t CurrentFileSize, CurrentReadSize;
//==========================================================

//===================ADD BY ZLB=============================
u8 LevelMode = 0;	//leveling mode 0 manual 1 auto
u8 EXTRUDER_NUM = 2;	//extruder number 1signal 2double
/*
*update font and ui resource from sd card to spi flash
*/
void update_icon(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	u8 *namebuf = NULL;
	FILINFO *pfno;
	#if _USE_LFN
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
	#endif
	namebuf = (u8 *)mymalloc(SRAMIN, 128);
	pfno = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO));
	if(namebuf == NULL || pfno == NULL)
	{
		myfree(SRAMIN, fileinfo.lfname);
		myfree(SRAMIN, namebuf);
		myfree(SRAMIN, pfno);
		return;
	}
	res = f_opendir(&dir, "0:\\icon");
	if(res)
		return;
	res = f_stat("1:icon", pfno);
	if(res)
	{
		res = f_mkdir("1:icon");
	}

	if(res == FR_OK)
	{

		while(1)
		{
			res = f_readdir(&dir, &fileinfo);
			if(res != FR_OK || fileinfo.fname[0] == 0) break;
			#if _USE_LFN
			fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
			#else
			fn = (u8 *)(fileinfo.fname);
			#endif
			res = f_typetell(fn);
			if((res & 0XF0) == 0)  	
			{
				printf("%s\n",  fn);
				sprintf(namebuf, "0:/icon/%s", fn);
				res = f_open(file, namebuf, FA_READ | FA_OPEN_EXISTING);
				if(res)
					return;
				memset(namebuf, 0, sizeof(namebuf));

				sprintf(namebuf, "1:/icon/%s", fn);
				res = f_open(ftemp, namebuf, FA_CREATE_ALWAYS | FA_WRITE);
				if(res)
					return;
				while(res == 0)
				{
					memset(namebuf, 0, sizeof(namebuf));
					res = f_read(file, namebuf, 128, &br);
					if(res || br == 0)
						break;
					res = f_write(ftemp, namebuf, br, &bw);
					if(res || bw == 0)
						break;
				}
				f_close(file);
				f_close(ftemp);

			}
		}
	}
	//read spi flash flie 
	res = f_opendir(&dir, "1:\\icon");
	if(res)
		return;
	while(1)
	{
		res = f_readdir(&dir, &fileinfo);
		if(res != FR_OK || fileinfo.fname[0] == 0) break;
		#if _USE_LFN
		fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
		#else
		fn = (u8 *)(fileinfo.fname);
		#endif
		res = f_typetell(fn);
		if((res & 0XF0) == 0) 
		{
			USR_UsrLog("%s size=%d\r\n", fn, fileinfo.fsize);
		}
	}
	myfree(SRAMIN, fileinfo.lfname);
	myfree(SRAMIN, namebuf);
	myfree(SRAMIN, pfno);


}






/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
extern USR_TIMER usr_timer;

void printinfo_update(void)
{
	u8 per;
	u32 time_remain;
	float persent_f;
	float print_percent = 0;

	//get print percentage
	CurrentReadSize = f_tell(&card.fgcode);
	print_percent = ((float)CurrentReadSize / PrintInfo.filesize) * 100;					//Read Data With f_gets
	PrintInfo.printper = print_percent;
	//get print time
	if(PrintInfo.printsd == TRUE)
	{
		PrintInfo.printtime = (u32)((millis() - starttime) / 1000);
	}
	
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:System Power Up Printing Infor Init
*/
void printinfo_init(void)
{
	PrintInfo.filesize = 0;
	strcpy(PrintInfo.printfile, "null");
	PrintInfo.printper = 0;
	PrintInfo.printsd = 0;
	PrintInfo.printtime = 0;
}







//#if define AUTO_LEVELING
extern volatile long count_position[NUM_AXIS];
extern volatile long endstops_trigsteps[3];

void bl_touch_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);


	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM8);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_DeInit(TIM8);

	//2.7K
	TIM_TimeBaseStructure.TIM_Period = 19999;
	TIM_TimeBaseStructure.TIM_Prescaler = 167;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);

	/* Output Compare Active Mode configuration: Channel4 */

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;

	TIM_OCInitStructure.TIM_Pulse = BL_UP;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;//
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
	TIM_OC3Init(TIM8, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM8, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM8, ENABLE);
	TIM_CtrlPWMOutputs(TIM8, ENABLE);
	TIM_Cmd(TIM8, ENABLE);
}



float get_middle(float a, float b, float c)
{
	if((b - a) * (a - c) >= 0.0)
	{
		return a;
	}
	else if((a - b) * (b - c) >= 0.0)
	{
		return b;
	}
	else
	{
		return c;
	}
}

void copyfiles(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
	DIR dir_des;
	res = f_opendir(&dir, "0:\\child_icon");
	if(res == FR_OK)
	{
		//open or creat folder
		res = f_opendir(&dir_des, "1:\\child_icon");
		if(res)
		{
			USR_ErrLog("f_opendir 1:\\child_icon error res=%d", res);
			res = f_mkdir("1:\\child_icon");
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				return;
			}
		}
		res = f_closedir(&dir_des);
		while(1)
		{
			res = f_readdir(&dir, &fileinfo);
			if(res != FR_OK || fileinfo.fname[0] == 0) break;
			fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);

			memset(pname, 0, sizeof(pname));
			sprintf(pname, "0:/child_icon/%s", fn);
			res = f_open(file, pname, FA_READ | FA_OPEN_EXISTING);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				PRINTFLINE;
				return;
			}

			memset(pname, 0, sizeof(pname));
			sprintf(pname, "1:/child_icon/%s", fn);
			res = f_open(ftemp, pname, FA_WRITE | FA_CREATE_ALWAYS);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				PRINTFLINE;
				return;
			}
			while(res == 0)
			{
				res = f_read(file, fatbuf, 512, (UINT *)&br); //源头读出512字节
				if(res || br == 0)
					break;
				res = f_write(ftemp, fatbuf, (UINT)br, &bw);
				if(res || bw < br)
					break;
			}
			USR_UsrLog("file name=%s", pname);
			USR_UsrLog("file size=%d", file->fsize);

			f_close(file);
			f_close(ftemp);
		}
		myfree(SRAMIN, fileinfo.lfname);
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:copy icons from TF to spi flash
*/
void copy_diy_icons(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
	DIR dir_des;
	res = f_opendir(&dir, "0:\\diy_icon");
	if(res == FR_OK)
	{
		res = f_opendir(&dir_des, "1:\\diy_icon");
		if(res)
		{
			USR_ErrLog("error res=%d", res);
			res = f_mkdir("1:\\diy_icon");
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				return;
			}
		}
		res = f_closedir(&dir_des);
		while(1)
		{
			res = f_readdir(&dir, &fileinfo);
			if(res != FR_OK || fileinfo.fname[0] == 0) break;
			fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);

			memset(pname, 0, sizeof(pname));
			sprintf(pname, "0:/diy_icon/%s", fn);

			res = f_open(file, pname, FA_READ | FA_OPEN_EXISTING);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				return;
			}

			memset(pname, 0, sizeof(pname));
			sprintf(pname, "1:/diy_icon/%s", fn);
			res = f_open(ftemp, pname, FA_WRITE | FA_CREATE_ALWAYS);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				return;
			}
			while(res == 0)
			{
				res = f_read(file, fatbuf, 512, (UINT *)&br); //源头读出512字节
				if(res || br == 0)
					break;
				res = f_write(ftemp, fatbuf, (UINT)br, &bw);
				if(res || bw < br)
					break;
			}
			USR_UsrLog("file name=%s", pname);
			USR_UsrLog("file size=%d", file->fsize);

			f_close(file);
			f_close(ftemp);
		}
		myfree(SRAMIN, fileinfo.lfname);
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void copy_font(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);
	res = f_opendir(&dir, "0:/SYSTEM/FONT");
	if(res == FR_OK)
	{
		PRINTFLINE;
		while(1)
		{
			res = f_readdir(&dir, &fileinfo);
			if(res != FR_OK || fileinfo.fname[0] == 0) break;
			fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);

			memset(pname, 0, sizeof(pname));
			sprintf(pname, "0:/SYSTEM/FONT/%s", fn);
			res = f_open(file, pname, FA_READ | FA_OPEN_EXISTING);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				PRINTFLINE;
				return;
			}
			memset(pname, 0, sizeof(pname));
			sprintf(pname, "1:/%s", fn);
			res = f_open(ftemp, pname, FA_WRITE | FA_CREATE_ALWAYS);
			if(res)
			{
				myfree(SRAMIN, fileinfo.lfname);
				PRINTFLINE;
				return;
			}
			while(res == 0)
			{
				res = f_read(file, fatbuf, 512, (UINT *)&br); //源头读出512字节
				if(res || br == 0)
					break;
				res = f_write(ftemp, fatbuf, (UINT)br, &bw);
				if(res || bw < br)
					break;
			}
			USR_UsrLog("file name=%s", pname);
			USR_UsrLog("file size=%d", file->fsize);

			f_close(file);
			f_close(ftemp);
		}
		myfree(SRAMIN, fileinfo.lfname);
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void ls_spi_icon(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);

	res = f_opendir(&dir, "1:\\child_icon");
	if(res)
	{
		myfree(SRAMIN, fileinfo.lfname);
		PRINTFLINE;
		return;
	}
	while(1)
	{
		res = f_readdir(&dir, &fileinfo);
		if(res != FR_OK || fileinfo.fname[0] == 0) break;
		fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
		USR_UsrLog("%s",	fn);
		USR_UsrLog("%d", fileinfo.fsize);
	}
	myfree(SRAMIN, fileinfo.lfname);

}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void ls_spi_diy_icon(void)
{
	u8 res;
	u8 *fn;   /* This function is assuming non-Unicode cfg. */
	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = mymalloc(SRAMIN, fileinfo.lfsize);

	res = f_opendir(&dir, "1:\\diy_icon");
	if(res)
	{
		myfree(SRAMIN, fileinfo.lfname);
		return;
	}
	while(1)
	{
		res = f_readdir(&dir, &fileinfo);
		if(res != FR_OK || fileinfo.fname[0] == 0) break;
		fn = (u8 *)(*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname);
		USR_UsrLog("%s\n",	fn);
		USR_UsrLog("%d\n", fileinfo.fsize);
	}
	myfree(SRAMIN, fileinfo.lfname);

}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void filament_lack(void)
{
	//1、停止打印
	card.sdprinting = false;
	//2、xy轴归零
	enquecommand("G28 XY");
	//3、跳转到更换耗材界面
	//tempMenu = print_menu;
	//CurrentMenu = filamentstep1_menu;
	redraw_menu = true;
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:sd card check
*/
void sd_check(void)
{
	if(card.cardOK == true && SD_CD == 1)
	{
		card_release();
		USR_UsrLog("SD card pull out\r\n");
	}
	if(card.cardOK == false && SD_CD == 0)
	{
		card_initsd();
		USR_UsrLog("SD card insert\r\n");
	}
}



u8 i_buff = 0, show_delay_time = 0;
u8 index_buff = 0;	//新采样温度值存放位置
#define SampleTime 20
#define TEMP_DELAY_TIME 5                               //减少温度跳变速度
float temperature_buff[EXTRUDERS + 1][SampleTime] = {0.0};
float show_extruder_temperature[2] = { 0 };
float show_bed_temperature = 0;


float tp_buff1[10] = {0};
int show_extruder_tp;
void show_temperature(void)
{
	static u8 index_tp1 = 0;
	float temp;
	u8 i, j;
	if(index_tp1 < 10)
		tp_buff1[index_tp1++] = current_temperature[0];
	else
	{
		index_tp1 = 0;
		//冒泡法排序
		for(i = 0; i < 9; ++i)
		{
			for(j = 0; j < 9 - i; ++j)
			{
				if(tp_buff1[j] > tp_buff1[j + 1])
				{
					temp = tp_buff1[j];
					tp_buff1[j] = tp_buff1[j + 1];
					tp_buff1[j + 1] = temp;
				}
			}
		}
		//取中间5个值求平均值
		temp = 0;
		for(i = 2; i < 7; i++)
		{
			temp += tp_buff1[i];
		}
		temp /= 5;
		show_extruder_tp = (int)(temp + 0.5);

	}
}

#if 1



#define PRINTF_ERROR(Str) printf("ERROR: %s \n",Str)
#define PRINTF_STATE(Str) printf("STATE: %s \n",Str)

#define CONFIGURATION_FPATH "0:Configuration.txt"
//file head
#define FILE_HEADER 		"#Parameter_List"

#define STRING_LEN  255

Para_T Para[] =
{
	{"axis_steps_per_unit",							4	, {0, 0, 0, 0}}, //1
	{"max_feedrate",								4	, {0, 0, 0, 0}}, //2	
	{"max_acceleration_units_per_sq_second",		4	, {0, 0, 0, 0}}, //3	
	{"acceleration",								1	, {0, 0, 0, 0}}, //4	
	{"retract_acceleration",						1	, {0, 0, 0, 0}}, //5	
	{"minimumfeedrate",								1	, {0, 0, 0, 0}}, //6	
	{"minsegmenttime",								1	, {0, 0, 0, 0}}, //7	
	{"max_xy_jerk",									1	, {0, 0, 0, 0}}, //8	
	{"max_z_jerk",									1	, {0, 0, 0, 0}}, //9	
	{"max_e_jerk",									1	, {0, 0, 0, 0}}, //10	
	{"kp",											1	, {0, 0, 0, 0}}, //11	
	{"ki",											1	, {0, 0, 0, 0}}, //12	
	{"kd",											1	, {0, 0, 0, 0}}, //13	
	{"INVERT_X_DIR",								1	, {0, 0, 0, 0}}, //14	
	{"INVERT_Y_DIR",								1	, {0, 0, 0, 0}}, //15	
	{"INVERT_Z_DIR",								1	, {0, 0, 0, 0}}, //16	
	{"INVERT_E0_DIR",								1	, {0, 0, 0, 0}}, //17	
	{"INVERT_E1_DIR",								1	, {0, 0, 0, 0}}, //18	
	{"INVERT_E2_DIR",								1	, {0, 0, 0, 0}}, //19	
	{"X_ENDSTOPS_INVERTING",						1	, {0, 0, 0, 0}}, //20	
	{"Y_ENDSTOPS_INVERTING",						1	, {0, 0, 0, 0}}, //21	
	{"Z_ENDSTOPS_INVERTING",						1	, {0, 0, 0, 0}}, //22	
	{"FILAMENT_INVERTING",							1	, {0, 0, 0, 0}}, //23	
	{"speed_temprature_factor",                     1	, {0, 0, 0, 0}}, //24   
	{"Machine_Size",                     			3	, {0, 0, 0, 0}}, //25	
	{"Para_Version",                     			1	, {0, 0, 0, 0}}, //26	
	{"LevelMode",									1	, {0, 0, 0, 0}}, //27 
	{"EXTRUDER_NUM",								1	, {0, 0, 0, 0}}, //28 
};
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
int Move_Fp(int Fp_Index, char *str)
{
	while((str[Fp_Index] == '\t') || (str[Fp_Index] == ' '))
	{
		Fp_Index++;
	}
	return Fp_Index;
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u16 ConfigurationWrite2EEPROM(void)
{
	FIL *fp = NULL;
	char Str_Buff[255];
	char *Str_Buff1;
	int  File_Index = 0;
	int  Parameter_Num = 0, i, j, res, len;
	Parameter_Num = sizeof(Para) / sizeof(Para_T);
	printf("parameters num = %d \n", Parameter_Num);
	fp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));
	res = f_open(fp, CONFIGURATION_FPATH, FA_READ);
	if(res != FR_OK)
	{
		PRINTF_ERROR("File Not Open!\n");
		return NON_CONFIGFILE;
	}
	else
	{
		f_gets(Str_Buff, STRING_LEN, fp);
		printf("FILE_HEADER:%s\n", FILE_HEADER);
		if(!strncmp(FILE_HEADER, Str_Buff, 15)) 
		{
			for(i = 0; i < Parameter_Num; i++)
			{
				File_Index = 0;
				f_gets(Str_Buff, STRING_LEN, fp);
				if(!f_eof(fp))
				{
					Str_Buff1 = Str_Buff;
					len = strlen(Para[i].name);
					if(!strncmp(Para[i].name, &Str_Buff1[1], len) && Str_Buff1[File_Index] == '>')
					{
						File_Index += 1;
						File_Index += len;
						File_Index = Move_Fp(File_Index, Str_Buff1);
						for(j = 0; j < Para[i].para_Num ; j++)
						{
							Para[i].Float_Data[j] = strtod(Str_Buff1 + File_Index, &Str_Buff1);
							File_Index = Move_Fp(0, Str_Buff1);
							USR_DbgLog("%f\n", Para[i].Float_Data[j]);
						}
						if(Str_Buff1[File_Index] == '#')
						{
							USR_DbgLog("Comments : %s", &Str_Buff1[File_Index]);
						}
						else
						{
							USR_ErrLog("no parameter");
						}
					}
					else if(Str_Buff1[File_Index] == '#')
					{
						i -= 1;
					}
					else
					{
						i -= 1;
					}
				}
				else
				{
					f_rename("0:Configuration.txt", "0:OldConfiguration.txt");
					return PARAM_NONALL;
				}
			}
		}
		else
		{
			USR_ErrLog("Configuration.txt file head error");
			f_rename("0:Configuration.txt", "0:OldConfiguration.txt");
			return	FILE_MISMATCH;
		}
	}
	f_rename("0:Configuration.txt", "0:OldConfiguration.txt");
	f_close(fp);


	USR_DbgLog("Configuration.txt read finish");
	return RDCONFIG_OK;
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void save_configfile(void)
{
	//保存参数
	axis_steps_per_unit[0] = Para[0].Float_Data[0];
	axis_steps_per_unit[1] = Para[0].Float_Data[1];
	axis_steps_per_unit[2] = Para[0].Float_Data[2];
	axis_steps_per_unit[3] = Para[0].Float_Data[3];

	max_feedrate[0] = Para[1].Float_Data[0];
	max_feedrate[1] = Para[1].Float_Data[1];
	max_feedrate[2] = Para[1].Float_Data[2];
	max_feedrate[3] = Para[1].Float_Data[3];

	max_acceleration_units_per_sq_second[0] = (unsigned long)Para[2].Float_Data[0];
	max_acceleration_units_per_sq_second[1] = (unsigned long)Para[2].Float_Data[1];
	max_acceleration_units_per_sq_second[2] = (unsigned long)Para[2].Float_Data[2];
	max_acceleration_units_per_sq_second[3] = (unsigned long)Para[2].Float_Data[3];

	acceleration = Para[3].Float_Data[0];
	retract_acceleration = Para[4].Float_Data[0];
	minimumfeedrate = Para[5].Float_Data[0];
	minsegmenttime = (unsigned long)Para[6].Float_Data[0];

	max_xy_jerk = Para[7].Float_Data[0];
	max_z_jerk = Para[8].Float_Data[0];
	max_e_jerk = Para[9].Float_Data[0];

	Kp = Para[10].Float_Data[0];
	Ki = Para[11].Float_Data[0];
	Kd = Para[12].Float_Data[0];

	INVERT_X_DIR = (u8)Para[13].Float_Data[0];
	INVERT_Y_DIR = (u8)Para[14].Float_Data[0];
	INVERT_Z_DIR = (u8)Para[15].Float_Data[0];
	INVERT_E0_DIR = (u8)Para[16].Float_Data[0];
	INVERT_E1_DIR = (u8)Para[17].Float_Data[0];
	INVERT_E2_DIR = (u8)Para[18].Float_Data[0];

	X_ENDSTOPS_INVERTING = (u8)Para[19].Float_Data[0];
	Y_ENDSTOPS_INVERTING = (u8)Para[20].Float_Data[0];
	Z_ENDSTOPS_INVERTING = (u8)Para[21].Float_Data[0];
	V_ENDSTOPS_INVERTING = (u8)Para[22].Float_Data[0];

	speed_temprature_factor = Para[23].Float_Data[0];
	X_MAX_POS = Para[24].Float_Data[0];
	Y_MAX_POS = Para[24].Float_Data[1];
	Z_MAX_POS = Para[24].Float_Data[2];
	ParaVersion = Para[25].Float_Data[0];
	LevelMode = (u8)Para[26].Float_Data[0];
	EXTRUDER_NUM = (u8)Para[27].Float_Data[0];
}


#endif

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks: power off module init 
*/
void PowerOff_Init(void)
{

	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_10);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOE, EXTI_PinSource15);

	EXTI_InitStructure.EXTI_Line = EXTI_Line15;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks: find the last E axis cmd
*/
u8 find_e(void)
{
	u8 i = 0;
	while((CmdBuff.cmdbuffer[bufindr][i] != 0) && (i < MAX_CMD_SIZE))	//值不为零
	{
		if(CmdBuff.cmdbuffer[bufindr][i] == 'E')
			return i;
		i++;
	}
	return 0;
}
void SavePrintInfo(void)
{
	u32 value;	//the file read point
	u8 temp, i;
	u16 sum = 0;
	card.sdprinting = FALSE;
	value = f_tell(&card.fgcode);
	for(i = 0; i < buflen; i++)
	{
		temp = strlen(CmdBuff.cmdbuffer[bufindr]);
		value -= temp;
		sum += temp;
		bufindr = (bufindr + 1) % BUFSIZE;
		if(code_seen('G') && i == 0)	//just get the last E axis cmd
		{
			switch((int)strtod(&CmdBuff.cmdbuffer[bufindr][1], NULL))
			{
				case 1:
					temp = find_e();
					if(temp)
					{
						current_position[3] = strtod(&CmdBuff.cmdbuffer[bufindr][temp + 1], NULL);
					}
					break;
				default:
					break;
			}
		}
	}
	AT24CXX_Write(POINTER_FILE, (u8 *)&value, 4);					

	AT24CXX_Write(HEIGHT_MODE, (u8 *)&current_position[2], 4);		
	AT24CXX_Write(E_G_CODE_EAXIS, (u8 *)&current_position[3], 4);	
	AT24CXX_Write(E_X_POS, (u8 *)&current_position[0], 4);	
	AT24CXX_Write(E_Y_POS, (u8 *)&current_position[1], 4);	
	temp = usr_timer.date * 24 + usr_timer.hour;
	AT24CXX_Write(PRINT_HOUR, (u8 *)&temp, 1);				
	AT24CXX_Write(PRINT_MIN, (u8 *)&usr_timer.min, 1);		
	AT24CXX_Write(E_MULIT_SPEED, (u8 *)&feedmultiply, 4);	
	AT24CXX_Write(E_ACTIVE_EXTRUDER, (u8 *)&active_extruder, sizeof(active_extruder));
	AT24CXX_Write(E_BMP, (u8 *)&bmp_flag, sizeof(bmp_flag));
	AT24CXX_Write(E_BMP_SIZE, (u8 *)&pic_size, sizeof(pic_size));

}
void EXTI15_10_IRQHandler(void)
{
	delay_ms(1);//	delay
	if(POWER_DOWN == 1 && PrintInfo.printsd != 0)	
	{
		SavePrintInfo();
		//save sd_card print flg
		AT24CXX_WriteOneByte(E_SDPRINT, true);			
	}
	EXTI_ClearITPendingBit(EXTI_Line15);
}

REPRINT reprint;

void RePrint(void)
{
	u8 res, i, len;
	memset(&reprint, 0, sizeof(reprint));
	if(AT24CXX_ReadOneByte(E_SDPRINT) != TRUE)	//ensure need reprint
		return;
	//need reprint
	reprint.reprint_flg = TRUE;

	res = AT24CXX_ReadOneByte(E_FILENAME);
	AT24CXX_Read(E_FILENAME + 1, pname, res);
	USR_DbgLog("read save file name is %s\r\n", pname);
	res = f_open(&card.fgcode, (const TCHAR *)pname, FA_READ);
	if(res)
	{
		USR_ErrLog("open reprint file error=%d ", res);
		reprint.reprint_flg = FALSE;	
		return;				
	}

	AT24CXX_Read(E_EXTRUDERTEMPERATURE, (u8 *)&reprint.temperature_extruder[0], sizeof(target_temperature[0]));
	USR_UsrLog("read save extruder temperature is %d", reprint.temperature_extruder[0]);
	AT24CXX_Read(E_BEDTEMPERATURE, (u8 *)&reprint.temperature_bed, sizeof(target_temperature_bed));
	USR_UsrLog("read save bed temperature is %d", reprint.temperature_bed);

	AT24CXX_Read(PRINT_HOUR, (u8 *)&usr_timer.hour, sizeof(usr_timer.hour));
	AT24CXX_Read(PRINT_DATE, (u8 *)&usr_timer.date, sizeof(usr_timer.date));
	AT24CXX_Read(PRINT_MIN, (u8 *)&usr_timer.min, sizeof(usr_timer.min));
	AT24CXX_Read(E_MULIT_SPEED, (u8 *)&feedmultiply, 4);
	feedmultiply = constrain(feedmultiply, 100, 500);
	AT24CXX_Read(E_ACTIVE_EXTRUDER, (u8 *)&reprint.active_extruder, sizeof(reprint.active_extruder));	
	USR_UsrLog("active_extruder = %d", reprint.active_extruder);
	if(reprint.active_extruder > 1)
		reprint.active_extruder = 0;
	AT24CXX_Read(E_BMP, (u8 *)&bmp_flag, sizeof(bmp_flag));	
	AT24CXX_Read(E_BMP_SIZE, (u8 *)&pic_size, sizeof(pic_size));
	AT24CXX_Read(POINTER_FILE, (u8 *)&reprint.lseek, sizeof(reprint.lseek));
	USR_UsrLog("read save filepoint is %d", reprint.lseek);

	res = f_lseek(&card.fgcode, reprint.lseek);	//跳转到续打位置
	if(res)
	{
		USR_ErrLog("f_lseek error=%d", res);
	}
	else
	{
		USR_UsrLog("fptr=%x", card.fgcode.fptr);
	}
	PrintInfo.printper = reprint.lseek * 100 / card.fgcode.fsize;

	AT24CXX_Read(E_G_CODE_EAXIS, (u8 *)&reprint.e_axis, sizeof(reprint.e_axis));
	AT24CXX_Read(HEIGHT_MODE, (u8 *)&reprint.z_axis, sizeof(reprint.z_axis));
	AT24CXX_Read(E_X_POS, (u8 *)&reprint.x_axis, sizeof(reprint.x_axis));
	AT24CXX_Read(E_Y_POS, (u8 *)&reprint.y_axis, sizeof(reprint.y_axis));

	PrintInfo.printsd = PRINT_PAUSE;
	len = strlen(pname);
	for(i = len; i > 0; i--)
	{
		if(pname[i] == '\\')
		{
			i++;
			break;
		}
	}
	strcpy(PrintInfo.printfile, &pname[i]);	
	PrintInfo.filesize = card.fgcode.fsize;	
	PrintInfo.printper = ((float)reprint.lseek / PrintInfo.filesize) * 100;
	PrintInfo.printtime = 0;
	card.sdprinting = false;	
	
	redraw_menu = TRUE;
	CurrentMenu = print_diy;

}
void gui_show_special(u8 *str, u16 x, u16 y, u16 width, u16 height, u8 font, u16 fcolor)
{
	u16 xpos = x;
	u16 ypos = y;
	u16 endx = x + width - 1;
	u16 endy = y + height - 1;
	if(width < font / 2)return ; 
	while(*str != '\0') 
	{
		if(*str == 0x0D && (*(str + 1) == 0X0A)) 
		{
			str += 2;
			xpos = x;
			ypos += font; 
		}
		else 
		{
			if((xpos + font / 2) > (endx + 1))
			{
				xpos = x;
				ypos += font;
			}
			gui_show_ptchar(xpos, ypos, endx, endy, 0, fcolor, font, *str, 1);
			xpos += font / 2;
			str += 1;
		}
	}
}

void BeepSet(u8 stat)
{
	if(stat == TRUE)
		BeepFlg = TRUE;
	else
	{
		BeepFlg = FALSE;
		BEEP = FALSE;
	}
}

