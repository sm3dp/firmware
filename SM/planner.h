/*
  planner.h - buffers movement commands and manages the acceleration profile plan
  Part of Grbl

  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

// This module is to be considered a sub-module of stepper.c. Please don't include 
// this file from any other module.

#ifndef planner_h
#define planner_h

#include "sm_firmware.h"



extern volatile unsigned long minsegmenttime;
extern volatile float max_feedrate[4]; // set the max speeds
extern volatile float axis_steps_per_unit[4];
extern volatile unsigned long max_acceleration_units_per_sq_second[4]; // Use M201 to override by software
extern volatile float minimumfeedrate;
extern volatile float acceleration;         // Normal acceleration mm/s^2  THIS IS THE DEFAULT ACCELERATION for all moves. M204 SXXXX
extern volatile float retract_acceleration; //  mm/s^2   filament pull-pack and push-forward  while standing still in the other axis M204 TXXXX
extern volatile float max_xy_jerk; //speed than can be stopped at once, if i understand correctly.
extern volatile float max_z_jerk;
extern volatile float max_e_jerk;
extern volatile float mintravelfeedrate;
extern volatile unsigned long axis_steps_per_sqr_second[NUM_AXIS];

extern volatile long position[4];

#ifdef AUTOTEMP
    extern bool autotemp_enabled;
    extern float autotemp_max;
    extern float autotemp_min;
    extern float autotemp_factor;
#endif
// This struct is used when buffering the setup for each linear movement "nominal" values are as specified in 
// the source g-code and may never actually be reached if acceleration management is active.
typedef struct {
  // Fields used by the bresenham algorithm for tracing the line
  long steps_x, steps_y, steps_z, steps_e;
  unsigned long step_event_count;
  long accelerate_until;
  long decelerate_after;                   
  long acceleration_rate;                   
  unsigned char direction_bits;             
  unsigned char active_extruder;
  #ifdef ADVANCE
    long advance_rate;
    volatile long initial_advance;
    volatile long final_advance;
    float advance;
  #endif

  // Fields used by the motion planner to manage acceleration
//  float speed_x, speed_y, speed_z, speed_e;        // Nominal mm/sec for each axis
  float nominal_speed;                              
  float entry_speed;                                
  float max_entry_speed;                           
  float millimeters;                                 
  float acceleration;                                
  unsigned char recalculate_flag;                    
  unsigned char nominal_length_flag;               

  // Settings for the trapezoid generator    
  unsigned long nominal_rate;                       
  unsigned long initial_rate;                      
  unsigned long final_rate;                          
  unsigned long acceleration_st;                     
  unsigned long fan_speed;                       
  #ifdef BARICUDA
  unsigned long valve_pressure;
  unsigned long e_to_p_pressure;
  #endif
  volatile char busy;
  volatile char laser;	//激光头开关
} block_t;

void plan_init(void);
// Add a new linear movement to the buffer. x, y and z is the signed, absolute target position in 
// millimaters. Feed rate specifies the speed of the motion.
void plan_buffer_line(const float x, const float y, const float z, const float e, float feed_rate, const uint8_t extruder);

// Set position. Used for G92 instructions.
void plan_set_position(const float x, const float y, const float z, const float e);
void plan_set_e_position(const float e);

void check_axes_activity(void);
uint8_t movesplanned(void); //return the nr of buffered moves

extern volatile float axis_steps_per_unit[4];
// Called when the current block is no longer needed. Discards the block and makes the memory
// availible for new blocks.    
void plan_discard_current_block(void);
block_t *plan_get_current_block(void);
bool blocks_queued(void);
bool blocks_full(void);

void allow_cold_extrudes(bool allow);
void reset_acceleration_rates(void);

#endif
