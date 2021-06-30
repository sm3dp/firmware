#include "delay.h"
#include "sm_firmware.h"
#include "stepper.h"
#include "Configuration_adv.h"
#include "Configuration.h"
#include "usart.h"
#include "language.h"
#include "temperature.h"
#include "lcd_menu.h"
#include "sm_plus.h"
#include "bsp.h"
#include "App_Language.h"
#include "App_Timer.h"
#include "stm32f4xx.h"

#define STEP_DELAY_TIME 30

#define DIGIPOT_CHANNELS {4,1,0,2,3} // X Y Z E0 E1 digipot channels to stepper driver mapping

u8 subsection_x_value = 1;
u8 subsection_y_value = 1;
u8 subsection_z_value = 1;
u8 subsection_e0_value = 1;
u8 subsection_e1_value = 1;


#define ENABLE_STEPPER_DRIVER_INTERRUPT()  TIM_ITConfig(TIM2,TIM_IT_Update, ENABLE);  //使能TIMx
#define DISABLE_STEPPER_DRIVER_INTERRUPT() TIM_ITConfig(TIM2,TIM_IT_Update, DISABLE);


block_t *current_block;  // A pointer to the block currently being traced


// Variables used by The Stepper Driver Interrupt
static unsigned char out_bits;        // The next stepping-bits to be output
static long counter_x,       // Counter variables for the bresenham line tracer
       counter_y,
       counter_z,
       counter_e;
volatile static unsigned long step_events_completed; // The number of step events executed in the current block
#ifdef ADVANCE
	static long advance_rate, advance, final_advance = 0;
	static long old_advance = 0;
	static long e_steps[3];
#endif
volatile long acceleration_time, deceleration_time;
//static unsigned long accelerate_until, decelerate_after, acceleration_rate, initial_rate, final_rate, nominal_rate;
static unsigned short acc_step_rate; // needed for deccelaration start point
static char step_loops;
static unsigned short TIME2_nominal;
//static unsigned short step_loops_nominal;

volatile long endstops_trigsteps[3] = {0, 0, 0};
volatile long endstops_stepsTotal, endstops_stepsDone;
static volatile bool endstop_x_hit = FALSE;
static volatile bool endstop_y_hit = FALSE;
static volatile bool endstop_z_hit = FALSE;

#ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
	bool abort_on_endstop_hit = FALSE;
#endif
#if defined X_MIN_PIN
	static bool old_x_min_endstop = TRUE;
#endif
#if defined X_MAX_PIN
	static bool old_x_max_endstop = TRUE;
#endif
#if defined Y_MIN_PIN
	static bool old_y_min_endstop = TRUE;
#endif
#if defined Y_MAX_PIN
	static bool old_y_max_endstop = TRUE;
#endif
#if defined Z_MIN_PIN
	static bool old_z_min_endstop = TRUE;
#endif
#if defined Z_MAX_PIN
	static bool old_z_max_endstop = TRUE;
#endif
static bool check_endstops = TRUE;



volatile long count_position[NUM_AXIS] = { 0, 0, 0, 0};
volatile signed char count_direction[NUM_AXIS] = { 1, 1, 1, 1};

//===========================================================================
//=============================functions         ============================
//===========================================================================

#define CHECK_ENDSTOPS  if(check_endstops)

//static u8 check_filament=1;		//断料检测  。。。。默认为1
//static bool old_filamenta_endstop=0;	//料盘a断料检测上个状态
//static bool old_filamentb_endstop=0;	//料盘b断料检测上个状态
//static bool old_filamentc_endstop=0;	//料盘c断料检测上个状态




#define MultiU24X24toH16(intRes, longIn1, longIn2) intRes= ((uint64_t)(longIn1) * (longIn2)) >> 24
#define MultiU16X8toH16(intRes, charIn1, intIn2) intRes = ((charIn1) * (intIn2)) >> 16

void checkHitEndstops(void)
{
	FunctionCode = FUN_6;
	if(endstop_x_hit || endstop_y_hit || endstop_z_hit)
	{
		SERIAL_ECHO_START;
		USR_UsrLog(MSG_ENDSTOPS_HIT);
		if(endstop_x_hit)
		{
			USR_UsrLog(" X:%f", (float)endstops_trigsteps[X_AXIS] / axis_steps_per_unit[X_AXIS]);
			// LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT "X");  ////////////////////////////////////////////////////
		}
		if(endstop_y_hit)
		{
			USR_UsrLog(" Y:%f", (float)endstops_trigsteps[Y_AXIS] / axis_steps_per_unit[Y_AXIS]);
			//  LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT "Y");   ////////////////////////////////////////////////////////////
		}
		if(endstop_z_hit)
		{
			USR_UsrLog(" Z:%f", (float)endstops_trigsteps[Z_AXIS] / axis_steps_per_unit[Z_AXIS]);
			//  LCD_MESSAGEPGM(MSG_ENDSTOPS_HIT "Z");  ////////////////////////////////////////////
		}
		endstop_x_hit = FALSE;
		endstop_y_hit = FALSE;
		endstop_z_hit = FALSE;

		#ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
		if(abort_on_endstop_hit)
		{
			card.sdprinting = FALSE;
			card.closefile();
			quickStop();
			setTargetHotend0(0);
			setTargetHotend1(0);
			setTargetHotend2(0);
		}
		#endif
	}
}

void endstops_hit_on_purpose(void)
{
	endstop_x_hit = FALSE;
	endstop_y_hit = FALSE;
	endstop_z_hit = FALSE;
}

void enable_endstops(bool check)
{
	check_endstops = check;
}


void st_wake_up(void)
{
	ENABLE_STEPPER_DRIVER_INTERRUPT();
}

void step_wait(void)
{
	u8 i;
	for(i = 0; i < 6; i++)
	{
		__NOP();
	}
}
/*
*Function:
*Input Paragramed:
*Output Paragramed:
*Remarks:
*/
uint16_t calc_timer(uint16_t step_rate)
{
	uint16_t timer;

	if(step_rate > MAX_STEP_FREQUENCY) step_rate = MAX_STEP_FREQUENCY;

	/*if steprate > 20KHz  step 4 times*/
	if(step_rate > 20000)
	{
		step_rate = (step_rate >> 2) & 0x3fff;
		step_loops = 2;
	}
	else if(step_rate > 10000)
	{
		step_rate = (step_rate >> 1) & 0x7fff;
		step_loops = 2;
	}
	else
	{
		step_loops = 1;
	}

	if(step_rate < 32)
	{
		step_rate = 32;
	}

	timer = 2000000 / step_rate;			//default 2000000
	if(timer < 150)
	{
		timer = 150;    //(20kHz this should never happen)
		USR_DbgLog(MSG_STEPPER_TO_HIGH);
		USR_DbgLog("%d", step_rate);
	}
	return timer;
}

// Initializes the trapezoid generator from the current block. Called whenever a new
// block begins.
void trapezoid_generator_reset(void)
{
	#ifdef ADVANCE
	advance = current_block->initial_advance;
	final_advance = current_block->final_advance;
	// Do E steps + advance steps
	e_steps[current_block->active_extruder] += ((advance >> 8) - old_advance);
	old_advance = advance >> 8;
	#endif
	deceleration_time = 0;
	// step_rate to timer interval
	TIME2_nominal = calc_timer(current_block->nominal_rate);

	// make a note of the number of step loops required at nominal speed
	//step_loops_nominal = step_loops;
	//	acc_step_rate = current_block->initial_rate;
	acc_step_rate = current_block->initial_rate;
	acceleration_time = calc_timer(acc_step_rate);

	#ifdef __TIM2_CCRV_MOTO
	TIM2->CCR1 = TIM2->CNT + acceleration_time;
	#else
	TIM_SetAutoreload(TIM2, acceleration_time - 1);
	#endif

}

// "The Stepper Driver Interrupt" - This timer interrupt is the workhorse.
// It pops blocks from the block_buffer and executes them by pulsing the stepper pins appropriately.
volatile uint16_t MOTO_TIMER = 0;
void App_MotoRunning(void)
{
	uint8_t stepdelay = STEP_DELAY_TIME;
	uint16_t step_rate;

	if(current_block == NULL)
	{
		// Anything in the buffer?
		current_block = plan_get_current_block();
		if(current_block != NULL)
		{
			current_block->busy = TRUE;
			trapezoid_generator_reset();
			counter_x = -(current_block->step_event_count >> 1);
			counter_y = counter_x;
			counter_z = counter_x;
			counter_e = counter_x;
			step_events_completed = 0;
			e_steps_count = 0;
			z_setps_count = 0;

			#ifdef Z_LATE_ENABLE
			if(current_block->steps_z > 0)
			{
				enable_z();
				#ifdef __TIM2_CCRV_MOTO
				TIM2->CCR1 = TIM2->CNT + 2000;
				#else
				TIM_SetAutoreload(TIM2, 2000 - 1);
				#endif
				return;
			}
			#endif

		}
		else
		{
			#ifdef __TIM2_CCRV_MOTO
			TIM2->CCR1 = TIM2->CNT + 5000;
			#else
			TIM_SetAutoreload(TIM2, 2000 - 1);
			#endif
		}
	}

	if(current_block != NULL)
	{
		#ifdef LASER
		if(PrinterMode == 1)
		{
			if(current_block->laser == 0)
			{
				LASER = FALSE;
				//	printf("LASER = FALSE is in stepper\r\n");
			}
			else if(current_block->laser == 1)
			{
				LASER = true;
				//printf("LASER = true is in stepper\r\n");
			}
		}
		#endif
		out_bits = current_block->direction_bits;

		if((out_bits & (1 << X_AXIS)) != 0)
		{
			X_DIR_PIN = INVERT_X_DIR;
			count_direction[X_AXIS] = -1;
		}
		else
		{
			X_DIR_PIN = !INVERT_X_DIR;
			count_direction[X_AXIS] = 1;
		}



		if((out_bits & (1 << Y_AXIS)) != 0)
		{
			Y_DIR_PIN = INVERT_Y_DIR;
			count_direction[Y_AXIS] = -1;
		}
		else
		{
			Y_DIR_PIN = !INVERT_Y_DIR;
			count_direction[Y_AXIS] = 1;
		}

		// Set direction en check limit switches
		#ifndef COREXY
		if((out_bits & (1 << X_AXIS)) != 0)  	 // stepping along -X axis
		{
		#else
		if((((out_bits & (1 << X_AXIS)) != 0) && (out_bits & (1 << Y_AXIS)) != 0))  	//-X occurs for -A and -B
		{
		#endif

			CHECK_ENDSTOPS
			{
				#if defined X_MIN_PIN
				bool x_min_endstop = X_MIN_PIN != X_ENDSTOPS_INVERTING ;
				if(x_min_endstop && old_x_min_endstop && (current_block->steps_x > 0))
				{
					endstops_trigsteps[X_AXIS] = count_position[X_AXIS];
					endstop_x_hit = TRUE;
					step_events_completed = current_block->step_event_count;
					CheckMotorOutRangeFlag = 1;
				}
				old_x_min_endstop = x_min_endstop;
				#endif
			}
		}
		else
		{
			// +direction
			CHECK_ENDSTOPS
			{
				#if defined X_MAX_PIN
				bool x_max_endstop = X_MAX_PIN != X_ENDSTOPS_INVERTING;
				if(x_max_endstop && old_x_max_endstop && (current_block->steps_x > 0))
				{
					endstops_trigsteps[X_AXIS] = count_position[X_AXIS];
					endstop_x_hit = TRUE;
					step_events_completed = current_block->step_event_count;
				}
				old_x_max_endstop = x_max_endstop;
				#endif
			}
		}

		#ifndef COREXY
		if((out_bits & (1 << Y_AXIS)) != 0)    // -direction
		{
		#else
		if((((out_bits & (1 << X_AXIS)) != 0) && (out_bits & (1 << Y_AXIS)) == 0))   // -Y occurs for -A and +B
		{
		#endif

			CHECK_ENDSTOPS
			{
				#if defined(Y_MIN_PIN) //&& Y_MIN_PIN > -1
				bool y_min_endstop = Y_MIN_PIN != Y_ENDSTOPS_INVERTING;
				if(y_min_endstop && old_y_min_endstop && (current_block->steps_y > 0))
				{
					endstops_trigsteps[Y_AXIS] = count_position[Y_AXIS];
					endstop_y_hit = TRUE;
					step_events_completed = current_block->step_event_count;
					CheckMotorOutRangeFlag = 1;
				}
				old_y_min_endstop = y_min_endstop;
				#endif
			}
		}
		else
		{
			// +direction
			CHECK_ENDSTOPS
			{
				#if defined(Y_MAX_PIN)// && Y_MAX_PIN > -1
				bool y_max_endstop = Y_MAX_PIN != Y_ENDSTOPS_INVERTING;
				if(y_max_endstop && old_y_max_endstop && (current_block->steps_y > 0))
				{
					endstops_trigsteps[Y_AXIS] = count_position[Y_AXIS];
					endstop_y_hit = TRUE;
					step_events_completed = current_block->step_event_count;
				}
				old_y_max_endstop = y_max_endstop;
				#endif
			}
		}

		if((out_bits & (1 << Z_AXIS)) != 0)
		{
			// -direction
			Z_DIR_PIN = INVERT_Z_DIR;

			#ifdef Z_DUAL_STEPPER_DRIVERS
			// WRITE(Z2_DIR_PIN,INVERT_Z_DIR);
			#endif

			#ifdef MESH_BED_LEVELING				//自动调平
			u8 leveling_stop = BLTOUCH_LIMIT != 0;
			//if(leveling_stop && old_leveling_stop && (current_block->steps_z > 0))
			if(leveling_stop && mbl_flg)
			{
				BL_TOUCH_PIN = BL_UP;
				endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
				step_events_completed = current_block->step_event_count;
				current_position[Z_AXIS] = (float)count_position[Z_AXIS] / axis_steps_per_unit[Z_AXIS];
				position[Z_AXIS] = count_position[Z_AXIS];

			}
			#endif

			count_direction[Z_AXIS] = -1;
			CHECK_ENDSTOPS
			{
				#if defined(Z_MIN_PIN)
				bool z_min_endstop = Z_MIN_PIN != Z_ENDSTOPS_INVERTING;
				if(z_min_endstop && old_z_min_endstop && (current_block->steps_z > 0))
				{
					endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
					endstop_z_hit = TRUE;
					step_events_completed = current_block->step_event_count;
					CheckMotorOutRangeFlag = 1;
				}
				old_z_min_endstop = z_min_endstop;
				#endif
			}
		}
		else
		{
			// +direction
			Z_DIR_PIN = !INVERT_Z_DIR;

			#ifdef Z_DUAL_STEPPER_DRIVERS
			// WRITE(Z2_DIR_PIN,!INVERT_Z_DIR);
			#endif

			count_direction[Z_AXIS] = 1;
			//      CHECK_ENDSTOPS
			{
				#if defined(Z_MAX_PIN)
				bool z_max_endstop = Z_MAX_PIN != Z_ENDSTOPS_INVERTING;
				if(z_max_endstop && old_z_max_endstop && (current_block->steps_z > 0))
				{
					endstops_trigsteps[Z_AXIS] = count_position[Z_AXIS];
					endstop_z_hit = TRUE;
					step_events_completed = current_block->step_event_count;
				}
				old_z_max_endstop = z_max_endstop;
				#endif
			}
		}

		#ifndef ADVANCE

		if((out_bits & (1 << E_AXIS)) != 0)
		{
			// -direction
			REV_E_DIR();
			count_direction[E_AXIS] = -1;
		}
		else
		{
			// +direction
			NORM_E_DIR();
			count_direction[E_AXIS] = 1;
		}

		#endif //!ADVANCE

		//GPIO_SetBits(GPIOE, GPIO_Pin_7);				//=====================START LOOP=======//
		u8 i, step_flg = 0;
		for(i = 0; i < step_loops; i++)
		{
			#ifdef ADVANCE

			counter_e += current_block->steps_e;
			if(counter_e > 0)
			{
				counter_e -= current_block->step_event_count;
				if((out_bits & (1 << E_AXIS)) != 0)   // - direction
				{
					e_steps[current_block->active_extruder]--;
				}
				else
				{
					e_steps[current_block->active_extruder]++;
				}
			}

			#endif //ADVANCE

			#if 1
			counter_x += current_block->steps_x;
			if(counter_x > 0)
			{
				X_STEP(1);
				step_flg |= 0x01;
			}

			counter_y += current_block->steps_y;
			if(counter_y > 0)
			{
				Y_STEP(1);
				step_flg |= 0x02;
			}


			counter_z += current_block->steps_z;
			if(counter_z > 0)
			{
				Z_STEP(1);
				step_flg |= 0x04;
			}


			#ifndef ADVANCE

			counter_e += current_block->steps_e;
			if(counter_e > 0)
			{
				WRITE_E_STEP(!INVERT_E_STEP_PIN);
				step_flg |= 0x08;
			}
			#endif //!ADVANCE


			#ifdef __TMC_DRIVER
			//stepdelay = STEP_DELAY_TIME; 		//延时
			while(stepdelay--)
			{
				__NOP();
				__NOP();
			}
			#endif

			if(step_flg & 0x01)
			{
				X_STEP(1);
				counter_x -= current_block->step_event_count;
				count_position[X_AXIS] += count_direction[X_AXIS];
				X_STEP(0);
				X_STEP(0);
			}
			if(step_flg & 0x02)
			{
				Y_STEP(1);
				counter_y -= current_block->step_event_count;
				count_position[Y_AXIS] += count_direction[Y_AXIS];
				Y_STEP(0);
				Y_STEP(0);
			}
			if(step_flg & 0x04)
			{
				counter_z -= current_block->step_event_count;
				count_position[Z_AXIS] += count_direction[Z_AXIS];
				Z_STEP(0);
				Z_STEP(0);
			}
			if(step_flg & 0x08)
			{
				counter_e -= current_block->step_event_count;
				count_position[E_AXIS] += count_direction[E_AXIS];
				WRITE_E_STEP(INVERT_E_STEP_PIN);
			}
			step_flg = 0;

			#endif	//if 1


			step_events_completed++;
			e_steps_count++;
			z_setps_count++;
			if(step_events_completed >= current_block->step_event_count)
			{
				current_block = NULL;
				plan_discard_current_block();
				break;
			}
		}							//==========================END LOOP

		// ====================================================Calculare new timer value

		if(step_events_completed <= (unsigned long int)current_block->accelerate_until)
		{
			MultiU24X24toH16(acc_step_rate, acceleration_time, current_block->acceleration_rate);
			acc_step_rate += current_block->initial_rate;

			// upper limit
			if(acc_step_rate > current_block->nominal_rate)
				acc_step_rate = current_block->nominal_rate;

			// step_rate to timer interval
			//timer = calc_timer(acc_step_rate);
			MOTO_TIMER = calc_timer(acc_step_rate);

			#ifdef __TIM2_CCRV_MOTO
			//TIM2->CCR1 = TIM2->CNT + timer - 1;
			TIM2_CCRV1 = timer - 1;
			#else
			//TIM_SetAutoreload(TIM2, timer - 1);
			TIM_SetAutoreload(TIM2, MOTO_TIMER - 1);
			#endif

			acceleration_time += MOTO_TIMER;//timer;
			#ifdef ADVANCE
			for(i = 0; i < step_loops; i++)
			{
				advance += advance_rate;
			}
			//if(advance > current_block->advance) advance = current_block->advance;
			// Do E steps + advance steps
			e_steps[current_block->active_extruder] += ((advance >> 8) - old_advance);
			old_advance = advance >> 8;

			#endif
		}
		else if(step_events_completed > (unsigned long int)current_block->decelerate_after)
		{
			MultiU24X24toH16(step_rate, deceleration_time, current_block->acceleration_rate);

			if(step_rate < acc_step_rate)
			{
				// Still decelerating?
				step_rate = max(acc_step_rate - step_rate, current_block->final_rate);
			}
			else
			{
				step_rate = current_block->final_rate;  // lower limit
			}

			// step_rate to timer interval
			//timer = calc_timer(step_rate);
			MOTO_TIMER = calc_timer(step_rate);

			#ifdef __TIM2_CCRV_MOTO
			//TIM2->CCR1 = TIM2->CNT + timer - 1;
			TIM2_CCRV1 = timer - 1;
			#else
			//TIM_SetAutoreload(TIM2, timer - 1);
			TIM_SetAutoreload(TIM2, MOTO_TIMER);
			#endif

			deceleration_time += MOTO_TIMER;//timer;
			#ifdef ADVANCE
			for(i = 0; i < step_loops; i++)
			{
				advance -= advance_rate;
			}
			if(advance < final_advance) advance = final_advance;
			// Do E steps + advance steps
			e_steps[current_block->active_extruder] += ((advance >> 8) - old_advance);
			old_advance = advance >> 8;
			#endif //ADVANCE
		}
		else
		{

			#ifdef __TIM2_CCRV_MOTO
			TIM2_CCRV1 = TIME2_nominal - 1;
			#else
			TIM_SetAutoreload(TIM2, TIME2_nominal - 1);
			#endif
		}
		// ======================================================================Calculare new timer value

		if(step_events_completed >= current_block->step_event_count)
		{
			current_block = NULL;
			plan_discard_current_block();
		}
	}
}


#ifdef ADVANCE

#endif // ADVANCE

void st_init(void)
{
	disable_x();
	disable_y();
	disable_z();
	disable_e0();
	disable_e1();
	disable_e2();

	X_STEP(0);
	Y_STEP(0);
	Z_STEP(0);
	E0_STEP(0);
	E1_STEP(0);

	#ifdef ADVANCE

	#endif

	enable_endstops(1); // Start with endstops active. After homing they can be disabled

}


// Block until all buffered steps are executed
void st_synchronize(void)
{
	while(blocks_queued())
	{
		manage_heater();
		manage_inactivity();
		lcd_update();
	}
}

void st_set_position(const long x, const long y, const long z, const long e)
{
	CRITICAL_SECTION_START;
	count_position[X_AXIS] = x;
	count_position[Y_AXIS] = y;
	count_position[Z_AXIS] = z;
	count_position[E_AXIS] = e;
	CRITICAL_SECTION_END;
}

void st_set_e_position(const long e)
{
	CRITICAL_SECTION_START;
	count_position[E_AXIS] = e;
	CRITICAL_SECTION_END;
}

long st_get_position(uint8_t axis)
{
	long count_pos;
	CRITICAL_SECTION_START;
	count_pos = count_position[axis];
	CRITICAL_SECTION_END;
	return count_pos;
}

void finishAndDisableSteppers(void)
{
	st_synchronize();
	disable_x();
	disable_y();
	disable_z();
	disable_e0();
	disable_e1();
}

void quickStop(void)
{
	DISABLE_STEPPER_DRIVER_INTERRUPT();
	while(blocks_queued())
		plan_discard_current_block();
	current_block = NULL;
	ENABLE_STEPPER_DRIVER_INTERRUPT();
}


void digipot_init(void) //Initialize Digipot Motor Current
{
	const uint8_t digipot_motor_current[] = DIGIPOT_MOTOR_CURRENT;
	int i;
	for(i = 0; i <= 4; i++)
		digipot_current(i, digipot_motor_current[i]);
}

void digipot_current(uint8_t driver, uint8_t current)
{
	const uint8_t digipot_ch[] = DIGIPOT_CHANNELS;
	//	printf("%d:%d\r\n",digipot_ch[driver],current);
	digitalPotWrite(digipot_ch[driver], (uint8_t)current);
}

void digitalPotWrite(uint8_t address, uint8_t value) // From Arduino DigitalPotControl example
{
	/*
	    DIGIPOTSS_PIN=1; // take the SS pin low to select the chip
	    SPI1_ReadWriteByte(address); //  send in the address and value via SPI:
	    SPI1_ReadWriteByte(value);
	    DIGIPOTSS_PIN=0; // take the SS pin high to de-select the chip:
		*/
}

void microstep_init(void)
{
	int i;
	for(i = 0; i <= 4; i++) microstep_mode(i, 8);
	// for(i=3;i<=4;i++) microstep_mode(i,16);
}

void microstep_ms(uint8_t driver, int8_t ms1, int8_t ms2, int8_t ms3)
{
	/*
	 if(ms1 > -1) switch(driver)
	 {
	   case 0:X_MS1_PIN=ms1 ; break;
	   case 1:Y_MS1_PIN=ms1 ; break;
	   case 2:Z_MS1_PIN=ms1 ; break;
	   case 3:E0_MS1_PIN=ms1 ; break;
	   case 4:E1_MS1_PIN=ms1 ; break;
	default:  break;
	 }
	 if(ms2 > -1)
	 switch(driver)
	 {
	   case 0:X_MS2_PIN=ms2 ; break;
	   case 1:Y_MS2_PIN=ms2 ; break;
	   case 2:Z_MS2_PIN=ms2 ; break;
	   case 3:E0_MS2_PIN=ms2 ; break;
	   case 4:E1_MS2_PIN=ms2 ; break;
	default:  break;
	 }
	   if(ms3 > -1) switch(driver)
	 {
	   case 0:X_MS3_PIN=ms3 ; break;
	   case 1:Y_MS3_PIN=ms3 ; break;
	   case 2:Z_MS3_PIN=ms3 ; break;
	   case 3:E0_MS3_PIN=ms3 ; break;
	   case 4:E1_MS3_PIN=ms3 ; break;
	default:  break;
	 }
	*/
}

void microstep_mode(uint8_t driver, uint8_t stepping_mode)
{
	switch(driver)
	{
		case 0:
			subsection_x_value = stepping_mode;
			break;
		case 1:
			subsection_y_value = stepping_mode;
			break;
		case 2:
			subsection_z_value = stepping_mode;
			break;
		case 3:
			subsection_e0_value = stepping_mode;
			break;
		case 4:
			subsection_e1_value = stepping_mode;
			break;
		default:
			break;
	}
	switch(stepping_mode)
	{
		case 1:
			microstep_ms(driver, MICROSTEP1);
			break;
		case 2:
			microstep_ms(driver, MICROSTEP2);
			break;
		case 4:
			microstep_ms(driver, MICROSTEP4);
			break;
		case 8:
			microstep_ms(driver, MICROSTEP8);
			break;
		case 16:
			microstep_ms(driver, MICROSTEP16);
			break;
		case 32:
			microstep_ms(driver, MICROSTEP32);
			break;
		case 64:
			microstep_ms(driver, MICROSTEP64);
			break;
		case 128:
			microstep_ms(driver, MICROSTEP128);
			break;
		default:
			break;
	}
}
void microstep_readings(void)
{
	USR_UsrLog("Motor_Subsection \n");
	USR_UsrLog("X: %d\n", subsection_x_value);
	USR_UsrLog("Y: %d\n", subsection_y_value);
	USR_UsrLog("Z: %d\n", subsection_z_value);
	USR_UsrLog("E0: %d\n", subsection_e0_value);
	USR_UsrLog("E1: %d\n", subsection_e1_value);
}

