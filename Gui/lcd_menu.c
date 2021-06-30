#include "lcd_menu.h"
#include "sm_plus.h"
#include "guix.h"
#include <math.h>
#include "bsp.h"
#include "stdlib.h"
#include "App_Language.h"
#include "App_Timer.h"
#include "ffconf.h"
#include "ff.h"
#include "App_Timer.h"
#include "App_ADC.h"
#include "temperature.h"


static uint32_t tp_scan_next_refresh_millis = 0;
static uint32_t display_next_refresh_millis = 0;
static uint32_t delay_time_millis = 0;
menuFunc_t CurrentMenu = start_diy;
menuFunc_t lastMenu = start_diy;
menuFunc_t tempMenu = start_diy;

bool redraw_menu = TRUE;
_btn_obj **key_group;

CHILD_DATA ChildData;
static uint32_t tp_scan_next_update_millis = 0;
static uint32_t display_next_update_millis = 0;
u8 *PrintPoint = NULL;		 //弹框显示文字指针
bool windows_flag = FALSE;

float speed_temprature_factor;//温度 速度对应参数 target_tempratue = target_tempratue*speed_temprature_factor*(feedmultiply-100)/100+target_tempratue;

extern block_t *current_block;  // A pointer to the block currently being traced
//访问SD卡中各个目录下文件
#define GCODE_FILE_SHOW_NUM 6
#define FILE_BASE "0:"
#define OK        0
#define CANT_DEEP 1

bool    file_path_status = OK;
u8 file_name_buf[6][100];
u8 gcodefile_index_base = 0;
u8 gcodefile_index_sub  = 0;
gcodefile_s gcodefile_t[6];
u32 gcodefile_index = 0;
u8 gcodefile_path[100] = "0:";//屏幕显示文件路径
u8 OldGcodeFilePath[100] = "0:";	//上级目录名
bool btnshow_change_flag = FALSE;
u8 gcodefile_num = 0;

//===================ADD BY KIM=============================
uint32_t CurrentFileSize = 0, CurrentReadSize = 0;
//==========================================================
//===================ADD BY ZLB=============================
volatile uint8_t errorIndex;
static uint16_t TemperatureADMax = 4051 >> 2; //温度-5度时的采集值
static uint16_t NozzleTemperatureADMin = 90 >> 2; //估计270度
static uint16_t BedTemperatureADMin = 1811 >> 2; //120度
struct ERRORFLAGSTRUCT
{
	uint8_t BedTemperatureOverHigh : 1;
	uint8_t NozzleTemperatureOverHigh : 1;
	uint8_t BedTemperatureOverLow : 1;
	uint8_t NozzleTemperatureOverLow : 1;
	uint8_t NozzleHeatFailure : 1;
	uint8_t BedHeatFailure : 1;
	uint8_t HeatOutControl : 1;
	uint8_t MotorOutRange : 1;
};

union ERRORDATA
{
	struct ERRORFLAGSTRUCT ErrorFlag;
	uint8_t data;

};
union ERRORDATA ErrorData;
//zlb add
volatile uint8_t MotorOutRangeFlag = 0;
volatile uint8_t CheckMotorOutRangeFlag = 0;

u8 *TEXT_J = "确认停止当前打印？";
u8 *TEXT_M = "Stop Printting?";


u8 OldPruse = 1;	//之前的暂停恢复状态 1为正在打印中 2为暂停中
#define COOLTEMP 50	//冷却温度值
u8 fake_temperature = 0;
_btn_obj **screen_key_group;

static CHILD_DATA oldChildData = {0, 0};
u8 FilamentNum = 0;	//00 1号料盘 01 2号料盘 02 3号料盘

void popout_screen(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 2;
	gui_phy.back_color = WHITE;
	if(redraw_menu == TRUE)
	{
		redraw_menu = FALSE;
		gui_draw_arcrectangle(75, 160, 180, 40, 6, 1, GRAYBLUE, GRAYBLUE);
		gui_fill_rectangle(75, 190, 180, 70, WHITE);
		gui_draw_rectangle(75, 190, 180, 70, GRAYBLUE);
		gui_show_strmid(75, 160, 180, 30, BLACK, 16, (u8 *)PrintPoint, 1);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
		if(key_group)
		{
			key_group[0] = btn_creat(90, 205, 60, 40, 0, 2);
			key_group[0]->bkctbl[0] = 0X8452;
			key_group[0]->bkctbl[1] = 0XAD97;
			key_group[0]->bkctbl[2] = 0XAD97;
			key_group[0]->bkctbl[3] = 0X8452;
			key_group[0]->caption = text_display.L_Confirm;
			btn_draw(key_group[0]);

			key_group[1] = btn_creat(180, 205, 60, 40, 1, 2);
			key_group[1]->bkctbl[0] = 0X8452;
			key_group[1]->bkctbl[1] = 0XAD97;
			key_group[1]->bkctbl[2] = 0XAD97;
			key_group[1]->bkctbl[3] = 0X8452;
			key_group[1]->caption = text_display.L_Cancel;
			btn_draw(key_group[1]);

		}
	}
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6)) //
		{
			case 0:

				if(tempMenu == NULL)
				{
					CurrentMenu = lastMenu;
				}
				else
				{
					CurrentMenu = tempMenu;
					tempMenu = NULL;
				}
				break;
			case 1:
				CurrentMenu = lastMenu;
				redraw_menu = TRUE;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}



u8 PrinterMode;// = 0;	//Printer mode 0:3d print 1:laser print

void lcd_update(void)
{
	FunctionCode = FUN_8;
	if(tp_scan_next_refresh_millis < millis())
	{
		tp_dev.scan(0);
		in_obj.get_key(&tp_dev, IN_TYPE_TOUCH);
		tp_scan_next_refresh_millis = millis() + 10;
		if(PrintInfo.printsd == 0)
		{
			WriteComm(0x36);
			WriteData(0x88);
		}
		if(TE == 0)
		{
			error_check();
			(*CurrentMenu)();
		}

	}
}

/*logo*/
u8 *PTH_LOGO = "1:/diy_icon/logo.bmp";

/*主界面*/  //20
u8 *PTH_ZY_DY = "1:/diy_icon/2001.bmp";		//打印图标
u8 *PTH_ZY_KZ = "1:/diy_icon/2002.bmp";		//控制图标
u8 *PTH_ZY_SD = "1:/diy_icon/2003.bmp";		//设置图标
u8 *PTH_ZY_3DP = "1:/diy_icon/2004.bmp";		//3d打印图片
u8 *PTH_ZY_LASER = "1:/diy_icon/2005.bmp";	//激光打印图片
u8 *PTH_ZY_BACK_TYPE = "1:/diy_icon/2006.bmp";	//激光打印图片和3d打印图片背景

/*控制*/
u8 *PTH_KZ1_010 = "1:/diy_icon/1801.bmp";		//预热
u8 *PTH_KZ1_011 = "1:/diy_icon/1802.bmp";		//移动
u8 *PTH_KZ1_012 = "1:/diy_icon/1803.bmp";		//归零
u8 *PTH_KZ1_013 = "1:/diy_icon/1804.bmp";		//挤出
u8 *PTH_KZ1_014 = "1:/diy_icon/1805.bmp";		//调平
u8 *PTH_KZ1_015 = "1:/diy_icon/1806.bmp";		//风扇
u8 *PTH_KZ1_016 = "1:/diy_icon/1807.bmp";		//紧急停止
u8 *PTH_KZ1_030 = "1:/diy_icon/1809.bmp";		//蜂鸣器开
u8 *PTH_KZ1_031 = "1:/diy_icon/1810.bmp";		//蜂鸣器关


u8 *PTH_KZ1_017 = "1:/diy_icon/0301.bmp";		//3D打印图标
u8 *PTH_KZ1_018 = "1:/diy_icon/0202.bmp";		//激光打印小图标

u8 *PTH_KZ1_005 = "1:/diy_icon/0201.bmp";		//控制图标背景
u8 *PTH_KZ1_009 = "1:/diy_icon/1808.bmp";		//调平
/*激光控制*/
u8 *PTH_FH = "1:/diy_icon/0201.bmp";			//返回
//u8 *PTH_KZ2_020 = "1:/diy_icon/0203.bmp";		//移动及原点设置
u8 *PTH_KZ2_021 = "1:/diy_icon/0204.bmp";		//打开激光头
u8 *PTH_KZ2_023 = "1:/diy_icon/0205.bmp";		//关闭激光头
//u8 *PTH_KZ2_022 = "1:/diy_icon/0206.bmp";		//风扇控制


/*设置界面*/
u8 *PTH_SZ_000 = "1:/diy_icon/1701.bmp";		//关于
u8 *PTH_SZ_001 = "1:/diy_icon/1702.bmp";		//语言
u8 *PTH_SZ_002 = "1:/diy_icon/1703.bmp";		//屏幕校准
u8 *PTH_SZ_003 = "1:/diy_icon/1704.bmp";		//状态
u8 *PTH_SZ_004 = "1:/diy_icon/1705.bmp";		//断点续打
u8 *PTH_SZ_005 = "1:/diy_icon/1706.bmp";		//wifi
/*预加热*/
u8 *PTH_YR_009 = "1:/diy_icon/1006.bmp";		//挤出头1小图标
u8 *PTH_YR_010 = "1:/diy_icon/1005.bmp";		//挤出头2小图标
u8 *PTH_YR_011 = "1:/diy_icon/1002.bmp";		//底板小图标
u8 *PTH_YR_000 = "1:/diy_icon/0302.bmp";		//减
u8 *PTH_YR_001 = "1:/diy_icon/0304.bmp";		//加
u8 *PTH_YR_002 = "1:/diy_icon/1001.bmp";		//喷头 底板图标背景
u8 *PTH_YR_005 = "1:/diy_icon/1003.bmp";		//温度1
u8 *PTH_YR_006 = "1:/diy_icon/1004.bmp";		//停止
u8 *PTH_YR_007 = "1:/diy_icon/1007.bmp";		//温度5
u8 *PTH_YR_008 = "1:/diy_icon/1008.bmp";		//温度10
u8 *PTH_YR_012 = "1:/diy_icon/1009.bmp";		//PLA
u8 *PTH_YR_013 = "1:/diy_icon/1010.bmp";		//ABS
u8 *PTH_YR_014 = "1:/diy_icon/1011.bmp";		//减小图标
u8 *PTH_YR_015 = "1:/diy_icon/1012.bmp";		//加小图标
u8 *PTH_YR_016 = "1:/diy_icon/1013.bmp";		//停止小图标


//移动界面
u8 *PTH_YD_000 = "1:/diy_icon/1301.bmp";		//E上
u8 *PTH_YD_001 = "1:/diy_icon/1205.bmp";		//Y上
u8 *PTH_YD_002 = "1:/diy_icon/1201.bmp";		//Z上
u8 *PTH_YD_003 = "1:/diy_icon/1303.bmp";		//E1
u8 *PTH_YD_004 = "1:/diy_icon/1304.bmp";		//E2
u8 *PTH_YD_005 = "1:/diy_icon/1203.bmp";		//X左
u8 *PTH_YD_006 = "1:/diy_icon/1207.bmp";		//归零
u8 *PTH_YD_007 = "1:/diy_icon/1204.bmp";		//x右
u8 *PTH_YD_008 = "1:/diy_icon/1302.bmp";		//E下
u8 *PTH_YD_009 = "1:/diy_icon/1206.bmp";		//Y下
u8 *PTH_YD_010 = "1:/diy_icon/1202.bmp";		//z下
u8 *PTH_YD_011 = "1:/diy_icon/1210.bmp";		//1mm
u8 *PTH_YD_012 = "1:/diy_icon/1209.bmp";		//5mm
u8 *PTH_YD_013 = "1:/diy_icon/1211.bmp";		//10mm
//u8 *PTH_YD_014 = "1:/diy_icon/1208.bmp";		//设置为当前点
//归零
u8 *PTH_GL_000 = "1:/diy_icon/0801.bmp";		//X归零
u8 *PTH_GL_001 = "1:/diy_icon/0802.bmp";		//Y归零
u8 *PTH_GL_002 = "1:/diy_icon/0803.bmp";		//Z归零
u8 *PTH_GL_003 = "1:/diy_icon/0804.bmp";		//归零全部
u8 *PTH_GL_004 = "1:/diy_icon/0805.bmp";		//停止归零

//挤出
u8 *PTH_JC_000 = "1:/diy_icon/1108.bmp";		//小挤出头1
u8 *PTH_JC_001 = "1:/diy_icon/1109.bmp";		//小挤出头2
u8 *PTH_JC_002 = "1:/diy_icon/1101.bmp";		//大挤出头1
u8 *PTH_JC_003 = "1:/diy_icon/1102.bmp";		//大挤出头2
u8 *PTH_JC_004 = "1:/diy_icon/1103.bmp";		//进料
u8 *PTH_JC_005 = "1:/diy_icon/1104.bmp";		//退料
u8 *PTH_JC_006 = "1:/diy_icon/1105.bmp";		//1mm
u8 *PTH_JC_007 = "1:/diy_icon/1106.bmp";		//5mm
u8 *PTH_JC_008 = "1:/diy_icon/1107.bmp";		//10mm
u8 *PTH_JC_009 = "1:/diy_icon/1110.bmp";		//快速
u8 *PTH_JC_010 = "1:/diy_icon/1111.bmp";		//中速
u8 *PTH_JC_011 = "1:/diy_icon/1112.bmp";		//慢速
//手动调平
u8 *PTH_LV_000 = "1:/diy_icon/2101.bmp";		//第一点
u8 *PTH_LV_001 = "1:/diy_icon/2102.bmp";		//第二点
u8 *PTH_LV_002 = "1:/diy_icon/2103.bmp";		//第三点
u8 *PTH_LV_003 = "1:/diy_icon/2104.bmp";		//第四点
u8 *PTH_LV_004 = "1:/diy_icon/2105.bmp";		//第五点
u8 *PTH_LV_005 = "1:/diy_icon/2106.bmp";		//背景
u8 *PTH_LV_006 = "1:/diy_icon/2107.bmp";		//加
u8 *PTH_LV_007 = "1:/diy_icon/2108.bmp";		//减
u8 *PTH_LV_008 = "1:/diy_icon/2109.bmp";		//自动调平
u8 *PTH_LV_009 = "1:/diy_icon/2110.bmp";		//设置Z轴偏移
u8 *PTH_LV_010 = "1:/diy_icon/2111.bmp";		//偏移值调整的步长0.1
u8 *PTH_LV_011 = "1:/diy_icon/2112.bmp";		//偏移值调整的步长0.05

//风扇
u8 *PTH_FS_000 = "1:/diy_icon/0901.bmp";		//风扇小图标1
u8 *PTH_FS_001 = "1:/diy_icon/1001.bmp";		//风扇背景
u8 *PTH_FS_004 = "1:/diy_icon/0904.bmp";		//速度1
u8 *PTH_FS_005 = "1:/diy_icon/0905.bmp";		//速度2
u8 *PTH_FS_006 = "1:/diy_icon/0906.bmp";		//速度3
u8 *PTH_FS_007 = "1:/diy_icon/1004.bmp";		//停止
u8 *PTH_FS_008 = "1:/diy_icon/0902.bmp";		//风扇小图标2
u8 *PTH_FS_009 = "1:/diy_icon/0903.bmp";		//风扇小图标3


//语言
u8 *PTH_YY_000 = "1:/diy_icon/0501.bmp";		//打勾
u8 *PTH_YY_001 = "1:/diy_icon/0502.bmp";		//简体中文
u8 *PTH_YY_002 = "1:/diy_icon/0503.bmp";		//英文
u8 *PTH_YY_003 = "1:/diy_icon/0504.bmp";		//德文
u8 *PTH_YY_004 = "1:/diy_icon/0505.bmp";		//法文
u8 *PTH_YY_005 = "1:/diy_icon/0506.bmp";		//西班牙文
u8 *PTH_YY_006 = "1:/diy_icon/0507.bmp";		//葡萄牙文
u8 *PTH_YY_007 = "1:/diy_icon/0508.bmp";		//日文
u8 *PTH_YY_008 = "1:/diy_icon/0509.bmp";		//意大利文

//状态
u8 *PTH_ZT_000 = "1:/diy_icon/0702.bmp";		//挤出头1
u8 *PTH_ZT_001 = "1:/diy_icon/0703.bmp";		//挤出头2
u8 *PTH_ZT_002 = "1:/diy_icon/0701.bmp";		//底板
u8 *PTH_ZT_003 = "1:/diy_icon/0704.bmp";		//框图

//打印
u8 *PTH_DY_000 = "1:/diy_icon/2004.bmp";		//3D缩略图
u8 *PTH_DY_001 = "1:/diy_icon/1901.bmp";		//打印时长
u8 *PTH_DY_002 = "1:/diy_icon/1913.bmp";		//挤出头1温度
u8 *PTH_DY_003 = "1:/diy_icon/1911.bmp";		//底板温度
u8 *PTH_DY_004 = "1:/diy_icon/1912.bmp";		//挤出头2温度
u8 *PTH_DY_005 = "1:/diy_icon/1903.bmp";		//打印速度
u8 *PTH_DY_006 = "1:/diy_icon/0101.bmp";		//打印暂停 激光
u8 *PTH_DY_007 = "1:/diy_icon/0102.bmp";		//打印继续 激光
u8 *PTH_DY_008 = "1:/diy_icon/0103.bmp";		//打印停止	激光
u8 *PTH_DY_106 = "1:/diy_icon/1910.bmp";		//打印暂停 3D
u8 *PTH_DY_107 = "1:/diy_icon/1907.bmp";		//打印继续 3D
u8 *PTH_DY_108 = "1:/diy_icon/1908.bmp";		//打印停止	3D
u8 *PTH_DY_009 = "1:/diy_icon/1906.bmp";		//打印控制
u8 *PTH_DY_011 = "1:/diy_icon/0106.bmp";		//显示缩略图背景
u8 *PTH_DY_012 = "1:/diy_icon/2005.bmp";		//激光打印缩略图
u8 *PTH_DY_013 = "1:/diy_icon/1909.bmp";		//3D打印进度条 背景
u8 *PTH_DY_014 = "1:/diy_icon/1902.bmp";		//3D打印缩略图 背景
u8 *PTH_DY_015 = "1:/diy_icon/1904.bmp";		//3D打印项目 背景
u8 *PTH_DY_016 = "1:/diy_icon/0108.bmp";		//激光雕刻进度条 时长 背景
//打印控制

u8 *PTH_DYKZ_001 = "1:/diy_icon/0401.bmp";		//加热控制//
u8 *PTH_DYKZ_002 = "1:/diy_icon/0403.bmp";		//速度控制//
u8 *PTH_DYKZ_003 = "1:/diy_icon/0410.bmp";		//背景图//
u8 *PTH_DYKZ_004 = "1:/diy_icon/1903.bmp";		//打印速度//
u8 *PTH_DYKZ_005 = "1:/diy_icon/0406.bmp";		//挤出头1//
u8 *PTH_DYKZ_006 = "1:/diy_icon/0407.bmp";		//挤出头2//
u8 *PTH_DYKZ_007 = "1:/diy_icon/0411.bmp";		//底板//
u8 *PTH_DYKZ_008 = "1:/diy_icon/0408.bmp";		//风扇1//
u8 *PTH_DYKZ_009 = "1:/diy_icon/0409.bmp";		//风扇2//
u8 *PTH_DYKZ_000 = "1:/diy_icon/0402.bmp";		//风扇控制//
u8 *PTH_DYKZ_011 = "1:/diy_icon/0405.bmp";		//挤出比//
u8 *PTH_DYKZ_012 = "1:/diy_icon/0404.bmp";		//挤出比控制//

//打印速度
u8 *PTH_DYSD_000 = "1:/diy_icon/0303.bmp";		//速度标志
u8 *PTH_DYSD_001 = "1:/diy_icon/0306.bmp";		//10
u8 *PTH_DYSD_002 = "1:/diy_icon/0307.bmp";		//50
u8 *PTH_DYSD_003 = "1:/diy_icon/0308.bmp";		//100
u8 *PTH_DYSD_004 = "1:/diy_icon/0305.bmp";		//恢复常速

//文件选择
u8 *PTH_FILE_000 = "1:/diy_icon/1504.bmp";	    //文件背景
u8 *PTH_FILE_001 = "1:/diy_icon/1503.bmp";       //文件夹背景
u8 *PTH_FILE_002 = "1:/diy_icon/1501.bmp";	    //向上翻页
u8 *PTH_FILE_003 = "1:/diy_icon/1502.bmp";       //向下翻页
u8 *PTH_FILE_004 = "1:/diy_icon/1507.bmp";       //文件/夹到尾遮挡

static u8 CheckPrintFlg = 0;	//打印状态检测标志
TextDisplay text_display;


u8 display_redraw = 0;	//draw icon step-by-step

void start_diy(void)
{
	minibmp_decode(PTH_LOGO, 0, 0, 320, 480, 0, 0);
	u8 i;
	for(i = 0; i < 20; i++)
	{
		delay_ms(100);
	}
	CurrentMenu = main_diy;
	redraw_menu = TRUE;
}

void main_diy(void) //20
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 4;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		display_next_update_millis = millis() - 1000;
		LCD_Clear(gui_phy.back_color);
		gui_fill_rectangle(17, 19, 286, 161, BACK2_DIY);
		PrinterMode == 0 ? minibmp_decode(PTH_ZY_3DP, 95, 40, 112, 114, 0, 0) : minibmp_decode(PTH_ZY_LASER, 108, 40, 103, 103, 0, 0);
		if(PrinterMode == 0)
		{
			//minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
			memset(fatbuf, 0, 50);
			if(text_display.language_choice == 0)
			{
				if(EXTRUDER_NUM == 2)
					sprintf(fatbuf, "双挤出");
				else
					sprintf(fatbuf, "单挤出");
			}
			else
			{
				if(EXTRUDER_NUM == 2)
					sprintf(fatbuf, "DoubleExtruder");
				else
					sprintf(fatbuf, "SingleExtruder");
			}
			gui_show_strmid(17, 155, 286, 16, WHITE, 16, fatbuf, 1);
		}
		else if(PrinterMode == 1)
		{
			//minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
			if(text_display.language_choice == 0)
			{
				gui_show_strmid(17, 155, 286, 16, WHITE, 16, "当前模式：激光雕刻", 1);
			}
			else
			{
				gui_show_strmid(17, 155, 286, 16, WHITE, 16, "Current Mode：Laser Carve", 1);
			}

		}
		//显示三个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(17, 191, 286, 81, 0, 1);
			key_group[1] = btn_creat(17, 282, 286, 81, 1, 1);
			key_group[2] = btn_creat(17, 376, 286, 81, 2, 1);
			key_group[3] = btn_creat(17, 0, 286, 81, 2, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_ZY_DY;
			key_group[0]->picbtnpathd = (u8 *)PTH_ZY_DY;

			key_group[1]->picbtnpathu = (u8 *)PTH_ZY_KZ;
			key_group[1]->picbtnpathd = (u8 *)PTH_ZY_KZ;

			key_group[2]->picbtnpathu = (u8 *)PTH_ZY_SD;
			key_group[2]->picbtnpathd = (u8 *)PTH_ZY_SD;

			for(i = 0; i < num; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		//显示文字
		gui_phy.back_color = BACK2_DIY;	//改为按键背景色
		gui_show_ptstr(195, 223, 195 + 80, 223 + 20, 0, WHITE, 16, text_display.L_Print, 0);
		gui_show_ptstr(195, 314, 195 + 80, 314 + 20, 0, WHITE, 16, text_display.L_Ctol, 0);
		gui_show_ptstr(195, 408, 195 + 80, 408 + 20, 0, WHITE, 16, text_display.L_Set, 0);

	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = TRUE;
				CurrentMenu = GetGcodeTab2_diy;
				break;
			case 1:
				redraw_menu = TRUE;
				if(PrinterMode == 0)
					CurrentMenu = control_3d_diy;		//3d打印控制界面
				else if(PrinterMode == 1)
					CurrentMenu = move2_diy;		//激光打印控制界面
				break;
			case 2:
				redraw_menu = TRUE;
				CurrentMenu = set_diy;		//设置界面
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

void set_diy(void)												//设置显示页面  设定  17
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 7;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//显示7个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(16, 69, 137, 120, 0, 1);
			key_group[1] = btn_creat(163, 69, 137, 120, 1, 1);
			key_group[2] = btn_creat(16, 197, 137, 120, 2, 1);
			key_group[3] = btn_creat(163, 197, 137, 120, 3, 1);
			key_group[4] = btn_creat(16, 325, 137, 120, 4, 1);
			key_group[5] = btn_creat(163, 325, 137, 120, 5, 1);
			key_group[6] = btn_creat(0, 0, 320, 54, 6, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_SZ_000;
			key_group[0]->picbtnpathd = (u8 *)PTH_SZ_000;
			key_group[1]->picbtnpathu = (u8 *)PTH_SZ_001;
			key_group[1]->picbtnpathd = (u8 *)PTH_SZ_001;
			key_group[2]->picbtnpathu = (u8 *)PTH_SZ_002;
			key_group[2]->picbtnpathd = (u8 *)PTH_SZ_002;
			key_group[3]->picbtnpathu = (u8 *)PTH_SZ_003;
			key_group[3]->picbtnpathd = (u8 *)PTH_SZ_003;
			if(PrinterMode == 0)
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_SZ_004;
				key_group[4]->picbtnpathd = (u8 *)PTH_SZ_004;
			}

			for(i = 0; i < num - 2; i++)
			{
				btn_draw(key_group[i]);
			}
			gui_show_strmid(16, 69 + 75, 137, 16, WHITE, 16, text_display.L_About, 1);
			gui_show_strmid(163, 69 + 75, 137, 16, WHITE, 16, text_display.L_Language, 1);
			gui_show_strmid(16, 197 + 75, 137, 16, WHITE, 16, text_display.L_Adjust, 1);
			gui_show_strmid(163, 197 + 75, 137, 16, WHITE, 16, text_display.L_Status, 1);
			if(PrinterMode == 0)gui_show_strmid(16, 325 + 75, 137, 16, WHITE, 16, text_display.L_Continue, 1);
		}
		//显示标题
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 19, 120, 16, WHITE, 16, text_display.L_Set, 0);


	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = TRUE;
				CurrentMenu = about_diy;
				break;
			case 1:
				redraw_menu = TRUE;
				CurrentMenu = language_diy;
				break;
			case 2:
				TP_Adjust();
				redraw_menu = TRUE;
				break;
			case 3:
				redraw_menu = TRUE;
				CurrentMenu = status_diy;
				break;
			case 4:
				if(PrinterMode == 0)
				{
					redraw_menu = TRUE;
					CurrentMenu = GetGcodeTab2_diy;
					reprint.break_point_flg = TRUE;		//置位断点续打标志
				}
				break;
			case 5:
				//redraw_menu = TRUE;
				//CurrentMenu = wifi_diy;
				break;
			case 6:
				redraw_menu = TRUE;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}


void control_3d_diy(void)					//控制显示界面 18
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 9;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{

		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//		minibmp_decode(PTH_KZ1_005,16,169,91,54,0,0);
		//		minibmp_decode(PTH_KZ1_005,212,49,91,54,0,0);
		//显示8个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建8个图标
		if(key_group)
		{
			key_group[0] = btn_creat(18, 72, 137, 83, 0, 1);
			key_group[1] = btn_creat(163, 72, 137, 83, 1, 1);

			//91 -24 /2
			key_group[2] = btn_creat(18, 165, 137, 83, 2, 1);

			key_group[3] = btn_creat(163, 165, 137, 83, 3, 1);
			key_group[4] = btn_creat(18, 258, 137, 83, 4, 1);
			key_group[5] = btn_creat(163, 258, 137, 83, 5, 1);
			key_group[6] = btn_creat(18, 351, 137, 83, 6, 1);
			key_group[7] = btn_creat(163, 351, 137, 83, 7, 1);
			key_group[8] = btn_creat(0, 0, 320, 54, 8, 1);
			key_group[0]->picbtnpathu = (u8 *)PTH_KZ1_010;
			key_group[0]->picbtnpathd = (u8 *)PTH_KZ1_010;
			key_group[1]->picbtnpathu = (u8 *)PTH_KZ1_011;
			key_group[1]->picbtnpathd = (u8 *)PTH_KZ1_011;
			key_group[2]->picbtnpathu = (u8 *)PTH_KZ1_012;
			key_group[2]->picbtnpathd = (u8 *)PTH_KZ1_012;
			key_group[3]->picbtnpathu = (u8 *)PTH_KZ1_013;
			key_group[3]->picbtnpathd = (u8 *)PTH_KZ1_013;
			if(LevelMode == 0)
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_KZ1_014;
				key_group[4]->picbtnpathd = (u8 *)PTH_KZ1_014;
			}
			else
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_KZ1_009;
				key_group[4]->picbtnpathd = (u8 *)PTH_KZ1_009;
			}

			key_group[5]->picbtnpathu = (u8 *)PTH_KZ1_015;
			key_group[5]->picbtnpathd = (u8 *)PTH_KZ1_015;
			key_group[6]->picbtnpathu = (u8 *)PTH_KZ1_016;
			key_group[6]->picbtnpathd = (u8 *)PTH_KZ1_016;
			if(BeepSwitch)
			{
				key_group[7]->picbtnpathu = (u8 *)PTH_KZ1_030;
				key_group[7]->picbtnpathd = (u8 *)PTH_KZ1_030;
			}
			else
			{
				key_group[7]->picbtnpathu = (u8 *)PTH_KZ1_031;
				key_group[7]->picbtnpathd = (u8 *)PTH_KZ1_031;
			}
			//key_group[8]->picbtnpathu = (u8 *)PTH_FH;
			//key_group[8]->picbtnpathd = (u8 *)PTH_FH;

			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
			//显示文字
			gui_show_strmid(18, 120, 137, 16, WHITE, 16, text_display.L_Preheat, 1);
			gui_show_strmid(163, 72 + 48, 137, 16, WHITE, 16, text_display.L_Move, 1);
			gui_show_strmid(18, 165 + 48, 137, 16, WHITE, 16, text_display.L_Zero, 1);
			gui_show_strmid(163, 165 + 48, 137, 16, WHITE, 16, text_display.L_Extrusion, 1);
			if(LevelMode == 0)
				gui_show_strmid(18, 258 + 48, 137, 16, WHITE, 16, text_display.L_Leveling, 1);
			else
				gui_show_strmid(18, 258 + 48, 137, 16, WHITE, 16, text_display.L_autoleveling, 1);
			gui_show_strmid(163, 258 + 48, 137, 16, WHITE, 16, text_display.L_Fan, 1);
			gui_show_strmid(18, 351 + 48, 137, 16, WHITE, 16, text_display.L_Stop, 1);

		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Ctol, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = TRUE;
				lastMenu = CurrentMenu;
				CurrentMenu = preheat_diy;
				break;
			case 1:
				redraw_menu = TRUE;
				CurrentMenu = move1_diy;
				break;
			case 2:
				redraw_menu = TRUE;
				CurrentMenu = zero_diy;
				break;
			case 3:
				redraw_menu = TRUE;
				lastMenu = CurrentMenu;
				CurrentMenu = extruder_diy;
				break;
			case 4:
				redraw_menu = TRUE;
				if(LevelMode == 0)
					CurrentMenu = leveling_diy;
				else if(LevelMode == 1)
					CurrentMenu = AutoLeveling_diy;

				break;
			case 5:
				redraw_menu = TRUE;
				lastMenu = CurrentMenu;
				CurrentMenu = fan_diy;
				break;
			case 6:
				gui_show_strmid(18, 351 + 48, 137, 16, WHITE, 16, text_display.L_Stop, 1);
				disable_x();
				disable_y();
				disable_z();
				break;
			case 7:
				BeepSwitch ^= 0x01;
				if(BeepSwitch)
				{
					key_group[7]->picbtnpathu = (u8 *)PTH_KZ1_030;
					key_group[7]->picbtnpathd = (u8 *)PTH_KZ1_030;

				}
				else
				{
					key_group[7]->picbtnpathu = (u8 *)PTH_KZ1_031;
					key_group[7]->picbtnpathd = (u8 *)PTH_KZ1_031;
				}
				btn_draw(key_group[7]);
				AT24CXX_Write(E_BEEP, (u8 *)&BeepSwitch, sizeof(BeepSwitch));
				break;
			case 8:
				redraw_menu = TRUE;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

void control_laser_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 4;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = false;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//显示4个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建8个图标
		if(key_group)
		{
			key_group[0] = btn_creat(37, 93, 247, 104, 0, 1);
			key_group[1] = btn_creat(37, 213, 247, 104, 1, 1);
			key_group[2] = btn_creat(37, 333, 247, 104, 2, 1);
			key_group[3] = btn_creat(0, 0, 320, 54, 7, 1);

			//		key_group[0]->picbtnpathu = (u8 *)PTH_KZ2_020;
			//	key_group[0]->picbtnpathd = (u8 *)PTH_KZ2_020;
			if(LaserSwitch)
			{
				key_group[1]->picbtnpathu = (u8 *)PTH_KZ2_021;
				key_group[1]->picbtnpathd = (u8 *)PTH_KZ2_021;
			}
			if(LaserSwitch == 0)
			{
				key_group[1]->picbtnpathu = (u8 *)PTH_KZ2_023;
				key_group[1]->picbtnpathd = (u8 *)PTH_KZ2_023;
			}


			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_LaserCtol, 0);

	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = true;
				lastMenu = CurrentMenu;
				CurrentMenu = move2_diy;
				break;
			case 1:	//激光控制
				if(LaserSwitch)	//打开
				{
					LaserSwitch = FALSE;
					key_group[1]->picbtnpathu = (u8 *)PTH_KZ2_023;
					key_group[1]->picbtnpathd = (u8 *)PTH_KZ2_023;
				}
				else if(LaserSwitch == 0)
				{
					TIM1->CCR2 = 700;
					LaserSwitch = true;
					key_group[1]->picbtnpathu = (u8 *)PTH_KZ2_021;
					key_group[1]->picbtnpathd = (u8 *)PTH_KZ2_021;
				}
				LASER = LaserSwitch;
				btn_draw(key_group[1]);
				break;
			case 2:
				redraw_menu = true;
				lastMenu = CurrentMenu;
				CurrentMenu = fan_diy;
				break;
			case 3:
				redraw_menu = true;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
//预热
static u8 preheat_choice = 0;	//加热类型选择 0：挤出头1加热 1：挤出头2加热 2：底板加热
static u8 heat_unit = 1;		//加热温度单位 1度 5度 10度
void preheat_diy(void)						//加热控制显示界面
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 8;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		if(PrintInfo.printsd)	//如果是从打印界面过来的需要做个标志，实时检测断料和打印完成
		{
			CheckPrintFlg = TRUE;
		}
		else
			CheckPrintFlg = FALSE;
		switch(display_redraw)
		{
			case 0:
				//画背景色
				LCD_Clear(gui_phy.back_color);
				break;
			case 1:
				//显示8个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
				if(key_group)
				{
					key_group[0] = btn_creat(24, 265, 132, 87, 0, 1); //减
					key_group[1] = btn_creat(167, 265, 132, 87, 1, 1); //加
					key_group[2] = btn_creat(24, 81, 274, 74, 2, 1); //喷头 底板背景
					key_group[3] = btn_creat(24, 361, 132, 87, 3, 1); //temperature degree
					key_group[4] = btn_creat(167, 361, 132, 87, 4, 1); //停止

					key_group[5] = btn_creat(24, 169, 132, 87, 5, 1);	//PLA
					key_group[6] = btn_creat(167, 169, 132, 87, 6, 1);	//ABS
					key_group[7] = btn_creat(0, 0, 480, 54, 7, 1);		//返回

					key_group[0]->picbtnpathu = (u8 *)PTH_YR_014;
					key_group[0]->picbtnpathd = (u8 *)PTH_YR_014;
					key_group[1]->picbtnpathu = (u8 *)PTH_YR_015;
					key_group[1]->picbtnpathd = (u8 *)PTH_YR_015;
					key_group[2]->picbtnpathu = (u8 *)PTH_YR_002;
					key_group[2]->picbtnpathd = (u8 *)PTH_YR_002;

					gui_phy.back_color = BACK2_DIY;
					switch(heat_unit)
					{
						case 1:
							key_group[3]->picbtnpathu = (u8 *)PTH_YR_005;
							key_group[3]->picbtnpathd = (u8 *)PTH_YR_005;
							break;
						case 5:
							key_group[3]->picbtnpathu = (u8 *)PTH_YR_007;
							key_group[3]->picbtnpathd = (u8 *)PTH_YR_007;
							break;
						case 10:
							key_group[3]->picbtnpathu = (u8 *)PTH_YR_008;
							key_group[3]->picbtnpathd = (u8 *)PTH_YR_008;
							break;
						default:
							break;
					}
					key_group[4]->picbtnpathu = (u8 *)PTH_YR_016;
					key_group[4]->picbtnpathd = (u8 *)PTH_YR_016;

					key_group[5]->picbtnpathu = (u8 *)PTH_YR_012;
					key_group[5]->picbtnpathd = (u8 *)PTH_YR_012;

					key_group[6]->picbtnpathu = (u8 *)PTH_YR_013;
					key_group[6]->picbtnpathd = (u8 *)PTH_YR_013;

					for(i = 0; i < num - 1; i++)
					{
						btn_draw(key_group[i]);
					}
				}
				break;
			case 2:
				//显示当前温度
				memset(fatbuf, 0, 50);
				switch(preheat_choice)
				{
					case 0:
						sprintf(fatbuf, "%03d℃/%03d℃", (s16)show_extruder_tp, target_temperature[0]);
						break;
					case 1:
						sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature[1], target_temperature[1]);
						break;
					case 2:
						sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature_bed, target_temperature_bed);
						break;
					default:
						break;
				}
				gui_phy.back_color = BACK2_DIY;
				gui_show_strmid(148, 115 - 7, 90, 16, WHITE, 16, fatbuf, 0);
				switch(preheat_choice)
				{
					case 0:
						minibmp_decode(PTH_YR_009, 98, 105 - 7, 40, 40, 0, 0);
						break;
					case 1:
						minibmp_decode(PTH_YR_010, 98, 105 - 7, 40, 40, 0, 0);
						break;
					case 2:
						minibmp_decode(PTH_YR_011, 98, 105 - 7, 40, 40, 0, 0);
						break;
					default:
						break;
				}
				break;
			case 3:
				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
				gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Preheat, 0);

				break;
			default:
				break;
		}
		if(display_redraw < 3)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
		}

	}
	//断料和打印完成检测
	if(CheckPrintFlg)
	{
		if(PrintInfo.printsd == 0 || PrintInfo.filament) //如果打印完成或者断料都跳到打印界面做处理
		{
			redraw_menu = TRUE;
			CurrentMenu = print_diy;
			CheckPrintFlg = FALSE;
		}
	}
	//刷新温度显示
	if(display_next_update_millis < millis())
	{
		display_next_update_millis = millis() + 500;
		memset(fatbuf, 0, 50);
		switch(preheat_choice)
		{
			case 0:
				sprintf(fatbuf, "%03d℃/%03d℃", (s16)show_extruder_tp, target_temperature[0]);
				break;
			case 1:
				sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature[1], target_temperature[1]);
				break;
			case 2:
				sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature_bed, target_temperature_bed);
				break;
			default:
				break;
		}
		gui_phy.back_color = BACK2_DIY;
		gui_show_strmid(148, 115 - 7, 90, 16, WHITE, 16, fatbuf, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{


			case 0:
				switch(preheat_choice)
				{
					case 0:
						if(target_temperature[0] > heat_unit)
							target_temperature[0] -= heat_unit;
						else
							target_temperature[0] = 0;
						break;
					case 1:	//60机型只操作挤出头1的温控
						if(target_temperature[0] > heat_unit)
							target_temperature[0] -= heat_unit;
						else
							target_temperature[0] = 0;
						break;
					case 2:
						if(target_temperature_bed > heat_unit)
							target_temperature_bed -= heat_unit;
						else
							target_temperature_bed = 0;
						break;
					default:
						break;
				}

				break;
			case 1:
				switch(preheat_choice)
				{
					case 0:
						if(target_temperature[0] < (HEATER_0_MAXTEMP - heat_unit))
							target_temperature[0] += heat_unit;
						else
							target_temperature[0] = HEATER_0_MAXTEMP;
						break;
					case 1:
						if(target_temperature[0] < (HEATER_1_MAXTEMP - heat_unit))
							target_temperature[0] += heat_unit;
						else
							target_temperature[0] = HEATER_1_MAXTEMP;
						break;
					case 2:
						if(target_temperature_bed < (BED_MAXTEMP - heat_unit))
							target_temperature_bed += heat_unit;
						else
							target_temperature_bed = BED_MAXTEMP;
						break;
					default:
						break;
				}
				break;
			case 2:
				preheat_choice += 1;	//加热设备选择
				preheat_choice %= 3;
				switch(preheat_choice)
				{
					case 0:
						minibmp_decode(PTH_YR_009, 98, 105 - 7, 40, 40, 0, 0);
						break;
					case 1:
						minibmp_decode(PTH_YR_010, 98, 105 - 7, 40, 40, 0, 0);
						break;
					case 2:
						minibmp_decode(PTH_YR_011, 98, 105 - 7, 40, 40, 0, 0);
						break;
					default:
						break;
				}
				//btn_draw(key_group[2]);
				break;
			case 3:
				gui_phy.back_color = BACK2_DIY;
				if(heat_unit == 1)
				{
					heat_unit = 5;
					key_group[3]->picbtnpathu = (u8 *)PTH_YR_007;
					key_group[3]->picbtnpathd = (u8 *)PTH_YR_007;

				}
				else if(heat_unit == 5)
				{
					heat_unit = 10;
					key_group[3]->picbtnpathu = (u8 *)PTH_YR_008;
					key_group[3]->picbtnpathd = (u8 *)PTH_YR_008;

				}
				else if(heat_unit == 10)
				{
					heat_unit = 1;
					key_group[3]->picbtnpathu = (u8 *)PTH_YR_005;
					key_group[3]->picbtnpathd = (u8 *)PTH_YR_005;

				}
				btn_draw(key_group[3]);
				break;
			case 4:
				
					switch(preheat_choice)
				{
					case 0:
						target_temperature[0] = 0;
						break;
					case 1:
						target_temperature[1] = 0;
						break;
					case 2:
						target_temperature_bed = 0;
						break;
					default:
						break;
				}		
				
				

				break;
			case 5:
				target_temperature[0] = 200;
				//target_temperature_bed = 0;
				break;
			case 6:
				target_temperature[0] = 230;
				//target_temperature_bed = 0;
				break;
			case 7:
				redraw_menu = TRUE;
				CurrentMenu = lastMenu;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u8 factor_move = 1;	//移动因子 1mm 5mm 10mm
void move1_diy(void)				//移动控制显示界面 3D 13
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 12;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);

		//显示12个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(12, 73, 90, 79, 0, 1);
			key_group[1] = btn_creat(115, 73, 90, 79, 1, 1);
			key_group[2] = btn_creat(217, 73, 90, 79, 2, 1);
			key_group[3] = btn_creat(12, 346, 142, 116, 3, 1);
			key_group[4] = btn_creat(12, 163, 90, 79, 4, 1); //X left
			key_group[5] = btn_creat(115, 163, 90, 79, 5, 1); // return
			key_group[6] = btn_creat(217, 163, 90, 79, 6, 1); //x right
			key_group[7] = btn_creat(12, 254, 90, 79, 7, 1);
			key_group[8] = btn_creat(115, 254, 90, 79, 8, 1);
			key_group[9] = btn_creat(217, 254, 90, 79, 9, 1);
			key_group[10] = btn_creat(165, 346, 142, 116, 10, 1);
			key_group[11] = btn_creat(0, 0, 480, 54, 11, 1);


			key_group[0]->picbtnpathu = (u8 *)PTH_YD_000;
			key_group[0]->picbtnpathd = (u8 *)PTH_YD_000;
			key_group[1]->picbtnpathu = (u8 *)PTH_YD_001;
			key_group[1]->picbtnpathd = (u8 *)PTH_YD_001;
			key_group[2]->picbtnpathu = (u8 *)PTH_YD_002;
			key_group[2]->picbtnpathd = (u8 *)PTH_YD_002;
			if(active_extruder == 0)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_YD_003;
				key_group[3]->picbtnpathd = (u8 *)PTH_YD_003;
			}
			else if(active_extruder)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_YD_004;
				key_group[3]->picbtnpathd = (u8 *)PTH_YD_004;
			}
			key_group[4]->picbtnpathu = (u8 *)PTH_YD_005;
			key_group[4]->picbtnpathd = (u8 *)PTH_YD_005;
			key_group[5]->picbtnpathu = (u8 *)PTH_YD_006;
			key_group[5]->picbtnpathd = (u8 *)PTH_YD_006;
			key_group[6]->picbtnpathu = (u8 *)PTH_YD_007;
			key_group[6]->picbtnpathd = (u8 *)PTH_YD_007;
			key_group[7]->picbtnpathu = (u8 *)PTH_YD_008;
			key_group[7]->picbtnpathd = (u8 *)PTH_YD_008;
			key_group[8]->picbtnpathu = (u8 *)PTH_YD_009;
			key_group[8]->picbtnpathd = (u8 *)PTH_YD_009;
			key_group[9]->picbtnpathu = (u8 *)PTH_YD_010;
			key_group[9]->picbtnpathd = (u8 *)PTH_YD_010;
			if(factor_move == 1)
			{
				key_group[10]->picbtnpathu = (u8 *)PTH_YD_011;
				key_group[10]->picbtnpathd = (u8 *)PTH_YD_011;
			}
			else if(factor_move == 5)
			{
				key_group[10]->picbtnpathu = (u8 *)PTH_YD_012;
				key_group[10]->picbtnpathd = (u8 *)PTH_YD_012;
			}
			else if(factor_move == 10)
			{
				key_group[10]->picbtnpathu = (u8 *)PTH_YD_013;
				key_group[10]->picbtnpathd = (u8 *)PTH_YD_013;
			}
			//			key_group[11]->picbtnpathu = (u8 *)PTH_FH;
			//			key_group[11]->picbtnpathd = (u8 *)PTH_FH;

			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(80, 17, 160, 16, WHITE, 16, text_display.L_Move, 0);



	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 E%.1f F%d", (float)(-factor_move), 600);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 1:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 Y%.1f F%d", (float)(-factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 2:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 Z%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 3:
				if(active_extruder == 0)
				{
					key_group[3]->picbtnpathu = (u8 *)PTH_YD_004;
					key_group[3]->picbtnpathd = (u8 *)PTH_YD_004;
					active_extruder = 1;
				}
				else if(active_extruder == 1)
				{
					key_group[3]->picbtnpathu = (u8 *)PTH_YD_003;
					key_group[3]->picbtnpathd = (u8 *)PTH_YD_003;
					active_extruder = 0;
				}
				btn_draw(key_group[3]);
				break;
			case 4:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 X%.1f F%d", (float)(-factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 5:
				enquecommand("G28");
				break;
			case 6:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 X%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 7:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 E%.1f F%d", (float)factor_move, 600);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 8:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 Y%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 9:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 Z%.1f F%d", (float)(-factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 10:
				if(factor_move == 1)
				{
					key_group[10]->picbtnpathu = (u8 *)PTH_YD_012;
					key_group[10]->picbtnpathd = (u8 *)PTH_YD_012;
					factor_move = 5;
				}
				else if(factor_move == 5)
				{
					key_group[10]->picbtnpathu = (u8 *)PTH_YD_013;
					key_group[10]->picbtnpathd = (u8 *)PTH_YD_013;
					factor_move = 10;
				}
				else if(factor_move == 10)
				{
					key_group[10]->picbtnpathu = (u8 *)PTH_YD_011;
					key_group[10]->picbtnpathd = (u8 *)PTH_YD_011;
					factor_move = 1;
				}
				btn_draw(key_group[10]);
				break;
			case 11:
				redraw_menu = TRUE;
				enquecommand("T0");
				CurrentMenu = control_3d_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
void move2_diy(void) //激光 移动  12
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 10;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = false;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//显示10个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(64, 73, 90, 79, 0, 1);
			key_group[1] = btn_creat(167, 73, 90, 79, 1, 1);
			key_group[2] = btn_creat(12, 346, 142, 116, 3, 1); //speed degree
			key_group[3] = btn_creat(12, 163, 90, 79, 4, 1);
			key_group[4] = btn_creat(115, 163, 90, 79, 5, 1);
			key_group[5] = btn_creat(217, 163, 90, 79, 6, 1);
			key_group[6] = btn_creat(64, 254, 90, 79, 7, 1);
			key_group[7] = btn_creat(167, 254, 90, 79, 8, 1);
			key_group[8] = btn_creat(165, 346, 142, 116, 10, 1);
			key_group[9] = btn_creat(0, 0, 320, 54, 5, 1);


			key_group[0]->picbtnpathu = (u8 *)PTH_YD_002;	//Z up
			key_group[0]->picbtnpathd = (u8 *)PTH_YD_002;
			key_group[1]->picbtnpathu = (u8 *)PTH_YD_001;	//Y up
			key_group[1]->picbtnpathd = (u8 *)PTH_YD_001;
			if(factor_move == 1)
			{
				key_group[2]->picbtnpathu = (u8 *)PTH_YD_011;
				key_group[2]->picbtnpathd = (u8 *)PTH_YD_011;
			}
			else if(factor_move == 5)
			{
				key_group[2]->picbtnpathu = (u8 *)PTH_YD_012;
				key_group[2]->picbtnpathd = (u8 *)PTH_YD_012;
			}
			else if(factor_move == 10)
			{
				key_group[2]->picbtnpathu = (u8 *)PTH_YD_013;
				key_group[2]->picbtnpathd = (u8 *)PTH_YD_013;
			}


			key_group[3]->picbtnpathu = (u8 *)PTH_YD_005;
			key_group[3]->picbtnpathd = (u8 *)PTH_YD_005;
			key_group[4]->picbtnpathu = (u8 *)PTH_YD_006;
			key_group[4]->picbtnpathd = (u8 *)PTH_YD_006;
			key_group[5]->picbtnpathu = (u8 *)PTH_YD_007;
			key_group[5]->picbtnpathd = (u8 *)PTH_YD_007;

			key_group[6]->picbtnpathu = (u8 *)PTH_YD_010;
			key_group[6]->picbtnpathd = (u8 *)PTH_YD_010;
			key_group[7]->picbtnpathu = (u8 *)PTH_YD_009;
			key_group[7]->picbtnpathd = (u8 *)PTH_YD_009;
			if(LaserSwitch)
			{
				key_group[8]->picbtnpathu = (u8 *)PTH_KZ2_021;
				key_group[8]->picbtnpathd = (u8 *)PTH_KZ2_021;
			}
			if(LaserSwitch == 0)
			{
				key_group[8]->picbtnpathu = (u8 *)PTH_KZ2_023;
				key_group[8]->picbtnpathd = (u8 *)PTH_KZ2_023;
			}


			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(80, 17, 160, 16, WHITE, 16, text_display.L_LaserMove, 0);
		//显示机器类型
		//		if(PrinterMode == 1)
		//		{
		//			minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
		//		}
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 Z%.1f F%d", (float)(factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 1:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 Y%.1f F%d", (float)(-factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 2:
				if(factor_move == 1)
				{
					key_group[2]->picbtnpathu = (u8 *)PTH_YD_012;
					key_group[2]->picbtnpathd = (u8 *)PTH_YD_012;
					factor_move = 5;
				}
				else if(factor_move == 5)
				{
					key_group[2]->picbtnpathu = (u8 *)PTH_YD_013;
					key_group[2]->picbtnpathd = (u8 *)PTH_YD_013;
					factor_move = 10;
				}
				else if(factor_move == 10)
				{
					key_group[2]->picbtnpathu = (u8 *)PTH_YD_011;
					key_group[2]->picbtnpathd = (u8 *)PTH_YD_011;
					factor_move = 1;
				}
				btn_draw(key_group[2]);
				break;
			case 3:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 X%.1f F%d", (float)(-factor_move), 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 4:
				enquecommand("G28");
				break;
			case 5:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 X%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 6:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 Z-%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 7:
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G0 Y%.1f F%d", (float)factor_move, 1200);
				enquecommand(fatbuf);
				enquecommand("G90");
				break;
			case 8:
				if(LaserSwitch)	//打开
				{
					LaserSwitch = FALSE;
					key_group[8]->picbtnpathu = (u8 *)PTH_KZ2_023;
					key_group[8]->picbtnpathd = (u8 *)PTH_KZ2_023;
				}
				else if(LaserSwitch == 0)
				{
					TIM1->CCR2 = 700;
					LaserSwitch = true;
					key_group[8]->picbtnpathu = (u8 *)PTH_KZ2_021;
					key_group[8]->picbtnpathd = (u8 *)PTH_KZ2_021;
				}
				LASER = LaserSwitch;
				btn_draw(key_group[8]);
				break;
			case 9:
				redraw_menu = true;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}


//归零
void zero_diy(void) //8
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 6;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);

		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(24, 176, 130, 122, 0, 1);
			key_group[1] = btn_creat(167, 176, 130, 122, 1, 1);
			key_group[2] = btn_creat(24, 314, 130, 122, 2, 1);
			key_group[3] = btn_creat(167, 314, 130, 122, 3, 1);
			key_group[4] = btn_creat(24, 88, 274, 74, 4, 1);
			key_group[5] = btn_creat(0, 0, 320, 54, 5, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_GL_000;
			key_group[0]->picbtnpathd = (u8 *)PTH_GL_000;
			key_group[1]->picbtnpathu = (u8 *)PTH_GL_001;
			key_group[1]->picbtnpathd = (u8 *)PTH_GL_001;
			key_group[2]->picbtnpathu = (u8 *)PTH_GL_002;
			key_group[2]->picbtnpathd = (u8 *)PTH_GL_002;
			key_group[3]->picbtnpathu = (u8 *)PTH_GL_003;
			key_group[3]->picbtnpathd = (u8 *)PTH_GL_003;
			key_group[4]->picbtnpathu = (u8 *)PTH_GL_004;
			key_group[4]->picbtnpathd = (u8 *)PTH_GL_004;
			//			key_group[5]->picbtnpathu = (u8 *)PTH_FH;
			//			key_group[5]->picbtnpathd = (u8 *)PTH_FH;

			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Zero, 0);
		//显示机器类型
		//		if(PrinterMode == 0)
		//		{
		//			minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
		//		}
		//		else if(PrinterMode == 1)
		//		{
		//			minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
		//		}

	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				enquecommand("G28 X");
				break;
			case 1:
				enquecommand("G28 Y");
				break;
			case 2:
				enquecommand("G28 Z");
				break;
			case 3:
				enquecommand("G28");
				break;
			case 4:
				quickStop();
				break;
			case 5:
				redraw_menu = TRUE;
				CurrentMenu = control_3d_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}

}
//挤出
static u8 factor_extruder = 1; //挤出头值 1mm 5mm 10mm
static u16 extruder_speed = 200;	//挤出机速度 200 600 1200
int OldTemperature = 0;
float OldE_Position = 0;
long OldE_Steps = 0;
void extruder_diy(void)					//挤出头控制显示界面  11
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 6;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		OldTemperature = target_temperature[0];
		OldE_Position = current_position[3];		//保存E轴坐标
		OldE_Steps =  current_position[3] * axis_steps_per_unit[E_AXIS];	//E轴当前步进值
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//draw big naddle icon back   BACK2_DIY
		gui_draw_arcrectangle(23, 76, 130, 171, 5, 0, BACK2_DIY, BACK2_DIY);

		//画两个挤出头
		minibmp_decode(PTH_JC_000, 30, 98, 27, 47, 0, 0);
		minibmp_decode(PTH_JC_001, 30, 176, 27, 47, 0, 0);
		//画挤出头背景图



		memset(fatbuf, 0, 50);
		sprintf(fatbuf, "%03d℃/%03d℃", (s16)show_extruder_tp, target_temperature[0]);
		gui_phy.back_color = BACK_DIY;
		gui_show_ptstr(60, 118, 60 + 88, 118 + 16, 0, WHITE, 16, fatbuf, 0);

		memset(fatbuf, 0, 50);
		sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature[1], target_temperature[1]);
		gui_show_ptstr(60, 192, 60 + 88, 192 + 16, 0, WHITE, 16, fatbuf, 0);
		gui_phy.back_color = BACK2_DIY;
		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建六个图标
		if(key_group)
		{
			key_group[0] = btn_creat(166, 76, 130, 171, 0, 1);
			key_group[1] = btn_creat(23, 256, 130, 94, 1, 1);
			key_group[2] = btn_creat(167, 256, 130, 94, 2, 1);
			key_group[3] = btn_creat(23, 363, 130, 94, 3, 1);
			key_group[4] = btn_creat(167, 363, 130, 94, 4, 1);
			key_group[5] = btn_creat(0, 0, 320, 54, 5, 1);
			if(active_extruder == 0)
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_JC_002;
				key_group[0]->picbtnpathd = (u8 *)PTH_JC_002;

			}
			else if(active_extruder == 1)
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_JC_003;
				key_group[0]->picbtnpathd = (u8 *)PTH_JC_003;
				//enquecommand("T0");
			}
			key_group[1]->picbtnpathu = (u8 *)PTH_JC_004;
			key_group[1]->picbtnpathd = (u8 *)PTH_JC_004;
			key_group[2]->picbtnpathu = (u8 *)PTH_JC_005;
			key_group[2]->picbtnpathd = (u8 *)PTH_JC_005;
			if(factor_extruder == 1)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_JC_006;
				key_group[3]->picbtnpathd = (u8 *)PTH_JC_006;
			}
			else if(factor_extruder == 5)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_JC_007;
				key_group[3]->picbtnpathd = (u8 *)PTH_JC_007;
			}
			else if(factor_extruder == 10)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_JC_008;
				key_group[3]->picbtnpathd = (u8 *)PTH_JC_008;
			}
			if(extruder_speed == 200)
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_JC_011;
				key_group[4]->picbtnpathd = (u8 *)PTH_JC_011;
			}
			else if(extruder_speed == 400)
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_JC_010;
				key_group[4]->picbtnpathd = (u8 *)PTH_JC_010;
			}
			else if(extruder_speed == 800)
			{
				key_group[4]->picbtnpathu = (u8 *)PTH_JC_009;
				key_group[4]->picbtnpathd = (u8 *)PTH_JC_009;
			}
			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Extrusion, 0);
		gui_phy.back_color = BACK2_DIY;
		gui_show_strmid(23, 256 + 55, 130, 16, WHITE, 16, text_display.L_Load, 0);
		gui_show_strmid(167, 256 + 55, 130, 16, WHITE, 16, text_display.L_Unload, 0);
		if(extruder_speed == 200)
			gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Slow, 0);
		else if(extruder_speed == 400)
			gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Normal, 0);
		else if(extruder_speed == 800)
			gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Fast, 0);
	}
	//刷新温度显示
	gui_phy.back_color = BACK_DIY;
	if(display_next_update_millis < millis())
	{
		display_next_update_millis = millis() + 500;
		memset(fatbuf, 0, 50);
		sprintf(fatbuf, "%03d℃/%03d℃", (s16)show_extruder_tp, target_temperature[0]);
		gui_show_ptstr(60, 118, 60 + 88, 118 + 16, 0, WHITE, 16, fatbuf, 0);

		memset(fatbuf, 0, 50);
		sprintf(fatbuf, "%03d℃/%03d℃", (s16)current_temperature[1], target_temperature[1]);
		gui_show_ptstr(60, 192, 60 + 88, 192 + 16, 0, WHITE, 16, fatbuf, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				if(active_extruder == 0)
				{
					key_group[0]->picbtnpathu = (u8 *)PTH_JC_003;
					key_group[0]->picbtnpathd = (u8 *)PTH_JC_003;
					active_extruder = 1;
				}
				else if(active_extruder == 1)
				{
					key_group[0]->picbtnpathu = (u8 *)PTH_JC_002;
					key_group[0]->picbtnpathd = (u8 *)PTH_JC_002;
					active_extruder = 0;

				}
				btn_draw(key_group[0]);
				break;
			case 1:
				if(target_temperature[0] < 190)
					enquecommand("M104 S200");
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 E%.1f F%d", (float)factor_extruder, extruder_speed);
				enquecommand(fatbuf);
				enquecommand("G90");
				gui_phy.back_color = BACK2_DIY;
				gui_show_strmid(23, 256 + 55, 130, 16, WHITE, 16, text_display.L_Load, 0);
				gui_phy.back_color = BACK_DIY;
				break;
			case 2:
				if(target_temperature[0] < 190)
					enquecommand("M104 S200");
				enquecommand("G91");
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "G1 E%.1f F%d", (float)(-factor_extruder), extruder_speed);
				enquecommand(fatbuf);
				enquecommand("G90");
				gui_phy.back_color = BACK2_DIY;
				gui_show_strmid(167, 256 + 55, 130, 16, WHITE, 16, text_display.L_Unload, 0);
				gui_phy.back_color = BACK_DIY;
				break;
			case 3:
				if(factor_extruder == 1)
				{
					factor_extruder = 5;
					key_group[3]->picbtnpathu = (u8 *)PTH_JC_007;
					key_group[3]->picbtnpathd = (u8 *)PTH_JC_007;
				}
				else if(factor_extruder == 5)
				{
					factor_extruder = 10;
					key_group[3]->picbtnpathu = (u8 *)PTH_JC_008;
					key_group[3]->picbtnpathd = (u8 *)PTH_JC_008;
				}
				else if(factor_extruder == 10)
				{
					factor_extruder = 1;
					key_group[3]->picbtnpathu = (u8 *)PTH_JC_006;
					key_group[3]->picbtnpathd = (u8 *)PTH_JC_006;
				}
				btn_draw(key_group[3]);
				break;
			case 4:
				//quickStop();
				gui_phy.back_color = BACK2_DIY;
				gui_fill_rectangle(167, 363 + 55, 130, 16, BACK2_DIY);
				if(extruder_speed == 200)
				{
					extruder_speed = 400;
					key_group[4]->picbtnpathu = (u8 *)PTH_JC_010;
					key_group[4]->picbtnpathd = (u8 *)PTH_JC_010;
					btn_draw(key_group[4]);
					gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Normal, 0);
				}
				else if(extruder_speed == 400)
				{
					extruder_speed = 800;
					key_group[4]->picbtnpathu = (u8 *)PTH_JC_009;
					key_group[4]->picbtnpathd = (u8 *)PTH_JC_009;
					btn_draw(key_group[4]);
					gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Fast, 0);
				}
				else if(extruder_speed == 800)
				{
					extruder_speed = 200;
					key_group[4]->picbtnpathu = (u8 *)PTH_JC_011;
					key_group[4]->picbtnpathd = (u8 *)PTH_JC_011;
					btn_draw(key_group[4]);
					gui_show_strmid(167, 363 + 55, 130, 16, WHITE, 16, text_display.L_Slow, 0);
				}

				break;
			case 5:
				//enquecommand("T0");
				redraw_menu = TRUE;
				CurrentMenu = lastMenu;
				target_temperature[0] = OldTemperature;
				current_position[3] = OldE_Position;	//恢复E轴坐标值
				position[3] = OldE_Steps;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u8 LevelingFlg = FALSE;	//手动调平归零标志
void leveling_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 6;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		LevelingFlg = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);

		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(16, 79, 137, 120, 0, 1);
			key_group[1] = btn_creat(163, 79, 137, 120, 1, 1);
			key_group[2] = btn_creat(16, 207, 137, 120, 2, 1);
			key_group[3] = btn_creat(163, 207, 137, 120, 3, 1);
			key_group[4] = btn_creat(16, 340, 137, 120, 4, 1);
			key_group[5] = btn_creat(0, 0, 320, 54, 5, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_LV_000;
			key_group[0]->picbtnpathd = (u8 *)PTH_LV_000;
			key_group[1]->picbtnpathu = (u8 *)PTH_LV_001;
			key_group[1]->picbtnpathd = (u8 *)PTH_LV_001;
			key_group[2]->picbtnpathu = (u8 *)PTH_LV_002;
			key_group[2]->picbtnpathd = (u8 *)PTH_LV_002;
			key_group[3]->picbtnpathu = (u8 *)PTH_LV_003;
			key_group[3]->picbtnpathd = (u8 *)PTH_LV_003;
			key_group[4]->picbtnpathu = (u8 *)PTH_LV_004;
			key_group[4]->picbtnpathd = (u8 *)PTH_LV_004;

			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Leveling, 0);


	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{

		switch(selx & ~(3 << 6))
		{
			case 0:
				if(LevelingFlg == FALSE)
				{
					LevelingFlg = TRUE;
					enquecommand("G28");
				}
				enquecommand("G0 Z5 F800");
				enquecommand("G0 X40 Y40 F1600");
				enquecommand("G0 Z0 F800");
				break;
			case 1:
				if(LevelingFlg == FALSE)
				{
					LevelingFlg = TRUE;
					enquecommand("G28");
				}
				enquecommand("G0 Z5 F800");
				enquecommand("G0 X210 Y40 F1600");
				enquecommand("G0 Z0 F600");
				break;
			case 2:
				if(LevelingFlg == FALSE)
				{
					LevelingFlg = TRUE;
					enquecommand("G28");
				}
				enquecommand("G0 Z5 F800");
				enquecommand("G0 X210 Y210 F1600");
				enquecommand("G0 Z0 F600");
				break;
			case 3:
				if(LevelingFlg == FALSE)
				{
					LevelingFlg = TRUE;
					enquecommand("G28");
				}
				enquecommand("G0 Z5 F800");
				enquecommand("G0 X40 Y210 F1600");
				enquecommand("G0 Z0 F600");
				break;
			case 4:
				if(LevelingFlg == FALSE)
				{
					LevelingFlg = TRUE;
					enquecommand("G28");
				}
				enquecommand("G0 Z5 F800");
				enquecommand("G0 X125 Y125 F1600");
				enquecommand("G0 Z0 F600");
				break;
			case 5:
				redraw_menu = TRUE;
				CurrentMenu = control_3d_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}

}
void AutoLevelingZMove_diy(void)
{

	u8 selx = 0XFF;
	u8 i;
	u8 num = 3;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		LCD_Clear(gui_phy.back_color);
		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(24, 177, 130, 122, 0, 1);
			key_group[1] = btn_creat(167, 177, 130, 122, 1, 1);
			key_group[2] = btn_creat(0, 0, 320, 54, 2, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_LV_006;
			key_group[0]->picbtnpathd = (u8 *)PTH_LV_006;
			key_group[1]->picbtnpathu = (u8 *)PTH_LV_007;
			key_group[1]->picbtnpathd = (u8 *)PTH_LV_007;


			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);

		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%s Z %s", text_display.L_Set, text_display.L_offset);
		gui_show_strmid(80, 17, 160, 16, WHITE, 16, fatbuf, 0);

		minibmp_decode(PTH_LV_005, 24, 88, 274, 74, 0, 0); //背景

		gui_phy.back_color = BACK2_DIY;
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%s:%1.2f  ", text_display.L_Leveling, BL_TOUCH_OFFSET);
		gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);

	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				if(BL_TOUCH_OFFSET < 2.0)
					BL_TOUCH_OFFSET += 0.1;
				gui_phy.back_color = BACK2_DIY;
				gui_draw_rectangle(80, 116, 160, 16, BACK2_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%1.2f  ", text_display.L_Leveling, BL_TOUCH_OFFSET);
				gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);
				break;
			case 1:
				if(BL_TOUCH_OFFSET > -1.0)
					BL_TOUCH_OFFSET -= 0.1;
				gui_phy.back_color = BACK2_DIY;
				gui_draw_rectangle(80, 116, 160, 16, BACK2_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%1.2f  ", text_display.L_Leveling, BL_TOUCH_OFFSET);
				gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				redraw_menu = TRUE;
				AT24CXX_Write(MBL_OFFSET, (u8 *)&BL_TOUCH_OFFSET, sizeof(BL_TOUCH_OFFSET));
				CurrentMenu = AutoLeveling_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
static u8 StepValue = 10;	//调整步长值
void AutoLeveling_diy(void)
{

	u8 selx = 0XFF;
	u8 i, j;
	u8 num = 5;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);

		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(24, 177, 130, 122, 0, 1);
			key_group[1] = btn_creat(167, 177, 130, 122, 1, 1);
			key_group[2] = btn_creat(167, 315, 130, 122, 2, 1);
			key_group[3] = btn_creat(24, 315, 130, 122, 3, 1);
			key_group[4] = btn_creat(0, 0, 320, 54, 4, 1);

			key_group[0]->picbtnpathu = (u8 *)PTH_LV_008;
			key_group[0]->picbtnpathd = (u8 *)PTH_LV_008;
			key_group[1]->picbtnpathu = (u8 *)PTH_LV_006;
			key_group[1]->picbtnpathd = (u8 *)PTH_LV_006;
			key_group[2]->picbtnpathu = (u8 *)PTH_LV_007;
			key_group[2]->picbtnpathd = (u8 *)PTH_LV_007;
			if(10 == StepValue)
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_LV_010;
				key_group[3]->picbtnpathd = (u8 *)PTH_LV_010;
			}
			else
			{
				key_group[3]->picbtnpathu = (u8 *)PTH_LV_011;
				key_group[3]->picbtnpathd = (u8 *)PTH_LV_011;
			}


			for(i = 0; i < num - 1; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(0, 17, 320, 16, WHITE, 16, text_display.L_autoleveling, 0);

		minibmp_decode(PTH_LV_005, 24, 88, 274, 74, 0, 0); //背景

		gui_phy.back_color = BACK2_DIY;
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%s:%1.2f  ", text_display.L_offset, BL_TOUCH_OFFSET);
		gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);


	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				enquecommand("G28L");
				//喷头移动到底板中心位置方便用户查看是否调平
			#ifndef MINI
				enquecommand("G0 X87.25 Y120.75 Z0 F600");
			#else
				enquecommand("G0 X55 Y55 Z0 F600");		//儿童mini款
			#endif
				break;
			case 1:
				i = blocks_queued();
				if(i)	//不能在移动过程中设置补偿值
					break;
				if(BL_TOUCH_OFFSET < 3.0)
					BL_TOUCH_OFFSET += (((float)StepValue) / 100);

				for(i = 0; i < 3; i++)
				{
					for(j = 0; j < 3; j++)
						matrix_offset[i][j] -= (((float)StepValue) / 100);	//在原来的基础上减0.05的偏移值
				}
				enquecommand("G0 Z0 F300");
				gui_phy.back_color = BACK2_DIY;
				gui_draw_rectangle(80, 116, 160, 16, BACK2_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%1.2f  ", text_display.L_Leveling, BL_TOUCH_OFFSET);
				gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				i = blocks_queued();
				if(i)	//不能在移动过程中设置补偿值
					break;
				if(BL_TOUCH_OFFSET > -2.0)
					BL_TOUCH_OFFSET -= (((float)StepValue) / 100);
				for(i = 0; i < 3; i++)
				{
					for(j = 0; j < 3; j++)
						matrix_offset[i][j] += (((float)StepValue) / 100);	//在原来的基础上0.05的偏移值
				}
				enquecommand("G0 Z0 F300");
				gui_phy.back_color = BACK2_DIY;
				gui_draw_rectangle(80, 116, 160, 16, BACK2_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%1.2f  ", text_display.L_Leveling, BL_TOUCH_OFFSET);
				gui_show_strmid(80, 116, 160, 16, WHITE, 16, fatbuf, 0);
				break;
			case 3:
				if(10 == StepValue)
				{
					StepValue = 5;
					key_group[3]->picbtnpathu = (u8 *)PTH_LV_011;
					key_group[3]->picbtnpathd = (u8 *)PTH_LV_011;
				}
				else
				{
					StepValue = 10;
					key_group[3]->picbtnpathu = (u8 *)PTH_LV_010;
					key_group[3]->picbtnpathd = (u8 *)PTH_LV_010;
				}
				btn_draw(key_group[3]);
				break;
			case 4:
				redraw_menu = TRUE;
				CurrentMenu = control_3d_diy;
				AT24CXX_Write(MBL_OFFSET, (u8 *)&BL_TOUCH_OFFSET, sizeof(BL_TOUCH_OFFSET));
				AT24CXX_Write(MBL_VALUE, (u8 *)matrix_offset, sizeof(matrix_offset));
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}




}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
static u8 fan_diy_num = 0;	//控制风扇号 0 1 2
static u8 fan_diy_speed = 10;	//控制风扇速度 10 20 50
#define FAN_SPEED_MIN 10//风扇最低速度
void fan_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 6;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		if(PrintInfo.printsd)	//如果是从打印界面过来的需要做个标志，实时检测断料和打印完成
		{
			CheckPrintFlg = TRUE;
		}
		else
			CheckPrintFlg = FALSE;
		switch(display_redraw)
		{
			case 0:
				//画背景色
				//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
				LCD_Clear(gui_phy.back_color);
				break;
			case 1:
				//显示6个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
				if(key_group)
				{
					key_group[0] = btn_creat(24, 176, 130, 122, 0, 1);
					key_group[1] = btn_creat(167, 176, 130, 122, 1, 1);
					key_group[2] = btn_creat(24, 88, 274, 74, 2, 1);
					key_group[3] = btn_creat(24, 314, 130, 122, 3, 1);
					key_group[4] = btn_creat(167, 314, 130, 122, 4, 1);
					key_group[5] = btn_creat(0, 0, 320, 54, 5, 1);


					key_group[0]->picbtnpathu = (u8 *)PTH_YR_000;
					key_group[0]->picbtnpathd = (u8 *)PTH_YR_000;
					key_group[1]->picbtnpathu = (u8 *)PTH_YR_001;
					key_group[1]->picbtnpathd = (u8 *)PTH_YR_001;

					key_group[2]->picbtnpathu = (u8 *)PTH_FS_001;
					//key_group[2]->picbtnpathd = (u8 *)PTH_FS_001;


					if(fan_diy_speed == 10)
					{
						key_group[3]->picbtnpathu = (u8 *)PTH_FS_004;
						key_group[3]->picbtnpathd = (u8 *)PTH_FS_004;
					}
					else if(fan_diy_speed == 20)
					{
						key_group[3]->picbtnpathu = (u8 *)PTH_FS_005;
						key_group[3]->picbtnpathd = (u8 *)PTH_FS_005;
					}
					else if(fan_diy_speed == 50)
					{
						key_group[3]->picbtnpathu = (u8 *)PTH_FS_006;
						key_group[3]->picbtnpathd = (u8 *)PTH_FS_006;
					}
					key_group[4]->picbtnpathu = (u8 *)PTH_FS_007;
					key_group[4]->picbtnpathd = (u8 *)PTH_FS_007;

					for(i = 0; i < num - 1; i++)
					{
						btn_draw(key_group[i]);
					}
				}
				break;
			case 3:
				gui_phy.back_color = BACK2_DIY;
				if(fan_diy_num == 0)
				{
					sprintf(fatbuf, "%d  ", fanSpeed);
					gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
					minibmp_decode(PTH_FS_000, 120, 105, 40, 40, 0, 0);
				}
				else if(fan_diy_num == 1)
				{
					sprintf(fatbuf, "%d  ", fanSpeed1);
					gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
					minibmp_decode(PTH_FS_008, 120, 105, 40, 40, 0, 0);
				}
				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
				gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Fan, 0);
				break;
			default:
				break;
		}
		if(display_redraw < 3)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
		}

	}
	//断料和打印完成检测
	if(CheckPrintFlg)
	{
		if(PrintInfo.printsd == 0 || PrintInfo.filament) //如果打印完成或者断料都跳到打印界面做处理
		{
			redraw_menu = TRUE;
			CurrentMenu = print_diy;
			CheckPrintFlg = FALSE;
		}
	}
	//按键检测
	gui_phy.back_color = BACK2_DIY;
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{

		switch(selx & ~(3 << 6))
		{
			case 0:
				switch(fan_diy_num)
				{
					case 0:

						fanSpeed -= fan_diy_speed;
						if(fanSpeed < fan_diy_speed)
						{
							fanSpeed = fan_diy_speed;
						}
						if(fanSpeed < FAN_SPEED_MIN)
						{
							fanSpeed = FAN_SPEED_MIN;
						}
						sprintf(fatbuf, "%d  ", fanSpeed);
						break;
					case 1:
						fanSpeed1 -= fan_diy_speed;
						if(fanSpeed1 < fan_diy_speed)
						{
							fanSpeed1 = fan_diy_speed;
						}
						if(fanSpeed1 < FAN_SPEED_MIN)
						{
							fanSpeed1 = FAN_SPEED_MIN;
						}
						sprintf(fatbuf, "%d  ", fanSpeed1);
						break;
						#ifdef FAN3_SHOW
					case 2:
						fanSpeed2 -= fan_diy_speed;
						if(fanSpeed2 < fan_diy_speed)
						{
							fanSpeed2 = fan_diy_speed;
						}
						if(fanSpeed2 < FAN_SPEED_MIN)
						{
							fanSpeed2 = FAN_SPEED_MIN;
						}
						sprintf(fatbuf, "%d  ", fanSpeed2);
						break;
						#endif
					default:
						break;
				}
				gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
				break;
			case 1:

				switch(fan_diy_num)
				{
					case 0:
						if(fanSpeed < (255 - fan_diy_speed))
							fanSpeed += fan_diy_speed;
						else fanSpeed = 255;

						if(fanSpeed > 255)
						{
							fanSpeed = 255;
						}
						sprintf(fatbuf, "%d  ", fanSpeed);
						break;
					case 1:
						if(fanSpeed1 < (255 - fan_diy_speed))
							fanSpeed1 += fan_diy_speed;
						else fanSpeed1 = 255;

						if(fanSpeed1 > 255)
						{
							fanSpeed1 = 255;
						}
						sprintf(fatbuf, "%d  ", fanSpeed1);
						break;
					case 2:
						if(fanSpeed2 < (255 - fan_diy_speed))
							fanSpeed2 += fan_diy_speed;
						else fanSpeed2 = 255;

						if(fanSpeed2 > 255)
						{
							fanSpeed2 = 255;
						}
						sprintf(fatbuf, "%d  ", fanSpeed2);
						break;
					default:
						break;
				}
				gui_phy.back_color = BACK2_DIY;
				gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				fan_diy_num += 1;

				#ifdef FAN3_SHOW
				fan_diy_num %= 3;
				#else
				fan_diy_num %= 2;
				#endif
				gui_phy.back_color = BACK2_DIY;
				switch(fan_diy_num)
				{
					case 0:
						sprintf(fatbuf, "%d  ", fanSpeed);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);

						//显示风扇小图标
						minibmp_decode(PTH_FS_000, 120, 105, 40, 40, 0, 0);
						break;
					case 1:
						sprintf(fatbuf, "%d  ", fanSpeed1);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);

						minibmp_decode(PTH_FS_008, 120, 105, 40, 40, 0, 0);
						break;
						#ifdef FAN3_SHOW
					case 2:
						sprintf(fatbuf, "%d  ", fanSpeed2);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
						minibmp_decode(PTH_FS_009, 120, 105, 40, 40, 0, 0);
						break;
						#endif
					default:
						break;
				}
				gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);

				break;
			case 3:
				if(fan_diy_speed == 10)
				{
					fan_diy_speed = 20;
					key_group[3]->picbtnpathu = (u8 *)PTH_FS_005;
					key_group[3]->picbtnpathd = (u8 *)PTH_FS_005;
				}
				else if(fan_diy_speed == 20)
				{
					fan_diy_speed = 50;
					key_group[3]->picbtnpathu = (u8 *)PTH_FS_006;
					key_group[3]->picbtnpathd = (u8 *)PTH_FS_006;
				}
				else if(fan_diy_speed == 50)
				{
					fan_diy_speed = 10;
					key_group[3]->picbtnpathu = (u8 *)PTH_FS_004;
					key_group[3]->picbtnpathd = (u8 *)PTH_FS_004;
				}

				btn_draw(key_group[3]);
				break;
			case 4:
				switch(fan_diy_num)
				{
					case 0:
						fanSpeed = 0;
						sprintf(fatbuf, "%d  ", fanSpeed);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
						break;
					case 1:
						fanSpeed1 = 0;
						sprintf(fatbuf, "%d  ", fanSpeed1);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);
						break;
						#ifdef FAN3_SHOW
					case 2:
						fanSpeed2 = 0;
						sprintf(fatbuf, "%d  ", fanSpeed2);
						gui_show_strmid(167, 115, 24, 16, WHITE, 16, fatbuf, 0);

						break;
						#endif
					default:
						break;
				}
				break;
			case 5:
				redraw_menu = TRUE;
				CurrentMenu = lastMenu;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void about_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;

	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//fill back color
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		//========================================================================
		gui_show_ptstr(110, 180, 220, 200, 0, WHITE, 16, (u8 *)COM_PILE_DATE, 1);
		gui_show_ptstr(120, 200, 220, 220, 0, WHITE, 16, (u8 *)COM_PILE_TIME, 1);
		//========================================================================
		//显示信息
		//		#ifdef LASER
		//		gui_show_ptstr(80, 220, 250, 236, 0, WHITE, 16, (u8 *)LASR_VERSION, 1);
		//		#else

		gui_show_strmid(0, 220, 320, 16, WHITE, 16, (u8 *)SW_VERSION, 0);
		//		#endif
		//	gui_show_ptstr(80, 240, 250, 260, 0, WHITE, 16, (u8 *)WIFI_VERSION, 1);
		gui_show_strmid(10, 240, 310, 16, WHITE, 16, (u8 *)CO_TD_INFO, 0);				//生产公司名称
		memset(fatbuf, 0, 50);
		sprintf(fatbuf, "configure file version:V%1.2f", ParaVersion);
		gui_show_strmid(0, 260, 320, 16, WHITE, 16, (u8 *)fatbuf, 0);				//配置文件版本
		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{

			key_group[0] = btn_creat(0, 0, 320, 54, 5, 1);

		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_About, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{

			case 0:
				redraw_menu = TRUE;
				CurrentMenu = set_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void set_display_language(void)
{
	switch(text_display.language_choice)
	{
		case Chinese:
			text_display.L_Print = C_Print;
			text_display.L_Ctol = C_Ctol;
			text_display.L_Set = C_Set;
			text_display.L_Preheat = C_Preheat;
			text_display.L_Move = C_Move;
			text_display.L_Extrusion = C_Extrusion;
			text_display.L_Fan = C_Fan;
			text_display.L_About = C_About;
			text_display.L_Language = C_Language;
			text_display.L_Status = C_Status;
			text_display.L_PrintName = C_PrintName;
			text_display.L_Pause = C_Pause;
			text_display.L_Pursue = C_Pursue;
			text_display.L_Stop = C_Stop;
			text_display.L_Tempertuare = C_Tempertuare;
			text_display.L_Speed = C_Speed;
			text_display.L_PrintCtrl = C_PrintCtrl;
			text_display.L_PrintSpeed = C_PrintSpeed;
			text_display.L_SDPrint = C_SDPrint;
			text_display.L_Back = C_Back;
			text_display.L_PrintFinish = C_PrintFinish;
			text_display.L_Confirm = C_Confirm;
			text_display.L_LaserCtol = C_LaserCtol;
			text_display.L_LaserMove = C_LaserMove;
			text_display.L_Zero = C_Zero;
			text_display.L_Ctrl = C_Ctol;
			text_display.L_Cancel = C_Cancel;
			text_display.L_filament = C_Filament;
			text_display.L_StopPrint = TEXT_J;
			text_display.L_Leveling = C_Leveling;
			text_display.L_Adjust = C_Adjust;
			text_display.L_Load = C_Load;
			text_display.L_Unload = C_Unload;
			text_display.L_Fast = C_Fast;
			text_display.L_Normal = C_Normal;
			text_display.L_Slow = C_Slow;
			text_display.L_autoleveling = C_autoleveling;
			text_display.L_offset = C_offset;
			text_display.L_Continue = C_Continue;
			break;
		case English:
			text_display.L_Print = E_Print;
			text_display.L_Ctol = E_Ctol;
			text_display.L_Set = E_Set;
			text_display.L_Preheat = E_Preheat;
			text_display.L_Move = E_Move;
			text_display.L_Extrusion = E_Extrusion;
			text_display.L_Fan = E_Fan;
			text_display.L_About = E_About;
			text_display.L_Language = E_Language;
			text_display.L_Status = E_Status;
			text_display.L_PrintName = E_PrintName;
			text_display.L_Pause = E_Pause;
			text_display.L_Pursue = E_Pursue;
			text_display.L_Stop = E_Stop;
			text_display.L_Tempertuare = E_Tempertuare;
			text_display.L_Speed = E_Speed;
			text_display.L_PrintCtrl = E_PrintCtrl;
			text_display.L_PrintSpeed = E_PrintSpeed;
			text_display.L_SDPrint = E_SDPrint;
			text_display.L_Back = E_Back;
			text_display.L_PrintFinish = E_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = E_LaserCtol;
			text_display.L_LaserMove = E_LaserMove;
			text_display.L_Zero = E_Zero;
			text_display.L_Ctrl = E_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = E_Filament;
			text_display.L_StopPrint = E_StopPrint;
			text_display.L_Leveling = E_Leveling;
			text_display.L_Adjust = E_Adjust;
			text_display.L_Load = E_Load;
			text_display.L_Unload = E_Unload;
			text_display.L_Fast = E_Fast;
			text_display.L_Normal = E_Normal;
			text_display.L_Slow = E_Slow;
			text_display.L_autoleveling = E_autoleveling;
			text_display.L_offset = E_offset;
			text_display.L_Continue = E_Continue;
			break;
		case German:
			text_display.L_Print = G_Print;
			text_display.L_Ctol = G_Ctol;
			text_display.L_Set = G_Set;
			text_display.L_Preheat = G_Preheat;
			text_display.L_Move = G_Move;
			text_display.L_Extrusion = G_Extrusion;
			text_display.L_Fan = G_Fan;
			text_display.L_About = G_About;
			text_display.L_Language = G_Language;
			text_display.L_Status = G_Status;
			text_display.L_PrintName = G_PrintName;
			text_display.L_Pause = G_Pause;
			text_display.L_Pursue = G_Pursue;
			text_display.L_Stop = G_Stop;
			text_display.L_Tempertuare = G_Tempertuare;
			text_display.L_Speed = G_Speed;
			text_display.L_PrintCtrl = G_PrintCtrl;
			text_display.L_PrintSpeed = G_PrintSpeed;
			text_display.L_SDPrint = G_SDPrint;
			text_display.L_Back = G_Back;
			text_display.L_PrintFinish = G_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = G_LaserCtol;
			text_display.L_LaserMove = G_LaserMove;
			text_display.L_Zero = G_Zero;
			text_display.L_Ctrl = G_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = G_Filament;
			text_display.L_StopPrint = G_StopPrint;
			text_display.L_Leveling = G_Leveling;
			text_display.L_Adjust = G_Adjust;
			text_display.L_Load = G_Load;
			text_display.L_Unload = G_Unload;
			text_display.L_Fast = G_Fast;
			text_display.L_Normal = G_Normal;
			text_display.L_Slow = G_Slow;
			text_display.L_autoleveling = G_autoleveling;
			text_display.L_offset = G_offset;
			text_display.L_Continue = G_Continue;
			break;
		case French:
			text_display.L_Print = F_Print;
			text_display.L_Ctol = F_Ctol;
			text_display.L_Set = F_Set;
			text_display.L_Preheat = F_Preheat;
			text_display.L_Move = F_Move;
			text_display.L_Extrusion = F_Extrusion;
			text_display.L_Fan = F_Fan;
			text_display.L_About = F_About;
			text_display.L_Language = F_Language;
			text_display.L_Status = F_Status;
			text_display.L_PrintName = F_PrintName;
			text_display.L_Pause = F_Pause;
			text_display.L_Pursue = F_Pursue;
			text_display.L_Stop = F_Stop;
			text_display.L_Tempertuare = F_Tempertuare;
			text_display.L_Speed = F_Speed;
			text_display.L_PrintCtrl = F_PrintCtrl;
			text_display.L_PrintSpeed = F_PrintSpeed;
			text_display.L_SDPrint = F_SDPrint;
			text_display.L_Back = F_Back;
			text_display.L_PrintFinish = F_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = F_LaserCtol;
			text_display.L_LaserMove = F_LaserMove;
			text_display.L_Zero = F_Zero;
			text_display.L_Ctrl = F_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = F_Filament;
			text_display.L_StopPrint = F_StopPrint;
			text_display.L_Leveling = F_Leveling;
			text_display.L_Adjust = F_Adjust;
			text_display.L_Load = F_Load;
			text_display.L_Unload = F_Unload;
			text_display.L_Fast = F_Fast;
			text_display.L_Normal = F_Normal;
			text_display.L_Slow = F_Slow;
			text_display.L_autoleveling = F_autoleveling;
			text_display.L_offset = F_offset;
			text_display.L_Continue = F_Continue;
			break;
		case Spanish:
			text_display.L_Print = S_Print;
			text_display.L_Ctol = S_Ctol;
			text_display.L_Set = S_Set;
			text_display.L_Preheat = S_Preheat;
			text_display.L_Move = S_Move;
			text_display.L_Extrusion = S_Extrusion;
			text_display.L_Fan = S_Fan;
			text_display.L_About = S_About;
			text_display.L_Language = S_Language;
			text_display.L_Status = S_Status;
			text_display.L_PrintName = S_PrintName;
			text_display.L_Pause = S_Pause;
			text_display.L_Pursue = S_Pursue;
			text_display.L_Stop = S_Stop;
			text_display.L_Tempertuare = S_Tempertuare;
			text_display.L_Speed = S_Speed;
			text_display.L_PrintCtrl = S_PrintCtrl;
			text_display.L_PrintSpeed = S_PrintSpeed;
			text_display.L_SDPrint = S_SDPrint;
			text_display.L_Back = S_Back;
			text_display.L_PrintFinish = S_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = S_LaserCtol;
			text_display.L_LaserMove = S_LaserMove;
			text_display.L_Zero = S_Zero;
			text_display.L_Ctrl = S_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = S_Filament;
			text_display.L_StopPrint = S_StopPrint;
			text_display.L_Leveling = S_Leveling;
			text_display.L_Adjust = S_Adjust;
			text_display.L_Load = S_Load;
			text_display.L_Unload = S_Unload;
			text_display.L_Fast = S_Fast;
			text_display.L_Normal = S_Normal;
			text_display.L_Slow = S_Slow;
			text_display.L_autoleveling = S_autoleveling;
			text_display.L_offset = S_offset;
			text_display.L_Continue = S_Continue;
			break;
		case Portuguese:
			text_display.L_Print = P_Print;
			text_display.L_Ctol = P_Ctol;
			text_display.L_Set = P_Set;
			text_display.L_Preheat = P_Preheat;
			text_display.L_Move = P_Move;
			text_display.L_Extrusion = P_Extrusion;
			text_display.L_Fan = P_Fan;
			text_display.L_About = P_About;
			text_display.L_Language = P_Language;
			text_display.L_Status = P_Status;
			text_display.L_PrintName = P_PrintName;
			text_display.L_Pause = P_Pause;
			text_display.L_Pursue = P_Pursue;
			text_display.L_Stop = P_Stop;
			text_display.L_Tempertuare = P_Tempertuare;
			text_display.L_Speed = P_Speed;
			text_display.L_PrintCtrl = P_PrintCtrl;
			text_display.L_PrintSpeed = P_PrintSpeed;
			text_display.L_SDPrint = P_SDPrint;
			text_display.L_Back = P_Back;
			text_display.L_PrintFinish = P_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = P_LaserCtol;
			text_display.L_LaserMove = P_LaserMove;
			text_display.L_Zero = P_Zero;
			text_display.L_Ctrl = P_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = P_Filament;
			text_display.L_StopPrint = P_StopPrint;
			text_display.L_Leveling = P_Leveling;
			text_display.L_Adjust = P_Adjust;
			text_display.L_Load = P_Load;
			text_display.L_Unload = P_Unload;
			text_display.L_Fast = P_Fast;
			text_display.L_Normal = P_Normal;
			text_display.L_Slow = P_Slow;
			text_display.L_autoleveling = P_autoleveling;
			text_display.L_offset = P_offset;
			text_display.L_Continue = P_Continue;
			break;
		case Japanese:
			text_display.L_Print = J_Print;
			text_display.L_Ctol = J_Ctol;
			text_display.L_Set = J_Set;
			text_display.L_Preheat = J_Preheat;
			text_display.L_Move = J_Move;
			text_display.L_Extrusion = J_Extrusion;
			text_display.L_Fan = J_Fan;
			text_display.L_About = J_About;
			text_display.L_Language = J_Language;
			text_display.L_Status = J_Status;
			text_display.L_PrintName = J_PrintName;
			text_display.L_Pause = J_Pause;
			text_display.L_Pursue = J_Pursue;
			text_display.L_Stop = J_Stop;
			text_display.L_Tempertuare = J_Tempertuare;
			text_display.L_Speed = J_Speed;
			text_display.L_PrintCtrl = J_PrintCtrl;
			text_display.L_PrintSpeed = J_PrintSpeed;
			text_display.L_SDPrint = J_SDPrint;
			text_display.L_Back = J_Back;
			text_display.L_PrintFinish = J_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = J_LaserCtol;
			text_display.L_LaserMove = J_LaserMove;
			text_display.L_Zero = J_Zero;
			text_display.L_Ctrl = J_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = J_Filament;
			text_display.L_StopPrint = J_StopPrint;
			text_display.L_Leveling = J_Leveling;
			text_display.L_Adjust = J_Adjust;
			text_display.L_Load = J_Load;
			text_display.L_Unload = J_Unload;
			text_display.L_Fast = J_Fast;
			text_display.L_Normal = J_Normal;
			text_display.L_Slow = J_Slow;
			text_display.L_autoleveling = J_autoleveling;
			text_display.L_offset = J_offset;
			text_display.L_Continue = J_Continue;
			break;
		case Russian:
			text_display.L_Print = R_Print;
			text_display.L_Ctol = R_Ctol;
			text_display.L_Set = R_Set;
			text_display.L_Preheat = R_Preheat;
			text_display.L_Move = R_Move;
			text_display.L_Extrusion = R_Extrusion;
			text_display.L_Fan = R_Fan;
			text_display.L_About = R_About;
			text_display.L_Language = R_Language;
			text_display.L_Status = R_Status;
			text_display.L_PrintName = R_PrintName;
			text_display.L_Pause = R_Pause;
			text_display.L_Pursue = R_Pursue;
			text_display.L_Stop = R_Stop;
			text_display.L_Tempertuare = R_Tempertuare;
			text_display.L_Speed = R_Speed;
			text_display.L_PrintCtrl = R_PrintCtrl;
			text_display.L_PrintSpeed = R_PrintSpeed;
			text_display.L_SDPrint = R_SDPrint;
			text_display.L_Back = R_Back;
			text_display.L_PrintFinish = R_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = R_LaserCtol;
			text_display.L_LaserMove = R_LaserMove;
			text_display.L_Zero = R_Zero;
			text_display.L_Ctrl = R_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = R_Filament;
			text_display.L_Leveling = E_Leveling;
			text_display.L_Fast = E_Fast;
			text_display.L_Normal = E_Normal;
			text_display.L_Slow = E_Slow;
			text_display.L_autoleveling = E_autoleveling;
			text_display.L_offset = E_offset;
			text_display.L_Continue = E_Continue;

			break;
		case Italian:
			text_display.L_Print = I_Print;
			text_display.L_Ctol = I_Ctol;
			text_display.L_Set = I_Set;
			text_display.L_Preheat = I_Preheat;
			text_display.L_Move = I_Move;
			text_display.L_Extrusion = I_Extrusion;
			text_display.L_Fan = I_Fan;
			text_display.L_About = I_About;
			text_display.L_Language = I_Language;
			text_display.L_Status = I_Status;
			text_display.L_PrintName = I_PrintName;
			text_display.L_Pause = I_Pause;
			text_display.L_Pursue = I_Pursue;
			text_display.L_Stop = I_Stop;
			text_display.L_Tempertuare = I_Tempertuare;
			text_display.L_Speed = I_Speed;
			text_display.L_PrintCtrl = I_PrintCtrl;
			text_display.L_PrintSpeed = I_PrintSpeed;
			text_display.L_SDPrint = I_SDPrint;
			text_display.L_Back = I_Back;
			text_display.L_PrintFinish = I_PrintFinish;
			text_display.L_Confirm = E_Confirm;
			text_display.L_LaserCtol = I_LaserCtol;
			text_display.L_LaserMove = I_LaserMove;
			text_display.L_Zero = I_Zero;
			text_display.L_Ctrl = I_Ctol;
			text_display.L_Cancel = E_Cancel;
			text_display.L_filament = I_Filament;
			text_display.L_StopPrint = I_StopPrint;
			text_display.L_Leveling = I_Leveling;
			text_display.L_Adjust = I_Adjust;
			text_display.L_Load = I_Load;
			text_display.L_Unload = I_Unload;
			text_display.L_Fast = I_Fast;
			text_display.L_Normal = I_Normal;
			text_display.L_Slow = I_Slow;
			text_display.L_autoleveling = I_autoleveling;
			text_display.L_offset = I_offset;
			text_display.L_Continue = I_Continue;
			break;
		default:
			break;
	}
}
void language_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 9;
	u16 pos[2] = {0};
	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		gui_phy.back_color = BACK_DIY;
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Language, 0);
		gui_draw_hline(0, 54, 320, WHITE);
		//display one buttun picture
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//create three button picture
		if(key_group)
		{
			key_group[0] = btn_creat(0, 0, 320, 54, 0, 1);

			key_group[1] = btn_creat(12, 72, 90, 121, 1, 1);
			key_group[2] = btn_creat(115, 72, 90, 121, 2, 1);
			key_group[3] = btn_creat(217, 72, 90, 121, 3, 1);
			key_group[4] = btn_creat(12, 206, 90, 121, 4, 1);
			key_group[5] = btn_creat(115, 206, 90, 121, 5, 1);
			key_group[6] = btn_creat(217, 206, 90, 121, 6, 1);
			key_group[7] = btn_creat(12, 341, 90, 121, 7, 1);
			key_group[8] = btn_creat(115, 341, 90, 121, 8, 1);

			key_group[1]->picbtnpathu = (u8 *)PTH_YY_001;
			key_group[1]->picbtnpathd = (u8 *)PTH_YY_001;
			key_group[2]->picbtnpathu = (u8 *)PTH_YY_002;
			key_group[2]->picbtnpathd = (u8 *)PTH_YY_002;
			key_group[3]->picbtnpathu = (u8 *)PTH_YY_003;
			key_group[3]->picbtnpathd = (u8 *)PTH_YY_003;
			key_group[4]->picbtnpathu = (u8 *)PTH_YY_004;
			key_group[4]->picbtnpathd = (u8 *)PTH_YY_004;
			key_group[5]->picbtnpathu = (u8 *)PTH_YY_005;
			key_group[5]->picbtnpathd = (u8 *)PTH_YY_005;
			key_group[6]->picbtnpathu = (u8 *)PTH_YY_006;
			key_group[6]->picbtnpathd = (u8 *)PTH_YY_006;
			key_group[7]->picbtnpathu = (u8 *)PTH_YY_007;
			key_group[7]->picbtnpathd = (u8 *)PTH_YY_007;
			key_group[8]->picbtnpathu = (u8 *)PTH_YY_008;
			key_group[8]->picbtnpathd = (u8 *)PTH_YY_008;
			for(i = 0; i < num; i++)
			{
				btn_draw(key_group[i]);
			}


		}
		//在选中的语言上打勾
		switch(text_display.language_choice)
		{
			case Chinese:
				pos[0] = 12 + 35;
				pos[1] = 72 + 90;
				break;
			case English:
				pos[0] = 115 + 35;
				pos[1] = 72 + 90;
				break;
			case German:
				pos[0] = 217 + 35;
				pos[1] = 72 + 90;
				break;
			case French:
				pos[0] = 12 + 35;
				pos[1] = 206 + 90;
				break;
			case Spanish:
				pos[0] = 115 + 35;
				pos[1] = 206 + 90;
				break;
			case Portuguese:
				pos[0] = 217 + 35;
				pos[1] = 206 + 90;
				break;
			case Japanese:
				pos[0] = 12 + 35;
				pos[1] = 341 + 90;
				break;
			case Italian:
				pos[0] = 115 + 35;
				pos[1] = 341 + 90;
				break;
			default:
				break;
		}
		minibmp_decode(PTH_YY_000, pos[0], pos[1], 27, 19, 0, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = TRUE;
				CurrentMenu = set_diy;
				break;
			case 1:
				text_display.language_choice = Chinese;
				break;
			case 2:
				text_display.language_choice = English;
				break;
			case 3:
				text_display.language_choice = German;
				break;
			case 4:
				text_display.language_choice = French;
				break;
			case 5:
				text_display.language_choice = Spanish;
				break;
			case 6:
				text_display.language_choice = Portuguese;
				break;
			case 7:
				text_display.language_choice = Japanese;
				break;
			case 8:
				text_display.language_choice = Italian;
				break;
			default:
				break;
		}
		set_display_language();
		gui_phy.back_color = BACK_DIY;
		//gui_fill_rectangle(255, 200, 23, 23, BACK2_DIY);
		//gui_fill_rectangle(100, 17, 120, 20, BACK_DIY);
		//gui_fill_rectangle(5, 17, 60, 16, BACK_DIY);
		gui_fill_rectangle(5, 17, 310, 16, BACK_DIY);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Language, 0);
		AT24CXX_Write(E_LANG, (u8 *)&text_display.language_choice, sizeof(text_display.language_choice));
		for(i = 0; i < num; i++)
		{
			btn_draw(key_group[i]);
		}
		//在选中的语言上打勾
		switch(text_display.language_choice)
		{
			case Chinese:
				pos[0] = 12 + 35;
				pos[1] = 72 + 90;
				break;
			case English:
				pos[0] = 115 + 35;
				pos[1] = 72 + 90;
				break;
			case German:
				pos[0] = 217 + 35;
				pos[1] = 72 + 90;
				break;
			case French:
				pos[0] = 12 + 35;
				pos[1] = 206 + 90;
				break;
			case Spanish:
				pos[0] = 115 + 35;
				pos[1] = 206 + 90;
				break;
			case Portuguese:
				pos[0] = 217 + 35;
				pos[1] = 206 + 90;
				break;
			case Japanese:
				pos[0] = 12 + 35;
				pos[1] = 341 + 90;
				break;
			case Italian:
				pos[0] = 115 + 35;
				pos[1] = 341 + 90;
				break;
			default:
				break;
		}
		minibmp_decode(PTH_YY_000, pos[0], pos[1], 27, 19, 0, 0);
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
//状态
void status_diy(void) // 7
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;

	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		//画背景色
		gui_phy.back_color = BACK_DIY;
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		gui_phy.back_color = BACK2_DIY;
		//画框图
		//minibmp_decode(PTH_ZT_003, 12, 76, 296, 376, 0, 0);
		gui_fill_rectangle(12, 76, 296, 376, BACK2_DIY);
		//显示坐标
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "X:%3.1f Y:%3.1f Z:%3.1f", current_position[0], current_position[1], current_position[2]);
		gui_show_ptstr(90, 164, 90 + 160, 164 + 16, 0, WHITE, 16, fatbuf, 0);
		if(PrinterMode == 0)
		{
			//显示图标
			minibmp_decode(PTH_ZT_000, 80, 209, 19, 33, 0, 0); //103
			minibmp_decode(PTH_ZT_001, 80, 280, 19, 31, 0, 0);
			minibmp_decode(PTH_ZT_002, 77, 354, 28, 26, 0, 0);


			//显示挤出头底板温度
			memset(fatbuf, 0, 100);
			sprintf(fatbuf, "%03d℃/%03d℃", show_extruder_tp, target_temperature[0]);
			gui_show_ptstr(120, 222, 120 + 100, 222 + 16, 0, WHITE, 16, fatbuf, 0);

			memset(fatbuf, 0, 100);
			sprintf(fatbuf, "%03d℃/%03d℃", (int)current_temperature[1], target_temperature[1]);
			gui_show_ptstr(120, 294, 120 + 100, 294 + 16, 0, WHITE, 16, fatbuf, 0);

			memset(fatbuf, 0, 100);
			sprintf(fatbuf, "%03d℃/%03d℃", (int)current_temperature_bed, target_temperature_bed);
			gui_show_ptstr(120, 360, 120 + 100, 360 + 16, 0, WHITE, 16, fatbuf, 0);
		}


		//显示6个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(0, 0, 320, 54, 5, 1);

		}
		gui_phy.back_color = BACK_DIY;
		gui_draw_hline(0, 54, 320, WHITE);
		gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
		gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Status, 0);
		//显示机器类型
		if(PrinterMode == 0)
		{
			//minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
			memset(fatbuf, 0, 50);
			if(text_display.language_choice == 0)
			{
				if(EXTRUDER_NUM == 2)
					sprintf(fatbuf, "当前模式：3D打印 双挤出");
				else
					sprintf(fatbuf, "当前模式：3D打印 单挤出");
			}
			else
			{
				if(EXTRUDER_NUM == 2)
					sprintf(fatbuf, "Current Mode：3D Print DoubleExtruder");
				else
					sprintf(fatbuf, "Current Mode：3D Print SingleExtruder");
			}
			gui_show_strmid(0, 120, 320, 16, WHITE, 16, fatbuf, 1);
		}
		else if(PrinterMode == 1)
		{
			//minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
			if(text_display.language_choice == 0)
			{
				gui_show_ptstr(82, 120, 180 + 140, 140, 0, WHITE, 16, "当前模式：激光雕刻", 1);
			}
			else
			{
				gui_show_ptstr(82, 120, 180 + 140, 140, 0, WHITE, 16, "Current Mode：Laser Carve", 1);
			}

		}
	}
	//刷新温度显示
	if(display_next_update_millis < millis() && (PrinterMode == 0))
	{
		display_next_update_millis = millis() + 500;
		gui_phy.back_color = BACK2_DIY;
		//显示坐标
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "X:%3.1f Y:%3.1f Z:%3.1f", current_position[0], current_position[1], current_position[2]);
		gui_show_ptstr(90, 164, 90 + 160, 164 + 16, 0, WHITE, 16, fatbuf, 0);
		//显示挤出头底板温度
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%03d℃/%03d℃", show_extruder_tp, target_temperature[0]);
		gui_show_ptstr(120, 222, 120 + 100, 222 + 16, 0, WHITE, 16, fatbuf, 0);

		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%03d℃/%03d℃", (int)current_temperature[1], target_temperature[1]);
		gui_show_ptstr(120, 294, 120 + 100, 294 + 16, 0, WHITE, 16, fatbuf, 0);

		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%03d℃/%03d℃", (int)current_temperature_bed, target_temperature_bed);
		gui_show_ptstr(120, 360, 120 + 100, 360 + 16, 0, WHITE, 16, fatbuf, 0);

	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				redraw_menu = TRUE;
				CurrentMenu = set_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
//wifi
void wifi_diy(void)
{

}

void progressbar_diy(u16 sx, u16 sy, u8 progress)
{
	int lenth;
	u8 str[5];
	if(progress > 100)
		progress = 100;
	//绘制已完成的矩形框
	lenth = 2 * progress; // 110/100*progress
	gui_fill_rectangle(sx, sy, lenth, 20, GREEN);
	//绘制剩余长度
	gui_fill_rectangle(sx + lenth, sy, 200 - lenth, 20, BACK_DIY);
	//显示百分比
	sprintf(str, "%d%%", progress);
	gui_show_strmid(sx + 200, sy + 2, 32, 16, WHITE, 16, str, 0);
}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
u8 old_print_stat = 0;
u8 old_print_pross = 0;	//历史打印进度
u8 display_switch = 0;	//Step by step display 
void laser_print_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 2;

	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu)
	{
		redraw_menu = false;
		gui_phy.back_color = BACK_DIY;
		//画背景色
		//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
		LCD_Clear(gui_phy.back_color);
		gui_phy.back_color = BACK2_DIY;
		//显示缩略图
		//minibmp_decode(PTH_DY_011, 17, 17, 286, 147, 0, 0); //背景
		gui_fill_rectangle(17, 17, 286, 147, BACK2_DIY);
		//minibmp_decode(PTH_DY_016, 17, 170, 286, 117, 0, 0); //背景
		gui_fill_rectangle(17, 170, 286, 117, BACK2_DIY);
		minibmp_decode(PTH_DY_012, 109, 30, 122, 122, 0, 0);



		//显示打印进度
		progressbar_diy(40, 182, PrintInfo.printper);
		//显示状态信息
		minibmp_decode(PTH_DY_001, 100, 210, 31, 31, 0, 0);
		old_print_stat = card.sdprinting;
		//打印时长
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%dh%02dm", PrintInfo.printtime / 3600, PrintInfo.printtime / 60 % 60);
		gui_show_ptstr(150, 218, 150 + 80, 218 + 16, 0, WHITE, 16, fatbuf, 0);
		//打印文件名
		gui_show_strmid(0, 254, 320, 16, WHITE, 16, PrintInfo.printfile, 0);
		//显示3个按钮图标
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(17, 296, 286, 80, 5, 1);
			if(card.sdprinting == TRUE)
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_006;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_006;
			}
			else
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_007;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_007;
			}

			key_group[1] = btn_creat(17, 384, 286, 80, 5, 1);
			key_group[1]->picbtnpathu = (u8 *)PTH_DY_008;
			key_group[1]->picbtnpathd = (u8 *)PTH_DY_008;

			for(i = 0; i < num; i++)
			{
				btn_draw(key_group[i]);
			}
		}
		if(card.sdprinting == TRUE)
			gui_show_ptstr(160, 329, 160 + 80, 329 + 16, 0, WHITE, 16, text_display.L_Pause, 0);
		else
			gui_show_ptstr(160, 329, 160 + 80, 329 + 16, 0, WHITE, 16, text_display.L_Pursue, 0);
		gui_show_ptstr(160, 415, 160 + 80, 415 + 16, 0, WHITE, 16, text_display.L_Stop, 0);

	}
	//刷新温度显示
	if(display_next_update_millis < millis())
	{
		display_next_update_millis = millis() + 2000;

		//暂停 继续图标刷新
		if(old_print_stat != card.sdprinting)
		{
			if(card.sdprinting == TRUE)
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_006;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_006;
			}
			else
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_007;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_007;
			}
			old_print_stat = card.sdprinting;
			btn_draw(key_group[0]);
			if(card.sdprinting == TRUE)
				gui_show_strmid(160, 329, 80, 16, WHITE, 16, text_display.L_Pause, 0);
			else
				gui_show_strmid(160, 329, 80, 16, WHITE, 16, text_display.L_Pursue, 0);
		}
		progressbar_diy(40, 182, PrintInfo.printper);

		//打印时长
		memset(fatbuf, 0, 100);
		sprintf(fatbuf, "%dh%02dm", (usr_timer.date * 24 + usr_timer.hour), usr_timer.min);
		gui_show_ptstr(150, 218, 150 + 80, 218 + 16, 0, WHITE, 16, fatbuf, 0);
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				if(card.sdprinting == TRUE)
				{
					card.sdprinting = FALSE;
					PrintInfo.printsd = 2;		//打印暂停
					enquecommand("M5");			//关闭激光头
				}
				else
				{
					card.sdprinting = TRUE;
					PrintInfo.printsd = 1;		//正在打印状态
					enquecommand("M3");			//打开激光头
				}
				break;
			case 1:
				lastMenu = CurrentMenu;
				if(text_display.language_choice == 0)
					PrintPoint = TEXT_J;
				else
					PrintPoint = TEXT_M;
				CurrentMenu = popout_screen;
				tempMenu = manual_stop_diy;
				redraw_menu = true;

				break;
			default:
				break;
		}
	}

	if(PrintInfo.printsd == 0 && card.sdprinting == 0)	//打印完成
	{
		progressbar_diy(40, 182, 100);
		CurrentMenu = stop_laser_diy;
		redraw_menu = true;
		PrintInfo.printsd = 0;
	}

	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
//*********add model preview by wg************
int StringToHex(char *str, unsigned char *out, unsigned int *outlen)
{
	char *p = str;
	char high = 0, low = 0;
	int tmplen = strlen(p), cnt = 0;
	tmplen = strlen(p);
	while(cnt < (tmplen / 2))
	{
		high = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
		low = (*(++ p) > '9' && ((*p <= 'F') || (*p <= 'f'))) ? *(p) - 48 - 7 : *(p) - 48;
		out[cnt] = ((high & 0x0f) << 4 | (low & 0x0f));
		p ++;
		cnt ++;
	}
	if(tmplen % 2 != 0)
		out[cnt] = ((*p > '9') && ((*p <= 'F') || (*p <= 'f'))) ? *p - 48 - 7 : *p - 48;
	if(outlen != NULL)
		*outlen = tmplen / 2 + tmplen % 2;
	return tmplen / 2 + tmplen % 2;
}
u8 *hexbuf;
unsigned int length = 0;
//********************end add**************

uint8_t last_bedtem = 0, last_min = 0;							//used to refresh temperature
uint16_t last_temp1 = 0, last_temp2 = 0;
float last_zlength = 0;														//used to refresh printing high
u8 filament_flg = 0;		//缺料标志位
void print_diy(void)
{
	u8 selx = 0XFF;
	u8 res = 0, i, j;
	u8 num = 3;
	u16 rgb_val = 0;
	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu)
	{
		switch(display_redraw)
		{
			case 0:
				gui_phy.back_color = BACK_DIY;
				//画背景色
				LCD_Clear(gui_phy.back_color);
				gui_show_strmid(0, 4, 320, 16, WHITE, 16, PrintInfo.printfile, 0);
				break;
			case 1:
				if((pictr_flag == true) || (bmp_flag == true))
				{
					pictr_flag = false;
					res = f_open(ftemp, "0:pictr.bmp", FA_READ);
					memset(fatbuf, 0, sizeof(fatbuf));
					if(pic_size == 100)	//像素为100*100图片
					{
						hexbuf = (u8 *)mymalloc(SRAMIN, pic_size * 2);
						for(i = 0; i < pic_size; i++)	//读完所有数据
						{
							res = f_read(ftemp, fatbuf, pic_size * 4, (UINT *)&br);
							StringToHex(fatbuf, hexbuf, (unsigned int *)&length);
							for(j = 0; j < pic_size; j++)	//显示中间138个像素
							{
								rgb_val = (u16)(hexbuf[j * 2]) << 8;
								rgb_val |= (u16)(hexbuf[j * 2 + 1]);
								_gui_draw_point(j + 185, i + 135, rgb_val);
							}
						}
					}
					else if(pic_size == 200)	//size 200*200
					{
						hexbuf = (u8 *)mymalloc(SRAMIN, pic_size * 2);
						for(i = 0; i < pic_size; i++)	//读完所有数据
						{
							res = f_read(ftemp, fatbuf, pic_size * 4, (UINT *)&br);
							StringToHex(fatbuf, hexbuf, (unsigned int *)&length);
							for(j = 0; j < pic_size; j++)	//显示中间138个像素
							{
								rgb_val = (u16)(hexbuf[j * 2 + 1]) << 8;
								rgb_val |= (u16)(hexbuf[j * 2]);
								if((j > 15) && (j <= 185))
									_gui_draw_point(j + 135, i + 90, rgb_val);
							}
						}
					}

					myfree(SRAMIN, hexbuf);
					f_close(ftemp);
				}
				else
					minibmp_decode(PTH_DY_000, 185, 135, 112, 114, 0, 0);	//显示缩略图
				//显示矩形框
				gui_fill_rectangle(22, 90, 128, 264 + 5, BACK2_DIY);
				break;
			case 2:
				//显示打印进度
				gui_fill_rectangle(22, 45, 276, 30, BACK2_DIY);
				progressbar_diy(37, 50, PrintInfo.printper);
				break;
			case 3:
				//显示状态信息
				minibmp_decode(PTH_DY_001, 30, 103, 31, 31, 0, 0);
				minibmp_decode(PTH_DY_003, 30, 148, 31, 31, 0, 0);
				minibmp_decode(PTH_DY_004, 30, 193, 31, 31, 0, 0);
				minibmp_decode(PTH_DY_002, 30, 238, 31, 31, 0, 0);
				minibmp_decode(PTH_DY_005, 30, 283, 31, 31, 0, 0);
				break;
			case 4:
				gui_phy.back_color = BACK2_DIY;
				old_print_stat = card.sdprinting;
				old_print_pross = 0;
				display_switch = 0;
				//打印时长
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%dh%02dm", (usr_timer.date * 24 + usr_timer.hour), usr_timer.min);
				gui_show_ptstr(70, 110, 70 + 64, 110 + 16, 0, WHITE, 16, fatbuf, 0);
				//显示挤出头1温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", show_extruder_tp);
				gui_show_ptstr(70, 245, 70 + 40, 245 + 16, 0, WHITE, 16, fatbuf, 0);
				//底板温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature_bed);
				gui_show_ptstr(70, 155, 70 + 40, 155 + 16, 0, WHITE, 16, fatbuf, 0);
				//挤出头2温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature[1]);
				gui_show_ptstr(70, 200, 70 + 40, 200 + 16, 0, WHITE, 16, fatbuf, 0);
				//打印速度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d%%", (int)feedmultiply);
				gui_show_ptstr(70, 290, 70 + 40, 290 + 16, 0, WHITE, 16, fatbuf, 0);
				//打印Z轴高度
				gui_fill_rectangle(38, 335, 100, 16, BACK2_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "Z:  %.2fmm", current_position[Z_AXIS]);
				gui_show_ptstr(38, 335, 38 + 100, 335 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
			case 5:
				//显示3个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
				if(key_group)
				{
					key_group[0] = btn_creat(22, 372, 128, 68, 5, 1);
					if(card.sdprinting == TRUE)
					{
						key_group[0]->picbtnpathu = (u8 *)PTH_DY_106;
						key_group[0]->picbtnpathd = (u8 *)PTH_DY_106;
					}
					else
					{
						key_group[0]->picbtnpathu = (u8 *)PTH_DY_107;
						key_group[0]->picbtnpathd = (u8 *)PTH_DY_107;
					}
					key_group[1] = btn_creat(170, 372, 128, 68, 5, 1);
					key_group[1]->picbtnpathu = (u8 *)PTH_DY_108;
					key_group[1]->picbtnpathd = (u8 *)PTH_DY_108;
					key_group[2] = btn_creat(170, 292, 128, 68, 5, 1);
					key_group[2]->picbtnpathu = (u8 *)PTH_DY_009;
					key_group[2]->picbtnpathd = (u8 *)PTH_DY_009;
					for(i = 0; i < num; i++)
					{
						btn_draw(key_group[i]);
					}
					if(card.sdprinting == TRUE)
						gui_show_ptstr(84, 396, 84 + 40, 396 + 16, 0, WHITE, 16, text_display.L_Pause, 0);
					else
						gui_show_ptstr(84, 396, 84 + 48, 396 + 16, 0, WHITE, 16, text_display.L_Pursue, 0);

					gui_show_ptstr(235, 396, 235 + 40, 396 + 16, 0, WHITE, 16, text_display.L_Stop, 0);
					gui_show_ptstr(235, 316, 235 + 40, 316 + 16, 0, WHITE, 16, text_display.L_Ctrl, 0);
				}
				break;
			default:
				break;
		}
		if(display_redraw < 5)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
			if(reprint.break_point_flg == TRUE)	//断点续打
			{
				reprint.break_point_flg = FALSE;

				//设置目标温度
				memset(fatbuf, 0, 60);
				sprintf(fatbuf, "M140 S%.1f", (float)reprint.temperature_bed);	//底板温度不等待
				enquecommand(fatbuf);

				memset(fatbuf, 0, 60);
				sprintf(fatbuf, "M109 S%.1f", (float)reprint.temperature_extruder[0]);	//挤出头温度加热等待
				enquecommand(fatbuf);
				memset(fatbuf, 0, 60);
				sprintf(fatbuf, "T%d", reprint.active_extruder);
				enquecommand(fatbuf);
				current_position[3] = reprint.e_axis;	//给e轴赋予当前位置坐标
				position[3] =  reprint.e_axis * axis_steps_per_unit[E_AXIS];	//E轴当前步进值

				current_position[Z_AXIS] = reprint.z_axis+10;	//给z轴赋予当前位置坐标
				position[Z_AXIS] =  (reprint.z_axis+10) * axis_steps_per_unit[Z_AXIS];
				
				enquecommand("G28 XY");			//挤出头到达目标温度后xy归零

				memset(fatbuf, 0, 60);
				sprintf(fatbuf, "G0 X%f Y%f F1600", (float)reprint.x_axis,(float)reprint.y_axis);	
				enquecommand(fatbuf);

				memset(fatbuf, 0, 60);
				sprintf(fatbuf, "G0 Z%f F800", (float)reprint.z_axis);	//z轴回到断电续打位置
				enquecommand(fatbuf);
				
				enquecommand("M106");	//打开风扇
				card.sdprinting = TRUE;	//开始打印
			}
		}

	}
	//刷新温度显示
	if(display_next_update_millis < millis())
	{
		display_next_update_millis = millis() + 200;						//every 200ms refresh pringing data
		filament_stat();	//断料检测
		//断料检测
		if(PrintInfo.filament && filament_flg == 0)	//检测到断料
		{
			card.sdprinting = FALSE;
			PrintInfo.printsd = PRINT_PAUSE;		//打印暂停
			redraw_menu = TRUE;
			CurrentMenu = filament_run_out_diy;
			if(BeepSwitch)
				BeepFlg = TRUE;
		}
		//暂停 继续图标刷新
		if(old_print_stat != card.sdprinting)
		{
			if(card.sdprinting == TRUE)
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_106;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_106;
			}
			else
			{
				key_group[0]->picbtnpathu = (u8 *)PTH_DY_107;
				key_group[0]->picbtnpathd = (u8 *)PTH_DY_107;
				enquecommand("G91");
				enquecommand("G0 Z5 E-5 F1200");
				enquecommand("G90");
				enquecommand("M114");		//获取当前xy坐标值
				enquecommand("G0 X5 Y5 F1200");
			}
			old_print_stat = card.sdprinting;
			btn_draw(key_group[0]);

			if(card.sdprinting == TRUE)
				gui_show_ptstr(84, 396, 84 + 40, 396 + 16, 0, WHITE, 16, text_display.L_Pause, 0);
			else
				gui_show_ptstr(84, 396, 84 + 48, 396 + 16, 0, WHITE, 16, text_display.L_Pursue, 0);

		}
		if(PrintInfo.printper > old_print_pross)
		{
			progressbar_diy(37, 50, PrintInfo.printper);
			old_print_pross = PrintInfo.printper;
		}
		if(PrintInfo.printsd == 2)	//打印暂停
		{
			enable_x();
			enable_y();
		}
		switch(display_switch)
		{
			case 0:
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%dh%02dm", (usr_timer.date * 24 + usr_timer.hour), usr_timer.min);
				if(last_min != usr_timer.min)
				{
					last_min = usr_timer.min;
					gui_show_ptstr(70, 110, 70 + 64, 110 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 1:
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", show_extruder_tp);
				if(last_temp1 != show_extruder_tp)
				{
					last_temp1 = show_extruder_tp;
					gui_show_ptstr(70, 245, 70 + 40, 245 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 2:
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)(current_temperature_bed + 0.5));
				if(last_bedtem != (int)(current_temperature_bed + 0.5))
				{
					last_bedtem = (int)(current_temperature_bed + 0.5);
					gui_show_ptstr(70, 155, 70 + 40, 155 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 3:
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature[1]);
				if(last_temp2 != (int)current_temperature[1])
				{
					last_temp2 = (int)current_temperature[1];
					gui_show_ptstr(70, 200, 70 + 40, 200 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 4:
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "Z:  %.2fmm", current_position[Z_AXIS]);
				if(last_zlength != current_position[Z_AXIS])
				{
					gui_fill_rectangle(38, 335, 100, 16, BACK2_DIY);
					last_zlength = current_position[Z_AXIS];
					gui_show_ptstr(38, 335, 38 + 100, 335 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 5:
				//打印速度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d%%", (int)feedmultiply);
				gui_show_ptstr(70, 290, 70 + 40, 290 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
		}
		display_switch++;
		display_switch = display_switch % 6;
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				if(card.sdprinting == TRUE)
				{
					card.sdprinting = FALSE;
					PrintInfo.printsd = 2;		//打印暂停
				}
				else
				{
					//续打处理代码
					if(reprint.reprint_flg == TRUE)	//续打标志
					{

						//设置目标温度
						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "M140 S%.1f", (float)reprint.temperature_bed);	//底板温度不等待
						enquecommand(fatbuf);

						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "M109 S%.1f", (float)reprint.temperature_extruder[0]);	//挤出头温度加热等待
						enquecommand(fatbuf);
						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "T%d", reprint.active_extruder);
						enquecommand(fatbuf);
						current_position[3] = reprint.e_axis;	//给e轴赋予当前位置坐标
						position[3] =  reprint.e_axis * axis_steps_per_unit[E_AXIS];	//E轴当前步进值

						current_position[Z_AXIS] = reprint.z_axis;	//给z轴赋予当前位置坐标
						position[Z_AXIS] =  reprint.z_axis * axis_steps_per_unit[Z_AXIS];

						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "G0 Z%f F800", (float)reprint.z_axis + 5);	//z轴回到断电续打位置
						enquecommand(fatbuf);
						printf("aaaaaaaa%s",fatbuf);
						enquecommand("G28 XY");			//挤出头到达目标温度后xyz归零

						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "G0 X%f Y%f F1200", (float)reprint.x_axis,(float)reprint.y_axis);	//xy归零
						enquecommand(fatbuf);
						
						memset(fatbuf, 0, 60);
						sprintf(fatbuf, "G0 Z%f F800", (float)reprint.z_axis);	//z轴回到断电续打位置
						enquecommand(fatbuf);
						printf("aaaaaaaa%s",fatbuf);
			
						enquecommand("M106");	//打开风扇

						reprint.reprint_flg = FALSE;
					}
					else
					{
						memset(fatbuf, 0, 50);
						sprintf(fatbuf, "G0 X%f Y%f F800", TempPosition[0], TempPosition[1]);
						enquecommand(fatbuf);					//XY轴回到模型位置
						enquecommand("G91");
						enquecommand("G0 Z-5 E5 F800");			//Z轴回到模型位置
						enquecommand("G90");
					}
					card.sdprinting = TRUE;
					PrintInfo.printsd = 1;		//正在打印状态
					filament_flg = 0;			//清除断料标志

				}
				break;
			case 1:
				lastMenu = CurrentMenu;
				PrintPoint = text_display.L_StopPrint;
				CurrentMenu = popout_screen;
				tempMenu = manual_stop_diy;
				redraw_menu = TRUE;
				gui_show_ptstr(235, 396, 235 + 40, 396 + 16, 0, WHITE, 16, text_display.L_Stop, 0);
				break;
			case 2:
				redraw_menu = TRUE;
				lastMenu = CurrentMenu;
				CurrentMenu = print_contrl_diy;
				break;

			default:
				break;
		}
	}

	if(PrintInfo.printsd == 0 && card.sdprinting == 0)	//打印完成
	{
		bmp_flag = FALSE;
		CurrentMenu = stop_diy;	//跳转到主界面
		redraw_menu = TRUE;
		PrintInfo.printsd = 0;
	}

	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void print_contrl_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 5;

	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu)
	{

		switch(display_redraw)
		{
			case 0:
				gui_phy.back_color = BACK_DIY;
				//画背景色
				//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
				LCD_Clear(gui_phy.back_color);
				gui_phy.back_color = BACK2_DIY;
				break;
			case 1:
				//显示信息背景图
				//minibmp_decode(PTH_DYKZ_003, 12, 63, 296, 265, 0, 0);
				gui_fill_rectangle(12, 63, 296, 265, BACK2_DIY);
				break;
			case 2:
				//显示状态信息
				//minibmp_decode(PTH_DYKZ_004,19,54,26,26,0,0);
				minibmp_decode(PTH_DYKZ_005, 35, 196, 23, 39, 0, 0);
				minibmp_decode(PTH_DYKZ_006, 205, 128, 23, 39, 0, 0);
				minibmp_decode(PTH_DYKZ_007, 203, 203, 29, 26, 0, 0);
				minibmp_decode(PTH_DYKZ_008, 32, 266, 32, 35, 0, 0);
				minibmp_decode(PTH_DYKZ_009, 205, 266, 32, 35, 0, 0);
				minibmp_decode(PTH_DYKZ_011, 33, 127, 21, 39, 0, 0);
				#ifdef FAN3_SHOW
				//minibmp_decode(PTH_DYKZ_010,100,179,26,26,0,0);
				#endif
				break;
			case 3:
				display_switch = 0;
				//速度

				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%d%%  ", text_display.L_PrintSpeed, (int)feedmultiply);
				gui_show_ptstr(103, 78, 103 + 72 + 40, 78 + 16, 0, WHITE, 16, fatbuf, 0);
				//挤出头1温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", show_extruder_tp);
				gui_show_ptstr(70, 213, 70 + 40, 213 + 16, 0, WHITE, 16, fatbuf, 0);
				//挤出头2温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature[1]);
				gui_show_ptstr(247, 148, 247 + 40, 148 + 16, 0, WHITE, 16, fatbuf, 0);
				//底板
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature_bed);
				gui_show_ptstr(247, 213, 247 + 40, 213 + 16, 0, WHITE, 16, fatbuf, 0);
				//风扇1
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed);
				gui_show_ptstr(70, 278, 70 + 40, 278 + 16, 0, WHITE, 16, fatbuf, 0);
				//风扇2
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed1);
				gui_show_ptstr(247, 278, 247 + 40, 278 + 16, 0, WHITE, 16, fatbuf, 0);

				//挤出比
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", extrudemultiply);
				gui_show_ptstr(70, 148, 70 + 40, 148 + 16, 0, WHITE, 16, fatbuf, 0);
				#ifdef FAN3_SHOW
				//风扇3
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed2);
				gui_show_ptstr(128, 179 + 5, 128 + 40, 184 + 16, 0, WHITE, 16, fatbuf, 0);
				#endif

				break;
			case 4:
				//显示5个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
				if(key_group)
				{
					key_group[0] = btn_creat(12, 338, 142, 57, 0, 1);
					key_group[0]->picbtnpathu = (u8 *)PTH_DYKZ_001;
					key_group[0]->picbtnpathd = (u8 *)PTH_DYKZ_001;
					key_group[1] = btn_creat(166, 338, 142, 57, 1, 1);
					key_group[1]->picbtnpathu = (u8 *)PTH_DYKZ_000;
					key_group[1]->picbtnpathd = (u8 *)PTH_DYKZ_000;
					key_group[2] = btn_creat(12, 402, 142, 57, 2, 1); //190
					key_group[2]->picbtnpathu = (u8 *)PTH_DYKZ_002;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYKZ_002;
					key_group[3] = btn_creat(166, 402, 142, 57, 2, 1); //190
					key_group[3]->picbtnpathu = (u8 *)PTH_DYKZ_012;
					key_group[3]->picbtnpathd = (u8 *)PTH_DYKZ_012;
					key_group[4] = btn_creat(0, 0, 320, 54, 5, 1);

					for(i = 0; i < num - 1; i++)
					{
						btn_draw(key_group[i]);
					}
				}
				break;
			case 5:
				gui_phy.back_color = BACK2_DIY;
				gui_show_strmid(220, 360, 80, 16, WHITE, 16, text_display.L_Fan, 0);
				gui_show_strmid(65, 360, 90, 16, WHITE, 16, text_display.L_Tempertuare, 0);
				gui_show_strmid(70, 420, 80, 16, WHITE, 16, text_display.L_Speed, 0);
				gui_show_strmid(225, 420, 80, 16, WHITE, 16, text_display.L_Extrusion, 0);
				break;
			case 6:

				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
				gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_PrintCtrl, 0);
				//显示机器类型
				//				if(PrinterMode == 0)
				//				{
				//					minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
				//				}
				//				else if(PrinterMode == 1)
				//				{
				//					minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
				//				}
				break;
			default:
				break;
		}
		if(display_redraw < 6)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
		}

	}
	if(PrintInfo.printsd == 0 || PrintInfo.filament) //如果打印完成或者断料都跳到打印界面做处理
	{
		redraw_menu = TRUE;
		CurrentMenu = print_diy;
		CheckPrintFlg = FALSE;
	}
	//刷新温度显示
	if(display_next_update_millis < millis())
	{
		display_next_update_millis = millis() + 2000;
		switch(display_switch)
		{
			case 0:
				//速度
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%s:%d%%  ", text_display.L_PrintSpeed, (int)feedmultiply);
				gui_show_ptstr(103, 78, 103 + 72 + 40, 78 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
			case 1:
				//挤出头1温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", show_extruder_tp);
				if(last_temp1 != show_extruder_tp)
				{
					last_temp1 = show_extruder_tp;
					gui_show_ptstr(70, 213, 70 + 40, 213 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 2:
				//挤出头2温度
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature[1]);
				if(last_temp2 != (int)current_temperature[1])
				{
					last_temp2 = (int)current_temperature[1];
					gui_show_ptstr(247, 148, 247 + 40, 148 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 3:
				//底板
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%03d℃", (int)current_temperature_bed);
				if(last_bedtem != (int)current_temperature_bed)
				{
					last_bedtem = (int)current_temperature_bed;
					gui_show_ptstr(247, 213, 247 + 40, 213 + 16, 0, WHITE, 16, fatbuf, 0);
				}
				break;
			case 4:
				//风扇1
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed);
				gui_show_ptstr(70, 278, 70 + 40, 278 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
			case 5:
				//风扇2
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed1);
				gui_show_ptstr(247, 278, 247 + 40, 278 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
				#ifdef FAN3_SHOW
			case 6:
				//风扇3
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d  ", fanSpeed2);
				gui_show_ptstr(128, 179 + 5, 128 + 40, 184 + 16, 0, WHITE, 16, fatbuf, 0);
				break;
				#endif
		}
		display_switch++;
		display_switch = display_switch % 7;
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{

			case 0:
				lastMenu = CurrentMenu;
				CurrentMenu = preheat_diy;
				redraw_menu = TRUE;
				break;
			case 1:
				lastMenu = CurrentMenu;
				CurrentMenu = fan_diy;
				redraw_menu = TRUE;
				break;
			case 2:
				lastMenu = CurrentMenu;
				CurrentMenu = speed_diy;
				redraw_menu = TRUE;
				break;
			case 3:
				lastMenu = CurrentMenu;
				CurrentMenu = extrudemultiply_diy;
				redraw_menu = TRUE;
				break;
			case 4:
				redraw_menu = TRUE;
				CurrentMenu = print_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:打印速度设置
*/
static u8 speed_factor_diy = 50;		// 10 50 100
void speed_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 5;
	//	float copy_temperature;
	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		switch(display_redraw)
		{
			case 0:
				//画背景色
				//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
				LCD_Clear(gui_phy.back_color);
				break;
			case 1:
				//显示速度小图标
				//minibmp_decode(PTH_DYSD_000, 24, 88, 274, 74, 0, 0);
				gui_fill_rectangle(24, 88, 274, 74, BACK2_DIY);
				//速度等级背景
				// minibmp_decode(PTY_DY_BG,16,143,127,69,0,0);
				//显示当前打印速度值
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", feedmultiply);
				gui_show_strmid(136, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				//显示5个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
				if(key_group)
				{
					key_group[0] = btn_creat(24, 176, 130, 122, 0, 1);
					key_group[0]->picbtnpathu = (u8 *)PTH_YR_000;
					key_group[0]->picbtnpathd = (u8 *)PTH_YR_000;
					key_group[1] = btn_creat(167, 176, 130, 122, 1, 1);
					key_group[1]->picbtnpathu = (u8 *)PTH_YR_001;
					key_group[1]->picbtnpathd = (u8 *)PTH_YR_001;

					key_group[2] = btn_creat(24, 314, 130, 122, 2, 1);

					if(speed_factor_diy == 10)
					{
						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_001;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_001;

					}
					else if(speed_factor_diy == 50)
					{
						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_002;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_002;

					}
					else if(speed_factor_diy == 100)
					{

						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_003;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_003;

					}

					key_group[3] = btn_creat(167, 314, 130, 122, 3, 1);
					key_group[3]->picbtnpathu = (u8 *)PTH_DYSD_004;
					key_group[3]->picbtnpathd = (u8 *)PTH_DYSD_004;

					key_group[4] = btn_creat(0, 0, 320, 54, 5, 1);
					//					key_group[4]->picbtnpathu = (u8 *)PTH_FH;
					//					key_group[4]->picbtnpathd = (u8 *)PTH_FH;

					for(i = 0; i < num - 1; i++)
					{
						btn_draw(key_group[i]);
					}
					//显示机器类型
					//					if(PrinterMode == 0)
					//					{
					//						minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
					//					}
					//					else if(PrinterMode == 1)
					//					{
					//						minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
					//					}
				}
				break;
			case 3:
				//显示机器类型
				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
				gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Speed, 0);
				/*
				if(PrinterMode == 0)
				{
					minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
				}
				else if(PrinterMode == 1)
				{
					minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
				}
				*/
				break;
			default:
				break;
		}
		if(display_redraw < 3)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
		}


	}
	if(PrintInfo.printsd == 0 || PrintInfo.filament) //如果打印完成或者断料都跳到打印界面做处理
	{
		redraw_menu = TRUE;
		CurrentMenu = print_diy;
		CheckPrintFlg = FALSE;
	}
	//按键检测
	//	float speed_temp;
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:

				feedmultiply -= speed_factor_diy;
				if(feedmultiply < speed_factor_diy)
				{
					feedmultiply = speed_factor_diy;
				}
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", feedmultiply);
				gui_show_strmid(136, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 1:				//倍数限制,最高到500%
				if(feedmultiply <= (DEFAULT_MAX_FEEDMULTIPLY - speed_factor_diy))
				{
					feedmultiply += speed_factor_diy;
				}
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", feedmultiply);
				gui_show_strmid(136, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:

				if(speed_factor_diy == 10)
				{
					speed_factor_diy = 50;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_002;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_002;

				}
				else if(speed_factor_diy == 50)
				{
					speed_factor_diy = 100;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_003;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_003;

				}
				else if(speed_factor_diy == 100)
				{
					speed_factor_diy = 10;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_001;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_001;

				}
				btn_draw(key_group[2]);
				break;
			case 3:
				feedmultiply = DEFAULT_FEEDMULTIPLY;						//RETURN TO MIN FEEDMULTIPLY
				//gui_fill_rectangle(136, 116, 40, 16, BACK_DIY);
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", feedmultiply);
				gui_show_strmid(136, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 4:
				redraw_menu = TRUE;
				CurrentMenu = print_contrl_diy;
				break;
			default:
				break;
		}
	}
	if(speed_temprature_factor > EPSINON)
	{
		target_temperature[0] = 185 * speed_temprature_factor * (feedmultiply - 100) / 100 + 185;
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:挤出机倍率设置
*/
static u8 extrudemulti_factor_diy = 50;		// 10 50 100

void extrudemultiply_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 5;
	//	float copy_temperature;
	gui_phy.back_color = BACK_DIY;
	if(redraw_menu)
	{
		switch(display_redraw)
		{
			case 0:
				//画背景色
				//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
				LCD_Clear(gui_phy.back_color);
				break;
			case 1:
				//显示速度小图标
				//minibmp_decode(PTH_DYSD_000, 24, 88, 274, 74, 0, 0);
				gui_fill_rectangle(24, 88, 274, 74, BACK2_DIY);
				//速度等级背景
				//        minibmp_decode(PTY_DY_BG,16,143,127,69,0,0);
				//显示当前打印速度值
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", extrudemultiply);
				gui_show_strmid(145, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				//显示5个按钮图标
				key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
				if(key_group)
				{
					key_group[0] = btn_creat(24, 176, 130, 122, 0, 1);
					key_group[0]->picbtnpathu = (u8 *)PTH_YR_000;
					key_group[0]->picbtnpathd = (u8 *)PTH_YR_000;
					key_group[1] = btn_creat(167, 176, 130, 122, 1, 1);
					key_group[1]->picbtnpathu = (u8 *)PTH_YR_001;
					key_group[1]->picbtnpathd = (u8 *)PTH_YR_001;

					key_group[2] = btn_creat(24, 314, 130, 122, 2, 1);

					if(extrudemulti_factor_diy == 10)
					{


						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_001;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_001;

					}
					else if(extrudemulti_factor_diy == 50)
					{


						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_002;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_002;

					}
					else if(extrudemulti_factor_diy == 100)
					{

						key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_003;
						key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_003;

					}

					key_group[3] = btn_creat(167, 314, 130, 122, 3, 1);
					key_group[3]->picbtnpathu = (u8 *)PTH_DYSD_004;
					key_group[3]->picbtnpathd = (u8 *)PTH_DYSD_004;

					key_group[4] = btn_creat(0, 0, 320, 54, 5, 1);

					for(i = 0; i < num - 1; i++)
					{
						btn_draw(key_group[i]);
					}
					//显示机器类型
					//					if(PrinterMode == 0)
					//					{
					//						minibmp_decode(PTH_KZ1_017, 280, 12, 30, 30, 0, 0);
					//					}
					//					else if(PrinterMode == 1)
					//					{
					//						minibmp_decode(PTH_KZ1_018, 280, 12, 30, 30, 0, 0);
					//					}
				}
				break;
			case 3:
				//显示机器类型
				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);
				gui_show_strmid(100, 17, 120, 16, WHITE, 16, text_display.L_Extrusion, 0);
				break;
			default:
				break;

		}
		if(display_redraw < 3)
		{
			display_redraw++;
			return;
		}
		else
		{
			display_redraw = 0;
			redraw_menu = FALSE;
		}


	}
	if(PrintInfo.printsd == 0 || PrintInfo.filament) //如果打印完成或者断料都跳到打印界面做处理
	{
		redraw_menu = TRUE;
		CurrentMenu = print_diy;
		CheckPrintFlg = FALSE;
	}
	//按键检测
	//	float speed_temp;
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				extrudemultiply -= extrudemulti_factor_diy;
				if(extrudemultiply < extrudemulti_factor_diy)
				{
					extrudemultiply = extrudemulti_factor_diy;
				}
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", extrudemultiply);
				gui_show_strmid(145, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 1:
				if(extrudemultiply <= (1000 - extrudemulti_factor_diy))
				{
					extrudemultiply += extrudemulti_factor_diy;
				}
				gui_phy.back_color = BACK2_DIY;
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", extrudemultiply);
				gui_show_strmid(145, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 2:
				gui_phy.back_color = BACK2_DIY;
				if(extrudemulti_factor_diy == 10)
				{
					extrudemulti_factor_diy = 50;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_002;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_002;
				}
				else if(extrudemulti_factor_diy == 50)
				{
					extrudemulti_factor_diy = 100;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_003;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_003;
				}
				else if(extrudemulti_factor_diy == 100)
				{
					extrudemulti_factor_diy = 10;
					key_group[2]->picbtnpathu = (u8 *)PTH_DYSD_001;
					key_group[2]->picbtnpathd = (u8 *)PTH_DYSD_001;
				}
				btn_draw(key_group[2]);
				break;
			case 3:
				extrudemultiply = 100;
				gui_phy.back_color = BACK2_DIY;
				//gui_fill_rectangle(150,96,20,16,BACK_DIY);
				memset(fatbuf, 0, 100);
				sprintf(fatbuf, "%d%%  ", extrudemultiply);
				gui_show_strmid(145, 116, 40, 16, WHITE, 16, fatbuf, 0);
				break;
			case 4:
				redraw_menu = TRUE;
				CurrentMenu = print_contrl_diy;
				break;
			default:
				break;
		}
	}
	if(speed_temprature_factor > EPSINON)
	{
		target_temperature[0] = 185 * speed_temprature_factor * (extrudemultiply - 100) / 100 + 185;
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

DIR FlBoxDir;		//flbox专用
FILINFO flboxfinfo;
u16 LastIndex = 0;	//上次索引
u16 LastFloderIndex[10] = {0};	//上个文件夹的文件索引
u8 PathNum = 0;			//路径层数
u8 LastNum = 0;			//上个目录显示的文件数量
GCODELIST_MODE GcodelistMode;


u8 LimitFlg = 0;			//bit0 1:可下翻 bit1 1:可上翻
/*
*Function:
*Input Paragramed:  entry_type 0第一次 1下一页 2上一页
*Output Paragramed:
*Remarks:通过路径获取文件夹下所有的文件及目录 返回数量
*/
u8 CheckSdCardFiles(GCODELIST_MODE entry_type)
{
	u8 i, j, res;
	u8 filewt_idx = 0;  //写入文件索引的索引
	u16 temp_16 = 0;
	u16 findex = 0;
	int type = 0;
	gcodefile_s gocdefile_temp;
	s8 in = 0;
	char *fn;   //This function is assuming non-Unicode cfg.
	#if _USE_LFN
	USR_UsrLog("malloc flboxfinfo.lfname");
	flboxfinfo.lfsize = _MAX_LFN * 2 + 1;
	flboxfinfo.lfname = mymalloc(SRAMIN, flboxfinfo.lfsize);
	memset(flboxfinfo.lfname, 0, flboxfinfo.lfsize);
	#endif
	switch(entry_type)
	{
		case FIRST_ENTRY:
			USR_UsrLog("entry_type=0");
			LimitFlg = 0;	//默认不能上翻下翻
			res = f_opendir(&FlBoxDir, (const TCHAR *)gcodefile_path); //打开一个目录
			if(res)
			{
				USR_ErrLog("open dir %s error", gcodefile_path);
				goto exit;
			}
			temp_16 = 0;
			filewt_idx = 0;
			while(temp_16 < 60000)	//避免死循环
			{
				temp_16++;
				res = f_readdir(&FlBoxDir, &flboxfinfo);
				if(res != FR_OK || flboxfinfo.fname[0] == 0)break;		//一直读到最后一个文件(夹)

				findex = (u16)FlBoxDir.index; //记录下此文件的index值
				USR_UsrLog("FlBoxDir.index=%d", FlBoxDir.index);
				#if _USE_LFN
				fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
				#else
				fn = flboxfinfo.fname;
				#endif
				USR_UsrLog("%s \n", fn);

				if(flboxfinfo.fattrib & (1 << 4))	//为目录
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("目录:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = FOLDER;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					if(filewt_idx >= 6)	//文件数量超过一页 置位可下翻标志位
						LimitFlg = 0x01;
					filewt_idx %= 6;
					USR_UsrLog("%s \n", fn);
					continue;				//跳过执行后面代码
				}
				type = f_typetell(fn);
				type >>= 4;
				if((1 << type) & (1 << 6))
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("Gcode:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = GCODE;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					if(filewt_idx >= 6)	//文件数量超过一页 置位可下翻标志位
						LimitFlg = 0x01;
					filewt_idx %= 6;
					USR_UsrLog("%s \n", fn);
				}
			}

			//对文件信息重新排序
			if(LimitFlg & 0x01)	//满一页
			{
				//顺序排列
				res = 6 - filewt_idx;
				for(i = 0; i < res; i++)
				{
					memcpy(&gocdefile_temp, &gcodefile_t[5], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[5]);
					for(in = 4; in >= 0; in--)
					{
						memcpy(&gcodefile_t[in + 1], &gcodefile_t[in], sizeof(gcodefile_s));
						strcpy(file_name_buf[in + 1], file_name_buf[in]);
					}
					memcpy(&gcodefile_t[0], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[0], fn);
				}
				//再倒序排列
				for(i = 0; i < 3; i++)
				{
					j = 5 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
				filewt_idx = 6;		//满一页返回6个文件数量
			}
			else	//如果数量不够一页不做顺序排序,只做倒序
			{
				if(filewt_idx == 0)
					goto exit;
				res = (filewt_idx - 1) / 2;
				for(i = 0; i < res; i++)
				{
					j = filewt_idx - 1 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
			}

			break;
		case PAGE_DOWN:		//触发下一页标志
			USR_UsrLog("PAGE_DOWN");
			if((LimitFlg & 0x01) == 0)
			{
				goto exit;
			}
			LimitFlg = 0x02;
			LastIndex = gcodefile_t[5].file_index;
			res = f_closedir(&FlBoxDir);
			if(res)
			{
				USR_ErrLog("close dir error");
				goto exit;
			}
			res = f_opendir(&FlBoxDir, (const TCHAR *)gcodefile_path); //打开一个目录
			if(res)
			{
				USR_ErrLog("open dir %s error", gcodefile_path);
				goto exit;
			}
			while(temp_16 < 60000)	//避免死循环
			{
				temp_16++;
				res = f_readdir(&FlBoxDir, &flboxfinfo);
				if(res != FR_OK || flboxfinfo.fname[0] == 0)break;		//一直读到最后一个文件(夹)

				if(FlBoxDir.index >= LastIndex)
				{
					dir_sdi(&FlBoxDir, findex);
					break;
				}
				else
					findex = (u16)FlBoxDir.index;

				USR_UsrLog("FlBoxDir.index=%d", FlBoxDir.index);
				#if _USE_LFN
				fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
				#else
				fn = flboxfinfo.fname;
				#endif
				USR_UsrLog("%s \n", fn);

				if(flboxfinfo.fattrib & (1 << 4))	//为目录
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("目录:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = FOLDER;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					if(filewt_idx >= 6)	//文件数量超过一页
						LimitFlg |= 0x01;
					filewt_idx %= 6;
					USR_UsrLog("%s \n", fn);
					continue;				//跳过执行后面代码
				}
				type = f_typetell(fn);
				type >>= 4;
				if((1 << type) & (1 << 6))
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("Gcode:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = GCODE;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					if(filewt_idx >= 6)	//文件数量超过一页
						LimitFlg |= 0x01;
					filewt_idx %= 6;
					USR_UsrLog("%s \n", fn);
				}
			}

			//对文件信息重新排序
			if(LimitFlg & 0x01)	//满一页
			{
				//顺序排列
				res = 6 - filewt_idx;
				for(i = 0; i < res; i++)
				{
					memcpy(&gocdefile_temp, &gcodefile_t[5], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[5]);
					for(in = 4; in >= 0; in--)
					{
						memcpy(&gcodefile_t[in + 1], &gcodefile_t[in], sizeof(gcodefile_s));
						strcpy(file_name_buf[in + 1], file_name_buf[in]);
					}
					memcpy(&gcodefile_t[0], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[0], fn);
				}
				//再倒序排列
				for(i = 0; i < 3; i++)
				{
					j = 5 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
				filewt_idx = 6;		//满一页返回6个文件数量
			}
			else	//如果数量不够一页不做顺序排序,只做倒序
			{
				if(filewt_idx == 0)
					goto exit;
				res = (filewt_idx - 1) / 2;
				for(i = 0; i < res; i++)
				{
					j = filewt_idx - 1 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
			}
			break;
		case PAGE_UP:
			USR_UsrLog("PAGE_UP");
			temp_16 = 0;
			filewt_idx = 0;
			while(temp_16 < 50000)	//避免死循环
			{
				temp_16++;
				if(filewt_idx >= 6)	//每次最多6个
					break;

				res = f_readdir(&FlBoxDir, &flboxfinfo);
				if(res != FR_OK || flboxfinfo.fname[0] == 0)break;

				findex = (u16)FlBoxDir.index; //记录下此文件的index值

				USR_UsrLog("FlBoxDir.index=%d", FlBoxDir.index);
				#if _USE_LFN
				fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
				#else
				fn = flboxfinfo.fname;
				#endif
				USR_UsrLog("%s \n", fn);

				if(flboxfinfo.fattrib & (1 << 4))	//为目录
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("目录:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = FOLDER;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					USR_UsrLog("%s \n", fn);
					continue;				//跳过执行后面代码
				}
				type = f_typetell(fn);
				type >>= 4;
				if((1 << type) & (1 << 6))
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("Gcode:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = GCODE;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					USR_UsrLog("%s \n", fn);
				}
			}
			if(filewt_idx < 6)	//不能上翻
				LimitFlg = 0x01;
			else
				LimitFlg = 0x03;	//可下翻
			//对文件信息重新排序
			//对文件信息重新排序
			if(LimitFlg & 0x02)	//满一页
			{
				//倒序排列
				for(i = 0; i < 3; i++)
				{
					j = 5 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
				filewt_idx = 6;		//满一页返回6个文件数量
			}
			else	//如果数量不够一页不做顺序排序,只做倒序
			{
				if(filewt_idx == 0)	//一个数据也没有
				{
					LimitFlg &= 0x01;
					filewt_idx = gcodefile_num;	//之前是多少还是显示多少
					goto exit;
				}
				res = (filewt_idx - 1) / 2;
				for(i = 0; i < res; i++)
				{
					j = filewt_idx - 1 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
			}
			break;
		case LAST_FLODER:
			USR_UsrLog("LAST_FLODER");
			res = f_opendir(&FlBoxDir, (const TCHAR *)gcodefile_path); //打开一个目录
			if(res)
			{
				USR_ErrLog("open dir %s error", gcodefile_path);
				goto exit;
			}
			temp_16 = 0;
			filewt_idx = 0;
			while(temp_16 < 50000)	//避免死循环
			{
				temp_16++;
				if(filewt_idx >= LastNum)	//每次最多6个
					break;

				res = f_readdir(&FlBoxDir, &flboxfinfo);
				if(res != FR_OK || flboxfinfo.fname[0] == 0)break;

				findex = (u16)FlBoxDir.index; //记录下此文件的index值
				if(findex < LastFloderIndex[PathNum])
					continue;

				USR_UsrLog("FlBoxDir.index=%d", FlBoxDir.index);
				#if _USE_LFN
				fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
				#else
				fn = flboxfinfo.fname;
				#endif
				USR_UsrLog("%s \n", fn);

				if(flboxfinfo.fattrib & (1 << 4))	//为目录
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("目录:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = FOLDER;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					USR_UsrLog("%s \n", fn);
					continue;				//跳过执行后面代码
				}
				type = f_typetell(fn);
				type >>= 4;
				if((1 << type) & (1 << 6))
				{
					USR_UsrLog("%d:  ", findex);
					USR_UsrLog("Gcode:");
					gcodefile_t[filewt_idx].file_index = findex;
					gcodefile_t[filewt_idx].file_type = GCODE;
					#if _USE_LFN
					fn = *flboxfinfo.lfname ? flboxfinfo.lfname : flboxfinfo.fname;
					#else
					fn = flboxfinfo.fname;
					#endif
					strcpy(file_name_buf[filewt_idx], fn);
					filewt_idx++;
					USR_UsrLog("%s \n", fn);
				}
			}
			if(filewt_idx < 6)	//不能上下翻页
				LimitFlg = 0x01;
			else
				LimitFlg = 0x03;	//可下翻
			//对文件信息重新排序
			if(LimitFlg & 0x02)	//满一页
			{
				//倒序排列
				for(i = 0; i < 3; i++)
				{
					j = 5 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);
				}
				filewt_idx = 6;		//满一页返回6个文件数量
			}
			else	//如果数量不够一页不做顺序排序,只做倒序
			{
				if(filewt_idx == 0)
					goto exit;
				res = (filewt_idx - 1) / 2;
				for(i = 0; i < res; i++)
				{
					j = filewt_idx - 1 - i;

					memcpy(&gocdefile_temp, &gcodefile_t[i], sizeof(gcodefile_s));
					strcpy(fn, file_name_buf[i]);

					memcpy(&gcodefile_t[i], &gcodefile_t[j], sizeof(gcodefile_s));
					strcpy(file_name_buf[i], file_name_buf[j]);

					memcpy(&gcodefile_t[j], &gocdefile_temp, sizeof(gcodefile_s));
					strcpy(file_name_buf[j], fn);

				}
			}
			break;
		default:
			break;
	}
exit:
	#if _USE_LFN
	USR_UsrLog("free flboxfinfo.lfname");
	myfree(SRAMIN, flboxfinfo.lfname);
	#endif
	USR_UsrLog("filewt_idx = %d", filewt_idx);
	return filewt_idx;
}
bool RePopout = FALSE;		//重画弹框
void GetGcodeTab2_diy(void)
{

	u8 selx = 0XFF;
	u8 i, j, res;

	u16 iconX_position[6], iconY_position[6];
	iconX_position[0] = 54;
	iconY_position[0] = 168;
	iconX_position[1] = 54;
	iconY_position[1] = 218;
	iconX_position[2] = 54;
	iconY_position[2] = 268;
	iconX_position[3] = 54;
	iconY_position[3] = 318;
	iconX_position[4] = 54;
	iconY_position[4] = 368;
	iconX_position[5] = 54;
	iconY_position[5] = 418;
	if(windows_flag == false)
	{
		if(redraw_menu == true)
		{
			redraw_menu = false;
			gcodefile_num = 0;
			btnshow_change_flag = FALSE;
			PathNum = 0;
			LastNum = 0;
			LastIndex = 0;
			LimitFlg = 0;
			strcpy(gcodefile_path, "0:");
			strcpy(OldGcodeFilePath, "0:");
			//gui_fill_rectangle(0, 0, lcddev.width, lcddev.height, gui_phy.back_color);
			LCD_Clear(gui_phy.back_color);
			//画背景
			screen_key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *) * 11);   //分配三个按键的资源
			if(screen_key_group)
			{
				for(i = 0; i < 2; i++)
				{
					screen_key_group[i] = btn_creat(66 + i * 126, 235, 60, 30, i, 2);
					screen_key_group[i]->bkctbl[0] = GRAYBLUE;
					screen_key_group[i]->bkctbl[1] = GRAYBLUE;
					screen_key_group[i]->bkctbl[2] = GRAYBLUE;
					screen_key_group[i]->bkctbl[3] = GRAYBLUE;
					screen_key_group[i]->bcfdcolor = WHITE;
					screen_key_group[i]->bcfucolor = WHITE;
				}
				screen_key_group[0]->caption = text_display.L_Print;                             //打印按键
				screen_key_group[1]->caption = text_display.L_Back;                              //返回按键

				screen_key_group[2] = btn_creat(0, 0, 320, 54, 5, 1);

				screen_key_group[3] = btn_creat(14, 67, 139, 72, 0, 1);
				screen_key_group[3]->picbtnpathu = (u8 *)PTH_FILE_002;
				screen_key_group[3]->picbtnpathd = (u8 *)PTH_FILE_002;

				screen_key_group[4] = btn_creat(167, 67, 139, 72, 0, 1);
				screen_key_group[4]->picbtnpathu = (u8 *)PTH_FILE_003;
				screen_key_group[4]->picbtnpathd = (u8 *)PTH_FILE_003;

				for(i = 5; i < 11; i++)
				{
					screen_key_group[i] = btn_creat(iconX_position[i - 5], iconY_position[i - 5], 266, 35, i, BTN_TYPE_TEXTB);
					screen_key_group[i]->bkctbl[0] = BACK_DIY;
					screen_key_group[i]->bkctbl[1] = BACK_DIY;
					screen_key_group[i]->bkctbl[2] = BACK_DIY;
					screen_key_group[i]->bkctbl[3] = BACK_DIY;
					screen_key_group[i]->bcfucolor = WHITE;
					screen_key_group[i]->caption_left = 0;
					screen_key_group[i]->caption_top = 3;
					screen_key_group[i]->caption = " ";
				}

				//读取文件(夹) 每次最多读取6个
				memset(gcodefile_t, 0, sizeof(gcodefile_s) * 6);
				USR_UsrLog("first traversal files");
				gcodefile_num = CheckSdCardFiles(FIRST_ENTRY);

				gui_phy.back_color = BACK_DIY;
				gui_draw_hline(0, 54, 320, WHITE);
				gui_show_strmid(5, 17, 64, 16, WHITE, 16, text_display.L_Back, 0);																																		//返回按键重构
				gui_show_strmid(0, 0, 320, 54, WHITE, 16, text_display.L_SDPrint, 1);               //名字栏TEXT显示
				//创建按钮
				for(i = 0; i < gcodefile_num; i++)
				{
					screen_key_group[5 + i]->caption = (u8 *)file_name_buf[i];

					if(gcodefile_t[i].file_type == GCODE)
					{
						minibmp_decode(PTH_FILE_000, iconX_position[i] - 37, iconY_position[i], 35, 35, 0, 0);	//加载文件背景
					}
					else if(gcodefile_t[i].file_type == FOLDER)		//加载文件夹背景
					{
						minibmp_decode(PTH_FILE_001, iconX_position[i] - 37, iconY_position[i], 35, 35, 0, 0);
					}
				}

				for(i = 2; i < 11; i++)
				{
					btn_draw(screen_key_group[i]);
				}
				//剩余显示背景色的按钮
				for(i = gcodefile_num; i < 6; i++)
				{
					gui_fill_rectangle(iconX_position[i] - 37, iconY_position[i], 303, 50, gui_phy.back_color);
				}
			}

		}


		if(btnshow_change_flag == true)
		{
			USR_UsrLog("显示文件按键改变");
			//显示文件按键改变
			btnshow_change_flag = false;
			for(i = 0; i < gcodefile_num; i++)
			{
				screen_key_group[5 + i]->caption = (u8 *)file_name_buf[i];
				btn_draw(screen_key_group[5 + i]);

				if(gcodefile_t[i].file_type == GCODE)
				{
					minibmp_decode(PTH_FILE_000, iconX_position[i] - 37, iconY_position[i], 35, 35, 0, 0);	//加载文件背景
				}
				else if(gcodefile_t[i].file_type == FOLDER)		//加载文件夹背景
				{
					minibmp_decode(PTH_FILE_001, iconX_position[i] - 37, iconY_position[i], 35, 35, 0, 0);
				}
			}
			//剩余显示背景色的按钮
			for(i = gcodefile_num; i < 6; i++)
			{
				gui_fill_rectangle(iconX_position[i] - 37, iconY_position[i], 303, 50, gui_phy.back_color);
			}
		}
		selx = screen_key_chk(screen_key_group + 2, &in_obj, 9);
		if(selx & (1 << 6))
		{
			bool press_flag = FALSE;
			switch(selx & ~(3 << 6))
			{
				case 0:
					if(0 == strcmp(gcodefile_path, "0:"))
					{
						USR_UsrLog("root directory,return main menu or set menu");
						PathNum = 0;
						redraw_menu = TRUE;
						if(reprint.break_point_flg == FALSE)
						{
							CurrentMenu = main_diy;
						}
						else
						{
							CurrentMenu = set_diy;
							reprint.break_point_flg = FALSE;
						}
					}
					else
					{
						for(i = 99; i > 0; i--)
						{
							if(gcodefile_path[i] == 0x5c)// '\'
							{
								for(j = i; j < 100; j++)
									gcodefile_path[j] = '\0';
								break;
							}
						}
						USR_UsrLog("Back to the previous level %s", gcodefile_path);
						PathNum--;
						gcodefile_num = CheckSdCardFiles(LAST_FLODER);

						btnshow_change_flag = true;
						redraw_menu = false;
						windows_flag = false;
					}
					break;
				case 1:
					if(LimitFlg & 0x02)	//到底标志
					{
						USR_UsrLog("previous page");
						btnshow_change_flag = true;
						redraw_menu = false;
						windows_flag = false;
						gcodefile_num = CheckSdCardFiles(PAGE_UP);
					}
					break;
				case 2:
					if(LimitFlg & 0x01)
					{
						USR_UsrLog("next page");
						btnshow_change_flag = true;
						redraw_menu = false;
						windows_flag = false;
						gcodefile_num = CheckSdCardFiles(PAGE_DOWN);
					}
					break;
				case 3:
					USR_UsrLog("0");
					gcodefile_index_sub = 0;
					press_flag = TRUE;
					break;
				case 4:
					USR_UsrLog("1");
					gcodefile_index_sub = 1;
					press_flag = TRUE;
					break;
				case 5:
					USR_UsrLog("2");
					gcodefile_index_sub = 2;
					press_flag = TRUE;
					break;
				case 6:
					USR_UsrLog("3");
					gcodefile_index_sub = 3;
					press_flag = TRUE;
					break;
				case 7:
					USR_UsrLog("4");
					gcodefile_index_sub = 4;
					press_flag = TRUE;
					break;
				case 8:
					USR_UsrLog("5");
					gcodefile_index_sub = 5;
					press_flag = TRUE;
					break;
				default:
					break;
			}
			if(press_flag)
			{
				press_flag = 0;
				if(gcodefile_num > gcodefile_index_sub)
				{
					if(gcodefile_t[gcodefile_index_sub].file_type == GCODE)//
					{
						RePopout = true;
						windows_flag = true;
					}
					else
					{
						if(PathNum >= 10)
							return;
						//如果是文件 1、  文件路径改变 2、按键名改变 (1、读取索引base改变 2、重新读取)
						btnshow_change_flag = true;
						redraw_menu = false;
						windows_flag = false;
						strcpy(OldGcodeFilePath, gcodefile_path);
						LastFloderIndex[PathNum] = gcodefile_t[gcodefile_num - 1].file_index;	//保存上个目录的最后文件索引值
						PathNum++;
						LastNum = gcodefile_num;

						gui_path_name(gcodefile_path, gcodefile_path, file_name_buf[gcodefile_index_sub]);	//文件名加入路径
						gcodefile_index_sub = 0;
						USR_UsrLog("进入路径%s", gcodefile_path);
						gcodefile_num = CheckSdCardFiles(FIRST_ENTRY);
					}
				}
			}
		}
	}
	else
	{
		if(RePopout == true)
		{

			RePopout = false;
			gui_draw_arcrectangle(0, 180, 320, 40, 6, 1, GRAYBLUE, GRAYBLUE);
			gui_fill_rectangle(0, 215, 320, 70, WHITE); //
			gui_draw_rectangle(0, 215, 320, 70, GRAYBLUE);
			gui_show_strmid(0, 180, 320, 32, WHITE, 16, file_name_buf[gcodefile_index_sub], 1);
			if(screen_key_group)
			{
				for(i = 0; i < 2; i++)
				{
					btn_draw(screen_key_group[i]);
				}
			}
		}
		selx = screen_key_chk(screen_key_group, &in_obj, 2);
		if(selx & (1 << 6))
		{
			switch(selx & ~(3 << 6))
			{
				case 0:
					if((PrinterMode == 0) && (PrintInfo.filament != 0))
					{
						if(BeepSwitch)
							BeepFlg = TRUE;
						windows_flag = false;
						btnshow_change_flag = true;
						redraw_menu = true;
						gui_memin_free(screen_key_group);
						CurrentMenu = print_filament;
						break;
					}

					strcpy((char *)card.filename, (const char *)file_name_buf[gcodefile_index_sub % GCODE_FILE_SHOW_NUM]);
					if(pname)
					{
						gui_path_name(pname, gcodefile_path, (u8 *)file_name_buf[gcodefile_index_sub % GCODE_FILE_SHOW_NUM]);
						USR_UsrLog("pname = %s", pname);
						selx = f_open(&card.fgcode, (const TCHAR *)pname, FA_READ);
						if(selx == FR_OK)
						{
							if(PrinterMode == 0)
							{
								CurrentMenu = print_diy;

							}

							else if(PrinterMode == 1)
							{
								enquecommand("M5");		//关闭激光头
								CurrentMenu = laser_print_diy;

							}
							i = 0xff;
							if(reprint.break_point_flg == TRUE)
							{
								memset(fatbuf, 0, sizeof(fatbuf));
								i = AT24CXX_ReadOneByte(E_FILENAME);
								AT24CXX_Read(E_FILENAME + 1, fatbuf, i);
								i = strncmp(fatbuf, pname, 50);
							}
							if(i == 0)	//文件名相同
							{
								AT24CXX_Read(E_EXTRUDERTEMPERATURE, (u8 *)&reprint.temperature_extruder[0], sizeof(target_temperature[0]));
								USR_UsrLog("read save extruder temperature is %d", reprint.temperature_extruder[0]);
								AT24CXX_Read(E_BEDTEMPERATURE, (u8 *)&reprint.temperature_bed, sizeof(target_temperature_bed));
								USR_UsrLog("read save bed temperature is %d", reprint.temperature_bed);

								//读取打印时间
								AT24CXX_Read(PRINT_HOUR, (u8 *)&usr_timer.hour, sizeof(usr_timer.hour));
								AT24CXX_Read(PRINT_DATE, (u8 *)&usr_timer.date, sizeof(usr_timer.date));
								AT24CXX_Read(PRINT_MIN, (u8 *)&usr_timer.min, sizeof(usr_timer.min));
								AT24CXX_Read(E_MULIT_SPEED, (u8 *)&feedmultiply, 4);	//打印倍速
								feedmultiply = constrain(feedmultiply, 100, 500);
								AT24CXX_Read(E_ACTIVE_EXTRUDER, (u8 *)&reprint.active_extruder, sizeof(reprint.active_extruder));	//打印倍速
								if(reprint.active_extruder > 1)
									reprint.active_extruder = 0;
								//读取续打偏移值
								AT24CXX_Read(POINTER_FILE, (u8 *)&reprint.lseek, sizeof(reprint.lseek));
								USR_UsrLog("read save filepoint is %d\r\n", reprint.lseek);

								res = f_lseek(&card.fgcode, reprint.lseek);	//跳转到续打位置
								if(res)
								{
									USR_ErrLog("f_lseek error=%d", res);
								}
								else
								{
									USR_UsrLog("fptr=%x", card.fgcode.fptr);
								}

								//读取保存E轴值和Z轴高度值

								AT24CXX_Read(E_G_CODE_EAXIS, (u8 *)&reprint.e_axis, sizeof(reprint.e_axis));
								AT24CXX_Read(HEIGHT_MODE, (u8 *)&reprint.z_axis, sizeof(reprint.z_axis));
								AT24CXX_Read(E_X_POS, (u8 *)&reprint.x_axis, sizeof(reprint.x_axis));
								AT24CXX_Read(E_Y_POS, (u8 *)&reprint.y_axis, sizeof(reprint.y_axis));
								AT24CXX_Read(E_ACTIVE_EXTRUDER, (u8 *)&active_extruder, sizeof(active_extruder));
								PrintInfo.printsd = 1;
								strcpy(PrintInfo.printfile, card.filename);	//更新打印文件名
								PrintInfo.filesize = card.fgcode.fsize;		//打印文件大小
								PrintInfo.printper = ((float)reprint.lseek / PrintInfo.filesize) * 100;
								PrintInfo.printtime = 0;
								card.sdprinting = FALSE;	//停止打印
							}
							else
							{
								strcpy(PrintInfo.printfile, file_name_buf[gcodefile_index_sub % GCODE_FILE_SHOW_NUM]); //更新打印文件名
								PrintInfo.filesize = card.fgcode.fsize;         //更新打印文件大小
								if(EXTRUDER_NUM == 1)							//更新默认打印速度
									feedmultiply = 100;
								else
									feedmultiply = 100;
								USR_UsrLog("printfile is %s\r\n", PrintInfo.printfile);
								USR_UsrLog("filesize is %d\r\n", PrintInfo.filesize);
								card_startFileprint();
								starttime = millis();
							}


							redraw_menu = true;
							windows_flag = false;
							for(i = 0; i < 11; i++)
							{
								btn_delete(screen_key_group[i]);
							}
							gui_memin_free(screen_key_group);


						}
					}
					break;
				case 1:
					windows_flag = false;
					btnshow_change_flag = true;
					gui_fill_rectangle(0, 180, 320, 110, BACK_DIY);
					break;
				default:
					break;
			}
		}
	}
	if((card.cardrelase == 1) || (card.cardOK == FALSE))
	{
		redraw_menu = true;
		windows_flag = FALSE;
		CurrentMenu = main_diy;
	}
	if(redraw_menu)
	{

		memset(LastFloderIndex, 0, sizeof(LastFloderIndex));

		for(i = 0; i < 11; i++)
		{
			btn_delete(screen_key_group[i]);
		}
		gui_memin_free(screen_key_group);
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void stop_laser_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;
	uint8_t temp[32];
	gui_phy.back_color = BACK2_DIY;

	if(redraw_menu == TRUE)
	{
		redraw_menu = FALSE;
		//打印完成动作
		gui_draw_arcrectangle(75, 300, 180, 40, 6, 1, GRAYBLUE, GRAYBLUE);
		gui_fill_rectangle(75, 300 + 30, 180, 70, WHITE);
		gui_draw_rectangle(75, 300 + 30, 180, 70, GRAYBLUE);
		gui_show_strmid(75, 300, 180, 30, WHITE, 16, text_display.L_PrintFinish, 1);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
		if(key_group)
		{
			key_group[0] = btn_creat(135, 300 + 45, 60, 40, 0, 2);
			key_group[0]->bkctbl[0] = 0X8452;
			key_group[0]->bkctbl[1] = 0XAD97;
			key_group[0]->bkctbl[2] = 0XAD97;
			key_group[0]->bkctbl[3] = 0X8452;
			key_group[0]->caption = (u8 *)text_display.L_Confirm;
			btn_draw(key_group[0]);
		}


	}
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6)) //
		{
			case 0:
				quickStop();
				card_closefile();
				starttime = 0;
				card.sdprinting = FALSE;
				PrintInfo.printsd = 0;
				AT24CXX_WriteOneByte(E_SDPRINT, FALSE);
				autotempShutdown();
				setTargetHotend(0, active_extruder);
				setTargetBed(0);

				memset(CmdBuff.cmdbuffer, 0, sizeof(CmdBuff.cmdbuffer));

				bufindr = 0;
				bufindw = 0;
				buflen = 0;

				disable_x();
				disable_y();
				enable_z();
				redraw_menu = TRUE;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void stop_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;
	uint8_t temp[32];
	gui_phy.back_color = BACK2_DIY;
	progressbar_diy(37, 50, 100);
	if(redraw_menu == TRUE)
	{
		redraw_menu = FALSE;
		//打印完成动作
		//enquecommand("G92 X0 Y0 Z0 E0");
		sprintf(temp, "G1 Z%f", current_position[Z_AXIS] + 10);
		enquecommand(temp);
		enquecommand("G28 XY");
		fanSpeed = 0;		//打印完毕关闭风扇

		gui_draw_arcrectangle(75, 160, 180, 40, 6, 1, GRAYBLUE, GRAYBLUE);
		gui_fill_rectangle(75, 190, 180, 70, WHITE);
		gui_draw_rectangle(75, 190, 180, 70, GRAYBLUE);
		gui_show_strmid(75, 160, 180, 30, WHITE, 16, text_display.L_PrintFinish, 1);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
		if(key_group)
		{
			key_group[0] = btn_creat(135, 205, 60, 40, 0, 2);
			key_group[0]->bkctbl[0] = 0X8452;
			key_group[0]->bkctbl[1] = 0XAD97;
			key_group[0]->bkctbl[2] = 0XAD97;
			key_group[0]->bkctbl[3] = 0X8452;
			key_group[0]->caption = (u8 *)text_display.L_Confirm;
			btn_draw(key_group[0]);
		}


	}
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6)) //
		{
			case 0:
				quickStop();
				card_closefile();
				starttime = 0;
				card.sdprinting = FALSE;
				PrintInfo.printsd = 0;
				AT24CXX_WriteOneByte(E_SDPRINT, FALSE);
				autotempShutdown();
				setTargetHotend(0, active_extruder);
				setTargetBed(0);
				fanSpeed = 0;
				fanSpeed1 = 0;
				enquecommand("M84");	//关闭所有电机
				memset(CmdBuff.cmdbuffer, 0, sizeof(CmdBuff.cmdbuffer));

				bufindr = 0;
				bufindw = 0;
				buflen = 0;
				disable_x();
				disable_y();
				disable_z();
				disable_e0();
				disable_e1();

				redraw_menu = TRUE;
				CurrentMenu = main_diy;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

void filament_run_out_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;

	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu == TRUE)
	{
		redraw_menu = FALSE;

		gui_draw_arcrectangle(75, 160, 180, 40, 6, 1, GRAYBLUE, GRAYBLUE);
		gui_fill_rectangle(75, 190, 180, 70, WHITE);
		gui_draw_rectangle(75, 190, 180, 70, GRAYBLUE);
		gui_show_strmid(75, 160, 180, 30, WHITE, 16, text_display.L_filament, 1);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
		if(key_group)
		{
			key_group[0] = btn_creat(135, 205, 60, 40, 0, 2);
			key_group[0]->bkctbl[0] = 0X8452;
			key_group[0]->bkctbl[1] = 0XAD97;
			key_group[0]->bkctbl[2] = 0XAD97;
			key_group[0]->bkctbl[3] = 0X8452;
			key_group[0]->caption = (u8 *)text_display.L_Confirm;
			btn_draw(key_group[0]);
		}


	}
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6)) //
		{
			case 0:
				redraw_menu = TRUE;
				filament_flg = 1;
				lastMenu = print_diy;
				CurrentMenu = extruder_diy;
				BeepFlg = FALSE;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
//打印前缺料提醒
void print_filament(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;

	gui_phy.back_color = BACK2_DIY;
	if(redraw_menu == TRUE)
	{
		redraw_menu = FALSE;

		gui_draw_arcrectangle(75, 160, 180, 40, 6, 1, GRAYBLUE, GRAYBLUE);
		gui_fill_rectangle(75, 190, 180, 70, WHITE);
		gui_draw_rectangle(75, 190, 180, 70, GRAYBLUE);
		gui_show_strmid(75, 160, 180, 30, WHITE, 16, text_display.L_filament, 1);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);
		if(key_group)
		{
			key_group[0] = btn_creat(135, 205, 60, 40, 0, 2);
			key_group[0]->bkctbl[0] = 0X8452;
			key_group[0]->bkctbl[1] = 0XAD97;
			key_group[0]->bkctbl[2] = 0XAD97;
			key_group[0]->bkctbl[3] = 0X8452;
			key_group[0]->caption = (u8 *)text_display.L_Confirm;
			btn_draw(key_group[0]);
		}


	}
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6)) //
		{
			case 0:
				redraw_menu = TRUE;
				CurrentMenu = main_diy;
				BeepFlg = FALSE;
				break;
			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:手动停止打印
*/
void manual_stop_diy(void)
{
	uint8_t temp[32];
	quickStop();
	SavePrintInfo();
	card_closefile();
	starttime = 0;
	card.sdprinting = FALSE;

	PrintInfo.printsd = 0;
	AT24CXX_WriteOneByte(E_SDPRINT, FALSE);
	autotempShutdown();
	setTargetHotend(0, active_extruder);
	setTargetBed(0);
	disable_x();
	disable_y();
	disable_z();
	disable_e0();
	disable_e1();
	memset(CmdBuff.cmdbuffer, 0, sizeof(CmdBuff.cmdbuffer));
	//========================Add By Kim=====================
	buflen = 0;
	bufindr = 0;
	bufindw = 0;
	//=======================================================
	if(PrinterMode == 0)
	{
		sprintf(temp, "G1 Z%f", current_position[Z_AXIS] + 10);
		enquecommand(temp);
		enquecommand(temp);
		enquecommand("G28 XY");
		enquecommand("M84");	//关闭所有电机
		fanSpeed = 0;
		fanSpeed1 = 0;
	}
	else
	{
		//LaserSwitch = FALSE;
		//LASER = LaserSwitch;
		enquecommand("M5");	//关激光
		enquecommand("M5");	//关激光
		enquecommand("M84");
	}
	filament_flg = 0;			//清除断料标志
	bmp_flag = FALSE;
	CurrentMenu = main_diy;
	redraw_menu = TRUE;
}
void demo_diy(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 1;
	static u8 name = 1;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(0, 0, 320, 240, 0, 1);
			key_group[0]->picbtnpathu = (u8 *)"1:/diy_UI/1.bmp";
			key_group[0]->picbtnpathd = (u8 *)"1:/diy_UI/1.bmp";
			btn_draw(key_group[0]);
		}
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				if(name >= 10)
					name = 1;
				memset(fatbuf, 0, 50);
				sprintf(fatbuf, "1:/diy_UI/%d.bmp", name++);
				key_group[0]->picbtnpathu = (u8 *)fatbuf;
				key_group[0]->picbtnpathd = (u8 *)fatbuf;
				btn_draw(key_group[0]);
				break;

			default:
				break;
		}
	}
	if(redraw_menu)
	{
		for(i = 0; i < num; i++)
		{
			btn_delete(key_group[i]);
		}
		gui_memin_free(key_group);
		display_next_update_millis = 0;
	}
}
u8 storage_test(void)
{
	FIL fil;
	FRESULT fr;
	u8 buff[12] = "eeprom_test";
	u8 res = 0;
	//sd卡
	fr = f_open(&fil, "0:sd", FA_CREATE_ALWAYS);
	if(fr)
		res |= 0x01;
	else
	{
		fr = f_open(&fil, "0:sd", FA_OPEN_EXISTING);
		if(fr)
			res |= 0x01;
	}

	//spiflash
	fr = f_open(&fil, "1:spi", FA_CREATE_ALWAYS);
	if(fr)
		res |= 0x02;
	else
	{
		fr = f_open(&fil, "1:spi", FA_OPEN_EXISTING);
	}
	if(fr)
		res |= 0x02;
	//eeprom
	AT24CXX_Write(TEST_ADDR, buff, sizeof(buff));
	memset(buff, 0, sizeof(buff));
	AT24CXX_Read(TEST_ADDR, buff, sizeof(buff));
	if(memcmp(buff, "eeprom_test", sizeof(buff)))
		res |= 0x04;
	return res;
}

u8 StopStatus[6] = {0};
void main_test(void)
{
	u8 selx = 0XFF;
	u8 i;
	u8 num = 6;
	u16 color;
	u8 res;
	GPIO_InitTypeDef  GPIO_InitStructure;
	if(redraw_menu)
	{
		redraw_menu = FALSE;
		display_next_update_millis = millis() + 1000;
		bl_touch_init();
		//初始化MOS管控制引脚
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
		GPIO_Init(GPIOD, &GPIO_InitStructure);
		GPIO_ResetBits(GPIOC, GPIO_Pin_8);
		GPIO_ResetBits(GPIOD, GPIO_Pin_14 | GPIO_Pin_15);
		//限位开关检测
		LCD_Clear(gui_phy.back_color);
		gui_fill_rectangle(0, 0, 320, 40, BACK2_DIY);
		gui_show_string("限位状态: X Y Z U V W ", 10, 10, 320, 16, 16, WHITE);
		StopStatus[0] = X_MIN_PIN;
		StopStatus[1] = Y_MIN_PIN;
		StopStatus[2] = Z_MIN_PIN;
		StopStatus[3] = STOP_U;
		StopStatus[4] = STOP_V;
		StopStatus[5] = STOP_W;
		for(i = 0; i < 6; i++)
		{
			if(StopStatus[i])
				color = RED;
			else
				color = GREEN;
			gui_draw_bigpoint(92 + i * 16, 35, color);
		}
		//温度检测
		gui_fill_rectangle(0, 45, 320, 48, BACK2_DIY);
		memset(fatbuf, 0, 50);
		gui_show_string("温度1   温度2   温度3   温度4", 10, 53, 320, 16, 16, WHITE);

		(current_temperature[0] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[0]);
		gui_show_string(fatbuf, 10, 69, 64, 16, 16, color);

		(current_temperature_bed > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature_bed);
		gui_show_string(fatbuf, 10 + 64, 69, 64, 16, 16, color);

		(current_temperature[1] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[1]);
		gui_show_string(fatbuf, 10 + 64 * 2, 69, 64, 16, 16, color);

		(current_temperature[2] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[2]);
		gui_show_string(fatbuf, 10 + 64 * 3, 69, 64, 16, 16, color);

		//存储设备检测

		gui_fill_rectangle(0, 98, 320, 48, BACK2_DIY);
		gui_show_string("EEPROM  SPIFLASH  SD卡", 10, 98 + 8, 320, 16, 16, WHITE);
		res = storage_test();
		if((res & 0x04) == 0)	//成功
		{
			gui_show_string("OK", 10, 98 + 24, 320, 16, 16, GREEN);
		}
		else
		{
			gui_show_string("ERROR", 10, 98 + 24, 320, 16, 16, RED);
		}

		if((res & 0x02) == 0)	//成功
		{
			gui_show_string("OK", 10 + 64, 98 + 24, 320, 16, 16, GREEN);
		}
		else
		{
			gui_show_string("ERROR", 10 + 64, 98 + 24, 320, 16, 16, RED);
		}

		if((res & 0x01) == 0)	//成功
		{
			gui_show_string("OK", 10 + 144, 98 + 24, 320, 16, 16, GREEN);
		}
		else
		{
			gui_show_string("ERROR", 10 + 144, 98 + 24, 320, 16, 16, RED);
		}

		gui_phy.back_color = BACK2_DIY;
		gui_fill_rectangle(0, 120 + 32, 320, 75, BACK2_DIY);
		gui_show_strmid(0, 125 + 32, 320, 16, WHITE, 16, "MOS管测试", 0);

		gui_fill_rectangle(0, 200 + 32, 320, 75, BACK2_DIY);
		gui_show_strmid(0, 205 + 32, 320, 16, WHITE, 16, "电机测试", 0);
		key_group = (_btn_obj **)gui_memin_malloc(sizeof(_btn_obj *)*num);	//创建三个图标
		if(key_group)
		{
			key_group[0] = btn_creat(0, 142 + 32, 120, 50, 0, 4);
			key_group[1] = btn_creat(180, 142 + 32, 120, 50, 1, 4);
			key_group[2] = btn_creat(0, 225 + 32, 120, 50, 2, 4);
			key_group[3] = btn_creat(180, 225 + 32, 120, 50, 3, 4);
			key_group[4] = btn_creat(0, 290 + 32, 120, 50, 4, 4);
			key_group[5] = btn_creat(180, 290 + 32, 120, 50, 4, 4);

			key_group[0]->caption = "MOS管打开";
			key_group[1]->caption = "MOS管关闭";
			key_group[2]->caption = "电机正转";
			key_group[3]->caption = "电机反转";
			key_group[4]->caption = "BL_TOUCH测试";
			key_group[5]->caption = "蜂鸣器测试";

			for(i = 0; i < num; i++)
			{
				key_group[i]->font = 16;
				key_group[i]->bcfdcolor = GREEN;
				key_group[i]->bcfucolor = GREEN;
			}
			for(i = 0; i < num; i++)
			{
				btn_draw(key_group[i]);
			}
		}
	}
	//按键检测
	selx = screen_key_chk(key_group, &in_obj, num);
	if(selx & (1 << 6))
	{
		switch(selx & ~(3 << 6))
		{
			case 0:
				key_group[0]->bcfdcolor = RED;
				key_group[0]->bcfucolor = RED;
				key_group[1]->bcfdcolor = GREEN;
				key_group[1]->bcfucolor = GREEN;
				btn_draw(key_group[0]);
				btn_draw(key_group[1]);
				GPIO_SetBits(GPIOC, GPIO_Pin_8);
				GPIO_SetBits(GPIOD, GPIO_Pin_14 | GPIO_Pin_15);
				fanSpeed = 255;
				fanSpeed1 = 255;
				fanSpeed2 = 255;
				break;
			case 1:
				key_group[0]->bcfdcolor = GREEN;
				key_group[0]->bcfucolor = GREEN;
				key_group[1]->bcfdcolor = RED;
				key_group[1]->bcfucolor = RED;
				btn_draw(key_group[0]);
				btn_draw(key_group[1]);
				GPIO_ResetBits(GPIOC, GPIO_Pin_8);
				GPIO_ResetBits(GPIOD, GPIO_Pin_14 | GPIO_Pin_15);
				fanSpeed = 0;
				fanSpeed1 = 0;
				fanSpeed2 = 0;
				break;
			case 2:
				key_group[2]->bcfdcolor = RED;
				key_group[2]->bcfucolor = RED;
				key_group[3]->bcfdcolor = GREEN;
				key_group[3]->bcfucolor = GREEN;
				btn_draw(key_group[2]);
				btn_draw(key_group[3]);
				X_DIR_PIN = 0;
				Y_DIR_PIN = 0;
				Z_DIR_PIN = 0;
				E0_DIR_PIN = 0;
				E1_DIR_PIN = 0;
				E2_DIR_PIN = 0;
				X_ENABLE_PIN = 0;
				Y_ENABLE_PIN = 0;
				Z_ENABLE_PIN = 0;
				E0_ENABLE_PIN = 0;
				E1_ENABLE_PIN = 0;
				E2_ENABLE_PIN = 0;
				for(color = 0; color < 6400; color++)
				{
					X_STEP_PIN = 1;
					Y_STEP_PIN = 1;
					Z_STEP_PIN = 1;
					E0_STEP_PIN = 1;
					E1_STEP_PIN = 1;
					E2_STEP_PIN = 1;
					delay_us(100);
					X_STEP_PIN = 0;
					Y_STEP_PIN = 0;
					Z_STEP_PIN = 0;
					E0_STEP_PIN = 0;
					E1_STEP_PIN = 0;
					E2_STEP_PIN = 0;
					delay_us(100);
				}
				X_ENABLE_PIN = 1;
				Y_ENABLE_PIN = 1;
				Z_ENABLE_PIN = 1;
				E0_ENABLE_PIN = 1;
				E1_ENABLE_PIN = 1;
				E2_ENABLE_PIN = 1;
				break;
			case 3:
				key_group[2]->bcfdcolor = GREEN;
				key_group[2]->bcfucolor = GREEN;
				key_group[3]->bcfdcolor = RED;
				key_group[3]->bcfucolor = RED;
				btn_draw(key_group[2]);
				btn_draw(key_group[3]);
				X_DIR_PIN = 1;
				Y_DIR_PIN = 1;
				Z_DIR_PIN = 1;
				E0_DIR_PIN = 1;
				E1_DIR_PIN = 1;
				E2_DIR_PIN = 1;
				X_ENABLE_PIN = 0;
				Y_ENABLE_PIN = 0;
				Z_ENABLE_PIN = 0;
				E0_ENABLE_PIN = 0;
				E1_ENABLE_PIN = 0;
				E2_ENABLE_PIN = 0;
				for(color = 0; color < 6400; color++)
				{
					X_STEP_PIN = 1;
					Y_STEP_PIN = 1;
					Z_STEP_PIN = 1;
					E0_STEP_PIN = 1;
					E1_STEP_PIN = 1;
					E2_STEP_PIN = 1;
					delay_us(100);
					X_STEP_PIN = 0;
					Y_STEP_PIN = 0;
					Z_STEP_PIN = 0;
					E0_STEP_PIN = 0;
					E1_STEP_PIN = 0;
					E2_STEP_PIN = 0;
					delay_us(100);
				}
				X_ENABLE_PIN = 1;
				Y_ENABLE_PIN = 1;
				Z_ENABLE_PIN = 1;
				E0_ENABLE_PIN = 1;
				E1_ENABLE_PIN = 1;
				E2_ENABLE_PIN = 1;
				break;
			case 4:
				for(i = 0; i < 3; i++)
				{
					BL_TOUCH_PIN = 647;
					delay_ms(200);
					delay_ms(200);
					BL_TOUCH_PIN = 1473;
					delay_ms(200);
					delay_ms(200);
				}
				break;
			case 5:
				BEEP = 1;
				delay_second(2);
				BEEP = 0;
				break;

			default:
				break;
		}
	}
	if(display_next_update_millis < millis())	//刷新数据
	{
		display_next_update_millis  = millis() + 1000;
		gui_show_string("限位状态: X Y Z U V W ", 10, 10, 320, 16, 16, WHITE);
		StopStatus[0] = X_MIN_PIN;
		StopStatus[1] = Y_MIN_PIN;
		StopStatus[2] = Z_MIN_PIN;
		StopStatus[3] = STOP_U;
		StopStatus[4] = STOP_V;
		StopStatus[5] = STOP_W;
		for(i = 0; i < 6; i++)
		{
			if(StopStatus[i])
				color = RED;
			else
				color = GREEN;
			gui_draw_bigpoint(92 + i * 16, 35, color);
		}
		gui_fill_rectangle(10, 69, 64, 16, BACK2_DIY);
		(current_temperature[0] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[0]);
		gui_show_string(fatbuf, 10, 69, 64, 16, 16, color);

		gui_fill_rectangle(10 + 64, 69, 64, 16, BACK2_DIY);
		(current_temperature_bed > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature_bed);
		gui_show_string(fatbuf, 10 + 64, 69, 64, 16, 16, color);

		gui_fill_rectangle(10 + 64 * 2, 69, 64, 16, BACK2_DIY);
		(current_temperature[1] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[1]);
		gui_show_string(fatbuf, 10 + 64 * 2, 69, 64, 16, 16, color);

		gui_fill_rectangle(10 + 64 * 3, 69, 64, 16, BACK2_DIY);
		(current_temperature[2] > 5.0) ? (color = GREEN) : (color = RED);
		memset(fatbuf, 0, 10);
		sprintf(fatbuf, "%3.1f℃", current_temperature[2]);
		gui_show_string(fatbuf, 10 + 64 * 3, 69, 64, 16, 16, color);
	}
}


/*
Creator:张林波
name:error_diy

*/
void error_diy(void)
{
	static u8 old_error_data = 0;
	u8 error_num=0,old_error_num=0;
	u8 i,j;
	for(i=0;i<8;i++)
	{
		if(ErrorData.data & 1<<i)
			error_num++;
		if(old_error_data & 1<<i)
			old_error_num++;
	}
	if(ErrorData.data != old_error_data)
	{
		if(ErrorData.data)
			BeepSet(TRUE);
		else
			BeepSet(FALSE);
		LCD_Fill(260,10,310,26 + 20*old_error_num,BACK_DIY);
		old_error_data = ErrorData.data;
	}
	j=0;
	for(i=0;i<8;i++)
	{
		memset(fatbuf,0,10);
		if(ErrorData.data & 1<<i)
		{
			sprintf(fatbuf,"ERROR%1d",i+1);
			gui_show_strmid(260,10 + j*20,50,20,RED,16,fatbuf,0);
			j++;
		}
	}
}

void disable_print(void)
{
	disable_heater();

	disable_x();
	disable_y();
	disable_z();
	disable_e0();
	disable_e1();
	disable_e2();
}


int RecordCurrent_temperature1;
int RecordCurrent_temperature_bed;

uint8_t temperatureCheckStep = 0;
uint8_t BedCheckStep = 0;


uint8_t NozzleMaxErrorCounter = 0;
uint8_t NozzleMinErrorCounter = 0;
uint8_t BedMaxErrorCounter = 0;
uint8_t BedMinErrorCounter = 0;
uint8_t BedHeatOutControlErrorCounter = 0;
uint8_t NozzleHeatOutControlErrorCounter = 0;
void error_check(void)
{
	//ErrorData.data = 0;
	if(TargetTemperatureChangeFlag)
	{
		TargetTemperatureChangeFlag = 0;
		//热失控检测
		if(target_temperature[0] > 40)
		{
			if((current_temperature[0] - target_temperature[0]) > 20)
			{
				NozzleHeatOutControlErrorCounter++;
				if(NozzleHeatOutControlErrorCounter >= 10)
				{
					//printf("热失控\r\n");
					//ErrorData.ErrorFlag.HeatOutControl = 1;
				}
			}
			else
			{
				NozzleHeatOutControlErrorCounter = 0;
				ErrorData.ErrorFlag.HeatOutControl = 0;
			}
		}

		if(target_temperature_bed > 40)
		{
			//printf("检测热失控%d,%d,%d\r\n",((current_temperature[0]-target_temperature[0])>20),((current_temperature_bed-target_temperature_bed)>20),(((current_temperature[0]-target_temperature[0])>20)||((current_temperature_bed-target_temperature_bed)>20)));

			if((current_temperature_bed - target_temperature_bed) > 20)
			{
				BedHeatOutControlErrorCounter++;
				if(BedHeatOutControlErrorCounter >= 10)
				{
					//printf("热失控\r\n");
					//ErrorData.ErrorFlag.HeatOutControl = 1;
				}
			}
			else
			{
				BedHeatOutControlErrorCounter = 0;
				ErrorData.ErrorFlag.HeatOutControl = 0;
			}
		}
		//喷头不加热检测
		if(((target_temperature[0] - current_temperature[0]) > 15)&&(current_temperature[0]<200))
		{
			//printf("喷头目标温度改变\r\n");
			TargetTemperatureChangeFlag = 0;
			if(fabs(RecordCurrent_temperature1 - current_temperature[0]) < 3)
			{
				temperatureCheckStep++;
				if(temperatureCheckStep >= 30)
				{
					if(fanSpeed > 150)		//风扇速度减50直到低于150不行则报错
					{
						temperatureCheckStep = 0;
						fanSpeed = constrain(fanSpeed-50,0,200);
					}
					else
					{
						ErrorData.ErrorFlag.NozzleHeatFailure = 1;
						disable_print();
					}
					//target_temperature[0] = 0;		//停止加热
					
				}
			}
			else
			{
				temperatureCheckStep = 0;
				//ErrorData.ErrorFlag.NozzleHeatFailure = 0;
			}
			RecordCurrent_temperature1 = current_temperature[0];

		}
		else
		{
			temperatureCheckStep = 0;
			//ErrorData.ErrorFlag.NozzleHeatFailure = 0;
		}
		//热床不加热检测
		if(((target_temperature_bed - current_temperature_bed) > 13) && (current_temperature_bed < 50))
		{
			TargetBedChangeFlag = 0;
			//printf("热床目标温度改变\r\n");
			if(fabs(RecordCurrent_temperature_bed - current_temperature_bed) < 2)
			{
				BedCheckStep++;
				if(BedCheckStep > 80)
				{
					ErrorData.ErrorFlag.BedHeatFailure = 1;
					//target_temperature_bed = 0;			//停止底板加热
					disable_print();
				}
			}
			else
			{
				BedCheckStep = 0;
				//ErrorData.ErrorFlag.NozzleHeatFailure = 0;
			}
			RecordCurrent_temperature_bed = current_temperature_bed;
		}
		else
		{
			BedCheckStep = 0;
			//ErrorData.ErrorFlag.NozzleHeatFailure = 0;
		}

	}
	//喷头温度过低
	if(TEMP_0_PIN > TemperatureADMax)
	{
		NozzleMaxErrorCounter++;
		if(NozzleMaxErrorCounter >= 5)
		{
			ErrorData.ErrorFlag.NozzleTemperatureOverLow = 1;
			//printf("喷头温度过低\r\n");
			disable_print();
		}
	}
	else
	{
		NozzleMaxErrorCounter = 0;
		ErrorData.ErrorFlag.NozzleTemperatureOverLow = 0;
	}
	//热床温度过低
	if(TEMP_BED_PIN > TemperatureADMax)
	{
		BedMaxErrorCounter++;
		if(BedMaxErrorCounter >= 5)
		{
			ErrorData.ErrorFlag.BedTemperatureOverLow = 1;
			//printf("热床温度过低\r\n");
			disable_print();
		}
	}
	else
	{
		BedMaxErrorCounter = 0;
		ErrorData.ErrorFlag.BedTemperatureOverLow = 0;
	}
	//喷头温度过高
	if(TEMP_0_PIN < NozzleTemperatureADMin)
	{
		NozzleMinErrorCounter++;
		if(NozzleMinErrorCounter >= 50)
		{
			ErrorData.ErrorFlag.NozzleTemperatureOverHigh = 1;
			//printf("喷头温度过高\r\n");
			disable_print();
		}
	}
	else
	{
		NozzleMinErrorCounter = 0;
		ErrorData.ErrorFlag.NozzleTemperatureOverHigh = 0;
	}

	//热床温度过高
	if(TEMP_BED_PIN < BedTemperatureADMin)
	{
		BedMinErrorCounter++;
		if(BedMinErrorCounter >= 50)
		{
			ErrorData.ErrorFlag.BedTemperatureOverHigh = 1;
			//printf("热床温度过高\r\n");
			disable_print();
		}
	}
	else
	{
		BedMinErrorCounter = 0;
		ErrorData.ErrorFlag.BedTemperatureOverHigh = 0;
	}
	ErrorData.ErrorFlag.MotorOutRange = 0;
	error_diy();
}


