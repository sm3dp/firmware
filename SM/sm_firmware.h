// Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.
// Licence: GPL

#ifndef _SM_FIRMWARE_H__
#define _SM_FIRMWARE_H__

//#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys.h"
#include "ConfigurationStore.h"


//#include "fastio.h"
#include "Configuration.h"
#include "pins.h"

#define OK_KEY (Get_Adc_Average(ADC_Channel_15,10)>3250)

void get_command(void);
void get_command2(void);

void process_commands(void);
void manage_inactivity(void);

//#define  enable_x()  X_ENABLE_PIN=X_ENABLE_ON
//#define disable_x()	 X_ENABLE_PIN=!X_ENABLE_ON

#define  enable_x()  X_EN(0)
#define disable_x()	 X_EN(1)

//#define  enable_y()  Y_ENABLE_PIN=Y_ENABLE_ON
//#define disable_y()	 Y_ENABLE_PIN=!Y_ENABLE_ON
#define  enable_y()  Y_EN(0)
#define disable_y()	 Y_EN(1)

//#define  enable_z()  Z_ENABLE_PIN=Z_ENABLE_ON
//#define disable_z()	 Z_ENABLE_PIN=!Z_ENABLE_ON
#define  enable_z()  Z_EN(0)
#define disable_z()	 Z_EN(1)

#define  enable_e0() E0_ENABLE_PIN=E_ENABLE_ON
#define disable_e0() E0_ENABLE_PIN=!E_ENABLE_ON

#define  enable_e1() E1_ENABLE_PIN=E_ENABLE_ON
#define disable_e1() E1_ENABLE_PIN=!E_ENABLE_ON

#define  enable_e2() E2_ENABLE_PIN=E_ENABLE_ON
#define disable_e2() E2_ENABLE_PIN=!E_ENABLE_ON


enum AxisEnum {X_AXIS=0, Y_AXIS=1, Z_AXIS=2, E_AXIS=3,X_HEAD=4,Y_HEAD=5};

#define SERIAL_ERROR_START	printf("Error:")
#define SERIAL_ECHO_START	printf("echo:")
#define PRINTFLINE			printf("line=%d",__LINE__)
#define PRINTFFILE			printf("%s",__FILE__)

void loop(void);
void setup(void);
void FlushSerialRequestResend(void);
void ClearToSend(void);

void get_coordinates(void);
void prepare_move(void);
void kill(void);
void Stop(void);

bool IsStopped(void);

void enquecommand(const char *cmd); //put an ascii command at the end of the current buffer.
//void enquecommand_P(const char *cmd); //put an ascii command at the end of the current buffer, read from flash
void prepare_arc_move(u8 isclockwise);

void clamp_to_software_endstops(float target[3]);

typedef struct
{
	char cmdbuffer[BUFSIZE][MAX_CMD_SIZE];	//gcode命令缓冲
	char type[BUFSIZE];						//gcode命令来源类型 0：串口1 sd卡 1：串口2
}CMDBUFF;

#define CRITICAL_SECTION_START  __NOP();//__disable_irq();
#define CRITICAL_SECTION_END    __NOP();//__enable_irq();
extern float homing_feedrate[];
extern bool axis_relative_modes[];
extern int feedmultiply;
extern int extrudemultiply; // Sets extrude multiply factor (in percent)
extern float current_position[NUM_AXIS];
extern volatile float destination[NUM_AXIS];

extern float add_homeing[3];
extern float min_pos[3];
extern float max_pos[3];
extern int fanSpeed;
extern int fanSpeed1;
extern int fanSpeed2;


#ifdef BARICUDA
extern int ValvePressure;
extern int EtoPPressure;
#endif

#ifdef FWRETRACT
extern bool autoretract_enabled;
extern bool retracted;
extern float retract_length, retract_feedrate, retract_zlift;
extern float retract_recover_length, retract_recover_feedrate;
#endif

extern unsigned long starttime;
extern unsigned long stoptime;
extern u8 SSID[40];

// Handling multiple extruders pins
extern uint8_t active_extruder;


extern int buflen;
extern int bufindr;
extern int bufindw;
extern CMDBUFF CmdBuff;





#ifdef MESH_BED_LEVELING

extern u8 mbl_active;
extern u8 mbl_flg;
extern float matrix_offset[3][3];
extern float BL_TOUCH_OFFSET;

float mbl_get_z(float x,float y);


#endif
#ifdef LASER
extern u8 LaserSwitch;	//激光头开关标志 0 关 1开
#endif


extern bool pictr_flag,bmp_flag;
extern u16 pic_size;
extern float TempPosition[2];	//xy轴坐标缓存


#endif
