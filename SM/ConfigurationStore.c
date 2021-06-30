#include "sm_firmware.h"
#include "planner.h"
#include "temperature.h"
#include "ConfigurationStore.h"
#include "usart.h"
#include "24cxx.h"
#include "sm_plus.h"
#include <math.h>
#include "App_Language.h"

#define EEPROM_OFFSET_CFG 100	//配置文件偏移量

#define DEFAULT_VALUE 0xcc


void Config_PrintSettings()
{
	// Always have this function, even with EEPROM_SETTINGS disabled, the current values will be shown
	SERIAL_ECHO_START;
	printf("Steps per unit:");
	SERIAL_ECHO_START;
	printf("  M92 X%f", axis_steps_per_unit[0]);
	printf(" Y%f", axis_steps_per_unit[1]);
	printf(" Z%f", axis_steps_per_unit[2]);
	printf(" E%f\r\n", axis_steps_per_unit[3]);

	printf("INVERT_X_DIR:%d\r\n", INVERT_X_DIR);
	printf("INVERT_Y_DIR:%d\r\n", INVERT_Y_DIR);
	printf("INVERT_Z_DIR:%d\r\n", INVERT_Z_DIR);
	printf("INVERT_E0_DIR:%d\r\n", INVERT_E0_DIR);
	SERIAL_ECHO_START;
	printf("Maximum feedrates (mm/s):\r\n");
	SERIAL_ECHO_START;
	printf("  M203 X%f", max_feedrate[0]);
	printf(" Y%f", max_feedrate[1]);
	printf(" Z%f", max_feedrate[2]);
	printf(" E%f\r\n", max_feedrate[3]);


	SERIAL_ECHO_START;
	printf("Maximum Acceleration (mm/s2):\r\n");
	SERIAL_ECHO_START;
	printf("  M201 X%ld", max_acceleration_units_per_sq_second[0]);
	printf(" Y%ld", max_acceleration_units_per_sq_second[1]);
	printf(" Z%ld", max_acceleration_units_per_sq_second[2]);
	printf(" E%ld\r\n", max_acceleration_units_per_sq_second[3]);
	//  SERIAL_ECHOLN("");
	SERIAL_ECHO_START;
	printf("Acceleration: S=acceleration, T=retract acceleration\r\n");
	SERIAL_ECHO_START;
	printf("  M204 S%f", acceleration);
	printf(" T%f\r\n", retract_acceleration);
	///   SERIAL_ECHOLN("");

	SERIAL_ECHO_START;
	printf("Advanced variables: S=Min feedrate (mm/s), T=Min travel feedrate (mm/s), B=minimum segment time (ms), X=maximum XY jerk (mm/s),  Z=maximum Z jerk (mm/s),  E=maximum E jerk (mm/s)\r\n");
	SERIAL_ECHO_START;
	printf("  M205 S%f", minimumfeedrate);
	printf(" T%f", mintravelfeedrate);
	printf(" B%ld", minsegmenttime);
	printf(" X%f", max_xy_jerk);
	printf(" Z%f", max_z_jerk);
	printf(" E%f\r\n", max_e_jerk);
	// SERIAL_ECHOLN("");

	SERIAL_ECHO_START;
	printf("Home offset (mm):\r\n");
	SERIAL_ECHO_START;
	printf("  M206 X%f", add_homeing[0]);
	printf(" Y%f", add_homeing[1]);
	printf(" Z%f", add_homeing[2]);
	//  SERIAL_ECHOLN("");
	#ifdef PIDTEMP
	SERIAL_ECHO_START;
	printf("PID settings:\r\n");
	SERIAL_ECHO_START;
	printf("   M301 P%f", Kp);
	printf(" I%f", unscalePID_i(Ki));
	printf(" D%f\r\n", unscalePID_d(Kd));
	//  SERIAL_ECHOLN("");
	#endif
}
#ifdef EEPROM_SETTINGS

/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
void Config_StoreSettings(void)
{
	u16 i = EEPROM_OFFSET_CFG;	//eeprom 初始偏移量
	u16 wlen;	//写入长度
	u8 temp = DEFAULT_VALUE;
	wlen = sizeof(temp);
	AT24CXX_Write(i, (u8 *)&temp, wlen);
	i += wlen;
	wlen = sizeof(axis_steps_per_sqr_second);
	AT24CXX_Write(i, (u8 *)&axis_steps_per_unit, wlen);
	i += wlen;
	wlen = sizeof(max_feedrate);
	AT24CXX_Write(i, (u8 *)&max_feedrate, wlen);
	i += wlen;
	wlen = sizeof(max_acceleration_units_per_sq_second);
	AT24CXX_Write(i, (u8 *)&max_acceleration_units_per_sq_second, wlen);
	i += wlen;
	wlen = sizeof(acceleration);
	AT24CXX_Write(i, (u8 *)&acceleration, wlen);
	i += wlen;
	wlen = sizeof(retract_acceleration);
	AT24CXX_Write(i, (u8 *)&retract_acceleration, wlen);
	i += wlen;
	wlen = sizeof(minimumfeedrate);
	AT24CXX_Write(i, (u8 *)&minimumfeedrate, wlen);
	i += wlen;
	wlen = sizeof(minsegmenttime);
	AT24CXX_Write(i, (u8 *)&minsegmenttime, wlen);
	i += wlen;
	wlen = sizeof(max_xy_jerk);
	AT24CXX_Write(i, (u8 *)&max_xy_jerk, wlen);
	i += wlen;
	wlen = sizeof(max_z_jerk);
	AT24CXX_Write(i, (u8 *)&max_z_jerk, wlen);
	i += wlen;
	wlen = sizeof(max_e_jerk);
	AT24CXX_Write(i, (u8 *)&max_e_jerk, wlen);
	i += wlen;

	#ifdef PIDTEMP
	wlen = sizeof(Kp);
	AT24CXX_Write(i, (u8 *)&Kp, wlen);
	i += wlen;
	wlen = sizeof(Ki);
	AT24CXX_Write(i, (u8 *)&Ki, wlen);
	i += wlen;
	wlen = sizeof(Kd);
	AT24CXX_Write(i, (u8 *)&Kd, wlen);
	i += wlen;
	#endif

	wlen = sizeof(INVERT_X_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_X_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_Y_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_Y_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_Z_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_Z_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E0_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_E0_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E1_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_E1_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E2_DIR);
	AT24CXX_Write(i, (u8 *)&INVERT_E2_DIR, wlen);
	i += wlen;

	wlen = sizeof(X_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&X_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(Y_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&Y_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(Z_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&Z_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(U_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&U_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(V_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&V_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(W_ENDSTOPS_INVERTING);
	AT24CXX_Write(i, (u8 *)&W_ENDSTOPS_INVERTING, wlen);
	//机器尺寸
	i += wlen;
	wlen = sizeof(X_MAX_POS);
	AT24CXX_Write(i, (u8 *)&X_MAX_POS, wlen);
	i += wlen;
	wlen = sizeof(Y_MAX_POS);
	AT24CXX_Write(i, (u8 *)&Y_MAX_POS, wlen);
	i += wlen;
	wlen = sizeof(Z_MAX_POS);
	AT24CXX_Write(i, (u8 *)&Z_MAX_POS, wlen);
	i += wlen;
	wlen = sizeof(ParaVersion);
	AT24CXX_Write(i, (u8 *)&ParaVersion, wlen);
	i += wlen;
	wlen = sizeof(LevelMode);
	AT24CXX_Write(i, (u8 *)&LevelMode, wlen);
	i += wlen;
	wlen = sizeof(EXTRUDER_NUM);
	AT24CXX_Write(i, (u8 *)&EXTRUDER_NUM, wlen);
	i += wlen;
	#ifdef FWRETRACT
	wlen = sizeof(retract_feedrate);
	AT24CXX_Write(i, (u8 *)&retract_feedrate, wlen);
	i += wlen;
	wlen = sizeof(retract_zlift);
	AT24CXX_Write(i, (u8 *)&retract_zlift, wlen);
	i += wlen;
	wlen = sizeof(retract_recover_length);
	AT24CXX_Write(i, (u8 *)&retract_recover_length, wlen);
	i += wlen;
	#endif
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:读取IIC 配置文件到系统
*/
void Config_RetrieveSettings()
{
	u16 i = EEPROM_OFFSET_CFG;	//eeprom 初始偏移量
	u16 wlen;	//写入长度
	u8 temp;

	#ifdef __USR_SYS_CONFIG_SPEED
	uint8_t num;
	//	float tmp1[] = DEFAULT_AXIS_STEPS_PER_UNIT;
	float tmp2[] = DEFAULT_MAX_FEEDRATE;
	float tmp3[] = DEFAULT_MAX_ACCELERATION;
	#endif

	USR_UsrLog("Start Init Print Paragramed Config...");
	wlen = sizeof(temp);
	AT24CXX_Read(i, (u8 *)&temp, wlen);
	i += wlen;
	if(temp != DEFAULT_VALUE)	//读到eeprom里没有存储默认的配置值 存储默认值到eeprom中
	{
		USR_UsrLog("***********Config_StoreSettings**************");
		Config_StoreSettings();
		return;
	}
	wlen = sizeof(axis_steps_per_sqr_second);
	AT24CXX_Read(i, (u8 *)&axis_steps_per_unit, wlen);
	i += wlen;

	wlen = sizeof(max_feedrate);
	AT24CXX_Read(i, (u8 *)&max_feedrate, wlen);
	i += wlen;
	wlen = sizeof(max_acceleration_units_per_sq_second);
	AT24CXX_Read(i, (u8 *)&max_acceleration_units_per_sq_second, wlen);
	i += wlen;
	wlen = sizeof(acceleration);
	AT24CXX_Read(i, (u8 *)&acceleration, wlen);
	i += wlen;
	wlen = sizeof(retract_acceleration);
	AT24CXX_Read(i, (u8 *)&retract_acceleration, wlen);
	i += wlen;
	wlen = sizeof(minimumfeedrate);
	AT24CXX_Read(i, (u8 *)&minimumfeedrate, wlen);
	i += wlen;
	wlen = sizeof(minsegmenttime);
	AT24CXX_Read(i, (u8 *)&minsegmenttime, wlen);
	i += wlen;
	wlen = sizeof(max_xy_jerk);
	AT24CXX_Read(i, (u8 *)&max_xy_jerk, wlen);
	i += wlen;
	wlen = sizeof(max_z_jerk);
	AT24CXX_Read(i, (u8 *)&max_z_jerk, wlen);
	i += wlen;
	wlen = sizeof(max_e_jerk);
	AT24CXX_Read(i, (u8 *)&max_e_jerk, wlen);
	i += wlen;

	/*Modified by Kim 210191217*/
	#ifdef __USR_SYS_CONFIG_SPEED
	for(num = 0; num < 4; num++)
	{
		//axis_steps_per_unit[num] = tmp1[num];
		max_feedrate[num] = tmp2[num];
		max_acceleration_units_per_sq_second[num] = tmp3[num];
	}

	acceleration = DEFAULT_ACCELERATION;
	retract_acceleration = DEFAULT_RETRACT_ACCELERATION;
	minimumfeedrate = DEFAULT_MINIMUMFEEDRATE;
	minsegmenttime = DEFAULT_MINSEGMENTTIME;
	mintravelfeedrate = DEFAULT_MINTRAVELFEEDRATE;
	max_xy_jerk = DEFAULT_XYJERK;
	max_z_jerk = DEFAULT_ZJERK;
	max_e_jerk = DEFAULT_EJERK;
	#endif
	//End Modified by Kim 20191217



	#ifdef PIDTEMP
	wlen = sizeof(Kp);
	AT24CXX_Read(i, (u8 *)&Kp, wlen);
	i += wlen;
	wlen = sizeof(Ki);
	AT24CXX_Read(i, (u8 *)&Ki, wlen);
	i += wlen;
	wlen = sizeof(Kd);
	AT24CXX_Read(i, (u8 *)&Kd, wlen);
	i += wlen;
	#endif
	//===================================================
	wlen = sizeof(INVERT_X_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_X_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_Y_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_Y_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_Z_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_Z_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E0_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_E0_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E1_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_E1_DIR, wlen);
	i += wlen;
	wlen = sizeof(INVERT_E2_DIR);
	AT24CXX_Read(i, (u8 *)&INVERT_E2_DIR, wlen);

	//=============================================================
	i += wlen;
	wlen = sizeof(X_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&X_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(Y_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&Y_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(Z_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&Z_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(U_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&U_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(V_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&V_ENDSTOPS_INVERTING, wlen);
	i += wlen;
	wlen = sizeof(W_ENDSTOPS_INVERTING);
	AT24CXX_Read(i, (u8 *)&W_ENDSTOPS_INVERTING, wlen);

	/*读取配置文件中存储的打印范围*/
	i += wlen;
	wlen = sizeof(X_MAX_POS);
	AT24CXX_Read(i, (u8 *)&X_MAX_POS, wlen);				//读取X打印范围
	i += wlen;
	wlen = sizeof(Y_MAX_POS);
	AT24CXX_Read(i, (u8 *)&Y_MAX_POS, wlen);				//读取Y打印范围
	i += wlen;
	wlen = sizeof(Z_MAX_POS);
	AT24CXX_Read(i, (u8 *)&Z_MAX_POS, wlen);				//读取Z打印范围
	i += wlen;
	wlen = sizeof(ParaVersion);
	AT24CXX_Read(i, (u8 *)&ParaVersion, wlen);			//读取配置文件版本号
	//======================================================
	i += wlen;
	wlen = sizeof(LevelMode);
	AT24CXX_Read(i, (u8 *)&LevelMode, wlen);			//读取配置文件版本号
	i += wlen;
	wlen = sizeof(EXTRUDER_NUM);
	AT24CXX_Read(i, (u8 *)&EXTRUDER_NUM, wlen);		//读取挤出头数量
	//======================================================
	i += wlen;
	#ifdef FWRETRACT
	wlen = sizeof(retract_feedrate);
	AT24CXX_Read(i, (u8 *)&retract_feedrate, wlen);
	i += wlen;
	wlen = sizeof(retract_zlift);
	AT24CXX_Read(i, (u8 *)&retract_zlift, wlen);
	i += wlen;
	wlen = sizeof(retract_recover_length);
	AT24CXX_Read(i, (u8 *)&retract_recover_length, wlen);
	i += wlen;
	#endif
	#ifdef MESH_BED_LEVELING
	wlen = sizeof(BL_TOUCH_OFFSET);
	AT24CXX_Read(MBL_OFFSET, (u8 *)&BL_TOUCH_OFFSET, wlen);
	if(fabs(BL_TOUCH_OFFSET) > 10.f)
	{
		BL_TOUCH_OFFSET = 1.39;
		wlen = sizeof(BL_TOUCH_OFFSET);
		AT24CXX_Write(i, (u8 *)&BL_TOUCH_OFFSET, wlen);
	}

	wlen = sizeof(matrix_offset);
	AT24CXX_Read(MBL_VALUE, (u8 *)&matrix_offset, wlen);
	for(i = 0; i < 9; i++)
	{
		//mbl_active = true;
		if(matrix_offset[i / 3][i % 3] > 5.f || matrix_offset[i / 3][i % 3] < -5.f)
		{
			mbl_active = false;
			break;
		}
	}
	#endif
	AT24CXX_Read(E_LANG, (u8 *)&text_display.language_choice, sizeof(text_display.language_choice));
	AT24CXX_Read(E_BEEP, (u8 *)&BeepSwitch, sizeof(BeepSwitch));
	if((BeepSwitch & 0xfe) != 0)
	{
		BeepSwitch = 1;	//默认打开
		AT24CXX_Write(E_BEEP, (u8 *)&BeepSwitch, sizeof(BeepSwitch));
	}
	USR_UsrLog("Start Init Print Paragramed Completed...");
}

#endif

