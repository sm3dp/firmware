/*
*Function:
*Programed by:
*Complete date:
*Modified by:
*Modified date:
*Version:
*Remarks:
*/
#include "temperature.h"
#include "sm_firmware.h"
//#include "ultralcd.h"
#include "delay.h"
#include "watchdog.h"
#include "usart.h"
#include "planner.h"
#include "thermistortables.h"
#include "lcd_menu.h"
#include "sm_plus.h"

#include "bsp.h"

#include "App_ADC.h"
#include "App_Language.h"
#include <math.h>

//===========================================================================
//=============================public variables============================
//===========================================================================
volatile int target_temperature[EXTRUDERS] = { 0 };//EXTRUDERS
volatile int target_temperature_bed = 0;
int current_temperature_raw[EXTRUDERS] = { 0 };
volatile float current_temperature[EXTRUDERS] = { 0 };
int current_temperature_bed_raw = 0;
volatile float current_temperature_bed = 0;

#ifdef PIDTEMP
	float Kp = DEFAULT_Kp;
	float Ki = (DEFAULT_Ki *PID_dT);
	float Kd = (DEFAULT_Kd / PID_dT);
	#ifdef PID_ADD_EXTRUSION_RATE
		float Kc = DEFAULT_Kc;
	#endif
#endif //PIDTEMP

#ifdef PIDTEMPBED
	float bedKp = DEFAULT_bedKp;
	float bedKi = (DEFAULT_bedKi *PID_dT);
	float bedKd = (DEFAULT_bedKd / PID_dT);
#endif //PIDTEMPBED


//===========================================================================
//=============================private variables============================
//===========================================================================
static volatile bool temp_meas_ready = false;

#ifdef PIDTEMP
//static cannot be external:
static float temp_iState[EXTRUDERS] = { 0 };
static float temp_dState[EXTRUDERS] = { 0 };
static float pTerm[EXTRUDERS];
static float iTerm[EXTRUDERS];
static float dTerm[EXTRUDERS];
//int output;
static float pid_error[EXTRUDERS];
static float temp_iState_min[EXTRUDERS];
static float temp_iState_max[EXTRUDERS];
// static float pid_input[EXTRUDERS];
// static float pid_output[EXTRUDERS];
static bool pid_reset[EXTRUDERS];
#endif //PIDTEMP
#ifdef PIDTEMPBED
//static cannot be external:
static float temp_iState_bed = { 0 };
static float temp_dState_bed = { 0 };
static float pTerm_bed;
static float iTerm_bed;
static float dTerm_bed;
//int output;
static float pid_error_bed;
static float temp_iState_min_bed;
static float temp_iState_max_bed;
#else //PIDTEMPBED
static unsigned long  previous_millis_bed_heater;
#endif //PIDTEMPBED
static unsigned char soft_pwm[EXTRUDERS];
static unsigned char soft_pwm_bed;

#if (defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1) || \
	(defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1) || \
	(defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1)
	static unsigned long extruder_autofan_last_check;
#endif

#if EXTRUDERS > 3
	# error Unsupported number of extruders
#elif EXTRUDERS > 2
	#define ARRAY_BY_EXTRUDERS(v1, v2, v3) { v1, v2, v3 }
#elif EXTRUDERS > 1
	#define ARRAY_BY_EXTRUDERS(v1, v2, v3) { v1, v2 }
#else
	#define ARRAY_BY_EXTRUDERS(v1, v2, v3) { v1 }
#endif

// Init min and max temp with extreme values to prevent false errors during startup
static int minttemp_raw[EXTRUDERS] = ARRAY_BY_EXTRUDERS(HEATER_0_RAW_LO_TEMP, HEATER_1_RAW_LO_TEMP, HEATER_2_RAW_LO_TEMP);
static int maxttemp_raw[EXTRUDERS] = ARRAY_BY_EXTRUDERS(HEATER_0_RAW_HI_TEMP, HEATER_1_RAW_HI_TEMP, HEATER_2_RAW_HI_TEMP);
static int minttemp[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0, 0, 0);
static int maxttemp[EXTRUDERS] = ARRAY_BY_EXTRUDERS(16383, 16383, 16383);
//static int bed_minttemp_raw = HEATER_BED_RAW_LO_TEMP; /* No bed mintemp error implemented?!? */
#ifdef BED_MAXTEMP
	static int bed_maxttemp_raw = HEATER_BED_RAW_HI_TEMP;
#endif
static void *heater_ttbl_map[EXTRUDERS] = ARRAY_BY_EXTRUDERS((void *)HEATER_0_TEMPTABLE, (void *)HEATER_1_TEMPTABLE, (void *)HEATER_2_TEMPTABLE);
static uint8_t heater_ttbllen_map[EXTRUDERS] = ARRAY_BY_EXTRUDERS(HEATER_0_TEMPTABLE_LEN, HEATER_1_TEMPTABLE_LEN, HEATER_2_TEMPTABLE_LEN);

static float analog2temp(int raw, uint8_t e);
static float analog2tempBed(int raw);
static void updateTemperaturesFromRawValues(void);

#ifdef WATCH_TEMP_PERIOD
	int watch_start_temp[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0, 0, 0);
	unsigned long watchmillis[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0, 0, 0);
#endif //WATCH_TEMP_PERIOD


//===========================================================================
//=============================   functions      ============================
//===========================================================================

void PID_autotune(float temp, int extruder, int ncycles)
{
	float input = 0.0;
	int cycles = 0;
	bool heating = true;

	unsigned long temp_millis = millis();
	unsigned long t1 = temp_millis;
	unsigned long t2 = temp_millis;
	long t_high = 0;
	long t_low = 0;

	long bias, d;
	float Ku, Tu;
	float Kp;//, Ki, Kd;
	float max = 0, min = 10000;

	if(extruder > EXTRUDERS)
	{
		printf("PID Autotune failed. Bad extruder number.");
		return;
	}
	printf("PID Autotune start");

	disable_heater(); // switch off all heaters.

	if(extruder < 0)
	{
		soft_pwm_bed = MAX_BED_POWER;
		bias = d = MAX_BED_POWER;
	}
	else
	{
		soft_pwm[extruder] = PID_MAX;
		bias = d = PID_MAX;
	}




	for(;;)
	{
		if(temp_meas_ready == true)
		{
			// temp sample ready
			updateTemperaturesFromRawValues();

			input = (extruder < 0) ? current_temperature_bed : current_temperature[extruder];

			max = max(max, input);
			min = min(min, input);

			if(heating == true && input > temp)
			{
				if(millis() - t2 > 5000)
				{
					heating = false;
					if(extruder < 0)
					{
						soft_pwm_bed = (bias - d) >> 1;
					}
					else
					{
						soft_pwm[extruder] = (bias - d) >> 1;
					}
					t1 = millis();
					t_high = t1 - t2;
					max = temp;
				}
			}
			if(heating == false && input < temp)
			{
				if(millis() - t1 > 5000)
				{
					heating = true;
					t2 = millis();
					t_low = t2 - t1;
					if(cycles > 0)
					{
						bias += (d * (t_high - t_low)) / (t_low + t_high);
						bias = constrain(bias, 20, (extruder < 0 ? (MAX_BED_POWER) : (PID_MAX)) - 20);
						if(bias > (extruder < 0 ? (MAX_BED_POWER) : (PID_MAX))) d = (extruder < 0 ? (MAX_BED_POWER) : (PID_MAX)) - 1 - bias;
						else d = bias;

						USR_UsrLog(" bias: %ld", bias);
						USR_UsrLog(" d: %ld", d);
						USR_UsrLog(" min: %f", min);
						USR_UsrLog(" max: %f", max);
						if(cycles > 2)
						{
							Ku = (4.0 * d) / (3.14159 * (max - min) / 2.0);
							Tu = ((float)(t_low + t_high) / 1000.0);
							USR_UsrLog(" Ku: %f", Ku);
							USR_UsrLog(" Tu: %f", Tu);
							Kp = 0.6 * Ku;
							Ki = 2 * Kp / Tu;
							Kd = Kp * Tu / 8;
							USR_UsrLog(" Clasic PID ");
							USR_UsrLog(" Kp: %f", Kp);
							USR_UsrLog(" Ki: %f", Ki);
							USR_UsrLog(" Kd: %f", Kd);
						}
					}
					if(extruder < 0)
						soft_pwm_bed = (bias + d) >> 1;
					else
						soft_pwm[extruder] = (bias + d) >> 1;
					cycles++;
					min = temp;
				}
			}
		}
		if(input > (temp + 20))
		{
			USR_UsrLog("PID Autotune failed! Temperature to high");
			return;
		}
		if(millis() - temp_millis > 2000)
		{
			int p;
			if(extruder < 0)
			{
				p = soft_pwm_bed;
				USR_UsrLog("ok B:");
			}
			else
			{
				p = soft_pwm[extruder];
				USR_UsrLog("ok T:");
			}

			printf("%f @:%d", input, p);
			temp_millis = millis();
		}
		if(((millis() - t1) + (millis() - t2)) > (10L * 60L * 1000L * 2L))
		{
			USR_UsrLog("PID Autotune failed! timeout");
			return;
		}
		if(cycles > ncycles)
		{
			USR_UsrLog("PID Autotune finished ! Place the Kp, Ki and Kd constants in the configuration.h");
			return;
		}
		lcd_update();
	}
}

void updatePID(void)
{
	#ifdef PIDTEMP
	int e;
	for(e = 0; e < EXTRUDERS; e++)
	{
		temp_iState_max[e] = PID_INTEGRAL_DRIVE_MAX / Ki;
	}
	#endif
	#ifdef PIDTEMPBED
	temp_iState_max_bed = PID_INTEGRAL_DRIVE_MAX / bedKi;
	#endif
}

int getHeaterPower(int heater)
{
	if(heater < 0)
		return soft_pwm_bed;
	return soft_pwm[heater];
}
u8 pid_offset = 31;
void manage_heater(void)
{
	float pid_input;
	float pid_output;
	float diff;
	int e;
	int pid_min;
	if(temp_meas_ready != true)   //better readability
		return;
	updateTemperaturesFromRawValues();



	//激光打印添加
	u32 tim_cnt = 0;

	tim_cnt = TIM2->CNT;
	if(tim_cnt > TIM2->ARR)
	{
		TIM2->CNT = 0;
	}

	for(e = 0; e < EXTRUDERS; e++)
	{
		#ifdef PIDTEMP
		pid_input = current_temperature[e];

		#ifndef PID_OPENLOOP
		pid_error[e] = target_temperature[e] - pid_input;
		if(pid_error[e] > PID_FUNCTIONAL_RANGE)
		{
			pid_output = BANG_MAX;
			pid_reset[e] = true;
		}
		else if(pid_error[e] < -PID_FUNCTIONAL_RANGE || target_temperature[e] == 0)
		{
			pid_output = 0;
			pid_reset[e] = true;
		}
		else
		{
			if(pid_reset[e] == true)
			{
				temp_iState[e] = 0.0;
				pid_reset[e] = false;
			}
			pTerm[e] = Kp * pid_error[e];
			temp_iState[e] += pid_error[e];
			temp_iState[e] = constrain(temp_iState[e], temp_iState_min[e], temp_iState_max[e]);
			iTerm[e] = Ki * temp_iState[e];

			//K1 defined in Configuration.h in the PID settings
#define K2 (1.0-K1)

			diff = fabs(pid_error[e]);
	//		if(diff > 2)
	//			Kd = 500;
	//		else
	//			Kd = 3000;
			dTerm[e] = (Kd * (pid_input - temp_dState[e])) * K2 + (K1 * dTerm[e]);
			temp_dState[e] = pid_input;


			//pid_output = constrain(pTerm[e] + iTerm[e] - dTerm[e], target_temperature[e] - TempValue, PID_MAX);
			//最新风扇罩子不需要管吹模型的风扇
			/*
			if(fanSpeed > 100)
			{
				pid_min  = constrain(target_temperature[e] - 20, 0, 200);
				pid_output = constrain(pTerm[e] + iTerm[e] - dTerm[e], pid_min, PID_MAX);
			}
			else
			*/
			{
				//pid_min  = constrain(target_temperature[e] - pid_offset, 0, 200);
				pid_min = pid_offset;
				pid_output = constrain(pTerm[e] + iTerm[e] - dTerm[e], pid_min, PID_MAX);
			}
			//pid_output = constrain(pTerm[e] + iTerm[e] - dTerm[e], 0, PID_MAX);
		}
		#else
		pid_output = constrain(target_temperature[e], 0, PID_MAX);
		#endif //PID_OPENLOOP


		#ifdef PID_DEBUG
		if(0 == e)
		{
			printf(" PIDDEBUG %d", e);
			printf(": Input %f", pid_input);
			printf(" Output %f", pid_output);
			printf(" pTerm %f", pTerm[e]);
			printf(" iTerm %f", iTerm[e]);
			printf(" dTerm %f", dTerm[e]);
			printf("\n\r");
		}
		#endif //PID_DEBUG

		#else /* PID off */
		pid_output = 0;
		if(current_temperature[e] < target_temperature[e])
		{
			pid_output = PID_MAX;
		}
		#endif	//PIDTEMP

		// Check if temperature is within the correct range
		if((current_temperature[e] > minttemp[e]) && (current_temperature[e] < maxttemp[e]))
		{
			//soft_pwm[e] = (int)pid_output >> 1;
			//soft_pwm[e] = (int)pid_output;
			soft_pwm[e] = (int)(pid_output * 1.0);
		}
		else
		{
			soft_pwm[e] = 0;
		}

		#ifdef WATCH_TEMP_PERIOD	 // 加热前检查
		if(watchmillis[e] && millis() - watchmillis[e] > WATCH_TEMP_PERIOD)
		{
			if(degHotend(e) < watch_start_temp[e] + WATCH_TEMP_INCREASE)
			{
				setTargetHotend(0, e);
				LCD_MESSAGEPGM("Heating failed");
				SERIAL_ECHO_START;
				SERIAL_ECHOLN("Heating failed");
			}
			else
			{
				watchmillis[e] = 0;
			}
		}
		#endif

	} // End extruder for loop

	#ifndef PIDTEMPBED
	if(millis() - previous_millis_bed_heater < BED_CHECK_INTERVAL)
		return;
	previous_millis_bed_heater = millis();
	#endif

	#if TEMP_SENSOR_BED != 0

	#ifdef PIDTEMPBED
	pid_input = current_temperature_bed;
	#ifndef PID_OPENLOOP
	pid_error_bed = target_temperature_bed - pid_input;
	pTerm_bed = bedKp * pid_error_bed;
	temp_iState_bed += pid_error_bed;
	temp_iState_bed = constrain(temp_iState_bed, temp_iState_min_bed, temp_iState_max_bed);
	iTerm_bed = bedKi * temp_iState_bed;

	//K1 defined in Configuration.h in the PID settings
#define K2 (1.0-K1)
	dTerm_bed = (bedKd * (pid_input - temp_dState_bed)) * K2 + (K1 * dTerm_bed);
	temp_dState_bed = pid_input;

	pid_output = constrain(pTerm_bed + iTerm_bed - dTerm_bed, 0, MAX_BED_POWER);

	#else
	pid_output = constrain(target_temperature_bed, 0, MAX_BED_POWER);
	#endif //PID_OPENLOOP

	if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
	{
		//soft_pwm_bed = (int)pid_output >> 1;
		soft_pwm_bed = (int)pid_output;
	}
	else
	{
		soft_pwm_bed = 0;
	}

	#elif !defined(BED_LIMIT_SWITCHING)
	// Check if temperature is within the correct range
	if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
	{
		if(current_temperature_bed >= target_temperature_bed)
		{
			soft_pwm_bed = 0;
		}
		else
		{
			//soft_pwm_bed = MAX_BED_POWER >> 1;
			soft_pwm_bed = MAX_BED_POWER;
		}
	}
	else
	{
		soft_pwm_bed = 0;
		HEATER_BED_PIN = 0;
	}
	#else //#ifdef BED_LIMIT_SWITCHING
	// Check if temperature is within the correct band
	if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
	{
		if(current_temperature_bed > target_temperature_bed + BED_HYSTERESIS)
		{
			soft_pwm_bed = 0;
		}
		else if(current_temperature_bed <= target_temperature_bed - BED_HYSTERESIS)
		{
			//soft_pwm_bed = MAX_BED_POWER >> 1;
			soft_pwm_bed = MAX_BED_POWER;
		}
	}
	else
	{
		soft_pwm_bed = 0;
		HEATER_BED_PIN = 0;
	}

	#endif
	#endif
}


static float analog2temp(int raw, uint8_t e)
{
	if(e >= EXTRUDERS)
	{
		SERIAL_ERROR_START;
		USR_UsrLog("%d", e);
		USR_UsrLog(" - Invalid extruder number !");
		kill();
	}


	if(heater_ttbl_map[e] != NULL)
	{
		float celsius = 0;
		uint8_t i;
		short (*tt)[][2] = (short (*)[][2])(heater_ttbl_map[e]);

		for(i = 1; i < heater_ttbllen_map[e]; i++)
		{
			if((*tt)[i][0] > raw)
			{
				celsius = (*tt)[i - 1][1] +
				          (raw - (*tt)[i - 1][0]) *
				          (float)((*tt)[i][1] - (*tt)[i - 1][1]) /
				          (float)((*tt)[i][0] - (*tt)[i - 1][0]);
				break;
			}
		}

		// Overflow: Set to last value in the table
		if(i == heater_ttbllen_map[e]) celsius = (*tt)[i - 1][1];

		return celsius;
	}
	return 0;
	//return ((raw * ((5.0 * 100.0) / 1024.0) / OVERSAMPLENR) * TEMP_SENSOR_AD595_GAIN) + TEMP_SENSOR_AD595_OFFSET;
}

// Derived from RepRap FiveD extruder::getTemperature()
// For bed temperature measurement.
static float analog2tempBed(int raw)
{
	#ifdef BED_USES_THERMISTOR
	float celsius = 0;
	u8 i;

	for(i = 1; i < BEDTEMPTABLE_LEN; i++)
	{
		if(BEDTEMPTABLE[i][0] > raw)
		{
			celsius  = BEDTEMPTABLE[i - 1][1] +
			           (raw - BEDTEMPTABLE[i - 1][0]) *
			           (float)(BEDTEMPTABLE[i][1] - BEDTEMPTABLE[i - 1][1]) /
			           (float)(BEDTEMPTABLE[i][0] - BEDTEMPTABLE[i - 1][0]);
			break;
		}
	}

	// Overflow: Set to last value in the table
	if(i == BEDTEMPTABLE_LEN) celsius = BEDTEMPTABLE[i - 1][1];

	return celsius;
	#elif defined BED_USES_AD595
	return ((raw * ((5.0 * 100.0) / 1024.0) / OVERSAMPLENR) * TEMP_SENSOR_AD595_GAIN) + TEMP_SENSOR_AD595_OFFSET;
	#else
	return 0;
	#endif
}

/* Called to get the raw values into the the actual temperatures. The raw values are created in interrupt context,
    and this function is called from normal context as it is too slow to run in interrupts and will block the stepper routine otherwise */

u8	e = 0;
static void updateTemperaturesFromRawValues(void)
{
	//看门狗
	IWDG_Feed();
	for(e = 0; e < EXTRUDERS; e++)
	{
		current_temperature[e] = analog2temp(current_temperature_raw[e], e);
		//current_temperature[e] *= 1.08;		//在挤出头实际为185摄氏度的时候采集为200摄氏度

	}

	current_temperature_bed = analog2tempBed(current_temperature_bed_raw);
	show_temperature();


	CRITICAL_SECTION_START;
	temp_meas_ready = false;
	CRITICAL_SECTION_END;
}

void tp_init()
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	// Finish init of mult extruder arrays
	int e;
	//	printf("tpint!\n\r");
	for(e = 0; e < EXTRUDERS; e++)
	{
		// populate with the first value
		maxttemp[e] = maxttemp[0];
		#ifdef PIDTEMP
		temp_iState_min[e] = 0.0;
		temp_iState_max[e] = PID_INTEGRAL_DRIVE_MAX / Ki;
		#endif //PIDTEMP

		#ifdef PIDTEMPBED
		temp_iState_min_bed = 0.0;
		temp_iState_max_bed = PID_INTEGRAL_DRIVE_MAX / bedKi;
		#endif //PIDTEMPBED
	}

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);

	//	TIM_TimeBaseStructure.TIM_Period = 127;
	TIM_TimeBaseStructure.TIM_Period = 255;
	TIM_TimeBaseStructure.TIM_Prescaler = 8399;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);


	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	#if defined(FAN_PIN)
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	#endif

	#if defined(HEATER_1_PIN)
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
	#endif

	#if defined(HEATER_0_PIN)
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	#endif

	#if defined(HEATER_BED_PIN)
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	#endif

	#if defined(E0_FAN)
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
	#endif

	TIM_Cmd(TIM3, ENABLE);  //使能TIM3
	TIM_Cmd(TIM4, ENABLE);  //使能TIM4

	#if 1

	// Use timer6 for temperature measurement
	// Interleave temperature interrupt with millies interrupt

	// Wait for temperature measurement to settle
	delay_ms(250);

	#ifdef HEATER_0_MINTEMP
	minttemp[0] = HEATER_0_MINTEMP;
	while(analog2temp(minttemp_raw[0], 0) < HEATER_0_MINTEMP)
	{
		#if HEATER_0_RAW_LO_TEMP < HEATER_0_RAW_HI_TEMP
		minttemp_raw[0] += OVERSAMPLENR;
		#else
		minttemp_raw[0] -= OVERSAMPLENR;
		#endif
	}
	#endif //MINTEMP

	#ifdef HEATER_0_MAXTEMP
	maxttemp[0] = HEATER_0_MAXTEMP;
	while(analog2temp(maxttemp_raw[0], 0) > HEATER_0_MAXTEMP)
	{
		#if HEATER_0_RAW_LO_TEMP < HEATER_0_RAW_HI_TEMP
		maxttemp_raw[0] -= OVERSAMPLENR;
		#else
		maxttemp_raw[0] += OVERSAMPLENR;
		#endif
	}
	#endif //MAXTEMP

	#if (EXTRUDERS > 1) && defined(HEATER_1_MINTEMP)
	minttemp[1] = HEATER_1_MINTEMP;
	while(analog2temp(minttemp_raw[1], 1) < HEATER_1_MINTEMP)
	{
		#if HEATER_1_RAW_LO_TEMP < HEATER_1_RAW_HI_TEMP
		minttemp_raw[1] += OVERSAMPLENR;
		#else
		minttemp_raw[1] -= OVERSAMPLENR;
		#endif
	}
	#endif // MINTEMP 1

	#if (EXTRUDERS > 1) && defined(HEATER_1_MAXTEMP)
	maxttemp[1] = HEATER_1_MAXTEMP;
	while(analog2temp(maxttemp_raw[1], 1) > HEATER_1_MAXTEMP)
	{
		#if HEATER_1_RAW_LO_TEMP < HEATER_1_RAW_HI_TEMP
		maxttemp_raw[1] -= OVERSAMPLENR;
		#else
		maxttemp_raw[1] += OVERSAMPLENR;
		#endif
	}
	#endif //MAXTEMP 1

	#if (EXTRUDERS > 2) && defined(HEATER_2_MINTEMP)
	minttemp[2] = HEATER_2_MINTEMP;
	while(analog2temp(minttemp_raw[2], 2) < HEATER_2_MINTEMP)
	{
		#if HEATER_2_RAW_LO_TEMP < HEATER_2_RAW_HI_TEMP
		minttemp_raw[2] += OVERSAMPLENR;
		#else
		minttemp_raw[2] -= OVERSAMPLENR;
		#endif
	}
	#endif //MINTEMP 2
	#if (EXTRUDERS > 2) && defined(HEATER_2_MAXTEMP)
	maxttemp[2] = HEATER_2_MAXTEMP;
	while(analog2temp(maxttemp_raw[2], 2) > HEATER_2_MAXTEMP)
	{
		#if HEATER_2_RAW_LO_TEMP < HEATER_2_RAW_HI_TEMP
		maxttemp_raw[2] -= OVERSAMPLENR;
		#else
		maxttemp_raw[2] += OVERSAMPLENR;
		#endif
	}
	#endif //MAXTEMP 2

	#ifdef BED_MINTEMP
	/* No bed MINTEMP error implemented?!? */ /*
	while(analog2tempBed(bed_minttemp_raw) < BED_MINTEMP) {
	#if HEATER_BED_RAW_LO_TEMP < HEATER_BED_RAW_HI_TEMP
	    bed_minttemp_raw += OVERSAMPLENR;
	#else
	    bed_minttemp_raw -= OVERSAMPLENR;
	#endif
	  }
	  */
	#endif //BED_MINTEMP
	#ifdef BED_MAXTEMP
	while(analog2tempBed(bed_maxttemp_raw) > BED_MAXTEMP)
	{
		#if HEATER_BED_RAW_LO_TEMP < HEATER_BED_RAW_HI_TEMP
		bed_maxttemp_raw -= OVERSAMPLENR;
		#else
		bed_maxttemp_raw += OVERSAMPLENR;
		#endif
	}
	#endif //BED_MAXTEMP
	//	printf("end!\n\r");
	#endif
}

void setWatch(void)
{
	#ifdef WATCH_TEMP_PERIOD
	for(int e = 0; e < EXTRUDERS; e++)
	{
		if(degHotend(e) < degTargetHotend(e) - (WATCH_TEMP_INCREASE * 2))
		{
			watch_start_temp[e] = degHotend(e);
			watchmillis[e] = millis();
		}
	}
	#endif
}


void disable_heater(void)
{
	int i;
	E0_FAN = 0;
	for(i = 0; i < EXTRUDERS; i++)
	{
		setTargetHotend(0, i);
	}
	setTargetBed(0);
	#if defined(TEMP_0_PIN)
	target_temperature[0] = 0;
	soft_pwm[0] = 0;
	#if defined(HEATER_0_PIN)
	HEATER_0_PIN = 0;
	#endif
	#endif

	#if defined(TEMP_1_PIN)
	target_temperature[1] = 0;
	soft_pwm[1] = 0;
	#if defined(HEATER_1_PIN)
	HEATER_1_PIN = 0;
	#endif
	#endif

	#if defined(TEMP_2_PIN)
	target_temperature[2] = 0;
	soft_pwm[2] = 0;
	#if defined(HEATER_2_PIN)
	HEATER_2_PIN = 0;
	#endif
	#endif

	#if defined(TEMP_BED_PIN)
	target_temperature_bed = 0;
	soft_pwm_bed = 0;
	#if defined(HEATER_BED_PIN)
	HEATER_BED_PIN = 0;
	#endif
	#endif
}

void max_temp_error(uint8_t e)
{
	disable_heater();
	if(IsStopped() == false)
	{
		SERIAL_ERROR_START;
		USR_ErrLog("%d", e);
		USR_ErrLog(": Extruder switched off. MAXTEMP triggered !");
		// LCD_ALERTMESSAGEPGM("Err: MAXTEMP");	 /////////////////////////////////
	}
	#ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
	Stop();
	#endif
}

void min_temp_error(uint8_t e)
{
	disable_heater();
	if(IsStopped() == false)
	{
		SERIAL_ERROR_START;
		USR_ErrLog("%d", e);
		USR_ErrLog(": Extruder switched off. MINTEMP triggered !");
		//LCD_ALERTMESSAGEPGM("Err: MINTEMP"); //////////////////////////////////
	}
	#ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
	Stop();
	#endif
}

void bed_max_temp_error(void)
{
	#ifdef HEATER_BED_PIN
	HEATER_BED_PIN = 0;
	#endif
	if(IsStopped() == false)
	{
		SERIAL_ERROR_START;
		USR_ErrLog("Temperature heated bed switched off. MAXTEMP triggered !!");
		//  LCD_ALERTMESSAGEPGM("Err: MAXTEMP BED");///////////////////////////////////////////
	}
	#ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
	Stop();
	#endif
}


// Timer 5 is shared with millies
void TIM5_IRQHandler(void)
{
	//these variables are only accesible from the ISR, but static, so they don't loose their value
	static unsigned char temp_count = 0;
	static unsigned long raw_temp_0_value = 0;
	#if EXTRUDERS > 1
	static unsigned long raw_temp_1_value = 0;
	#endif
	#if EXTRUDERS > 2
	static unsigned long raw_temp_2_value = 0;
	#endif
	static unsigned long raw_temp_bed_value = 0;
	static unsigned char temp_state = 0;
	FunctionCode = IRQ_3;
	if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update);

		HEATER_0_PIN = (uint16_t)soft_pwm[0];
		#if EXTRUDERS > 1
		HEATER_1_PIN = (uint16_t)soft_pwm[1];
		#endif
		#if EXTRUDERS > 2
		HEATER_2_PIN = (uint16_t)soft_pwm[2];
		#endif

		#ifdef HEATER_BED_PIN
		HEATER_BED_PIN = (uint16_t)soft_pwm_bed;
		#endif

		#ifdef FAN_PIN
		FAN_PIN = (uint16_t)fanSpeed;
		#endif

		switch(temp_state)
		{
			case 0:
				#if defined(TEMP_0_PIN)
				raw_temp_0_value += TEMP_0_PIN;
				#endif
				temp_state = 1;
				break;
			case 1:
				#if defined(TEMP_1_PIN)
				raw_temp_1_value += TEMP_1_PIN;
				#endif
				temp_state = 2;
				break;
			case 2:
				#if defined(TEMP_2_PIN)
				raw_temp_2_value += TEMP_2_PIN;
				#endif
				temp_state = 3;
				break;
			case 3:
				#if defined(TEMP_BED_PIN)
				raw_temp_bed_value += TEMP_BED_PIN; //28ms
				#endif

				temp_state = 0;
				temp_count++;
				break;

			default:
				USR_ErrLog("Temp measurement error!");
				break;
		}

		//lcd_buttons_update();	///////////////////////////////////

		if(temp_count >= 16)   // 4 ms * 16 = 64ms.
		{
			//
			if(!temp_meas_ready)
			{
				//Only update the raw values if they have been read. Else we could be updating them during reading.
				current_temperature_raw[0] = raw_temp_0_value;
				#if EXTRUDERS > 1
				current_temperature_raw[1] = raw_temp_1_value;
				#endif
				#if EXTRUDERS > 2
				current_temperature_raw[2] = raw_temp_2_value;
				#endif
				current_temperature_bed_raw = raw_temp_bed_value;
				//("\r\n ex0:%d\r\n",current_temperature_raw[0]);
				//	  printf("\r\n bed:%d\r\n",current_temperature_bed_raw);
			}

			temp_meas_ready = true;
			temp_count = 0;
			raw_temp_0_value = 0;
			#if EXTRUDERS > 1
			raw_temp_1_value = 0;
			#endif
			#if EXTRUDERS > 2
			raw_temp_2_value = 0;
			#endif
			raw_temp_bed_value = 0;
			//            if(PrintInfo.printsd == 1){                             //打印状态才检测温度传感器状态
			//    			#if HEATER_0_RAW_LO_TEMP > HEATER_0_RAW_HI_TEMP
			//    			if(current_temperature_raw[0] <= maxttemp_raw[0]) {
			//    				max_temp_error(0);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[0] >= maxttemp_raw[0]) {
			//    				max_temp_error(0);
			//    			}
			//    			#endif

			//    			#if HEATER_0_RAW_LO_TEMP > HEATER_0_RAW_HI_TEMP
			//    			if(current_temperature_raw[0] >= minttemp_raw[0]) {
			//    				min_temp_error(0);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[0] <= minttemp_raw[0]) {
			//    				min_temp_error(0);
			//    			}
			//    			#endif


			//    			#if EXTRUDERS > 1
			//    			#if HEATER_1_RAW_LO_TEMP > HEATER_1_RAW_HI_TEMP
			//    			if(current_temperature_raw[1] <= maxttemp_raw[1]) {
			//    				max_temp_error(1);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[1] >= maxttemp_raw[1]) {
			//    				max_temp_error(1);
			//    			}
			//    			#endif
			//    			#if HEATER_1_RAW_LO_TEMP > HEATER_1_RAW_HI_TEMP
			//    			if(current_temperature_raw[1] >= minttemp_raw[1]) {
			//    				min_temp_error(1);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[1] <= minttemp_raw[1]) {
			//    				min_temp_error(1);
			//    			}
			//    			#endif
			//    			#endif


			//    			#if EXTRUDERS > 2
			//    			#if HEATER_2_RAW_LO_TEMP > HEATER_2_RAW_HI_TEMP
			//    			if(current_temperature_raw[2] <= maxttemp_raw[2]) {
			//    				max_temp_error(2);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[2] >= maxttemp_raw[2]) {
			//    				max_temp_error(2);
			//    			}
			//    			#endif

			//    			#if HEATER_2_RAW_LO_TEMP > HEATER_2_RAW_HI_TEMP
			//    			if(current_temperature_raw[2] >= minttemp_raw[2]) {
			//    				min_temp_error(2);
			//    			}
			//    			#else
			//    			if(current_temperature_raw[2] <= minttemp_raw[2]) {
			//    				min_temp_error(2);
			//    			}
			//    			#endif

			//    			#endif


			//    			/* No bed MINTEMP error? */
			//    			#if defined(BED_MAXTEMP) && (TEMP_SENSOR_BED != 0)
			//    			# if HEATER_BED_RAW_LO_TEMP > HEATER_BED_RAW_HI_TEMP
			//    			if(current_temperature_bed_raw <= bed_maxttemp_raw) {
			//    				target_temperature_bed = 0;
			//    				bed_max_temp_error();
			//    			}
			//    			#else
			//    			if(current_temperature_bed_raw >= bed_maxttemp_raw) {
			//    				target_temperature_bed = 0;
			//    				bed_max_temp_error();
			//    			}
			//    			#endif
			//    			#endif
			//           }
		}
	}

}

#ifdef PIDTEMP
// Apply the scale factors to the PID values


float scalePID_i(float i)
{
	return i * PID_dT;
}

float unscalePID_i(float i)
{
	return i / PID_dT;
}

float scalePID_d(float d)
{
	return d / PID_dT;
}

float unscalePID_d(float d)
{
	return d * PID_dT;
}

#endif //PIDTEMP
float degHotend(u8 extruder)
{
	return current_temperature[extruder];
}

float degBed(void)
{
	return current_temperature_bed;
}


float degTargetHotend(u8 extruder)
{
	return target_temperature[extruder];
}

float degTargetBed(void)
{
	return target_temperature_bed;
}
/*
**将设定值赋予对应挤出头目标温度值
*/
void setTargetHotend(const float celsius, uint8_t extruder)
{
	if(celsius == 0) E0_FAN = 0;
	else E0_FAN = 1;
	if(celsius > HEATER_0_MAXTEMP)
	{
		//target_temperature[extruder] = HEATER_0_MAXTEMP;
		target_temperature[0] = HEATER_0_MAXTEMP;

	}
	else
	{
		//target_temperature[extruder] = celsius;
		target_temperature[0] = celsius;
	}
	printf("target_temperature[0]:%d\r\n", target_temperature[0]);
}

void setTargetBed(const float celsius)
{
	if(celsius > BED_MAXTEMP)
	{
		target_temperature_bed = BED_MAXTEMP;
	}
	else
	{
		target_temperature_bed = celsius;
	}
}
/*
判断挤出头当前温度是否大于目标温度
*/
bool isHeatingHotend(u8 extruder)
{
	return target_temperature[extruder] > current_temperature[extruder];
}

bool isHeatingBed(void)
{
	return target_temperature_bed > current_temperature_bed;
}

bool isCoolingHotend(u8 extruder)
{
	return target_temperature[extruder] < current_temperature[extruder];
}

bool isCoolingBed(void)
{
	return target_temperature_bed < current_temperature_bed;
}
void autotempShutdown(void)
{
	#ifdef AUTOTEMP
	if(autotemp_enabled)
	{
		autotemp_enabled = false;
		if(degTargetHotend(active_extruder) > autotemp_min)
			setTargetHotend(0, active_extruder);
	}
	#endif
}
