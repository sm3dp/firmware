#ifndef __CHILDREN_MENU_H__
#define __CHILDREN_MENU_H__

#include "touch.h"
#include "guix.h"
#include "gui.h"
#include "piclib.h"
#include "temperature.h"
#include "gcodeplayer.h"
#include "button.h"
#include "filelistbox.h"
#include "gcodeplayer.h"
#include "stepper.h"

//#define CHINESE 0   /*  界面中文显示      */
//#define ENGLISH 1   /*  界面英文显示      */
typedef void (*menuFunc_t)();
typedef struct{
u8 wifi_stat;	//wifi状态 0正常 1连接但无互联网 2未连接 3未绑定手机
u8 door_stat;	//打印机舱门状态 0关闭 1打开
//	u8 filament_stat;	//低三位代表三个料盘状态 0正常 1缺料
//	u8 print_present;	//打印百分比
//	u8 print_stat;		//打印状态 0未打印 1打印中 2打印完成
//	u32 print_time;		//打印时间
u8 model_stat;		//打印模型状态 0底板上没检测到有模型存在 1有模型存在
}CHILD_DATA;
extern CHILD_DATA ChildData;
extern bool windows_flag;
extern menuFunc_t CurrentMenu;
extern menuFunc_t tempMenu;
extern bool redraw_menu;
extern u8 PrinterMode;
extern float speed_temprature_factor;

extern volatile uint8_t MotorOutRangeFlag;
extern volatile uint8_t CheckMotorOutRangeFlag;

#define BACK_DIY 		 0x1210	//DIY打印机背景色	 
#define BACK2_DIY		0x3b16	//按钮背景色

#define MAX_GCODE_NUM 100
//文件类型
typedef enum{
    GCODE = 0,
    FOLDER,
}file_type_enum;
//可用文件的结构体 gcode 和 文件夹
typedef struct {
    file_type_enum file_type;//true Gcode  false 文件夹
	u16  file_index;//文件索引
}gcodefile_s;
extern gcodefile_s gcodefile_t[6];
typedef enum
{
	Chinese = 0,
	English,
	German,
	French,
	Spanish,
	Portuguese,
	Japanese,
	Russian,
	Italian,
}LANGUAGE_SELECT;
typedef enum
{
	FIRST_ENTRY = 0,	//第一次
	PAGE_DOWN,			//下一页
	PAGE_UP,			//上一页
	LAST_FLODER,		//上一级菜单
}GCODELIST_MODE;
typedef struct
{
	u8 *L_Print;
	u8 *L_Ctol;
	u8 *L_Set;
	u8 *L_Preheat;
	u8 *L_Move;
	u8 *L_Extrusion;
	u8 *L_Fan;
	u8 *L_About;
	u8 *L_Language;
	u8 *L_Status;
	u8 *L_PrintName;
	u8 *L_Pause;
	u8 *L_Pursue;
	u8 *L_Stop;
	u8 *L_Tempertuare;
	u8 *L_Speed;
	u8 *L_PrintCtrl;
	u8 *L_PrintSpeed;
	u8 *L_SDPrint;
	u8 *L_Back;
	u8 *L_PrintFinish;
	u8 *L_Confirm;
	u8 *L_Cancel;
	u8 *L_LaserCtol;
	u8 *L_LaserMove;
	u8 *L_Zero;
	u8 *L_Ctrl;
	u8 *L_filament;			//缺料文字提醒
	u8 *L_StopPrint;
	u8 *L_Leveling;
	u8 *L_Adjust;
	u8 *L_Load;
	u8 *L_Unload;
	u8 *L_Fast;
	u8 *L_Normal;
	u8 *L_Slow;
	u8 *L_autoleveling;
	u8 *L_offset;
	u8 *L_Continue;
	u8 language_choice;		//语言选择 0中文 1英文

}TextDisplay;
extern TextDisplay text_display;
extern u8 gcodefile_index_base;
extern u8 gcodefile_index_sub;
#define EPSINON 0.00001
//#define FAN3_SHOW //风扇3显示打开
void lcd_update(void);
void start_menu(void);
void main_menu(void);
void set_menu(void);
void wifi_menu(void);
void filamentstep1_menu(void);
void filamentstep2_menu(void);
void filamentstep3_menu(void);
void filamentlaststep_menu(void);
void language_menu(void);
void about_menu(void);
void adjscreen_menu(void);
void print_menu(void);
void filament_error(void);
void door_error(void);
void extruder_cool(void);
void printcomplete_menu(void);
void wifi2_menu(void);
void progress_bar(u16 sx,u16 sy,u8 progress);
void model_check(void);
void popout_screen(void);
void stop_print(void);
void QRSelect_menu(void);
void reprint_menu(void);

void reprint_menu(void);


void start_diy(void);
void main_diy(void);
void set_diy(void);
void control_3d_diy(void);
void control_laser_diy(void);
void preheat_diy(void);
void move1_diy(void);
void move2_diy(void);
void zero_diy(void);
void extruder_diy(void);
void leveling_diy(void);
void fan_diy(void);
void about_diy(void);
void language_diy(void);
void status_diy(void);
void wifi_diy(void);
void print_contrl_diy(void);
void speed_diy(void);
void gecodelist_diy(void);
void getgcodetab_diy(void);
void print_diy(void);
void stop_diy(void);
void demo_diy(void);
void manual_stop_diy(void);
void extrudemultiply_diy(void);
void stop_laser_diy(void);
u8 check_sd_allfile(u8 *fp);
void readindex_file(u8 file_index_,u8* file_name,u8 *fp);
void set_display_language(void);
void filament_run_out_diy(void);
void GetGcodeTab2_diy(void);
void AutoLeveling_diy(void);
void AutoLevelingZMove_diy(void);
void print_filament(void);
void main_test(void);
void error_diy(void);
void error_check(void);
#endif

